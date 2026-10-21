#include "tck_portable_region_service_reports.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioServiceReportDimensionHandleSet(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Dimension-handle-set service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Dimension-handle-set service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-dimension-handle-set-subject");
  Session observer(options, model, "service-report-dimension-handle-set-observer");
  auto const federation =
      federationName(options, "service-report-dimension-handle-set");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-dimension-handle-set-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-dimension-handle-set-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Dimension-handle-set service-report lookup returned an invalid report class handle");
  std::vector<rti::ParameterHandle> const reportParameters = [&] {
    std::vector<rti::ParameterHandle> result;
    for (auto const& name : {
             L"HLAservice",
             L"HLAserviceType",
             L"HLAsuccessIndicator",
             L"HLAsuppliedArguments",
             L"HLAreturnedArgument",
             L"HLAexception",
             L"HLAserialNumber",
             L"HLAfederate"}) {
      auto const parameter = observer.rtiAmbassador().getParameterHandle(
          reportClass,
          name);
      require(
          parameter.isValid(),
          "Dimension-handle-set service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      dimension.isValid(),
      "Dimension-handle-set service-report lookup returned an invalid dimension handle");
  auto const region = subject.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  require(
      region.isValid(),
      "Dimension-handle-set service-report setup returned an invalid region handle");
  require(
      !subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "Dimension-handle-set service-report test could not select MOM delivery");
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const dimensions = subject.rtiAmbassador().getDimensionHandleSet(region);
  require(
      dimensions == rti::DimensionHandleSet{dimension},
      "GetDimensionHandleSet did not return the adapter region's single dimension");

  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 1U; },
      options,
      "successful GetDimensionHandleSet service-report callback");
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 1U,
      "GetDimensionHandleSet delivered an unexpected number of service reports");
  auto const& report = reports.front();
  require(
      report.interaction == reportClass,
      "GetDimensionHandleSet service report used the wrong interaction class");
  require(
      report.parameters.size() == reportParameters.size(),
      "GetDimensionHandleSet service report had " +
          std::to_string(report.parameters.size()) + " parameters; expected " +
          std::to_string(reportParameters.size()));
  require(
      report.tag.empty(),
      "GetDimensionHandleSet service report carried an unexpected tag");
  require(
      !report.producer.isValid(),
      "GetDimensionHandleSet service report carried an unexpected producer handle");
  require(
      !report.regions.has_value(),
      "GetDimensionHandleSet service report carried unexpected region designators");
  require(
      report.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "GetDimensionHandleSet service report did not use reliable transportation");
  auto const reportedFederate = observer.rtiAmbassador().decodeFederateHandle(
      report.parameters.at(reportParameters.at(7U)));
  require(
      reportedFederate == subject.federateHandle(),
      "GetDimensionHandleSet service report identified the wrong joined federate");
  auto const quotedRegion =
      std::wstring{L"\""} + region.toString() + L"\"";
  auto const dimensionSetText =
      std::wstring{L"[\""} + dimension.toString() + L"\"]";
  requirePortableServiceReport(
      observer,
      reports.front(),
      reportClass,
      reportParameters,
      L"GetDimensionHandleSet",
      {{42, L"Region handle", quotedRegion}},
      {11, L"A set of dimensions", dimensionSetText},
      0,
      "GetDimensionHandleSet service report");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().deleteRegion(region);
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportDimensionHandleSetContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDimensionHandleSet(options, model);
}

