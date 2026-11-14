#include "tck_portable_member_control_scenarios.hpp"
#include "tck_service_report_file_support.hpp"

#include <filesystem>
#include <string>
#include <type_traits>
#include <vector>

namespace hla_rti_cpp_tck::detail {

static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::setObjectClassRelevanceAdvisorySwitch),
        void (rti::RTIambassador::*)(bool)>,
    "IEEE 1516.1-2025 Set Object Class Relevance Advisory Switch takes bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::setAttributeRelevanceAdvisorySwitch),
        void (rti::RTIambassador::*)(bool)>,
    "IEEE 1516.1-2025 Set Attribute Relevance Advisory Switch takes bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::getAttributeScopeAdvisorySwitch),
        bool (rti::RTIambassador::*)() const>,
    "IEEE 1516.1-2025 Get Attribute Scope Advisory Switch returns bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::setAttributeScopeAdvisorySwitch),
        void (rti::RTIambassador::*)(bool)>,
    "IEEE 1516.1-2025 Set Attribute Scope Advisory Switch takes bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::getInteractionRelevanceAdvisorySwitch),
        bool (rti::RTIambassador::*)() const>,
    "IEEE 1516.1-2025 Get Interaction Relevance Advisory Switch returns bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::setInteractionRelevanceAdvisorySwitch),
        void (rti::RTIambassador::*)(bool)>,
    "IEEE 1516.1-2025 Set Interaction Relevance Advisory Switch takes bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::getConveyRegionDesignatorSetsSwitch),
        bool (rti::RTIambassador::*)() const>,
    "IEEE 1516.1-2025 Get Convey Region Designator Sets Switch returns bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::setConveyRegionDesignatorSetsSwitch),
        void (rti::RTIambassador::*)(bool)>,
    "IEEE 1516.1-2025 Set Convey Region Designator Sets Switch takes bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::getAutomaticResignDirective),
        rti::ResignAction (rti::RTIambassador::*)()>,
    "IEEE 1516.1-2025 Get Automatic Resign Directive returns ResignAction");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::setAutomaticResignDirective),
        void (rti::RTIambassador::*)(rti::ResignAction)>,
    "IEEE 1516.1-2025 Set Automatic Resign Directive takes ResignAction");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::getServiceReportingSwitch),
        bool (rti::RTIambassador::*)() const>,
    "IEEE 1516.1-2025 Get Service Reporting Switch returns bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::setServiceReportingSwitch),
        void (rti::RTIambassador::*)(bool)>,
    "IEEE 1516.1-2025 Set Service Reporting Switch takes bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::getExceptionReportingSwitch),
        bool (rti::RTIambassador::*)() const>,
    "IEEE 1516.1-2025 Get Exception Reporting Switch returns bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::setExceptionReportingSwitch),
        void (rti::RTIambassador::*)(bool)>,
    "IEEE 1516.1-2025 Set Exception Reporting Switch takes bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::getSendServiceReportsToFileSwitch),
        bool (rti::RTIambassador::*)() const>,
    "IEEE 1516.1-2025 Get Send Service Reports to File Switch returns bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::setSendServiceReportsToFileSwitch),
        void (rti::RTIambassador::*)(bool)>,
    "IEEE 1516.1-2025 Set Send Service Reports to File Switch takes bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::getAutoProvideSwitch),
        bool (rti::RTIambassador::*)() const>,
    "IEEE 1516.1-2025 Get Auto Provide Switch returns bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::getDelaySubscriptionEvaluationSwitch),
        bool (rti::RTIambassador::*)() const>,
    "IEEE 1516.1-2025 Get Delay Subscription Evaluation Switch returns bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::getAllowRelaxedDDMSwitch),
        bool (rti::RTIambassador::*)() const>,
    "IEEE 1516.1-2025 Get Allow Relaxed DDM Switch returns bool");
static_assert(
    std::is_same_v<
        decltype(&rti::RTIambassador::getNonRegulatedGrantSwitch),
        bool (rti::RTIambassador::*)() const>,
    "IEEE 1516.1-2025 Get Non Regulated Grant Switch returns bool");

std::filesystem::path const& supportSwitchGetterLifecycleFom(
    Options const& options) {
  auto const fixture =
      options.scenarioFoms.find("support-switch-getter-lifecycle");
  require(
      fixture != options.scenarioFoms.end() && !fixture->second.empty(),
      "Support-switch getter lifecycle testing requires its adapter-supplied isolated FOM");
  return fixture->second;
}

