#include "internal/mom_service_report_encoding.hpp"

#include <RTI/encoding/BasicDataElements.h>

#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <limits>
#include <vector>

namespace umbra::detail {
namespace {

using rti1516_2025::Octet;
using rti1516_2025::VariableLengthData;

void appendPadding(std::vector<Octet>& output, std::size_t boundary) {
  auto const remainder = output.size() % boundary;
  if (remainder != 0U) {
    output.insert(output.end(), boundary - remainder, static_cast<Octet>(0));
  }
}

void appendInt16BE(std::vector<Octet>& output, std::uint16_t value) {
  output.push_back(static_cast<Octet>((value >> 8U) & 0xffU));
  output.push_back(static_cast<Octet>(value & 0xffU));
}

void appendDataElement(std::vector<Octet>& output, rti1516_2025::DataElement const& value) {
  appendPadding(output, value.getOctetBoundary());
  value.encodeInto(output);
}

VariableLengthData toVariableLengthData(std::vector<Octet> const& bytes) {
  return VariableLengthData(bytes.data(), bytes.size());
}

std::wstring formatMomStringArray(std::vector<std::wstring> const& values) {
  std::wstring result = L"[";
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index != 0U) {
      result.push_back(L',');
    }
    result += formatMomString(values[index]);
  }
  result.push_back(L']');
  return result;
}

std::wstring formatMomStringPairList(
    std::vector<std::pair<std::wstring, std::wstring>> const& values) {
  std::wstring result = L"{";
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index != 0U) {
      result.push_back(L',');
    }
    result += formatMomString(values[index].first);
    result.push_back(L':');
    result += formatMomString(values[index].second);
  }
  result.push_back(L'}');
  return result;
}

std::wstring formatMomServiceArgumentList(
    std::vector<MomServiceArgument> const& arguments) {
  std::wstring result = L"[";
  for (std::size_t index = 0; index < arguments.size(); ++index) {
    if (index != 0U) {
      result.push_back(L',');
    }
    result += formatMomServiceArgumentRecord(arguments[index]);
  }
  result.push_back(L']');
  return result;
}

}  // namespace

std::wstring formatMomNull() {
  return L"null";
}

std::wstring formatMomBoolean(bool value) {
  return value ? L"true" : L"false";
}

std::wstring formatMomNumber(std::wstring const& value) {
  if (value.empty()) {
    throw rti1516_2025::EncoderException(L"A MOM Number must contain at least one digit.");
  }
  std::size_t index = value.front() == L'-' ? 1U : 0U;
  auto const wholeStart = index;
  while (index < value.size() && std::iswdigit(value[index]) != 0) {
    ++index;
  }
  if (index == wholeStart) {
    throw rti1516_2025::EncoderException(L"A MOM Number must contain digits before an optional decimal part.");
  }
  if (index < value.size() && value[index] == L'.') {
    auto const decimalStart = ++index;
    while (index < value.size() && std::iswdigit(value[index]) != 0) {
      ++index;
    }
    if (index == decimalStart) {
      throw rti1516_2025::EncoderException(L"A MOM Number decimal part must contain at least one digit.");
    }
  }
  if (index != value.size()) {
    throw rti1516_2025::EncoderException(L"A MOM Number contains an invalid character.");
  }
  return value;
}

std::wstring formatMomString(std::wstring const& value) {
  std::wstring result;
  result.reserve(value.size() + 2U);
  result.push_back(L'\"');
  for (wchar_t const character : value) {
    switch (character) {
      case L'\'': result += L"\\'"; break;
      case L'\"': result += L"\\\""; break;
      case L'\\': result += L"\\\\"; break;
      case L'\n': result += L"\\n"; break;
      case L'\r': result += L"\\r"; break;
      case L'\t': result += L"\\t"; break;
      default: result.push_back(character); break;
    }
  }
  result.push_back(L'\"');
  return result;
}

std::wstring formatMomStringSet(std::set<std::wstring> const& values) {
  std::vector<std::wstring> orderedValues;
  orderedValues.reserve(values.size());
  for (auto const& value : values) {
    orderedValues.push_back(value);
  }
  return formatMomStringArray(orderedValues);
}

std::wstring formatMomLogicalTime(rti1516_2025::LogicalTime const& value) {
  // Table 5 makes this value a JSON-like String containing time.toString().
  // The standard API deliberately leaves that diagnostic representation to the
  // selected logical-time implementation, so preserve it verbatim.
  return formatMomString(value.toString());
}

