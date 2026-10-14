#include "tck_joined_federate_mom_reflections_received_counts.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomReflectionsReceivedCounts(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM reflections-received testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM reflections-received testing requires an adapter-supplied standard MIM");

  Session receiver(options, model, "owner");
  Session sender(options, model, "member");
  Session requester(options, model, "member");
  auto const federation =
      federationName(options, "joined-federate-mom-reflections-received-counts");
  receiver.connect();
  sender.connect();
  requester.connect();
  if (options.logicalTimeImplementationName.empty()) {
    receiver.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    receiver.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }
  receiver.join(
      options.ownerFederateName + L"-mom-reflections-received-receiver",
      options.federateType,
      federation);
  sender.join(
      options.memberFederateName + L"-mom-reflections-received-sender",
      options.federateType,
      federation);
  requester.join(
      options.memberFederateName + L"-mom-reflections-received-requester",
      options.federateType,
      federation);

  auto const senderObjectClass = sender.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const senderAttribute = sender.rtiAmbassador().getAttributeHandle(
      senderObjectClass,
      options.attributeName);
  auto const receiverObjectClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverObjectClass,
      options.attributeName);
  require(
      senderObjectClass.isValid() &&
          sender.rtiAmbassador().getObjectClassName(senderObjectClass) ==
              options.objectClassName &&
          senderAttribute.isValid() &&
          sender.rtiAmbassador().getAttributeName(
              senderObjectClass,
              senderAttribute) == options.attributeName,
      "Joined-federate MOM reflections-received sender lookup did not round-trip");
  require(
      receiverObjectClass.isValid() &&
          receiver.rtiAmbassador().getObjectClassName(receiverObjectClass) ==
              options.objectClassName &&
          receiverAttribute.isValid() &&
          receiver.rtiAmbassador().getAttributeName(
              receiverObjectClass,
              receiverAttribute) == options.attributeName,
      "Joined-federate MOM reflections-received receiver lookup did not round-trip");

  auto const reliable = sender.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffort =
      sender.rtiAmbassador().getTransportationTypeHandle(L"HLAbestEffort");
  require(
      reliable.isValid() && bestEffort.isValid() &&
          sender.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable" &&
          sender.rtiAmbassador().getTransportationTypeName(bestEffort) == L"HLAbestEffort",
      "Joined-federate MOM reflections-received lookup could not resolve standard transports");

  sender.rtiAmbassador().publishObjectClassAttributes(
      senderObjectClass,
      rti::AttributeHandleSet{senderAttribute});
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverObjectClass,
      rti::AttributeHandleSet{receiverAttribute},
      true,
      L"");
  auto const object = sender.rtiAmbassador().registerObjectInstance(senderObjectClass);
  require(
      object.isValid(),
      "Joined-federate MOM reflections-received registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "joined-federate MOM reflections-received object discovery");
  receiver.recorder().clearReflection();

  auto updateAttribute = [&](std::uint8_t value) {
    std::vector<std::uint8_t> bytes{value};
    sender.rtiAmbassador().updateAttributeValues(
        object,
        rti::AttributeHandleValueMap{{
            senderAttribute,
            rti::VariableLengthData(bytes.data(), bytes.size())}},
        rti::VariableLengthData{});
  };
  // The first update uses the standard reliable default. The per-instance
  // request after that first callback makes the next two updates best effort.
  updateAttribute(0x71U);
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflectionCount() >= 1U; },
      options,
      "joined-federate MOM reflections-received reliable delivery");

  auto const confirmationBefore =
      sender.recorder().attributeTransportationConfirmations().size();
  sender.rtiAmbassador().requestAttributeTransportationTypeChange(
      object,
      rti::AttributeHandleSet{senderAttribute},
      bestEffort);
  waitFor(
      sender,
      [&] {
        return sender.recorder().attributeTransportationConfirmations().size() >
            confirmationBefore;
      },
      options,
      "joined-federate MOM reflections-received transportation change");
  auto const transportationConfirmations =
      sender.recorder().attributeTransportationConfirmations();
  require(
      transportationConfirmations.back().object == object &&
          transportationConfirmations.back().attributes ==
              rti::AttributeHandleSet{senderAttribute} &&
          transportationConfirmations.back().transportation == bestEffort,
      "Joined-federate MOM reflections-received transportation change did not commit best effort");

  updateAttribute(0x72U);
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflectionCount() >= 2U; },
      options,
      "joined-federate MOM reflections-received first best-effort delivery");
  updateAttribute(0x73U);
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflectionCount() >= 3U; },
      options,
      "joined-federate MOM reflections-received second best-effort delivery");

  auto const reflections = receiver.recorder().reflections();
  require(
      reflections.size() == 3U,
      "Joined-federate MOM reflections-received receiver observed the wrong delivery count");
  std::size_t reliableReflections = 0U;
  std::size_t bestEffortReflections = 0U;
  for (auto const& reflection : reflections) {
    require(
        reflection.present && reflection.object == object &&
            reflection.values.size() == 1U &&
            reflection.values.count(receiverAttribute) == 1U &&
            reflection.tag.empty() && reflection.producer == sender.federateHandle() &&
            !reflection.regions.has_value(),
        "Joined-federate MOM reflections-received delivery returned non-standard metadata");
    auto const value = copyBytes(reflection.values.at(receiverAttribute));
    require(
        value.size() == 1U &&
            (value.front() == 0x71U || value.front() == 0x72U || value.front() == 0x73U),
        "Joined-federate MOM reflections-received delivery returned the wrong payload");
    if (reflection.transportation == reliable) {
      ++reliableReflections;
    } else if (reflection.transportation == bestEffort) {
      ++bestEffortReflections;
    } else {
      require(
          false,
          "Joined-federate MOM reflections-received delivery returned an unknown transport");
    }
  }
  require(
      reliableReflections == 1U && bestEffortReflections == 2U,
      "Joined-federate MOM reflections-received delivery returned wrong transport counts");

  auto const reportClass = requester.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportReflectionsReceived");
  auto const requestClass = requester.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestReflectionsReceived");
  auto const requestFederate = requester.rtiAmbassador().getParameterHandle(
      requestClass,
      L"HLAfederate");
  auto const reportTransportation = requester.rtiAmbassador().getParameterHandle(
      reportClass,
      L"HLAtransportation");
  auto const reportCounts = requester.rtiAmbassador().getParameterHandle(
      reportClass,
      L"HLAreflectCounts");
  require(
      reportClass.isValid() && requestClass.isValid() && requestFederate.isValid() &&
          reportTransportation.isValid() && reportCounts.isValid() &&
          requester.rtiAmbassador().getInteractionClassName(reportClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportReflectionsReceived" &&
          requester.rtiAmbassador().getInteractionClassName(requestClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestReflectionsReceived" &&
          requester.rtiAmbassador().getParameterName(requestClass, requestFederate) ==
              L"HLAfederate" &&
          requester.rtiAmbassador().getParameterName(reportClass, reportTransportation) ==
              L"HLAtransportation" &&
          requester.rtiAmbassador().getParameterName(reportClass, reportCounts) ==
              L"HLAreflectCounts",
      "Joined-federate MOM reflections-received lookup did not round-trip the standard report surface");
  requester.rtiAmbassador().subscribeInteractionClass(reportClass, true);

  auto const expectedClassEncoding = copyBytes(receiverObjectClass.encode());
  auto formatBytes = [](std::vector<std::uint8_t> const& bytes) {
    std::ostringstream stream;
    stream << std::hex;
    for (auto const byte : bytes) {
      if (byte < 0x10U) {
        stream << '0';
      }
      stream << static_cast<unsigned int>(byte);
    }
    return stream.str();
  };
  auto decodePositiveCount = [&](rti::VariableLengthData const& encoded,
                                 std::int32_t expectedCount,
                                 std::string const& description) {
    rti::HLAfixedRecord countPrototype;
    countPrototype.appendElement(rti::HLAvariableArray{rti::HLAbyte{}})
        .appendElement(rti::HLAinteger32BE{});
    rti::HLAvariableArray counts{countPrototype};
    counts.decode(encoded);
    require(
        counts.size() == 1U,
        description + " returned the wrong number of positive object-class counts");
    auto const& count = dynamic_cast<rti::HLAfixedRecord const&>(counts.get(0U));
    auto const& encodedClass = dynamic_cast<rti::HLAvariableArray const&>(count.get(0U));
    auto const actualClassEncoding = copyBytes(encodedClass.encode());
    require(
        actualClassEncoding == expectedClassEncoding,
        description + " returned the wrong object class (expected " +
            formatBytes(expectedClassEncoding) + ", got " +
            formatBytes(actualClassEncoding) + ")");
    auto const countValue = dynamic_cast<rti::HLAinteger32BE const&>(count.get(1U)).get();
    require(
        countValue == expectedCount,
        description + " returned the wrong reflection count");
  };
  auto verifyEmptyCount = [&](rti::VariableLengthData const& encoded,
                              std::string const& description) {
    if (encoded.size() == 0U) {
      return;
    }
    rti::HLAfixedRecord countPrototype;
    countPrototype.appendElement(rti::HLAvariableArray{rti::HLAbyte{}})
        .appendElement(rti::HLAinteger32BE{});
    rti::HLAvariableArray counts{countPrototype};
    counts.decode(encoded);
    require(
        counts.size() == 0U,
        description + " encoded a non-empty reflection count list for a NULL response");
  };

  auto verifyReports = [&](std::vector<InteractionRecord> const& reports,
                           std::string const& description,
                           bool expectReflections) {
    require(
        reports.size() == 2U,
        description + " did not return exactly one report for each standard transport");
    bool sawReliable = false;
    bool sawBestEffort = false;
    for (auto const& report : reports) {
      require(
          report.present && report.interaction == reportClass && report.parameters.size() == 2U &&
              report.parameters.count(reportTransportation) == 1U &&
              report.parameters.count(reportCounts) == 1U && report.tag.empty() &&
              report.transportation == reliable && !report.producer.isValid() &&
              !report.regions.has_value(),
          description + " returned non-standard report metadata");
      auto const transportBytes = copyBytes(report.parameters.at(reportTransportation));
      if (transportBytes == copyBytes(reliable.encode())) {
        require(!sawReliable, description + " returned duplicate reliable reports");
        sawReliable = true;
        if (expectReflections) {
          decodePositiveCount(
              report.parameters.at(reportCounts),
              1,
              description + " reliable count");
        } else {
          verifyEmptyCount(
              report.parameters.at(reportCounts),
              description + " reliable NULL count");
        }
      } else if (transportBytes == copyBytes(bestEffort.encode())) {
        require(!sawBestEffort, description + " returned duplicate best-effort reports");
        sawBestEffort = true;
        if (expectReflections) {
          decodePositiveCount(
              report.parameters.at(reportCounts),
              2,
              description + " best-effort count");
        } else {
          verifyEmptyCount(
              report.parameters.at(reportCounts),
              description + " best-effort NULL count");
        }
      } else {
        require(false, description + " returned an unknown transportation value");
      }
    }
    require(
        sawReliable && sawBestEffort,
        description + " omitted a standard transportation bucket");
  };

  auto requestReports = [&](rti::FederateHandle const& subject,
                            std::string const& description) {
    requester.recorder().clearInteraction();
    requester.rtiAmbassador().sendInteraction(
        requestClass,
        rti::ParameterHandleValueMap{{requestFederate, subject.encode()}},
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] { return requester.recorder().interactions().size() >= 2U; },
        options,
        description);
    return requester.recorder().interactions();
  };

  auto const receiverReports = requestReports(
      receiver.federateHandle(),
      "joined-federate MOM reflections-received receiver reports");
  verifyReports(receiverReports, "joined-federate MOM reflections-received receiver", true);
  auto const requesterReports = requestReports(
      requester.federateHandle(),
      "joined-federate MOM reflections-received empty reports");
  verifyReports(requesterReports, "joined-federate MOM reflections-received empty", false);

  requester.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverObjectClass,
      rti::AttributeHandleSet{receiverAttribute});
  sender.rtiAmbassador().unpublishObjectClassAttributes(
      senderObjectClass,
      rti::AttributeHandleSet{senderAttribute});
  requester.resign(rti::NO_ACTION);
  sender.resign(rti::DELETE_OBJECTS);
  receiver.resign(rti::NO_ACTION);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  sender.disconnect();
  receiver.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
