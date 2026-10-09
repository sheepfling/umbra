#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/message_retraction_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"

#include <RTI/FederateAmbassador.h>

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {

struct EmbeddedFederationManagementAccess {
  umbra::detail::EmbeddedFederationRegistry& registry() const {
    return embeddedFederationRegistry();
  }
};

EmbeddedFederationManagementAccess embeddedFederationManagement() {
  return {};
}

std::mutex& federationManagementMutex() {
  return ambassadorFederationManagementMutex();
}

void submitReceiveOrderCallback(
    umbra::detail::FederateCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    umbra::detail::FederateCallbackInvocation invocation) {
  submitAmbassadorReceiveOrderCallback(
      std::move(callbackRoute),
      std::move(federationName),
      receivingFederateId,
      std::move(invocation));
}

TransportationTypeHandle transportationHandleFromEmbeddedName(
    std::string const& transportationName,
    wchar_t const* context) {
  return ambassadorTransportationHandleFromEmbeddedName(transportationName, context);
}

void queueObjectInstanceNameReservation(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t federateId,
    bool succeeded,
    std::wstring objectInstanceName) {
  if (!callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has an object-instance name reservation without a callback route.");
  }
  callbackRoute([
      federationName = std::move(federationName),
      federateId,
      succeeded,
      objectInstanceName = std::move(objectInstanceName)](FederateAmbassador& recipient) mutable {
    {
      std::scoped_lock lock(federationManagementMutex());
      if (!embeddedFederationManagement().registry().memberById(
              federationName,
              federateId)) {
        return;
      }
    }
    if (succeeded) {
      recipient.objectInstanceNameReservationSucceeded(objectInstanceName);
    } else {
      recipient.objectInstanceNameReservationFailed(objectInstanceName);
    }
  });
}

void queueMultipleObjectInstanceNameReservation(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t federateId,
    bool succeeded,
    std::set<std::wstring> objectInstanceNames) {
  if (!callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has multiple object-instance name reservation without a callback route.");
  }
  callbackRoute([
      federationName = std::move(federationName),
      federateId,
      succeeded,
      objectInstanceNames = std::move(objectInstanceNames)](
      FederateAmbassador& recipient) mutable {
    {
      std::scoped_lock lock(federationManagementMutex());
      if (!embeddedFederationManagement().registry().memberById(
              federationName,
              federateId)) {
        return;
      }
    }
    if (succeeded) {
      recipient.multipleObjectInstanceNameReservationSucceeded(objectInstanceNames);
    } else {
      recipient.multipleObjectInstanceNameReservationFailed(objectInstanceNames);
    }
  });
}

std::wstring discoverObjectInstanceServiceReportRecord(
    std::uint64_t objectInstanceHandle,
    std::uint64_t objectClassHandle,
    std::wstring const& objectInstanceName,
    std::uint64_t producingFederateId,
    std::uint32_t serialNumber) {
  using umbra::detail::MomArgumentType;
  using umbra::detail::formatMomFederateHandle;
  using umbra::detail::formatMomObjectClassHandle;
  using umbra::detail::formatMomObjectInstanceHandle;
  using umbra::detail::formatMomString;
  using umbra::detail::formatMomSuccessfulVoidServiceReportRecord;

  return formatMomSuccessfulVoidServiceReportRecord(
      serialNumber,
      L"DiscoverObjectInstance",
      {{MomArgumentType::object_instance_handle,
        L"Object instance handle",
        formatMomObjectInstanceHandle(makeObjectInstanceHandle(objectInstanceHandle))},
       {MomArgumentType::object_class_handle,
        L"Object class designator",
        formatMomObjectClassHandle(makeObjectClassHandle(objectClassHandle))},
       {MomArgumentType::string,
        L"Object instance name",
        formatMomString(objectInstanceName)},
       {MomArgumentType::federate_handle,
        L"Producing joined federate designator",
        formatMomFederateHandle(makeFederateHandle(producingFederateId))}});
}

std::wstring removeObjectInstanceServiceReportRecord(
    std::uint64_t objectInstanceHandle,
    VariableLengthData const& userSuppliedTag,
    std::uint64_t producingFederateId,
    OrderType sentOrderType,
    LogicalTime const* optionalTimestamp,
    std::optional<OrderType> optionalReceivedOrderType,
    std::optional<std::uint64_t> optionalRetractionHandle,
    std::uint32_t serialNumber) {
  using umbra::detail::MomArgumentType;
  using umbra::detail::formatMomFederateHandle;
  using umbra::detail::formatMomLogicalTime;
  using umbra::detail::formatMomMessageRetractionHandle;
  using umbra::detail::formatMomNull;
  using umbra::detail::formatMomObjectInstanceHandle;
  using umbra::detail::formatMomOrderType;
  using umbra::detail::formatMomSuccessfulVoidServiceReportRecord;
  using umbra::detail::formatMomUserSuppliedTag;

  // Preserve the §6.17 narrative order, including every optional slot as a
  // Table 5 Null when the selected callback overload did not supply it.
  std::vector<umbra::detail::MomServiceArgument> suppliedArguments{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(makeObjectInstanceHandle(objectInstanceHandle))},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(userSuppliedTag)},
      {MomArgumentType::order_type,
       L"Sent message order type",
       formatMomOrderType(sentOrderType)},
      {MomArgumentType::federate_handle,
       L"Producing joined federate designator",
       formatMomFederateHandle(makeFederateHandle(producingFederateId))},
  };
  if (optionalTimestamp) {
    suppliedArguments.push_back(
        {MomArgumentType::logical_time,
         L"Optional timestamp",
         formatMomLogicalTime(*optionalTimestamp)});
  } else {
    suppliedArguments.push_back(
        {MomArgumentType::null_value, L"Optional timestamp", formatMomNull()});
  }
  if (optionalReceivedOrderType) {
    suppliedArguments.push_back(
        {MomArgumentType::order_type,
         L"Optional receive message order type",
         formatMomOrderType(*optionalReceivedOrderType)});
  } else {
    suppliedArguments.push_back(
        {MomArgumentType::null_value,
         L"Optional receive message order type",
         formatMomNull()});
  }
  if (optionalRetractionHandle) {
    suppliedArguments.push_back(
        {MomArgumentType::message_retraction_handle,
         L"Optional message retraction designator",
         formatMomMessageRetractionHandle(*optionalRetractionHandle)});
  } else {
    suppliedArguments.push_back(
        {MomArgumentType::null_value,
         L"Optional message retraction designator",
         formatMomNull()});
  }

  return formatMomSuccessfulVoidServiceReportRecord(
      serialNumber,
      L"RemoveObjectInstance",
      suppliedArguments);
}

