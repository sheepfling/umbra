#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/RtiConfiguration.h>
#include <RTI/auth/HLAnoCredentials.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAopaqueData.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/encoding/HLAfixedArray.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariantRecord.h>
#include <RTI/encoding/HLAextendableVariantRecord.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64TimeFactory.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/LogicalTimeFactory.h>

#include "internal/attribute_handle.hpp"
#include "internal/dimension_handle.hpp"
#include "internal/federate_handle.hpp"
#include "internal/interaction_class_handle.hpp"
#include "internal/message_retraction_handle.hpp"
#include "internal/object_class_handle.hpp"
#include "internal/object_instance_handle.hpp"
#include "internal/parameter_handle.hpp"
#include "internal/region_handle.hpp"
#include "internal/transportation_type_handle.hpp"

#include <pybind11/pybind11.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <set>
#include <utility>
#include <vector>

namespace py = pybind11;
namespace rti = rti1516_2025;

namespace {

PyObject* native_rti_error_type = nullptr;

std::string utf8(std::wstring const& value) {
  return py::cast(value).cast<std::string>();
}

std::wstring wide(std::string const& value) {
  return py::str(value).cast<std::wstring>();
}

template <typename Handle>
py::bytes encoded(Handle const& handle) {
  auto const value = handle.encode();
  return py::bytes(static_cast<char const*>(value.data()), value.size());
}

template <typename Element>
py::bytes element_bytes(Element const& element) {
  auto const value = element.encode();
  return py::bytes(static_cast<char const*>(value.data()), value.size());
}

py::bytes variable_bytes(rti::VariableLengthData const& value) {
  return py::bytes(static_cast<char const*>(value.data()), value.size());
}

py::dict logical_time_snapshot(rti::LogicalTime const& value) {
  py::dict result;
  result["encodedValue"] = variable_bytes(value.encode());
  result["implementationName"] = utf8(value.implementationName());
  result["initial"] = value.isInitial();
  result["final"] = value.isFinal();
  result["text"] = utf8(value.toString());
  if (auto const* integer = dynamic_cast<rti::HLAinteger64Time const*>(&value)) {
    result["value"] = integer->getTime();
  } else if (auto const* floating = dynamic_cast<rti::HLAfloat64Time const*>(&value)) {
    result["value"] = floating->getTime();
  } else {
    result["value"] = py::none();
  }
  return result;
}

py::dict logical_interval_snapshot(rti::LogicalTimeInterval const& value) {
  py::dict result;
  result["encodedValue"] = variable_bytes(value.encode());
  result["implementationName"] = utf8(value.implementationName());
  result["zero"] = value.isZero();
  result["epsilon"] = value.isEpsilon();
  result["text"] = utf8(value.toString());
  if (auto const* integer = dynamic_cast<rti::HLAinteger64Interval const*>(&value)) {
    result["value"] = integer->getInterval();
  } else if (auto const* floating = dynamic_cast<rti::HLAfloat64Interval const*>(&value)) {
    result["value"] = floating->getInterval();
  } else {
    result["value"] = py::none();
  }
  return result;
}

py::object python_logical_time(rti::LogicalTime const& value) {
  py::gil_scoped_acquire acquire;
  auto const api = py::module_::import("hla.rti1516_2025");
  auto const implementation = utf8(value.implementationName());
  auto const type = implementation == "HLAinteger64Time"
      ? api.attr("HLAinteger64Time")
      : implementation == "HLAfloat64Time"
          ? api.attr("HLAfloat64Time")
          : api.attr("LogicalTime");
  py::object numeric = py::none();
  if (auto const* integer = dynamic_cast<rti::HLAinteger64Time const*>(&value)) {
    numeric = py::int_(integer->getTime());
  } else if (auto const* floating = dynamic_cast<rti::HLAfloat64Time const*>(&value)) {
    numeric = py::float_(floating->getTime());
  }
  return type(
      variable_bytes(value.encode()),
      implementation,
      value.isInitial(),
      value.isFinal(),
      numeric,
      utf8(value.toString()));
}

rti::VariableLengthData variable_length_data(py::bytes const& value) {
  std::string const copied = value;
  return rti::VariableLengthData(copied.data(), copied.size());
}

rti::AttributeHandleSet attribute_handles_from(py::iterable const& encoded_attributes) {
  rti::AttributeHandleSet result;
  for (py::handle const item : encoded_attributes) {
    result.insert(rti::umbra_binding_detail::decodeAttributeHandle(
        variable_length_data(py::cast<py::bytes>(item))));
  }
  return result;
}

rti::InteractionClassHandleSet interaction_class_handles_from(
    py::iterable const& encoded_interactions) {
  rti::InteractionClassHandleSet result;
  for (py::handle const item : encoded_interactions) {
    result.insert(rti::umbra_binding_detail::decodeInteractionClassHandle(
        variable_length_data(py::cast<py::bytes>(item))));
  }
  return result;
}

rti::DimensionHandleSet dimension_handles_from(py::iterable const& encoded_dimensions) {
  rti::DimensionHandleSet result;
  for (py::handle const item : encoded_dimensions) {
    result.insert(rti::umbra_binding_detail::decodeDimensionHandle(
        variable_length_data(py::cast<py::bytes>(item))));
  }
  return result;
}

rti::RegionHandleSet region_handles_from(py::iterable const& encoded_regions) {
  rti::RegionHandleSet result;
  for (py::handle const item : encoded_regions) {
    result.insert(rti::umbra_binding_detail::decodeRegionHandle(
        variable_length_data(py::cast<py::bytes>(item))));
  }
  return result;
}

rti::FederateHandleSet federate_handles_from(py::iterable const& encoded_federates) {
  rti::FederateHandleSet result;
  for (py::handle const item : encoded_federates) {
    result.insert(rti::umbra_binding_detail::decodeFederateHandle(
        variable_length_data(py::cast<py::bytes>(item))));
  }
  return result;
}

std::vector<std::wstring> wide_strings_from(py::iterable const& values) {
  std::vector<std::wstring> result;
  for (py::handle const item : values) {
    result.emplace_back(py::str(item).cast<std::wstring>());
  }
  return result;
}

rti::AttributeHandleSetRegionHandleSetPairVector attribute_region_pairs_from(
    py::iterable const& encoded_pairs) {
  rti::AttributeHandleSetRegionHandleSetPairVector result;
  for (py::handle const item : encoded_pairs) {
    py::tuple const pair = py::cast<py::tuple>(item);
    if (pair.size() != 2) {
      throw std::invalid_argument(
          "Each attribute-region entry must contain an attribute set and region set.");
    }
    result.emplace_back(
        attribute_handles_from(py::cast<py::iterable>(pair[0])),
        region_handles_from(py::cast<py::iterable>(pair[1])));
  }
  return result;
}

py::object python_region_handle_set(rti::RegionHandleSet const& regions) {
  py::gil_scoped_acquire acquire;
  auto const api = py::module_::import("hla.rti1516_2025");
  py::list values;
  for (auto const& region : regions) {
    values.append(api.attr("RegionHandle")(encoded(region)));
  }
  return api.attr("RegionHandleSet")(values);
}

py::object python_attribute_handle_set(rti::AttributeHandleSet const& attributes) {
  py::gil_scoped_acquire acquire;
  auto const api = py::module_::import("hla.rti1516_2025");
  py::list values;
  for (auto const& attribute : attributes) {
    values.append(api.attr("AttributeHandle")(encoded(attribute)));
  }
  return api.attr("AttributeHandleSet")(values);
}

py::object python_dimension_handle_set(rti::DimensionHandleSet const& dimensions) {
  py::gil_scoped_acquire acquire;
  auto const api = py::module_::import("hla.rti1516_2025");
  py::list values;
  for (auto const& dimension : dimensions) {
    values.append(api.attr("DimensionHandle")(encoded(dimension)));
  }
  return api.attr("DimensionHandleSet")(values);
}

rti::AttributeHandleValueMap attribute_values_from(py::iterable const& encoded_attributes) {
  rti::AttributeHandleValueMap result;
  for (py::handle const item : encoded_attributes) {
    py::tuple const pair = py::cast<py::tuple>(item);
    if (pair.size() != 2) {
      throw std::invalid_argument("Each attribute-value entry must contain a handle and value.");
    }
    result.emplace(
        rti::umbra_binding_detail::decodeAttributeHandle(
            variable_length_data(py::cast<py::bytes>(pair[0]))),
        variable_length_data(py::cast<py::bytes>(pair[1])));
  }
  return result;
}

rti::ParameterHandleValueMap parameter_values_from(py::iterable const& encoded_parameters) {
  rti::ParameterHandleValueMap result;
  for (py::handle const item : encoded_parameters) {
    py::tuple const pair = py::cast<py::tuple>(item);
    if (pair.size() != 2) {
      throw std::invalid_argument("Each parameter-value entry must contain a handle and value.");
    }
    result.emplace(
        rti::umbra_binding_detail::decodeParameterHandle(
            variable_length_data(py::cast<py::bytes>(pair[0]))),
        variable_length_data(py::cast<py::bytes>(pair[1])));
  }
  return result;
}

rti::ResignAction resign_action_from(std::string const& value) {
  if (value == "NO_ACTION") return rti::NO_ACTION;
  if (value == "UNCONDITIONALLY_DIVEST_ATTRIBUTES") return rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES;
  if (value == "DELETE_OBJECTS") return rti::DELETE_OBJECTS;
  if (value == "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS") return rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS;
  if (value == "DELETE_OBJECTS_THEN_DIVEST") return rti::DELETE_OBJECTS_THEN_DIVEST;
  if (value == "CANCEL_THEN_DELETE_THEN_DIVEST") return rti::CANCEL_THEN_DELETE_THEN_DIVEST;
  throw rti::InvalidResignAction(L"Unknown Python ResignAction value.");
}

std::string resign_action_name(rti::ResignAction value) {
  switch (value) {
    case rti::NO_ACTION: return "NO_ACTION";
    case rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES: return "UNCONDITIONALLY_DIVEST_ATTRIBUTES";
    case rti::DELETE_OBJECTS: return "DELETE_OBJECTS";
    case rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS:
      return "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS";
    case rti::DELETE_OBJECTS_THEN_DIVEST: return "DELETE_OBJECTS_THEN_DIVEST";
    case rti::CANCEL_THEN_DELETE_THEN_DIVEST: return "CANCEL_THEN_DELETE_THEN_DIVEST";
  }
  throw rti::InvalidResignAction(L"Unknown C++ ResignAction value.");
}

rti::OrderType order_type_from(std::string const& value) {
  if (value == "RECEIVE" || value == "Receive") return rti::RECEIVE;
  if (value == "TIMESTAMP" || value == "TimeStamp") return rti::TIMESTAMP;
  throw rti::InvalidOrderName(L"Unknown Python OrderType value.");
}

std::string order_type_name(rti::OrderType value) {
  switch (value) {
    case rti::RECEIVE: return "Receive";
    case rti::TIMESTAMP: return "TimeStamp";
  }
  throw rti::InvalidOrderType(L"Unknown C++ OrderType value.");
}

py::object python_order_type(rti::OrderType value) {
  auto const api = py::module_::import("hla.rti1516_2025");
  return api.attr("OrderType").attr(
      value == rti::RECEIVE ? "RECEIVE" : "TIMESTAMP");
}

py::object python_retraction_handle(rti::MessageRetractionHandle const* value) {
  if (value == nullptr) {
    return py::none();
  }
  auto const api = py::module_::import("hla.rti1516_2025");
  return api.attr("MessageRetractionHandle")(encoded(*value));
}

rti::ServiceGroup service_group_from(std::string const& value) {
  if (value == "FEDERATION_MANAGEMENT") return rti::FEDERATION_MANAGEMENT;
  if (value == "DECLARATION_MANAGEMENT") return rti::DECLARATION_MANAGEMENT;
  if (value == "OBJECT_MANAGEMENT") return rti::OBJECT_MANAGEMENT;
  if (value == "OWNERSHIP_MANAGEMENT") return rti::OWNERSHIP_MANAGEMENT;
  if (value == "TIME_MANAGEMENT") return rti::TIME_MANAGEMENT;
  if (value == "DATA_DISTRIBUTION_MANAGEMENT") return rti::DATA_DISTRIBUTION_MANAGEMENT;
  if (value == "SUPPORT_SERVICES") return rti::SUPPORT_SERVICES;
  throw rti::InvalidServiceGroup(L"Unknown Python ServiceGroup value.");
}

class PythonFederateAmbassador final : public rti::NullFederateAmbassador {
 public:
  explicit PythonFederateAmbassador(py::object callback_target)
      : callback_target_(std::move(callback_target)) {}

  void connectionLost(std::wstring const& fault_description) override {
    py::gil_scoped_acquire acquire;
    try {
      callback_target_.attr("connectionLost")(utf8(fault_description));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python connectionLost callback failed");
    }
  }

