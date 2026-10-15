#include "tck_federation_synchronization_mom_queries.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFederationSynchronizationMomQueries(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Federation synchronization MOM queries require an adapter-supplied standard MIM");

  Session subject(options, model, "federation-synchronization-mom-subject");
  Session observer(options, model, "federation-synchronization-mom-observer");
  auto const federation = federationName(options, "federation-synchronization-mom-queries");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-federation-synchronization-mom-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-federation-synchronization-mom-observer",
      options.federateType,
      federation);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);

  std::vector<std::uint8_t> tagBytes{0x4DU, 0x4FU, 0x4DU, 0x53U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  rti::VariableLengthData const emptyTag;
  std::wstring const label = L"mom-synchronization-query";
  subject.rtiAmbassador().registerFederationSynchronizationPoint(label, tag);
  waitFor(
      subject,
      observer,
      [&] {
        return subject.synchronizationPointRegistrations().size() == 1U &&
            observer.synchronizationPointAnnouncements().size() == 1U;
      },
      options,
      "MOM synchronization-point registration and announcement");
  auto const registration = subject.synchronizationPointRegistrations().front();
  require(
      registration.succeeded && registration.label == label,
      "MOM synchronization query setup returned the wrong registration result");
  auto const announcement = observer.synchronizationPointAnnouncements().front();
  require(
      announcement.label == label && announcement.tag == tagBytes,
      "MOM synchronization query setup returned the wrong announcement");

  auto const pointsRequestClass = subject.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPoints");
  auto const pointsReportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPoints");
  auto const pointsReportParameter = observer.rtiAmbassador().getParameterHandle(
      pointsReportClass,
      L"HLAsyncPoints");
  auto const statusRequestClass = subject.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPointStatus");
  auto const statusRequestParameter = subject.rtiAmbassador().getParameterHandle(
      statusRequestClass,
      L"HLAsyncPointName");
  auto const statusReportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPointStatus");
  auto const statusReportNameParameter = observer.rtiAmbassador().getParameterHandle(
      statusReportClass,
      L"HLAsyncPointName");
  auto const statusReportFederatesParameter = observer.rtiAmbassador().getParameterHandle(
      statusReportClass,
      L"HLAsyncPointFederates");
  require(
      pointsRequestClass.isValid() && pointsReportClass.isValid() &&
          pointsReportParameter.isValid() && statusRequestClass.isValid() &&
          statusRequestParameter.isValid() && statusReportClass.isValid() &&
          statusReportNameParameter.isValid() &&
          statusReportFederatesParameter.isValid(),
      "MOM synchronization query lookup returned an invalid handle");
  observer.rtiAmbassador().subscribeInteractionClass(pointsReportClass);
  observer.rtiAmbassador().subscribeInteractionClass(statusReportClass);
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(reliable.isValid(), "MOM synchronization query could not resolve HLAreliable");

  auto const decodePointList = [](rti::VariableLengthData const& encoded) {
    rti::HLAunicodeString pointPrototype;
    rti::HLAvariableArray points{
        static_cast<rti::DataElement const&>(pointPrototype)};
    points.decode(encoded);
    std::vector<std::wstring> result;
    result.reserve(points.size());
    for (std::size_t index = 0U; index < points.size(); ++index) {
      result.push_back(dynamic_cast<rti::HLAunicodeString const&>(points.get(index)).get());
    }
    return result;
  };
  auto const decodeStatuses = [&](rti::VariableLengthData const& encoded) {
    rti::HLAvariableArrayT<rti::HLAoctet> federatePrototype;
    rti::HLAfixedRecord recordPrototype;
    recordPrototype.appendElement(federatePrototype)
        .appendElement(rti::HLAinteger32BE{});
    rti::HLAvariableArray statuses{recordPrototype};
    statuses.decode(encoded);
    std::map<rti::FederateHandle, rti::Integer32> result;
    for (std::size_t index = 0U; index < statuses.size(); ++index) {
      auto const& record = dynamic_cast<rti::HLAfixedRecord const&>(statuses.get(index));
      auto const& federateReference = dynamic_cast<rti::HLAvariableArray const&>(
          record.get(0U));
      auto const federate = observer.rtiAmbassador().decodeFederateHandle(
          federateReference.encode());
      auto const status = dynamic_cast<rti::HLAinteger32BE const&>(record.get(1U)).get();
      result.emplace(federate, status);
    }
    return result;
  };
  auto const verifyReport = [&](InteractionRecord const& report,
                                rti::InteractionClassHandle const& expectedClass,
                                std::size_t expectedParameterCount) {
    require(
        report.interaction == expectedClass &&
            report.parameters.size() == expectedParameterCount &&
            report.tag.empty() && !report.producer.isValid() &&
            !report.regions.has_value() && report.transportation == reliable,
        "MOM synchronization query returned non-standard interaction metadata");
  };

  auto const requestPointList = [&] {
    observer.recorder().clearInteraction();
    subject.rtiAmbassador().sendInteraction(
        pointsRequestClass,
        rti::ParameterHandleValueMap{},
        emptyTag);
    waitFor(
        subject,
        observer,
        [&] { return observer.recorder().interactions().size() == 1U; },
        options,
        "MOM synchronization-point list report");
    auto const report = observer.recorder().interactions().front();
    verifyReport(report, pointsReportClass, 1U);
    require(
        report.parameters.count(pointsReportParameter) == 1U,
        "MOM synchronization-point list report omitted HLAsyncPoints");
    return decodePointList(report.parameters.at(pointsReportParameter));
  };

  auto const requestStatus = [&](std::wstring const& requestedLabel) {
    observer.recorder().clearInteraction();
    subject.rtiAmbassador().sendInteraction(
        statusRequestClass,
        rti::ParameterHandleValueMap{{
            statusRequestParameter,
            rti::HLAunicodeString{requestedLabel}.encode()}},
        emptyTag);
    waitFor(
        subject,
        observer,
        [&] { return observer.recorder().interactions().size() == 1U; },
        options,
        "MOM synchronization-point status report");
    auto const report = observer.recorder().interactions().front();
    verifyReport(report, statusReportClass, 2U);
    require(
        report.parameters.count(statusReportNameParameter) == 1U &&
            report.parameters.count(statusReportFederatesParameter) == 1U,
        "MOM synchronization-point status report omitted a standard parameter");
    rti::HLAunicodeString returnedLabel;
    returnedLabel.decode(report.parameters.at(statusReportNameParameter));
    require(
        returnedLabel.get() == requestedLabel,
        "MOM synchronization-point status report returned the wrong label");
    return decodeStatuses(report.parameters.at(statusReportFederatesParameter));
  };

  require(
      requestPointList() == std::vector<std::wstring>{label},
      "MOM synchronization-point list report returned the wrong active points");
  auto const beforeAchievement = requestStatus(label);
  require(
      beforeAchievement.size() == 2U &&
          beforeAchievement.at(subject.federateHandle()) == 2 &&
          beforeAchievement.at(observer.federateHandle()) == 2,
      "MOM synchronization-point status report returned the wrong pending state");

  subject.rtiAmbassador().synchronizationPointAchieved(label);
  for (int pass = 0; pass != 8; ++pass) {
    subject.pump();
    observer.pump();
  }
  auto const afterSubjectAchievement = requestStatus(label);
  require(
      afterSubjectAchievement.size() == 2U &&
          afterSubjectAchievement.at(subject.federateHandle()) == 3 &&
          afterSubjectAchievement.at(observer.federateHandle()) == 2,
      "MOM synchronization-point status report returned the wrong partial state");
  require(
      requestStatus(L"missing-mom-synchronization-point").empty(),
      "MOM synchronization-point status report returned a stale missing label");

  // Under HLA_EVOKED the report is queued and subscriber-gated at delivery;
  // under HLA_IMMEDIATE it is delivered synchronously before unsubscribe.
  observer.recorder().clearInteraction();
  subject.rtiAmbassador().sendInteraction(
      pointsRequestClass,
      rti::ParameterHandleValueMap{},
      emptyTag);
  if (model == rti::HLA_EVOKED) {
    observer.rtiAmbassador().unsubscribeInteractionClass(pointsReportClass);
    while (subject.evokeMultipleCallbacks(0.0, 0.0) ||
           observer.evokeMultipleCallbacks(0.0, 0.0)) {
    }
    require(
        observer.recorder().interactions().empty(),
        "MOM synchronization-point report was delivered after an evoked unsubscribe");
  } else {
    require(
        observer.recorder().interactions().size() == 1U,
        "Immediate MOM synchronization-point report was not delivered synchronously");
    observer.rtiAmbassador().unsubscribeInteractionClass(pointsReportClass);
  }
  observer.rtiAmbassador().subscribeInteractionClass(pointsReportClass);

  observer.rtiAmbassador().synchronizationPointAchieved(label);
  waitFor(
      subject,
      observer,
      [&] {
        return subject.federationSynchronized().size() == 1U &&
            observer.federationSynchronized().size() == 1U;
      },
      options,
      "MOM synchronization-point completion");
  observer.recorder().clearInteraction();
  require(
      requestPointList().empty(),
      "MOM synchronization-point list retained a completed point");

  observer.rtiAmbassador().unsubscribeInteractionClass(pointsReportClass);
  observer.rtiAmbassador().unsubscribeInteractionClass(statusReportClass);
  observer.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioFederationSynchronizationMomQueriesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationSynchronizationMomQueries(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