std::wstring provideAttributeValueUpdateServiceReportRecord(
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    VariableLengthData const& userSuppliedTag,
    std::uint32_t serialNumber) {
  using umbra::detail::MomArgumentType;
  using umbra::detail::formatMomAttributeHandleSet;
  using umbra::detail::formatMomObjectInstanceHandle;
  using umbra::detail::formatMomSuccessfulVoidServiceReportRecord;
  using umbra::detail::formatMomUserSuppliedTag;

  AttributeHandleSet attributes;
  for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
    attributes.insert(makeAttributeHandle(attributeHandle));
  }
  // §6.22.1 fixes this callback's three supplied arguments. The copied
  // request tag is retained through callback-time revalidation, including the
  // zero-length RTI-invoked Auto Provide form.
  return formatMomSuccessfulVoidServiceReportRecord(
      serialNumber,
      L"ProvideAttributeValueUpdate",
      {{MomArgumentType::object_instance_handle,
        L"Object instance designator",
        formatMomObjectInstanceHandle(makeObjectInstanceHandle(objectInstanceHandle))},
       {MomArgumentType::attribute_handle_set,
        L"Set of attribute designators",
        formatMomAttributeHandleSet(attributes)},
       {MomArgumentType::table_5_user_supplied_tag,
        L"User-supplied tag",
        formatMomUserSuppliedTag(userSuppliedTag)}});
}

void queueAttributeRelevanceAdvisories(
    std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient> advisories,
    std::wstring const& federationName);

void queueObjectInstanceDiscovery(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    bool rtiOwnedMomObject) {
  callbackRoute([
      federationName = std::move(federationName),
      receivingFederateId,
      objectInstanceHandle,
      rtiOwnedMomObject,
      serviceReportRoute = std::move(serviceReportRoute)](FederateAmbassador& recipient) {
    std::optional<umbra::detail::KnownObjectInstanceSnapshot> discovery;
    {
      std::scoped_lock lock(federationManagementMutex());
      if (rtiOwnedMomObject) {
        discovery = embeddedFederationManagement().registry()
                        .beginJoinedFederateMomObjectDiscovery(
                            federationName,
                            receivingFederateId,
                            objectInstanceHandle);
      } else {
        discovery = embeddedFederationManagement().registry().beginObjectInstanceDiscovery(
            federationName,
            receivingFederateId,
            objectInstanceHandle);
      }
    }
    if (!discovery) {
      return;
    }

    if (serviceReportRoute) {
      // §6.9 is an RTI-initiated recipient service. The registry's
      // callback-time recheck above is the actual delivery boundary, so only
      // an invoked Discover Object Instance appends a selected-file record.
      // This preserves the report-before-callback ordering in HLA_EVOKED
      // without recording a cancelled or stale planned discovery.
      serviceReportRoute(
          static_cast<std::uint16_t>(umbra::detail::MomServiceType::object_management),
          [
              objectInstanceHandle = discovery->objectInstanceHandle,
              objectClassHandle = discovery->knownObjectClassHandle,
              objectInstanceName = discovery->objectInstanceName,
              producingFederateId = discovery->producingFederateId](
              std::uint32_t serialNumber) {
            return discoverObjectInstanceServiceReportRecord(
                objectInstanceHandle,
                objectClassHandle,
                objectInstanceName,
                producingFederateId,
                serialNumber);
          });
    }

    // The registry commits the recipient's known-instance state before this
    // callback, so a FederateAmbassador may safely use the matching 2025
    // support services from within Discover Object Instance.
    recipient.discoverObjectInstance(
        makeObjectInstanceHandle(discovery->objectInstanceHandle),
        makeObjectClassHandle(discovery->knownObjectClassHandle),
        discovery->objectInstanceName,
        rtiOwnedMomObject
            ? FederateHandle{}
            : makeFederateHandle(discovery->producingFederateId));

    if (rtiOwnedMomObject) {
      // The initial MOM values are RTI-owned and are reflected directly after
      // discovery. Revalidate the known-instance and active subscription
      // boundary so a queued callback cannot leak values after a declaration
      // change. Regional eligibility is evaluated against the immutable
      // HLAfederate point; periodic and other conditional scheduling remains
      // a later slice (event-driven switch updates use their own queue path).
      std::optional<umbra::detail::JoinedFederateMomAttributeValueUpdateRecipient>
          initialValues;
      {
        std::scoped_lock lock(federationManagementMutex());
        auto plan = embeddedFederationManagement().registry()
                        .planJoinedFederateMomAttributeValueUpdate(
                            federationName,
                            receivingFederateId,
                            objectInstanceHandle,
                            discovery->initialAttributeHandles,
                            true);
        if (plan.status ==
                umbra::detail::JoinedFederateMomAttributeValueUpdateStatus::applied &&
            plan.recipient) {
          initialValues = std::move(plan.recipient);
        }
      }
      if (initialValues && !initialValues->attributeValues.empty()) {
        AttributeHandleValueMap attributeValues;
        for (auto const& [attributeHandle, value] : initialValues->attributeValues) {
          attributeValues.emplace(makeAttributeHandle(attributeHandle), value);
        }
        recipient.reflectAttributeValues(
            makeObjectInstanceHandle(initialValues->objectInstanceHandle),
            attributeValues,
            VariableLengthData{},
            transportationHandleFromEmbeddedName(
                umbra::detail::hla::utf8::mom::reliable,
                L"The embedded federation could not reconstruct the MOM transportation type."),
            FederateHandle{},
            nullptr);
      }
      return;
    }

    // §5.8 requires the values of the subscribed attributes for each object
    // discovered by this Subscribe Object Class Attributes invocation. Plan
    // after Discover Object Instance has entered user code so a reentrant
    // unsubscribe, resignation, or region change fences out stale values.
    std::vector<umbra::detail::ObjectInstanceInitialAttributeReflection>
        initialAttributeReflections;
    {
      std::scoped_lock lock(federationManagementMutex());
      initialAttributeReflections = embeddedFederationManagement().registry()
                                        .planInitialObjectInstanceAttributeReflectionsForDiscovery(
                                            federationName,
                                            receivingFederateId,
                                            objectInstanceHandle);
    }
    for (auto const& initialReflection : initialAttributeReflections) {
      AttributeHandleValueMap attributeValues;
      for (auto const& [attributeHandle, value] : initialReflection.attributeValues) {
        attributeValues.emplace(makeAttributeHandle(attributeHandle), value);
      }
      recipient.reflectAttributeValues(
          makeObjectInstanceHandle(objectInstanceHandle),
          attributeValues,
          VariableLengthData{},
          transportationHandleFromEmbeddedName(
              initialReflection.transportationName,
              L"The embedded federation could not reconstruct an initial attribute transportation type."),
          makeFederateHandle(discovery->producingFederateId),
          nullptr);
    }

    // Discovery establishes the receiver's known-instance state. Any
    // attributes already relevant through its active ordinary or regional
    // declaration therefore cross the initial Attribute Relevance Advisory
    // edge now, after Discover Object Instance has entered user code.
    std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient>
        initialAttributeRelevanceAdvisories;
    {
      std::scoped_lock lock(federationManagementMutex());
      initialAttributeRelevanceAdvisories = embeddedFederationManagement()
          .registry()
          .planInitialAttributeRelevanceAdvisoriesForDiscovery(
              federationName,
              receivingFederateId,
              objectInstanceHandle);
    }
    queueAttributeRelevanceAdvisories(
        std::move(initialAttributeRelevanceAdvisories),
        federationName);

    // Auto Provide is an RTI-invoked service that follows a newly completed
    // discovery. It solicits each current owner of an in-scope attribute with
    // the mandatory empty tag; the provider's callback-time recheck still
    // suppresses stale work after resignation, deletion, or scope changes.
    umbra::detail::AttributeValueUpdateRequestPlan autoProvidePlan;
    {
      std::scoped_lock lock(federationManagementMutex());
      autoProvidePlan = embeddedFederationManagement().registry()
                            .planAutoProvideForDiscovery(
                                federationName,
                                receivingFederateId,
                                objectInstanceHandle);
    }
    if (autoProvidePlan.status !=
        umbra::detail::AttributeValueUpdateRequestStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation could not plan Auto Provide callbacks after discovery.");
    }
    for (auto& provider : autoProvidePlan.recipients) {
      if (!provider.callbackRoute) {
        throw RTIinternalError(
            L"The embedded federation has an Auto Provide recipient without a callback route.");
      }
      queueAmbassadorAttributeValueUpdateProvide(
          std::move(provider.callbackRoute),
          std::move(provider.serviceReportRoute),
          federationName,
          receivingFederateId,
          provider.providingFederateId,
          provider.objectInstanceHandle,
          std::move(provider.requestedAttributeHandles),
          VariableLengthData(),
          0U);
    }

    // Discovery establishes the known-class boundary required for ownership
    // assumption eligibility. Continue any prior resign/divestiture search
    // only after the standard Discover callback has entered user code, so an
    // immediate callback model cannot observe an assumption before discovery.
    std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient>
        newlyEligibleAssumptions;
    {
      std::scoped_lock lock(federationManagementMutex());
      newlyEligibleAssumptions = embeddedFederationManagement().registry()
                                     .planAttributeOwnershipAssumptionsForFederate(
                                         federationName,
                                         receivingFederateId);
    }
    queueAmbassadorAttributeOwnershipAssumptionRecipients(
        std::move(newlyEligibleAssumptions),
        federationName,
        VariableLengthData());
  });
}

}  // namespace

