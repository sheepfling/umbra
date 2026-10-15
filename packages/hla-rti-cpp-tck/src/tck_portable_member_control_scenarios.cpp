#include "tck_portable_member_control_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioSupportSwitchGetterLifecycleBoundariesPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Support-switch getter lifecycle testing requires an adapter-supplied FOM");

  Session subject(options, model, "support-switch-getters-lifecycle-subject");
  auto const federation =
      federationName(options, "support-switch-getter-lifecycle-boundaries");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-support-switch-getter-lifecycle-subject",
      options.federateType,
      federation);

  auto& ambassador = subject.rtiAmbassador();
  static_cast<void>(ambassador.getObjectClassRelevanceAdvisorySwitch());
  static_cast<void>(ambassador.getAttributeRelevanceAdvisorySwitch());
  static_cast<void>(ambassador.getAttributeScopeAdvisorySwitch());
  static_cast<void>(ambassador.getInteractionRelevanceAdvisorySwitch());
  static_cast<void>(ambassador.getConveyRegionDesignatorSetsSwitch());
  static_cast<void>(ambassador.getServiceReportingSwitch());
  static_cast<void>(ambassador.getExceptionReportingSwitch());
  static_cast<void>(ambassador.getSendServiceReportsToFileSwitch());
  static_cast<void>(ambassador.getAutoProvideSwitch());
  static_cast<void>(ambassador.getDelaySubscriptionEvaluationSwitch());
  static_cast<void>(ambassador.getAdvisoriesUseKnownClassSwitch());
  static_cast<void>(ambassador.getAllowRelaxedDDMSwitch());
  static_cast<void>(ambassador.getNonRegulatedGrantSwitch());

  auto requireBoundary = [&](auto&& getter,
                             wchar_t const* exceptionName,
                             char const* context) {
    requireException(getter, exceptionName, context);
  };
  subject.resign(rti::NO_ACTION);
  requireBoundary(
      [&] { static_cast<void>(ambassador.getObjectClassRelevanceAdvisorySwitch()); },
      L"FederateNotExecutionMember",
      "reading getObjectClassRelevanceAdvisorySwitch after resignation");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getAttributeRelevanceAdvisorySwitch()); },
      L"FederateNotExecutionMember",
      "reading getAttributeRelevanceAdvisorySwitch after resignation");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getAttributeScopeAdvisorySwitch()); },
      L"FederateNotExecutionMember",
      "reading getAttributeScopeAdvisorySwitch after resignation");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getInteractionRelevanceAdvisorySwitch()); },
      L"FederateNotExecutionMember",
      "reading getInteractionRelevanceAdvisorySwitch after resignation");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getConveyRegionDesignatorSetsSwitch()); },
      L"FederateNotExecutionMember",
      "reading getConveyRegionDesignatorSetsSwitch after resignation");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getServiceReportingSwitch()); },
      L"FederateNotExecutionMember",
      "reading getServiceReportingSwitch after resignation");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getExceptionReportingSwitch()); },
      L"FederateNotExecutionMember",
      "reading getExceptionReportingSwitch after resignation");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getSendServiceReportsToFileSwitch()); },
      L"FederateNotExecutionMember",
      "reading getSendServiceReportsToFileSwitch after resignation");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getAutoProvideSwitch()); },
      L"FederateNotExecutionMember",
      "reading getAutoProvideSwitch after resignation");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getDelaySubscriptionEvaluationSwitch()); },
      L"FederateNotExecutionMember",
      "reading getDelaySubscriptionEvaluationSwitch after resignation");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getAdvisoriesUseKnownClassSwitch()); },
      L"FederateNotExecutionMember",
      "reading getAdvisoriesUseKnownClassSwitch after resignation");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getAllowRelaxedDDMSwitch()); },
      L"FederateNotExecutionMember",
      "reading getAllowRelaxedDDMSwitch after resignation");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getNonRegulatedGrantSwitch()); },
      L"FederateNotExecutionMember",
      "reading getNonRegulatedGrantSwitch after resignation");

  ambassador.destroyFederationExecution(federation);
  subject.disconnect();
  requireBoundary(
      [&] { static_cast<void>(ambassador.getObjectClassRelevanceAdvisorySwitch()); },
      L"NotConnected",
      "reading getObjectClassRelevanceAdvisorySwitch after disconnect");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getAttributeRelevanceAdvisorySwitch()); },
      L"NotConnected",
      "reading getAttributeRelevanceAdvisorySwitch after disconnect");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getAttributeScopeAdvisorySwitch()); },
      L"NotConnected",
      "reading getAttributeScopeAdvisorySwitch after disconnect");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getInteractionRelevanceAdvisorySwitch()); },
      L"NotConnected",
      "reading getInteractionRelevanceAdvisorySwitch after disconnect");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getConveyRegionDesignatorSetsSwitch()); },
      L"NotConnected",
      "reading getConveyRegionDesignatorSetsSwitch after disconnect");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getServiceReportingSwitch()); },
      L"NotConnected",
      "reading getServiceReportingSwitch after disconnect");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getExceptionReportingSwitch()); },
      L"NotConnected",
      "reading getExceptionReportingSwitch after disconnect");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getSendServiceReportsToFileSwitch()); },
      L"NotConnected",
      "reading getSendServiceReportsToFileSwitch after disconnect");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getAutoProvideSwitch()); },
      L"NotConnected",
      "reading getAutoProvideSwitch after disconnect");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getDelaySubscriptionEvaluationSwitch()); },
      L"NotConnected",
      "reading getDelaySubscriptionEvaluationSwitch after disconnect");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getAdvisoriesUseKnownClassSwitch()); },
      L"NotConnected",
      "reading getAdvisoriesUseKnownClassSwitch after disconnect");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getAllowRelaxedDDMSwitch()); },
      L"NotConnected",
      "reading getAllowRelaxedDDMSwitch after disconnect");
  requireBoundary(
      [&] { static_cast<void>(ambassador.getNonRegulatedGrantSwitch()); },
      L"NotConnected",
      "reading getNonRegulatedGrantSwitch after disconnect");
}

