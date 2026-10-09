#pragma once

#include "internal/federation/federation_registry_service_types.hpp"

namespace umbra::detail {

// Private state/results for the bounded 2025 order-type control services.
// Order changes are synchronous in the standard API: class defaults affect
// future instances, instance changes affect the selected owned attributes,
// and interaction changes affect future sends by the invoking publisher.
enum class AttributeOrderTypeChangeStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_instance_not_known,
  attribute_not_owned,
  attribute_not_defined,
  invalid_order_type,
};

enum class AttributeOrderTypeDefaultStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_class_not_defined,
  attribute_not_defined,
  invalid_order_type,
};

enum class InteractionOrderTypeChangeStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  interaction_class_not_published,
  interaction_class_not_defined,
  invalid_order_type,
};

// Private 2025 transportation-type control results.  The embedded profile
// supports the two mandatory standard transport names and keeps the state
// federation-owned so receive-order and timestamped planners observe the
// same effective type.  The callback boundary is the commit point for a
// requested change; a default change is prospective and does not rewrite
// already registered/owned instance attributes.
enum class AttributeTransportationTypeChangeStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_instance_not_known,
  attribute_already_being_changed,
  attribute_not_owned,
  attribute_not_defined,
  invalid_transportation_type,
  callback_route_missing,
  inconsistent_catalog,
};

struct AttributeTransportationTypeChangePlan {
  AttributeTransportationTypeChangeStatus status =
      AttributeTransportationTypeChangeStatus::applied;
  std::uint64_t requestId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
  std::string transportationName;
  ObjectInstanceCallbackRoute callbackRoute;
};

// A fresh-registry restore has no serialized callback closure for a pending
// attribute transportation-type change. Keep the durable request identity
// separate from the public plan so the ambassador can rebind it to the live
// requester route after Federation Restored.
struct AttributeTransportationTypeChangeWorkItem {
  std::uint64_t requestingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t requestId = 0;
  ObjectInstanceCallbackRoute callbackRoute;
};

struct AttributeTransportationTypeChangeDelivery {
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
  std::string transportationName;
};

enum class AttributeTransportationTypeDefaultStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_class_not_defined,
  attribute_not_defined,
  invalid_transportation_type,
  inconsistent_catalog,
};

enum class AttributeTransportationTypeQueryStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_instance_not_known,
  attribute_not_defined,
  callback_route_missing,
  inconsistent_catalog,
};

struct AttributeTransportationTypeQueryPlan {
  AttributeTransportationTypeQueryStatus status =
      AttributeTransportationTypeQueryStatus::applied;
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t attributeHandle = 0;
  std::string transportationName;
  ObjectInstanceCallbackRoute callbackRoute;
};

enum class InteractionTransportationTypeChangeStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  interaction_class_already_being_changed,
  interaction_class_not_published,
  interaction_class_not_defined,
  invalid_transportation_type,
  callback_route_missing,
  inconsistent_catalog,
};

struct InteractionTransportationTypeChangePlan {
  InteractionTransportationTypeChangeStatus status =
      InteractionTransportationTypeChangeStatus::applied;
  std::uint64_t interactionClassHandle = 0;
  std::string transportationName;
  InteractionCallbackRoute callbackRoute;
};

// A fresh-registry restore has no serialized callback closure for a pending
// interaction transportation-type change. Keep the durable class identity
// separate from the public plan so the ambassador can rebind it to the live
// requester route after Federation Restored.
struct InteractionTransportationTypeChangeWorkItem {
  std::uint64_t requestingFederateId = 0;
  std::uint64_t interactionClassHandle = 0;
  InteractionCallbackRoute callbackRoute;
};

enum class InteractionTransportationTypeQueryStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  interaction_class_not_defined,
  callback_route_missing,
  inconsistent_catalog,
};

struct InteractionTransportationTypeQueryPlan {
  InteractionTransportationTypeQueryStatus status =
      InteractionTransportationTypeQueryStatus::applied;
  std::uint64_t queriedFederateId = 0;
  std::uint64_t interactionClassHandle = 0;
  std::string transportationName;
  InteractionCallbackRoute callbackRoute;
};

}  // namespace umbra::detail