namespace {

void queueObjectInstanceDiscoveries(
    std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries,
    std::wstring const& federationName);
void queueObjectInstanceRemovals(
    std::vector<umbra::detail::ObjectInstanceRemovalRecipient> removals,
    std::wstring const& federationName,
    VariableLengthData const& userSuppliedTag);
void queueTimestampedObjectInstanceRemoval(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t messageId,
    VariableLengthData userSuppliedTag,
    std::shared_ptr<LogicalTime const> timestamp,
    bool provideRetraction,
    OrderType sentOrderType,
    OrderType receivedOrderType);
void queueObjectInstanceScopeChanges(
    std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes,
    std::wstring const& federationName);
void queueAttributeRelevanceAdvisories(
    std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient> advisories,
    std::wstring const& federationName);
void queueJoinedFederateMomAttributeValueUpdate(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    bool requireActiveSubscription,
    std::optional<std::map<std::uint64_t, VariableLengthData>> plannedAttributeValues,
    std::optional<std::map<std::uint64_t, std::set<std::uint64_t>>>
        requestRegionsByAttribute);
void queueJoinedFederateMomConditionalAttributeUpdate(
    std::wstring const& federationName,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    std::optional<std::uint64_t> excludedReceivingFederateId,
    std::optional<std::map<std::uint64_t, VariableLengthData>> plannedAttributeValues);
struct JoinedFederateMomConditionalWork {
  std::wstring federationName;
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
};
std::optional<JoinedFederateMomConditionalWork>
joinedFederateMomConditionalWorkFor(
    umbra::detail::EmbeddedFederationRegistry& registry,
    std::wstring const& federationName,
    std::uint64_t joinedFederateId,
    std::vector<std::string_view> attributeNames);
std::optional<JoinedFederateMomConditionalWork>
federationMomConditionalWorkFor(
    umbra::detail::EmbeddedFederationRegistry& registry,
    std::wstring const& federationName,
    std::vector<std::string_view> attributeNames);
void queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
    std::wstring const& federationName,
    std::uint64_t joinedFederateId,
    std::vector<std::string_view> attributeNames,
    std::optional<std::uint64_t> excludedReceivingFederateId);
void queueFederationMomConditionalAttributeUpdate(
    std::wstring const& federationName,
    std::vector<std::string_view> attributeNames,
    std::optional<std::uint64_t> excludedReceivingFederateId);
void pumpDueJoinedFederateMomPeriodicUpdates(std::wstring const& federationName);

}  // namespace

void queueAmbassadorObjectInstanceNameReservation(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t federateId,
    bool succeeded,
    std::wstring objectInstanceName) {
  queueObjectInstanceNameReservation(
      std::move(callbackRoute),
      std::move(federationName),
      federateId,
      succeeded,
      std::move(objectInstanceName));
}

void queueAmbassadorMultipleObjectInstanceNameReservation(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t federateId,
    bool succeeded,
    std::set<std::wstring> objectInstanceNames) {
  queueMultipleObjectInstanceNameReservation(
      std::move(callbackRoute),
      std::move(federationName),
      federateId,
      succeeded,
      std::move(objectInstanceNames));
}

void queueAmbassadorObjectInstanceDiscoveries(
    std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries,
    std::wstring const& federationName) {
  queueObjectInstanceDiscoveries(std::move(discoveries), federationName);
}

