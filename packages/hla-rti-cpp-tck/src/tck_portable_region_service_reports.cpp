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
  require(
      options.ddmDimensionNames.size() >= 2U,
      "Dimension-handle-set service-report testing requires two adapter-selected model dimensions");

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

  auto const federateDimension = observer.rtiAmbassador().getDimensionHandle(
      L"HLAfederate");
  auto const serviceGroupDimension = observer.rtiAmbassador().getDimensionHandle(
      L"HLAserviceGroup");
  require(
      federateDimension.isValid() && serviceGroupDimension.isValid(),
      "Service-report routing setup could not resolve the standard MOM dimensions");
  rti::FederateHandle invalidFederate;
  require(
      !invalidFederate.isValid(),
      "default-constructed FederateHandle was unexpectedly valid");
  requireException(
      [&] {
        static_cast<void>(
            observer.rtiAmbassador().normalizeFederateHandle(invalidFederate));
      },
      L"InvalidFederateHandle",
      "normalizing an invalid federate handle");
  auto const reportedFederateValue = observer.rtiAmbassador().normalizeFederateHandle(
      subject.federateHandle());
  auto const observerFederateValue = observer.rtiAmbassador().normalizeFederateHandle(
      observer.federateHandle());
  auto const federateRegion = observer.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{federateDimension});
  require(
      federateRegion.isValid(),
      "Could not create a region using the standard HLAfederate dimension");
  unsigned long const federatePointValues[] = {
      reportedFederateValue,
      observerFederateValue};
  for (auto const federatePointValue : federatePointValues) {
    require(
        federatePointValue < std::numeric_limits<unsigned long>::max(),
        "Normalized federate point value cannot be represented as a half-open range");
    rti::RangeBounds const expectedBounds(
        federatePointValue,
        federatePointValue + 1UL);
    observer.rtiAmbassador().setRangeBounds(
        federateRegion,
        federateDimension,
        expectedBounds);
    observer.rtiAmbassador().commitRegionModifications(
        rti::RegionHandleSet{federateRegion});
    auto const actualBounds = observer.rtiAmbassador().getRangeBounds(
        federateRegion,
        federateDimension);
    require(
        actualBounds.getLowerBound() == expectedBounds.getLowerBound() &&
            actualBounds.getUpperBound() == expectedBounds.getUpperBound(),
        "Normalized federate value could not be used as an HLAfederate point range");
  }
  observer.rtiAmbassador().deleteRegion(federateRegion);

  auto const serviceGroupUpperBound =
      observer.rtiAmbassador().getDimensionUpperBound(serviceGroupDimension);
  require(
      serviceGroupUpperBound == 7UL,
      "The 2025 MIM HLAserviceGroup dimension must have upper bound seven");
  auto const serviceGroupRegion = observer.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{serviceGroupDimension});
  require(
      serviceGroupRegion.isValid(),
      "Could not create a region using the standard HLAserviceGroup dimension");

  rti::ServiceGroup const serviceGroups[] = {
      rti::FEDERATION_MANAGEMENT,
      rti::DECLARATION_MANAGEMENT,
      rti::OBJECT_MANAGEMENT,
      rti::OWNERSHIP_MANAGEMENT,
      rti::TIME_MANAGEMENT,
      rti::DATA_DISTRIBUTION_MANAGEMENT,
      rti::SUPPORT_SERVICES};
  unsigned long supportServicesGroup = 0UL;
  for (auto const serviceGroup : serviceGroups) {
    auto const normalizedServiceGroup =
        observer.rtiAmbassador().normalizeServiceGroup(serviceGroup);
    require(
        normalizedServiceGroup < serviceGroupUpperBound,
        "Normalize Service Group returned a value outside the HLAserviceGroup dimension");
    rti::RangeBounds const expectedBounds(
        normalizedServiceGroup,
        normalizedServiceGroup + 1UL);
    observer.rtiAmbassador().setRangeBounds(
        serviceGroupRegion,
        serviceGroupDimension,
        expectedBounds);
    observer.rtiAmbassador().commitRegionModifications(
        rti::RegionHandleSet{serviceGroupRegion});
    auto const actualBounds = observer.rtiAmbassador().getRangeBounds(
        serviceGroupRegion,
        serviceGroupDimension);
    require(
        actualBounds.getLowerBound() == expectedBounds.getLowerBound() &&
            actualBounds.getUpperBound() == expectedBounds.getUpperBound(),
        "Normalized service-group value could not be used as an HLAserviceGroup point range");
    if (serviceGroup == rti::SUPPORT_SERVICES) {
      supportServicesGroup = normalizedServiceGroup;
    }
  }
  observer.rtiAmbassador().deleteRegion(serviceGroupRegion);

  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  observer.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);

  std::size_t callbackRegionSets = 0U;
  std::set<std::wstring> callbackDimensionNames;
  std::map<std::wstring, std::pair<unsigned long, unsigned long>>
      callbackDimensionBounds;
  observer.recorder().setReceiveInteractionHook(
      [&](rti::RegionHandleSet const* callbackRegions) {
        require(
            callbackRegions != nullptr && callbackRegions->size() == 1U,
            "HLAreportServiceInvocation callback did not contain exactly one region designator");
        ++callbackRegionSets;
        for (auto const& callbackRegion : *callbackRegions) {
          auto const callbackDimensions =
              observer.rtiAmbassador().getDimensionHandleSet(callbackRegion);
          for (auto const& callbackDimension : callbackDimensions) {
            auto const name = observer.rtiAmbassador().getDimensionName(
                callbackDimension);
            auto const bounds = observer.rtiAmbassador().getRangeBounds(
                callbackRegion,
                callbackDimension);
            callbackDimensionNames.insert(name);
            require(
                callbackDimensionBounds.emplace(
                    name,
                    std::make_pair(
                        bounds.getLowerBound(), bounds.getUpperBound())).second,
                "HLAreportServiceInvocation callback repeated a region dimension");
          }
        }
      });

  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  auto const secondDimension = subject.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.at(0U));
  require(
      dimension.isValid() && secondDimension.isValid() && dimension != secondDimension,
      "Dimension-handle-set service-report setup did not resolve two distinct model dimensions");
  auto const expectedDimensions =
      rti::DimensionHandleSet{dimension, secondDimension};
  auto const region = subject.rtiAmbassador().createRegion(
      expectedDimensions);
  require(
      region.isValid(),
      "Dimension-handle-set service-report setup returned an invalid region handle");
  require(
      !subject.rtiAmbassador().getSendServiceReportsToFileSwitch(),
      "Dimension-handle-set service-report test could not select MOM delivery");
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const dimensions = subject.rtiAmbassador().getDimensionHandleSet(region);
  require(
      dimensions == expectedDimensions,
      "GetDimensionHandleSet did not return exactly both dimensions used to create the adapter region");

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
      report.regions.has_value() && report.regions->size() == 1U &&
          callbackRegionSets == 1U,
      "GetDimensionHandleSet service report did not convey its single callback region");
  require(
      callbackDimensionNames ==
          std::set<std::wstring>{L"HLAfederate", L"HLAserviceGroup"},
      "GetDimensionHandleSet service report region did not contain exactly the HLAfederate and HLAserviceGroup dimensions");
  auto const actualFederateBounds = callbackDimensionBounds.at(L"HLAfederate");
  auto const actualServiceGroupBounds = callbackDimensionBounds.at(L"HLAserviceGroup");
  auto const expectedFederateBounds = std::make_pair(
      reportedFederateValue,
      reportedFederateValue + 1UL);
  auto const expectedServiceGroupBounds = std::make_pair(
      supportServicesGroup,
      supportServicesGroup + 1UL);
  require(
      actualFederateBounds == expectedFederateBounds,
      "GetDimensionHandleSet service report HLAfederate range was [" +
          std::to_string(actualFederateBounds.first) + "," +
          std::to_string(actualFederateBounds.second) + "); expected [" +
          std::to_string(expectedFederateBounds.first) + "," +
          std::to_string(expectedFederateBounds.second) + ")");
  require(
      actualServiceGroupBounds == expectedServiceGroupBounds,
      "GetDimensionHandleSet service report HLAserviceGroup range was [" +
          std::to_string(actualServiceGroupBounds.first) + "," +
          std::to_string(actualServiceGroupBounds.second) + "); expected [" +
          std::to_string(expectedServiceGroupBounds.first) + "," +
          std::to_string(expectedServiceGroupBounds.second) + ") for SUPPORT_SERVICES");
  require(
      report.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "GetDimensionHandleSet service report did not use reliable transportation");
  auto const reportedFederate = observer.rtiAmbassador().decodeFederateHandle(
      report.parameters.at(reportParameters.at(7U)));
  require(
      reportedFederate == subject.federateHandle(),
      "GetDimensionHandleSet service report identified the wrong joined federate");
  require(
      observer.rtiAmbassador().normalizeFederateHandle(reportedFederate) ==
          reportedFederateValue,
      "GetDimensionHandleSet service report region did not use the normalized HLAfederate parameter value");
  rti::HLAunicodeString reportedService;
  reportedService.decode(report.parameters.at(reportParameters.at(0U)));
  require(
      reportedService.get() == L"GetDimensionHandleSet",
      "GetDimensionHandleSet service report region was filtered for the wrong service");
  auto const quotedRegion =
      std::wstring{L"\""} + region.toString() + L"\"";
  std::wstring dimensionSetText{L"["};
  bool firstExpectedDimension = true;
  for (auto const& expectedDimension : expectedDimensions) {
    if (!firstExpectedDimension) {
      dimensionSetText += L",";
    }
    dimensionSetText += L"\"";
    dimensionSetText += expectedDimension.toString();
    dimensionSetText += L"\"";
    firstExpectedDimension = false;
  }
  dimensionSetText += L"]";
  requirePortableServiceReport(
      observer,
      reports.front(),
      reportClass,
      reportParameters,
      L"GetDimensionHandleSet",
      {{42, L"Region handle", quotedRegion}},
      {11, L"A set of dimensions", dimensionSetText},
      0,
      "GetDimensionHandleSet service report",
      6,
      true);

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  rti::RegionHandle invalidRegion;
  require(
      !invalidRegion.isValid(),
      "default-constructed RegionHandle was unexpectedly valid");
  requireException(
      [&] {
        static_cast<void>(
            subject.rtiAmbassador().getDimensionHandleSet(invalidRegion));
      },
      L"InvalidRegion",
      "GetDimensionHandleSet with an invalid region");
  require(
      observer.recorder().interactions().size() == reports.size(),
      "disabled service reporting delivered a GetDimensionHandleSet failure report");
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
  require(
      !options.ddmDimensionNames.empty(),
      "Range-bounds service-report testing requires an adapter-supplied second dimension");
  auto const absentDimension = subject.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  require(
      absentDimension.isValid() && absentDimension != dimension,
      "Range-bounds service-report setup requires a valid dimension absent from the region");
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

  auto const updatedLowerBound = 0UL;
  auto const updatedUpperBound =
      dimensionUpperBound > 1UL ? dimensionUpperBound - 1UL : 0UL;
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setRangeBounds(
      region,
      dimension,
      rti::RangeBounds{updatedLowerBound, updatedUpperBound});
  subject.rtiAmbassador().commitRegionModifications(rti::RegionHandleSet{region});
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  auto const updatedBounds = subject.rtiAmbassador().getRangeBounds(region, dimension);
  require(
      updatedBounds.getLowerBound() == updatedLowerBound &&
          updatedBounds.getUpperBound() == updatedUpperBound,
      "GetRangeBounds did not return the latest committed bounds for the region");

  rti::RegionHandle invalidRegion;
  bool invalidRegionRejected = false;
  try {
    static_cast<void>(
        subject.rtiAmbassador().getRangeBounds(invalidRegion, dimension));
  } catch (rti::InvalidRegion const&) {
    invalidRegionRejected = true;
  }
  require(
      invalidRegionRejected,
      "GetRangeBounds did not reject an invalid region with InvalidRegion");

  bool regionDoesNotContainDimensionRejected = false;
  try {
    static_cast<void>(
        subject.rtiAmbassador().getRangeBounds(region, absentDimension));
  } catch (rti::RegionDoesNotContainSpecifiedDimension const&) {
    regionDoesNotContainDimensionRejected = true;
  }
  require(
      regionDoesNotContainDimensionRejected,
      "GetRangeBounds did not reject a dimension absent from the region");

  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 4U; },
      options,
      "two successful and two failed GetRangeBounds service-report callbacks");
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 4U,
      "two GetRangeBounds successes and two failures delivered an unexpected number of service reports");
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
  auto const quotedAbsentDimension =
      std::wstring{L"\""} + absentDimension.toString() + L"\"";
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
  auto const updatedBoundsText = L"{\"lower\":" +
      std::to_wstring(updatedLowerBound) + L",\"upper\":" +
      std::to_wstring(updatedUpperBound) + L"}";
  requirePortableServiceReport(
      observer,
      reports.at(1U),
      reportClass,
      reportParameters,
      L"GetRangeBounds",
      {{42, L"Region handle", quotedRegion},
       {10, L"Dimension handle", quotedDimension}},
      {41, L"Range bounds", updatedBoundsText},
      1,
      "GetRangeBounds report after a later region commit",
      5);
  auto const quotedInvalidRegion =
      std::wstring{L"\""} + invalidRegion.toString() + L"\"";
  requirePortableServiceReportFailure(
      observer,
      reports.at(2U),
      reportClass,
      reportParameters,
      L"GetRangeBounds",
      {{42, L"Region handle", quotedInvalidRegion},
       {10, L"Dimension handle", quotedDimension}},
      L"InvalidRegion",
      2,
      "GetRangeBounds InvalidRegion service report",
      5);
  auto const failureFederate = observer.rtiAmbassador().decodeFederateHandle(
      reports.at(2U).parameters.at(reportParameters.at(7U)));
  require(
      failureFederate == subject.federateHandle(),
      "GetRangeBounds failure service report identified the wrong joined federate");
  requirePortableServiceReportFailure(
      observer,
      reports.at(3U),
      reportClass,
      reportParameters,
      L"GetRangeBounds",
      {{42, L"Region handle", quotedRegion},
       {10, L"Dimension handle", quotedAbsentDimension}},
      L"RegionDoesNotContainSpecifiedDimension",
      3,
      "GetRangeBounds RegionDoesNotContainSpecifiedDimension service report",
      5);
  auto const missingDimensionFailureFederate =
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(3U).parameters.at(reportParameters.at(7U)));
  require(
      missingDimensionFailureFederate == subject.federateHandle(),
      "GetRangeBounds dimension-mismatch failure report identified the wrong joined federate");

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
  require(
      !options.ddmDimensionNames.empty(),
      "SetRangeBounds service-report testing requires an adapter-supplied second dimension");
  auto const absentDimension = subject.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  require(
      absentDimension.isValid() && absentDimension != dimension,
      "SetRangeBounds service-report setup requires a valid dimension absent from the region");
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
  auto const invalidBounds =
      rti::RangeBounds{upperBound, upperBound - 1UL};
  bool invalidBoundsRejected = false;
  try {
    subject.rtiAmbassador().setRangeBounds(region, dimension, invalidBounds);
  } catch (rti::InvalidRangeBound const&) {
    invalidBoundsRejected = true;
  }
  require(
      invalidBoundsRejected,
      "SetRangeBounds did not reject lower > upper with InvalidRangeBound");
  bool absentDimensionRejected = false;
  try {
    subject.rtiAmbassador().setRangeBounds(
        region,
        absentDimension,
        rti::RangeBounds{lowerBound, upperBound});
  } catch (rti::RegionDoesNotContainSpecifiedDimension const&) {
    absentDimensionRejected = true;
  }
  require(
      absentDimensionRejected,
      "SetRangeBounds accepted a dimension absent from the region");
  rti::RegionHandle invalidRegion;
  bool invalidRegionRejected = false;
  try {
    subject.rtiAmbassador().setRangeBounds(
        invalidRegion,
        dimension,
        rti::RangeBounds{lowerBound, upperBound});
  } catch (rti::InvalidRegion const&) {
    invalidRegionRejected = true;
  }
  require(
      invalidRegionRejected,
      "SetRangeBounds accepted an invalid region");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 4U; },
      options,
      "successful and failed SetRangeBounds service-report callbacks");
  auto const reports = observer.recorder().interactions();
  require(
      reports.size() == 4U,
      "SetRangeBounds success and failures delivered an unexpected number of service reports");
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
  requirePortableServiceReportFailure(
      observer,
      reports.at(1U),
      reportClass,
      reportParameters,
      L"SetRangeBounds",
      {{42, L"Region handle", quotedRegion},
       {10, L"Dimension handle", quotedDimension},
       {35, L"Range lower bound", std::to_wstring(invalidBounds.getLowerBound())},
       {35, L"Range upper bound", std::to_wstring(invalidBounds.getUpperBound())}},
      L"InvalidRangeBound",
      1,
      "SetRangeBounds InvalidRangeBound service report",
      5);
  auto const quotedAbsentDimension =
      std::wstring{L"\""} + absentDimension.toString() + L"\"";
  requirePortableServiceReportFailure(
      observer,
      reports.at(2U),
      reportClass,
      reportParameters,
      L"SetRangeBounds",
      {{42, L"Region handle", quotedRegion},
       {10, L"Dimension handle", quotedAbsentDimension},
       {35, L"Range lower bound", std::to_wstring(lowerBound)},
       {35, L"Range upper bound", std::to_wstring(upperBound)}},
      L"RegionDoesNotContainSpecifiedDimension",
      2,
      "SetRangeBounds RegionDoesNotContainSpecifiedDimension service report",
      5);
  auto const quotedInvalidRegion =
      std::wstring{L"\""} + invalidRegion.toString() + L"\"";
  requirePortableServiceReportFailure(
      observer,
      reports.at(3U),
      reportClass,
      reportParameters,
      L"SetRangeBounds",
      {{42, L"Region handle", quotedInvalidRegion},
       {10, L"Dimension handle", quotedDimension},
       {35, L"Range lower bound", std::to_wstring(lowerBound)},
       {35, L"Range upper bound", std::to_wstring(upperBound)}},
      L"InvalidRegion",
      3,
      "SetRangeBounds InvalidRegion service report",
      5);
  auto const reportedFederate = observer.rtiAmbassador().decodeFederateHandle(
      report.parameters.at(reportParameters.at(7U)));
  require(
      reportedFederate == subject.federateHandle(),
      "SetRangeBounds service report identified the wrong joined federate");
  auto const failureFederate = observer.rtiAmbassador().decodeFederateHandle(
      reports.at(1U).parameters.at(reportParameters.at(7U)));
  require(
      failureFederate == subject.federateHandle(),
      "SetRangeBounds failure service report identified the wrong joined federate");
  auto const absentDimensionFailureFederate =
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(2U).parameters.at(reportParameters.at(7U)));
  require(
      absentDimensionFailureFederate == subject.federateHandle(),
      "SetRangeBounds absent-dimension report identified the wrong joined federate");
  auto const invalidRegionFailureFederate =
      observer.rtiAmbassador().decodeFederateHandle(
          reports.at(3U).parameters.at(reportParameters.at(7U)));
  require(
      invalidRegionFailureFederate == subject.federateHandle(),
      "SetRangeBounds invalid-region report identified the wrong joined federate");

  auto const observerDimension = observer.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      observerDimension.isValid(),
      "SetRangeBounds foreign-region setup returned an invalid dimension handle");
  auto const foreignRegion = observer.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{observerDimension});
  require(
      foreignRegion.isValid(),
      "SetRangeBounds foreign-region setup returned an invalid region handle");
  auto const observerUpperBound =
      observer.rtiAmbassador().getDimensionUpperBound(observerDimension);
  observer.rtiAmbassador().setRangeBounds(
      foreignRegion,
      observerDimension,
      rti::RangeBounds{0UL, observerUpperBound});
  observer.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{foreignRegion});

  bool foreignRegionRejected = false;
  try {
    subject.rtiAmbassador().setRangeBounds(
        foreignRegion,
        dimension,
        rti::RangeBounds{lowerBound, upperBound});
  } catch (rti::RegionNotCreatedByThisFederate const&) {
    foreignRegionRejected = true;
  }
  require(
      foreignRegionRejected,
      "SetRangeBounds accepted a region created by another joined federate");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 5U; },
      options,
      "failed SetRangeBounds RegionNotCreatedByThisFederate service-report callback");
  auto const reportsAfterForeignFailure = observer.recorder().interactions();
  require(
      reportsAfterForeignFailure.size() == 5U,
      "SetRangeBounds success and failures delivered an unexpected number of service reports");
  auto const quotedForeignRegion =
      std::wstring{L"\""} + foreignRegion.toString() + L"\"";
  requirePortableServiceReportFailure(
      observer,
      reportsAfterForeignFailure.at(4U),
      reportClass,
      reportParameters,
      L"SetRangeBounds",
      {{42, L"Region handle", quotedForeignRegion},
       {10, L"Dimension handle", quotedDimension},
       {35, L"Range lower bound", std::to_wstring(lowerBound)},
       {35, L"Range upper bound", std::to_wstring(upperBound)}},
      L"RegionNotCreatedByThisFederate",
      4,
      "SetRangeBounds RegionNotCreatedByThisFederate service report",
      5);
  auto const foreignFailureFederate =
      observer.rtiAmbassador().decodeFederateHandle(
          reportsAfterForeignFailure.at(4U).parameters.at(reportParameters.at(7U)));
  require(
      foreignFailureFederate == subject.federateHandle(),
      "SetRangeBounds foreign-region failure report identified the wrong joined federate");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().commitRegionModifications(rti::RegionHandleSet{region});
  auto const updatedBounds = subject.rtiAmbassador().getRangeBounds(region, dimension);
  require(
      updatedBounds.getLowerBound() == lowerBound &&
          updatedBounds.getUpperBound() == upperBound,
      "SetRangeBounds did not retain the adapter-valid bounds after CommitRegionModifications");
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().deleteRegion(region);
  observer.rtiAmbassador().deleteRegion(foreignRegion);
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

  auto const observerDimension = observer.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      observerDimension.isValid(),
      "CommitRegionModifications foreign-region setup returned an invalid dimension handle");
  auto const foreignRegion = observer.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{observerDimension});
  require(
      foreignRegion.isValid(),
      "CommitRegionModifications foreign-region setup returned an invalid region handle");
  auto const observerUpperBound =
      observer.rtiAmbassador().getDimensionUpperBound(observerDimension);
  observer.rtiAmbassador().setRangeBounds(
      foreignRegion,
      observerDimension,
      rti::RangeBounds{0UL, observerUpperBound});
  observer.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{foreignRegion});
  rti::RegionHandleSet const foreignRegions{foreignRegion};
  bool foreignRegionRejected = false;
  try {
    subject.rtiAmbassador().commitRegionModifications(foreignRegions);
  } catch (rti::RegionNotCreatedByThisFederate const&) {
    foreignRegionRejected = true;
  }
  require(
      foreignRegionRejected,
      "CommitRegionModifications accepted a region created by another joined federate");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 2U; },
      options,
      "failed CommitRegionModifications RegionNotCreatedByThisFederate service-report callback");
  auto const reportsAfterFailure = observer.recorder().interactions();
  require(
      reportsAfterFailure.size() == 2U,
      "CommitRegionModifications success and failure delivered an unexpected number of service reports");
  auto const foreignRegionSetText =
      std::wstring{L"[\""} + foreignRegion.toString() + L"\"]";
  requirePortableServiceReportFailure(
      observer,
      reportsAfterFailure.at(1U),
      reportClass,
      reportParameters,
      L"CommitRegionModifications",
      {{43, L"Set of region designators", foreignRegionSetText}},
      L"RegionNotCreatedByThisFederate",
      1,
      "CommitRegionModifications RegionNotCreatedByThisFederate service report",
      5);
  auto const failureFederate = observer.rtiAmbassador().decodeFederateHandle(
      reportsAfterFailure.at(1U).parameters.at(reportParameters.at(7U)));
  require(
      failureFederate == subject.federateHandle(),
      "CommitRegionModifications failure service report identified the wrong joined federate");

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
  observer.rtiAmbassador().deleteRegion(foreignRegion);
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

  bool repeatedDeleteRejected = false;
  try {
    subject.rtiAmbassador().deleteRegion(region);
  } catch (rti::InvalidRegion const&) {
    repeatedDeleteRejected = true;
  }
  require(
      repeatedDeleteRejected,
      "DeleteRegion did not reject the already-deleted region with InvalidRegion");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 2U; },
      options,
      "failed DeleteRegion InvalidRegion service-report callback");
  auto const reportsAfterFailure = observer.recorder().interactions();
  require(
      reportsAfterFailure.size() == 2U,
      "DeleteRegion success and failure delivered an unexpected number of service reports");
  requirePortableServiceReportFailure(
      observer,
      reportsAfterFailure.at(1U),
      reportClass,
      reportParameters,
      L"DeleteRegion",
      {{42, L"Region designator", quotedRegion}},
      L"InvalidRegion",
      1,
      "DeleteRegion InvalidRegion service report",
      5);
  auto const failureFederate = observer.rtiAmbassador().decodeFederateHandle(
      reportsAfterFailure.at(1U).parameters.at(reportParameters.at(7U)));
  require(
      failureFederate == subject.federateHandle(),
      "DeleteRegion failure service report identified the wrong joined federate");

  auto const observerDimension = observer.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      observerDimension.isValid(),
      "DeleteRegion foreign-region setup returned an invalid dimension handle");
  auto const foreignRegion = observer.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{observerDimension});
  require(
      foreignRegion.isValid(),
      "DeleteRegion foreign-region setup returned an invalid region handle");
  bool foreignRegionRejected = false;
  try {
    subject.rtiAmbassador().deleteRegion(foreignRegion);
  } catch (rti::RegionNotCreatedByThisFederate const&) {
    foreignRegionRejected = true;
  }
  require(
      foreignRegionRejected,
      "DeleteRegion accepted a region created by another joined federate");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 3U; },
      options,
      "failed DeleteRegion RegionNotCreatedByThisFederate service-report callback");
  auto const reportsAfterForeignFailure = observer.recorder().interactions();
  require(
      reportsAfterForeignFailure.size() == 3U,
      "DeleteRegion success and failures delivered an unexpected number of service reports");
  auto const quotedForeignRegion =
      std::wstring{L"\""} + foreignRegion.toString() + L"\"";
  requirePortableServiceReportFailure(
      observer,
      reportsAfterForeignFailure.at(2U),
      reportClass,
      reportParameters,
      L"DeleteRegion",
      {{42, L"Region designator", quotedForeignRegion}},
      L"RegionNotCreatedByThisFederate",
      2,
      "DeleteRegion RegionNotCreatedByThisFederate service report",
      5);
  auto const foreignFailureFederate =
      observer.rtiAmbassador().decodeFederateHandle(
          reportsAfterForeignFailure.at(2U).parameters.at(reportParameters.at(7U)));
  require(
      foreignFailureFederate == subject.federateHandle(),
      "DeleteRegion foreign-region failure report identified the wrong joined federate");

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  bool deletedRegionQueryRejected = false;
  try {
    static_cast<void>(subject.rtiAmbassador().getDimensionHandleSet(region));
  } catch (rti::InvalidRegion const&) {
    deletedRegionQueryRejected = true;
  }
  require(
      deletedRegionQueryRejected,
      "GetDimensionHandleSet did not reject the deleted region with InvalidRegion");
  require(
      observer.recorder().interactions().size() == reportsAfterForeignFailure.size(),
      "disabled service reporting delivered a GetDimensionHandleSet failure report");
  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.resign(rti::NO_ACTION);
  observer.rtiAmbassador().deleteRegion(foreignRegion);
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

  rti::DimensionHandle const invalidDimension;
  require(
      !invalidDimension.isValid(),
      "CreateRegion failure setup did not produce an invalid dimension handle");
  rti::DimensionHandleSet const invalidDimensions{invalidDimension};
  bool invalidDimensionRejected = false;
  try {
    static_cast<void>(subject.rtiAmbassador().createRegion(invalidDimensions));
  } catch (rti::InvalidDimensionHandle const&) {
    invalidDimensionRejected = true;
  }
  require(
      invalidDimensionRejected,
      "CreateRegion accepted a DimensionHandleSet containing an invalid handle");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 2U; },
      options,
      "failed CreateRegion InvalidDimensionHandle service-report callback");
  auto const reportsAfterFailure = observer.recorder().interactions();
  require(
      reportsAfterFailure.size() == 2U,
      "CreateRegion success and failure delivered an unexpected number of service reports");
  auto const invalidDimensionSetValue =
      std::wstring{L"[\""} + invalidDimension.toString() + L"\"]";
  requirePortableServiceReportFailure(
      observer,
      reportsAfterFailure.at(1U),
      reportClass,
      reportParameters,
      L"CreateRegion",
      {{11, L"Set of dimension designators", invalidDimensionSetValue}},
      L"InvalidDimensionHandle",
      1,
      "CreateRegion InvalidDimensionHandle service report",
      5);
  auto const failureFederate = observer.rtiAmbassador().decodeFederateHandle(
      reportsAfterFailure.at(1U).parameters.at(reportParameters.at(7U)));
  require(
      failureFederate == subject.federateHandle(),
      "CreateRegion failure service report identified the wrong joined federate");

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
      std::vector<std::wstring>{options.modelFom.wstring()},
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

  auto const observerDimension = observer.rtiAmbassador().getDimensionHandle(
      options.fomDimensionName);
  require(
      observerDimension.isValid(),
      "Associate Regions for Updates foreign-region setup returned an invalid dimension handle");
  auto const foreignRegion = observer.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{observerDimension});
  require(
      foreignRegion.isValid(),
      "Associate Regions for Updates foreign-region setup returned an invalid region handle");
  auto const observerUpperBound =
      observer.rtiAmbassador().getDimensionUpperBound(observerDimension);
  observer.rtiAmbassador().setRangeBounds(
      foreignRegion,
      observerDimension,
      rti::RangeBounds{0UL, observerUpperBound});
  observer.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{foreignRegion});

  rti::AttributeHandleSetRegionHandleSetPairVector const foreignAssociation{
      {attributes, rti::RegionHandleSet{foreignRegion}},
  };
  bool foreignRegionRejected = false;
  try {
    subject.rtiAmbassador().associateRegionsForUpdates(object, foreignAssociation);
  } catch (rti::RegionNotCreatedByThisFederate const&) {
    foreignRegionRejected = true;
  }
  require(
      foreignRegionRejected,
      "AssociateRegionsForUpdates accepted a region created by another joined federate");
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 2U; },
      options,
      "failed AssociateRegionsForUpdates RegionNotCreatedByThisFederate service-report callback");
  auto const reportsAfterFailure = observer.recorder().interactions();
  require(
      reportsAfterFailure.size() == 2U,
      "AssociateRegionsForUpdates success and failure delivered an unexpected number of service reports");
  auto const foreignAssociationValue =
      std::wstring{L"[{\"attributeHandleSet\":["} +
      quoted(attribute.toString()) + L"],\"regionHandleSet\":[" +
      quoted(foreignRegion.toString()) + L"]}]";
  requirePortableServiceReportFailure(
      observer,
      reportsAfterFailure.at(1U),
      reportClass,
      reportParameters,
      L"AssociateRegionsForUpdates",
      {{37, L"Object instance designator", quoted(object.toString())},
       {4,
        L"Collection of attribute designator set and region designator set pairs",
        foreignAssociationValue}},
      L"RegionNotCreatedByThisFederate",
      1,
      "AssociateRegionsForUpdates RegionNotCreatedByThisFederate service report",
      5);
  auto const failureFederate = observer.rtiAmbassador().decodeFederateHandle(
      reportsAfterFailure.at(1U).parameters.at(reportParameters.at(7U)));
  require(
      failureFederate == subject.federateHandle(),
      "AssociateRegionsForUpdates failure service report identified the wrong joined federate");

  subject.rtiAmbassador().unassociateRegionsForUpdates(object, association);
  waitFor(
      subject,
      observer,
      [&] { return observer.recorder().interactions().size() >= 3U; },
      options,
      "Unassociate Regions for Updates service-report callback");
  auto const reportsAfterUnassociation = observer.recorder().interactions();
  require(
      reportsAfterUnassociation.size() == 3U,
      "Associate/Unassociate Regions for Updates delivered an unexpected number of service reports");
  requirePortableServiceReport(
      observer,
      reportsAfterUnassociation.at(2U),
      reportClass,
      reportParameters,
      L"UnassociateRegionsForUpdates",
      {{37, L"Object instance designator", quoted(object.toString())},
       {4,
        L"Collection of attribute designator set and region designator set pairs",
        associationValue}},
      {34, L"", L"null"},
      2,
      "Unassociate Regions for Updates service report",
      5);

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().deleteRegion(region);
  subject.rtiAmbassador().deleteRegion(secondRegion);
  observer.rtiAmbassador().deleteRegion(foreignRegion);
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
