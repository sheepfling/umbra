#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioFederateMomReportAddedSubclassNotSent(
    Options const& options,
    rti::CallbackModel model) {
  auto const extensionFom = options.scenarioFoms.find(
      "mom-report-added-subclass-not-sent");
  require(
      extensionFom != options.scenarioFoms.end() &&
          !extensionFom->second.empty() && !options.fom.empty() &&
          !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "MOM report-subclass TCK requires its extension FOM, application FOM, standard MIM, and logical-time configuration");

  Session publisher(options, model, "mom-report-subclass-publisher");
  Session parentObserver(options, model, "mom-report-parent-observer");
  Session extensionObserver(options, model, "mom-report-extension-observer");
  auto const federation = federationName(
      options, "federate-mom-report-added-subclass-not-sent");
  publisher.connect();
  parentObserver.connect();
  extensionObserver.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{
          options.fom.wstring(), extensionFom->second.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-report-subclass-publisher",
      options.federateType,
      federation);
  parentObserver.join(
      options.memberFederateName + L"-report-parent-observer",
      options.federateType,
      federation);
  extensionObserver.join(
      options.memberFederateName + L"-report-extension-observer",
      options.federateType,
      federation);

  constexpr auto reportName =
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
  constexpr auto extensionReportName =
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation.TckReportServiceInvocationExtension";
  auto const parentReportClass =
      parentObserver.rtiAmbassador().getInteractionClassHandle(reportName);
  auto const extensionReportClass =
      extensionObserver.rtiAmbassador().getInteractionClassHandle(
          extensionReportName);
  require(
      parentReportClass.isValid() && extensionReportClass.isValid(),
      "The standard MOM report class or FOM-added report subclass is unavailable");
  parentObserver.rtiAmbassador().subscribeInteractionClass(parentReportClass);
  extensionObserver.rtiAmbassador().subscribeInteractionClass(
      extensionReportClass);

  auto const applicationInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const applicationParameter = publisher.rtiAmbassador().getParameterHandle(
      applicationInteraction, options.parameterName);
  require(
      applicationInteraction.isValid() && applicationParameter.isValid(),
      "The adapter application interaction or parameter is unavailable");
  publisher.rtiAmbassador().publishInteractionClass(applicationInteraction);
  publisher.rtiAmbassador().setServiceReportingSwitch(false);
  parentObserver.recorder().clearInteraction();
  extensionObserver.recorder().clearInteraction();
  publisher.rtiAmbassador().setServiceReportingSwitch(true);

  std::array<unsigned char, 1U> const payload{0x53U};
  rti::ParameterHandleValueMap const values{{
      applicationParameter,
      rti::VariableLengthData{payload.data(), payload.size()},
  }};
  publisher.rtiAmbassador().sendInteraction(
      applicationInteraction, values, rti::VariableLengthData{});
  waitFor(
      parentObserver,
      [&] { return !parentObserver.recorder().interactions().empty(); },
      options,
      "the standard HLAreportServiceInvocation callback");

  if (model == rti::HLA_EVOKED) {
    while (extensionObserver.rtiAmbassador().evokeCallback(0.0)) {
    }
  }
  auto const parentReports = parentObserver.recorder().interactions();
  require(
      parentReports.size() == 1U &&
          parentReports.front().interaction == parentReportClass,
      "The RTI did not send exactly one report at the predefined HLAreportServiceInvocation class");
  auto const extensionReports = extensionObserver.recorder().interactions();
  require(
      std::none_of(
          extensionReports.begin(),
          extensionReports.end(),
          [&](InteractionRecord const& report) {
            return report.interaction == extensionReportClass;
          }),
      "The RTI sent an HLAreportServiceInvocation interaction using the FOM-added subclass");

  publisher.resign(rti::NO_ACTION);
  extensionObserver.resign(rti::NO_ACTION);
  parentObserver.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  extensionObserver.disconnect();
  parentObserver.disconnect();
}