void scenarioSupportSwitchGetterLifecycleBoundariesContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioSupportSwitchGetterLifecycleBoundariesPortable(options, model);
}

void scenarioSupportSwitchSetterLifecycleBoundariesPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Support-switch setter lifecycle testing requires an adapter-supplied FOM");

  Session subject(options, model, "support-switch-setters-lifecycle-subject");
  auto const federation =
      federationName(options, "support-switch-setter-lifecycle-boundaries");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-support-switch-setter-lifecycle-subject",
      options.federateType,
      federation);

  auto& ambassador = subject.rtiAmbassador();
  auto requireAccepted = [&](auto&& getter, auto&& setter, char const* context) {
    auto const value = getter();
    setter(value);
    require(getter() == value, context);
  };
  requireAccepted(
      [&] { return ambassador.getObjectClassRelevanceAdvisorySwitch(); },
      [&](bool value) { ambassador.setObjectClassRelevanceAdvisorySwitch(value); },
      "Object Class Relevance Advisory setter did not preserve its observed value");
  requireAccepted(
      [&] { return ambassador.getAttributeRelevanceAdvisorySwitch(); },
      [&](bool value) { ambassador.setAttributeRelevanceAdvisorySwitch(value); },
      "Attribute Relevance Advisory setter did not preserve its observed value");
  requireAccepted(
      [&] { return ambassador.getAttributeScopeAdvisorySwitch(); },
      [&](bool value) { ambassador.setAttributeScopeAdvisorySwitch(value); },
      "Attribute Scope Advisory setter did not preserve its observed value");
  requireAccepted(
      [&] { return ambassador.getInteractionRelevanceAdvisorySwitch(); },
      [&](bool value) { ambassador.setInteractionRelevanceAdvisorySwitch(value); },
      "Interaction Relevance Advisory setter did not preserve its observed value");
  requireAccepted(
      [&] { return ambassador.getConveyRegionDesignatorSetsSwitch(); },
      [&](bool value) { ambassador.setConveyRegionDesignatorSetsSwitch(value); },
      "Convey Region Designator Sets setter did not preserve its observed value");
  requireAccepted(
      [&] { return ambassador.getServiceReportingSwitch(); },
      [&](bool value) { ambassador.setServiceReportingSwitch(value); },
      "Service Reporting setter did not preserve its observed value");
  requireAccepted(
      [&] { return ambassador.getExceptionReportingSwitch(); },
      [&](bool value) { ambassador.setExceptionReportingSwitch(value); },
      "Exception Reporting setter did not preserve its observed value");
  requireAccepted(
      [&] { return ambassador.getSendServiceReportsToFileSwitch(); },
      [&](bool value) { ambassador.setSendServiceReportsToFileSwitch(value); },
      "Send Service Reports to File setter did not preserve its observed value");

  auto requireBoundary = [&](auto&& setter,
                             wchar_t const* exceptionName,
                             char const* context) {
    requireException([&] { setter(false); }, exceptionName, context);
  };
  subject.resign(rti::NO_ACTION);
  requireBoundary(
      [&](bool value) { ambassador.setObjectClassRelevanceAdvisorySwitch(value); },
      L"FederateNotExecutionMember",
      "setting Object Class Relevance Advisory after resignation");
  requireBoundary(
      [&](bool value) { ambassador.setAttributeRelevanceAdvisorySwitch(value); },
      L"FederateNotExecutionMember",
      "setting Attribute Relevance Advisory after resignation");
  requireBoundary(
      [&](bool value) { ambassador.setAttributeScopeAdvisorySwitch(value); },
      L"FederateNotExecutionMember",
      "setting Attribute Scope Advisory after resignation");
  requireBoundary(
      [&](bool value) { ambassador.setInteractionRelevanceAdvisorySwitch(value); },
      L"FederateNotExecutionMember",
      "setting Interaction Relevance Advisory after resignation");
  requireBoundary(
      [&](bool value) { ambassador.setConveyRegionDesignatorSetsSwitch(value); },
      L"FederateNotExecutionMember",
      "setting Convey Region Designator Sets after resignation");
  requireBoundary(
      [&](bool value) { ambassador.setServiceReportingSwitch(value); },
      L"FederateNotExecutionMember",
      "setting Service Reporting after resignation");
  requireBoundary(
      [&](bool value) { ambassador.setExceptionReportingSwitch(value); },
      L"FederateNotExecutionMember",
      "setting Exception Reporting after resignation");
  requireBoundary(
      [&](bool value) { ambassador.setSendServiceReportsToFileSwitch(value); },
      L"FederateNotExecutionMember",
      "setting Send Service Reports to File after resignation");

  ambassador.destroyFederationExecution(federation);
  subject.disconnect();
  requireBoundary(
      [&](bool value) { ambassador.setObjectClassRelevanceAdvisorySwitch(value); },
      L"NotConnected",
      "setting Object Class Relevance Advisory after disconnect");
  requireBoundary(
      [&](bool value) { ambassador.setAttributeRelevanceAdvisorySwitch(value); },
      L"NotConnected",
      "setting Attribute Relevance Advisory after disconnect");
  requireBoundary(
      [&](bool value) { ambassador.setAttributeScopeAdvisorySwitch(value); },
      L"NotConnected",
      "setting Attribute Scope Advisory after disconnect");
  requireBoundary(
      [&](bool value) { ambassador.setInteractionRelevanceAdvisorySwitch(value); },
      L"NotConnected",
      "setting Interaction Relevance Advisory after disconnect");
  requireBoundary(
      [&](bool value) { ambassador.setConveyRegionDesignatorSetsSwitch(value); },
      L"NotConnected",
      "setting Convey Region Designator Sets after disconnect");
  requireBoundary(
      [&](bool value) { ambassador.setServiceReportingSwitch(value); },
      L"NotConnected",
      "setting Service Reporting after disconnect");
  requireBoundary(
      [&](bool value) { ambassador.setExceptionReportingSwitch(value); },
      L"NotConnected",
      "setting Exception Reporting after disconnect");
  requireBoundary(
      [&](bool value) { ambassador.setSendServiceReportsToFileSwitch(value); },
      L"NotConnected",
      "setting Send Service Reports to File after disconnect");
}