  void reportFederationExecutions(
      rti::FederationExecutionInformationVector const& report) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      auto const information_type = api.attr("FederationExecutionInformation");
      auto const information_set_type = api.attr("FederationExecutionInformationSet");
      py::list records;
      for (auto const& information : report) {
        records.append(information_type(
            utf8(information.federationExecutionName),
            utf8(information.logicalTimeImplementationName)));
      }
      callback_target_.attr("reportFederationExecutions")(information_set_type(records));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python reportFederationExecutions callback failed");
    }
  }

  void reportFederationExecutionMembers(
      std::wstring const& federation_name,
      rti::FederationExecutionMemberInformationVector const& report) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      auto const information_type = api.attr("FederationExecutionMemberInformation");
      auto const information_set_type = api.attr("FederationExecutionMemberInformationSet");
      py::list records;
      for (auto const& information : report) {
        records.append(information_type(
            utf8(information.federateName),
            utf8(information.federateType)));
      }
      callback_target_.attr("reportFederationExecutionMembers")(
          utf8(federation_name), information_set_type(records));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python reportFederationExecutionMembers callback failed");
    }
  }

  void reportFederationExecutionDoesNotExist(
      std::wstring const& federation_name) override {
    py::gil_scoped_acquire acquire;
    try {
      callback_target_.attr("reportFederationExecutionDoesNotExist")(utf8(federation_name));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python reportFederationExecutionDoesNotExist callback failed");
    }
  }

  void federateResigned(std::wstring const& reason) override {
    py::gil_scoped_acquire acquire;
    try {
      callback_target_.attr("federateResigned")(utf8(reason));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python federateResigned callback failed");
    }
  }

  void startRegistrationForObjectClass(
      rti::ObjectClassHandle const& object_class) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("startRegistrationForObjectClass")(
          api.attr("ObjectClassHandle")(encoded(object_class)));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python startRegistrationForObjectClass callback failed");
    }
  }

  void stopRegistrationForObjectClass(
      rti::ObjectClassHandle const& object_class) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("stopRegistrationForObjectClass")(
          api.attr("ObjectClassHandle")(encoded(object_class)));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python stopRegistrationForObjectClass callback failed");
    }
  }

  void turnInteractionsOn(
      rti::InteractionClassHandle const& interaction_class) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("turnInteractionsOn")(
          api.attr("InteractionClassHandle")(encoded(interaction_class)));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python turnInteractionsOn callback failed");
    }
  }

  void turnInteractionsOff(
      rti::InteractionClassHandle const& interaction_class) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("turnInteractionsOff")(
          api.attr("InteractionClassHandle")(encoded(interaction_class)));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python turnInteractionsOff callback failed");
    }
  }

  void discoverObjectInstance(
      rti::ObjectInstanceHandle const& object_instance,
      rti::ObjectClassHandle const& object_class,
      std::wstring const& object_instance_name,
      rti::FederateHandle const& producing_federate) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("discoverObjectInstance")(
          api.attr("ObjectInstanceHandle")(encoded(object_instance)),
          api.attr("ObjectClassHandle")(encoded(object_class)),
          utf8(object_instance_name),
          api.attr("FederateHandle")(encoded(producing_federate)));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python discoverObjectInstance callback failed");
    }
  }

  void objectInstanceNameReservationSucceeded(
      std::wstring const& object_instance_name) override {
    py::gil_scoped_acquire acquire;
    try {
      callback_target_.attr("objectInstanceNameReservationSucceeded")(
          utf8(object_instance_name));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python objectInstanceNameReservationSucceeded callback failed");
    }
  }

  void objectInstanceNameReservationFailed(
      std::wstring const& object_instance_name) override {
    py::gil_scoped_acquire acquire;
    try {
      callback_target_.attr("objectInstanceNameReservationFailed")(
          utf8(object_instance_name));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python objectInstanceNameReservationFailed callback failed");
    }
  }

  void multipleObjectInstanceNameReservationSucceeded(
      std::set<std::wstring> const& object_instance_names) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      py::list names;
      for (auto const& name : object_instance_names) {
        names.append(utf8(name));
      }
      callback_target_.attr("multipleObjectInstanceNameReservationSucceeded")(
          api.attr("ObjectInstanceNameSet")(names));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python multipleObjectInstanceNameReservationSucceeded callback failed");
    }
  }

  void multipleObjectInstanceNameReservationFailed(
      std::set<std::wstring> const& object_instance_names) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      py::list names;
      for (auto const& name : object_instance_names) {
        names.append(utf8(name));
      }
      callback_target_.attr("multipleObjectInstanceNameReservationFailed")(
          api.attr("ObjectInstanceNameSet")(names));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python multipleObjectInstanceNameReservationFailed callback failed");
    }
  }

  void removeObjectInstance(
      rti::ObjectInstanceHandle const& object_instance,
      rti::VariableLengthData const& user_supplied_tag,
      rti::FederateHandle const& producing_federate) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      auto const* tag_bytes = static_cast<char const*>(user_supplied_tag.data());
      callback_target_.attr("removeObjectInstance")(
          api.attr("ObjectInstanceHandle")(encoded(object_instance)),
          py::bytes(tag_bytes, user_supplied_tag.size()),
          api.attr("FederateHandle")(encoded(producing_federate)));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python removeObjectInstance callback failed");
    }
  }

  void provideAttributeValueUpdate(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& user_supplied_tag) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      py::list handles;
      for (auto const& attribute : attributes) {
        handles.append(api.attr("AttributeHandle")(encoded(attribute)));
      }
      auto const* tag_bytes = static_cast<char const*>(user_supplied_tag.data());
      callback_target_.attr("provideAttributeValueUpdate")(
          api.attr("ObjectInstanceHandle")(encoded(object_instance)),
          api.attr("AttributeHandleSet")(handles),
          py::bytes(tag_bytes, user_supplied_tag.size()));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python provideAttributeValueUpdate callback failed");
    }
  }

  void attributesInScope(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& attributes) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("attributesInScope")(
          api.attr("ObjectInstanceHandle")(encoded(object_instance)),
          python_attribute_handle_set(attributes));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python attributesInScope callback failed");
    }
  }

  void attributesOutOfScope(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& attributes) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("attributesOutOfScope")(
          api.attr("ObjectInstanceHandle")(encoded(object_instance)),
          python_attribute_handle_set(attributes));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python attributesOutOfScope callback failed");
    }
  }

  void turnUpdatesOnForObjectInstance(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& attributes) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("turnUpdatesOnForObjectInstance")(
          api.attr("ObjectInstanceHandle")(encoded(object_instance)),
          python_attribute_handle_set(attributes));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python turnUpdatesOnForObjectInstance callback failed");
    }
  }

  void turnUpdatesOnForObjectInstance(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& attributes,
      std::wstring const& update_rate_designator) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("turnUpdatesOnForObjectInstance")(
          api.attr("ObjectInstanceHandle")(encoded(object_instance)),
          python_attribute_handle_set(attributes),
          utf8(update_rate_designator));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python turnUpdatesOnForObjectInstance callback failed");
    }
  }

  void turnUpdatesOffForObjectInstance(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& attributes) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("turnUpdatesOffForObjectInstance")(
          api.attr("ObjectInstanceHandle")(encoded(object_instance)),
          python_attribute_handle_set(attributes));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python turnUpdatesOffForObjectInstance callback failed");
    }
  }

  void confirmAttributeTransportationTypeChange(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& attributes,
      rti::TransportationTypeHandle const& transportation_type) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("confirmAttributeTransportationTypeChange")(
          api.attr("ObjectInstanceHandle")(encoded(object_instance)),
          python_attribute_handle_set(attributes),
          api.attr("TransportationTypeHandle")(encoded(transportation_type)));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python confirmAttributeTransportationTypeChange callback failed");
    }
  }

  void reportAttributeTransportationType(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandle const& attribute,
      rti::TransportationTypeHandle const& transportation_type) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("reportAttributeTransportationType")(
          api.attr("ObjectInstanceHandle")(encoded(object_instance)),
          api.attr("AttributeHandle")(encoded(attribute)),
          api.attr("TransportationTypeHandle")(encoded(transportation_type)));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python reportAttributeTransportationType callback failed");
    }
  }

  void confirmInteractionTransportationTypeChange(
      rti::InteractionClassHandle const& interaction_class,
      rti::TransportationTypeHandle const& transportation_type) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("confirmInteractionTransportationTypeChange")(
          api.attr("InteractionClassHandle")(encoded(interaction_class)),
          api.attr("TransportationTypeHandle")(encoded(transportation_type)));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python confirmInteractionTransportationTypeChange callback failed");
    }
  }

  void reportInteractionTransportationType(
      rti::FederateHandle const& federate,
      rti::InteractionClassHandle const& interaction_class,
      rti::TransportationTypeHandle const& transportation_type) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("reportInteractionTransportationType")(
          api.attr("FederateHandle")(encoded(federate)),
          api.attr("InteractionClassHandle")(encoded(interaction_class)),
          api.attr("TransportationTypeHandle")(encoded(transportation_type)));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python reportInteractionTransportationType callback failed");
    }
  }

  void removeObjectInstance(
      rti::ObjectInstanceHandle const& object_instance,
      rti::VariableLengthData const& user_supplied_tag,
      rti::FederateHandle const& producing_federate,
      rti::LogicalTime const& time,
      rti::OrderType sent_order_type,
      rti::OrderType received_order_type,
      rti::MessageRetractionHandle const* optional_retraction) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      auto const* tag_bytes = static_cast<char const*>(user_supplied_tag.data());
      auto callback = callback_target_.attr("removeObjectInstance");
      py::tuple timed_arguments = py::make_tuple(
          api.attr("ObjectInstanceHandle")(encoded(object_instance)),
          py::bytes(tag_bytes, user_supplied_tag.size()),
          api.attr("FederateHandle")(encoded(producing_federate)),
          python_logical_time(time),
          python_order_type(sent_order_type),
          python_order_type(received_order_type),
          python_retraction_handle(optional_retraction));
      try {
        callback(*timed_arguments);
      } catch (py::error_already_set const& error) {
        if (!error.matches(PyExc_TypeError)) throw;
        PyErr_Clear();
        callback(
            api.attr("ObjectInstanceHandle")(encoded(object_instance)),
            py::bytes(tag_bytes, user_supplied_tag.size()),
            api.attr("FederateHandle")(encoded(producing_federate)));
      }
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python timed removeObjectInstance callback failed");
    }
  }

  void reflectAttributeValues(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleValueMap const& attribute_values,
      rti::VariableLengthData const& user_supplied_tag,
      rti::TransportationTypeHandle const& transportation_type,
      rti::FederateHandle const& producing_federate,
      rti::RegionHandleSet const* optional_sent_regions) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      py::dict values;
      for (auto const& [attribute, value] : attribute_values) {
        auto const* value_bytes = static_cast<char const*>(value.data());
        values[api.attr("AttributeHandle")(encoded(attribute))] =
            py::bytes(value_bytes, value.size());
      }
      auto const* tag_bytes = static_cast<char const*>(user_supplied_tag.data());
      auto callback = callback_target_.attr("reflectAttributeValues");
      if (optional_sent_regions == nullptr) {
        callback(
            api.attr("ObjectInstanceHandle")(encoded(object_instance)),
            api.attr("AttributeHandleValueMap")(values),
            py::bytes(tag_bytes, user_supplied_tag.size()),
            api.attr("TransportationTypeHandle")(encoded(transportation_type)),
            api.attr("FederateHandle")(encoded(producing_federate)));
      } else {
        callback(
            api.attr("ObjectInstanceHandle")(encoded(object_instance)),
            api.attr("AttributeHandleValueMap")(values),
            py::bytes(tag_bytes, user_supplied_tag.size()),
            api.attr("TransportationTypeHandle")(encoded(transportation_type)),
            api.attr("FederateHandle")(encoded(producing_federate)),
            python_region_handle_set(*optional_sent_regions));
      }
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python reflectAttributeValues callback failed");
    }
  }

  void reflectAttributeValues(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleValueMap const& attribute_values,
      rti::VariableLengthData const& user_supplied_tag,
      rti::TransportationTypeHandle const& transportation_type,
      rti::FederateHandle const& producing_federate,
      rti::RegionHandleSet const* optional_sent_regions,
      rti::LogicalTime const& time,
      rti::OrderType sent_order_type,
      rti::OrderType received_order_type,
      rti::MessageRetractionHandle const* optional_retraction) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      py::dict values;
      for (auto const& [attribute, value] : attribute_values) {
        auto const* value_bytes = static_cast<char const*>(value.data());
        values[api.attr("AttributeHandle")(encoded(attribute))] =
            py::bytes(value_bytes, value.size());
      }
      auto const* tag_bytes = static_cast<char const*>(user_supplied_tag.data());
      auto callback = callback_target_.attr("reflectAttributeValues");
      py::object sent_regions = optional_sent_regions == nullptr
          ? py::none()
          : python_region_handle_set(*optional_sent_regions);
      py::tuple timed_arguments = py::make_tuple(
          api.attr("ObjectInstanceHandle")(encoded(object_instance)),
          api.attr("AttributeHandleValueMap")(values),
          py::bytes(tag_bytes, user_supplied_tag.size()),
          api.attr("TransportationTypeHandle")(encoded(transportation_type)),
          api.attr("FederateHandle")(encoded(producing_federate)),
          sent_regions,
          python_logical_time(time),
          python_order_type(sent_order_type),
          python_order_type(received_order_type),
          python_retraction_handle(optional_retraction));
      try {
        callback(*timed_arguments);
      } catch (py::error_already_set const& error) {
        if (!error.matches(PyExc_TypeError)) throw;
        PyErr_Clear();
        if (optional_sent_regions == nullptr) {
          callback(
              api.attr("ObjectInstanceHandle")(encoded(object_instance)),
              api.attr("AttributeHandleValueMap")(values),
              py::bytes(tag_bytes, user_supplied_tag.size()),
              api.attr("TransportationTypeHandle")(encoded(transportation_type)),
              api.attr("FederateHandle")(encoded(producing_federate)));
        } else {
          callback(
              api.attr("ObjectInstanceHandle")(encoded(object_instance)),
              api.attr("AttributeHandleValueMap")(values),
              py::bytes(tag_bytes, user_supplied_tag.size()),
              api.attr("TransportationTypeHandle")(encoded(transportation_type)),
              api.attr("FederateHandle")(encoded(producing_federate)),
              sent_regions);
        }
      }
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python timed reflectAttributeValues callback failed");
    }
  }

  void receiveInteraction(
      rti::InteractionClassHandle const& interaction_class,
      rti::ParameterHandleValueMap const& parameter_values,
      rti::VariableLengthData const& user_supplied_tag,
      rti::TransportationTypeHandle const& transportation_type,
      rti::FederateHandle const& producing_federate,
      rti::RegionHandleSet const* optional_sent_regions) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      py::dict values;
      for (auto const& [parameter, value] : parameter_values) {
        auto const* value_bytes = static_cast<char const*>(value.data());
        values[api.attr("ParameterHandle")(encoded(parameter))] =
            py::bytes(value_bytes, value.size());
      }
      auto const* tag_bytes = static_cast<char const*>(user_supplied_tag.data());
      auto callback = callback_target_.attr("receiveInteraction");
      if (optional_sent_regions == nullptr) {
        callback(
            api.attr("InteractionClassHandle")(encoded(interaction_class)),
            api.attr("ParameterHandleValueMap")(values),
            py::bytes(tag_bytes, user_supplied_tag.size()),
            api.attr("TransportationTypeHandle")(encoded(transportation_type)),
            api.attr("FederateHandle")(encoded(producing_federate)));
      } else {
        callback(
            api.attr("InteractionClassHandle")(encoded(interaction_class)),
            api.attr("ParameterHandleValueMap")(values),
            py::bytes(tag_bytes, user_supplied_tag.size()),
            api.attr("TransportationTypeHandle")(encoded(transportation_type)),
            api.attr("FederateHandle")(encoded(producing_federate)),
            python_region_handle_set(*optional_sent_regions));
      }
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python receiveInteraction callback failed");
    }
  }

  void receiveInteraction(
      rti::InteractionClassHandle const& interaction_class,
      rti::ParameterHandleValueMap const& parameter_values,
      rti::VariableLengthData const& user_supplied_tag,
      rti::TransportationTypeHandle const& transportation_type,
      rti::FederateHandle const& producing_federate,
      rti::RegionHandleSet const* optional_sent_regions,
      rti::LogicalTime const& time,
      rti::OrderType sent_order_type,
      rti::OrderType received_order_type,
      rti::MessageRetractionHandle const* optional_retraction) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      py::dict values;
      for (auto const& [parameter, value] : parameter_values) {
        auto const* value_bytes = static_cast<char const*>(value.data());
        values[api.attr("ParameterHandle")(encoded(parameter))] =
            py::bytes(value_bytes, value.size());
      }
      auto const* tag_bytes = static_cast<char const*>(user_supplied_tag.data());
      auto callback = callback_target_.attr("receiveInteraction");
      py::object sent_regions = optional_sent_regions == nullptr
          ? py::none()
          : python_region_handle_set(*optional_sent_regions);
      py::tuple timed_arguments = py::make_tuple(
          api.attr("InteractionClassHandle")(encoded(interaction_class)),
          api.attr("ParameterHandleValueMap")(values),
          py::bytes(tag_bytes, user_supplied_tag.size()),
          api.attr("TransportationTypeHandle")(encoded(transportation_type)),
          api.attr("FederateHandle")(encoded(producing_federate)),
          sent_regions,
          python_logical_time(time),
          python_order_type(sent_order_type),
          python_order_type(received_order_type),
          python_retraction_handle(optional_retraction));
      try {
        callback(*timed_arguments);
      } catch (py::error_already_set const& error) {
        if (!error.matches(PyExc_TypeError)) throw;
        PyErr_Clear();
        if (optional_sent_regions == nullptr) {
          callback(
              api.attr("InteractionClassHandle")(encoded(interaction_class)),
              api.attr("ParameterHandleValueMap")(values),
              py::bytes(tag_bytes, user_supplied_tag.size()),
              api.attr("TransportationTypeHandle")(encoded(transportation_type)),
              api.attr("FederateHandle")(encoded(producing_federate)));
        } else {
          callback(
              api.attr("InteractionClassHandle")(encoded(interaction_class)),
              api.attr("ParameterHandleValueMap")(values),
              py::bytes(tag_bytes, user_supplied_tag.size()),
              api.attr("TransportationTypeHandle")(encoded(transportation_type)),
              api.attr("FederateHandle")(encoded(producing_federate)),
              sent_regions);
        }
      }
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python timed receiveInteraction callback failed");
    }
  }

  void receiveDirectedInteraction(
      rti::InteractionClassHandle const& interaction_class,
      rti::ObjectInstanceHandle const& object_instance,
      rti::ParameterHandleValueMap const& parameter_values,
      rti::VariableLengthData const& user_supplied_tag,
      rti::TransportationTypeHandle const& transportation_type,
      rti::FederateHandle const& producing_federate) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      py::dict values;
      for (auto const& [parameter, value] : parameter_values) {
        values[api.attr("ParameterHandle")(encoded(parameter))] = variable_bytes(value);
      }
      callback_target_.attr("receiveDirectedInteraction")(
          api.attr("InteractionClassHandle")(encoded(interaction_class)),
          api.attr("ObjectInstanceHandle")(encoded(object_instance)),
          api.attr("ParameterHandleValueMap")(values),
          variable_bytes(user_supplied_tag),
          api.attr("TransportationTypeHandle")(encoded(transportation_type)),
          api.attr("FederateHandle")(encoded(producing_federate)));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python receiveDirectedInteraction callback failed");
    }
  }

  void receiveDirectedInteraction(
      rti::InteractionClassHandle const& interaction_class,
      rti::ObjectInstanceHandle const& object_instance,
      rti::ParameterHandleValueMap const& parameter_values,
      rti::VariableLengthData const& user_supplied_tag,
      rti::TransportationTypeHandle const& transportation_type,
      rti::FederateHandle const& producing_federate,
      rti::LogicalTime const& time,
      rti::OrderType sent_order_type,
      rti::OrderType received_order_type,
      rti::MessageRetractionHandle const* optional_retraction) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      py::dict values;
      for (auto const& [parameter, value] : parameter_values) {
        values[api.attr("ParameterHandle")(encoded(parameter))] = variable_bytes(value);
      }
      auto callback = callback_target_.attr("receiveDirectedInteraction");
      py::tuple timed_arguments = py::make_tuple(
          api.attr("InteractionClassHandle")(encoded(interaction_class)),
          api.attr("ObjectInstanceHandle")(encoded(object_instance)),
          api.attr("ParameterHandleValueMap")(values),
          variable_bytes(user_supplied_tag),
          api.attr("TransportationTypeHandle")(encoded(transportation_type)),
          api.attr("FederateHandle")(encoded(producing_federate)),
          python_logical_time(time),
          python_order_type(sent_order_type),
          python_order_type(received_order_type),
          python_retraction_handle(optional_retraction));
      try {
        callback(*timed_arguments);
      } catch (py::error_already_set const& error) {
        if (!error.matches(PyExc_TypeError)) throw;
        PyErr_Clear();
        callback(
            api.attr("InteractionClassHandle")(encoded(interaction_class)),
            api.attr("ObjectInstanceHandle")(encoded(object_instance)),
            api.attr("ParameterHandleValueMap")(values),
            variable_bytes(user_supplied_tag),
            api.attr("TransportationTypeHandle")(encoded(transportation_type)),
            api.attr("FederateHandle")(encoded(producing_federate)));
      }
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python timed receiveDirectedInteraction callback failed");
    }
  }

  void timeRegulationEnabled(rti::LogicalTime const& time) override {
    py::gil_scoped_acquire acquire;
    try {
      callback_target_.attr("timeRegulationEnabled")(python_logical_time(time));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python timeRegulationEnabled callback failed");
    }
  }

  void timeConstrainedEnabled(rti::LogicalTime const& time) override {
    py::gil_scoped_acquire acquire;
    try {
      callback_target_.attr("timeConstrainedEnabled")(python_logical_time(time));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python timeConstrainedEnabled callback failed");
    }
  }

  void flushQueueGrant(
      rti::LogicalTime const& time,
      rti::LogicalTime const& optimistic_time) override {
    py::gil_scoped_acquire acquire;
    try {
      callback_target_.attr("flushQueueGrant")(
          python_logical_time(time), python_logical_time(optimistic_time));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python flushQueueGrant callback failed");
    }
  }

  void timeAdvanceGrant(rti::LogicalTime const& time) override {
    py::gil_scoped_acquire acquire;
    try {
      callback_target_.attr("timeAdvanceGrant")(python_logical_time(time));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python timeAdvanceGrant callback failed");
    }
  }

  void requestRetraction(rti::MessageRetractionHandle const& retraction) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("requestRetraction")(
          api.attr("MessageRetractionHandle")(encoded(retraction)));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python requestRetraction callback failed");
    }
  }

  void synchronizationPointRegistrationSucceeded(
      std::wstring const& synchronization_point_label) override {
    py::gil_scoped_acquire acquire;
    try {
      callback_target_.attr("synchronizationPointRegistrationSucceeded")(
          utf8(synchronization_point_label));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python synchronizationPointRegistrationSucceeded callback failed");
    }
  }

  void synchronizationPointRegistrationFailed(
      std::wstring const& synchronization_point_label,
      rti::SynchronizationPointFailureReason reason) override {
    py::gil_scoped_acquire acquire;
    try {
      std::string reason_name;
      switch (reason) {
        case rti::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE:
          reason_name = "SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE";
          break;
        case rti::SYNCHRONIZATION_SET_MEMBER_NOT_JOINED:
          reason_name = "SYNCHRONIZATION_SET_MEMBER_NOT_JOINED";
          break;
        default:
          throw std::logic_error("unknown SynchronizationPointFailureReason");
      }
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("synchronizationPointRegistrationFailed")(
          utf8(synchronization_point_label),
          api.attr("SynchronizationPointFailureReason").attr(reason_name.c_str()));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python synchronizationPointRegistrationFailed callback failed");
    }
  }

  void announceSynchronizationPoint(
      std::wstring const& synchronization_point_label,
      rti::VariableLengthData const& user_supplied_tag) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const* bytes = static_cast<char const*>(user_supplied_tag.data());
      callback_target_.attr("announceSynchronizationPoint")(
          utf8(synchronization_point_label),
          py::bytes(bytes, user_supplied_tag.size()));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python announceSynchronizationPoint callback failed");
    }
  }

  void federationSynchronized(
      std::wstring const& synchronization_point_label,
      rti::FederateHandleSet const& failed_to_sync_set) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      auto const handle_type = api.attr("FederateHandle");
      auto const handle_set_type = api.attr("FederateHandleSet");
      py::list handles;
      for (auto const& handle : failed_to_sync_set) {
        handles.append(handle_type(encoded(handle)));
      }
      callback_target_.attr("federationSynchronized")(
          utf8(synchronization_point_label), handle_set_type(handles));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python federationSynchronized callback failed");
    }
  }

  void federationSaveStatusResponse(
      rti::FederateHandleSaveStatusPairVector const& response) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      auto const handle_type = api.attr("FederateHandle");
      auto const pair_type = api.attr("FederateHandleSaveStatusPair");
      auto const status_type = api.attr("SaveStatus");
      py::list records;
      for (auto const& [handle, status] : response) {
        std::string status_name;
        switch (status) {
          case rti::NO_SAVE_IN_PROGRESS: status_name = "NO_SAVE_IN_PROGRESS"; break;
          case rti::FEDERATE_INSTRUCTED_TO_SAVE: status_name = "FEDERATE_INSTRUCTED_TO_SAVE"; break;
          case rti::FEDERATE_SAVING: status_name = "FEDERATE_SAVING"; break;
          case rti::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE:
            status_name = "FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE"; break;
          default: throw std::logic_error("unknown SaveStatus");
        }
        records.append(pair_type(handle_type(encoded(handle)), status_type.attr(status_name.c_str())));
      }
      callback_target_.attr("federationSaveStatusResponse")(py::tuple(records));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python federationSaveStatusResponse callback failed");
    }
  }

  void initiateFederateSave(std::wstring const& label) override {
    py::gil_scoped_acquire acquire;
    try {
      callback_target_.attr("initiateFederateSave")(utf8(label));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python initiateFederateSave callback failed");
    }
  }

  void initiateFederateSave(
      std::wstring const& label,
      rti::LogicalTime const& time) override {
    py::gil_scoped_acquire acquire;
    try {
      callback_target_.attr("initiateFederateSave")(utf8(label), python_logical_time(time));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(
          L"Python timestamped initiateFederateSave callback failed");
    }
  }

  void federationSaved() override {
    py::gil_scoped_acquire acquire;
    try {
      callback_target_.attr("federationSaved")();
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python federationSaved callback failed");
    }
  }

  void federationNotSaved(rti::SaveFailureReason reason) override {
    py::gil_scoped_acquire acquire;
    try {
      std::string name;
      switch (reason) {
        case rti::RTI_UNABLE_TO_SAVE: name = "RTI_UNABLE_TO_SAVE"; break;
        case rti::FEDERATE_REPORTED_FAILURE_DURING_SAVE:
          name = "FEDERATE_REPORTED_FAILURE_DURING_SAVE"; break;
        case rti::FEDERATE_RESIGNED_DURING_SAVE: name = "FEDERATE_RESIGNED_DURING_SAVE"; break;
        case rti::RTI_DETECTED_FAILURE_DURING_SAVE: name = "RTI_DETECTED_FAILURE_DURING_SAVE"; break;
        case rti::SAVE_TIME_CANNOT_BE_HONORED: name = "SAVE_TIME_CANNOT_BE_HONORED"; break;
        case rti::SAVE_ABORTED: name = "SAVE_ABORTED"; break;
        default: throw std::logic_error("unknown SaveFailureReason");
      }
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("federationNotSaved")(api.attr("SaveFailureReason").attr(name.c_str()));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python federationNotSaved callback failed");
    }
  }

  void federationRestoreStatusResponse(
      rti::FederateRestoreStatusVector const& response) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      auto const handle_type = api.attr("FederateHandle");
      auto const record_type = api.attr("FederateRestoreStatus");
      auto const status_type = api.attr("RestoreStatus");
      py::list records;
      for (auto const& item : response) {
        std::string name;
        switch (item.status) {
          case rti::NO_RESTORE_IN_PROGRESS: name = "NO_RESTORE_IN_PROGRESS"; break;
          case rti::FEDERATE_RESTORE_REQUEST_PENDING: name = "FEDERATE_RESTORE_REQUEST_PENDING"; break;
          case rti::FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN: name = "FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN"; break;
          case rti::FEDERATE_PREPARED_TO_RESTORE: name = "FEDERATE_PREPARED_TO_RESTORE"; break;
          case rti::FEDERATE_RESTORING: name = "FEDERATE_RESTORING"; break;
          case rti::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE:
            name = "FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE"; break;
          default: throw std::logic_error("unknown RestoreStatus");
        }
        records.append(record_type(
            handle_type(encoded(item.preRestoreHandle)),
            handle_type(encoded(item.postRestoreHandle)),
            status_type.attr(name.c_str())));
      }
      callback_target_.attr("federationRestoreStatusResponse")(py::tuple(records));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python federationRestoreStatusResponse callback failed");
    }
  }

  void requestFederationRestoreSucceeded(std::wstring const& label) override {
    py::gil_scoped_acquire acquire;
    try { callback_target_.attr("requestFederationRestoreSucceeded")(utf8(label)); }
    catch (py::error_already_set const&) { throw rti::FederateInternalError(L"Python requestFederationRestoreSucceeded callback failed"); }
  }

  void requestFederationRestoreFailed(std::wstring const& label) override {
    py::gil_scoped_acquire acquire;
    try { callback_target_.attr("requestFederationRestoreFailed")(utf8(label)); }
    catch (py::error_already_set const&) { throw rti::FederateInternalError(L"Python requestFederationRestoreFailed callback failed"); }
  }

  void federationRestoreBegun() override {
    py::gil_scoped_acquire acquire;
    try { callback_target_.attr("federationRestoreBegun")(); }
    catch (py::error_already_set const&) { throw rti::FederateInternalError(L"Python federationRestoreBegun callback failed"); }
  }

  void initiateFederateRestore(
      std::wstring const& label,
      std::wstring const& federate_name,
      rti::FederateHandle const& post_restore_handle) override {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("initiateFederateRestore")(
          utf8(label), utf8(federate_name), api.attr("FederateHandle")(encoded(post_restore_handle)));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python initiateFederateRestore callback failed");
    }
  }

  void federationRestored() override {
    py::gil_scoped_acquire acquire;
    try { callback_target_.attr("federationRestored")(); }
    catch (py::error_already_set const&) { throw rti::FederateInternalError(L"Python federationRestored callback failed"); }
  }

  void federationNotRestored(rti::RestoreFailureReason reason) override {
    py::gil_scoped_acquire acquire;
    try {
      std::string name;
      switch (reason) {
        case rti::RTI_UNABLE_TO_RESTORE: name = "RTI_UNABLE_TO_RESTORE"; break;
        case rti::FEDERATE_REPORTED_FAILURE_DURING_RESTORE:
          name = "FEDERATE_REPORTED_FAILURE_DURING_RESTORE"; break;
        case rti::FEDERATE_RESIGNED_DURING_RESTORE:
          name = "FEDERATE_RESIGNED_DURING_RESTORE"; break;
        case rti::RTI_DETECTED_FAILURE_DURING_RESTORE:
          name = "RTI_DETECTED_FAILURE_DURING_RESTORE"; break;
        case rti::RESTORE_ABORTED: name = "RESTORE_ABORTED"; break;
        default: throw std::logic_error("unknown RestoreFailureReason");
      }
      auto const api = py::module_::import("hla.rti1516_2025");
      callback_target_.attr("federationNotRestored")(api.attr("RestoreFailureReason").attr(name.c_str()));
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python federationNotRestored callback failed");
    }
  }

  void requestAttributeOwnershipAssumption(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& offered_attributes,
      rti::VariableLengthData const& user_supplied_tag) override {
    dispatch_ownership_set_callback(
        "requestAttributeOwnershipAssumption",
        object_instance,
        offered_attributes,
        &user_supplied_tag);
  }

  void requestDivestitureConfirmation(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& released_attributes,
      rti::VariableLengthData const& user_supplied_tag) override {
    dispatch_ownership_set_callback(
        "requestDivestitureConfirmation",
        object_instance,
        released_attributes,
        &user_supplied_tag);
  }

  void attributeOwnershipAcquisitionNotification(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& secured_attributes,
      rti::VariableLengthData const& user_supplied_tag) override {
    dispatch_ownership_set_callback(
        "attributeOwnershipAcquisitionNotification",
        object_instance,
        secured_attributes,
        &user_supplied_tag);
  }

  void attributeOwnershipUnavailable(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const& user_supplied_tag) override {
    dispatch_ownership_set_callback(
        "attributeOwnershipUnavailable", object_instance, attributes, &user_supplied_tag);
  }

  void requestAttributeOwnershipRelease(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& candidate_attributes,
      rti::VariableLengthData const& user_supplied_tag) override {
    dispatch_ownership_set_callback(
        "requestAttributeOwnershipRelease",
        object_instance,
        candidate_attributes,
        &user_supplied_tag);
  }

  void confirmAttributeOwnershipAcquisitionCancellation(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& attributes) override {
    dispatch_ownership_set_callback(
        "confirmAttributeOwnershipAcquisitionCancellation",
        object_instance,
        attributes,
        nullptr);
  }

  void informAttributeOwnership(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& attributes,
      rti::FederateHandle const& owner) override {
    dispatch_ownership_set_callback(
        "informAttributeOwnership", object_instance, attributes, nullptr, &owner);
  }

  void attributeIsNotOwned(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& attributes) override {
    dispatch_ownership_set_callback(
        "attributeIsNotOwned", object_instance, attributes, nullptr);
  }

  void attributeIsOwnedByRTI(
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& attributes) override {
    dispatch_ownership_set_callback(
        "attributeIsOwnedByRTI", object_instance, attributes, nullptr);
  }

 private:
  void dispatch_ownership_set_callback(
      char const* callback_name,
      rti::ObjectInstanceHandle const& object_instance,
      rti::AttributeHandleSet const& attributes,
      rti::VariableLengthData const* optional_tag,
      rti::FederateHandle const* optional_owner = nullptr) {
    py::gil_scoped_acquire acquire;
    try {
      auto const api = py::module_::import("hla.rti1516_2025");
      py::list values;
      for (auto const& attribute : attributes) {
        values.append(api.attr("AttributeHandle")(encoded(attribute)));
      }
      auto callback = callback_target_.attr(callback_name);
      if (optional_owner != nullptr) {
        callback(
            api.attr("ObjectInstanceHandle")(encoded(object_instance)),
            api.attr("AttributeHandleSet")(values),
            api.attr("FederateHandle")(encoded(*optional_owner)));
      } else if (optional_tag != nullptr) {
        auto const* tag_bytes = static_cast<char const*>(optional_tag->data());
        callback(
            api.attr("ObjectInstanceHandle")(encoded(object_instance)),
            api.attr("AttributeHandleSet")(values),
            py::bytes(tag_bytes, optional_tag->size()));
      } else {
        callback(
            api.attr("ObjectInstanceHandle")(encoded(object_instance)),
            api.attr("AttributeHandleSet")(values));
      }
    } catch (py::error_already_set const&) {
      throw rti::FederateInternalError(L"Python ownership callback failed");
    }
  }

  py::object callback_target_;
};