void scenarioFederateMomReportAddedSubclassFullInteraction(
    Options const& options,
    rti::CallbackModel model) {
  auto const extensionFom = options.scenarioFoms.find(
      "mom-report-added-subclass-not-sent");
  require(
      extensionFom != options.scenarioFoms.end() &&
          !extensionFom->second.empty() && !options.fom.empty() &&
          !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "MOM report-subclass delivery TCK requires its extension FOM, application FOM, standard MIM, and logical-time configuration");

  Session publisher(options, model, "mom-report-subclass-full-publisher");
  Session subscriber(options, model, "mom-report-subclass-full-subscriber");
  auto const federation = federationName(
      options, "federate-mom-report-added-subclass-full-interaction");
  auto const publisherName =
      options.ownerFederateName + L"-report-subclass-full-publisher";
  publisher.connect();
  subscriber.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{
          options.fom.wstring(), extensionFom->second.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(publisherName, options.federateType, federation);
  subscriber.join(
      options.memberFederateName + L"-report-subclass-full-subscriber",
      options.federateType,
      federation);

  constexpr auto extensionReportName =
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation.TckReportServiceInvocationExtension";
  auto const publisherClass =
      publisher.rtiAmbassador().getInteractionClassHandle(extensionReportName);
  auto const subscriberClass =
      subscriber.rtiAmbassador().getInteractionClassHandle(extensionReportName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          publisherClass == subscriberClass,
      "The publishable FOM-added HLAreportServiceInvocation subclass was unavailable or inconsistent");

  constexpr std::array<std::wstring_view, 9U> parameterNames{
      L"HLAfederate",
      L"HLAservice",
      L"HLAserviceType",
      L"HLAsuccessIndicator",
      L"HLAsuppliedArguments",
      L"HLAreturnedArgument",
      L"HLAexception",
      L"HLAserialNumber",
      L"TckReportServiceInvocationExtensionPayload",
  };
  std::array<rti::ParameterHandle, parameterNames.size()> publisherParameters;
  std::array<rti::ParameterHandle, parameterNames.size()> subscriberParameters;
  for (std::size_t index = 0U; index < parameterNames.size(); ++index) {
    auto const parameterName = std::wstring{parameterNames[index]};
    publisherParameters[index] = publisher.rtiAmbassador().getParameterHandle(
        publisherClass, parameterName);
    subscriberParameters[index] = subscriber.rtiAmbassador().getParameterHandle(
        subscriberClass, parameterName);
    require(
        publisherParameters[index].isValid() &&
            subscriberParameters[index].isValid() &&
            publisherParameters[index] == subscriberParameters[index],
        "A predefined or extension report parameter handle was unavailable or inconsistent");
  }

  subscriber.rtiAmbassador().subscribeInteractionClass(subscriberClass);
  publisher.rtiAmbassador().publishInteractionClass(publisherClass);
  auto const publisherHandle =
      publisher.rtiAmbassador().getFederateHandle(publisherName);
  require(publisherHandle.isValid(), "The interaction publisher handle was invalid");

  rti::HLAunicodeString const service{L"TckAddedSubclassDeliveryProbe"};
  rti::HLAinteger16BE const serviceType{6};
  rti::HLAboolean const success{true};
  rti::HLAvariableArray const suppliedArguments{rti::HLAfixedRecord{}};
  rti::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti::HLAinteger32BE{34})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  rti::HLAunicodeString const exception{};
  rti::HLAinteger32BE const serialNumber{0};
  std::array<unsigned char, 1U> const extensionPayload{0xD4U};
  rti::ParameterHandleValueMap const sentParameters{
      {publisherParameters[0], publisherHandle.encode()},
      {publisherParameters[1], service.encode()},
      {publisherParameters[2], serviceType.encode()},
      {publisherParameters[3], success.encode()},
      {publisherParameters[4], suppliedArguments.encode()},
      {publisherParameters[5], returnedArgument.encode()},
      {publisherParameters[6], exception.encode()},
      {publisherParameters[7], serialNumber.encode()},
      {publisherParameters[8],
       rti::VariableLengthData{extensionPayload.data(), extensionPayload.size()}},
  };
  std::array<unsigned char, 3U> const tagBytes{0x54U, 0x43U, 0x4BU};
  rti::VariableLengthData const tag{tagBytes.data(), tagBytes.size()};
  publisher.rtiAmbassador().sendInteraction(publisherClass, sentParameters, tag);

  waitFor(
      subscriber,
      [&] { return !subscriber.recorder().interactions().empty(); },
      options,
      "the full FOM-added report-subclass interaction");
  auto const received = subscriber.recorder().interactions();
  require(
      received.size() == 1U && received.front().interaction == subscriberClass,
      "The subclass subscriber did not receive exactly one interaction at the FOM-added class");
  auto const& interaction = received.front();
  require(
      interaction.producer == publisherHandle &&
          interaction.transportation ==
              subscriber.rtiAmbassador().getTransportationTypeHandle(
                  L"HLAreliable") &&
          interaction.tag == copyBytes(tag) &&
          interaction.parameters.size() == sentParameters.size(),
      "The received extension interaction lost its publisher, reliable transport, tag, or parameter-map cardinality");
  for (std::size_t index = 0U; index < parameterNames.size(); ++index) {
    auto const sent = sentParameters.find(publisherParameters[index]);
    auto const delivered = interaction.parameters.find(subscriberParameters[index]);
    require(
        sent != sentParameters.end() && delivered != interaction.parameters.end() &&
            copyBytes(sent->second) == copyBytes(delivered->second),
        "The subclass subscriber did not receive a predefined or extension parameter byte-for-byte");
  }
  require(
      subscriber.rtiAmbassador().decodeFederateHandle(
          interaction.parameters.at(subscriberParameters[0])) == publisherHandle,
      "The full subclass interaction did not preserve its inherited HLAfederate parameter");
  rti::HLAunicodeString receivedService;
  receivedService.decode(interaction.parameters.at(subscriberParameters[1]));
  rti::HLAinteger16BE receivedServiceType;
  receivedServiceType.decode(interaction.parameters.at(subscriberParameters[2]));
  rti::HLAboolean receivedSuccess;
  receivedSuccess.decode(interaction.parameters.at(subscriberParameters[3]));
  rti::HLAvariableArray receivedSuppliedArguments{rti::HLAfixedRecord{}};
  receivedSuppliedArguments.decode(
      interaction.parameters.at(subscriberParameters[4]));
  rti::HLAfixedRecord receivedReturnedArgument;
  receivedReturnedArgument.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  receivedReturnedArgument.decode(
      interaction.parameters.at(subscriberParameters[5]));
  rti::HLAunicodeString receivedException;
  receivedException.decode(interaction.parameters.at(subscriberParameters[6]));
  rti::HLAinteger32BE receivedSerialNumber;
  receivedSerialNumber.decode(interaction.parameters.at(subscriberParameters[7]));
  require(
      receivedService.get() == L"TckAddedSubclassDeliveryProbe" &&
          receivedServiceType.get() == 6 && receivedSuccess.get() &&
          receivedSuppliedArguments.size() == 0U &&
          dynamic_cast<rti::HLAinteger32BE const&>(
              receivedReturnedArgument.get(0U)).get() == 34 &&
          receivedException.get().empty() && receivedSerialNumber.get() == 0 &&
          copyBytes(interaction.parameters.at(subscriberParameters[8])) ==
              std::vector<std::uint8_t>{0xD4U},
      "The full extension interaction did not preserve the decoded standard and added parameter values");

  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

void scenarioFederateMomReportAddedSubclassPromotionDropsExtensionParameter(
    Options const& options,
    rti::CallbackModel model) {
  auto const extensionFom = options.scenarioFoms.find(
      "mom-report-added-subclass-not-sent");
  require(
      extensionFom != options.scenarioFoms.end() &&
          !extensionFom->second.empty() && !options.fom.empty() &&
          !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "MOM report-subclass promotion TCK requires its extension FOM, application FOM, standard MIM, and logical-time configuration");

  Session publisher(options, model, "mom-report-subclass-promotion-publisher");
  Session parentSubscriber(options, model, "mom-report-parent-class-subscriber");
  auto const federation = federationName(
      options,
      "federate-mom-report-added-subclass-parent-promotion-drops-extension-parameter");
  auto const publisherName =
      options.ownerFederateName + L"-report-subclass-promotion-publisher";
  publisher.connect();
  parentSubscriber.connect();
  publisher.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{
          options.fom.wstring(), extensionFom->second.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(publisherName, options.federateType, federation);
  parentSubscriber.join(
      options.memberFederateName + L"-report-parent-class-subscriber",
      options.federateType,
      federation);

  constexpr auto parentReportName =
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
  constexpr auto extensionReportName =
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation.TckReportServiceInvocationExtension";
  auto const publisherClass =
      publisher.rtiAmbassador().getInteractionClassHandle(extensionReportName);
  auto const subscriberClass =
      parentSubscriber.rtiAmbassador().getInteractionClassHandle(parentReportName);
  auto const subscriberExtensionClass = parentSubscriber.rtiAmbassador()
      .getInteractionClassHandle(extensionReportName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          subscriberExtensionClass.isValid() &&
          publisherClass == subscriberExtensionClass &&
          publisherClass != subscriberClass,
      "The standard report class and FOM-added report subclass were unavailable or inconsistent");

  constexpr std::array<std::wstring_view, 9U> parameterNames{
      L"HLAfederate",
      L"HLAservice",
      L"HLAserviceType",
      L"HLAsuccessIndicator",
      L"HLAsuppliedArguments",
      L"HLAreturnedArgument",
      L"HLAexception",
      L"HLAserialNumber",
      L"TckReportServiceInvocationExtensionPayload",
  };
  std::array<rti::ParameterHandle, parameterNames.size()> publisherParameters;
  std::array<rti::ParameterHandle, parameterNames.size() - 1U> parentParameters;
  for (std::size_t index = 0U; index < parameterNames.size(); ++index) {
    auto const parameterName = std::wstring{parameterNames[index]};
    publisherParameters[index] = publisher.rtiAmbassador().getParameterHandle(
        publisherClass, parameterName);
    require(
        publisherParameters[index].isValid(),
        "A predefined or extension report parameter was unavailable to the publisher");
    if (index + 1U < parameterNames.size()) {
      parentParameters[index] = parentSubscriber.rtiAmbassador().getParameterHandle(
          subscriberClass, parameterName);
      require(
          parentParameters[index].isValid() &&
              publisherParameters[index] == parentParameters[index],
          "An inherited report parameter handle was unavailable or inconsistent");
    }
  }
  auto const subscriberExtensionParameter = parentSubscriber.rtiAmbassador()
      .getParameterHandle(
          subscriberExtensionClass,
          std::wstring{parameterNames.back()});
  require(
      subscriberExtensionParameter.isValid(),
      "The extension-only parameter handle was unavailable to the parent subscriber");

  parentSubscriber.rtiAmbassador().subscribeInteractionClass(subscriberClass);
  publisher.rtiAmbassador().publishInteractionClass(publisherClass);
  auto const publisherHandle =
      publisher.rtiAmbassador().getFederateHandle(publisherName);
  require(publisherHandle.isValid(), "The interaction publisher handle was invalid");

  rti::HLAunicodeString const service{L"TckAddedSubclassPromotionProbe"};
  rti::HLAinteger16BE const serviceType{6};
  rti::HLAboolean const success{true};
  rti::HLAvariableArray const suppliedArguments{rti::HLAfixedRecord{}};
  rti::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti::HLAinteger32BE{34})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  rti::HLAunicodeString const exception{};
  rti::HLAinteger32BE const serialNumber{1};
  std::array<unsigned char, 1U> const extensionPayload{0xA9U};
  rti::ParameterHandleValueMap const sentParameters{
      {publisherParameters[0], publisherHandle.encode()},
      {publisherParameters[1], service.encode()},
      {publisherParameters[2], serviceType.encode()},
      {publisherParameters[3], success.encode()},
      {publisherParameters[4], suppliedArguments.encode()},
      {publisherParameters[5], returnedArgument.encode()},
      {publisherParameters[6], exception.encode()},
      {publisherParameters[7], serialNumber.encode()},
      {publisherParameters[8],
       rti::VariableLengthData{extensionPayload.data(), extensionPayload.size()}},
  };
  std::array<unsigned char, 3U> const tagBytes{0x50U, 0x52U, 0x4DU};
  rti::VariableLengthData const tag{tagBytes.data(), tagBytes.size()};
  publisher.rtiAmbassador().sendInteraction(publisherClass, sentParameters, tag);

  waitFor(
      parentSubscriber,
      [&] { return !parentSubscriber.recorder().interactions().empty(); },
      options,
      "the subclass interaction promoted to the predefined parent class");
  auto const received = parentSubscriber.recorder().interactions();
  require(
      received.size() == 1U && received.front().interaction == subscriberClass,
      "The parent-class subscriber did not receive exactly one promoted interaction at its subscribed class");
  auto const& interaction = received.front();
  require(
      interaction.producer == publisherHandle &&
          interaction.transportation ==
              parentSubscriber.rtiAmbassador().getTransportationTypeHandle(
                  L"HLAreliable") &&
          interaction.tag == copyBytes(tag) &&
          interaction.parameters.size() == parentParameters.size() &&
          interaction.parameters.find(subscriberExtensionParameter) ==
              interaction.parameters.end(),
      "The promoted interaction retained the subclass class/extension parameter or lost its publisher, transport, or tag");
  for (std::size_t index = 0U; index < parentParameters.size(); ++index) {
    auto const sent = sentParameters.find(publisherParameters[index]);
    auto const delivered = interaction.parameters.find(parentParameters[index]);
    require(
        sent != sentParameters.end() && delivered != interaction.parameters.end() &&
            copyBytes(sent->second) == copyBytes(delivered->second),
        "Parent-class promotion did not preserve an inherited report parameter");
  }
  require(
      parentSubscriber.rtiAmbassador().decodeFederateHandle(
          interaction.parameters.at(parentParameters[0])) == publisherHandle,
      "The promoted interaction did not preserve its inherited HLAfederate parameter");
  rti::HLAunicodeString receivedService;
  receivedService.decode(interaction.parameters.at(parentParameters[1]));
  rti::HLAinteger16BE receivedServiceType;
  receivedServiceType.decode(interaction.parameters.at(parentParameters[2]));
  rti::HLAboolean receivedSuccess;
  receivedSuccess.decode(interaction.parameters.at(parentParameters[3]));
  rti::HLAvariableArray receivedSuppliedArguments{rti::HLAfixedRecord{}};
  receivedSuppliedArguments.decode(interaction.parameters.at(parentParameters[4]));
  rti::HLAfixedRecord receivedReturnedArgument;
  receivedReturnedArgument.appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAunicodeString{})
      .appendElement(rti::HLAunicodeString{});
  receivedReturnedArgument.decode(interaction.parameters.at(parentParameters[5]));
  rti::HLAunicodeString receivedException;
  receivedException.decode(interaction.parameters.at(parentParameters[6]));
  rti::HLAinteger32BE receivedSerialNumber;
  receivedSerialNumber.decode(interaction.parameters.at(parentParameters[7]));
  require(
      receivedService.get() == L"TckAddedSubclassPromotionProbe" &&
          receivedServiceType.get() == 6 && receivedSuccess.get() &&
          receivedSuppliedArguments.size() == 0U &&
          dynamic_cast<rti::HLAinteger32BE const&>(
              receivedReturnedArgument.get(0U)).get() == 34 &&
          receivedException.get().empty() && receivedSerialNumber.get() == 1,
      "The promoted interaction did not preserve the complete inherited report parameter values");

  parentSubscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  parentSubscriber.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
