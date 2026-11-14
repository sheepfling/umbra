#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The FOM name-resolution test requires the Umbra source directory."
#endif

namespace {

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0};
  return L"umbra-fom-name-resolution-" + std::to_wstring(++counter);
}

std::filesystem::path rootlessFomPath() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "packages" /
      "hla-rti-cpp-tck" / "fom" / "fom-root-classes-omitted-tck.xml";
}

}  // namespace

TEST_CASE(
    "2025 FOM class names are complete, case-sensitive, and allow omitted root declarations",
    "[integration][development-profile][fom-name-resolution]"
    "[rti.service.get-object-class-handle]"
    "[rti.service.get-interaction-class-handle]"
    "[rti.service.get-attribute-handle][rti.service.get-parameter-handle]"
    "[2025]") {
  NullFederateAmbassador federate;
  auto rti = makeRti();
  auto const federation = nextFederationName();
  auto const fom = rootlessFomPath().wstring();

  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federation, fom, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"fom-name-resolution-member", L"test", federation));

  auto const objectRoot = rti->getObjectClassHandle(L"HLAobjectRoot");
  auto const objectClass =
      rti->getObjectClassHandle(L"HLAobjectRoot.TckObject");
  REQUIRE(rti->getObjectClassName(objectRoot) == L"HLAobjectRoot");
  REQUIRE(rti->getObjectClassName(objectClass) == L"HLAobjectRoot.TckObject");
  REQUIRE_THROWS_AS(
      rti->getObjectClassHandle(L"TckObject"), rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      rti->getObjectClassHandle(L"HLAobjectRoot.tckObject"),
      rti1516_2025::NameNotFound);
  auto const attribute = rti->getAttributeHandle(objectClass, L"Value");
  REQUIRE(rti->getAttributeName(objectClass, attribute) == L"Value");
  REQUIRE_THROWS_AS(
      rti->getAttributeHandle(objectClass, L"value"),
      rti1516_2025::NameNotFound);

  auto const interactionRoot =
      rti->getInteractionClassHandle(L"HLAinteractionRoot");
  auto const interactionClass =
      rti->getInteractionClassHandle(L"HLAinteractionRoot.TckInteraction");
  REQUIRE(rti->getInteractionClassName(interactionRoot) ==
          L"HLAinteractionRoot");
  REQUIRE(rti->getInteractionClassName(interactionClass) ==
          L"HLAinteractionRoot.TckInteraction");
  REQUIRE_THROWS_AS(
      rti->getInteractionClassHandle(L"TckInteraction"),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      rti->getInteractionClassHandle(L"HLAinteractionRoot.tckInteraction"),
      rti1516_2025::NameNotFound);
  auto const parameter =
      rti->getParameterHandle(interactionClass, L"Payload");
  REQUIRE(rti->getParameterName(interactionClass, parameter) == L"Payload");
  REQUIRE_THROWS_AS(
      rti->getParameterHandle(interactionClass, L"payload"),
      rti1516_2025::NameNotFound);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federation));
  REQUIRE_NOTHROW(rti->disconnect());
}
