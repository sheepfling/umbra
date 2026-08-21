#pragma once

#include <RTI/Enums.h>
#include <RTI/Handle.h>
#include <RTI/Typedefs.h>
#include <RTI/VariableLengthData.h>
#include <RTI/time/LogicalTime.h>
#include <RTI/time/LogicalTimeInterval.h>

#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

// These values are the standard MIM HLAserviceTypeEnum representation.  Keep
// this private model tied to the MIM rather than creating a competing public
// C++ interface type.
enum class MomServiceType : std::uint16_t {
  federation_management = 0,
  declaration_management = 1,
  object_management = 2,
  ownership_management = 3,
  time_management = 4,
  data_distribution_management = 5,
  support_services = 6,
};

// The argument-type values below match the static MIM HLAargumentType
// enumeration except where a Table 5 service-report example gives a conflicting
// literal. The first report slices need AttributeHandle, AttributeHandleSet,
// AttributeHandleValueMap, Null, Boolean, DimensionHandle, FederateHandle, FederateHandleSet,
// AttributeSetRegionSetPairList,
// InteractionClassHandle,
// InteractionClassHandleSet, LogicalTime, LogicalTimeInterval,
// MessageRetractionHandle, Number, ObjectClassHandle, ObjectInstanceHandle,
// OrderType, ParameterHandleValueMap, RegionHandle, RegionHandleSet,
// ServiceGroup, String, StringSet,
// SaveFailureReason, FederateHandleSaveStatusPairSet, FederateRestoreStatusSet,
// SynchronizationPointFailureReason, and
// TransportationTypeHandle; retaining those MIM values keeps future service
// wrappers byte-compatible without conflating these display values with C++
// types.
enum class MomArgumentType : std::int32_t {
  attribute_handle = 0,
  attribute_handle_set = 1,
  attribute_handle_value_map = 2,
  attribute_set_region_set_pair_list = 4,
  boolean = 6,
  dimension_handle = 10,
  federate_handle = 15,
  federate_handle_save_status_pair_set = 17,
  federate_handle_set = 18,
  federate_restore_status_set = 20,
  interaction_class_handle = 27,
  interaction_class_handle_set = 28,
  logical_time = 31,
  logical_time_interval = 32,
  message_retraction_handle = 33,
  null_value = 34,
  number = 35,
  object_class_handle = 36,
  object_instance_handle = 37,
  order_type = 38,
  parameter_handle_value_map = 40,
  region_handle = 42,
  region_handle_set = 43,
  resign_action = 44,
  save_failure_reason = 48,
  service_group = 50,
  string = 53,
  string_set = 54,
  synchronization_point_failure_reason = 56,
  transportation_type_handle = 59,
  // Table 5's UserSuppliedTag service-report example uses 63, while the
  // unmodified 2025 standard MIM assigns UserSuppliedTag value 60. File-report
  // serialization follows the Table 5 literal, but a future emitted MOM
  // interaction must revisit that source conflict rather than treating 63 as a
  // static-MIM value. See RL-077.
  table_5_user_supplied_tag = 63,
};

struct MomServiceArgument {
  MomArgumentType type = MomArgumentType::null_value;
  std::wstring name;
  std::wstring value;
};

// Table 5's ServiceReportInitialRecord has no application-data encoding.
// It is the JSON-like text prefix written exactly once to an RTI-owned report
// file for a joined-federate lifetime.  Keep its source fields explicit so
// the embedded runtime cannot substitute a homegrown log header.
struct MomServiceReportInitialRecord {
  std::wstring callbackModel;
  std::wstring configurationName;
  std::wstring rtiAddress;
  std::wstring additionalSettings;
  std::vector<std::pair<std::wstring, std::wstring>> optionalInternalData;

  std::wstring federationName;
  std::wstring rtiVersion;
  std::wstring mimDesignator;
  std::vector<std::wstring> federationFomModuleDesignators;
  std::wstring timeImplementationName;
  bool autoProvide = false;