std::wstring formatMomLogicalTimeInterval(
    rti1516_2025::LogicalTimeInterval const& value) {
  // Table 5 makes this value a JSON-like String containing interval.toString().
  // The standard API deliberately leaves that diagnostic representation to the
  // selected logical-time implementation, so preserve it verbatim.
  return formatMomString(value.toString());
}

std::wstring formatMomFederateHandle(rti1516_2025::FederateHandle const& value) {
  // Table 5 describes this as String(handle.toString()). Preserve the public
  // binding's diagnostic text rather than assuming a numeric identity or the
  // visual spelling of an illustrative table example.
  return formatMomString(value.toString());
}

std::wstring formatMomDimensionHandle(rti1516_2025::DimensionHandle const& value) {
  // Table 5 describes this as String(handle.toString()). Preserve the public
  // binding's diagnostic text rather than assuming a numeric identity.
  return formatMomString(value.toString());
}

std::wstring formatMomFederateHandleSet(
    rti1516_2025::FederateHandleSet const& values) {
  // Table 5 names this Array<FederateHandle>. Table 4 defines arrays as
  // bracketed, comma-separated basic elements, and FederateHandle itself is
  // a quoted handle.toString() string. The official C++ set has deterministic
  // handle order, so retain that order rather than inventing a report-only
  // sorting policy.
  std::wstring result = L"[";
  for (auto iterator = values.begin(); iterator != values.end(); ++iterator) {
    if (iterator != values.begin()) {
      result.push_back(L',');
    }
    result += formatMomString(iterator->toString());
  }
  result.push_back(L']');
  return result;
}

std::wstring formatMomFederateHandleSaveStatusPairVector(
    rti1516_2025::FederateHandleSaveStatusPairVector const& values) {
  // Table 5 calls this Array<FederateHandleSaveStatusPair>. Each pair is the
  // JSON-like Record<handle:FederateHandle, status:SaveStatus> form, not a
  // generic C++ pair or an implementation-defined status number.
  std::wstring result = L"[";
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index != 0U) {
      result.push_back(L',');
    }
    auto const& [handle, status] = values[index];
    result += L"{\"handle\":" + formatMomFederateHandle(handle) +
        L",\"status\":" + formatMomSaveStatus(status) + L"}";
  }
  result.push_back(L']');
  return result;
}

std::wstring formatMomFederateRestoreStatusVector(
    rti1516_2025::FederateRestoreStatusVector const& values) {
  // The official MIM's type-20 name is FederateRestoreStatusSet. Table 5's
  // matching collection row misnames the collection and its element, but its
  // preceding singular row gives the usable record field names and values.
  // Preserve public vector order rather than adding an RTI-local reporting
  // sort. See RL-083.
  std::wstring result = L"[";
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index != 0U) {
      result.push_back(L',');
    }
    auto const& value = values[index];
    result += L"{\"preRestoreHandle\":" + formatMomFederateHandle(value.preRestoreHandle) +
        L",\"postRestoreHandle\":" + formatMomFederateHandle(value.postRestoreHandle) +
        L",\"status\":" + formatMomRestoreStatus(value.status) + L"}";
  }
  result.push_back(L']');
  return result;
}

std::wstring formatMomInteractionClassHandle(
    rti1516_2025::InteractionClassHandle const& value) {
  // Table 5 describes this as String(handle.toString()). Preserve the public
  // binding's diagnostic text rather than assuming a numeric identity or the
  // visual spelling of an illustrative table example.
  return formatMomString(value.toString());
}

std::wstring formatMomInteractionClassHandleSet(
    rti1516_2025::InteractionClassHandleSet const& values) {
  // Table 5 names this Array<InteractionClassHandle>; Table 4 defines arrays
  // as bracketed, comma-separated basic elements. InteractionClassHandle
  // itself is a quoted handle.toString() string. The official C++
  // InteractionClassHandleSet is a std::set, so iterating it retains the
  // binding's deterministic handle order without introducing a competing
  // report-only ordering policy.
  std::wstring result = L"[";
  for (auto iterator = values.begin(); iterator != values.end(); ++iterator) {
    if (iterator != values.begin()) {
      result.push_back(L',');
    }
    result += formatMomString(iterator->toString());
  }
  result.push_back(L']');
  return result;
}

