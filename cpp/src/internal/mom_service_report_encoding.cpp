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