void queueAmbassadorObjectInstanceRemovals(
    std::vector<umbra::detail::ObjectInstanceRemovalRecipient> removals,
    std::wstring const& federationName,
    VariableLengthData const& userSuppliedTag) {
  queueObjectInstanceRemovals(
      std::move(removals),
      federationName,
      userSuppliedTag);
}

void queueAmbassadorTimestampedObjectInstanceRemoval(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t messageId,
    VariableLengthData userSuppliedTag,
    std::shared_ptr<LogicalTime const> timestamp,
    bool provideRetraction,
    OrderType sentOrderType,
    OrderType receivedOrderType) {
  queueTimestampedObjectInstanceRemoval(
      std::move(callbackRoute),
      std::move(serviceReportRoute),
      std::move(federationName),
      receivingFederateId,
      objectInstanceHandle,
      messageId,
      std::move(userSuppliedTag),
      std::move(timestamp),
      provideRetraction,
      sentOrderType,
      receivedOrderType);
}

void queueAmbassadorObjectInstanceScopeChanges(
    std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes,
    std::wstring const& federationName) {
  queueObjectInstanceScopeChanges(std::move(changes), federationName);
}

void queueAmbassadorAttributeRelevanceAdvisories(
    std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient> advisories,
    std::wstring const& federationName) {
  queueAttributeRelevanceAdvisories(std::move(advisories), federationName);
}

void queueAmbassadorJoinedFederateMomAttributeValueUpdate(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    bool requireActiveSubscription,
    std::optional<std::map<std::uint64_t, VariableLengthData>> plannedAttributeValues,
    std::optional<std::map<std::uint64_t, std::set<std::uint64_t>>>
        requestRegionsByAttribute) {
  queueJoinedFederateMomAttributeValueUpdate(
      std::move(callbackRoute),
      std::move(federationName),
      receivingFederateId,
      objectInstanceHandle,
      std::move(requestedAttributeHandles),
      requireActiveSubscription,
      std::move(plannedAttributeValues),
      std::move(requestRegionsByAttribute));
}

std::optional<AmbassadorJoinedFederateMomConditionalWork>
ambassadorJoinedFederateMomConditionalWorkFor(
    umbra::detail::EmbeddedFederationRegistry& registry,
    std::wstring const& federationName,
    std::uint64_t joinedFederateId,
    std::vector<std::string_view> attributeNames) {
  auto work = joinedFederateMomConditionalWorkFor(
      registry,
      federationName,
      joinedFederateId,
      std::move(attributeNames));
  if (!work) {
    return std::nullopt;
  }
  return AmbassadorJoinedFederateMomConditionalWork{
      std::move(work->federationName),
      work->objectInstanceHandle,
      std::move(work->attributeHandles)};
}

std::optional<AmbassadorJoinedFederateMomConditionalWork>
ambassadorFederationMomConditionalWorkFor(
    umbra::detail::EmbeddedFederationRegistry& registry,
    std::wstring const& federationName,
    std::vector<std::string_view> attributeNames) {
  auto work = federationMomConditionalWorkFor(
      registry,
      federationName,
      std::move(attributeNames));
  if (!work) {
    return std::nullopt;
  }
  return AmbassadorJoinedFederateMomConditionalWork{
      std::move(work->federationName),
      work->objectInstanceHandle,
      std::move(work->attributeHandles)};
}

void queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
    std::wstring const& federationName,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    std::optional<std::uint64_t> excludedReceivingFederateId) {
  queueJoinedFederateMomConditionalAttributeUpdate(
      federationName,
      objectInstanceHandle,
      std::move(requestedAttributeHandles),
      excludedReceivingFederateId,
      std::nullopt);
}

void queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
    std::wstring const& federationName,
    std::uint64_t joinedFederateId,
    std::vector<std::string_view> attributeNames,
    std::optional<std::uint64_t> excludedReceivingFederateId) {
  queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
      federationName,
      joinedFederateId,
      std::move(attributeNames),
      excludedReceivingFederateId);
}

void pumpAmbassadorDueJoinedFederateMomPeriodicUpdates(
    std::wstring const& federationName) {
  pumpDueJoinedFederateMomPeriodicUpdates(federationName);
}

void queueAmbassadorFederationMomConditionalAttributeUpdate(
    std::wstring const& federationName,
    std::vector<std::string_view> attributeNames,
    std::optional<std::uint64_t> excludedReceivingFederateId) {
  queueFederationMomConditionalAttributeUpdate(
      federationName,
      std::move(attributeNames),
      excludedReceivingFederateId);
}