std::wstring formatMomObjectClassHandle(
    rti1516_2025::ObjectClassHandle const& value) {
  // Table 5 gives this the same String(handle.toString()) rule as the
  // interaction-class designator. Preserve the public binding's diagnostic
  // text rather than assuming a numeric identity or illustrative spelling.
  return formatMomString(value.toString());
}

std::wstring formatMomObjectInstanceHandle(
    rti1516_2025::ObjectInstanceHandle const& value) {
  // Table 5 gives this the same String(handle.toString()) rule as the
  // interaction-class designator. Preserve the public binding's diagnostic
  // text rather than assuming a numeric identity or illustrative spelling.
  return formatMomString(value.toString());
}

std::wstring formatMomAttributeHandle(rti1516_2025::AttributeHandle const& value) {
  // Table 5 gives this the same String(handle.toString()) rule as the other
  // ordinary handle forms. It is deliberately distinct from the array wrapper
  // used by AttributeHandleSet, because a service can supply one attribute.
  return formatMomString(value.toString());
}

std::wstring formatMomAttributeHandleSet(
    rti1516_2025::AttributeHandleSet const& values) {
  // Table 5 names this Array<AttributeHandle>; Table 4 defines arrays as
  // bracketed, comma-separated basic elements. AttributeHandle itself is a
  // quoted handle.toString() string. The official C++ AttributeHandleSet is a
  // std::set, so iterating it retains the binding's deterministic handle order
  // without introducing a competing report-only ordering policy.
  std::wstring result = L"[";
  for (auto iterator = values.begin(); iterator != values.end(); ++iterator) {
    if (iterator != values.begin()) {
      result.push_back(L',');
    }
    result += formatMomString(iterator->toString());
  }
  result.push_back(L']');
  return result;
}

std::wstring formatMomRegionHandle(rti1516_2025::RegionHandle const& value) {
  // Table 5 describes this as String(handle.toString()). Preserve the public
  // binding's diagnostic text rather than assuming a numeric identity.
  return formatMomString(value.toString());
}

std::wstring formatMomRegionHandleSet(
    rti1516_2025::RegionHandleSet const& values) {
  // Table 5 names this Array<RegionHandle>; Table 4 defines arrays as
  // bracketed, comma-separated basic elements. RegionHandle itself is a
  // quoted handle.toString() string, and the official C++ set has stable
  // handle order.
  std::wstring result = L"[";
  for (auto iterator = values.begin(); iterator != values.end(); ++iterator) {
    if (iterator != values.begin()) {
      result.push_back(L',');
    }
    result += formatMomString(iterator->toString());
  }
  result.push_back(L']');
  return result;
}

std::wstring formatMomAttributeSetRegionSetPairList(
    rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const& values) {
  // Table 5's AttributeRegionAssociationList is the file-text spelling for
  // the MIM type-4 AttributeSetRegionSetPairList. Preserve the official C++
  // vector order, and use each native set's deterministic handle order.
  std::wstring result = L"[";
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index != 0U) {
      result.push_back(L',');
    }
    auto const& [attributeHandles, regionHandles] = values[index];
    result += L"{\"attributeHandleSet\":" +
        formatMomAttributeHandleSet(attributeHandles) +
        L",\"regionHandleSet\":" +
        formatMomRegionHandleSet(regionHandles) + L"}";
  }
  result.push_back(L']');
  return result;
}

std::wstring formatMomAttributeHandleValueMap(
    rti1516_2025::AttributeHandleValueMap const& values) {
  // Table 5 names this PairList<AttributeHandle:BinaryData>. The official C++
  // map has a deterministic handle order; preserving it gives stable report
  // files without inventing a report-only ordering policy. Each key follows
  // AttributeHandle's quoted handle.toString() form, and each value is Table
  // 5 Binary Data (quoted base-64).
  std::wstring result = L"{";
  for (auto iterator = values.begin(); iterator != values.end(); ++iterator) {
    if (iterator != values.begin()) {
      result.push_back(L',');
    }
    result += formatMomString(iterator->first.toString());
    result.push_back(L':');
    result += formatMomBinaryData(iterator->second);
  }
  result.push_back(L'}');
  return result;
}