  std::wstring federateHandle;
  std::wstring federateName;
  std::wstring federateType;
  std::wstring federateHost;
  std::vector<std::wstring> federateFomModuleDesignators;
};

// Table 5 textual forms used in HLAargumentValue.  These helpers keep the
// JSON-like representation private to MOM reporting; they are not a general
// JSON API or an application data encoding facility.
[[nodiscard]] std::wstring formatMomNull();
[[nodiscard]] std::wstring formatMomBoolean(bool value);
[[nodiscard]] std::wstring formatMomNumber(std::wstring const& value);
[[nodiscard]] std::wstring formatMomString(std::wstring const& value);
// Table 5 specifies StringSet as Array<String>.  The official C++ multiple
// name services use std::set<std::wstring>; preserve that deterministic native
// iteration order and emit the standard bracketed, quoted String elements.
[[nodiscard]] std::wstring formatMomStringSet(
    std::set<std::wstring> const& values);
// Table 5 specifies LogicalTime as a String containing the exact value
// returned by the time's standard toString() operation. Do not substitute a
// numeric encoding: a federation time implementation may choose its own
// diagnostic representation.
[[nodiscard]] std::wstring formatMomLogicalTime(
    rti1516_2025::LogicalTime const& value);
// Table 5 specifies LogicalTimeInterval as a String containing the exact
// value returned by the interval's standard toString() operation. Do not
// substitute a numeric encoding: a federation time implementation may choose
// its own diagnostic representation.
[[nodiscard]] std::wstring formatMomLogicalTimeInterval(
    rti1516_2025::LogicalTimeInterval const& value);
// Table 5 specifies FederateHandle as a String containing the exact value
// returned by the handle's standard toString() operation.
[[nodiscard]] std::wstring formatMomFederateHandle(
    rti1516_2025::FederateHandle const& value);
// Table 5 specifies DimensionHandle as a String containing the exact value
// returned by the handle's standard toString() operation.
[[nodiscard]] std::wstring formatMomDimensionHandle(
    rti1516_2025::DimensionHandle const& value);
// Table 5 specifies FederateHandleSet as an Array<FederateHandle>. The
// public C++ FederateHandleSet is ordered, so its native iteration preserves
// a stable report representation without an RTI-specific ordering rule.
[[nodiscard]] std::wstring formatMomFederateHandleSet(
    rti1516_2025::FederateHandleSet const& values);
// Table 5 calls this collection FederateHandleSaveStatusPairSet and encodes it
// as Array<FederateHandleSaveStatusPair>. The official C++ binding calls the
// same ordered collection FederateHandleSaveStatusPairVector; preserve that
// API iteration order rather than inventing a reporting-only sort.
[[nodiscard]] std::wstring formatMomFederateHandleSaveStatusPairVector(
    rti1516_2025::FederateHandleSaveStatusPairVector const& values);
// The official MIM calls this collection FederateRestoreStatusSet (type 20),
// while the official C++ binding exposes its ordered value as
// FederateRestoreStatusVector. Table 5's collection row has a conflicting
// name and malformed second example; retain the MIM/binding identity and the
// valid singular-record field names. See RL-083.
[[nodiscard]] std::wstring formatMomFederateRestoreStatusVector(
    rti1516_2025::FederateRestoreStatusVector const& values);
// Table 5 specifies InteractionClassHandle as a String containing the exact
// value returned by the handle's standard toString() operation. This differs
// from the separate MessageRetractionHandle example below, whose Table 5 row
// depicts an explicit angle-bracket form.
[[nodiscard]] std::wstring formatMomInteractionClassHandle(
    rti1516_2025::InteractionClassHandle const& value);
// Table 5 specifies InteractionClassHandleSet as
// Array<InteractionClassHandle>. Table 4 defines that form as a bracketed
// comma-separated array, while Table 5 defines each InteractionClassHandle
// element as its quoted handle.toString() value.
[[nodiscard]] std::wstring formatMomInteractionClassHandleSet(
    rti1516_2025::InteractionClassHandleSet const& values);