void scenarioServiceReportRangeBounds(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Range-bounds service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Range-bounds service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-range-bounds-subject");
  Session observer(options, model, "service-report-range-bounds-observer");
  auto const federation = federationName(options, "service-report-range-bounds");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-range-bounds-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-range-bounds-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Range-bounds service-report lookup returned an invalid report class handle");
  std::vector<rti::ParameterHandle> const reportParameters = [&] {
    std::vector<rti::ParameterHandle> result;
    for (auto const& name : {
             L"HLAservice",
             L"HLAserviceType",
             L"HLAsuccessIndicator",
             L"HLAsuppliedArguments",
             L"HLAreturnedArgument",
             L"HLAexception",
             L"HLAserialNumber",
             L"HLAfederate"}) {
      auto const parameter = observer.rtiAmbassador().getParameterHandle(
          reportClass,
          name);
      require(
          parameter.isValid(),
          "Range-bounds service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      dimension.isValid(),
      "Range-bounds service-report setup returned an invalid dimension handle");
  auto const dimensionUpperBound =
      subject.rtiAmbassador().getDimensionUpperBound(dimension);
  require(
      dimensionUpperBound > 0UL,
      "Range-bounds service-report testing requires a positive adapter dimension upper bound");
  auto const region = subject.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  require(
      region.isValid(),
      "Range-bounds service-report setup returned an invalid region handle");
  auto const lowerBound = dimensionUpperBound > 1UL ? 1UL : 0UL;
  subject.rtiAmbassador().setRangeBounds(
      region,
      dimension,
      rti::RangeBounds{lowerBound, dimensionUpperBound});
  subject.rtiAmbassador().commitRegionModifications(rti::RegionHandleSet{region});
  require(
      !subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "Range-bounds service-report test could not select MOM delivery");
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const bounds = subject.rtiAmbassador().getRangeBounds(region, dimension);
  require(
      bounds.getLowerBound() == lowerBound &&
          bounds.getUpperBound() == dimensionUpperBound,
      "GetRangeBounds did not return the adapter region's committed bounds");

  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 1U; },
      options,
      "successful GetRangeBounds service-report callback");
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 1U,
      "GetRangeBounds delivered an unexpected number of service reports");
  auto const& report = reports.front();
  require(
      report.interaction == reportClass,
      "GetRangeBounds service report used the wrong interaction class");
  require(
      report.parameters.size() == reportParameters.size(),
      "GetRangeBounds service report had " +
          std::to_string(report.parameters.size()) + " parameters; expected " +
          std::to_string(reportParameters.size()));
  require(
      report.tag.empty(),
      "GetRangeBounds service report carried an unexpected tag");
  require(
      !report.producer.isValid(),
      "GetRangeBounds service report carried an unexpected producer handle");
  require(
      !report.regions.has_value(),
      "GetRangeBounds service report carried unexpected region designators");
  require(
      report.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "GetRangeBounds service report did not use reliable transportation");
  auto const reportedFederate = observer.rtiAmbassador().decodeFederateHandle(
      report.parameters.at(reportParameters.at(7U)));
  require(
      reportedFederate == subject.federateHandle(),
      "GetRangeBounds service report identified the wrong joined federate");
  auto const quotedRegion =
      std::wstring{L"\""} + region.toString() + L"\"";
  auto const quotedDimension =
      std::wstring{L"\""} + dimension.toString() + L"\"";
  auto const boundsText = L"{\"lower\":" + std::to_wstring(lowerBound) +
      L",\"upper\":" + std::to_wstring(dimensionUpperBound) + L"}";
  requirePortableServiceReport(
      observer,
      reports.front(),
      reportClass,
      reportParameters,
      L"GetRangeBounds",
      {{42, L"Region handle", quotedRegion},
       {10, L"Dimension handle", quotedDimension}},
      {41, L"Range bounds", boundsText},
      0,
      "GetRangeBounds service report",
      5);

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().deleteRegion(region);
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportRangeBoundsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportRangeBounds(options, model);
}