rti::CallbackModel callback_model_from(std::string const& value) {
  if (value == "immediate") {
    return rti::HLA_IMMEDIATE;
  }
  if (value == "evoked") {
    return rti::HLA_EVOKED;
  }
  throw rti::UnsupportedCallbackModel(L"callback model must be 'immediate' or 'evoked'");
}

std::string additional_settings_result_from(rti::AdditionalSettingsResultCode code) {
  switch (code) {
    case rti::SETTINGS_IGNORED:
      return "ignored";
    case rti::SETTINGS_FAILED_TO_PARSE:
      return "failed_to_parse";
    case rti::SETTINGS_APPLIED:
      return "applied";
  }
  throw std::logic_error("unknown AdditionalSettingsResultCode");
}

std::string synchronization_point_failure_reason_from(
    rti::SynchronizationPointFailureReason reason) {
  switch (reason) {
    case rti::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE:
      return "SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE";
    case rti::SYNCHRONIZATION_SET_MEMBER_NOT_JOINED:
      return "SYNCHRONIZATION_SET_MEMBER_NOT_JOINED";
  }
  throw std::logic_error("unknown SynchronizationPointFailureReason");
}

struct NativeConfigurationResult {
  bool configuration_used;
  bool address_used;
  std::string additional_settings_result;
  std::string message;
};

class NativeAmbassador {
 public:
  NativeAmbassador() : ambassador_(rti::RTIambassadorFactory().createRTIambassador()) {}

  NativeConfigurationResult connect(
      py::object callback_target,
      std::string const& callback_model,
      std::string const& configuration_name,
      std::string const& rti_address,
      std::string const& additional_settings,
      bool has_configuration,
      bool has_no_credentials) {
    auto callback = std::make_unique<PythonFederateAmbassador>(std::move(callback_target));
    auto const model = callback_model_from(callback_model);
    rti::ConfigurationResult result;
    if (has_configuration) {
      auto configuration = rti::RtiConfiguration::createConfiguration()
                               .withConfigurationName(wide(configuration_name))
                               .withRtiAddress(wide(rti_address))
                               .withAdditionalSettings(wide(additional_settings));
      if (has_no_credentials) {
        result = ambassador_->connect(*callback, model, configuration, rti::HLAnoCredentials());
      } else {
        result = ambassador_->connect(*callback, model, configuration);
      }
    } else if (has_no_credentials) {
      result = ambassador_->connect(*callback, model, rti::HLAnoCredentials());
    } else {
      result = ambassador_->connect(*callback, model);
    }
    federate_ambassador_ = std::move(callback);
    return {
        result.configurationUsed,
        result.addressUsed,
        additional_settings_result_from(result.additionalSettingsResult),
        utf8(result.message),
    };
  }

  void disconnect() {
    ambassador_->disconnect();
    federate_ambassador_.reset();
  }

  bool evoke_callback(double approximate_minimum_time_seconds) {
    return ambassador_->evokeCallback(approximate_minimum_time_seconds);
  }

  bool evoke_multiple_callbacks(
      double approximate_minimum_time_seconds,
      double approximate_maximum_time_seconds) {
    return ambassador_->evokeMultipleCallbacks(
        approximate_minimum_time_seconds, approximate_maximum_time_seconds);
  }

