#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include "internal/federation/federation_registry.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The FOM handle allocator test requires the Umbra source directory."
#endif

namespace {

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FomHandleAllocator;

struct RandomProviderState final {
  std::atomic<std::int64_t> successfulSamplesBeforeFailure{-1};
  std::atomic_uint64_t nextValue{1U};
  std::atomic_uint64_t failedReads{0U};
};

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0U};
  return L"umbra-fom-random-source-failure-" + std::to_wstring(++counter);
}

std::filesystem::path tckFom(std::string_view filename) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "packages" /
      "hla-rti-cpp-tck" / "fom" / filename;
}

FomHandleAllocator::RandomValueProvider controlledProvider(
    std::shared_ptr<RandomProviderState> const& state) {
  return [state] {
    auto remaining = state->successfulSamplesBeforeFailure.load();
    while (remaining >= 0) {
      if (remaining == 0) {
        ++state->failedReads;
        return std::optional<std::uint64_t>{};
      }
      if (state->successfulSamplesBeforeFailure.compare_exchange_weak(
              remaining,
              remaining - 1)) {
        break;
      }
    }
    return std::optional<std::uint64_t>{state->nextValue.fetch_add(1U)};
  };
}

}  // namespace

TEST_CASE(
    "FDD handle identities remain stable across allocator copies",
    "[unit][internal-only][no-normative-mapping][2025][fom]"
    "[fom-handle-uniqueness][fom-derived-handle-identity][repeatability]") {
  std::uint64_t nextValue = 1U;
  FomHandleAllocator allocator([&nextValue] {
    return std::optional<std::uint64_t>{nextValue++};
  });

  auto const objectClassHandle =
      allocator.allocate("object-class", "HLAobjectRoot.TckObject");
  REQUIRE(objectClassHandle == 3U);
  REQUIRE(allocator.allocate("object-class", "HLAobjectRoot.TckObject") ==
          objectClassHandle);

  FomHandleAllocator copiedAllocator = allocator;
  REQUIRE(copiedAllocator.allocate(
              "object-class", "HLAobjectRoot.TckObject") ==
          objectClassHandle);
  REQUIRE(copiedAllocator.allocate(
              "attribute", "HLAobjectRoot.TckObject", "Value") == 4U);
  REQUIRE(copiedAllocator.allocate("interaction-class", "HLAobjectRoot.TckObject") ==
          5U);
  REQUIRE(nextValue == 6U);
}

TEST_CASE(
    "FOM handle allocation reserves predefined transportation values",
    "[unit][internal-only][no-normative-mapping][2025][fom]"
    "[fom-handle-uniqueness][transportation-type]") {
  std::uint64_t nextValue = 1U;
  FomHandleAllocator allocator([&nextValue] {
    return std::optional<std::uint64_t>{nextValue++};
  });

  REQUIRE(allocator.allocate("transportation-type", "TckTransport") == 3U);
  REQUIRE(nextValue == 4U);
}

