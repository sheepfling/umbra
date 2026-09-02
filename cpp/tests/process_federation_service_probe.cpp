#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_callback_bridge.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"

#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FederationRegistryStatus;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::FomValidationStatus;
using umbra::detail::InteractionClassDeclarationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ProcessFederationClient;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportConnection;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceStatus;
using umbra::detail::decodeProcessFederationRegisterObjectInstanceResult;

class ProbeFederateAmbassador final : public rti1516_2025::NullFederateAmbassador {
 public:
  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::ParameterHandleValueMap const& parameterValues,
      rti1516_2025::VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const& transportationType,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
    received = true;
    interactionClassHandle = interactionClass;
    transportationTypeHandle = transportationType;
    producingFederateHandle = producingFederate;
    parameterCount = parameterValues.size();
    hasOptionalSentRegions = optionalSentRegions != nullptr;
    tag.clear();
    if (userSuppliedTag.size() != 0U) {
      auto const* first = static_cast<std::uint8_t const*>(userSuppliedTag.data());
      tag.assign(first, first + userSuppliedTag.size());
    }
  }

  bool received = false;
  rti1516_2025::InteractionClassHandle interactionClassHandle;
  rti1516_2025::TransportationTypeHandle transportationTypeHandle;
  rti1516_2025::FederateHandle producingFederateHandle;
  std::size_t parameterCount = 0U;
  bool hasOptionalSentRegions = false;
  std::vector<std::uint8_t> tag;
};

std::filesystem::path resourcePath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relative;
}

PrevalidatedFomModule validatedModule(
    std::filesystem::path const& source,
    FomModuleKind kind,
    std::wstring designator) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      source,
      resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  if (result.status != FomValidationStatus::valid || !result.module) {
    throw std::runtime_error("The process probe FOM did not validate.");
  }
  return *result.module;
}

