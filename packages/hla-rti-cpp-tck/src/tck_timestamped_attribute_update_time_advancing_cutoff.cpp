#include "tck_main_scenarios.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioTimestampedAttributeUpdateTimeAdvancingCutoff(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "timestamped attribute advancing cutoff requires an adapter-selected logical-time implementation");

  Session sender(options, model, "timestamp-advancing-attribute-sender");
  Session peer(options, model, "timestamp-advancing-attribute-peer");
  auto const federation = federationName(
      options,
      "timestamped-attribute-update-time-advancing-cutoff");
  connectAndJoin(sender, peer, options, federation, options.fom);

  auto& senderAmbassador = sender.rtiAmbassador();
  auto const objectClass = senderAmbassador.getObjectClassHandle(options.objectClassName);
  auto const attribute = senderAmbassador.getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "timestamped attribute advancing-cutoff FOM lookup returned an invalid handle");

  rti::AttributeHandleSet const attributes{attribute};
  senderAmbassador.publishObjectClassAttributes(objectClass, attributes);
  senderAmbassador.changeDefaultAttributeOrderType(
      objectClass,
      attributes,
      rti::TIMESTAMP);
  auto const object = senderAmbassador.registerObjectInstance(objectClass);
  require(
      object.isValid(),
      "timestamped attribute advancing-cutoff registration returned an invalid handle");

  auto const senderTime = makeTimeContext(sender);
  auto const peerTime = makeTimeContext(peer);
  require(
      senderTime.factory->getName() == peerTime.factory->getName() &&
          encodeTime(*senderTime.initial) == encodeTime(*peerTime.initial) &&
          senderTime.epsilon->isEpsilon() && peerTime.epsilon->isEpsilon(),
      "timestamped attribute advancing-cutoff members selected incompatible logical times");

  auto senderLookahead = senderTime.factory->makeZero();
  auto peerLookahead = peerTime.factory->makeZero();
  require(
      senderLookahead != nullptr && peerLookahead != nullptr,
      "logical-time factory returned no zero lookahead");
  for (std::size_t step = 0U; step != 5U; ++step) {
    *senderLookahead += *senderTime.epsilon;
    *peerLookahead += *peerTime.epsilon;
  }

  senderAmbassador.enableTimeConstrained();
  waitFor(
      sender,
      [&] { return sender.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "timestamped attribute advancing-cutoff Time Constrained callback");
  senderAmbassador.enableTimeRegulation(*senderLookahead);
  waitFor(
      sender,
      [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "timestamped attribute advancing-cutoff sender Time Regulation callback");
  peer.rtiAmbassador().enableTimeRegulation(*peerLookahead);
  waitFor(
      peer,
      [&] { return peer.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "timestamped attribute advancing-cutoff peer Time Regulation callback");

  auto const requestedTime = timeAfter(
      *senderTime.factory,
      *senderTime.initial,
      *senderTime.epsilon,
      10U);
  auto const belowBound = timeAfter(
      *senderTime.factory,
      *senderTime.initial,
      *senderTime.epsilon,
      14U);
  auto const equalBound = timeAfter(
      *senderTime.factory,
      *senderTime.initial,
      *senderTime.epsilon,
      15U);
  auto const aboveBound = timeAfter(
      *senderTime.factory,
      *senderTime.initial,
      *senderTime.epsilon,
      16U);
  auto const peerRequestedTime = timeAfter(
      *peerTime.factory,
      *peerTime.initial,
      *peerTime.epsilon,
      10U);
  require(
      encodeTime(*requestedTime) == encodeTime(*peerRequestedTime),
      "timestamped attribute advancing-cutoff members computed different TAR(10) values");

  senderAmbassador.timeAdvanceRequest(*requestedTime);
  auto const pendingDeadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(250);
  do {
    sender.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  } while (std::chrono::steady_clock::now() < pendingDeadline);
  require(
      sender.recorder().timeAdvanceGrants().empty(),
      "the sender's TAR(10) was not held pending by the peer regulator");

  std::vector<std::uint8_t> const valueBytes{0x42U};
  rti::AttributeHandleValueMap const values{{
      attribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x41U, 0x52U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  // During TAR(10), lookahead 5 makes timestamps below 15 invalid. The
  // equality and later timestamps are positive controls for the strict bound.
  requireException(
      [&] {
        senderAmbassador.updateAttributeValues(
            object,
            values,
            tag,
            *belowBound);
      },
      L"InvalidLogicalTime",
      "timestamped attribute update below requested TAR time plus lookahead");
  static_cast<void>(senderAmbassador.updateAttributeValues(
      object,
      values,
      tag,
      *equalBound));
  static_cast<void>(senderAmbassador.updateAttributeValues(
      object,
      values,
      tag,
      *aboveBound));

  peer.rtiAmbassador().timeAdvanceRequest(*peerRequestedTime);
  waitFor(
      sender,
      [&] { return sender.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "timestamped attribute advancing-cutoff sender TAR(10) grant");
  waitFor(
      peer,
      [&] { return peer.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "timestamped attribute advancing-cutoff peer TAR(10) grant");
  auto const senderGrants = sender.recorder().timeAdvanceGrants();
  auto const peerGrants = peer.recorder().timeAdvanceGrants();
  require(
      senderGrants.front().encoded == encodeTime(*requestedTime) &&
          peerGrants.front().encoded == encodeTime(*peerRequestedTime),
      "timestamped attribute advancing-cutoff did not grant both federates at TAR(10)");

  sender.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  peer.resign(rti::NO_ACTION);
  senderAmbassador.destroyFederationExecution(federation);
  peer.disconnect();
  sender.disconnect();
}

void scenarioTimestampedAttributeUpdateTimeAdvancingCutoffContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateTimeAdvancingCutoff(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
