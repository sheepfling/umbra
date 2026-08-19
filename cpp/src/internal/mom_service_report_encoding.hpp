#pragma once

#include <RTI/VariableLengthData.h>

#include <cstdint>
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

// The argument-type values are the static MIM HLAargumentType enumeration.
// The first report slice needs Null, Boolean, Number, ServiceGroup, and
// String; retaining the MIM values here keeps future service wrappers
// byte-compatible without conflating these display values with C++ types.
enum class MomArgumentType : std::int32_t {
  boolean = 6,
  null_value = 34,
  number = 35,
  service_group = 50,
  string = 53,
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
// The HLAargument record is the textual depiction used alongside the exact
// MIM interaction encoding. It deliberately remains distinct from Table 5's
// ServiceReportRecord log form: that table's ReturnArgument alias is not
// declared, so a generic file-record renderer would be an unsourced local
// convention (RL-042).
[[nodiscard]] std::wstring formatMomServiceArgumentRecord(
    MomServiceArgument const& argument);
[[nodiscard]] std::wstring formatMomServiceReportInitialRecord(
    MomServiceReportInitialRecord const& record);

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