std::wstring formatMomParameterHandleValueMap(
    rti1516_2025::ParameterHandleValueMap const& values) {
  // Table 5 names this PairList<ParameterHandle:BinaryData>. The official C++
  // map has a deterministic handle order; preserving it gives stable report
  // files without inventing a report-only ordering policy. Each key follows
  // ParameterHandle's quoted handle.toString() form, and each value is Table
  // 5 Binary Data (quoted base-64).
  std::wstring result = L"{";
  for (auto iterator = values.begin(); iterator != values.end(); ++iterator) {
    if (iterator != values.begin()) {
      result.push_back(L',');
    }
    result += formatMomString(iterator->first.toString());
    result.push_back(L':');
    result += formatMomBinaryData(iterator->second);
  }
  result.push_back(L'}');
  return result;
}

std::wstring formatMomBinaryData(VariableLengthData const& value) {
  // Section 11.5.1 calls this Binary Data: base-64 data delimited with double
  // quotes. Use the conventional base-64 alphabet and canonical padding; the
  // resulting alphabet contains no character that needs the separate String
  // escaping rules.
  static constexpr wchar_t alphabet[] =
      L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  // Octet is the official signed Integer8 alias. Interpret each stored octet
  // as unsigned before building a base-64 group; otherwise a high-bit byte
  // sign-extends and corrupts its preceding sextets.
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  std::wstring result;
  result.reserve(2U + ((value.size() + 2U) / 3U) * 4U);
  result.push_back(L'"');
  for (std::size_t index = 0; index < value.size(); index += 3U) {
    auto const remaining = value.size() - index;
    auto const first = static_cast<std::uint32_t>(bytes[index]);
    auto const second = remaining > 1U ? static_cast<std::uint32_t>(bytes[index + 1U]) : 0U;
    auto const third = remaining > 2U ? static_cast<std::uint32_t>(bytes[index + 2U]) : 0U;
    auto const group = (first << 16U) | (second << 8U) | third;
    result.push_back(alphabet[(group >> 18U) & 0x3fU]);
    result.push_back(alphabet[(group >> 12U) & 0x3fU]);
    result.push_back(remaining > 1U ? alphabet[(group >> 6U) & 0x3fU] : L'=');
    result.push_back(remaining > 2U ? alphabet[group & 0x3fU] : L'=');
  }
  result.push_back(L'"');
  return result;
}

std::wstring formatMomUserSuppliedTag(VariableLengthData const& value) {
  return formatMomBinaryData(value);
}

std::wstring formatMomTransportationTypeHandle(
    rti1516_2025::TransportationTypeHandle const& value) {
  // Table 5 gives this the same String(handle.toString()) rule as the
  // interaction-class designator. The output must not substitute the
  // implementation's public transportation name or MIM wire bytes.
  return formatMomString(value.toString());
}

std::wstring formatMomMessageRetractionHandle(std::uint64_t value) {
  // Table 5's MessageRetractionDesignator entry uses the explicit
  // MessageRetractionHandle<2345> textual form. It is intentionally not the
  // public handle's diagnostic MessageRetractionHandle(2345) spelling.
  return formatMomString(
      L"MessageRetractionHandle<" + std::to_wstring(value) + L">");
}

std::wstring formatMomResignAction(rti1516_2025::ResignAction action) {
  // Table 5's ResignAction row uses the C++ enum spellings, not the distinct
  // HLAresignAction enumerator names used on the MIM wire.
  switch (action) {
    case rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES:
      return formatMomString(L"UNCONDITIONALLY_DIVEST_ATTRIBUTES");
    case rti1516_2025::DELETE_OBJECTS:
      return formatMomString(L"DELETE_OBJECTS");
    case rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS:
      return formatMomString(L"CANCEL_PENDING_OWNERSHIP_ACQUISITIONS");
    case rti1516_2025::DELETE_OBJECTS_THEN_DIVEST:
      return formatMomString(L"DELETE_OBJECTS_THEN_DIVEST");
    case rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST:
      return formatMomString(L"CANCEL_THEN_DELETE_THEN_DIVEST");
    case rti1516_2025::NO_ACTION:
      return formatMomString(L"NO_ACTION");
  }
  throw rti1516_2025::EncoderException(
      L"The supplied ResignAction has no Table 5 textual representation.");
}

std::wstring formatMomSynchronizationPointFailureReason(
    rti1516_2025::SynchronizationPointFailureReason reason) {
  switch (reason) {
    case rti1516_2025::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE:
      return formatMomString(L"SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE");
    case rti1516_2025::SYNCHRONIZATION_SET_MEMBER_NOT_JOINED:
      return formatMomString(L"SYNCHRONIZATION_SET_MEMBER_NOT_JOINED");
  }
  throw rti1516_2025::EncoderException(
      L"The supplied SynchronizationPointFailureReason has no Table 5 textual representation.");
}