void scenarioServiceReportSetRangeBounds(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "SetRangeBounds service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "SetRangeBounds service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-set-range-bounds-subject");
  Session observer(options, model, "service-report-set-range-bounds-observer");
  auto const federation = federationName(options, "service-report-set-range-bounds");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-set-range-bounds-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-set-range-bounds-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "SetRangeBounds service-report lookup returned an invalid report class handle");
  std::vector<rti::ParameterHandle> const reportParameters = [&] {
    std::vector<rti::ParameterHandle> result;
    for (auto const& name : {
             L"HLAservice",
             L"HLAserviceType",
             L"HLAsuccessIndicator",
             L"HLAsuppliedArguments",
             L"HLAreturnedArgument",
             L"HLAexception",
             L"HLAserialNumber",
             L"HLAfederate"}) {
      auto const parameter = observer.rtiAmbassador().getParameterHandle(
          reportClass,
          name);
      require(
          parameter.isValid(),
          "SetRangeBounds service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      dimension.isValid(),
      "SetRangeBounds service-report setup returned an invalid dimension handle");
  auto const upperBound =
      subject.rtiAmbassador().getDimensionUpperBound(dimension);
  require(
      upperBound > 0UL,
      "SetRangeBounds service-report testing requires a positive adapter dimension upper bound");
  auto const region = subject.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  require(
      region.isValid(),
      "SetRangeBounds service-report setup returned an invalid region handle");
  subject.rtiAmbassador().setRangeBounds(
      region,
      dimension,
      rti::RangeBounds{0UL, upperBound});
  subject.rtiAmbassador().commitRegionModifications(rti::RegionHandleSet{region});
  require(
      !subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "SetRangeBounds service-report test could not select MOM delivery");

  auto const lowerBound = upperBound > 1UL ? 1UL : 0UL;
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  subject.rtiAmbassador().setRangeBounds(
      region,
      dimension,
      rti::RangeBounds{lowerBound, upperBound});
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 1U; },
      options,
      "successful SetRangeBounds service-report callback");
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 1U,
      "SetRangeBounds delivered an unexpected number of service reports");
  auto const& report = reports.front();
  auto const quotedRegion =
      std::wstring{L"\""} + region.toString() + L"\"";
  auto const quotedDimension =
      std::wstring{L"\""} + dimension.toString() + L"\"";
  requirePortableServiceReport(
      observer,
      report,
      reportClass,
      reportParameters,
      L"SetRangeBounds",
      {{42, L"Region handle", quotedRegion},
       {10, L"Dimension handle", quotedDimension},
       {35, L"Range lower bound", std::to_wstring(lowerBound)},
       {35, L"Range upper bound", std::to_wstring(upperBound)}},
      {34, L"", L"null"},
      0,
      "SetRangeBounds service report",
      5);
  auto const reportedFederate = observer.rtiAmbassador().decodeFederateHandle(
      report.parameters.at(reportParameters.at(7U)));
  require(
      reportedFederate == subject.federateHandle(),
      "SetRangeBounds service report identified the wrong joined federate");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  auto const updatedBounds = subject.rtiAmbassador().getRangeBounds(region, dimension);
  require(
      updatedBounds.getLowerBound() == lowerBound &&
          updatedBounds.getUpperBound() == upperBound,
      "SetRangeBounds did not retain the adapter-valid bounds");
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().deleteRegion(region);
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportSetRangeBoundsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportSetRangeBounds(options, model);
}

