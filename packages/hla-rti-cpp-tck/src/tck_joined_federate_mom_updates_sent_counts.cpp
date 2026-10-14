#include "tck_joined_federate_mom_updates_sent_counts.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomUpdatesSentCounts(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM updates-sent testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM updates-sent testing requires an adapter-supplied standard MIM");

  Session target(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(options, "joined-federate-mom-updates-sent-counts");
  target.connect();
  requester.connect();
  if (options.logicalTimeImplementationName.empty()) {
    target.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    target.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }
  target.join(
      options.ownerFederateName + L"-mom-updates-sent-target",
      options.federateType,
      federation);
  requester.join(
      options.memberFederateName + L"-mom-updates-sent-requester",
      options.federateType,
      federation);

  auto const targetClass = target.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const targetReliable = target.rtiAmbassador().getAttributeHandle(
      targetClass,
      options.attributeName);
  require(
      targetClass.isValid() &&
          target.rtiAmbassador().getObjectClassName(targetClass) ==
              options.objectClassName,
      "Joined-federate MOM updates-sent object-class lookup did not round-trip");
  require(
      targetReliable.isValid() &&
          target.rtiAmbassador().getAttributeName(targetClass, targetReliable) ==
              options.attributeName,
      "Joined-federate MOM updates-sent attribute lookup did not round-trip");

  auto const reliable = target.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffort = target.rtiAmbassador().getTransportationTypeHandle(L"HLAbestEffort");
  require(
      reliable.isValid() && bestEffort.isValid() &&
          target.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable" &&
          target.rtiAmbassador().getTransportationTypeName(bestEffort) == L"HLAbestEffort",
      "Joined-federate MOM updates-sent lookup could not resolve standard transports");
  target.rtiAmbassador().publishObjectClassAttributes(
      targetClass,
      rti::AttributeHandleSet{targetReliable});

  auto const object = target.rtiAmbassador().registerObjectInstance(targetClass);
  require(
      object.isValid(),
      "Joined-federate MOM updates-sent registration returned an invalid object handle");
  auto sendValue = [&](rti::AttributeHandle const& attribute, std::uint8_t value) {
    std::vector<std::uint8_t> bytes{value};
    rti::AttributeHandleValueMap values;
    values.emplace(
        attribute,
        rti::VariableLengthData(bytes.data(), bytes.size()));
    target.rtiAmbassador().updateAttributeValues(
        object,
        values,
        rti::VariableLengthData{});
  };
  sendValue(targetReliable, 0x51U);
  target.rtiAmbassador().requestAttributeTransportationTypeChange(
      object,
      rti::AttributeHandleSet{targetReliable},
      bestEffort);
  waitFor(
      target,
      [&] {
        auto const confirmations = target.recorder().attributeTransportationConfirmations();
        return !confirmations.empty();
      },
      options,
      "joined-federate MOM updates-sent transportation change");
  auto const transportationConfirmations =
      target.recorder().attributeTransportationConfirmations();
  require(
      transportationConfirmations.back().object == object &&
          transportationConfirmations.back().attributes.count(targetReliable) == 1U &&
          transportationConfirmations.back().transportation == bestEffort,
      "joined-federate MOM updates-sent transportation change did not commit best effort");
  sendValue(targetReliable, 0x52U);
  sendValue(targetReliable, 0x53U);

  auto const reportClass = requester.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportUpdatesSent");
  auto const requestClass = requester.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestUpdatesSent");
  auto const requestFederate = requester.rtiAmbassador().getParameterHandle(
      requestClass,
      L"HLAfederate");
  auto const reportTransportation = requester.rtiAmbassador().getParameterHandle(
      reportClass,
      L"HLAtransportation");
  auto const reportCounts = requester.rtiAmbassador().getParameterHandle(
      reportClass,
      L"HLAupdateCounts");
  require(
      reportClass.isValid() && requestClass.isValid() && requestFederate.isValid() &&
          reportTransportation.isValid() && reportCounts.isValid() &&
          requester.rtiAmbassador().getInteractionClassName(reportClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportUpdatesSent" &&
          requester.rtiAmbassador().getInteractionClassName(requestClass) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestUpdatesSent" &&
          requester.rtiAmbassador().getParameterName(requestClass, requestFederate) ==
              L"HLAfederate" &&
          requester.rtiAmbassador().getParameterName(reportClass, reportTransportation) ==
              L"HLAtransportation" &&
          requester.rtiAmbassador().getParameterName(reportClass, reportCounts) ==
              L"HLAupdateCounts",
      "Joined-federate MOM updates-sent lookup did not round-trip the standard report surface");
  requester.rtiAmbassador().subscribeInteractionClass(reportClass, true);

  auto const expectedClassEncoding = copyBytes(targetClass.encode());
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
                                 std::string const& description) {
    rti::HLAfixedRecord countPrototype;
    countPrototype.appendElement(rti::HLAvariableArray{rti::HLAbyte{}})
        .appendElement(rti::HLAinteger32BE{});
    rti::HLAvariableArray counts{countPrototype};
    counts.decode(encoded);
    require(
        counts.size() == 1U,
        description + " returned the wrong number of positive class counts");
    auto const& count = dynamic_cast<rti::HLAfixedRecord const&>(counts.get(0U));
    auto const& encodedClass = dynamic_cast<rti::HLAvariableArray const&>(count.get(0U));
    auto const countValue = dynamic_cast<rti::HLAinteger32BE const&>(count.get(1U)).get();
    auto const actualClassEncoding = copyBytes(encodedClass.encode());
    require(
        actualClassEncoding == expectedClassEncoding,
        description + " returned the wrong registered object class (expected " +
            formatBytes(expectedClassEncoding) + ", got " +
            formatBytes(actualClassEncoding) + ")");
    require(
        countValue > 0,
        description + " returned a non-positive class count");
    return countValue;
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
        description + " encoded a non-empty class count list for a NULL response");
  };

  auto verifyReports = [&](std::vector<InteractionRecord> const& reports,
                           std::string const& description,
                           bool expectUpdates) {
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
        if (expectUpdates) {
          require(
              decodePositiveCount(
                  report.parameters.at(reportCounts),
                  description + " reliable count") == 1,
              description + " returned the wrong reliable update count");
        } else {
          verifyEmptyCount(
              report.parameters.at(reportCounts),
              description + " reliable NULL count");
        }
      } else if (transportBytes == copyBytes(bestEffort.encode())) {
        require(!sawBestEffort, description + " returned duplicate best-effort reports");
        sawBestEffort = true;
        if (expectUpdates) {
          require(
              decodePositiveCount(
                  report.parameters.at(reportCounts),
                  description + " best-effort count") == 2,
              description + " returned the wrong best-effort update count");
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

  auto const targetReports = requestReports(
      target.federateHandle(),
      "joined-federate MOM updates-sent target reports");
  verifyReports(targetReports, "joined-federate MOM updates-sent target", true);
  auto const requesterReports = requestReports(
      requester.federateHandle(),
      "joined-federate MOM updates-sent empty reports");
  verifyReports(requesterReports, "joined-federate MOM updates-sent empty", false);

  requester.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  target.rtiAmbassador().unpublishObjectClassAttributes(
      targetClass,
      rti::AttributeHandleSet{targetReliable});
  requester.resign(rti::NO_ACTION);
  target.resign(rti::NO_ACTION);
  target.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  target.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