std::wstring formatMomSaveFailureReason(rti1516_2025::SaveFailureReason reason) {
  switch (reason) {
    case rti1516_2025::RTI_UNABLE_TO_SAVE:
      return formatMomString(L"RTI_UNABLE_TO_SAVE");
    case rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_SAVE:
      return formatMomString(L"FEDERATE_REPORTED_FAILURE_DURING_SAVE");
    case rti1516_2025::FEDERATE_RESIGNED_DURING_SAVE:
      return formatMomString(L"FEDERATE_RESIGNED_DURING_SAVE");
    case rti1516_2025::RTI_DETECTED_FAILURE_DURING_SAVE:
      return formatMomString(L"RTI_DETECTED_FAILURE_DURING_SAVE");
    case rti1516_2025::SAVE_TIME_CANNOT_BE_HONORED:
      return formatMomString(L"SAVE_TIME_CANNOT_BE_HONORED");
    case rti1516_2025::SAVE_ABORTED:
      return formatMomString(L"SAVE_ABORTED");
  }
  throw rti1516_2025::EncoderException(
      L"The supplied SaveFailureReason has no Table 5 textual representation.");
}

std::wstring formatMomSaveStatus(rti1516_2025::SaveStatus status) {
  switch (status) {
    case rti1516_2025::NO_SAVE_IN_PROGRESS:
      return formatMomString(L"NO_SAVE_IN_PROGRESS");
    case rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE:
      return formatMomString(L"FEDERATE_INSTRUCTED_TO_SAVE");
    case rti1516_2025::FEDERATE_SAVING:
      return formatMomString(L"FEDERATE_SAVING");
    case rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE:
      return formatMomString(L"FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE");
  }
  throw rti1516_2025::EncoderException(
      L"The supplied SaveStatus has no Table 5 textual representation.");
}

std::wstring formatMomRestoreStatus(rti1516_2025::RestoreStatus status) {
  switch (status) {
    case rti1516_2025::NO_RESTORE_IN_PROGRESS:
      return formatMomString(L"NO_RESTORE_IN_PROGRESS");
    case rti1516_2025::FEDERATE_RESTORE_REQUEST_PENDING:
      return formatMomString(L"FEDERATE_RESTORE_REQUEST_PENDING");
    case rti1516_2025::FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN:
      return formatMomString(L"FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN");
    case rti1516_2025::FEDERATE_PREPARED_TO_RESTORE:
      return formatMomString(L"FEDERATE_PREPARED_TO_RESTORE");
    case rti1516_2025::FEDERATE_RESTORING:
      return formatMomString(L"FEDERATE_RESTORING");
    case rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE:
      return formatMomString(L"FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE");
  }
  throw rti1516_2025::EncoderException(
      L"The supplied RestoreStatus has no Table 5 textual representation.");
}

std::wstring formatMomOrderType(rti1516_2025::OrderType orderType) {
  switch (orderType) {
    case rti1516_2025::RECEIVE:
      return formatMomString(L"RECEIVE");
    case rti1516_2025::TIMESTAMP:
      return formatMomString(L"TIMESTAMP");
  }
  throw rti1516_2025::EncoderException(
      L"The supplied OrderType has no Table 5 textual representation.");
}

std::wstring formatMomServiceArgumentRecord(MomServiceArgument const& argument) {
  return L"{\"HLAargumentType\":" +
      std::to_wstring(static_cast<std::int32_t>(argument.type)) +
      L",\"HLAargumentName\":" + formatMomString(argument.name) +
      L",\"HLAargumentValue\":" + argument.value + L"}";
}