  void enable_callbacks() { ambassador_->enableCallbacks(); }
  void disable_callbacks() { ambassador_->disableCallbacks(); }
  void list_federation_executions() { ambassador_->listFederationExecutions(); }
  void list_federation_execution_members(std::string const& federation_name) {
    ambassador_->listFederationExecutionMembers(wide(federation_name));
  }
  py::bytes join_federation_execution(
      std::string const& federate_type,
      std::string const& federation_name,
      std::string const& federate_name,
      bool has_federate_name) {
    auto const type = wide(federate_type);
    auto const federation = wide(federation_name);
    return has_federate_name
        ? encoded(ambassador_->joinFederationExecution(wide(federate_name), type, federation))
        : encoded(ambassador_->joinFederationExecution(type, federation));
  }
  py::bytes join_federation_execution_with_modules(
      std::string const& federate_type,
      std::string const& federation_name,
      std::string const& federate_name,
      bool has_federate_name,
      py::iterable const& additional_fom_modules) {
    auto const modules = wide_strings_from(additional_fom_modules);
    auto const type = wide(federate_type);
    auto const federation = wide(federation_name);
    return has_federate_name
        ? encoded(ambassador_->joinFederationExecution(
              wide(federate_name), type, federation, modules))
        : encoded(ambassador_->joinFederationExecution(type, federation, modules));
  }
  void resign_federation_execution(std::string const& resign_action) {
    ambassador_->resignFederationExecution(resign_action_from(resign_action));
  }
  void register_federation_synchronization_point(std::string const& label, py::bytes const& tag) {
    std::string const copied_tag = tag;
    rti::VariableLengthData const data(copied_tag.data(), copied_tag.size());
    ambassador_->registerFederationSynchronizationPoint(wide(label), data);
  }
  void register_federation_synchronization_point_with_set(
      std::string const& label,
      py::bytes const& tag,
      py::iterable const& synchronization_set) {
    std::string const copied_tag = tag;
    rti::VariableLengthData const data(copied_tag.data(), copied_tag.size());
    ambassador_->registerFederationSynchronizationPoint(
        wide(label), data, federate_handles_from(synchronization_set));
  }
  void synchronization_point_achieved(std::string const& label, bool successfully) {
    ambassador_->synchronizationPointAchieved(wide(label), successfully);
  }
  void query_federation_save_status() { ambassador_->queryFederationSaveStatus(); }
  void request_federation_save(std::string const& label) {
    ambassador_->requestFederationSave(wide(label));
  }
  void request_federation_save_with_time(
      std::string const& label,
      py::bytes const& time) {
    ambassador_->requestFederationSave(
        wide(label),
        *ambassador_->getTimeFactory()->decodeLogicalTime(variable_length_data(time)));
  }
  void federate_save_begun() { ambassador_->federateSaveBegun(); }
  void federate_save_complete() { ambassador_->federateSaveComplete(); }
  void federate_save_not_complete() { ambassador_->federateSaveNotComplete(); }
  void abort_federation_save() { ambassador_->abortFederationSave(); }
  void query_federation_restore_status() { ambassador_->queryFederationRestoreStatus(); }
  void request_federation_restore(std::string const& label) {
    ambassador_->requestFederationRestore(wide(label));
  }
  void federate_restore_complete() { ambassador_->federateRestoreComplete(); }
  void federate_restore_not_complete() { ambassador_->federateRestoreNotComplete(); }
  void abort_federation_restore() { ambassador_->abortFederationRestore(); }
  void publish_object_class_attributes(
      py::bytes const& object_class, py::iterable const& attributes) {
    ambassador_->publishObjectClassAttributes(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class)),
        attribute_handles_from(attributes));
  }
  void unpublish_object_class(py::bytes const& object_class) {
    ambassador_->unpublishObjectClass(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class)));
  }
  void unpublish_object_class_attributes(
      py::bytes const& object_class, py::iterable const& attributes) {
    ambassador_->unpublishObjectClassAttributes(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class)),
        attribute_handles_from(attributes));
  }
  void publish_object_class_directed_interactions(
      py::bytes const& object_class,
      py::iterable const& interaction_classes) {
    ambassador_->publishObjectClassDirectedInteractions(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class)),
        interaction_class_handles_from(interaction_classes));
  }
  void unpublish_object_class_directed_interactions(
      py::bytes const& object_class,
      py::object const& interaction_classes) {
    auto const decoded_object_class = rti::umbra_binding_detail::decodeObjectClassHandle(
        variable_length_data(object_class));
    if (interaction_classes.is_none()) {
      ambassador_->unpublishObjectClassDirectedInteractions(decoded_object_class);
    } else {
      ambassador_->unpublishObjectClassDirectedInteractions(
          decoded_object_class,
          interaction_class_handles_from(py::cast<py::iterable>(interaction_classes)));
    }
  }
  void subscribe_object_class_attributes(
      py::bytes const& object_class,
      py::iterable const& attributes,
      bool active,
      std::string const& update_rate_designator) {
    ambassador_->subscribeObjectClassAttributes(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class)),
        attribute_handles_from(attributes),
        active,
        wide(update_rate_designator));
  }
  void unsubscribe_object_class(py::bytes const& object_class) {
    ambassador_->unsubscribeObjectClass(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class)));
  }
  void unsubscribe_object_class_attributes(
      py::bytes const& object_class, py::iterable const& attributes) {
    ambassador_->unsubscribeObjectClassAttributes(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class)),
        attribute_handles_from(attributes));
  }
  void subscribe_object_class_directed_interactions(
      py::bytes const& object_class,
      py::iterable const& interaction_classes,
      bool universally) {
    ambassador_->subscribeObjectClassDirectedInteractions(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class)),
        interaction_class_handles_from(interaction_classes),
        universally);
  }
  void unsubscribe_object_class_directed_interactions(
      py::bytes const& object_class,
      py::object const& interaction_classes) {
    auto const decoded_object_class = rti::umbra_binding_detail::decodeObjectClassHandle(
        variable_length_data(object_class));
    if (interaction_classes.is_none()) {
      ambassador_->unsubscribeObjectClassDirectedInteractions(decoded_object_class);
    } else {
      ambassador_->unsubscribeObjectClassDirectedInteractions(
          decoded_object_class,
          interaction_class_handles_from(py::cast<py::iterable>(interaction_classes)));
    }
  }
  void subscribe_object_class_attributes_with_regions(
      py::bytes const& object_class,
      py::iterable const& attributes_and_regions,
      bool active,
      std::string const& update_rate_designator) {
    ambassador_->subscribeObjectClassAttributesWithRegions(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class)),
        attribute_region_pairs_from(attributes_and_regions),
        active,
        wide(update_rate_designator));
  }
  void unsubscribe_object_class_attributes_with_regions(
      py::bytes const& object_class,
      py::iterable const& attributes_and_regions) {
    ambassador_->unsubscribeObjectClassAttributesWithRegions(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class)),
        attribute_region_pairs_from(attributes_and_regions));
  }
  void publish_interaction_class(py::bytes const& interaction_class) {
    ambassador_->publishInteractionClass(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)));
  }
  void unpublish_interaction_class(py::bytes const& interaction_class) {
    ambassador_->unpublishInteractionClass(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)));
  }
  void subscribe_interaction_class(py::bytes const& interaction_class, bool active) {
    ambassador_->subscribeInteractionClass(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)),
        active);
  }
  void unsubscribe_interaction_class(py::bytes const& interaction_class) {
    ambassador_->unsubscribeInteractionClass(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)));
  }
  void reserve_object_instance_name(std::string const& object_instance_name) {
    ambassador_->reserveObjectInstanceName(wide(object_instance_name));
  }
  void release_object_instance_name(std::string const& object_instance_name) {
    ambassador_->releaseObjectInstanceName(wide(object_instance_name));
  }
  void reserve_multiple_object_instance_names(py::iterable const& object_instance_names) {
    std::set<std::wstring> names;
    for (py::handle const item : object_instance_names) {
      names.insert(wide(py::cast<std::string>(item)));
    }
    ambassador_->reserveMultipleObjectInstanceNames(names);
  }
  void release_multiple_object_instance_names(py::iterable const& object_instance_names) {
    std::set<std::wstring> names;
    for (py::handle const item : object_instance_names) {
      names.insert(wide(py::cast<std::string>(item)));
    }
    ambassador_->releaseMultipleObjectInstanceNames(names);
  }
  py::bytes register_object_instance(
      py::bytes const& object_class,
      std::string const& object_instance_name,
      bool has_object_instance_name) {
    auto const decoded_object_class = rti::umbra_binding_detail::decodeObjectClassHandle(
        variable_length_data(object_class));
    return has_object_instance_name
        ? encoded(ambassador_->registerObjectInstance(
            decoded_object_class, wide(object_instance_name)))
        : encoded(ambassador_->registerObjectInstance(decoded_object_class));
  }
  py::bytes register_object_instance_with_regions(
      py::bytes const& object_class,
      py::iterable const& attributes_and_regions,
      std::string const& object_instance_name,
      bool has_object_instance_name) {
    auto const decoded_object_class = rti::umbra_binding_detail::decodeObjectClassHandle(
        variable_length_data(object_class));
    auto const decoded_pairs = attribute_region_pairs_from(attributes_and_regions);
    return has_object_instance_name
        ? encoded(ambassador_->registerObjectInstanceWithRegions(
              decoded_object_class, decoded_pairs, wide(object_instance_name)))
        : encoded(ambassador_->registerObjectInstanceWithRegions(
              decoded_object_class, decoded_pairs));
  }
  void associate_regions_for_updates(
      py::bytes const& object_instance,
      py::iterable const& attributes_and_regions) {
    ambassador_->associateRegionsForUpdates(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_region_pairs_from(attributes_and_regions));
  }
  void unassociate_regions_for_updates(
      py::bytes const& object_instance,
      py::iterable const& attributes_and_regions) {
    ambassador_->unassociateRegionsForUpdates(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_region_pairs_from(attributes_and_regions));
  }
  py::bytes get_object_instance_handle(std::string const& object_instance_name) {
    return encoded(ambassador_->getObjectInstanceHandle(wide(object_instance_name)));
  }
  std::string get_object_instance_name(py::bytes const& object_instance) {
    return utf8(ambassador_->getObjectInstanceName(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance))));
  }
  void delete_object_instance(
      py::bytes const& object_instance,
      py::bytes const& user_supplied_tag) {
    ambassador_->deleteObjectInstance(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        variable_length_data(user_supplied_tag));
  }
  void local_delete_object_instance(py::bytes const& object_instance) {
    ambassador_->localDeleteObjectInstance(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)));
  }
  py::bytes delete_object_instance_with_time(
      py::bytes const& object_instance,
      py::bytes const& user_supplied_tag,
      py::bytes const& time) {
    auto logical_time = ambassador_->getTimeFactory()->decodeLogicalTime(
        variable_length_data(time));
    return encoded(ambassador_->deleteObjectInstance(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        variable_length_data(user_supplied_tag),
        *logical_time));
  }
  void update_attribute_values(
      py::bytes const& object_instance,
      py::iterable const& attribute_values,
      py::bytes const& user_supplied_tag) {
    ambassador_->updateAttributeValues(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_values_from(attribute_values),
        variable_length_data(user_supplied_tag));
  }
  py::bytes update_attribute_values_with_time(
      py::bytes const& object_instance,
      py::iterable const& attribute_values,
      py::bytes const& user_supplied_tag,
      py::bytes const& time) {
    auto logical_time = ambassador_->getTimeFactory()->decodeLogicalTime(
        variable_length_data(time));
    return encoded(ambassador_->updateAttributeValues(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_values_from(attribute_values),
        variable_length_data(user_supplied_tag),
        *logical_time));
  }
  void request_attribute_value_update_with_regions(
      py::bytes const& object_class,
      py::iterable const& attributes_and_regions,
      py::bytes const& user_supplied_tag) {
    ambassador_->requestAttributeValueUpdateWithRegions(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class)),
        attribute_region_pairs_from(attributes_and_regions),
        variable_length_data(user_supplied_tag));
  }
  void request_attribute_value_update_for_instance(
      py::bytes const& object_instance,
      py::iterable const& attributes,
      py::bytes const& user_supplied_tag) {
    ambassador_->requestAttributeValueUpdate(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_handles_from(attributes),
        variable_length_data(user_supplied_tag));
  }
  void request_attribute_value_update_for_class(
      py::bytes const& object_class,
      py::iterable const& attributes,
      py::bytes const& user_supplied_tag) {
    ambassador_->requestAttributeValueUpdate(
        rti::umbra_binding_detail::decodeObjectClassHandle(
            variable_length_data(object_class)),
        attribute_handles_from(attributes),
        variable_length_data(user_supplied_tag));
  }
  void change_attribute_order_type(
      py::bytes const& object_instance,
      py::iterable const& attributes,
      std::string const& order_type) {
    ambassador_->changeAttributeOrderType(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_handles_from(attributes),
        order_type_from(order_type));
  }
  void change_default_attribute_order_type(
      py::bytes const& object_class,
      py::iterable const& attributes,
      std::string const& order_type) {
    ambassador_->changeDefaultAttributeOrderType(
        rti::umbra_binding_detail::decodeObjectClassHandle(
            variable_length_data(object_class)),
        attribute_handles_from(attributes),
        order_type_from(order_type));
  }
  void change_interaction_order_type(
      py::bytes const& interaction_class,
      std::string const& order_type) {
    ambassador_->changeInteractionOrderType(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)),
        order_type_from(order_type));
  }
  void request_attribute_transportation_type_change(
      py::bytes const& object_instance,
      py::iterable const& attributes,
      py::bytes const& transportation_type) {
    ambassador_->requestAttributeTransportationTypeChange(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_handles_from(attributes),
        rti::umbra_binding_detail::decodeTransportationTypeHandle(
            variable_length_data(transportation_type)));
  }
  void change_default_attribute_transportation_type(
      py::bytes const& object_class,
      py::iterable const& attributes,
      py::bytes const& transportation_type) {
    ambassador_->changeDefaultAttributeTransportationType(
        rti::umbra_binding_detail::decodeObjectClassHandle(
            variable_length_data(object_class)),
        attribute_handles_from(attributes),
        rti::umbra_binding_detail::decodeTransportationTypeHandle(
            variable_length_data(transportation_type)));
  }
  void query_attribute_transportation_type(
      py::bytes const& object_instance,
      py::bytes const& attribute) {
    ambassador_->queryAttributeTransportationType(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        rti::umbra_binding_detail::decodeAttributeHandle(variable_length_data(attribute)));
  }
  void request_interaction_transportation_type_change(
      py::bytes const& interaction_class,
      py::bytes const& transportation_type) {
    ambassador_->requestInteractionTransportationTypeChange(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)),
        rti::umbra_binding_detail::decodeTransportationTypeHandle(
            variable_length_data(transportation_type)));
  }
  void query_interaction_transportation_type(
      py::bytes const& federate,
      py::bytes const& interaction_class) {
    ambassador_->queryInteractionTransportationType(
        rti::umbra_binding_detail::decodeFederateHandle(variable_length_data(federate)),
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)));
  }
  void query_attribute_ownership(
      py::bytes const& object_instance,
      py::iterable const& attributes) {
    ambassador_->queryAttributeOwnership(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_handles_from(attributes));
  }
  bool is_attribute_owned_by_federate(
      py::bytes const& object_instance,
      py::bytes const& attribute) {
    return ambassador_->isAttributeOwnedByFederate(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        rti::umbra_binding_detail::decodeAttributeHandle(variable_length_data(attribute)));
  }
  void unconditional_attribute_ownership_divestiture(
      py::bytes const& object_instance,
      py::iterable const& attributes,
      py::bytes const& user_supplied_tag) {
    ambassador_->unconditionalAttributeOwnershipDivestiture(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_handles_from(attributes),
        variable_length_data(user_supplied_tag));
  }
  void negotiated_attribute_ownership_divestiture(
      py::bytes const& object_instance,
      py::iterable const& attributes,
      py::bytes const& user_supplied_tag) {
    ambassador_->negotiatedAttributeOwnershipDivestiture(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_handles_from(attributes),
        variable_length_data(user_supplied_tag));
  }
  void confirm_divestiture(
      py::bytes const& object_instance,
      py::iterable const& attributes,
      py::bytes const& user_supplied_tag) {
    ambassador_->confirmDivestiture(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_handles_from(attributes),
        variable_length_data(user_supplied_tag));
  }
  void cancel_negotiated_attribute_ownership_divestiture(
      py::bytes const& object_instance,
      py::iterable const& attributes) {
    ambassador_->cancelNegotiatedAttributeOwnershipDivestiture(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_handles_from(attributes));
  }
  void attribute_ownership_acquisition(
      py::bytes const& object_instance,
      py::iterable const& attributes,
      py::bytes const& user_supplied_tag) {
    ambassador_->attributeOwnershipAcquisition(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_handles_from(attributes),
        variable_length_data(user_supplied_tag));
  }
  void attribute_ownership_acquisition_if_available(
      py::bytes const& object_instance,
      py::iterable const& attributes,
      py::bytes const& user_supplied_tag) {
    ambassador_->attributeOwnershipAcquisitionIfAvailable(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_handles_from(attributes),
        variable_length_data(user_supplied_tag));
  }
  void cancel_attribute_ownership_acquisition(
      py::bytes const& object_instance,
      py::iterable const& attributes) {
    ambassador_->cancelAttributeOwnershipAcquisition(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_handles_from(attributes));
  }
  void attribute_ownership_release_denied(
      py::bytes const& object_instance,
      py::iterable const& attributes,
      py::bytes const& user_supplied_tag) {
    ambassador_->attributeOwnershipReleaseDenied(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_handles_from(attributes),
        variable_length_data(user_supplied_tag));
  }
  py::object attribute_ownership_divestiture_if_wanted(
      py::bytes const& object_instance,
      py::iterable const& attributes,
      py::bytes const& user_supplied_tag) {
    rti::AttributeHandleSet divested_attributes;
    ambassador_->attributeOwnershipDivestitureIfWanted(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        attribute_handles_from(attributes),
        variable_length_data(user_supplied_tag),
        divested_attributes);
    return python_attribute_handle_set(divested_attributes);
  }
  void send_interaction(
      py::bytes const& interaction_class,
      py::iterable const& parameter_values,
      py::bytes const& user_supplied_tag) {
    ambassador_->sendInteraction(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)),
        parameter_values_from(parameter_values),
        variable_length_data(user_supplied_tag));
  }
  py::bytes send_interaction_with_time(
      py::bytes const& interaction_class,
      py::iterable const& parameter_values,
      py::bytes const& user_supplied_tag,
      py::bytes const& time) {
    auto logical_time = ambassador_->getTimeFactory()->decodeLogicalTime(
        variable_length_data(time));
    return encoded(ambassador_->sendInteraction(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)),
        parameter_values_from(parameter_values),
        variable_length_data(user_supplied_tag),
        *logical_time));
  }
  void send_directed_interaction(
      py::bytes const& interaction_class,
      py::bytes const& object_instance,
      py::iterable const& parameter_values,
      py::bytes const& user_supplied_tag) {
    ambassador_->sendDirectedInteraction(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)),
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        parameter_values_from(parameter_values),
        variable_length_data(user_supplied_tag));
  }
  py::bytes send_directed_interaction_with_time(
      py::bytes const& interaction_class,
      py::bytes const& object_instance,
      py::iterable const& parameter_values,
      py::bytes const& user_supplied_tag,
      py::bytes const& time) {
    auto logical_time = ambassador_->getTimeFactory()->decodeLogicalTime(
        variable_length_data(time));
    return encoded(ambassador_->sendDirectedInteraction(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)),
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        parameter_values_from(parameter_values),
        variable_length_data(user_supplied_tag),
        *logical_time));
  }
  void subscribe_interaction_class_with_regions(
      py::bytes const& interaction_class,
      py::iterable const& regions,
      bool active) {
    ambassador_->subscribeInteractionClassWithRegions(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)),
        region_handles_from(regions),
        active);
  }
  void unsubscribe_interaction_class_with_regions(
      py::bytes const& interaction_class,
      py::iterable const& regions) {
    ambassador_->unsubscribeInteractionClassWithRegions(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)),
        region_handles_from(regions));
  }
  void send_interaction_with_regions(
      py::bytes const& interaction_class,
      py::iterable const& parameter_values,
      py::iterable const& regions,
      py::bytes const& user_supplied_tag) {
    ambassador_->sendInteractionWithRegions(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)),
        parameter_values_from(parameter_values),
        region_handles_from(regions),
        variable_length_data(user_supplied_tag));
  }
  py::bytes send_interaction_with_regions_with_time(
      py::bytes const& interaction_class,
      py::iterable const& parameter_values,
      py::iterable const& regions,
      py::bytes const& user_supplied_tag,
      py::bytes const& time) {
    auto logical_time = ambassador_->getTimeFactory()->decodeLogicalTime(
        variable_length_data(time));
    return encoded(ambassador_->sendInteractionWithRegions(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)),
        parameter_values_from(parameter_values),
        region_handles_from(regions),
        variable_length_data(user_supplied_tag),
        *logical_time));
  }
  void retract(py::bytes const& retraction) {
    ambassador_->retract(rti::umbra_binding_detail::decodeMessageRetractionHandle(
        variable_length_data(retraction)));
  }
  py::bytes decode_message_retraction_handle(py::bytes const& encoded_value) const {
    return encoded(ambassador_->decodeMessageRetractionHandle(
        variable_length_data(encoded_value)));
  }
  py::bytes create_region(py::iterable const& dimensions) {
    return encoded(ambassador_->createRegion(dimension_handles_from(dimensions)));
  }
  void commit_region_modifications(py::iterable const& regions) {
    ambassador_->commitRegionModifications(region_handles_from(regions));
  }
  void delete_region(py::bytes const& region) {
    ambassador_->deleteRegion(
        rti::umbra_binding_detail::decodeRegionHandle(variable_length_data(region)));
  }
  py::list get_dimension_handle_set(py::bytes const& region) {
    py::list result;
    for (auto const& dimension : ambassador_->getDimensionHandleSet(
             rti::umbra_binding_detail::decodeRegionHandle(variable_length_data(region)))) {
      result.append(encoded(dimension));
    }
    return result;
  }
  py::dict get_range_bounds(
      py::bytes const& region,
      py::bytes const& dimension) {
    auto const bounds = ambassador_->getRangeBounds(
        rti::umbra_binding_detail::decodeRegionHandle(variable_length_data(region)),
        rti::umbra_binding_detail::decodeDimensionHandle(variable_length_data(dimension)));
    py::dict result;
    result["lowerBound"] = bounds.getLowerBound();
    result["upperBound"] = bounds.getUpperBound();
    return result;
  }
  void set_range_bounds(
      py::bytes const& region,
      py::bytes const& dimension,
      unsigned long lower_bound,
      unsigned long upper_bound) {
    ambassador_->setRangeBounds(
        rti::umbra_binding_detail::decodeRegionHandle(variable_length_data(region)),
        rti::umbra_binding_detail::decodeDimensionHandle(variable_length_data(dimension)),
        rti::RangeBounds(lower_bound, upper_bound));
  }
  bool get_convey_region_designator_sets_switch() const {
    return ambassador_->getConveyRegionDesignatorSetsSwitch();
  }
  void set_convey_region_designator_sets_switch(bool switch_value) {
    ambassador_->setConveyRegionDesignatorSetsSwitch(switch_value);
  }
  bool get_object_class_relevance_advisory_switch() const {
    return ambassador_->getObjectClassRelevanceAdvisorySwitch();
  }
  void set_object_class_relevance_advisory_switch(bool switch_value) {
    ambassador_->setObjectClassRelevanceAdvisorySwitch(switch_value);
  }
  bool get_attribute_relevance_advisory_switch() const {
    return ambassador_->getAttributeRelevanceAdvisorySwitch();
  }
  void set_attribute_relevance_advisory_switch(bool switch_value) {
    ambassador_->setAttributeRelevanceAdvisorySwitch(switch_value);
  }
  bool get_attribute_scope_advisory_switch() const {
    return ambassador_->getAttributeScopeAdvisorySwitch();
  }
  void set_attribute_scope_advisory_switch(bool switch_value) {
    ambassador_->setAttributeScopeAdvisorySwitch(switch_value);
  }
  bool get_interaction_relevance_advisory_switch() const {
    return ambassador_->getInteractionRelevanceAdvisorySwitch();
  }
  void set_interaction_relevance_advisory_switch(bool switch_value) {
    ambassador_->setInteractionRelevanceAdvisorySwitch(switch_value);
  }
  std::string get_automatic_resign_directive() const {
    return resign_action_name(ambassador_->getAutomaticResignDirective());
  }
  void set_automatic_resign_directive(std::string const& value) {
    ambassador_->setAutomaticResignDirective(resign_action_from(value));
  }
  bool get_service_reporting_switch() const {
    return ambassador_->getServiceReportingSwitch();
  }
  void set_service_reporting_switch(bool switch_value) {
    ambassador_->setServiceReportingSwitch(switch_value);
  }
  bool get_exception_reporting_switch() const {
    return ambassador_->getExceptionReportingSwitch();
  }
  void set_exception_reporting_switch(bool switch_value) {
    ambassador_->setExceptionReportingSwitch(switch_value);
  }
  bool get_send_service_reports_to_file_switch() const {
    return ambassador_->getSendServiceReportsToFileSwitch();
  }
  void set_send_service_reports_to_file_switch(bool enabled) {
    ambassador_->setSendServiceReportsToFileSwitch(enabled);
  }
  std::string hla_version() const {
    return "IEEE 1516.1-2025";
  }
  bool get_auto_provide_switch() const {
    return ambassador_->getAutoProvideSwitch();
  }
  bool get_delay_subscription_evaluation_switch() const {
    return ambassador_->getDelaySubscriptionEvaluationSwitch();
  }
  bool get_advisories_use_known_class_switch() const {
    return ambassador_->getAdvisoriesUseKnownClassSwitch();
  }
  bool get_allow_relaxed_ddm_switch() const {
    return ambassador_->getAllowRelaxedDDMSwitch();
  }
  bool get_non_regulated_grant_switch() const {
    return ambassador_->getNonRegulatedGrantSwitch();
  }
  std::string time_factory_name() const {
    return utf8(ambassador_->getTimeFactory()->getName());
  }
  py::dict make_initial_time() const {
    return logical_time_snapshot(*ambassador_->getTimeFactory()->makeInitial());
  }
  py::dict make_final_time() const {
    return logical_time_snapshot(*ambassador_->getTimeFactory()->makeFinal());
  }
  py::dict make_zero_interval() const {
    return logical_interval_snapshot(*ambassador_->getTimeFactory()->makeZero());
  }
  py::dict make_epsilon_interval() const {
    return logical_interval_snapshot(*ambassador_->getTimeFactory()->makeEpsilon());
  }
  py::dict make_logical_time(py::object value) const {
    auto factory = ambassador_->getTimeFactory();
    if (auto* integer = dynamic_cast<rti::HLAinteger64TimeFactory*>(factory.get())) {
      return logical_time_snapshot(*integer->makeLogicalTime(py::cast<std::int64_t>(value)));
    }
    if (auto* floating = dynamic_cast<rti::HLAfloat64TimeFactory*>(factory.get())) {
      return logical_time_snapshot(*floating->makeLogicalTime(py::cast<double>(value)));
    }
    throw rti::RTIinternalError(L"Umbra cannot construct a value for this logical-time factory.");
  }
  py::dict make_logical_interval(py::object value) const {
    auto factory = ambassador_->getTimeFactory();
    if (auto* integer = dynamic_cast<rti::HLAinteger64TimeFactory*>(factory.get())) {
      return logical_interval_snapshot(
          *integer->makeLogicalTimeInterval(py::cast<std::int64_t>(value)));
    }
    if (auto* floating = dynamic_cast<rti::HLAfloat64TimeFactory*>(factory.get())) {
      return logical_interval_snapshot(
          *floating->makeLogicalTimeInterval(py::cast<double>(value)));
    }
    throw rti::RTIinternalError(L"Umbra cannot construct an interval for this logical-time factory.");
  }
  py::dict decode_logical_time(py::bytes const& encoded_value) const {
    return logical_time_snapshot(*ambassador_->getTimeFactory()->decodeLogicalTime(
        variable_length_data(encoded_value)));
  }
  py::dict decode_logical_interval(py::bytes const& encoded_value) const {
    return logical_interval_snapshot(*ambassador_->getTimeFactory()->decodeLogicalTimeInterval(
        variable_length_data(encoded_value)));
  }
  py::dict add_logical_time(
      py::bytes const& encoded_time,
      py::bytes const& encoded_addend) const {
    auto time = ambassador_->getTimeFactory()->decodeLogicalTime(
        variable_length_data(encoded_time));
    auto addend = ambassador_->getTimeFactory()->decodeLogicalTimeInterval(
        variable_length_data(encoded_addend));
    *time += *addend;
    return logical_time_snapshot(*time);
  }
  py::dict subtract_logical_time(
      py::bytes const& encoded_time,
      py::bytes const& encoded_subtrahend) const {
    auto time = ambassador_->getTimeFactory()->decodeLogicalTime(
        variable_length_data(encoded_time));
    auto subtrahend = ambassador_->getTimeFactory()->decodeLogicalTimeInterval(
        variable_length_data(encoded_subtrahend));
    *time -= *subtrahend;
    return logical_time_snapshot(*time);
  }
  py::dict difference_logical_time(
      py::bytes const& encoded_minuend,
      py::bytes const& encoded_subtrahend) const {
    auto minuend = ambassador_->getTimeFactory()->decodeLogicalTime(
        variable_length_data(encoded_minuend));
    auto subtrahend = ambassador_->getTimeFactory()->decodeLogicalTime(
        variable_length_data(encoded_subtrahend));
    auto difference = ambassador_->getTimeFactory()->makeZero();
    difference->setToDifference(*minuend, *subtrahend);
    return logical_interval_snapshot(*difference);
  }
  void enable_time_regulation(py::bytes const& lookahead) {
    ambassador_->enableTimeRegulation(
        *ambassador_->getTimeFactory()->decodeLogicalTimeInterval(
            variable_length_data(lookahead)));
  }
  void disable_time_regulation() { ambassador_->disableTimeRegulation(); }
  void enable_time_constrained() { ambassador_->enableTimeConstrained(); }
  void disable_time_constrained() { ambassador_->disableTimeConstrained(); }
  void enable_asynchronous_delivery() { ambassador_->enableAsynchronousDelivery(); }
  void disable_asynchronous_delivery() { ambassador_->disableAsynchronousDelivery(); }
  void modify_lookahead(py::bytes const& lookahead) {
    ambassador_->modifyLookahead(
        *ambassador_->getTimeFactory()->decodeLogicalTimeInterval(
            variable_length_data(lookahead)));
  }
  py::dict query_lookahead() const {
    auto interval = ambassador_->getTimeFactory()->makeZero();
    ambassador_->queryLookahead(*interval);
    return logical_interval_snapshot(*interval);
  }
  void time_advance_request(py::bytes const& time) {
    ambassador_->timeAdvanceRequest(
        *ambassador_->getTimeFactory()->decodeLogicalTime(variable_length_data(time)));
  }
  void time_advance_request_available(py::bytes const& time) {
    ambassador_->timeAdvanceRequestAvailable(
        *ambassador_->getTimeFactory()->decodeLogicalTime(variable_length_data(time)));
  }
  void next_message_request(py::bytes const& time) {
    ambassador_->nextMessageRequest(
        *ambassador_->getTimeFactory()->decodeLogicalTime(variable_length_data(time)));
  }
  void next_message_request_available(py::bytes const& time) {
    ambassador_->nextMessageRequestAvailable(
        *ambassador_->getTimeFactory()->decodeLogicalTime(variable_length_data(time)));
  }
  void flush_queue_request(py::bytes const& time) {
    ambassador_->flushQueueRequest(
        *ambassador_->getTimeFactory()->decodeLogicalTime(variable_length_data(time)));
  }
  py::dict query_logical_time() const {
    auto factory = ambassador_->getTimeFactory();
    auto time = factory->makeInitial();
    ambassador_->queryLogicalTime(*time);
    return logical_time_snapshot(*time);
  }
  py::dict query_galt() const {
    auto time = ambassador_->getTimeFactory()->makeInitial();
    py::dict result;
    result["valid"] = ambassador_->queryGALT(*time);
    result["time"] = logical_time_snapshot(*time);
    return result;
  }
  py::dict query_lits() const {
    auto time = ambassador_->getTimeFactory()->makeInitial();
    py::dict result;
    result["valid"] = ambassador_->queryLITS(*time);
    result["time"] = logical_time_snapshot(*time);
    return result;
  }
  py::bytes decode_federate_handle(py::bytes const& encoded_value) const {
    return encoded(ambassador_->decodeFederateHandle(variable_length_data(encoded_value)));
  }
  py::bytes decode_object_class_handle(py::bytes const& encoded_value) const {
    return encoded(ambassador_->decodeObjectClassHandle(variable_length_data(encoded_value)));
  }
  py::bytes decode_object_instance_handle(py::bytes const& encoded_value) const {
    return encoded(
        ambassador_->decodeObjectInstanceHandle(variable_length_data(encoded_value)));
  }
  py::bytes decode_attribute_handle(py::bytes const& encoded_value) const {
    return encoded(ambassador_->decodeAttributeHandle(variable_length_data(encoded_value)));
  }
  py::bytes decode_interaction_class_handle(py::bytes const& encoded_value) const {
    return encoded(
        ambassador_->decodeInteractionClassHandle(variable_length_data(encoded_value)));
  }
  py::bytes decode_parameter_handle(py::bytes const& encoded_value) const {
    return encoded(ambassador_->decodeParameterHandle(variable_length_data(encoded_value)));
  }
  py::bytes decode_transportation_type_handle(py::bytes const& encoded_value) const {
    return encoded(rti::umbra_binding_detail::decodeTransportationTypeHandle(
        variable_length_data(encoded_value)));
  }
  py::bytes decode_dimension_handle(py::bytes const& encoded_value) const {
    return encoded(ambassador_->decodeDimensionHandle(variable_length_data(encoded_value)));
  }
  py::bytes decode_region_handle(py::bytes const& encoded_value) const {
    return encoded(ambassador_->decodeRegionHandle(variable_length_data(encoded_value)));
  }
  py::bytes get_object_class_handle(std::string const& object_class_name) {
    return encoded(ambassador_->getObjectClassHandle(wide(object_class_name)));
  }
  std::string get_object_class_name(py::bytes const& object_class) {
    return utf8(ambassador_->getObjectClassName(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class))));
  }
  py::bytes get_federate_handle(std::string const& federate_name) {
    return encoded(ambassador_->getFederateHandle(wide(federate_name)));
  }
  std::string get_federate_name(py::bytes const& federate) {
    return utf8(ambassador_->getFederateName(
        rti::umbra_binding_detail::decodeFederateHandle(variable_length_data(federate))));
  }
  py::bytes get_known_object_class_handle(py::bytes const& object_instance) {
    return encoded(ambassador_->getKnownObjectClassHandle(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance))));
  }
  py::bytes get_attribute_handle(
      py::bytes const& object_class, std::string const& attribute_name) {
    return encoded(ambassador_->getAttributeHandle(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class)),
        wide(attribute_name)));
  }
  std::string get_attribute_name(
      py::bytes const& object_class, py::bytes const& attribute) {
    return utf8(ambassador_->getAttributeName(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class)),
        rti::umbra_binding_detail::decodeAttributeHandle(variable_length_data(attribute))));
  }
  double get_update_rate_value(std::string const& update_rate_designator) {
    return ambassador_->getUpdateRateValue(wide(update_rate_designator));
  }
  double get_update_rate_value_for_attribute(
      py::bytes const& object_instance, py::bytes const& attribute) {
    return ambassador_->getUpdateRateValueForAttribute(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)),
        rti::umbra_binding_detail::decodeAttributeHandle(variable_length_data(attribute)));
  }
  py::bytes get_interaction_class_handle(std::string const& interaction_class_name) {
    return encoded(ambassador_->getInteractionClassHandle(wide(interaction_class_name)));
  }
  std::string get_interaction_class_name(py::bytes const& interaction_class) {
    return utf8(ambassador_->getInteractionClassName(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class))));
  }
  py::bytes get_parameter_handle(
      py::bytes const& interaction_class, std::string const& parameter_name) {
    return encoded(ambassador_->getParameterHandle(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)),
        wide(parameter_name)));
  }
  std::string get_parameter_name(
      py::bytes const& interaction_class, py::bytes const& parameter) {
    return utf8(ambassador_->getParameterName(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)),
        rti::umbra_binding_detail::decodeParameterHandle(variable_length_data(parameter))));
  }
  std::string get_order_type(std::string const& order_type_name_value) {
    return order_type_name(ambassador_->getOrderType(wide(order_type_name_value)));
  }
  std::string get_order_name(std::string const& order_type_value) {
    return utf8(ambassador_->getOrderName(order_type_from(order_type_value)));
  }
  py::bytes get_transportation_type_handle(std::string const& transportation_type_name) {
    return encoded(ambassador_->getTransportationTypeHandle(wide(transportation_type_name)));
  }
  std::string get_transportation_type_name(py::bytes const& transportation_type) {
    return utf8(ambassador_->getTransportationTypeName(
        rti::umbra_binding_detail::decodeTransportationTypeHandle(
            variable_length_data(transportation_type))));
  }
  py::bytes get_dimension_handle(std::string const& dimension_name) {
    return encoded(ambassador_->getDimensionHandle(wide(dimension_name)));
  }
  std::string get_dimension_name(py::bytes const& dimension) {
    return utf8(ambassador_->getDimensionName(
        rti::umbra_binding_detail::decodeDimensionHandle(variable_length_data(dimension))));
  }
  py::list get_available_dimensions_for_object_class(py::bytes const& object_class) {
    py::list result;
    for (auto const& dimension : ambassador_->getAvailableDimensionsForObjectClass(
             rti::umbra_binding_detail::decodeObjectClassHandle(
                 variable_length_data(object_class)))) {
      result.append(encoded(dimension));
    }
    return result;
  }
  py::list get_available_dimensions_for_interaction_class(py::bytes const& interaction_class) {
    py::list result;
    for (auto const& dimension : ambassador_->getAvailableDimensionsForInteractionClass(
             rti::umbra_binding_detail::decodeInteractionClassHandle(
                 variable_length_data(interaction_class)))) {
      result.append(encoded(dimension));
    }
    return result;
  }
  unsigned long get_dimension_upper_bound(py::bytes const& dimension) {
    return ambassador_->getDimensionUpperBound(
        rti::umbra_binding_detail::decodeDimensionHandle(variable_length_data(dimension)));
  }
  unsigned long normalize_service_group(std::string const& service_group) {
    return ambassador_->normalizeServiceGroup(service_group_from(service_group));
  }
  unsigned long normalize_federate_handle(py::bytes const& federate) {
    return ambassador_->normalizeFederateHandle(
        rti::umbra_binding_detail::decodeFederateHandle(variable_length_data(federate)));
  }
  unsigned long normalize_object_class_handle(py::bytes const& object_class) {
    return ambassador_->normalizeObjectClassHandle(
        rti::umbra_binding_detail::decodeObjectClassHandle(variable_length_data(object_class)));
  }
  unsigned long normalize_interaction_class_handle(py::bytes const& interaction_class) {
    return ambassador_->normalizeInteractionClassHandle(
        rti::umbra_binding_detail::decodeInteractionClassHandle(
            variable_length_data(interaction_class)));
  }
  unsigned long normalize_object_instance_handle(py::bytes const& object_instance) {
    return ambassador_->normalizeObjectInstanceHandle(
        rti::umbra_binding_detail::decodeObjectInstanceHandle(
            variable_length_data(object_instance)));
  }
  void create_federation_execution(
      std::string const& federation_name,
      std::string const& fom_module,
      std::string const& logical_time_implementation_name) {
    ambassador_->createFederationExecution(
        wide(federation_name), wide(fom_module), wide(logical_time_implementation_name));
  }
  void create_federation_execution_with_modules(
      std::string const& federation_name,
      py::iterable const& fom_modules,
      std::string const& logical_time_implementation_name) {
    ambassador_->createFederationExecution(
        wide(federation_name),
        wide_strings_from(fom_modules),
        wide(logical_time_implementation_name));
  }
  void create_federation_execution_with_mim(
      std::string const& federation_name,
      py::iterable const& fom_modules,
      std::string const& mim_module,
      std::string const& logical_time_implementation_name) {
    ambassador_->createFederationExecutionWithMIM(
        wide(federation_name),
        wide_strings_from(fom_modules),
        wide(mim_module),
        wide(logical_time_implementation_name));
  }
  void destroy_federation_execution(std::string const& federation_name) {
    ambassador_->destroyFederationExecution(wide(federation_name));
  }

 private:
  std::unique_ptr<rti::RTIambassador> ambassador_;
  std::unique_ptr<PythonFederateAmbassador> federate_ambassador_;
};