void scenarioSupportSwitchSetterLifecycleBoundariesContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioSupportSwitchSetterLifecycleBoundariesPortable(options, model);
}

void scenarioAutomaticResignDirectiveLifecycleBoundariesPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Automatic-resign-directive lifecycle testing requires an adapter-supplied FOM");

  Session subject(options, model, "automatic-resign-directive-lifecycle-subject");
  auto const federation =
      federationName(options, "automatic-resign-directive-lifecycle-boundaries");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-resign-directive",
      options.federateType,
      federation);

  auto& ambassador = subject.rtiAmbassador();
  ambassador.setAutomaticResignDirective(rti::DELETE_OBJECTS);
  require(
      ambassador.getAutomaticResignDirective() == rti::DELETE_OBJECTS,
      "Automatic resign directive did not round-trip while joined");
  ambassador.setAutomaticResignDirective(rti::NO_ACTION);

  auto requireBoundary = [&](auto&& operation,
                             wchar_t const* exceptionName,
                             char const* context) {
    requireException(operation, exceptionName, context);
  };
  subject.resign(rti::NO_ACTION);
  requireBoundary(
      [&] { static_cast<void>(ambassador.getAutomaticResignDirective()); },
      L"FederateNotExecutionMember",
      "reading the automatic resign directive after resignation");
  requireBoundary(
      [&] { ambassador.setAutomaticResignDirective(rti::NO_ACTION); },
      L"FederateNotExecutionMember",
      "setting the automatic resign directive after resignation");

  ambassador.destroyFederationExecution(federation);
  subject.disconnect();
  requireBoundary(
      [&] { static_cast<void>(ambassador.getAutomaticResignDirective()); },
      L"NotConnected",
      "reading the automatic resign directive after disconnect");
  requireBoundary(
      [&] { ambassador.setAutomaticResignDirective(rti::NO_ACTION); },
      L"NotConnected",
      "setting the automatic resign directive after disconnect");
}