std::wstring formatMomServiceReportInitialRecord(
    MomServiceReportInitialRecord const& record) {
  // The member order follows the Table 5 example.  Record members are
  // semantically unordered, but preserving the published order gives log
  // readers stable output and makes independent conformance comparison
  // practical.
  return L"{\"Configuration\":{\"CallbackModel\":" +
      formatMomString(record.callbackModel) +
      L",\"ConfigurationName\":" + formatMomString(record.configurationName) +
      L",\"RTIaddress\":" + formatMomString(record.rtiAddress) +
      L",\"AdditionalSettings\":" + formatMomString(record.additionalSettings) +
      L",\"OptionalInternalData\":" + formatMomStringPairList(record.optionalInternalData) +
      L"},\"HLAmanager.HLAfederation\":{\"HLAfederationName\":" +
      formatMomString(record.federationName) +
      L",\"HLARTIversion\":" + formatMomString(record.rtiVersion) +
      L",\"HLAMIMDesignator\":" + formatMomString(record.mimDesignator) +
      L",\"HLAFOMmoduleDesignatorList\":" +
      formatMomStringArray(record.federationFomModuleDesignators) +
      L",\"HLAtimeImplementationName\":" + formatMomString(record.timeImplementationName) +
      L",\"HLAautoProvide\":" + formatMomBoolean(record.autoProvide) +
      L"},\"HLAmanager.HLAfederate\":{\"HLAfederateHandle\":" +
      formatMomString(record.federateHandle) +
      L",\"HLAfederateName\":" + formatMomString(record.federateName) +
      L",\"HLAfederateType\":" + formatMomString(record.federateType) +
      L",\"HLAfederateHost\":" + formatMomString(record.federateHost) +
      L",\"HLAFOMmoduleDesignatorList\":" +
      formatMomStringArray(record.federateFomModuleDesignators) + L"}}";
}

std::wstring formatMomSuccessfulVoidServiceReportRecord(
    std::uint32_t serialNumber,
    std::wstring const& service,
    std::vector<MomServiceArgument> const& suppliedArguments) {
  // Preserve the member order shown in Table 5.  [null] is deliberately
  // hard-coded here: it is the standard's explicit successful-void form, not
  // an inference about the unresolved ReturnArgument alias (RL-042).
  return L"{\"HLAserialNumber\":" + formatMomNumber(std::to_wstring(serialNumber)) +
      L",\"HLAreturnedArgument\":[null]" +
      L",\"HLAservice\":" + formatMomString(service) +
      L",\"HLAsuppliedArguments\":" + formatMomServiceArgumentList(suppliedArguments) +
      L",\"HLAsuccessIndicator\":true,\"HLAexception\":null}";
}

VariableLengthData encodeMomServiceArgument(MomServiceArgument const& argument) {
  std::vector<Octet> bytes;
  rti1516_2025::HLAinteger32BE type{static_cast<rti1516_2025::Integer32>(argument.type)};
  rti1516_2025::HLAunicodeString name{argument.name};
  rti1516_2025::HLAunicodeString value{argument.value};
  appendDataElement(bytes, type);
  appendDataElement(bytes, name);
  appendDataElement(bytes, value);
  return toVariableLengthData(bytes);
}

VariableLengthData encodeMomServiceArgumentList(std::vector<MomServiceArgument> const& arguments) {
  if (arguments.size() >
      static_cast<std::size_t>(std::numeric_limits<rti1516_2025::Integer32>::max())) {
    throw rti1516_2025::EncoderException(L"The MOM supplied-argument list is too large to encode.");
  }
  std::vector<Octet> bytes;
  rti1516_2025::HLAinteger32BE count{
      static_cast<rti1516_2025::Integer32>(arguments.size())};
  appendDataElement(bytes, count);
  for (auto const& argument : arguments) {
    auto const encoded = encodeMomServiceArgument(argument);
    auto const* data = static_cast<Octet const*>(encoded.data());
    appendPadding(bytes, 4U);
    bytes.insert(bytes.end(), data, data + encoded.size());
  }
  return toVariableLengthData(bytes);
}

EncodedMomServiceInvocation encodeMomServiceInvocation(
    std::wstring const& service,
    MomServiceType serviceType,
    bool success,
    std::vector<MomServiceArgument> const& suppliedArguments,
    MomServiceArgument const& returnedArgument,
    std::wstring const& exception,
    std::int32_t serialNumber) {
  rti1516_2025::HLAunicodeString encodedService{service};
  rti1516_2025::HLAboolean encodedSuccess{success};
  rti1516_2025::HLAinteger32BE encodedSerial{serialNumber};
  std::vector<Octet> serviceTypeBytes;
  appendInt16BE(serviceTypeBytes, static_cast<std::uint16_t>(serviceType));
  return {
      encodedService.encode(),
      toVariableLengthData(serviceTypeBytes),
      encodedSuccess.encode(),
      encodeMomServiceArgumentList(suppliedArguments),
      encodeMomServiceArgument(returnedArgument),
      rti1516_2025::HLAunicodeString{exception}.encode(),
      encodedSerial.encode(),
  };
}

}  // namespace umbra::detail
