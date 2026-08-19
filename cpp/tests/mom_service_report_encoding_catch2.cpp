#include <catch2/catch_test_macros.hpp>

#include "internal/mom_service_report_encoding.hpp"

#include <RTI/encoding/BasicDataElements.h>

#include <initializer_list>
#include <vector>

namespace {

std::vector<rti1516_2025::Octet> octets(rti1516_2025::VariableLengthData const& value) {
  auto const* data = static_cast<rti1516_2025::Octet const*>(value.data());
  return data == nullptr ? std::vector<rti1516_2025::Octet>{}
                         : std::vector<rti1516_2025::Octet>(data, data + value.size());
}

std::vector<rti1516_2025::Octet> byteValues(std::initializer_list<unsigned int> values) {
  std::vector<rti1516_2025::Octet> result;
  result.reserve(values.size());
  for (auto const value : values) {
    result.push_back(static_cast<rti1516_2025::Octet>(value));
  }
  return result;
}

}  // namespace

TEST_CASE("MOM service-report records use the standard MIM fixed-record layout", "[mom][encoding]") {
  using namespace umbra::detail;

  MomServiceArgument argument{MomArgumentType::number, L"n", L"7"};
  REQUIRE(octets(encodeMomServiceArgument(argument)) == byteValues({
      0U, 0U, 0U, 35U,
      0U, 0U, 0U, 1U, 0U, 0x6eU,
      0U, 0U,
      0U, 0U, 0U, 1U, 0U, 0x37U,
  }));

  REQUIRE(octets(encodeMomServiceArgumentList({argument})) == byteValues({
      0U, 0U, 0U, 1U,
      0U, 0U, 0U, 35U,
      0U, 0U, 0U, 1U, 0U, 0x6eU,
      0U, 0U,
      0U, 0U, 0U, 1U, 0U, 0x37U,
  }));

  MomServiceArgument secondArgument{MomArgumentType::number, L"m", L"8"};
  REQUIRE(octets(encodeMomServiceArgumentList({argument, secondArgument})) == byteValues({
      0U, 0U, 0U, 2U,
      0U, 0U, 0U, 35U,
      0U, 0U, 0U, 1U, 0U, 0x6eU,
      0U, 0U,
      0U, 0U, 0U, 1U, 0U, 0x37U,
      0U, 0U,
      0U, 0U, 0U, 35U,
      0U, 0U, 0U, 1U, 0U, 0x6dU,
      0U, 0U,
      0U, 0U, 0U, 1U, 0U, 0x38U,
  }));
}

TEST_CASE("MOM service-report parameters carry their declared MIM encodings", "[mom][encoding]") {
  using namespace umbra::detail;

  auto const report = encodeMomServiceInvocation(
      L"NormalizeServiceGroup",
      MomServiceType::support_services,
      true,
      {},
      {MomArgumentType::service_group, L"return", L"SupportServices"},
      L"",
      0);

  REQUIRE(octets(report.serviceType) == byteValues({0U, 6U}));
  REQUIRE(octets(report.successIndicator) == byteValues({0U, 0U, 0U, 1U}));
  REQUIRE(octets(report.suppliedArguments) == byteValues({0U, 0U, 0U, 0U}));
  REQUIRE(octets(report.serialNumber) == byteValues({0U, 0U, 0U, 0U}));
  REQUIRE(octets(report.exception) == byteValues({0U, 0U, 0U, 0U}));
}

TEST_CASE("MOM service-report argument text follows the Table 5 JSON-like primitives", "[mom][encoding]") {
  using namespace umbra::detail;

  REQUIRE(formatMomNull() == L"null");
  REQUIRE(formatMomBoolean(true) == L"true");
  REQUIRE(formatMomBoolean(false) == L"false");
  REQUIRE(formatMomNumber(L"-12.50") == L"-12.50");
  REQUIRE_THROWS(formatMomNumber(L"12."));
  REQUIRE_THROWS(formatMomNumber(L"one"));
  REQUIRE(formatMomString(L"a'\"\\\n\r\t") == L"\"a\\'\\\"\\\\\\n\\r\\t\"");
}

TEST_CASE("MOM service-report files begin with the Table 5 initial record", "[mom][encoding]") {
  using namespace umbra::detail;

  MomServiceReportInitialRecord record;
  record.callbackModel = L"HLA_EVOKED";
  record.configurationName = L"embedded";
  record.rtiAddress = L"in-process";
  record.additionalSettings = L"serviceReportDirectory=C:\\logs";
  record.optionalInternalData = {{L"Process ID", L"42"}};
  record.federationName = L"TestDogFight";
  record.rtiVersion = L"Umbra 0.1.0";
  record.mimDesignator = L"HLAstandardMIM";
  record.federationFomModuleDesignators = {L"Restaurant.xml", L"Extensions.xml"};
  record.timeImplementationName = L"HLAfloat64Time";
  record.autoProvide = true;
  record.federateHandle = L"FederateHandle(21)";
  record.federateName = L"Fighter1";
  record.federateType = L"Fighter";
  record.federateHost = L"umbra-embedded";
  record.federateFomModuleDesignators = record.federationFomModuleDesignators;

  REQUIRE(formatMomServiceReportInitialRecord(record) ==
      L"{\"Configuration\":{\"CallbackModel\":\"HLA_EVOKED\",\"ConfigurationName\":\"embedded\",\"RTIaddress\":\"in-process\",\"AdditionalSettings\":\"serviceReportDirectory=C:\\\\logs\",\"OptionalInternalData\":{\"Process ID\":\"42\"}},\"HLAmanager.HLAfederation\":{\"HLAfederationName\":\"TestDogFight\",\"HLARTIversion\":\"Umbra 0.1.0\",\"HLAMIMDesignator\":\"HLAstandardMIM\",\"HLAFOMmoduleDesignatorList\":[\"Restaurant.xml\",\"Extensions.xml\"],\"HLAtimeImplementationName\":\"HLAfloat64Time\",\"HLAautoProvide\":true},\"HLAmanager.HLAfederate\":{\"HLAfederateHandle\":\"FederateHandle(21)\",\"HLAfederateName\":\"Fighter1\",\"HLAfederateType\":\"Fighter\",\"HLAfederateHost\":\"umbra-embedded\",\"HLAFOMmoduleDesignatorList\":[\"Restaurant.xml\",\"Extensions.xml\"]}}");
}

TEST_CASE("MOM service-report interaction argument records do not imply a Table 5 file record", "[mom][encoding]") {
  using namespace umbra::detail;

  MomServiceArgument supplied{
      MomArgumentType::number,
      L"value",
      formatMomNumber(L"7"),
  };
  MomServiceArgument returned{
      MomArgumentType::service_group,
      L"return",
      formatMomString(L"SUPPORT_SERVICES"),
  };
  REQUIRE(formatMomServiceArgumentRecord(supplied) ==
          L"{\"HLAargumentType\":35,\"HLAargumentName\":\"value\",\"HLAargumentValue\":7}");
  REQUIRE(formatMomServiceArgumentRecord(returned) ==
          L"{\"HLAargumentType\":50,\"HLAargumentName\":\"return\","
          L"\"HLAargumentValue\":\"SUPPORT_SERVICES\"}");
  // Table 5's log-only ReturnArgument alias has no declared general mapping
  // to this interaction-level HLAargument record (RL-042).  Keep an emitted
  // file-record formatter absent until that mapping is sourced.
}