class NativeElementBridge {
 public:
  virtual ~NativeElementBridge() = default;
  virtual std::unique_ptr<rti::DataElement> clone_data_element() const = 0;
};

class NativeHLAinteger32BE : public NativeElementBridge {
 public:
  NativeHLAinteger32BE() = default;
  explicit NativeHLAinteger32BE(std::int32_t value) : element_(value) {}

  std::int32_t get_value() const { return element_.get(); }
  void set_value(std::int32_t value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAinteger32BE element_;
};

class NativeHLAinteger16BE : public NativeElementBridge {
 public:
  NativeHLAinteger16BE() = default;
  explicit NativeHLAinteger16BE(std::int16_t value) : element_(value) {}

  std::int16_t get_value() const { return element_.get(); }
  void set_value(std::int16_t value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAinteger16BE element_;
};

class NativeHLAinteger16LE : public NativeElementBridge {
 public:
  NativeHLAinteger16LE() = default;
  explicit NativeHLAinteger16LE(std::int16_t value) : element_(value) {}

  std::int16_t get_value() const { return element_.get(); }
  void set_value(std::int16_t value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAinteger16LE element_;
};

class NativeHLAinteger32LE : public NativeElementBridge {
 public:
  NativeHLAinteger32LE() = default;
  explicit NativeHLAinteger32LE(std::int32_t value) : element_(value) {}