void scenarioAutomaticResignDirectiveLifecycleBoundariesContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAutomaticResignDirectiveLifecycleBoundariesPortable(options, model);
}

void scenarioAutomaticResignDirectiveEnumValuesPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Automatic-resign-directive value testing requires an adapter-supplied FOM");

  Session subject(options, model, "automatic-resign-directive-values-subject");
  auto const federation =
      federationName(options, "automatic-resign-directive-standard-values");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-resign-values",
      options.federateType,
      federation);

  auto& ambassador = subject.rtiAmbassador();
  rti::ResignAction const validActions[] = {
      rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES,
      rti::DELETE_OBJECTS,
      rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS,
      rti::DELETE_OBJECTS_THEN_DIVEST,
      rti::CANCEL_THEN_DELETE_THEN_DIVEST,
      rti::NO_ACTION};
  for (rti::ResignAction action : validActions) {
    ambassador.setAutomaticResignDirective(action);
    require(
        ambassador.getAutomaticResignDirective() == action,
        "A standard ResignAction value did not round-trip through its getter and setter");
  }

  auto const invalidAction = static_cast<rti::ResignAction>(
      static_cast<int>(rti::NO_ACTION) + 1);
  requireException(
      [&] { ambassador.setAutomaticResignDirective(invalidAction); },
      L"InvalidResignAction",
      "setting a value outside the standard ResignAction enumerators");

  ambassador.setAutomaticResignDirective(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  ambassador.destroyFederationExecution(federation);
  subject.disconnect();
}

void scenarioAutomaticResignDirectiveEnumValuesContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAutomaticResignDirectiveEnumValuesPortable(options, model);
}

void scenarioAdvisorySwitchValueRoundTripsPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Advisory-switch value testing requires an adapter-supplied FOM");

  Session subject(options, model, "advisory-switch-values-subject");
  auto const federation =
      federationName(options, "advisory-switch-standard-value-round-trips");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-advisory-switch",
      options.federateType,
      federation);

  auto& ambassador = subject.rtiAmbassador();
  auto requireToggle = [&](auto&& getter,
                           auto&& setter,
                           char const* context) {
    bool const original = getter();
    bool const changed = !original;
    setter(changed);
    require(getter() == changed, context);
    setter(original);
    require(getter() == original, context);
  };
  requireToggle(
      [&] { return ambassador.getObjectClassRelevanceAdvisorySwitch(); },
      [&](bool value) { ambassador.setObjectClassRelevanceAdvisorySwitch(value); },
      "Object Class Relevance Advisory switch did not follow the value set");
  requireToggle(
      [&] { return ambassador.getAttributeRelevanceAdvisorySwitch(); },
      [&](bool value) { ambassador.setAttributeRelevanceAdvisorySwitch(value); },
      "Attribute Relevance Advisory switch did not follow the value set");
  requireToggle(
      [&] { return ambassador.getAttributeScopeAdvisorySwitch(); },
      [&](bool value) { ambassador.setAttributeScopeAdvisorySwitch(value); },
      "Attribute Scope Advisory switch did not follow the value set");
  requireToggle(
      [&] { return ambassador.getInteractionRelevanceAdvisorySwitch(); },
      [&](bool value) { ambassador.setInteractionRelevanceAdvisorySwitch(value); },
      "Interaction Relevance Advisory switch did not follow the value set");

  subject.resign(rti::NO_ACTION);
  ambassador.destroyFederationExecution(federation);
  subject.disconnect();
}

void scenarioAdvisorySwitchValueRoundTripsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAdvisorySwitchValueRoundTripsPortable(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