void queueAmbassadorReceiveOrderAttributeUpdate(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> sentAttributeHandles,
    std::vector<AmbassadorAttributeValue> sentAttributes,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    std::string transportationName,
    bool reliableTransportation,
    std::optional<std::set<std::uint64_t>> sentRegionHandles,
    bool defaultRegionUsed,
    std::optional<std::map<std::uint64_t, umbra::detail::RegionSpecificationSnapshot>>
        sentRegionSnapshots) {
  submitReceiveOrderCallback(
      std::move(callbackRoute),
      federationName,
      receivingFederateId,
      [
          federationName,
          producingFederateId,
          receivingFederateId,
          objectInstanceHandle,
          sentAttributeHandles = std::move(sentAttributeHandles),
          sentAttributes = std::move(sentAttributes),
          userSuppliedTag = std::move(userSuppliedTag),
          transportationType = std::move(transportationType),
          transportationName = std::move(transportationName),
          reliableTransportation,
          sentRegionHandles = std::move(sentRegionHandles),
          defaultRegionUsed,
          sentRegionSnapshots = std::move(sentRegionSnapshots)](FederateAmbassador& recipient) mutable {
        std::optional<umbra::detail::ReceiveOrderAttributeUpdateRecipient> projection;
        {
          std::scoped_lock lock(federationManagementMutex());
          projection = embeddedFederationManagement().registry()
              .receiveOrderAttributeUpdateRecipientFor(
                  federationName,
                  producingFederateId,
                  receivingFederateId,
                  objectInstanceHandle,
                  sentAttributeHandles,
                  sentRegionHandles ? &*sentRegionHandles : nullptr,
                  sentRegionSnapshots ? &*sentRegionSnapshots : nullptr);
        }
        if (!projection) {
          return;
        }

        // Recheck eligibility at callback time, limited to this update's
        // submitted attributes so changing subscriptions cannot merge passels.
        AttributeHandleValueMap attributeValues = ambassadorProjectAttributeValues(
            sentAttributes,
            projection->receivedAttributeHandles);
        if (!reliableTransportation) {
          for (auto iterator = attributeValues.begin(); iterator != attributeValues.end();) {
            auto const handle = iterator->first;
            std::uint64_t numeric = 0;
            for (auto const candidate : sentAttributeHandles) {
              if (makeAttributeHandle(candidate) == handle) {
                numeric = candidate;
                break;
              }
            }
            auto const rate = projection->maximumUpdateRatesByAttribute.find(numeric);
            auto const key = ambassadorUpdateRateAdmissionKey(
                federationName,
                receivingFederateId,
                objectInstanceHandle,
                projection->subscriptionGeneration,
                numeric);
            bool const admitted = !key || ambassadorAdmitUpdateRate(
                *key,
                rate == projection->maximumUpdateRatesByAttribute.end()
                    ? 0.0
                    : rate->second,
                false);
            if (!admitted) {
              iterator = attributeValues.erase(iterator);
            } else {
              ++iterator;
            }
          }
          if (attributeValues.empty()) {
            return;
          }
        }
        std::optional<RegionHandleSet> optionalSentRegions;
        if (projection->conveyRegionDesignatorSets &&
            (sentRegionHandles || defaultRegionUsed)) {
          optionalSentRegions.emplace();
          if (sentRegionHandles) {
            for (std::uint64_t const regionHandle : *sentRegionHandles) {
              optionalSentRegions->insert(makeRegionHandle(regionHandle));
            }
          }
        }
        {
          std::scoped_lock lock(federationManagementMutex());
          static_cast<void>(embeddedFederationManagement().registry()
                                .recordSuccessfulObjectInstanceReflection(
                                    federationName,
                                    receivingFederateId,
                                    objectInstanceHandle));
        }
        recordAmbassadorSuccessfulReflectionReceipt(
            federationName,
            receivingFederateId,
            objectInstanceHandle,
            std::move(transportationName));
        recipient.reflectAttributeValues(
            makeObjectInstanceHandle(objectInstanceHandle),
            attributeValues,
            userSuppliedTag,
            transportationType,
            makeFederateHandle(producingFederateId),
            optionalSentRegions ? &*optionalSentRegions : nullptr);
      });
}

std::wstring makeAmbassadorDiscoverObjectInstanceServiceReportRecord(
    std::uint64_t objectInstanceHandle,
    std::uint64_t objectClassHandle,
    std::wstring const& objectInstanceName,
    std::uint64_t producingFederateId,
    std::uint32_t serialNumber) {
  return discoverObjectInstanceServiceReportRecord(
      objectInstanceHandle,
      objectClassHandle,
      objectInstanceName,
      producingFederateId,
      serialNumber);
}

std::wstring makeAmbassadorRemoveObjectInstanceServiceReportRecord(
    std::uint64_t objectInstanceHandle,
    VariableLengthData const& userSuppliedTag,
    std::uint64_t producingFederateId,
    OrderType sentOrderType,
    LogicalTime const* optionalTimestamp,
    std::optional<OrderType> optionalReceivedOrderType,
    std::optional<std::uint64_t> optionalRetractionHandle,
    std::uint32_t serialNumber) {
  return removeObjectInstanceServiceReportRecord(
      objectInstanceHandle,
      userSuppliedTag,
      producingFederateId,
      sentOrderType,
      optionalTimestamp,
      optionalReceivedOrderType,
      optionalRetractionHandle,
      serialNumber);
}

std::wstring makeAmbassadorProvideAttributeValueUpdateServiceReportRecord(
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    VariableLengthData const& userSuppliedTag,
    std::uint32_t serialNumber) {
  return provideAttributeValueUpdateServiceReportRecord(
      objectInstanceHandle,
      requestedAttributeHandles,
      userSuppliedTag,
      serialNumber);
}

