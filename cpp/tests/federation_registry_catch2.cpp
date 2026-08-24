#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"

#include <algorithm>
#include <memory>

#include <RTI/time/HLAinteger64Time.h>

namespace {

using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FederationRegistryStatus;
using umbra::detail::FederationTimeGrantStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::InstrumentationLayer;

PrevalidatedFomModule module(std::wstring designator, std::wstring source) {
  return {
      std::move(designator),
      std::move(source),
      L"C:/fom/IEEE1516-DIF-2025.xsd",
      FomModuleKind::fom,
      L"IEEE1516-DIF-2025.xsd",
  };
}

FederationDefinition validDefinition() {
  return {
      {
          module(L"file:///fom/base.xml", L"C:/fom/base.xml"),
          module(L"file:///fom/extensions.xml", L"C:/fom/extensions.xml"),
      },
      L"HLAinteger64Time",
  };
}

}  // namespace

TEST_CASE("The embedded federation registry preserves a prevalidated definition", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;

  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(registry.contains(L"exercise"));

  auto definition = registry.definitionFor(L"exercise");
  REQUIRE(definition.has_value());
  REQUIRE(definition->logicalTimeImplementationName == L"HLAinteger64Time");
  REQUIRE(definition->fomModules.size() == 2);
  REQUIRE(definition->fomModules.front().designator == L"file:///fom/base.xml");
  REQUIRE(definition->fomModules.front().schemaDesignator == L"IEEE1516-DIF-2025.xsd");
}

