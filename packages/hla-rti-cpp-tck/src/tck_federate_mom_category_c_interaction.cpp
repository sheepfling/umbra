#include "tck_main_scenarios.hpp"

#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioFederateMomCategoryCInteraction(
    Options const& options,
    rti::CallbackModel model) {
  auto const scenarioFom = options.scenarioFoms.find(
      "mom-category-c-interaction");
  require(
      scenarioFom != options.scenarioFoms.end() &&
          !scenarioFom->second.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Category-c MOM interaction TCK requires its extension FOM, standard MIM, and logical-time configuration");

  Session publisher(options, model, "mom-category-c-publisher");
  Session observer(options, model, "mom-category-c-observer");
  auto const federation =
      federationName(options, "federate-mom-category-c-interaction");
  publisher.connect();
  observer.connect();

  std::vector<std::wstring> fomModules;
  if (!options.fom.empty()) {
    fomModules.push_back(options.fom.wstring());
  }
  fomModules.push_back(scenarioFom->second.wstring());
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      fomModules,
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-mom-category-c-publisher",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-mom-category-c-observer",
      options.federateType,
      federation);

  constexpr auto interactionName =
      L"HLAinteractionRoot.HLAmanager.HLAfederate.TckCategoryCInteraction";
  auto const publisherClass =
      publisher.rtiAmbassador().getInteractionClassHandle(interactionName);
  auto const observerClass =
      observer.rtiAmbassador().getInteractionClassHandle(interactionName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherClass, L"HLAserviceReporting");
  auto const observerParameter = observer.rtiAmbassador().getParameterHandle(
      observerClass, L"HLAserviceReporting");
  require(
      publisherClass.isValid() && observerClass.isValid() &&
          publisherParameter.isValid() && observerParameter.isValid(),
      "The FOM-added category-c interaction or its switch-shaped parameter is unavailable");
  publisher.rtiAmbassador().publishInteractionClass(publisherClass);
  observer.rtiAmbassador().subscribeInteractionClass(observerClass);
  require(
      !publisher.rtiAmbassador().getServiceReportingSwitch(),
      "The publisher's predefined HLAserviceReporting switch did not start false");

  auto const switchShapedValue = rti::HLAinteger32BE{1}.encode();
  rti::ParameterHandleValueMap const parameters{
      {publisherParameter, switchShapedValue},
  };
  publisher.rtiAmbassador().sendInteraction(
      publisherClass, parameters, rti::VariableLengthData{});
  waitFor(
      observer,
      [&] { return !observer.recorder().interactions().empty(); },
      options,
      "the federate-originated category-c interaction");
  if (model == rti::HLA_EVOKED) {
    while (observer.rtiAmbassador().evokeCallback(0.0)) {
    }
  }

  auto const interactions = observer.recorder().interactions();
  require(
      interactions.size() == 1U &&
          interactions.front().interaction == observerClass &&
          interactions.front().producer == publisher.federateHandle() &&
          interactions.front().parameters.size() == 1U &&
          interactions.front().parameters.count(observerParameter) == 1U,
      "The RTI did not route exactly the original category-c interaction from its joined publisher");
  rti::HLAinteger32BE receivedSwitchShapedValue;
  receivedSwitchShapedValue.decode(
      interactions.front().parameters.at(observerParameter));
  require(
      receivedSwitchShapedValue.get() == 1,
      "The RTI changed the category-c interaction's switch-shaped payload while routing it");
  require(
      !publisher.rtiAmbassador().getServiceReportingSwitch(),
      "The RTI processed an ignored category-c interaction as predefined HLAsetSwitches");

  observer.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