TEST_CASE(
    "System random-source failure aborts FOM reconciliation without partial federation state",
    "[unit][integration][internal-only][no-normative-mapping][2025]"
    "[fom][fom-handle-uniqueness][entropy-failure]") {
  SECTION("Create Federation Execution returns an RTI error and publishes no state") {
    auto const state = std::make_shared<RandomProviderState>();
    state->successfulSamplesBeforeFailure = 0;
    auto const registry = std::make_shared<EmbeddedFederationRegistry>(
        nullptr,
        nullptr,
        controlledProvider(state));
    public_federation_restore_test_support::ScopedEmbeddedFederationRegistry registryScope(
        registry);

    NullFederateAmbassador federate;
    auto rti = makeRti();
    auto const federation = nextFederationName();
    REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
    REQUIRE_THROWS_AS(
        rti->createFederationExecution(
            federation,
            tckFom("fom-root-classes-omitted-tck.xml").wstring(),
            L"HLAinteger64Time"),
        rti1516_2025::RTIinternalError);

    REQUIRE(state->failedReads == 1U);
    REQUIRE_FALSE(registry->contains(federation));
    REQUIRE(registry->federationExecutions().empty());
    REQUIRE_FALSE(registry->membersFor(federation));
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("Additional-FOM failure preserves the existing catalog and handles") {
    auto const state = std::make_shared<RandomProviderState>();
    auto const registry = std::make_shared<EmbeddedFederationRegistry>(
        nullptr,
        nullptr,
        controlledProvider(state));
    public_federation_restore_test_support::ScopedEmbeddedFederationRegistry registryScope(
        registry);

    NullFederateAmbassador ownerFederate;
    NullFederateAmbassador applicantFederate;
    auto owner = makeRti();
    auto applicant = makeRti();
    auto const federation = nextFederationName();
    auto const baseFom = tckFom("fom-handle-uniqueness-tck.xml").wstring();
    auto const extensionFom = tckFom("fom-handle-extension-tck.xml").wstring();

    REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
    REQUIRE_NOTHROW(applicant->connect(applicantFederate, HLA_EVOKED));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federation, baseFom, L"HLAinteger64Time"));

    auto const originalDefinition = registry->definitionFor(federation);
    auto const originalObjectHandle = registry->objectClassHandleFor(
        federation,
        "HLAobjectRoot.TckHandleObject");
    REQUIRE(originalDefinition);
    REQUIRE(originalObjectHandle);
    REQUIRE(registry->memberCount(federation) == 0U);

    auto const captureHandleSnapshot = [&] {
      return std::vector<std::optional<std::uint64_t>>{
          registry->objectClassHandleFor(
              federation,
              "HLAobjectRoot.TckHandleObject"),
          registry->attributeHandleFor(
              federation,
              "HLAobjectRoot.TckHandleObject",
              "TckHandleAttribute"),
          registry->interactionClassHandleFor(
              federation,
              "HLAinteractionRoot.TckHandleInteraction"),
          registry->parameterHandleFor(
              federation,
              "HLAinteractionRoot.TckHandleInteraction",
              "TckHandleParameter"),
          registry->dimensionHandleFor(federation, "TckHandleDimension"),
          registry->transportationTypeHandleFor(
              federation,
              "TckHandleTransportation"),
      };
    };
    auto const originalHandles = captureHandleSnapshot();
    REQUIRE(originalHandles.size() == 6U);
    for (auto const& handle : originalHandles) {
      REQUIRE(handle);
    }

    // Let one new object-class handle be allocated into the candidate copy,
    // then fail while the extension's new attribute handle is being allocated.
    state->successfulSamplesBeforeFailure = 1;
    REQUIRE_THROWS_AS(
        applicant->joinFederationExecution(
            L"entropy-failure-applicant",
            L"observer",
            federation,
            std::vector<std::wstring>{extensionFom}),
        rti1516_2025::RTIinternalError);

    auto const definitionAfterFailure = registry->definitionFor(federation);
    REQUIRE(state->failedReads == 1U);
    REQUIRE(registry->memberCount(federation) == 0U);
    REQUIRE(definitionAfterFailure);
    REQUIRE(definitionAfterFailure->fomModules.size() ==
            originalDefinition->fomModules.size());
    REQUIRE(definitionAfterFailure->catalog == originalDefinition->catalog);
    REQUIRE_FALSE(registry->objectClassHandleFor(
        federation,
        "HLAobjectRoot.TckHandleExtensionObject"));
    REQUIRE(registry->objectClassHandleFor(
                federation,
                "HLAobjectRoot.TckHandleObject") == originalObjectHandle);
    REQUIRE(captureHandleSnapshot() == originalHandles);

    // Recovery uses the same provider state and proves the failed join did
    // not publish a membership or consume/replace the existing class handle.
    state->successfulSamplesBeforeFailure = -1;
    REQUIRE_NOTHROW(applicant->joinFederationExecution(
        L"entropy-failure-applicant",
        L"observer",
        federation,
        std::vector<std::wstring>{extensionFom}));
    REQUIRE(registry->memberCount(federation) == 1U);
    REQUIRE(registry->objectClassHandleFor(
                federation,
                "HLAobjectRoot.TckHandleObject") == originalObjectHandle);
    REQUIRE(captureHandleSnapshot() == originalHandles);
    REQUIRE(registry->objectClassHandleFor(
        federation,
        "HLAobjectRoot.TckHandleExtensionObject"));

    REQUIRE_NOTHROW(applicant->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federation));
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(applicant->disconnect());
  }
}