TEST_CASE(
    "The embedded federation registry exposes internal operation timing",
    "[unit][kernel][federation-registry][instrumentation][foundation][federation-management]") {
  auto instrumentation = std::make_shared<umbra::detail::RuntimeInstrumentation>();
  EmbeddedFederationRegistry registry(instrumentation);

  REQUIRE(registry.create(L"instrumented", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto const snapshot = registry.runtimeInstrumentationSnapshotForTesting();
  auto const found = std::find_if(
      snapshot.operations.begin(),
      snapshot.operations.end(),
      [](umbra::detail::InstrumentationOperationSnapshot const& operation) {
        return operation.layer == InstrumentationLayer::federation_registry &&
            operation.name == "create";
      });
  REQUIRE(found != snapshot.operations.end());
  REQUIRE(found->calls == 1);
  REQUIRE(found->totalDurationNanoseconds > 0);
}

TEST_CASE("The embedded federation registry rejects missing definitions without imposing name policy", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  FederationDefinition noModules;
  FederationDefinition repeatedDesignators{
      {
          module(L"file:///fom/base.xml", L"C:/fom/base.xml"),
          module(L"file:///fom/base.xml", L"C:/fom/base.xml"),
      },
      L"",
  };

  REQUIRE(registry.create(L"", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(registry.create(L"no-modules", noModules).status == FederationRegistryStatus::invalid_request);
  REQUIRE(
      registry.create(L"repeated-module-designators", repeatedDesignators).status ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(
      registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::federation_already_exists);

  auto explicitEmptyName = registry.join(L"exercise", L"", L"");
  REQUIRE(explicitEmptyName.status == FederationRegistryStatus::applied);
  REQUIRE(explicitEmptyName.membership.has_value());
  REQUIRE(explicitEmptyName.membership->name.empty());
  REQUIRE(explicitEmptyName.membership->type.empty());
}

TEST_CASE("The embedded federation registry maintains active membership and destroy invariants", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto named = registry.join(L"exercise", L"trainer", L"alice");
  REQUIRE(named.status == FederationRegistryStatus::applied);
  REQUIRE(named.membership.has_value());
  REQUIRE(named.membership->name == L"alice");
  REQUIRE(named.membership->id != 0);

  auto generated = registry.join(L"exercise", L"observer");
  REQUIRE(generated.status == FederationRegistryStatus::applied);
  REQUIRE(generated.membership.has_value());
  REQUIRE(generated.membership->name == L"federate-2");
  REQUIRE(generated.membership->id != named.membership->id);
  REQUIRE(registry.memberCount(L"exercise") == 2);

  auto namedByName = registry.memberByName(L"exercise", L"alice");
  REQUIRE(namedByName.has_value());
  REQUIRE(namedByName->id == named.membership->id);
  auto generatedById = registry.memberById(L"exercise", generated.membership->id);
  REQUIRE(generatedById.has_value());
  REQUIRE(generatedById->name == generated.membership->name);
  REQUIRE_FALSE(registry.memberByName(L"exercise", L"missing").has_value());
  REQUIRE_FALSE(registry.memberById(L"missing", named.membership->id).has_value());

  REQUIRE(
      registry.join(L"exercise", L"trainer", L"alice").status ==
      FederationRegistryStatus::federate_name_already_in_use);
  REQUIRE(
      registry.destroy(L"exercise").status == FederationRegistryStatus::federates_currently_joined);

  REQUIRE(
      registry.resign(L"exercise", named.membership->id).status == FederationRegistryStatus::applied);
  REQUIRE_FALSE(registry.memberByName(L"exercise", L"alice").has_value());
  REQUIRE_FALSE(registry.memberById(L"exercise", named.membership->id).has_value());
  REQUIRE(
      registry.resign(L"exercise", generated.membership->id).status == FederationRegistryStatus::applied);
  REQUIRE(registry.memberCount(L"exercise") == 0);
  REQUIRE(registry.destroy(L"exercise").status == FederationRegistryStatus::applied);
  REQUIRE_FALSE(registry.contains(L"exercise"));
}

TEST_CASE("The embedded federation registry makes generated names unique despite user lookalikes", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto lookalike = registry.join(L"exercise", L"trainer", L"federate-1");
  REQUIRE(lookalike.status == FederationRegistryStatus::applied);
  auto generated = registry.join(L"exercise", L"observer");
  REQUIRE(generated.status == FederationRegistryStatus::applied);
  REQUIRE(generated.membership.has_value());
  REQUIRE(generated.membership->name == L"federate-2");
  REQUIRE(generated.membership->id == 2);
}

TEST_CASE("The embedded federation registry commits an additional-module definition with membership", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  auto original = validDefinition();
  auto replacement = validDefinition();
  replacement.logicalTimeImplementationName = L"HLAfloat64Time";

  REQUIRE(registry.create(L"exercise", original).status == FederationRegistryStatus::applied);
  auto joined = registry.joinWithDefinition(L"exercise", replacement, L"observer", L"bob");
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership.has_value());
  REQUIRE(registry.memberCount(L"exercise") == 1);
  auto afterJoin = registry.definitionFor(L"exercise");
  REQUIRE(afterJoin.has_value());
  REQUIRE(afterJoin->logicalTimeImplementationName == L"HLAfloat64Time");

  auto incompatibleReplacement = validDefinition();
  incompatibleReplacement.logicalTimeImplementationName = L"HLAinteger64Time";
  auto rejected = registry.joinWithDefinition(
      L"exercise",
      incompatibleReplacement,
      L"observer",
      L"carol");
  REQUIRE(rejected.status == FederationRegistryStatus::invalid_request);
  REQUIRE(registry.memberCount(L"exercise") == 1);
  auto afterRejectedJoin = registry.definitionFor(L"exercise");
  REQUIRE(afterRejectedJoin.has_value());
  REQUIRE(afterRejectedJoin->logicalTimeImplementationName == L"HLAfloat64Time");

  auto duplicate = registry.joinWithDefinition(
      L"exercise",
      replacement,
      L"observer",
      L"bob");
  REQUIRE(duplicate.status == FederationRegistryStatus::federate_name_already_in_use);
}

TEST_CASE("The embedded federation registry reports missing federation and membership distinctly", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;

  REQUIRE(
      registry.join(L"missing", L"trainer").status ==
      FederationRegistryStatus::federation_does_not_exist);
  REQUIRE(
      registry.destroy(L"missing").status == FederationRegistryStatus::federation_does_not_exist);

  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(
      registry.resign(L"exercise", 42).status == FederationRegistryStatus::federate_not_member);
}

TEST_CASE(
    "The registry commits runtime time state with its federate membership",
    "[unit][kernel][federation-registry][time-management]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto timeState = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto joined = registry.joinWithTimeState(L"exercise", timeState, L"trainer", L"alice");
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership.has_value());

  auto advance = timeState->requestAdvance(std::make_shared<rti1516_2025::HLAinteger64Time>(3));
  REQUIRE(advance.generation != 0);
  timeState.reset();

  auto snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->definition.logicalTimeImplementationName == L"HLAinteger64Time");
  REQUIRE(snapshot->federates.size() == 1);
  REQUIRE(snapshot->federates.front().membership.id == joined.membership->id);
  REQUIRE(snapshot->federates.front().membership.name == L"alice");
  REQUIRE(snapshot->federates.front().time.timeAdvancePending);
  REQUIRE(snapshot->federates.front().time.requestedTime);

  REQUIRE(
      registry.resign(L"exercise", joined.membership->id).status ==
      FederationRegistryStatus::applied);
  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates.empty());
}

TEST_CASE(
    "The registry projects private TSO coordination into its time snapshot",
    "[unit][kernel][federation-registry][time-management][tso][lits]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto receiverTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto observerTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto receiver = registry.joinWithTimeState(L"exercise", receiverTime, L"receiver", L"receiver");
  auto observer = registry.joinWithTimeState(L"exercise", observerTime, L"observer", L"observer");
  REQUIRE(receiver.status == FederationRegistryStatus::applied);
  REQUIRE(observer.status == FederationRegistryStatus::applied);
  REQUIRE(receiver.membership);
  REQUIRE(observer.membership);

  auto const messageId = registry.allocateTsoMessageId(L"exercise");
  REQUIRE(messageId.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(messageId.messageId != 0);
  auto const timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  REQUIRE(
      registry.enqueueTsoMessage(
          L"exercise",
          messageId.messageId,
          receiver.membership->id,
          timestamp)
          .queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(
      registry.enqueueTsoMessage(
          L"exercise",
          messageId.messageId,
          observer.membership->id,
          timestamp)
          .queueStatus == umbra::detail::TsoMessageQueueStatus::applied);

  auto snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates.size() == 2);
  REQUIRE(snapshot->federates[0].queuedTsoMessages.size() == 1);
  REQUIRE(snapshot->federates[0].inTransitTsoMessages.empty());
  REQUIRE(snapshot->federates[0].deliveredTsoMessagesSinceLastAdvance.empty());

  auto delivery = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(delivery.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(delivery.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivery.delivery.messages.size() == 1);

  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates[0].queuedTsoMessages.empty());
  REQUIRE(snapshot->federates[0].inTransitTsoMessages.size() == 1);
  REQUIRE(snapshot->federates[1].queuedTsoMessages.size() == 1);

  auto completed = registry.completeTsoDelivery(L"exercise", delivery.delivery.messages.front());
  REQUIRE(completed.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(completed.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates[0].inTransitTsoMessages.empty());
  REQUIRE(snapshot->federates[0].deliveredTsoMessagesSinceLastAdvance.size() == 1);

  auto const retraction = registry.retractTsoMessage(L"exercise", messageId.messageId);
  REQUIRE(retraction.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retraction.queueResult.status == umbra::detail::TsoMessageQueueStatus::message_already_delivered);

  REQUIRE(
      registry.resign(L"exercise", receiver.membership->id).status ==
      FederationRegistryStatus::applied);
  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates.size() == 1);
  REQUIRE(snapshot->federates.front().membership.id == observer.membership->id);
  REQUIRE(snapshot->federates.front().queuedTsoMessages.size() == 1);
}

TEST_CASE(
    "The registry schedules a newly eligible constrained TAR after a regulator advances",
    "[unit][kernel][federation-registry][time-management][galt][time-advance-grant]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto receiverTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto regulatorTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto receiver = registry.joinWithTimeState(L"exercise", receiverTime, L"receiver", L"receiver");
  auto regulator = registry.joinWithTimeState(L"exercise", regulatorTime, L"regulator", L"regulator");
  REQUIRE(receiver.membership);
  REQUIRE(regulator.membership);

  auto constrained = receiverTime->requestTimeConstrained();
  REQUIRE(constrained.generation != 0);
  REQUIRE(receiverTime->grantTimeConstrained(constrained.generation));
  auto regulation = regulatorTime->requestTimeRegulation(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(2));
  REQUIRE(regulation.generation != 0);
  REQUIRE(regulatorTime->grantTimeRegulation(regulation.generation));

  auto receiverAdvance = receiverTime->requestAdvance(
      std::make_shared<rti1516_2025::HLAinteger64Time>(2));
  REQUIRE(receiverAdvance.generation != 0);
  std::size_t receiverDispatches = 0;
  auto waiting = registry.requestTimeAdvanceGrant(
      L"exercise",
      receiver.membership->id,
      receiverAdvance.generation,
      [&receiverDispatches] { ++receiverDispatches; });
  REQUIRE(waiting.status == FederationTimeGrantStatus::applied);
  REQUIRE(waiting.dispatches.empty());

  // The regulator's pending target 1 plus lookahead 2 raises the receiver's
  // GALT from 2 to 3, making the receiver's TAR(2) strictly below the bound
  // even before the regulator receives its own grant.
  auto regulatorAdvance = regulatorTime->requestAdvance(
      std::make_shared<rti1516_2025::HLAinteger64Time>(1));
  REQUIRE(regulatorAdvance.generation != 0);
  std::size_t regulatorDispatches = 0;
  auto eligible = registry.requestTimeAdvanceGrant(
      L"exercise",
      regulator.membership->id,
      regulatorAdvance.generation,
      [&regulatorDispatches] { ++regulatorDispatches; });
  REQUIRE(eligible.status == FederationTimeGrantStatus::applied);
  REQUIRE(eligible.dispatches.size() == 2);
  for (auto& dispatch : eligible.dispatches) {
    dispatch();
  }
  REQUIRE(receiverDispatches == 1);
  REQUIRE(regulatorDispatches == 1);

  REQUIRE(
      registry.beginTimeAdvanceGrant(
          L"exercise",
          receiver.membership->id,
          receiverAdvance.generation) == FederationTimeGrantStatus::applied);
  REQUIRE(receiverTime->grant(receiverAdvance.generation));
  REQUIRE(
      registry.beginTimeAdvanceGrant(
          L"exercise",
          regulator.membership->id,
          regulatorAdvance.generation) == FederationTimeGrantStatus::applied);
  REQUIRE(regulatorTime->grant(regulatorAdvance.generation));

  auto receiverSnapshot = receiverTime->snapshot();
  auto regulatorSnapshot = regulatorTime->snapshot();
  REQUIRE_FALSE(receiverSnapshot.timeAdvancePending);
  REQUIRE_FALSE(regulatorSnapshot.timeAdvancePending);
}