namespace {

void queueObjectInstanceDiscoveries(
    std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries,
    std::wstring const& federationName) {
  // Do not hold a sender's ambassador lock or the federation lock while
  // submitting. HLA_IMMEDIATE may enter another federate's discovery callback
  // synchronously, including its support-service calls.
  for (std::size_t index = 0; index < discoveries.size(); ++index) {
    auto& discovery = discoveries[index];
    try {
      if (!discovery.callbackRoute) {
        throw RTIinternalError(
            L"The embedded federation has a discovery recipient without a callback route.");
      }
      queueObjectInstanceDiscovery(
          std::move(discovery.callbackRoute),
          std::move(discovery.serviceReportRoute),
          federationName,
          discovery.receivingFederateId,
          discovery.objectInstanceHandle,
          discovery.rtiOwnedMomObject);
    } catch (...) {
      // An immediate callback can have committed known-instance state before
      // propagating a user exception.  The registry clears only still-pending
      // reservations, preserving that committed state while allowing all
      // undelivered recipients to be reconsidered later.
      std::scoped_lock lock(federationManagementMutex());
      auto& registry = embeddedFederationManagement().registry();
      for (std::size_t pending = index; pending < discoveries.size(); ++pending) {
        if (discoveries[pending].rtiOwnedMomObject) {
          registry.cancelJoinedFederateMomObjectDiscovery(
              federationName,
              discoveries[pending].receivingFederateId,
              discoveries[pending].objectInstanceHandle);
        } else {
          registry.cancelObjectInstanceDiscovery(
              federationName,
              discoveries[pending].receivingFederateId,
              discoveries[pending].objectInstanceHandle);
        }
      }
      throw;
    }
  }
}

void queueJoinedFederateMomAttributeValueUpdate(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    bool requireActiveSubscription = false,
    std::optional<std::map<std::uint64_t, VariableLengthData>>
        plannedAttributeValues = std::nullopt,
    std::optional<std::map<std::uint64_t, std::set<std::uint64_t>>>
        requestRegionsByAttribute = std::nullopt) {
  if (!callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has a MOM attribute reflection without a callback route.");
  }
  callbackRoute([
      federationName = std::move(federationName),
      receivingFederateId,
      objectInstanceHandle,
      requestedAttributeHandles = std::move(requestedAttributeHandles),
      requireActiveSubscription,
      plannedAttributeValues = std::move(plannedAttributeValues),
      requestRegionsByAttribute = std::move(requestRegionsByAttribute)](
      FederateAmbassador& recipient) mutable {
    std::optional<umbra::detail::JoinedFederateMomAttributeValueUpdateRecipient>
        reflection;
    {
      std::scoped_lock lock(federationManagementMutex());
      auto plan = embeddedFederationManagement().registry()
                      .planJoinedFederateMomAttributeValueUpdate(
                          federationName,
                          receivingFederateId,
                          objectInstanceHandle,
                          requestedAttributeHandles,
                          requireActiveSubscription,
                          requestRegionsByAttribute
                              ? &*requestRegionsByAttribute
                              : nullptr);
      if (plan.status ==
              umbra::detail::JoinedFederateMomAttributeValueUpdateStatus::applied &&
          plan.recipient) {
        reflection = std::move(plan.recipient);
      }
    }
    if (!reflection || reflection->attributeValues.empty()) {
      return;
    }

    std::map<std::uint64_t, VariableLengthData> values;
    if (plannedAttributeValues.has_value()) {
      for (auto const& [attributeHandle, reflectedValue] :
           reflection->attributeValues) {
        auto const planned = plannedAttributeValues->find(attributeHandle);
        values.emplace(
            attributeHandle,
            planned == plannedAttributeValues->end() ? reflectedValue
                                                       : planned->second);
      }
    } else {
      values = reflection->attributeValues;
    }
    if (values.empty()) {
      return;
    }

    AttributeHandleValueMap attributeValues;
    for (auto const& [attributeHandle, value] : values) {
      attributeValues.emplace(makeAttributeHandle(attributeHandle), value);
    }
    recipient.reflectAttributeValues(
        makeObjectInstanceHandle(reflection->objectInstanceHandle),
        attributeValues,
        VariableLengthData{},
        transportationHandleFromEmbeddedName(
            umbra::detail::hla::utf8::mom::reliable,
            L"The embedded federation could not reconstruct the MOM transportation type."),
        FederateHandle{},
        nullptr);
  });
}

void queueJoinedFederateMomConditionalAttributeUpdate(
    std::wstring const& federationName,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    std::optional<std::uint64_t> excludedReceivingFederateId = std::nullopt,
    std::optional<std::map<std::uint64_t, VariableLengthData>>
        plannedAttributeValues = std::nullopt) {
  umbra::detail::JoinedFederateMomAttributeValueUpdateClassPlan plan;
  {
    std::scoped_lock lock(federationManagementMutex());
    plan = embeddedFederationManagement().registry()
               .planJoinedFederateMomAttributeValueUpdateForObject(
                   federationName,
                   objectInstanceHandle,
                   requestedAttributeHandles,
                   true,
                   excludedReceivingFederateId);
  }
  for (auto& recipient : plan.recipients) {
    if (!recipient.callbackRoute || recipient.attributeValues.empty()) {
      continue;
    }
    auto plannedValues = std::move(recipient.attributeValues);
    if (plannedAttributeValues.has_value()) {
      for (auto const& [attributeHandle, value] : *plannedAttributeValues) {
        auto const planned = plannedValues.find(attributeHandle);
        if (planned != plannedValues.end()) {
          planned->second = value;
        }
      }
    }
    queueJoinedFederateMomAttributeValueUpdate(
        std::move(recipient.callbackRoute),
        federationName,
        recipient.receivingFederateId,
        recipient.objectInstanceHandle,
        requestedAttributeHandles,
        true,
        std::move(plannedValues));
  }
}

std::optional<JoinedFederateMomConditionalWork>
joinedFederateMomConditionalWorkFor(
    umbra::detail::EmbeddedFederationRegistry& registry,
    std::wstring const& federationName,
    std::uint64_t joinedFederateId,
    std::vector<std::string_view> attributeNames) {
  auto const object = registry.joinedFederateMomObjectFor(
      federationName,
      joinedFederateId);
  if (!object) {
    return std::nullopt;
  }
  JoinedFederateMomConditionalWork result;
  result.federationName = federationName;
  result.objectInstanceHandle = object->objectInstanceHandle;
  for (std::string_view const attributeName : attributeNames) {
    auto const handle = registry.attributeHandleFor(
        federationName,
        umbra::detail::hla::utf8::mom::federate_object_class,
        std::string(attributeName));
    if (!handle) {
      return std::nullopt;
    }
    result.attributeHandles.insert(*handle);
  }
  return result;
}

std::optional<JoinedFederateMomConditionalWork>
federationMomConditionalWorkFor(
    umbra::detail::EmbeddedFederationRegistry& registry,
    std::wstring const& federationName,
    std::vector<std::string_view> attributeNames) {
  auto const object = registry.federationMomObjectFor(federationName);
  if (!object) {
    return std::nullopt;
  }
  JoinedFederateMomConditionalWork result;
  result.federationName = federationName;
  result.objectInstanceHandle = object->objectInstanceHandle;
  for (std::string_view const attributeName : attributeNames) {
    auto const handle = registry.attributeHandleFor(
        federationName,
        umbra::detail::hla::utf8::mom::federation_object_class,
        std::string(attributeName));
    if (!handle) {
      return std::nullopt;
    }
    result.attributeHandles.insert(*handle);
  }
  return result;
}

void queueJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
    std::wstring const& federationName,
    std::uint64_t joinedFederateId,
    std::vector<std::string_view> attributeNames,
    std::optional<std::uint64_t> excludedReceivingFederateId) {
  std::optional<JoinedFederateMomConditionalWork> work;
  {
    std::scoped_lock lock(federationManagementMutex());
    work = joinedFederateMomConditionalWorkFor(
        embeddedFederationManagement().registry(),
        federationName,
        joinedFederateId,
        std::move(attributeNames));
  }
  if (work) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        work->federationName,
        work->objectInstanceHandle,
        std::move(work->attributeHandles),
        excludedReceivingFederateId);
  }
}

void queueFederationMomConditionalAttributeUpdate(
    std::wstring const& federationName,
    std::vector<std::string_view> attributeNames,
    std::optional<std::uint64_t> excludedReceivingFederateId = std::nullopt) {
  std::optional<JoinedFederateMomConditionalWork> work;
  {
    std::scoped_lock lock(federationManagementMutex());
    work = federationMomConditionalWorkFor(
        embeddedFederationManagement().registry(),
        federationName,
        std::move(attributeNames));
  }
  if (work) {
    queueJoinedFederateMomConditionalAttributeUpdate(
        work->federationName,
        work->objectInstanceHandle,
        std::move(work->attributeHandles),
        excludedReceivingFederateId);
  }
}