void scenarioServiceReportCommitRegionModifications(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "CommitRegionModifications service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "CommitRegionModifications service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-commit-region-subject");
  Session observer(options, model, "service-report-commit-region-observer");
  auto const federation =
      federationName(options, "service-report-commit-region-modifications");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-commit-region-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-commit-region-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "CommitRegionModifications service-report lookup returned an invalid report class handle");
  std::vector<rti::ParameterHandle> const reportParameters = [&] {
    std::vector<rti::ParameterHandle> result;
    for (auto const& name : {
             L"HLAservice",
             L"HLAserviceType",
             L"HLAsuccessIndicator",
             L"HLAsuppliedArguments",
             L"HLAreturnedArgument",
             L"HLAexception",
             L"HLAserialNumber",
             L"HLAfederate"}) {
      auto const parameter = observer.rtiAmbassador().getParameterHandle(
          reportClass,
          name);
      require(
          parameter.isValid(),
          "CommitRegionModifications service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      dimension.isValid(),
      "CommitRegionModifications service-report setup returned an invalid dimension handle");
  auto const upperBound =
      subject.rtiAmbassador().getDimensionUpperBound(dimension);
  require(
      upperBound > 0UL,
      "CommitRegionModifications service-report testing requires a positive adapter dimension upper bound");
  auto const region = subject.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  auto const secondRegion = subject.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  require(
      region.isValid() && secondRegion.isValid() && region != secondRegion,
      "CommitRegionModifications service-report setup returned invalid or duplicate region handles");
  subject.rtiAmbassador().setRangeBounds(
      region,
      dimension,
      rti::RangeBounds{0UL, upperBound});
  subject.rtiAmbassador().setRangeBounds(
      secondRegion,
      dimension,
      rti::RangeBounds{0UL, upperBound});
  rti::RegionHandleSet const regions{region, secondRegion};
  require(
      regions.size() == 2U,
      "CommitRegionModifications service-report fixture did not retain two distinct regions");
  require(
      !subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "CommitRegionModifications service-report test could not select MOM delivery");

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  subject.rtiAmbassador().commitRegionModifications(regions);
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 1U; },
      options,
      "successful CommitRegionModifications service-report callback");
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 1U,
      "CommitRegionModifications delivered an unexpected number of service reports");
  auto const& report = reports.front();
  std::wstring regionSetText{L"["};
  bool firstRegion = true;
  for (auto const& regionHandle : regions) {
    if (!firstRegion) {
      regionSetText += L",";
    }
    firstRegion = false;
    regionSetText += L"\"" + regionHandle.toString() + L"\"";
  }
  regionSetText += L"]";
  requirePortableServiceReport(
      observer,
      report,
      reportClass,
      reportParameters,
      L"CommitRegionModifications",
      {{43, L"Set of region designators", regionSetText}},
      {34, L"", L"null"},
      0,
      "CommitRegionModifications service report",
      5);
  auto const reportedFederate = observer.rtiAmbassador().decodeFederateHandle(
      report.parameters.at(reportParameters.at(7U)));
  require(
      reportedFederate == subject.federateHandle(),
      "CommitRegionModifications service report identified the wrong joined federate");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  auto const committedBounds = subject.rtiAmbassador().getRangeBounds(region, dimension);
  auto const secondCommittedBounds =
      subject.rtiAmbassador().getRangeBounds(secondRegion, dimension);
  require(
      committedBounds.getLowerBound() == 0UL &&
          committedBounds.getUpperBound() == upperBound &&
          secondCommittedBounds.getLowerBound() == 0UL &&
          secondCommittedBounds.getUpperBound() == upperBound,
      "CommitRegionModifications did not commit bounds for both adapter-valid regions");
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().deleteRegion(region);
  subject.rtiAmbassador().deleteRegion(secondRegion);
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportCommitRegionModificationsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportCommitRegionModifications(options, model);
}

void scenarioServiceReportDeleteRegion(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "DeleteRegion service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "DeleteRegion service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-delete-region-subject");
  Session observer(options, model, "service-report-delete-region-observer");
  auto const federation = federationName(options, "service-report-delete-region");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-delete-region-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-delete-region-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "DeleteRegion service-report lookup returned an invalid report class handle");
  std::vector<rti::ParameterHandle> const reportParameters = [&] {
    std::vector<rti::ParameterHandle> result;
    for (auto const& name : {
             L"HLAservice",
             L"HLAserviceType",
             L"HLAsuccessIndicator",
             L"HLAsuppliedArguments",
             L"HLAreturnedArgument",
             L"HLAexception",
             L"HLAserialNumber",
             L"HLAfederate"}) {
      auto const parameter = observer.rtiAmbassador().getParameterHandle(
          reportClass,
          name);
      require(
          parameter.isValid(),
          "DeleteRegion service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      dimension.isValid(),
      "DeleteRegion service-report setup returned an invalid dimension handle");
  auto const region = subject.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  require(
      region.isValid(),
      "DeleteRegion service-report setup returned an invalid region handle");
  require(
      !subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "DeleteRegion service-report test could not select MOM delivery");

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  subject.rtiAmbassador().deleteRegion(region);
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 1U; },
      options,
      "successful DeleteRegion service-report callback");
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 1U,
      "DeleteRegion delivered an unexpected number of service reports");
  auto const& report = reports.front();
  auto const quotedRegion =
      std::wstring{L"\""} + region.toString() + L"\"";
  requirePortableServiceReport(
      observer,
      report,
      reportClass,
      reportParameters,
      L"DeleteRegion",
      {{42, L"Region designator", quotedRegion}},
      {34, L"", L"null"},
      0,
      "DeleteRegion service report",
      5);
  auto const reportedFederate = observer.rtiAmbassador().decodeFederateHandle(
      report.parameters.at(reportParameters.at(7U)));
  require(
      reportedFederate == subject.federateHandle(),
      "DeleteRegion service report identified the wrong joined federate");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  bool deletedRegionRejected = false;
  try {
    static_cast<void>(subject.rtiAmbassador().getDimensionHandleSet(region));
  } catch (rti::InvalidRegion const&) {
    deletedRegionRejected = true;
  }
  require(
      deletedRegionRejected,
      "GetDimensionHandleSet did not reject the deleted region with InvalidRegion");
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportDeleteRegionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDeleteRegion(options, model);
}