// Table 5 specifies ObjectClassHandle as a String containing the exact value
// returned by the handle's standard toString() operation.
[[nodiscard]] std::wstring formatMomObjectClassHandle(
    rti1516_2025::ObjectClassHandle const& value);
// Table 5 specifies ObjectInstanceHandle as a String containing the exact
// value returned by the handle's standard toString() operation.
[[nodiscard]] std::wstring formatMomObjectInstanceHandle(
    rti1516_2025::ObjectInstanceHandle const& value);
// Table 5 specifies AttributeHandle as a String containing the exact value
// returned by the handle's standard toString() operation.
[[nodiscard]] std::wstring formatMomAttributeHandle(
    rti1516_2025::AttributeHandle const& value);
// Table 5 specifies AttributeHandleSet as Array<AttributeHandle>. Table 4
// defines that form as a bracketed comma-separated array, while Table 5
// defines each AttributeHandle element as its quoted handle.toString() value.
[[nodiscard]] std::wstring formatMomAttributeHandleSet(
    rti1516_2025::AttributeHandleSet const& values);
// Table 5 specifies RegionHandle as a String containing the exact value
// returned by the handle's standard toString() operation.
[[nodiscard]] std::wstring formatMomRegionHandle(
    rti1516_2025::RegionHandle const& value);
// Table 5 specifies RegionHandleSet as Array<RegionHandle>. The official
// C++ RegionHandleSet is ordered, so preserve its iteration order and the
// public RegionHandle::toString() spelling.
[[nodiscard]] std::wstring formatMomRegionHandleSet(
    rti1516_2025::RegionHandleSet const& values);
// Table 5 calls this AttributeRegionAssociationList and encodes it as an
// Array of records containing an AttributeHandleSet and RegionHandleSet. The
// MIM's corresponding type-4 name is AttributeSetRegionSetPairList, matching
// the official C++ vector used by the regional object services.
[[nodiscard]] std::wstring formatMomAttributeSetRegionSetPairList(
    rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const& values);
// Table 5 specifies AttributeHandleValueMap as
// PairList<AttributeHandle:BinaryData>. Table 4 defines a PairList as a
// brace-delimited collection whose keys and values are separated by colons;
// AttributeHandle and Binary Data respectively use the quoted handle.toString
// and base-64 forms below.
[[nodiscard]] std::wstring formatMomAttributeHandleValueMap(
    rti1516_2025::AttributeHandleValueMap const& values);
// Table 5 specifies ParameterHandleValueMap as
// PairList<ParameterHandle:BinaryData>. Table 4 defines a PairList as a
// brace-delimited collection whose keys and values are separated by colons;
// ParameterHandle and Binary Data respectively use the quoted
// handle.toString and base-64 forms below.
[[nodiscard]] std::wstring formatMomParameterHandleValueMap(
    rti1516_2025::ParameterHandleValueMap const& values);
// Section 11.5.1 defines Binary Data as double-quoted base-64 data. This is a
// service-report text formatter, not an application-data encoding or a
// replacement for VariableLengthData.
[[nodiscard]] std::wstring formatMomBinaryData(
    rti1516_2025::VariableLengthData const& value);
// Table 5 gives UserSuppliedTag the Binary Data form. Retain this descriptive
// wrapper at the call sites whose argument is specifically a user-supplied
// tag, rather than conflating its special Table 5 type literal with the value
// encoding shared by ordinary binary-data arguments.
[[nodiscard]] std::wstring formatMomUserSuppliedTag(
    rti1516_2025::VariableLengthData const& value);
// Table 5 specifies TransportationTypeHandle as a String containing the exact
// value returned by the handle's standard toString() operation.
[[nodiscard]] std::wstring formatMomTransportationTypeHandle(
    rti1516_2025::TransportationTypeHandle const& value);