  std::int32_t get_value() const { return element_.get(); }
  void set_value(std::int32_t value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAinteger32LE element_;
};

class NativeHLAinteger64BE : public NativeElementBridge {
 public:
  NativeHLAinteger64BE() = default;
  explicit NativeHLAinteger64BE(std::int64_t value) : element_(value) {}

  std::int64_t get_value() const { return element_.get(); }
  void set_value(std::int64_t value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAinteger64BE element_;
};

class NativeHLAinteger64LE : public NativeElementBridge {
 public:
  NativeHLAinteger64LE() = default;
  explicit NativeHLAinteger64LE(std::int64_t value) : element_(value) {}

  std::int64_t get_value() const { return element_.get(); }
  void set_value(std::int64_t value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAinteger64LE element_;
};

class NativeHLAfloat64BE : public NativeElementBridge {
 public:
  NativeHLAfloat64BE() = default;
  explicit NativeHLAfloat64BE(double value) : element_(value) {}

  double get_value() const { return element_.get(); }
  void set_value(double value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAfloat64BE element_;
};

class NativeHLAfloat32BE : public NativeElementBridge {
 public:
  NativeHLAfloat32BE() = default;
  explicit NativeHLAfloat32BE(float value) : element_(value) {}

  float get_value() const { return element_.get(); }
  void set_value(float value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAfloat32BE element_;
};

class NativeHLAfloat64LE : public NativeElementBridge {
 public:
  NativeHLAfloat64LE() = default;
  explicit NativeHLAfloat64LE(double value) : element_(value) {}

  double get_value() const { return element_.get(); }
  void set_value(double value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAfloat64LE element_;
};

class NativeHLAfloat32LE : public NativeElementBridge {
 public:
  NativeHLAfloat32LE() = default;
  explicit NativeHLAfloat32LE(float value) : element_(value) {}

  float get_value() const { return element_.get(); }
  void set_value(float value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAfloat32LE element_;
};

class NativeHLAunsignedInteger32BE : public NativeElementBridge {
 public:
  NativeHLAunsignedInteger32BE() = default;
  explicit NativeHLAunsignedInteger32BE(std::uint32_t value) : element_(value) {}

  std::uint32_t get_value() const { return element_.get(); }
  void set_value(std::uint32_t value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAunsignedInteger32BE element_;
};

class NativeHLAunsignedInteger16BE : public NativeElementBridge {
 public:
  NativeHLAunsignedInteger16BE() = default;
  explicit NativeHLAunsignedInteger16BE(std::uint16_t value) : element_(value) {}

  std::uint16_t get_value() const { return element_.get(); }
  void set_value(std::uint16_t value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAunsignedInteger16BE element_;
};

class NativeHLAunsignedInteger16LE : public NativeElementBridge {
 public:
  NativeHLAunsignedInteger16LE() = default;
  explicit NativeHLAunsignedInteger16LE(std::uint16_t value) : element_(value) {}

  std::uint16_t get_value() const { return element_.get(); }
  void set_value(std::uint16_t value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAunsignedInteger16LE element_;
};

class NativeHLAunsignedInteger32LE : public NativeElementBridge {
 public:
  NativeHLAunsignedInteger32LE() = default;
  explicit NativeHLAunsignedInteger32LE(std::uint32_t value) : element_(value) {}

  std::uint32_t get_value() const { return element_.get(); }
  void set_value(std::uint32_t value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAunsignedInteger32LE element_;
};

class NativeHLAunsignedInteger64BE : public NativeElementBridge {
 public:
  NativeHLAunsignedInteger64BE() = default;
  explicit NativeHLAunsignedInteger64BE(std::uint64_t value) : element_(value) {}

  std::uint64_t get_value() const { return element_.get(); }
  void set_value(std::uint64_t value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAunsignedInteger64BE element_;
};

class NativeHLAunsignedInteger64LE : public NativeElementBridge {
 public:
  NativeHLAunsignedInteger64LE() = default;
  explicit NativeHLAunsignedInteger64LE(std::uint64_t value) : element_(value) {}

  std::uint64_t get_value() const { return element_.get(); }
  void set_value(std::uint64_t value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAunsignedInteger64LE element_;
};

class NativeHLAbyte : public NativeElementBridge {
 public:
  NativeHLAbyte() = default;
  explicit NativeHLAbyte(std::uint8_t value) : element_(value) {}

  std::uint8_t get_value() const { return element_.get(); }
  void set_value(std::uint8_t value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAbyte element_;
};

class NativeHLAoctet : public NativeElementBridge {
 public:
  NativeHLAoctet() = default;
  explicit NativeHLAoctet(std::uint8_t value) : element_(value) {}

  std::uint8_t get_value() const { return element_.get(); }
  void set_value(std::uint8_t value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAoctet element_;
};

class NativeHLAASCIIchar : public NativeElementBridge {
 public:
  NativeHLAASCIIchar() = default;
  explicit NativeHLAASCIIchar(std::uint8_t value) : element_(static_cast<char>(value)) {}

  std::uint8_t get_value() const {
    return static_cast<std::uint8_t>(static_cast<unsigned char>(element_.get()));
  }
  void set_value(std::uint8_t value) { element_.set(static_cast<char>(value)); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAASCIIchar element_;
};

class NativeHLAASCIIstring : public NativeElementBridge {
 public:
  NativeHLAASCIIstring() = default;
  explicit NativeHLAASCIIstring(std::string const& value) : element_(value) {}

  std::string get_value() const { return element_.get(); }
  void set_value(std::string const& value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAASCIIstring element_;
};

class NativeHLAunicodeChar : public NativeElementBridge {
 public:
  NativeHLAunicodeChar() = default;
  explicit NativeHLAunicodeChar(std::uint16_t value)
      : element_(static_cast<wchar_t>(value)) {}

  std::uint16_t get_value() const {
    return static_cast<std::uint16_t>(element_.get());
  }
  void set_value(std::uint16_t value) { element_.set(static_cast<wchar_t>(value)); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAunicodeChar element_;
};

class NativeHLAoctetPairBE : public NativeElementBridge {
 public:
  NativeHLAoctetPairBE() = default;
  explicit NativeHLAoctetPairBE(std::uint16_t value)
      : element_(rti::OctetPair{
            static_cast<rti::Octet>((value >> 8U) & 0xffU),
            static_cast<rti::Octet>(value & 0xffU)}) {}

  std::uint16_t get_value() const {
    auto const pair = element_.get();
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(static_cast<std::uint8_t>(pair.first)) << 8U) |
        static_cast<std::uint8_t>(pair.second));
  }
  void set_value(std::uint16_t value) {
    element_.set(rti::OctetPair{
        static_cast<rti::Octet>((value >> 8U) & 0xffU),
        static_cast<rti::Octet>(value & 0xffU)});
  }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAoctetPairBE element_;
};

class NativeHLAoctetPairLE : public NativeElementBridge {
 public:
  NativeHLAoctetPairLE() = default;
  explicit NativeHLAoctetPairLE(std::uint16_t value)
      : element_(rti::OctetPair{
            static_cast<rti::Octet>((value >> 8U) & 0xffU),
            static_cast<rti::Octet>(value & 0xffU)}) {}

  std::uint16_t get_value() const {
    auto const pair = element_.get();
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(static_cast<std::uint8_t>(pair.first)) << 8U) |
        static_cast<std::uint8_t>(pair.second));
  }
  void set_value(std::uint16_t value) {
    element_.set(rti::OctetPair{
        static_cast<rti::Octet>((value >> 8U) & 0xffU),
        static_cast<rti::Octet>(value & 0xffU)});
  }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAoctetPairLE element_;
};

class NativeHLAopaqueData : public NativeElementBridge {
 public:
  NativeHLAopaqueData() = default;
  explicit NativeHLAopaqueData(py::bytes const& value) { set_value(value); }

  py::bytes get_value() const {
    auto const* data = element_.get();
    auto const length = element_.dataLength();
    return py::bytes(
        data == nullptr ? "" : reinterpret_cast<char const*>(data), length);
  }
  void set_value(py::bytes const& value) {
    std::string copied = value;
    element_.set(
        copied.empty() ? nullptr : reinterpret_cast<rti::Octet const*>(copied.data()),
        copied.size());
  }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }
  std::size_t data_length() const { return element_.dataLength(); }

 private:
  rti::HLAopaqueData element_;
};

class NativeHLAboolean : public NativeElementBridge {
 public:
  NativeHLAboolean() = default;
  explicit NativeHLAboolean(bool value) : element_(value) {}

  bool get_value() const { return element_.get(); }
  void set_value(bool value) { element_.set(value); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAboolean element_;
};

class NativeHLAunicodeString : public NativeElementBridge {
 public:
  NativeHLAunicodeString() = default;
  explicit NativeHLAunicodeString(std::string const& value) : element_(wide(value)) {}

  std::string get_value() const { return utf8(element_.get()); }
  void set_value(std::string const& value) { element_.set(wide(value)); }
  py::bytes to_byte_array() const { return element_bytes(element_); }
  void decode(py::bytes const& value) { element_.decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_.getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_.getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override { return element_.clone(); }

 private:
  rti::HLAunicodeString element_;
};

std::unique_ptr<rti::DataElement> native_prototype(py::object const& prototype);

class NativeHLAvariableArray : public NativeElementBridge {
 public:
  explicit NativeHLAvariableArray(py::object const& prototype)
      : prototype_(native_prototype(prototype)),
        element_(std::make_unique<rti::HLAvariableArray>(*prototype_)) {}

  std::size_t size() const { return element_->size(); }

  void add_element(py::bytes const& encoded) {
    auto value = prototype_->clone();
    value->decode(variable_length_data(encoded));
    element_->addElement(*value);
  }

  void set_element(std::size_t index, py::bytes const& encoded) {
    auto value = prototype_->clone();
    value->decode(variable_length_data(encoded));
    element_->set(index, *value);
  }

  py::bytes get_element_bytes(std::size_t index) const {
    return element_bytes(element_->get(index));
  }

  py::bytes to_byte_array() const { return element_bytes(*element_); }
  void decode(py::bytes const& value) { element_->decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_->getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_->getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override {
    return element_->clone();
  }

 private:
  std::unique_ptr<rti::DataElement> prototype_;
  std::unique_ptr<rti::HLAvariableArray> element_;
};

class NativeHLAfixedArray : public NativeElementBridge {
 public:
  NativeHLAfixedArray(py::object const& prototype, std::size_t length)
      : prototype_(native_prototype(prototype)),
        element_(std::make_unique<rti::HLAfixedArray>(*prototype_, length)) {}

  std::size_t size() const { return element_->size(); }

  void set_element(std::size_t index, py::bytes const& encoded) {
    auto value = prototype_->clone();
    value->decode(variable_length_data(encoded));
    element_->set(index, *value);
  }

  py::bytes get_element_bytes(std::size_t index) const {
    return element_bytes(element_->get(index));
  }

  py::bytes to_byte_array() const { return element_bytes(*element_); }
  void decode(py::bytes const& value) { element_->decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_->getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_->getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override {
    return element_->clone();
  }

 private:
  std::unique_ptr<rti::DataElement> prototype_;
  std::unique_ptr<rti::HLAfixedArray> element_;
};

class NativeHLAfixedRecord : public NativeElementBridge {
 public:
  NativeHLAfixedRecord() : element_(std::make_unique<rti::HLAfixedRecord>()) {}

  std::size_t size() const { return element_->size(); }

  void append_element(py::object const& prototype, py::bytes const& encoded) {
    auto value = native_prototype(prototype);
    value->decode(variable_length_data(encoded));
    element_->appendElement(*value);
  }

  void set_element(
      std::size_t index,
      py::object const& prototype,
      py::bytes const& encoded) {
    auto value = native_prototype(prototype);
    value->decode(variable_length_data(encoded));
    element_->set(index, *value);
  }

  py::bytes get_element_bytes(std::size_t index) const {
    return element_bytes(element_->get(index));
  }

  py::bytes to_byte_array() const { return element_bytes(*element_); }
  void decode(py::bytes const& value) { element_->decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_->getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_->getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override {
    return element_->clone();
  }

 private:
  std::unique_ptr<rti::HLAfixedRecord> element_;
};

class NativeHLAvariantRecord : public NativeElementBridge {
 public:
  explicit NativeHLAvariantRecord(py::object const& discriminant_prototype)
      : discriminant_prototype_(native_prototype(discriminant_prototype)),
        element_(std::make_unique<rti::HLAvariantRecord>(*discriminant_prototype_)) {}

  void add_variant(
      py::object const& discriminant_prototype,
      py::bytes const& discriminant_encoded,
      py::object const& value_prototype,
      py::bytes const& value_encoded) {
    auto discriminant = native_prototype(discriminant_prototype);
    discriminant->decode(variable_length_data(discriminant_encoded));
    auto value = native_prototype(value_prototype);
    value->decode(variable_length_data(value_encoded));
    element_->addVariant(*discriminant, *value);
  }

  void set_variant(
      py::object const& discriminant_prototype,
      py::bytes const& discriminant_encoded,
      py::object const& value_prototype,
      py::bytes const& value_encoded) {
    auto discriminant = native_prototype(discriminant_prototype);
    discriminant->decode(variable_length_data(discriminant_encoded));
    auto value = native_prototype(value_prototype);
    value->decode(variable_length_data(value_encoded));
    element_->setVariant(*discriminant, *value);
  }

  void set_discriminant(
      py::object const& discriminant_prototype,
      py::bytes const& discriminant_encoded) {
    auto discriminant = native_prototype(discriminant_prototype);
    discriminant->decode(variable_length_data(discriminant_encoded));
    element_->setDiscriminant(*discriminant);
  }

  py::bytes get_discriminant_bytes() const { return element_bytes(element_->getDiscriminant()); }
  py::bytes get_variant_bytes() const { return element_bytes(element_->getVariant()); }
  py::bytes to_byte_array() const { return element_bytes(*element_); }
  void decode(py::bytes const& value) { element_->decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_->getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_->getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override {
    return element_->clone();
  }

 private:
  std::unique_ptr<rti::DataElement> discriminant_prototype_;
  std::unique_ptr<rti::HLAvariantRecord> element_;
};

class NativeHLAextendableVariantRecord : public NativeElementBridge {
 public:
  explicit NativeHLAextendableVariantRecord(py::object const& discriminant_prototype)
      : discriminant_prototype_(native_prototype(discriminant_prototype)),
        element_(std::make_unique<rti::HLAextendableVariantRecord>(*discriminant_prototype_)) {}

  void add_variant(
      py::object const& discriminant_prototype,
      py::bytes const& discriminant_encoded,
      py::object const& value_prototype,
      py::bytes const& value_encoded) {
    auto discriminant = native_prototype(discriminant_prototype);
    discriminant->decode(variable_length_data(discriminant_encoded));
    auto value = native_prototype(value_prototype);
    value->decode(variable_length_data(value_encoded));
    element_->addVariant(*discriminant, *value);
  }

  void set_variant(
      py::object const& discriminant_prototype,
      py::bytes const& discriminant_encoded,
      py::object const& value_prototype,
      py::bytes const& value_encoded) {
    auto discriminant = native_prototype(discriminant_prototype);
    discriminant->decode(variable_length_data(discriminant_encoded));
    auto value = native_prototype(value_prototype);
    value->decode(variable_length_data(value_encoded));
    element_->setVariant(*discriminant, *value);
  }

  void set_discriminant(
      py::object const& discriminant_prototype,
      py::bytes const& discriminant_encoded) {
    auto discriminant = native_prototype(discriminant_prototype);
    discriminant->decode(variable_length_data(discriminant_encoded));
    element_->setDiscriminant(*discriminant);
  }

  py::bytes get_discriminant_bytes() const { return element_bytes(element_->getDiscriminant()); }
  py::bytes get_variant_bytes() const { return element_bytes(element_->getVariant()); }
  py::bytes to_byte_array() const { return element_bytes(*element_); }
  void decode(py::bytes const& value) { element_->decode(variable_length_data(value)); }
  std::size_t get_encoded_length() const { return element_->getEncodedLength(); }
  unsigned int get_octet_boundary() const { return element_->getOctetBoundary(); }
  std::unique_ptr<rti::DataElement> clone_data_element() const override {
    return element_->clone();
  }