void scenarioServiceReportCreateRegion(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "CreateRegion service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "CreateRegion service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-create-region-subject");
  Session observer(options, model, "service-report-create-region-observer");
  auto const federation = federationName(options, "service-report-create-region");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-create-region-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-create-region-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "CreateRegion service-report lookup returned an invalid report class handle");
  std::vector<rti::ParameterHandle> const reportParameters = [&] {
    std::vector<rti::ParameterHandle> result;
    for (auto const& name : {
             L"HLAservice",
             L"HLAserviceType",
             L"HLAsuccessIndicator",
             L"HLAsuppliedArguments",
             L"HLAreturnedArgument",
             L"HLAexception",
             L"HLAserialNumber",
             L"HLAfederate"}) {
      auto const parameter = observer.rtiAmbassador().getParameterHandle(
          reportClass,
          name);
      require(
          parameter.isValid(),
          "CreateRegion service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      dimension.isValid(),
      "CreateRegion service-report setup returned an invalid dimension handle");
  require(
      !subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "CreateRegion service-report test could not select MOM delivery");

  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const region = subject.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  require(region.isValid(), "CreateRegion returned an invalid region handle");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 1U; },
      options,
      "successful CreateRegion service-report callback");
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 1U,
      "CreateRegion delivered an unexpected number of service reports");
  auto const& report = reports.front();
  auto const dimensionSetValue =
      std::wstring{L"[\""} + dimension.toString() + L"\"]";
  auto const quotedRegion =
      std::wstring{L"\""} + region.toString() + L"\"";
  requirePortableServiceReport(
      observer,
      report,
      reportClass,
      reportParameters,
      L"CreateRegion",
      {{11, L"Set of dimension designators", dimensionSetValue}},
      {42, L"Region designator", quotedRegion},
      0,
      "CreateRegion service report",
      5);
  auto const reportedFederate = observer.rtiAmbassador().decodeFederateHandle(
      report.parameters.at(reportParameters.at(7U)));
  require(
      reportedFederate == subject.federateHandle(),
      "CreateRegion service report identified the wrong joined federate");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().deleteRegion(region);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportCreateRegionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportCreateRegion(options, model);
}

void scenarioRegionRangeValidation(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Region range validation requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Region range validation requires an adapter-supplied standard MIM");

  Session participant(options, model, "region-range-validation");
  auto const federation = federationName(options, "region-range-validation");
  participant.connect();
  participant.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  participant.join(
      options.ownerFederateName + L"-region-range-validation",
      options.federateType,
      federation);

  auto const dimension = participant.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      dimension.isValid(),
      "Region range validation setup returned an invalid dimension handle");
  auto const upperBound =
      participant.rtiAmbassador().getDimensionUpperBound(dimension);
  require(
      upperBound > 0UL,
      "Region range validation requires a positive adapter dimension upper bound");
  auto const region = participant.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  require(
      region.isValid(),
      "Region range validation setup returned an invalid region handle");

  participant.rtiAmbassador().setRangeBounds(
      region,
      dimension,
      rti::RangeBounds{0UL, upperBound});
  participant.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{region});
  auto const validBounds =
      participant.rtiAmbassador().getRangeBounds(region, dimension);
  require(
      validBounds.getLowerBound() == 0UL &&
          validBounds.getUpperBound() == upperBound,
      "A range ending at the dimension upper bound was not accepted");

  auto const requireInvalidRange = [&](rti::RangeBounds const& invalidBounds,
                                       char const* description) {
    bool rejected = false;
    try {
      participant.rtiAmbassador().setRangeBounds(
          region,
          dimension,
          invalidBounds);
    } catch (rti::InvalidRangeBound const&) {
      rejected = true;
    }
    require(
        rejected,
        "SetRangeBounds did not throw InvalidRangeBound for " +
            std::string{description});
  };
  requireInvalidRange(
      rti::RangeBounds{upperBound, upperBound},
      "equal lower and upper bounds");
  requireInvalidRange(
      rti::RangeBounds{upperBound, 0UL},
      "a lower bound greater than the upper bound");

  participant.rtiAmbassador().deleteRegion(region);
  participant.resign(rti::NO_ACTION);
  participant.rtiAmbassador().destroyFederationExecution(federation);
  participant.disconnect();
}

void scenarioRegionRangeValidationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionRangeValidation(options, model);
}

void scenarioRegionRangeDimensionValidation(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Region dimension-membership validation requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Region dimension-membership validation requires an adapter-supplied standard MIM");

  Session participant(options, model, "region-range-dimension-validation");
  auto const federation =
      federationName(options, "region-range-dimension-validation");
  participant.connect();
  participant.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  participant.join(
      options.ownerFederateName + L"-region-range-dimension-validation",
      options.federateType,
      federation);

  auto const dimension = participant.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      dimension.isValid(),
      "Region dimension-membership setup returned an invalid dimension handle");
  auto const upperBound =
      participant.rtiAmbassador().getDimensionUpperBound(dimension);
  require(
      upperBound > 0UL,
      "Region dimension-membership validation requires a positive dimension upper bound");
  auto const region = participant.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  participant.rtiAmbassador().setRangeBounds(
      region,
      dimension,
      rti::RangeBounds{0UL, upperBound});
  participant.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{region});

  auto const absentDimension = rti::DimensionHandle{};
  bool getRejected = false;
  try {
    static_cast<void>(
        participant.rtiAmbassador().getRangeBounds(region, absentDimension));
  } catch (rti::RegionDoesNotContainSpecifiedDimension const&) {
    getRejected = true;
  }
  require(
      getRejected,
      "GetRangeBounds did not reject a dimension absent from the region with "
      "RegionDoesNotContainSpecifiedDimension");

  bool setRejected = false;
  try {
    participant.rtiAmbassador().setRangeBounds(
        region,
        absentDimension,
        rti::RangeBounds{0UL, upperBound});
  } catch (rti::RegionDoesNotContainSpecifiedDimension const&) {
    setRejected = true;
  }
  require(
      setRejected,
      "SetRangeBounds did not reject a dimension absent from the region with "
      "RegionDoesNotContainSpecifiedDimension");

  participant.rtiAmbassador().deleteRegion(region);
  participant.resign(rti::NO_ACTION);
  participant.rtiAmbassador().destroyFederationExecution(federation);
  participant.disconnect();
}

void scenarioRegionRangeDimensionValidationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionRangeDimensionValidation(options, model);
}