FederationDefinition composedRestaurantDefinition() {
  std::vector<PrevalidatedFomModule> modules{
      validatedModule(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:process-probe-mim"),
      validatedModule(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:process-probe-restaurant"),
  };
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid ||
      !result.catalog || !result.fdd) {
    throw std::runtime_error("The process probe FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

void writeText(std::filesystem::path const& path, std::string const& text) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) {
    throw std::runtime_error("The process probe could not write its status file.");
  }
  output << text;
  if (!output) {
    throw std::runtime_error("The process probe could not flush its status file.");
  }
}

template <typename Reader>
auto waitForFile(
    std::filesystem::path const& path,
    Reader&& reader) {
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(30);
  while (std::chrono::steady_clock::now() < deadline) {
    if (std::filesystem::exists(path)) {
      try {
        return reader(path);
      } catch (std::exception const&) {
        // The writer may still be flushing a newly-created marker. Retry until
        // the bounded startup deadline rather than accepting a partial value.
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  throw std::runtime_error("The process probe timed out waiting for a marker file.");
}

std::uint16_t readPort(std::filesystem::path const& path) {
  return waitForFile(path, [](std::filesystem::path const& marker) {
    std::ifstream input(marker, std::ios::binary);
    unsigned int port = 0U;
    input >> port;
    if (!input || port == 0U || port > 65535U) {
      throw std::runtime_error("The process probe port marker is invalid.");
    }
    return static_cast<std::uint16_t>(port);
  });
}

std::uint64_t readHandle(std::filesystem::path const& path) {
  return waitForFile(path, [](std::filesystem::path const& marker) {
    std::ifstream input(marker, std::ios::binary);
    std::uint64_t handle = 0U;
    input >> handle;
    if (!input || handle == 0U) {
      throw std::runtime_error("The process probe handle marker is invalid.");
    }
    return handle;
  });
}

int runServer(std::filesystem::path const& directory) {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceOptions serviceOptions;
  serviceOptions.pushReceiveOrderEvents = true;
  ProcessFederationService service(
      registry, composedRestaurantDefinition(), serviceOptions);
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  if (!listener) {
    throw std::runtime_error("The process probe could not open its listener.");
  }
  writeText(directory / "port.txt", std::to_string(listener->address().port));

  std::unique_ptr<ProcessTransportSession> sender;
  std::unique_ptr<ProcessTransportSession> receiver;
  for (int index = 0; index < 2; ++index) {
    auto connection = listener->accept(
        nullptr,
        {"process-probe-server", static_cast<std::uint64_t>(0x71U + index)},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    auto const endpointId = connection->peerIdentity().endpointId;
    if (endpointId == "process-probe-sender" && !sender) {
      sender = std::make_unique<ProcessTransportSession>(std::move(connection));
    } else if (endpointId == "process-probe-receiver" && !receiver) {
      receiver = std::make_unique<ProcessTransportSession>(std::move(connection));
    } else {
      throw std::runtime_error("The process probe received an unexpected endpoint.");
    }
  }
  if (!sender || !receiver) {
    throw std::runtime_error("The process probe did not receive both endpoints.");
  }

  auto senderHandler = service.handlerFor(*sender);
  auto receiverHandler = service.handlerFor(*receiver);
  if (!ProcessTransportServiceDispatcher::serveOne(*sender, senderHandler) ||
      !ProcessTransportServiceDispatcher::serveOne(*sender, senderHandler) ||
      !ProcessTransportServiceDispatcher::serveOne(*receiver, receiverHandler)) {
    throw std::runtime_error("The process probe lost a Create or Join request.");
  }

  auto const senderMember = registry.memberByName(
      L"process-execution", L"process-probe-sender");
  auto const receiverMember = registry.memberByName(
      L"process-execution", L"process-probe-receiver");
  auto const interactionClass = registry.interactionClassHandleFor(
      L"process-execution",
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  if (!senderMember || !receiverMember || !interactionClass) {
    throw std::runtime_error("The process probe registry state is incomplete.");
  }
  if (registry.setInteractionClassPublication(
          L"process-execution", senderMember->id, *interactionClass, true) !=
          InteractionClassDeclarationStatus::applied ||
      registry.setInteractionClassSubscription(
          L"process-execution", receiverMember->id, *interactionClass, true) !=
          InteractionClassDeclarationStatus::applied) {
    throw std::runtime_error("The process probe declarations were rejected.");
  }
  writeText(directory / "interaction-class.txt", std::to_string(*interactionClass));

  if (!ProcessTransportServiceDispatcher::serveOne(*sender, senderHandler)) {
    throw std::runtime_error("The process probe lost its interaction request.");
  }
  if (!serviceOptions.pushReceiveOrderEvents &&
      !ProcessTransportServiceDispatcher::serveOne(*receiver, receiverHandler)) {
    throw std::runtime_error("The process probe lost its interaction request.");
  }
  service.detach(*sender);
  service.detach(*receiver);
  sender->connection()->close();
  receiver->connection()->close();
  writeText(directory / "server.ok", "ok\n");
  return 0;
}

// This server role is deliberately kept as a test fixture: the installed
// package smoke owns both federate processes and uses only the official public
// RTIambassador on those client sides.  The fixture supplies the private
// process service that is not itself part of the public IEEE binding yet.
int runPublicServer(
    std::filesystem::path const& directory,
    bool connectionLoss,
    bool parameterized,
    bool objectRegistration,
    bool namedRegistration,
    bool attributeUpdate,
    bool directedRetraction) {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceOptions serviceOptions;
  serviceOptions.pushReceiveOrderEvents = true;
  ProcessFederationService service(
      registry, composedRestaurantDefinition(), serviceOptions);
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  if (!listener) {
    throw std::runtime_error(
        "The installable process profile server could not open its listener.");
  }
  writeText(directory / "port.txt", std::to_string(listener->address().port));

  std::unique_ptr<ProcessTransportSession> sender;
  std::unique_ptr<ProcessTransportSession> receiver;
  for (int index = 0; index < 2; ++index) {
    auto connection = listener->accept(
        nullptr,
        {"package-process-server", static_cast<std::uint64_t>(0x91U + index)},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    auto const endpointId = connection->peerIdentity().endpointId;
    if (endpointId == "package-process-sender" && !sender) {
      sender = std::make_unique<ProcessTransportSession>(std::move(connection));
    } else if (endpointId == "package-process-receiver" && !receiver) {
      receiver = std::make_unique<ProcessTransportSession>(std::move(connection));
    } else {
      throw std::runtime_error(
          "The installable process profile server received an unexpected endpoint.");
    }
  }
  if (!sender || !receiver) {
    throw std::runtime_error(
        "The installable process profile server did not receive both clients.");
  }

  auto baseSenderHandler = service.handlerFor(*sender);
  auto senderHandler = [&](TransportServiceMessage const& request) {
    return baseSenderHandler(request);
  };
  auto receiverHandler = service.handlerFor(*receiver);
  if (!ProcessTransportServiceDispatcher::serveOne(*sender, senderHandler) ||
      !ProcessTransportServiceDispatcher::serveOne(*sender, senderHandler) ||
      !ProcessTransportServiceDispatcher::serveOne(*receiver, receiverHandler)) {
    throw std::runtime_error(
        "The installable process profile server lost Create or Join.");
  }

  auto const senderMember = registry.memberByName(
      L"process-execution", L"package-process-sender");
  auto const receiverMember = registry.memberByName(
      L"process-execution", L"package-process-receiver");
  auto serveExpectedSender = [&](TransportServiceOperation operation) {
    return ProcessTransportServiceDispatcher::serveOne(
        *sender,
        [&](TransportServiceMessage const& request) {
          if (request.operation != operation) {
            throw std::runtime_error(
                "The installable process profile server received an unexpected sender operation " +
                std::to_string(static_cast<unsigned int>(request.operation)) +
                " (expected " +
                std::to_string(static_cast<unsigned int>(operation)) + ").");
          }
          return senderHandler(request);
        });
  };
  auto serveExpectedReceiver = [&](TransportServiceOperation operation) {
    return ProcessTransportServiceDispatcher::serveOne(
        *receiver,
        [&](TransportServiceMessage const& request) {
          if (request.operation != operation) {
            throw std::runtime_error(
                "The installable directed-retraction server received an unexpected receiver operation " +
                std::to_string(static_cast<unsigned int>(request.operation)) +
                " (expected " +
                std::to_string(static_cast<unsigned int>(operation)) + ").");
          }
          return receiverHandler(request);
        });
  };
  auto const interactionClass = registry.interactionClassHandleFor(
      L"process-execution",
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const objectClass = registry.objectClassHandleFor(
      L"process-execution", "HLAobjectRoot.Customer");
  if (!senderMember || !receiverMember || !interactionClass || !objectClass) {
    throw std::runtime_error(
        "The installable process profile registry state is incomplete.");
  }
  if (registry.setInteractionClassPublication(
          L"process-execution", senderMember->id, *interactionClass, true) !=
          InteractionClassDeclarationStatus::applied ||
      registry.setInteractionClassSubscription(
          L"process-execution", receiverMember->id, *interactionClass, true) !=
          InteractionClassDeclarationStatus::applied) {
    throw std::runtime_error(
        "The installable process profile declarations were rejected.");
  }
  writeText(directory / "object-class.txt", std::to_string(*objectClass));
  writeText(directory / "interaction-class.txt", std::to_string(*interactionClass));

  if (directedRetraction) {
    auto const directedObjectClass = registry.objectClassHandleFor(
        L"process-execution", "HLAobjectRoot.Employee.Server");
    auto const directedInteractionClass = registry.interactionClassHandleFor(
        L"process-execution", "HLAinteractionRoot.ServerAction.TakeOrder");
    auto const attribute = registry.attributeHandleFor(
        L"process-execution",
        "HLAobjectRoot.Employee.Server",
        "HLAprivilegeToDeleteObject");
    if (!directedObjectClass || !directedInteractionClass || !attribute) {
      throw std::runtime_error(
          "The installable directed-retraction server could not resolve the directed object or attribute.");
    }
    // The consumer deliberately performs the public declaration and target
    // registration sequence.  Validate that each request crosses this
    // process service boundary before the timestamped send/retract pair.
    if (!serveExpectedSender(TransportServiceOperation::get_object_class_handle) ||
        !serveExpectedSender(TransportServiceOperation::get_interaction_class_handle) ||
        !serveExpectedSender(TransportServiceOperation::get_attribute_handle) ||
        !serveExpectedSender(TransportServiceOperation::publish_object_class_attributes) ||
        !serveExpectedSender(TransportServiceOperation::publish_object_class_directed_interactions) ||
        !serveExpectedSender(TransportServiceOperation::register_object_instance) ||
        !serveExpectedReceiver(TransportServiceOperation::get_object_class_handle) ||
        !serveExpectedReceiver(TransportServiceOperation::get_attribute_handle) ||
        !serveExpectedReceiver(TransportServiceOperation::get_interaction_class_handle) ||
        !serveExpectedReceiver(TransportServiceOperation::subscribe_object_class_attributes) ||
        !serveExpectedReceiver(TransportServiceOperation::subscribe_object_class_directed_interactions)) {
      throw std::runtime_error(
          "The installable directed-retraction server lost a lookup, declaration, or registration request.");
    }
    auto const published = registry.publishedObjectClassAttributeHandles(
        L"process-execution", senderMember->id, *directedObjectClass);
    if (!published || !published->contains(*attribute)) {
      throw std::runtime_error(
          "The installable directed-retraction publication was rejected.");
    }
    if (!serveExpectedSender(TransportServiceOperation::send_directed_interaction) ||
        !serveExpectedReceiver(TransportServiceOperation::receive_interaction) ||
        !serveExpectedSender(TransportServiceOperation::retract) ||
        !serveExpectedReceiver(TransportServiceOperation::receive_interaction) ||
        !serveExpectedSender(TransportServiceOperation::send_directed_interaction) ||
        !serveExpectedSender(TransportServiceOperation::retract) ||
        !serveExpectedReceiver(TransportServiceOperation::receive_interaction)) {
      throw std::runtime_error(
          "The installable directed-retraction server lost its positive/retracted directed sequence.");
    }
    writeText(directory / "send.ok", "ok\n");
    if (!ProcessTransportServiceDispatcher::serveOne(*sender, senderHandler) ||
        !ProcessTransportServiceDispatcher::serveOne(*receiver, receiverHandler)) {
      throw std::runtime_error(
          "The installable directed-retraction server lost a public Resign.");
    }
    service.detach(*sender);
    service.detach(*receiver);
    sender->connection()->close();
    receiver->connection()->close();
    writeText(directory / "server.ok", "ok\n");
    return 0;
  }

  if (namedRegistration) {
    auto const attribute = registry.attributeHandleFor(
        L"process-execution",
        "HLAobjectRoot.Customer",
        "HLAprivilegeToDeleteObject");
    if (!attribute) {
      throw std::runtime_error(
          "The installable process profile named-registration server could not resolve the object attribute.");
    }
    std::size_t reservationCount = 0U;
    std::size_t registrationCount = 0U;
    auto serveNamedSender = [&](TransportServiceOperation operation) {
      return ProcessTransportServiceDispatcher::serveOne(
          *sender,
          [&](TransportServiceMessage const& request) {
            if (request.operation != operation) {
              throw std::runtime_error(
                  "The installable process profile named-registration server received an unexpected sender operation.");
            }
            auto response = senderHandler(request);
            if (operation == TransportServiceOperation::reserve_object_instance_name &&
                response.status == TransportServiceStatus::ok) {
              auto const result =
                  umbra::detail::decodeProcessFederationReserveObjectInstanceNameResult(
                      response.payload);
              ++reservationCount;
              if (reservationCount == 1U) {
                if (!result.succeeded ||
                    result.status !=
                        umbra::detail::ObjectInstanceNameReservationStatus::applied) {
                  throw std::runtime_error(
                      "The installable named-registration server rejected the reservation.");
                }
              } else if (
                  result.status !=
                  umbra::detail::ObjectInstanceNameReservationStatus::illegal_name) {
                throw std::runtime_error(
                    "The installable named-registration server did not preserve IllegalName.");
              }
            }
            if (operation == TransportServiceOperation::register_object_instance &&
                response.status == TransportServiceStatus::ok) {
              auto const result =
                  umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                      response.payload);
              ++registrationCount;
              if (registrationCount == 1U) {
                if (result.status !=
                        umbra::detail::ObjectInstanceRegistrationStatus::applied ||
                    result.objectInstanceHandle == 0U ||
                    result.objectInstanceName != L"package-process-named-object") {
                  throw std::runtime_error(
                      "The installable named-registration server did not preserve the named registration.");
                }
              } else if (
                  result.status !=
                  umbra::detail::ObjectInstanceRegistrationStatus::object_instance_name_in_use) {
                throw std::runtime_error(
                    "The installable named-registration server did not preserve ObjectInstanceNameInUse.");
              }
            }
            return response;
          });
    };
    if (!serveNamedSender(TransportServiceOperation::get_object_class_handle) ||
        !serveNamedSender(TransportServiceOperation::get_attribute_handle) ||
        !serveNamedSender(TransportServiceOperation::publish_object_class_attributes) ||
        !serveNamedSender(TransportServiceOperation::reserve_object_instance_name) ||
        !serveNamedSender(TransportServiceOperation::register_object_instance) ||
        !serveNamedSender(TransportServiceOperation::register_object_instance) ||
        !serveNamedSender(TransportServiceOperation::reserve_object_instance_name)) {
      throw std::runtime_error(
          "The installable process profile named-registration server lost a public operation.");
    }
    writeText(directory / "send.ok", "ok\n");
    if (!ProcessTransportServiceDispatcher::serveOne(*sender, senderHandler) ||
        !ProcessTransportServiceDispatcher::serveOne(*receiver, receiverHandler)) {
      throw std::runtime_error(
          "The installable process profile named-registration server lost a public Resign.");
    }
    service.detach(*sender);
    service.detach(*receiver);
    sender->connection()->close();
    receiver->connection()->close();
    writeText(directory / "server.ok", "ok\n");
    return 0;
  }

  if (objectRegistration) {
    auto const attribute = registry.attributeHandleFor(
        L"process-execution",
        "HLAobjectRoot.Customer",
        "HLAprivilegeToDeleteObject");
    if (!attribute) {
      throw std::runtime_error(
          "The installable process profile could not resolve the object attribute.");
    }
    if (!serveExpectedSender(TransportServiceOperation::get_object_class_handle) ||
        !serveExpectedSender(TransportServiceOperation::get_attribute_handle) ||
        !serveExpectedSender(TransportServiceOperation::publish_object_class_attributes)) {
      throw std::runtime_error(
          "The installable process profile object-registration server lost lookup or publication.");
    }
    auto const published = registry.publishedObjectClassAttributeHandles(
        L"process-execution", senderMember->id, *objectClass);
    if (!published || !published->contains(*attribute)) {
      throw std::runtime_error(
          "The installable process profile object-registration publication was rejected.");
    }
    if (!serveExpectedSender(TransportServiceOperation::register_object_instance)) {
      throw std::runtime_error(
          "The installable process profile object-registration server lost registration.");
    }
    writeText(directory / "send.ok", "ok\n");
    if (!ProcessTransportServiceDispatcher::serveOne(*sender, senderHandler) ||
        !ProcessTransportServiceDispatcher::serveOne(*receiver, receiverHandler)) {
      throw std::runtime_error(
          "The installable process profile object-registration server lost a public Resign.");
    }
    service.detach(*sender);
    service.detach(*receiver);
    sender->connection()->close();
    receiver->connection()->close();
    writeText(directory / "server.ok", "ok\n");
    return 0;
  }

  if (attributeUpdate) {
    auto const attribute = registry.attributeHandleFor(
        L"process-execution",
        "HLAobjectRoot.Customer",
        "HLAprivilegeToDeleteObject");
    if (!attribute) {
      throw std::runtime_error(
          "The installable process profile attribute-update server could not resolve the object attribute.");
    }
    auto serveExpectedReceiver = [&](TransportServiceOperation operation) {
      return ProcessTransportServiceDispatcher::serveOne(
          *receiver,
          [&](TransportServiceMessage const& request) {
            if (request.operation != operation) {
              throw std::runtime_error(
                  "The installable process profile attribute-update server received an unexpected receiver operation.");
            }
            return receiverHandler(request);
          });
    };
    if (!serveExpectedReceiver(TransportServiceOperation::get_object_class_handle) ||
        !serveExpectedReceiver(TransportServiceOperation::get_attribute_handle) ||
        !serveExpectedReceiver(TransportServiceOperation::subscribe_object_class_attributes) ||
        !serveExpectedSender(TransportServiceOperation::get_object_class_handle) ||
        !serveExpectedSender(TransportServiceOperation::get_attribute_handle) ||
        !serveExpectedSender(TransportServiceOperation::publish_object_class_attributes) ||
        !serveExpectedSender(TransportServiceOperation::register_object_instance) ||
        !serveExpectedSender(TransportServiceOperation::update_attribute_values) ||
        !ProcessTransportServiceDispatcher::serveOne(*receiver, receiverHandler)) {
      throw std::runtime_error(
          "The installable process profile attribute-update server lost a public operation.");
    }
    writeText(directory / "send.ok", "ok\n");
    if (!ProcessTransportServiceDispatcher::serveOne(*sender, senderHandler) ||
        !ProcessTransportServiceDispatcher::serveOne(*receiver, receiverHandler)) {
      throw std::runtime_error(
          "The installable process profile attribute-update server lost a public Resign.");
    }
    service.detach(*sender);
    service.detach(*receiver);
    sender->connection()->close();
    receiver->connection()->close();
    writeText(directory / "server.ok", "ok\n");
    return 0;
  }

  if (connectionLoss) {
    // Model a real process/socket loss on the receiver side.  Apply the
    // registry's automatic-resign policy before closing the transport so the
    // surviving sender cannot route a later interaction to a stale member.
    auto const lost = registry.connectionLost(
        L"process-execution", receiverMember->id);
    if (lost.status != FederationRegistryStatus::applied) {
      throw std::runtime_error(
          "The installable process profile could not apply receiver Connection Lost.");
    }
    service.detach(*receiver);
    receiver->connection()->close();
    writeText(directory / "receiver-closed.ok", "ok\n");
    waitForFile(directory / "receiver-loss.ok", [](std::filesystem::path const& marker) {
      std::ifstream input(marker, std::ios::binary);
      std::string value;
      input >> value;
      if (!input || value != "callback-ok") {
        throw std::runtime_error(
            "The installable process profile receiver loss marker is invalid.");
      }
      return true;
    });

    // The surviving public sender remains usable after the peer disappears:
    // resolve the class, send a no-recipient interaction, then resign normally.
    if (!serveExpectedSender(TransportServiceOperation::get_interaction_class_handle) ||
        !serveExpectedSender(TransportServiceOperation::send_interaction)) {
      throw std::runtime_error(
          "The installable process profile loss server lost lookup or Send Interaction.");
    }
    writeText(directory / "send.ok", "ok\n");
    if (!ProcessTransportServiceDispatcher::serveOne(*sender, senderHandler)) {
      throw std::runtime_error(
          "The installable process profile loss server lost sender Resign.");
    }
    service.detach(*sender);
    sender->connection()->close();
    writeText(directory / "server.ok", "ok\n");
    return 0;
  }

  // Keep every public lookup on the service boundary rather than manufacturing
  // handles in the package consumer. The parameterized mode adds the object
  // class and parameter lookups before the shared Send Interaction request.
  if (!serveExpectedSender(TransportServiceOperation::get_interaction_class_handle) ||
      (parameterized &&
       !serveExpectedSender(TransportServiceOperation::get_object_class_handle)) ||
      (parameterized &&
       !serveExpectedSender(TransportServiceOperation::get_parameter_handle)) ||
      !serveExpectedSender(TransportServiceOperation::send_interaction)) {
    throw std::runtime_error(
        "The installable process profile server lost lookup or Send Interaction.");
  }
  // The public receiver's Evoke path first sends the legacy receive poll while
  // draining the pushed event frame.  Answer that poll before accepting the
  // lifecycle requests so the client can cross the official callback boundary
  // without leaving an in-flight request on the socket.
  if (!ProcessTransportServiceDispatcher::serveOne(*receiver, receiverHandler)) {
    throw std::runtime_error(
        "The installable process profile server lost the receiver callback poll.");
  }
  writeText(directory / "send.ok", "ok\n");

  // Both public clients resign through the official RTIambassador before the
  // fixture closes their sockets.  This keeps the package run on the normal
  // lifecycle path instead of relying on destructor cleanup.
  if (!ProcessTransportServiceDispatcher::serveOne(*sender, senderHandler) ||
      !ProcessTransportServiceDispatcher::serveOne(*receiver, receiverHandler)) {
    throw std::runtime_error(
        "The installable process profile server lost a public Resign.");
  }
  service.detach(*sender);
  service.detach(*receiver);
  sender->connection()->close();
  receiver->connection()->close();
  writeText(directory / "server.ok", "ok\n");
  return 0;
}

int runSender(std::filesystem::path const& directory) {
  auto const port = readPort(directory / "port.txt");
  ProcessFederationClient client(
      {"127.0.0.1", port}, {"process-probe-sender", 0x81U});
  client.createFederationExecution(L"process-execution");
  auto const joinResult = client.joinFederationExecution(
      L"process-execution",
      L"process-probe-sender",
      L"process-probe-sender");
  writeText(directory / "sender-id.txt", std::to_string(joinResult.federateId));
  auto const interactionClass = readHandle(directory / "interaction-class.txt");
  std::vector<std::uint8_t> const payload{0x50U, 0x52U, 0x4fU, 0x42U};
  auto const sendResult = client.sendInteraction(
      L"process-execution",
      joinResult.federateId,
      interactionClass,
      {},
      payload);
  if (sendResult.recipientCount != 1U) {
    throw std::runtime_error("The process probe Send Interaction had the wrong fanout.");
  }
  client.close();
  writeText(directory / "sender.ok", "ok\n");
  return 0;
}

int runReceiver(std::filesystem::path const& directory) {
  auto const port = readPort(directory / "port.txt");
  ProcessFederationClient client(
      {"127.0.0.1", port}, {"process-probe-receiver", 0x82U});
  auto const joinResult = client.joinFederationExecution(
      L"process-execution",
      L"process-probe-receiver",
      L"process-probe-receiver");
  if (joinResult.federateId == 0U) {
    throw std::runtime_error("The process probe receiver Join returned no identity.");
  }
  auto const interactionClass = readHandle(directory / "interaction-class.txt");
  auto const senderId = readHandle(directory / "sender-id.txt");
  std::vector<std::uint8_t> const expectedPayload{0x50U, 0x52U, 0x4fU, 0x42U};
  ProbeFederateAmbassador ambassador;
  // The current server emits a pushed event.  The client owns the receive
  // boundary and projects it into the official callback bridge.
  client.attachCallbackBridge(ambassador);
  client.dispatchPushedReceiveOrder();
  if (client.pendingCallbackCount() != 1U) {
    throw std::runtime_error("The process probe callback was not queued.");
  }
  static_cast<void>(client.evokeOne(std::chrono::milliseconds(0)));
  auto const transportationValue =
      rti1516_2025::umbra_binding_detail::standardTransportationTypeValue(
          L"HLAreliable");
  if (!transportationValue || !ambassador.received ||
      ambassador.interactionClassHandle !=
          rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
              interactionClass) ||
      ambassador.transportationTypeHandle !=
          rti1516_2025::umbra_binding_detail::makeTransportationTypeHandle(
              *transportationValue) ||
      ambassador.producingFederateHandle !=
          rti1516_2025::umbra_binding_detail::makeFederateHandle(
              senderId) ||
      ambassador.parameterCount != 0U ||
      ambassador.tag != expectedPayload || ambassador.hasOptionalSentRegions) {
    throw std::runtime_error("The process probe official callback was not preserved.");
  }
  client.close();
  writeText(directory / "receiver.ok", "callback-ok\n");
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    return 2;
  }
  auto const directory = std::filesystem::path(argv[2]);
  try {
    if (std::string(argv[1]) == "server") {
      return runServer(directory);
    }
    if (std::string(argv[1]) == "public-server") {
      return runPublicServer(directory, false, false, false, false, false, false);
    }
    if (std::string(argv[1]) == "public-server-loss") {
      return runPublicServer(directory, true, false, false, false, false, false);
    }
    if (std::string(argv[1]) == "public-server-parameterized") {
      return runPublicServer(directory, false, true, false, false, false, false);
    }
    if (std::string(argv[1]) == "public-server-object-registration") {
      return runPublicServer(directory, false, false, true, false, false, false);
    }
    if (std::string(argv[1]) == "public-server-named-registration") {
      return runPublicServer(directory, false, false, false, true, false, false);
    }
    if (std::string(argv[1]) == "public-server-attribute-update") {
      return runPublicServer(directory, false, false, false, false, true, false);
    }
    if (std::string(argv[1]) == "public-server-directed-retraction") {
      return runPublicServer(directory, false, false, false, false, false, true);
    }
    if (std::string(argv[1]) == "sender") {
      return runSender(directory);
    }
    if (std::string(argv[1]) == "receiver") {
      return runReceiver(directory);
    }
  } catch (std::exception const& error) {
    try {
      writeText(directory / (std::string(argv[1]) + ".error"), error.what());
    } catch (...) {
    }
    return 1;
  }
  return 2;
}