// The registry owns each target's immutable deadline and advances it once;
// this helper applies the ordinary active-subscription MOM planner after
// releasing the federation lock. HLA_EVOKED calls it at the Evoke boundary,
// while the private HLA_IMMEDIATE scheduler calls it from its timer thread.
void pumpDueJoinedFederateMomPeriodicUpdates(std::wstring const& federationName) {
  std::vector<umbra::detail::JoinedFederateMomPeriodicUpdate> due;
  {
    std::scoped_lock lock(federationManagementMutex());
    due = embeddedFederationManagement().registry()
        .takeDueJoinedFederateMomPeriodicUpdates(
            federationName,
            std::chrono::steady_clock::now());
  }
  for (auto& update : due) {
    if (update.objectInstanceHandle == 0U || update.attributeHandles.empty()) {
      continue;
    }
    queueJoinedFederateMomConditionalAttributeUpdate(
        federationName,
        update.objectInstanceHandle,
        update.attributeHandles,
        std::nullopt,
        std::move(update.attributeValues));
  }
}

void queueObjectInstanceScopeChanges(
    std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes,
    std::wstring const& federationName) {
  for (auto& change : changes) {
    // Scope-transition planning is deliberately independent from Attribute
    // Relevance planning. Avoid enqueueing a no-op Attributes In/Out Of Scope
    // callback when the receiving federate has its separate Attribute Scope
    // Advisory switch disabled; the callback-time check below still protects
    // the enabled case from stale queued state.
    {
      std::scoped_lock lock(federationManagementMutex());
      auto const scopeSwitch = embeddedFederationManagement().registry()
          .attributeScopeAdvisorySwitchFor(federationName, change.receivingFederateId);
      if (!scopeSwitch || !*scopeSwitch) {
        continue;
      }
    }
    if (!change.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an object-instance scope change without a callback route.");
    }
    change.callbackRoute([
        federationName,
        receivingFederateId = change.receivingFederateId,
        objectInstanceHandle = change.objectInstanceHandle,
        scheduledAttributeHandles = std::move(change.attributeHandles),
        expectedInScope = change.inScope](FederateAmbassador& recipient) mutable {
      std::set<std::uint64_t> eligibleAttributeHandles;
      {
        std::scoped_lock lock(federationManagementMutex());
        eligibleAttributeHandles = embeddedFederationManagement().registry()
            .objectInstanceScopeAttributes(
                federationName,
                receivingFederateId,
                objectInstanceHandle,
                scheduledAttributeHandles,
                expectedInScope);
      }
      if (eligibleAttributeHandles.empty()) {
        return;
      }

      AttributeHandleSet attributes;
      for (std::uint64_t const attributeHandle : eligibleAttributeHandles) {
        attributes.insert(makeAttributeHandle(attributeHandle));
      }
      if (expectedInScope) {
        recipient.attributesInScope(
            makeObjectInstanceHandle(objectInstanceHandle),
            attributes);
      } else {
        recipient.attributesOutOfScope(
            makeObjectInstanceHandle(objectInstanceHandle),
            attributes);
      }
    });
  }
}

void queueAttributeRelevanceAdvisories(
    std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient> advisories,
    std::wstring const& federationName) {
  for (auto& advisory : advisories) {
    if (!advisory.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an attribute relevance advisory without a callback route.");
    }
    advisory.callbackRoute([
        federationName,
        providingFederateId = advisory.providingFederateId,
        receivingFederateId = advisory.receivingFederateId,
        objectInstanceHandle = advisory.objectInstanceHandle,
        scheduledAttributeHandles = std::move(advisory.attributeHandles),
        turnUpdatesOn = advisory.turnUpdatesOn](
        FederateAmbassador& recipient) mutable {
      std::map<std::optional<std::string>, std::set<std::uint64_t>>
          eligibleAttributeHandlesByUpdateRate;
      {
        std::scoped_lock lock(federationManagementMutex());
        auto& registry = embeddedFederationManagement().registry();
        auto const eligibleAttributeHandles = registry
            .attributeRelevanceAdvisoryAttributes(
                federationName,
                providingFederateId,
                receivingFederateId,
                objectInstanceHandle,
                scheduledAttributeHandles,
                turnUpdatesOn);
        for (std::uint64_t const attributeHandle : eligibleAttributeHandles) {
          auto const updateRateDesignator = turnUpdatesOn
              ? registry.attributeRelevanceAdvisoryUpdateRateDesignatorFor(
                    federationName,
                    receivingFederateId,
                    objectInstanceHandle,
                    attributeHandle)
              : std::nullopt;
          eligibleAttributeHandlesByUpdateRate[updateRateDesignator].insert(
              attributeHandle);
        }
      }
      if (eligibleAttributeHandlesByUpdateRate.empty()) {
        return;
      }

      for (auto const& [updateRateDesignator, eligibleAttributeHandles] :
           eligibleAttributeHandlesByUpdateRate) {
        AttributeHandleSet attributes;
        for (std::uint64_t const attributeHandle : eligibleAttributeHandles) {
          attributes.insert(makeAttributeHandle(attributeHandle));
        }
        if (turnUpdatesOn) {
          if (updateRateDesignator) {
            auto const wideDesignator = umbra::detail::wideFromUtf8(*updateRateDesignator);
            if (!wideDesignator) {
              throw RTIinternalError(
                  L"The embedded federation retained an invalid UTF-8 update-rate designator.");
            }
            recipient.turnUpdatesOnForObjectInstance(
                makeObjectInstanceHandle(objectInstanceHandle),
                attributes,
                *wideDesignator);
          } else {
            recipient.turnUpdatesOnForObjectInstance(
                makeObjectInstanceHandle(objectInstanceHandle),
                attributes);
          }
        } else {
          recipient.turnUpdatesOffForObjectInstance(
              makeObjectInstanceHandle(objectInstanceHandle),
              attributes);
        }
      }
    });
  }
}