void scenarioServiceReportAssociateRegionsForUpdates(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Associate Regions for Updates service-report testing requires an adapter-supplied P0 FOM");
  require(
      !options.modelFom.empty(),
      "Associate Regions for Updates service-report testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Associate Regions for Updates service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-associate-regions-subject");
  Session observer(options, model, "service-report-associate-regions-observer");
  auto const federation =
      federationName(options, "service-report-associate-regions-for-updates");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{
          options.fom.wstring(),
          options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-associate-regions-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-associate-regions-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Associate Regions for Updates service-report lookup returned an invalid interaction handle");
  std::vector<rti::ParameterHandle> const reportParameters = [&] {
    std::vector<rti::ParameterHandle> result;
    for (auto const& name : {
             L"HLAservice",
             L"HLAserviceType",
             L"HLAsuccessIndicator",
             L"HLAsuppliedArguments",
             L"HLAreturnedArgument",
             L"HLAexception",
             L"HLAserialNumber",
             L"HLAfederate"}) {
      auto const parameter = observer.rtiAmbassador().getParameterHandle(
          reportClass,
          name);
      require(
          parameter.isValid(),
          "Associate Regions for Updates service-report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const attribute = subject.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.typedIdentityAttributeName);
  auto const secondAttribute = subject.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.typedIntegerAttributeName);
  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      objectClass.isValid() && attribute.isValid() &&
          secondAttribute.isValid() && attribute != secondAttribute &&
          dimension.isValid(),
      "Associate Regions for Updates service-report setup returned an invalid FOM handle");
  rti::AttributeHandleSet const attributes{attribute};
  rti::AttributeHandleSet const secondAttributes{secondAttribute};
  subject.rtiAmbassador().publishObjectClassAttributes(
      objectClass,
      rti::AttributeHandleSet{attribute, secondAttribute});

  auto const dimensionUpperBound =
      subject.rtiAmbassador().getDimensionUpperBound(dimension);
  require(
      dimensionUpperBound > 0UL,
      "Associate Regions for Updates service-report testing requires a positive dimension upper bound");
  auto const region = subject.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  auto const secondRegion = subject.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  require(
      region.isValid() && secondRegion.isValid() && region != secondRegion,
      "Associate Regions for Updates service-report setup returned invalid or duplicate region handles");
  subject.rtiAmbassador().setRangeBounds(
      region,
      dimension,
      rti::RangeBounds{0UL, dimensionUpperBound});
  subject.rtiAmbassador().setRangeBounds(
      secondRegion,
      dimension,
      rti::RangeBounds{0UL, dimensionUpperBound});
  subject.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{region, secondRegion});
  auto const object = subject.rtiAmbassador().registerObjectInstance(objectClass);
  require(
      object.isValid(),
      "Associate Regions for Updates service-report setup returned an invalid object handle");

  require(
      !subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "Associate Regions for Updates service-report test could not select MOM delivery");
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  rti::AttributeHandleSetRegionHandleSetPairVector const association{
      {attributes, rti::RegionHandleSet{region}},
      {secondAttributes, rti::RegionHandleSet{secondRegion}},
  };
  subject.rtiAmbassador().associateRegionsForUpdates(object, association);

  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 1U; },
      options,
      "Associate Regions for Updates service-report callback");
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 1U,
      "Associate Regions for Updates delivered an unexpected number of service reports");
  auto const& report = reports.front();
  require(
      report.interaction == reportClass,
      "Associate Regions for Updates service report used the wrong interaction class");
  require(
      report.parameters.size() == reportParameters.size(),
      "Associate Regions for Updates service report had an unexpected parameter count");
  require(
      report.tag.empty() && !report.producer.isValid() &&
          !report.regions.has_value(),
      "Associate Regions for Updates service report carried application metadata");
  require(
      report.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Associate Regions for Updates service report did not use reliable transportation");
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          report.parameters.at(reportParameters.at(7U))) ==
          subject.federateHandle(),
      "Associate Regions for Updates service report identified the wrong federate");

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const associationValue =
      std::wstring{L"[{\"attributeHandleSet\":["} +
      quoted(attribute.toString()) +
      L"],\"regionHandleSet\":[" + quoted(region.toString()) +
      L"]},{\"attributeHandleSet\":[" +
      quoted(secondAttribute.toString()) +
      L"],\"regionHandleSet\":[" + quoted(secondRegion.toString()) + L"]}]";
  requirePortableServiceReport(
      observer,
      report,
      reportClass,
      reportParameters,
      L"AssociateRegionsForUpdates",
      {{37, L"Object instance designator", quoted(object.toString())},
       {4,
        L"Collection of attribute designator set and region designator set pairs",
        associationValue}},
      {34, L"", L"null"},
      0,
      "Associate Regions for Updates service report",
      5);

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().unassociateRegionsForUpdates(object, association);
  subject.rtiAmbassador().deleteRegion(region);
  subject.rtiAmbassador().deleteRegion(secondRegion);
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportAssociateRegionsForUpdatesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportAssociateRegionsForUpdates(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