void scenarioSupportSwitchGetterLifecycleBoundariesPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.serviceReportDirectory.empty(),
      "Send Service Reports to File getter testing requires a configured report directory");

  Session subject(options, model, "support-switch-getters-lifecycle-subject");
  auto const federation =
      federationName(options, "support-switch-getter-lifecycle-boundaries");
  std::vector<std::wstring> fomModules{
      supportSwitchGetterLifecycleFom(options).wstring()};
  constexpr bool allowRelaxedDdmEnabled = true;
  service_report_file_support::connectWithServiceReportDirectory(subject, options);
  subject.rtiAmbassador().createFederationExecution(
      federation,
      fomModules,
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
  bool const initialServiceReporting = ambassador.getServiceReportingSwitch();
  ambassador.setServiceReportingSwitch(false);
  require(
      !ambassador.getServiceReportingSwitch(),
      "Service Reporting getter did not return FALSE when disabled");
  ambassador.setServiceReportingSwitch(true);
  require(
      ambassador.getServiceReportingSwitch(),
      "Service Reporting getter did not return TRUE when enabled");
  ambassador.setServiceReportingSwitch(initialServiceReporting);
  require(
      ambassador.getServiceReportingSwitch() == initialServiceReporting,
      "Service Reporting getter did not return the restored value");
  bool const initialExceptionReporting =
      ambassador.getExceptionReportingSwitch();
  ambassador.setExceptionReportingSwitch(false);
  require(
      !ambassador.getExceptionReportingSwitch(),
      "Exception Reporting getter did not return FALSE when disabled");
  ambassador.setExceptionReportingSwitch(true);
  require(
      ambassador.getExceptionReportingSwitch(),
      "Exception Reporting getter did not return TRUE when enabled");
  ambassador.setExceptionReportingSwitch(initialExceptionReporting);
  require(
      ambassador.getExceptionReportingSwitch() == initialExceptionReporting,
      "Exception Reporting getter did not return the restored value");
  bool const initialFileReporting =
      ambassador.getSendServiceReportsToFileSwitch();
  ambassador.setSendServiceReportsToFileSwitch(false);
  require(
      !ambassador.getSendServiceReportsToFileSwitch(),
      "Send Service Reports to File getter did not return FALSE when disabled");
  ambassador.setSendServiceReportsToFileSwitch(true);
  require(
      ambassador.getSendServiceReportsToFileSwitch(),
      "Send Service Reports to File getter did not return TRUE when enabled");
  ambassador.setSendServiceReportsToFileSwitch(initialFileReporting);
  require(
      ambassador.getSendServiceReportsToFileSwitch() == initialFileReporting,
      "Send Service Reports to File getter did not return the restored value");
  static_cast<void>(ambassador.getAutoProvideSwitch());
  static_cast<void>(ambassador.getDelaySubscriptionEvaluationSwitch());
  static_cast<void>(ambassador.getAdvisoriesUseKnownClassSwitch());
  require(
      ambassador.getAllowRelaxedDDMSwitch() == allowRelaxedDdmEnabled,
      "Get Allow Relaxed DDM Switch did not match the explicit FOM configuration");
  require(
      ambassador.getNonRegulatedGrantSwitch() == allowRelaxedDdmEnabled,
      "Get Non Regulated Grant Switch did not match the explicit FOM configuration");

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
  require(
      !options.serviceReportDirectory.empty(),
      "Send Service Reports to File setter testing requires a configured report directory");

  Session subject(options, model, "support-switch-setters-lifecycle-subject");
  auto const federation =
      federationName(options, "support-switch-setter-lifecycle-boundaries");
  service_report_file_support::connectWithServiceReportDirectory(subject, options);
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-support-switch-setter-lifecycle-subject",
      options.federateType,
      federation);

  auto& ambassador = subject.rtiAmbassador();
  auto requireBooleanValues = [&](auto&& getter,
                                  auto&& setter,
                                  char const* switchName) {
    bool const initialValue = getter();
    setter(false);
    require(
        !getter(),
        std::string(switchName) + " setter did not apply FALSE");
    setter(true);
    require(
        getter(),
        std::string(switchName) + " setter did not apply TRUE");
    setter(initialValue);
    require(
        getter() == initialValue,
        std::string(switchName) + " setter did not restore the original value");
  };
  requireBooleanValues(
      [&] { return ambassador.getObjectClassRelevanceAdvisorySwitch(); },
      [&](bool value) { ambassador.setObjectClassRelevanceAdvisorySwitch(value); },
      "Object Class Relevance Advisory");
  requireBooleanValues(
      [&] { return ambassador.getAttributeRelevanceAdvisorySwitch(); },
      [&](bool value) { ambassador.setAttributeRelevanceAdvisorySwitch(value); },
      "Attribute Relevance Advisory");
  requireBooleanValues(
      [&] { return ambassador.getAttributeScopeAdvisorySwitch(); },
      [&](bool value) { ambassador.setAttributeScopeAdvisorySwitch(value); },
      "Attribute Scope Advisory");
  requireBooleanValues(
      [&] { return ambassador.getInteractionRelevanceAdvisorySwitch(); },
      [&](bool value) { ambassador.setInteractionRelevanceAdvisorySwitch(value); },
      "Interaction Relevance Advisory");
  requireBooleanValues(
      [&] { return ambassador.getConveyRegionDesignatorSetsSwitch(); },
      [&](bool value) { ambassador.setConveyRegionDesignatorSetsSwitch(value); },
      "Convey Region Designator Sets");
  requireBooleanValues(
      [&] { return ambassador.getServiceReportingSwitch(); },
      [&](bool value) { ambassador.setServiceReportingSwitch(value); },
      "Service Reporting");
  requireBooleanValues(
      [&] { return ambassador.getExceptionReportingSwitch(); },
      [&](bool value) { ambassador.setExceptionReportingSwitch(value); },
      "Exception Reporting");
  requireBooleanValues(
      [&] { return ambassador.getSendServiceReportsToFileSwitch(); },
      [&](bool value) { ambassador.setSendServiceReportsToFileSwitch(value); },
      "Send Service Reports to File");

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
  Session member(options, model, "advisory-switch-values-member");
  member.connect();
  member.join(
      options.memberFederateName + L"-advisory-switch",
      options.federateType,
      federation);

  auto& ambassador = subject.rtiAmbassador();
  auto& memberAmbassador = member.rtiAmbassador();
  bool const memberObjectClassRelevance =
      memberAmbassador.getObjectClassRelevanceAdvisorySwitch();
  bool const memberAttributeRelevance =
      memberAmbassador.getAttributeRelevanceAdvisorySwitch();
  bool const memberAttributeScope =
      memberAmbassador.getAttributeScopeAdvisorySwitch();
  bool const memberInteractionRelevance =
      memberAmbassador.getInteractionRelevanceAdvisorySwitch();
  bool const memberConveyRegionDesignatorSets =
      memberAmbassador.getConveyRegionDesignatorSetsSwitch();
  bool const originalObjectClassRelevance =
      ambassador.getObjectClassRelevanceAdvisorySwitch();
  ambassador.setObjectClassRelevanceAdvisorySwitch(false);
  require(
      !ambassador.getObjectClassRelevanceAdvisorySwitch(),
      "Object Class Relevance Advisory switch getter did not return false");
  require(
      memberAmbassador.getObjectClassRelevanceAdvisorySwitch() ==
          memberObjectClassRelevance,
      "changing the subject's Object Class Relevance Advisory switch changed the member's value");
  ambassador.setObjectClassRelevanceAdvisorySwitch(true);
  require(
      ambassador.getObjectClassRelevanceAdvisorySwitch(),
      "Object Class Relevance Advisory switch getter did not return true");
  require(
      memberAmbassador.getObjectClassRelevanceAdvisorySwitch() ==
          memberObjectClassRelevance,
      "enabling the subject's Object Class Relevance Advisory switch changed the member's value");
  ambassador.setObjectClassRelevanceAdvisorySwitch(originalObjectClassRelevance);
  require(
      ambassador.getObjectClassRelevanceAdvisorySwitch() ==
          originalObjectClassRelevance,
      "Object Class Relevance Advisory switch getter did not return the restored value");

  bool const originalAttributeRelevance =
      ambassador.getAttributeRelevanceAdvisorySwitch();
  ambassador.setAttributeRelevanceAdvisorySwitch(false);
  require(
      !ambassador.getAttributeRelevanceAdvisorySwitch(),
      "Attribute Relevance Advisory switch getter did not return false when disabled");
  require(
      memberAmbassador.getAttributeRelevanceAdvisorySwitch() ==
          memberAttributeRelevance,
      "changing the subject's Attribute Relevance Advisory switch changed the member's value");
  ambassador.setAttributeRelevanceAdvisorySwitch(true);
  require(
      ambassador.getAttributeRelevanceAdvisorySwitch(),
      "Attribute Relevance Advisory switch getter did not return true when enabled");
  require(
      memberAmbassador.getAttributeRelevanceAdvisorySwitch() ==
          memberAttributeRelevance,
      "enabling the subject's Attribute Relevance Advisory switch changed the member's value");
  ambassador.setAttributeRelevanceAdvisorySwitch(originalAttributeRelevance);
  require(
      ambassador.getAttributeRelevanceAdvisorySwitch() ==
          originalAttributeRelevance,
      "Attribute Relevance Advisory switch getter did not return the restored value");

  bool const originalAttributeScope =
      ambassador.getAttributeScopeAdvisorySwitch();
  ambassador.setAttributeScopeAdvisorySwitch(false);
  require(
      !ambassador.getAttributeScopeAdvisorySwitch(),
      "Attribute Scope Advisory getter did not return FALSE when disabled");
  require(
      memberAmbassador.getAttributeScopeAdvisorySwitch() == memberAttributeScope,
      "changing the subject's Attribute Scope Advisory switch changed the member's value");
  ambassador.setAttributeScopeAdvisorySwitch(true);
  require(
      ambassador.getAttributeScopeAdvisorySwitch(),
      "Attribute Scope Advisory getter did not return TRUE when enabled");
  require(
      memberAmbassador.getAttributeScopeAdvisorySwitch() == memberAttributeScope,
      "enabling the subject's Attribute Scope Advisory switch changed the member's value");
  ambassador.setAttributeScopeAdvisorySwitch(originalAttributeScope);
  require(
      ambassador.getAttributeScopeAdvisorySwitch() == originalAttributeScope,
      "Attribute Scope Advisory getter did not return the restored value");
  bool const originalInteractionRelevance =
      ambassador.getInteractionRelevanceAdvisorySwitch();
  ambassador.setInteractionRelevanceAdvisorySwitch(false);
  require(
      !ambassador.getInteractionRelevanceAdvisorySwitch(),
      "Interaction Relevance Advisory getter did not return FALSE when disabled");
  require(
      memberAmbassador.getInteractionRelevanceAdvisorySwitch() ==
          memberInteractionRelevance,
      "changing the subject's Interaction Relevance Advisory switch changed the member's value");
  ambassador.setInteractionRelevanceAdvisorySwitch(true);
  require(
      ambassador.getInteractionRelevanceAdvisorySwitch(),
      "Interaction Relevance Advisory getter did not return TRUE when enabled");
  require(
      memberAmbassador.getInteractionRelevanceAdvisorySwitch() ==
          memberInteractionRelevance,
      "enabling the subject's Interaction Relevance Advisory switch changed the member's value");
  ambassador.setInteractionRelevanceAdvisorySwitch(originalInteractionRelevance);
  require(
      ambassador.getInteractionRelevanceAdvisorySwitch() ==
          originalInteractionRelevance,
      "Interaction Relevance Advisory getter did not return the restored value");

  bool const originalConveyRegionDesignatorSets =
      ambassador.getConveyRegionDesignatorSetsSwitch();
  ambassador.setConveyRegionDesignatorSetsSwitch(false);
  require(
      !ambassador.getConveyRegionDesignatorSetsSwitch(),
      "Convey Region Designator Sets getter did not return FALSE when disabled");
  require(
      memberAmbassador.getConveyRegionDesignatorSetsSwitch() ==
          memberConveyRegionDesignatorSets,
      "changing the subject's Convey Region Designator Sets switch changed the member's value");
  ambassador.setConveyRegionDesignatorSetsSwitch(true);
  require(
      ambassador.getConveyRegionDesignatorSetsSwitch(),
      "Convey Region Designator Sets getter did not return TRUE when enabled");
  require(
      memberAmbassador.getConveyRegionDesignatorSetsSwitch() ==
          memberConveyRegionDesignatorSets,
      "enabling the subject's Convey Region Designator Sets switch changed the member's value");
  ambassador.setConveyRegionDesignatorSetsSwitch(
      originalConveyRegionDesignatorSets);
  require(
      ambassador.getConveyRegionDesignatorSetsSwitch() ==
          originalConveyRegionDesignatorSets,
      "Convey Region Designator Sets getter did not return the restored value");

  subject.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
  ambassador.destroyFederationExecution(federation);
  subject.disconnect();
  member.disconnect();
}

void scenarioAdvisorySwitchValueRoundTripsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAdvisorySwitchValueRoundTripsPortable(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
