#include "tck_fom_module_scenarios.hpp"

#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& directedReferenceFom(Options const& options,
                                                  std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Directed reference resolution requires an adapter-supplied FOM");
  return found->second;
}

}  // namespace

void scenarioFomDirectedReferenceResolution(Options const& options,
                                            rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Directed reference resolution requires adapter-supplied logical time");
  auto const& objectFom = directedReferenceFom(options, "directed-reference-object");
  auto const& interactionFom =
      directedReferenceFom(options, "directed-reference-interaction");
  auto const& missing = directedReferenceFom(options, "directed-reference-missing");

  Session sender(options, model, "directed-reference-sender");
  Session receiver(options, model, "directed-reference-receiver");
  auto const federation = federationName(options, "fom-directed-reference-resolution");
  sender.connect();
  receiver.connect();
  auto const create = [&](std::vector<std::filesystem::path> const& modules) {
    std::vector<std::wstring> paths;
    for (auto const& module : modules) paths.push_back(module.wstring());
    sender.rtiAmbassador().createFederationExecution(
        federation, paths, options.logicalTimeImplementationName);
  };
  create({objectFom, interactionFom});
  sender.join(L"tck-directed-reference-sender", options.federateType, federation);
  receiver.join(L"tck-directed-reference-receiver", options.federateType, federation);
  auto& sending = sender.rtiAmbassador();
  auto& receiving = receiver.rtiAmbassador();
  auto const className = std::wstring{L"HLAobjectRoot.TckDirectedReferenceObject"};
  auto const interactionName =
      std::wstring{L"HLAinteractionRoot.TckDirectedReferenceInteraction"};
  auto const senderClass = sending.getObjectClassHandle(className);
  auto const receiverClass = receiving.getObjectClassHandle(className);
  auto const senderInteraction = sending.getInteractionClassHandle(interactionName);
  auto const receiverInteraction = receiving.getInteractionClassHandle(interactionName);
  auto const marker = sending.getAttributeHandle(senderClass, L"Marker");
  require(senderClass.isValid() && senderClass == receiverClass &&
              senderInteraction.isValid() && senderInteraction == receiverInteraction &&
              marker.isValid(),
          "Later FOM module did not resolve the directed interaction and target class");

  rti::AttributeHandleSet const attributes{marker};
  rti::InteractionClassHandleSet const interactions{senderInteraction};
  sending.publishObjectClassAttributes(senderClass, attributes);
  receiving.subscribeObjectClassAttributes(receiverClass, attributes, true, L"");
  sending.publishObjectClassDirectedInteractions(senderClass, interactions);
  receiving.subscribeObjectClassDirectedInteractions(receiverClass, interactions, true);
  auto const target = sending.registerObjectInstance(senderClass);
  waitFor(receiver, [&] { return receiver.recorder().hasDiscovery(target); }, options,
          "directed-reference receiver discovering target");

  std::vector<std::uint8_t> const tagBytes{0x44U, 0x52U, 0x46U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  sending.sendDirectedInteraction(senderInteraction, target,
                                  rti::ParameterHandleValueMap{}, tag);
  waitFor(receiver,
          [&] { return receiver.recorder().directedInteractions().size() == 1U; },
          options, "later-module directed interaction delivery");
  auto const deliveries = receiver.recorder().directedInteractions();
  require(deliveries.size() == 1U &&
              deliveries.front().interaction == receiverInteraction &&
              deliveries.front().object == target &&
              deliveries.front().parameters.empty() &&
              deliveries.front().tag == tagBytes,
          "Later-module directed interaction did not reach its subscribed target");

  receiving.unsubscribeObjectClassDirectedInteractions(receiverClass, interactions);
  sending.unpublishObjectClassDirectedInteractions(senderClass, interactions);
  receiving.unsubscribeObjectClassAttributes(receiverClass, attributes);
  sending.unpublishObjectClassAttributes(senderClass, attributes);
  receiver.resign(rti::NO_ACTION);
  sender.resign(rti::DELETE_OBJECTS);
  sending.destroyFederationExecution(federation);

  requireException([&] { create({missing}); }, L"InconsistentFOM",
                   "unresolved directed-interaction class reference");
  create({objectFom, interactionFom});
  sending.destroyFederationExecution(federation);
  receiver.disconnect();
  sender.disconnect();
}

void scenarioFomDirectedReferenceResolutionContract(Options const& options,
                                                    rti::CallbackModel model) {
  scenarioFomDirectedReferenceResolution(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