// Table 5 depicts a MessageRetractionHandle as
// MessageRetractionHandle<decimal-identity>, not the implementation's general
// diagnostic toString() form. Keep the source-backed report representation
// separate from the public handle's presentation method.
[[nodiscard]] std::wstring formatMomMessageRetractionHandle(std::uint64_t value);
// Table 5 explicitly renders ResignAction as the C++ enum spelling inside a
// JSON-like String. Keep this closed mapping next to the other Table 5 forms
// so successful-void report wrappers cannot substitute MIM wire enumerators.
[[nodiscard]] std::wstring formatMomResignAction(
    rti1516_2025::ResignAction action);
// Table 5 renders SynchronizationPointFailureReason as the quoted public
// enumeration spelling. Keep the closed mapping with the other Table 5 enum
// helpers so callback reporting cannot substitute a private numeric value.
[[nodiscard]] std::wstring formatMomSynchronizationPointFailureReason(
    rti1516_2025::SynchronizationPointFailureReason reason);
// Table 5 renders SaveFailureReason as the quoted public enumeration spelling.
// Keep the closed mapping separate from the callback adapter so a report does
// not accidentally emit a numeric or implementation-local value.
[[nodiscard]] std::wstring formatMomSaveFailureReason(
    rti1516_2025::SaveFailureReason reason);
// Table 5 renders SaveStatus as the quoted public enumeration spelling. Keep
// the closed mapping separate from the status-pair formatter so no numeric or
// implementation-local value can enter a service report.
[[nodiscard]] std::wstring formatMomSaveStatus(rti1516_2025::SaveStatus status);
// The official C++ RestoreStatus enumeration supplies the Table 5 quoted
// public spelling inside each FederateRestoreStatus record.
[[nodiscard]] std::wstring formatMomRestoreStatus(rti1516_2025::RestoreStatus status);
// Table 5 renders OrderType as the quoted MIM spelling RECEIVE or TIMESTAMP.
// Keep this closed mapping separate from the MIM wire enumeration so report
// writers cannot accidentally emit the numeric representation.
[[nodiscard]] std::wstring formatMomOrderType(rti1516_2025::OrderType orderType);
// The HLAargument record is the textual depiction used alongside the exact
// MIM interaction encoding. It deliberately remains distinct from Table 5's
// ServiceReportRecord log form: that table's ReturnArgument alias is not
// declared, so a generic file-record renderer would be an unsourced local
// convention (RL-042).
[[nodiscard]] std::wstring formatMomServiceArgumentRecord(
    MomServiceArgument const& argument);
[[nodiscard]] std::wstring formatMomServiceReportInitialRecord(
    MomServiceReportInitialRecord const& record);
// Table 5 explicitly gives [null] as the returned-argument representation
// for a successful void service.  This intentionally narrow formatter is the
// only emitted ServiceReportRecord form until the general ReturnArgument
// mapping is sourced (RL-042); it must not be widened into a generic result
// or exception formatter by convention.
[[nodiscard]] std::wstring formatMomSuccessfulVoidServiceReportRecord(
    std::uint32_t serialNumber,
    std::wstring const& service,
    std::vector<MomServiceArgument> const& suppliedArguments);

struct EncodedMomServiceInvocation {
  rti1516_2025::VariableLengthData service;
  rti1516_2025::VariableLengthData serviceType;
  rti1516_2025::VariableLengthData successIndicator;
  rti1516_2025::VariableLengthData suppliedArguments;
  rti1516_2025::VariableLengthData returnedArgument;
  rti1516_2025::VariableLengthData exception;
  rti1516_2025::VariableLengthData serialNumber;
};

[[nodiscard]] rti1516_2025::VariableLengthData encodeMomServiceArgument(
    MomServiceArgument const& argument);

[[nodiscard]] rti1516_2025::VariableLengthData encodeMomServiceArgumentList(
    std::vector<MomServiceArgument> const& arguments);

[[nodiscard]] EncodedMomServiceInvocation encodeMomServiceInvocation(
    std::wstring const& service,
    MomServiceType serviceType,
    bool success,
    std::vector<MomServiceArgument> const& suppliedArguments,
    MomServiceArgument const& returnedArgument,
    std::wstring const& exception,
    std::int32_t serialNumber);

}  // namespace umbra::detail
