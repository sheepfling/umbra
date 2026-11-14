#include "tck_language_api_transparency.hpp"

#include "tck_harness.hpp"

#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

std::vector<std::uint8_t> bytes(std::string const& text) {
  std::vector<std::uint8_t> result;
  result.reserve(text.size());
  for (char character : text) {
    result.push_back(static_cast<std::uint8_t>(character));
  }
  return result;
}

}  // namespace

void scenarioLanguageApiTransparency(
    Options const& options,
    rti::CallbackModel callbackModel) {
  auto const federation = federationName(options, "language-api-transparency");
  Session cppPeer(options, callbackModel, "language-transparency-cpp-peer");
  cppPeer.connect();
  cppPeer.join(
      L"cpp-tck-language-peer", options.federateType, federation);

  auto const interaction =
      cppPeer.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const parameter = cppPeer.rtiAmbassador().getParameterHandle(
      interaction, options.parameterName);
  auto const javaPeer = cppPeer.rtiAmbassador().getFederateHandle(
      L"java-tck-language-peer");
  require(
      interaction.isValid() && parameter.isValid() && javaPeer.isValid() &&
          cppPeer.rtiAmbassador().getInteractionClassName(interaction) ==
              options.interactionClassName &&
          cppPeer.rtiAmbassador().getParameterName(interaction, parameter) ==
              options.parameterName &&
          cppPeer.rtiAmbassador().getFederateName(javaPeer) ==
              L"java-tck-language-peer",
      "Java API peer did not resolve through the C++ API's standard federation lookups");

  cppPeer.rtiAmbassador().publishInteractionClass(interaction);
  cppPeer.rtiAmbassador().subscribeInteractionClass(interaction, true);

  std::string const model =
      callbackModel == rti::HLA_EVOKED ? "HLA_EVOKED" : "HLA_IMMEDIATE";
  std::string const request = "CPP-TO-JAVA:" + model;
  std::string const requestTag = "cpp-tag:" + model;
  auto const requestBytes = bytes(request);
  auto const requestTagBytes = bytes(requestTag);
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      parameter,
      rti::VariableLengthData(requestBytes.data(), requestBytes.size()));
  cppPeer.rtiAmbassador().sendInteraction(
      interaction,
      parameters,
      rti::VariableLengthData(requestTagBytes.data(), requestTagBytes.size()));

  waitFor(
      cppPeer,
      [&] { return cppPeer.recorder().interactions().size() == 1U; },
      options,
      "Java-to-C++ response in the mixed-language federation");
  auto const received = cppPeer.recorder().interactions();
  auto const expectedResponse = bytes("JAVA-TO-CPP:" + model);
  auto const expectedResponseTag = bytes("java-tag:" + model);
  require(
      received.size() == 1U && received.front().interaction == interaction &&
          received.front().parameters.size() == 1U &&
          received.front().parameters.count(parameter) == 1U &&
          copyBytes(received.front().parameters.at(parameter)) ==
              expectedResponse &&
          received.front().tag == expectedResponseTag &&
          received.front().producer == javaPeer &&
          cppPeer.rtiAmbassador().getFederateName(received.front().producer) ==
              L"java-tck-language-peer",
      "Java API reply or its federate identity did not round-trip through the C++ API");

  cppPeer.rtiAmbassador().unsubscribeInteractionClass(interaction);
  cppPeer.rtiAmbassador().unpublishInteractionClass(interaction);
  cppPeer.resign(rti::NO_ACTION);
  cppPeer.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