void queueObjectInstanceRemoval(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    VariableLengthData userSuppliedTag,
    bool rtiOwnedMomObject) {
  submitReceiveOrderCallback(
      std::move(callbackRoute),
      federationName,
      receivingFederateId,
      [
      federationName,
      receivingFederateId,
      objectInstanceHandle,
      rtiOwnedMomObject,
      userSuppliedTag = std::move(userSuppliedTag),
      serviceReportRoute = std::move(serviceReportRoute)](FederateAmbassador& recipient) mutable {
    std::optional<umbra::detail::RemovedObjectInstanceSnapshot> removal;
    {
      std::scoped_lock lock(federationManagementMutex());
      auto& registry = embeddedFederationManagement().registry();
      removal = rtiOwnedMomObject
          ? registry.beginJoinedFederateMomObjectRemoval(
                federationName,
                receivingFederateId,
                objectInstanceHandle)
          : registry.beginObjectInstanceRemoval(
                federationName,
                receivingFederateId,
                objectInstanceHandle);
    }
    if (!removal) {
      return;
    }

    if (serviceReportRoute) {
      // §6.17 is RTI-initiated at the receiving federate. The callback-time
      // transition above is therefore the only point at which a selected-file
      // report may reserve a serial and become durable.
      serviceReportRoute(
          static_cast<std::uint16_t>(umbra::detail::MomServiceType::object_management),
          [
              objectInstanceHandle = removal->objectInstanceHandle,
              userSuppliedTag,
              producingFederateId = removal->producingFederateId](
              std::uint32_t serialNumber) {
            return removeObjectInstanceServiceReportRecord(
                objectInstanceHandle,
                userSuppliedTag,
                producingFederateId,
                RECEIVE,
                nullptr,
                std::nullopt,
                std::nullopt,
                serialNumber);
          });
    }

    // Removal commits the recipient's transition to unknown before its
    // callback. That mirrors the lifecycle boundary rather than preserving a
    // stale instance through user code after Remove Object Instance begins.
    recipient.removeObjectInstance(
        makeObjectInstanceHandle(removal->objectInstanceHandle),
        userSuppliedTag,
        rtiOwnedMomObject
            ? FederateHandle{}
            : makeFederateHandle(removal->producingFederateId));
  });
}

void queueObjectInstanceRemovals(
    std::vector<umbra::detail::ObjectInstanceRemovalRecipient> removals,
    std::wstring const& federationName,
    VariableLengthData const& userSuppliedTag) {
  // As with discovery, callback submission runs after all sender and
  // federation locks are released. HLA_IMMEDIATE may synchronously enter a
  // recipient's Remove Object Instance callback.
  for (std::size_t index = 0; index < removals.size(); ++index) {
    auto& removal = removals[index];
    try {
      if (!removal.callbackRoute) {
        throw RTIinternalError(
            L"The embedded federation has a removal recipient without a callback route.");
      }
      queueObjectInstanceRemoval(
          std::move(removal.callbackRoute),
          std::move(removal.serviceReportRoute),
          federationName,
          removal.receivingFederateId,
          removal.objectInstanceHandle,
          userSuppliedTag,
          removal.rtiOwnedMomObject);
    } catch (...) {
      // A synchronous callback may already have removed the current
      // recipient's known state. Clear only still-pending reservations so the
      // remaining known recipients are not stranded in an in-flight state.
      std::scoped_lock lock(federationManagementMutex());
      auto& registry = embeddedFederationManagement().registry();
      for (std::size_t pending = index; pending < removals.size(); ++pending) {
        if (removals[pending].rtiOwnedMomObject) {
          registry.cancelJoinedFederateMomObjectRemoval(
              federationName,
              removals[pending].receivingFederateId,
              removals[pending].objectInstanceHandle);
        } else {
          registry.cancelObjectInstanceRemoval(
              federationName,
              removals[pending].receivingFederateId,
              removals[pending].objectInstanceHandle);
        }
      }
      throw;
    }
  }
}

void queueTimestampedObjectInstanceRemoval(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t messageId,
    VariableLengthData userSuppliedTag,
    std::shared_ptr<LogicalTime const> timestamp,
    bool provideRetraction,
    rti1516_2025::OrderType sentOrderType,
    rti1516_2025::OrderType receivedOrderType) {
  if (!callbackRoute || !timestamp || messageId == 0) {
    throw RTIinternalError(
        L"The embedded federation has an incomplete timestamped object-removal payload.");
  }
  submitReceiveOrderCallback(
      std::move(callbackRoute),
      federationName,
      receivingFederateId,
      [
      federationName,
      receivingFederateId,
      objectInstanceHandle,
      messageId,
      userSuppliedTag = std::move(userSuppliedTag),
      timestamp = std::move(timestamp),
      provideRetraction,
      sentOrderType,
      receivedOrderType,
      serviceReportRoute = std::move(serviceReportRoute)](FederateAmbassador& recipient) mutable {
    std::optional<umbra::detail::RemovedObjectInstanceSnapshot> removal;
    {
      std::scoped_lock lock(federationManagementMutex());
      removal = embeddedFederationManagement().registry().beginTsoObjectInstanceRemoval(
          federationName,
          receivingFederateId,
          objectInstanceHandle,
          messageId);
    }
    if (!removal) {
      // Local Delete Object Instance, a resignation, or a concurrent
      // retraction can make the accepted removal ineligible before this
      // receive-order callback enters user code. Close a still-pending
      // timestamped recipient as suppressed so its retraction ledger and
      // object-lifetime guard do not retain a phantom delivery.
      {
        std::scoped_lock lock(federationManagementMutex());
        static_cast<void>(embeddedFederationManagement().registry()
                              .finishTsoRecipientCallbackSuppressed(
                                  federationName,
                                  receivingFederateId,
                                  messageId));
      }
      return;
    }

    if (serviceReportRoute) {
      // This path is timestamped at the sending federate, but reaches a
      // non-time-constrained recipient through its receive-order callback
      // boundary. Record both order types explicitly before user code runs.
      serviceReportRoute(
          static_cast<std::uint16_t>(umbra::detail::MomServiceType::object_management),
          [
              objectInstanceHandle = removal->objectInstanceHandle,
              userSuppliedTag,
              producingFederateId = removal->producingFederateId,
              timestamp,
              messageId,
              provideRetraction,
              sentOrderType,
              receivedOrderType](std::uint32_t serialNumber) {
            return removeObjectInstanceServiceReportRecord(
                objectInstanceHandle,
                userSuppliedTag,
                producingFederateId,
                sentOrderType,
                timestamp.get(),
                receivedOrderType,
                provideRetraction
                    ? std::optional<std::uint64_t>{messageId}
                    : std::nullopt,
                serialNumber);
          });
    }

    std::optional<MessageRetractionHandle> retraction;
    if (provideRetraction) {
      retraction.emplace(makeMessageRetractionHandle(messageId));
    }
    recipient.removeObjectInstance(
        makeObjectInstanceHandle(removal->objectInstanceHandle),
        userSuppliedTag,
        makeFederateHandle(removal->producingFederateId),
        *timestamp,
        sentOrderType,
        receivedOrderType,
        retraction ? &*retraction : nullptr);
  });
}

}  // namespace
#endif
}  // namespace rti1516_2025::umbra_binding_detail