 private:
  std::unique_ptr<rti::DataElement> discriminant_prototype_;
  std::unique_ptr<rti::HLAextendableVariantRecord> element_;
};

std::unique_ptr<rti::DataElement> native_prototype(py::object const& prototype) {
#define UMBRA_NATIVE_PROTOTYPE(Type) \
  if (py::isinstance<Type>(prototype)) { \
    return py::cast<Type&>(prototype).clone_data_element(); \
  }
  UMBRA_NATIVE_PROTOTYPE(NativeHLAinteger32BE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAinteger16BE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAinteger16LE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAinteger32LE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAinteger64BE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAinteger64LE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAfloat64BE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAfloat32BE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAfloat64LE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAfloat32LE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAunsignedInteger32BE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAunsignedInteger16BE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAunsignedInteger16LE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAunsignedInteger32LE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAunsignedInteger64BE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAunsignedInteger64LE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAbyte)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAoctet)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAASCIIchar)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAASCIIstring)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAunicodeChar)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAoctetPairBE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAoctetPairLE)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAopaqueData)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAboolean)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAunicodeString)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAvariableArray)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAfixedArray)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAfixedRecord)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAvariantRecord)
  UMBRA_NATIVE_PROTOTYPE(NativeHLAextendableVariantRecord)
#undef UMBRA_NATIVE_PROTOTYPE
  throw py::type_error("prototype must be a native DataElement");
}

}  // namespace

PYBIND11_MODULE(_native, module) {
  module.doc() = "Narrow pybind11 bridge to the Umbra C++ RTI foundation.";
  native_rti_error_type = PyErr_NewException(
      "umbra._native.rti1516_2025._native.NativeRtiError", PyExc_RuntimeError, nullptr);
  module.add_object("NativeRtiError", py::reinterpret_steal<py::object>(native_rti_error_type));

  py::register_exception_translator([](std::exception_ptr exception) {
    try {
      if (exception) {
        std::rethrow_exception(exception);
      }
    } catch (rti::Exception const& error) {
      auto const message = utf8(error.name()) + ": " + utf8(error.what());
      PyErr_SetString(native_rti_error_type, message.c_str());
    }
  });

  py::class_<NativeConfigurationResult>(module, "NativeConfigurationResult")
      .def_readonly("configuration_used", &NativeConfigurationResult::configuration_used)
      .def_readonly("address_used", &NativeConfigurationResult::address_used)
      .def_readonly("additional_settings_result", &NativeConfigurationResult::additional_settings_result)
      .def_readonly("message", &NativeConfigurationResult::message);

  py::class_<NativeHLAinteger32BE>(module, "NativeHLAinteger32BE")
      .def(py::init<>())
      .def(py::init<std::int32_t>())
      .def("get_value", &NativeHLAinteger32BE::get_value)
      .def("set_value", &NativeHLAinteger32BE::set_value)
      .def("to_byte_array", &NativeHLAinteger32BE::to_byte_array)
      .def("decode", &NativeHLAinteger32BE::decode)
      .def("get_encoded_length", &NativeHLAinteger32BE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAinteger32BE::get_octet_boundary);

  py::class_<NativeHLAinteger16BE>(module, "NativeHLAinteger16BE")
      .def(py::init<>())
      .def(py::init<std::int16_t>())
      .def("get_value", &NativeHLAinteger16BE::get_value)
      .def("set_value", &NativeHLAinteger16BE::set_value)
      .def("to_byte_array", &NativeHLAinteger16BE::to_byte_array)
      .def("decode", &NativeHLAinteger16BE::decode)
      .def("get_encoded_length", &NativeHLAinteger16BE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAinteger16BE::get_octet_boundary);

  py::class_<NativeHLAinteger16LE>(module, "NativeHLAinteger16LE")
      .def(py::init<>())
      .def(py::init<std::int16_t>())
      .def("get_value", &NativeHLAinteger16LE::get_value)
      .def("set_value", &NativeHLAinteger16LE::set_value)
      .def("to_byte_array", &NativeHLAinteger16LE::to_byte_array)
      .def("decode", &NativeHLAinteger16LE::decode)
      .def("get_encoded_length", &NativeHLAinteger16LE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAinteger16LE::get_octet_boundary);

  py::class_<NativeHLAinteger32LE>(module, "NativeHLAinteger32LE")
      .def(py::init<>())
      .def(py::init<std::int32_t>())
      .def("get_value", &NativeHLAinteger32LE::get_value)
      .def("set_value", &NativeHLAinteger32LE::set_value)
      .def("to_byte_array", &NativeHLAinteger32LE::to_byte_array)
      .def("decode", &NativeHLAinteger32LE::decode)
      .def("get_encoded_length", &NativeHLAinteger32LE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAinteger32LE::get_octet_boundary);

  py::class_<NativeHLAinteger64BE>(module, "NativeHLAinteger64BE")
      .def(py::init<>())
      .def(py::init<std::int64_t>())
      .def("get_value", &NativeHLAinteger64BE::get_value)
      .def("set_value", &NativeHLAinteger64BE::set_value)
      .def("to_byte_array", &NativeHLAinteger64BE::to_byte_array)
      .def("decode", &NativeHLAinteger64BE::decode)
      .def("get_encoded_length", &NativeHLAinteger64BE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAinteger64BE::get_octet_boundary);

  py::class_<NativeHLAinteger64LE>(module, "NativeHLAinteger64LE")
      .def(py::init<>())
      .def(py::init<std::int64_t>())
      .def("get_value", &NativeHLAinteger64LE::get_value)
      .def("set_value", &NativeHLAinteger64LE::set_value)
      .def("to_byte_array", &NativeHLAinteger64LE::to_byte_array)
      .def("decode", &NativeHLAinteger64LE::decode)
      .def("get_encoded_length", &NativeHLAinteger64LE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAinteger64LE::get_octet_boundary);

  py::class_<NativeHLAfloat64BE>(module, "NativeHLAfloat64BE")
      .def(py::init<>())
      .def(py::init<double>())
      .def("get_value", &NativeHLAfloat64BE::get_value)
      .def("set_value", &NativeHLAfloat64BE::set_value)
      .def("to_byte_array", &NativeHLAfloat64BE::to_byte_array)
      .def("decode", &NativeHLAfloat64BE::decode)
      .def("get_encoded_length", &NativeHLAfloat64BE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAfloat64BE::get_octet_boundary);

  py::class_<NativeHLAfloat32BE>(module, "NativeHLAfloat32BE")
      .def(py::init<>())
      .def(py::init<float>())
      .def("get_value", &NativeHLAfloat32BE::get_value)
      .def("set_value", &NativeHLAfloat32BE::set_value)
      .def("to_byte_array", &NativeHLAfloat32BE::to_byte_array)
      .def("decode", &NativeHLAfloat32BE::decode)
      .def("get_encoded_length", &NativeHLAfloat32BE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAfloat32BE::get_octet_boundary);

  py::class_<NativeHLAfloat64LE>(module, "NativeHLAfloat64LE")
      .def(py::init<>())
      .def(py::init<double>())
      .def("get_value", &NativeHLAfloat64LE::get_value)
      .def("set_value", &NativeHLAfloat64LE::set_value)
      .def("to_byte_array", &NativeHLAfloat64LE::to_byte_array)
      .def("decode", &NativeHLAfloat64LE::decode)
      .def("get_encoded_length", &NativeHLAfloat64LE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAfloat64LE::get_octet_boundary);

  py::class_<NativeHLAfloat32LE>(module, "NativeHLAfloat32LE")
      .def(py::init<>())
      .def(py::init<float>())
      .def("get_value", &NativeHLAfloat32LE::get_value)
      .def("set_value", &NativeHLAfloat32LE::set_value)
      .def("to_byte_array", &NativeHLAfloat32LE::to_byte_array)
      .def("decode", &NativeHLAfloat32LE::decode)
      .def("get_encoded_length", &NativeHLAfloat32LE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAfloat32LE::get_octet_boundary);

  py::class_<NativeHLAunsignedInteger32BE>(module, "NativeHLAunsignedInteger32BE")
      .def(py::init<>())
      .def(py::init<std::uint32_t>())
      .def("get_value", &NativeHLAunsignedInteger32BE::get_value)
      .def("set_value", &NativeHLAunsignedInteger32BE::set_value)
      .def("to_byte_array", &NativeHLAunsignedInteger32BE::to_byte_array)
      .def("decode", &NativeHLAunsignedInteger32BE::decode)
      .def("get_encoded_length", &NativeHLAunsignedInteger32BE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAunsignedInteger32BE::get_octet_boundary);

  py::class_<NativeHLAunsignedInteger16BE>(module, "NativeHLAunsignedInteger16BE")
      .def(py::init<>())
      .def(py::init<std::uint16_t>())
      .def("get_value", &NativeHLAunsignedInteger16BE::get_value)
      .def("set_value", &NativeHLAunsignedInteger16BE::set_value)
      .def("to_byte_array", &NativeHLAunsignedInteger16BE::to_byte_array)
      .def("decode", &NativeHLAunsignedInteger16BE::decode)
      .def("get_encoded_length", &NativeHLAunsignedInteger16BE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAunsignedInteger16BE::get_octet_boundary);

  py::class_<NativeHLAunsignedInteger16LE>(module, "NativeHLAunsignedInteger16LE")
      .def(py::init<>())
      .def(py::init<std::uint16_t>())
      .def("get_value", &NativeHLAunsignedInteger16LE::get_value)
      .def("set_value", &NativeHLAunsignedInteger16LE::set_value)
      .def("to_byte_array", &NativeHLAunsignedInteger16LE::to_byte_array)
      .def("decode", &NativeHLAunsignedInteger16LE::decode)
      .def("get_encoded_length", &NativeHLAunsignedInteger16LE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAunsignedInteger16LE::get_octet_boundary);

  py::class_<NativeHLAunsignedInteger32LE>(module, "NativeHLAunsignedInteger32LE")
      .def(py::init<>())
      .def(py::init<std::uint32_t>())
      .def("get_value", &NativeHLAunsignedInteger32LE::get_value)
      .def("set_value", &NativeHLAunsignedInteger32LE::set_value)
      .def("to_byte_array", &NativeHLAunsignedInteger32LE::to_byte_array)
      .def("decode", &NativeHLAunsignedInteger32LE::decode)
      .def("get_encoded_length", &NativeHLAunsignedInteger32LE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAunsignedInteger32LE::get_octet_boundary);

  py::class_<NativeHLAunsignedInteger64BE>(module, "NativeHLAunsignedInteger64BE")
      .def(py::init<>())
      .def(py::init<std::uint64_t>())
      .def("get_value", &NativeHLAunsignedInteger64BE::get_value)
      .def("set_value", &NativeHLAunsignedInteger64BE::set_value)
      .def("to_byte_array", &NativeHLAunsignedInteger64BE::to_byte_array)
      .def("decode", &NativeHLAunsignedInteger64BE::decode)
      .def("get_encoded_length", &NativeHLAunsignedInteger64BE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAunsignedInteger64BE::get_octet_boundary);

  py::class_<NativeHLAunsignedInteger64LE>(module, "NativeHLAunsignedInteger64LE")
      .def(py::init<>())
      .def(py::init<std::uint64_t>())
      .def("get_value", &NativeHLAunsignedInteger64LE::get_value)
      .def("set_value", &NativeHLAunsignedInteger64LE::set_value)
      .def("to_byte_array", &NativeHLAunsignedInteger64LE::to_byte_array)
      .def("decode", &NativeHLAunsignedInteger64LE::decode)
      .def("get_encoded_length", &NativeHLAunsignedInteger64LE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAunsignedInteger64LE::get_octet_boundary);

  py::class_<NativeHLAbyte>(module, "NativeHLAbyte")
      .def(py::init<>())
      .def(py::init<std::uint8_t>())
      .def("get_value", &NativeHLAbyte::get_value)
      .def("set_value", &NativeHLAbyte::set_value)
      .def("to_byte_array", &NativeHLAbyte::to_byte_array)
      .def("decode", &NativeHLAbyte::decode)
      .def("get_encoded_length", &NativeHLAbyte::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAbyte::get_octet_boundary);

  py::class_<NativeHLAoctet>(module, "NativeHLAoctet")
      .def(py::init<>())
      .def(py::init<std::uint8_t>())
      .def("get_value", &NativeHLAoctet::get_value)
      .def("set_value", &NativeHLAoctet::set_value)
      .def("to_byte_array", &NativeHLAoctet::to_byte_array)
      .def("decode", &NativeHLAoctet::decode)
      .def("get_encoded_length", &NativeHLAoctet::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAoctet::get_octet_boundary);

  py::class_<NativeHLAASCIIchar>(module, "NativeHLAASCIIchar")
      .def(py::init<>())
      .def(py::init<std::uint8_t>())
      .def("get_value", &NativeHLAASCIIchar::get_value)
      .def("set_value", &NativeHLAASCIIchar::set_value)
      .def("to_byte_array", &NativeHLAASCIIchar::to_byte_array)
      .def("decode", &NativeHLAASCIIchar::decode)
      .def("get_encoded_length", &NativeHLAASCIIchar::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAASCIIchar::get_octet_boundary);

  py::class_<NativeHLAASCIIstring>(module, "NativeHLAASCIIstring")
      .def(py::init<>())
      .def(py::init<std::string const&>())
      .def("get_value", &NativeHLAASCIIstring::get_value)
      .def("set_value", &NativeHLAASCIIstring::set_value)
      .def("to_byte_array", &NativeHLAASCIIstring::to_byte_array)
      .def("decode", &NativeHLAASCIIstring::decode)
      .def("get_encoded_length", &NativeHLAASCIIstring::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAASCIIstring::get_octet_boundary);

  py::class_<NativeHLAunicodeChar>(module, "NativeHLAunicodeChar")
      .def(py::init<>())
      .def(py::init<std::uint16_t>())
      .def("get_value", &NativeHLAunicodeChar::get_value)
      .def("set_value", &NativeHLAunicodeChar::set_value)
      .def("to_byte_array", &NativeHLAunicodeChar::to_byte_array)
      .def("decode", &NativeHLAunicodeChar::decode)
      .def("get_encoded_length", &NativeHLAunicodeChar::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAunicodeChar::get_octet_boundary);

  py::class_<NativeHLAoctetPairBE>(module, "NativeHLAoctetPairBE")
      .def(py::init<>())
      .def(py::init<std::uint16_t>())
      .def("get_value", &NativeHLAoctetPairBE::get_value)
      .def("set_value", &NativeHLAoctetPairBE::set_value)
      .def("to_byte_array", &NativeHLAoctetPairBE::to_byte_array)
      .def("decode", &NativeHLAoctetPairBE::decode)
      .def("get_encoded_length", &NativeHLAoctetPairBE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAoctetPairBE::get_octet_boundary);

  py::class_<NativeHLAoctetPairLE>(module, "NativeHLAoctetPairLE")
      .def(py::init<>())
      .def(py::init<std::uint16_t>())
      .def("get_value", &NativeHLAoctetPairLE::get_value)
      .def("set_value", &NativeHLAoctetPairLE::set_value)
      .def("to_byte_array", &NativeHLAoctetPairLE::to_byte_array)
      .def("decode", &NativeHLAoctetPairLE::decode)
      .def("get_encoded_length", &NativeHLAoctetPairLE::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAoctetPairLE::get_octet_boundary);

  py::class_<NativeHLAopaqueData>(module, "NativeHLAopaqueData")
      .def(py::init<>())
      .def(py::init<py::bytes const&>())
      .def("get_value", &NativeHLAopaqueData::get_value)
      .def("set_value", &NativeHLAopaqueData::set_value)
      .def("to_byte_array", &NativeHLAopaqueData::to_byte_array)
      .def("decode", &NativeHLAopaqueData::decode)
      .def("get_encoded_length", &NativeHLAopaqueData::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAopaqueData::get_octet_boundary)
      .def("data_length", &NativeHLAopaqueData::data_length);

  py::class_<NativeHLAvariableArray>(module, "NativeHLAvariableArray")
      .def(py::init<py::object const&>())
      .def("size", &NativeHLAvariableArray::size)
      .def("add_element", &NativeHLAvariableArray::add_element)
      .def("set_element", &NativeHLAvariableArray::set_element)
      .def("get_element_bytes", &NativeHLAvariableArray::get_element_bytes)
      .def("to_byte_array", &NativeHLAvariableArray::to_byte_array)
      .def("decode", &NativeHLAvariableArray::decode)
      .def("get_encoded_length", &NativeHLAvariableArray::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAvariableArray::get_octet_boundary);

  py::class_<NativeHLAfixedArray>(module, "NativeHLAfixedArray")
      .def(py::init<py::object const&, std::size_t>())
      .def("size", &NativeHLAfixedArray::size)
      .def("set_element", &NativeHLAfixedArray::set_element)
      .def("get_element_bytes", &NativeHLAfixedArray::get_element_bytes)
      .def("to_byte_array", &NativeHLAfixedArray::to_byte_array)
      .def("decode", &NativeHLAfixedArray::decode)
      .def("get_encoded_length", &NativeHLAfixedArray::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAfixedArray::get_octet_boundary);

  py::class_<NativeHLAfixedRecord>(module, "NativeHLAfixedRecord")
      .def(py::init<>())
      .def("size", &NativeHLAfixedRecord::size)
      .def("append_element", &NativeHLAfixedRecord::append_element)
      .def("set_element", &NativeHLAfixedRecord::set_element)
      .def("get_element_bytes", &NativeHLAfixedRecord::get_element_bytes)
      .def("to_byte_array", &NativeHLAfixedRecord::to_byte_array)
      .def("decode", &NativeHLAfixedRecord::decode)
      .def("get_encoded_length", &NativeHLAfixedRecord::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAfixedRecord::get_octet_boundary);

  py::class_<NativeHLAvariantRecord>(module, "NativeHLAvariantRecord")
      .def(py::init<py::object const&>())
      .def("add_variant", &NativeHLAvariantRecord::add_variant)
      .def("set_variant", &NativeHLAvariantRecord::set_variant)
      .def("set_discriminant", &NativeHLAvariantRecord::set_discriminant)
      .def("get_discriminant_bytes", &NativeHLAvariantRecord::get_discriminant_bytes)
      .def("get_variant_bytes", &NativeHLAvariantRecord::get_variant_bytes)
      .def("to_byte_array", &NativeHLAvariantRecord::to_byte_array)
      .def("decode", &NativeHLAvariantRecord::decode)
      .def("get_encoded_length", &NativeHLAvariantRecord::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAvariantRecord::get_octet_boundary);

  py::class_<NativeHLAextendableVariantRecord>(module, "NativeHLAextendableVariantRecord")
      .def(py::init<py::object const&>())
      .def("add_variant", &NativeHLAextendableVariantRecord::add_variant)
      .def("set_variant", &NativeHLAextendableVariantRecord::set_variant)
      .def("set_discriminant", &NativeHLAextendableVariantRecord::set_discriminant)
      .def("get_discriminant_bytes", &NativeHLAextendableVariantRecord::get_discriminant_bytes)
      .def("get_variant_bytes", &NativeHLAextendableVariantRecord::get_variant_bytes)
      .def("to_byte_array", &NativeHLAextendableVariantRecord::to_byte_array)
      .def("decode", &NativeHLAextendableVariantRecord::decode)
      .def("get_encoded_length", &NativeHLAextendableVariantRecord::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAextendableVariantRecord::get_octet_boundary);

  py::class_<NativeHLAboolean>(module, "NativeHLAboolean")
      .def(py::init<>())
      .def(py::init<bool>())
      .def("get_value", &NativeHLAboolean::get_value)
      .def("set_value", &NativeHLAboolean::set_value)
      .def("to_byte_array", &NativeHLAboolean::to_byte_array)
      .def("decode", &NativeHLAboolean::decode)
      .def("get_encoded_length", &NativeHLAboolean::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAboolean::get_octet_boundary);

  py::class_<NativeHLAunicodeString>(module, "NativeHLAunicodeString")
      .def(py::init<>())
      .def(py::init<std::string const&>())
      .def("get_value", &NativeHLAunicodeString::get_value)
      .def("set_value", &NativeHLAunicodeString::set_value)
      .def("to_byte_array", &NativeHLAunicodeString::to_byte_array)
      .def("decode", &NativeHLAunicodeString::decode)
      .def("get_encoded_length", &NativeHLAunicodeString::get_encoded_length)
      .def("get_octet_boundary", &NativeHLAunicodeString::get_octet_boundary);

  py::class_<NativeAmbassador>(module, "NativeAmbassador")
      .def(py::init<>())
      .def(
          "connect",
          &NativeAmbassador::connect,
          py::arg("federate_ambassador"),
          py::arg("callback_model"),
          py::arg("configuration_name") = "",
          py::arg("rti_address") = "",
          py::arg("additional_settings") = "",
          py::arg("has_configuration") = false,
          py::arg("has_no_credentials") = false)
      .def("disconnect", &NativeAmbassador::disconnect)
      .def("evoke_callback", &NativeAmbassador::evoke_callback)
      .def("evoke_multiple_callbacks", &NativeAmbassador::evoke_multiple_callbacks)
      .def("enable_callbacks", &NativeAmbassador::enable_callbacks)
      .def("disable_callbacks", &NativeAmbassador::disable_callbacks)
      .def("list_federation_executions", &NativeAmbassador::list_federation_executions)
      .def("list_federation_execution_members", &NativeAmbassador::list_federation_execution_members)
      .def("join_federation_execution", &NativeAmbassador::join_federation_execution)
      .def("join_federation_execution_with_modules", &NativeAmbassador::join_federation_execution_with_modules)
      .def("resign_federation_execution", &NativeAmbassador::resign_federation_execution)
      .def("register_federation_synchronization_point", &NativeAmbassador::register_federation_synchronization_point)
      .def("register_federation_synchronization_point_with_set", &NativeAmbassador::register_federation_synchronization_point_with_set)
      .def("synchronization_point_achieved", &NativeAmbassador::synchronization_point_achieved)
      .def("query_federation_save_status", &NativeAmbassador::query_federation_save_status)
      .def("request_federation_save", &NativeAmbassador::request_federation_save)
      .def("request_federation_save_with_time", &NativeAmbassador::request_federation_save_with_time)
      .def("federate_save_begun", &NativeAmbassador::federate_save_begun)
      .def("federate_save_complete", &NativeAmbassador::federate_save_complete)
      .def("federate_save_not_complete", &NativeAmbassador::federate_save_not_complete)
      .def("abort_federation_save", &NativeAmbassador::abort_federation_save)
      .def("query_federation_restore_status", &NativeAmbassador::query_federation_restore_status)
      .def("request_federation_restore", &NativeAmbassador::request_federation_restore)
      .def("federate_restore_complete", &NativeAmbassador::federate_restore_complete)
      .def("federate_restore_not_complete", &NativeAmbassador::federate_restore_not_complete)
      .def("abort_federation_restore", &NativeAmbassador::abort_federation_restore)
      .def("publish_object_class_attributes", &NativeAmbassador::publish_object_class_attributes)
      .def("unpublish_object_class", &NativeAmbassador::unpublish_object_class)
      .def("unpublish_object_class_attributes", &NativeAmbassador::unpublish_object_class_attributes)
      .def("publish_object_class_directed_interactions", &NativeAmbassador::publish_object_class_directed_interactions)
      .def("unpublish_object_class_directed_interactions", &NativeAmbassador::unpublish_object_class_directed_interactions)
      .def("subscribe_object_class_attributes", &NativeAmbassador::subscribe_object_class_attributes)
      .def("subscribe_object_class_attributes_with_regions", &NativeAmbassador::subscribe_object_class_attributes_with_regions)
      .def("subscribe_object_class_directed_interactions", &NativeAmbassador::subscribe_object_class_directed_interactions)
      .def("unsubscribe_object_class", &NativeAmbassador::unsubscribe_object_class)
      .def("unsubscribe_object_class_attributes", &NativeAmbassador::unsubscribe_object_class_attributes)
      .def("unsubscribe_object_class_attributes_with_regions", &NativeAmbassador::unsubscribe_object_class_attributes_with_regions)
      .def("unsubscribe_object_class_directed_interactions", &NativeAmbassador::unsubscribe_object_class_directed_interactions)
      .def("publish_interaction_class", &NativeAmbassador::publish_interaction_class)
      .def("unpublish_interaction_class", &NativeAmbassador::unpublish_interaction_class)
      .def("subscribe_interaction_class", &NativeAmbassador::subscribe_interaction_class)
      .def("unsubscribe_interaction_class", &NativeAmbassador::unsubscribe_interaction_class)
      .def("reserve_object_instance_name", &NativeAmbassador::reserve_object_instance_name)
      .def("release_object_instance_name", &NativeAmbassador::release_object_instance_name)
      .def("reserve_multiple_object_instance_names", &NativeAmbassador::reserve_multiple_object_instance_names)
      .def("release_multiple_object_instance_names", &NativeAmbassador::release_multiple_object_instance_names)
      .def("register_object_instance", &NativeAmbassador::register_object_instance)
      .def("register_object_instance_with_regions", &NativeAmbassador::register_object_instance_with_regions)
      .def("associate_regions_for_updates", &NativeAmbassador::associate_regions_for_updates)
      .def("unassociate_regions_for_updates", &NativeAmbassador::unassociate_regions_for_updates)
      .def("get_object_instance_handle", &NativeAmbassador::get_object_instance_handle)
      .def("get_object_instance_name", &NativeAmbassador::get_object_instance_name)
      .def("delete_object_instance", &NativeAmbassador::delete_object_instance)
      .def("local_delete_object_instance", &NativeAmbassador::local_delete_object_instance)
      .def("delete_object_instance_with_time", &NativeAmbassador::delete_object_instance_with_time)
      .def("update_attribute_values", &NativeAmbassador::update_attribute_values)
      .def("update_attribute_values_with_time", &NativeAmbassador::update_attribute_values_with_time)
      .def("request_attribute_value_update_for_instance", &NativeAmbassador::request_attribute_value_update_for_instance)
      .def("request_attribute_value_update_for_class", &NativeAmbassador::request_attribute_value_update_for_class)
      .def("request_attribute_value_update_with_regions", &NativeAmbassador::request_attribute_value_update_with_regions)
      .def("change_attribute_order_type", &NativeAmbassador::change_attribute_order_type)
      .def("change_default_attribute_order_type", &NativeAmbassador::change_default_attribute_order_type)
      .def("change_interaction_order_type", &NativeAmbassador::change_interaction_order_type)
      .def("request_attribute_transportation_type_change", &NativeAmbassador::request_attribute_transportation_type_change)
      .def("change_default_attribute_transportation_type", &NativeAmbassador::change_default_attribute_transportation_type)
      .def("query_attribute_transportation_type", &NativeAmbassador::query_attribute_transportation_type)
      .def("request_interaction_transportation_type_change", &NativeAmbassador::request_interaction_transportation_type_change)
      .def("query_interaction_transportation_type", &NativeAmbassador::query_interaction_transportation_type)
      .def("query_attribute_ownership", &NativeAmbassador::query_attribute_ownership)
      .def("is_attribute_owned_by_federate", &NativeAmbassador::is_attribute_owned_by_federate)
      .def("unconditional_attribute_ownership_divestiture", &NativeAmbassador::unconditional_attribute_ownership_divestiture)
      .def("negotiated_attribute_ownership_divestiture", &NativeAmbassador::negotiated_attribute_ownership_divestiture)
      .def("confirm_divestiture", &NativeAmbassador::confirm_divestiture)
      .def("cancel_negotiated_attribute_ownership_divestiture", &NativeAmbassador::cancel_negotiated_attribute_ownership_divestiture)
      .def("attribute_ownership_acquisition", &NativeAmbassador::attribute_ownership_acquisition)
      .def("attribute_ownership_acquisition_if_available", &NativeAmbassador::attribute_ownership_acquisition_if_available)
      .def("cancel_attribute_ownership_acquisition", &NativeAmbassador::cancel_attribute_ownership_acquisition)
      .def("attribute_ownership_release_denied", &NativeAmbassador::attribute_ownership_release_denied)
      .def("attribute_ownership_divestiture_if_wanted", &NativeAmbassador::attribute_ownership_divestiture_if_wanted)
      .def("send_interaction", &NativeAmbassador::send_interaction)
      .def("send_interaction_with_time", &NativeAmbassador::send_interaction_with_time)
      .def("send_directed_interaction", &NativeAmbassador::send_directed_interaction)
      .def("send_directed_interaction_with_time", &NativeAmbassador::send_directed_interaction_with_time)
      .def("subscribe_interaction_class_with_regions", &NativeAmbassador::subscribe_interaction_class_with_regions)
      .def("unsubscribe_interaction_class_with_regions", &NativeAmbassador::unsubscribe_interaction_class_with_regions)
      .def("send_interaction_with_regions", &NativeAmbassador::send_interaction_with_regions)
      .def("send_interaction_with_regions_with_time", &NativeAmbassador::send_interaction_with_regions_with_time)
      .def("retract", &NativeAmbassador::retract)
      .def(
          "decode_message_retraction_handle",
          &NativeAmbassador::decode_message_retraction_handle)
      .def("create_region", &NativeAmbassador::create_region)
      .def("commit_region_modifications", &NativeAmbassador::commit_region_modifications)
      .def("delete_region", &NativeAmbassador::delete_region)
      .def("get_dimension_handle_set", &NativeAmbassador::get_dimension_handle_set)
      .def("get_range_bounds", &NativeAmbassador::get_range_bounds)
      .def("set_range_bounds", &NativeAmbassador::set_range_bounds)
      .def("get_convey_region_designator_sets_switch", &NativeAmbassador::get_convey_region_designator_sets_switch)
      .def("set_convey_region_designator_sets_switch", &NativeAmbassador::set_convey_region_designator_sets_switch)
      .def("get_object_class_relevance_advisory_switch", &NativeAmbassador::get_object_class_relevance_advisory_switch)
      .def("set_object_class_relevance_advisory_switch", &NativeAmbassador::set_object_class_relevance_advisory_switch)
      .def("get_attribute_relevance_advisory_switch", &NativeAmbassador::get_attribute_relevance_advisory_switch)
      .def("set_attribute_relevance_advisory_switch", &NativeAmbassador::set_attribute_relevance_advisory_switch)
      .def("get_attribute_scope_advisory_switch", &NativeAmbassador::get_attribute_scope_advisory_switch)
      .def("set_attribute_scope_advisory_switch", &NativeAmbassador::set_attribute_scope_advisory_switch)
      .def("get_interaction_relevance_advisory_switch", &NativeAmbassador::get_interaction_relevance_advisory_switch)
      .def("set_interaction_relevance_advisory_switch", &NativeAmbassador::set_interaction_relevance_advisory_switch)
      .def("get_automatic_resign_directive", &NativeAmbassador::get_automatic_resign_directive)
      .def("set_automatic_resign_directive", &NativeAmbassador::set_automatic_resign_directive)
      .def("get_service_reporting_switch", &NativeAmbassador::get_service_reporting_switch)
      .def("set_service_reporting_switch", &NativeAmbassador::set_service_reporting_switch)
      .def("get_exception_reporting_switch", &NativeAmbassador::get_exception_reporting_switch)
      .def("set_exception_reporting_switch", &NativeAmbassador::set_exception_reporting_switch)
      .def("get_send_service_reports_to_file_switch", &NativeAmbassador::get_send_service_reports_to_file_switch)
      .def(
          "set_send_service_reports_to_file_switch",
          &NativeAmbassador::set_send_service_reports_to_file_switch)
      .def("hla_version", &NativeAmbassador::hla_version)
      .def("get_auto_provide_switch", &NativeAmbassador::get_auto_provide_switch)
      .def("get_delay_subscription_evaluation_switch", &NativeAmbassador::get_delay_subscription_evaluation_switch)
      .def("get_advisories_use_known_class_switch", &NativeAmbassador::get_advisories_use_known_class_switch)
      .def("get_allow_relaxed_ddm_switch", &NativeAmbassador::get_allow_relaxed_ddm_switch)
      .def("get_non_regulated_grant_switch", &NativeAmbassador::get_non_regulated_grant_switch)
      .def("time_factory_name", &NativeAmbassador::time_factory_name)
      .def("make_initial_time", &NativeAmbassador::make_initial_time)
      .def("make_final_time", &NativeAmbassador::make_final_time)
      .def("make_zero_interval", &NativeAmbassador::make_zero_interval)
      .def("make_epsilon_interval", &NativeAmbassador::make_epsilon_interval)
      .def("make_logical_time", &NativeAmbassador::make_logical_time)
      .def("make_logical_interval", &NativeAmbassador::make_logical_interval)
      .def("decode_logical_time", &NativeAmbassador::decode_logical_time)
      .def("decode_logical_interval", &NativeAmbassador::decode_logical_interval)
      .def("add_logical_time", &NativeAmbassador::add_logical_time)
      .def("subtract_logical_time", &NativeAmbassador::subtract_logical_time)
      .def("difference_logical_time", &NativeAmbassador::difference_logical_time)
      .def("enable_time_regulation", &NativeAmbassador::enable_time_regulation)
      .def("disable_time_regulation", &NativeAmbassador::disable_time_regulation)
      .def("enable_time_constrained", &NativeAmbassador::enable_time_constrained)
      .def("disable_time_constrained", &NativeAmbassador::disable_time_constrained)
      .def("enable_asynchronous_delivery", &NativeAmbassador::enable_asynchronous_delivery)
      .def("disable_asynchronous_delivery", &NativeAmbassador::disable_asynchronous_delivery)
      .def("modify_lookahead", &NativeAmbassador::modify_lookahead)
      .def("query_lookahead", &NativeAmbassador::query_lookahead)
      .def("time_advance_request", &NativeAmbassador::time_advance_request)
      .def("time_advance_request_available", &NativeAmbassador::time_advance_request_available)
      .def("next_message_request", &NativeAmbassador::next_message_request)
      .def("next_message_request_available", &NativeAmbassador::next_message_request_available)
      .def("flush_queue_request", &NativeAmbassador::flush_queue_request)
      .def("query_logical_time", &NativeAmbassador::query_logical_time)
      .def("query_galt", &NativeAmbassador::query_galt)
      .def("query_lits", &NativeAmbassador::query_lits)
      .def("decode_federate_handle", &NativeAmbassador::decode_federate_handle)
      .def("decode_object_class_handle", &NativeAmbassador::decode_object_class_handle)
      .def("decode_object_instance_handle", &NativeAmbassador::decode_object_instance_handle)
      .def("decode_attribute_handle", &NativeAmbassador::decode_attribute_handle)
      .def("decode_interaction_class_handle", &NativeAmbassador::decode_interaction_class_handle)
      .def("decode_parameter_handle", &NativeAmbassador::decode_parameter_handle)
      .def(
          "decode_transportation_type_handle",
          &NativeAmbassador::decode_transportation_type_handle)
      .def("decode_dimension_handle", &NativeAmbassador::decode_dimension_handle)
      .def("decode_region_handle", &NativeAmbassador::decode_region_handle)
      .def("get_object_class_handle", &NativeAmbassador::get_object_class_handle)
      .def("get_object_class_name", &NativeAmbassador::get_object_class_name)
      .def("get_federate_handle", &NativeAmbassador::get_federate_handle)
      .def("get_federate_name", &NativeAmbassador::get_federate_name)
      .def("get_known_object_class_handle", &NativeAmbassador::get_known_object_class_handle)
      .def("get_attribute_handle", &NativeAmbassador::get_attribute_handle)
      .def("get_attribute_name", &NativeAmbassador::get_attribute_name)
      .def("get_update_rate_value", &NativeAmbassador::get_update_rate_value)
      .def("get_update_rate_value_for_attribute", &NativeAmbassador::get_update_rate_value_for_attribute)
      .def("get_interaction_class_handle", &NativeAmbassador::get_interaction_class_handle)
      .def("get_interaction_class_name", &NativeAmbassador::get_interaction_class_name)
      .def("get_parameter_handle", &NativeAmbassador::get_parameter_handle)
      .def("get_parameter_name", &NativeAmbassador::get_parameter_name)
      .def("get_order_type", &NativeAmbassador::get_order_type)
      .def("get_order_name", &NativeAmbassador::get_order_name)
      .def("get_transportation_type_handle", &NativeAmbassador::get_transportation_type_handle)
      .def("get_transportation_type_name", &NativeAmbassador::get_transportation_type_name)
      .def("get_dimension_handle", &NativeAmbassador::get_dimension_handle)
      .def("get_dimension_name", &NativeAmbassador::get_dimension_name)
      .def("get_available_dimensions_for_object_class", &NativeAmbassador::get_available_dimensions_for_object_class)
      .def("get_available_dimensions_for_interaction_class", &NativeAmbassador::get_available_dimensions_for_interaction_class)
      .def("get_dimension_upper_bound", &NativeAmbassador::get_dimension_upper_bound)
      .def("normalize_service_group", &NativeAmbassador::normalize_service_group)
      .def("normalize_federate_handle", &NativeAmbassador::normalize_federate_handle)
      .def("normalize_object_class_handle", &NativeAmbassador::normalize_object_class_handle)
      .def("normalize_interaction_class_handle", &NativeAmbassador::normalize_interaction_class_handle)
      .def("normalize_object_instance_handle", &NativeAmbassador::normalize_object_instance_handle)
      .def("create_federation_execution", &NativeAmbassador::create_federation_execution)
      .def("create_federation_execution_with_modules", &NativeAmbassador::create_federation_execution_with_modules)
      .def("create_federation_execution_with_mim", &NativeAmbassador::create_federation_execution_with_mim)
      .def("destroy_federation_execution", &NativeAmbassador::destroy_federation_execution);

  module.def("rti_name", [] { return utf8(rti::rtiName()); });
  module.def("rti_version", [] { return utf8(rti::rtiVersion()); });
}
