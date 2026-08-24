#include <catch2/catch_test_macros.hpp>

#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <initializer_list>
#include <set>
#include <tuple>
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

TEST_CASE(
    "MOM service-report records use the standard MIM fixed-record layout",
    "[mom][encoding][service-reporting][unit]") {
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

TEST_CASE(
    "MOM service-report parameters carry their declared MIM encodings",
    "[mom][encoding][service-reporting][unit]") {
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

TEST_CASE(
    "MOM service-report argument text follows the Table 5 JSON-like primitives",
    "[mom][encoding][service-reporting][unit]") {
  using namespace umbra::detail;

  REQUIRE(formatMomNull() == L"null");
  REQUIRE(formatMomBoolean(true) == L"true");
  REQUIRE(formatMomBoolean(false) == L"false");
  REQUIRE(formatMomNumber(L"-12.50") == L"-12.50");
  REQUIRE_THROWS(formatMomNumber(L"12."));
  REQUIRE_THROWS(formatMomNumber(L"one"));
  REQUIRE(formatMomString(L"a'\"\\\n\r\t") == L"\"a\\'\\\"\\\\\\n\\r\\t\"");
  REQUIRE(formatMomStringSet({L"Object2", L"Object1"}) ==
      L"[\"Object1\",\"Object2\"]");
}

TEST_CASE(
    "MOM service-report files begin with the Table 5 initial record",
    "[mom][encoding][service-reporting][unit]") {
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

TEST_CASE(
    "MOM service-report files format successful void records using Table 5",
    "[mom][encoding][service-reporting][unit]") {
  using namespace umbra::detail;

  MomServiceArgument supplied{
      MomArgumentType::boolean,
      L"SwitchValue",
      formatMomBoolean(false),
  };
  REQUIRE(formatMomServiceArgumentRecord(supplied) ==
          L"{\"HLAargumentType\":6,\"HLAargumentName\":\"SwitchValue\","
          L"\"HLAargumentValue\":false}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              7U,
              L"SetExceptionReportingSwitch",
              {supplied}) ==
          L"{\"HLAserialNumber\":7,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"SetExceptionReportingSwitch\","
          L"\"HLAsuppliedArguments\":[{\"HLAargumentType\":6,"
          L"\"HLAargumentName\":\"SwitchValue\",\"HLAargumentValue\":false}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files preserve empty supplied arguments for no-argument services",
    "[mom][encoding][service-reporting][unit][time-management][time-role]") {
  using namespace umbra::detail;

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              7U,
              L"EnableTimeConstrained",
              {}) ==
          L"{\"HLAserialNumber\":7,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"EnableTimeConstrained\","
          L"\"HLAsuppliedArguments\":[],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files encode failed invocations with a Null return and exception",
    "[mom][encoding][service-reporting][unit][service-failure]") {
  using namespace umbra::detail;

  MomServiceArgument supplied{
      MomArgumentType::string,
      L"Object class name",
      formatMomString(L"HLAobjectRoot.Missing"),
  };

  REQUIRE(formatMomFailedServiceReportRecord(
              4U,
              L"GetObjectClassHandle",
              {supplied},
              L"NameNotFound: The supplied object class name is not defined.") ==
          L"{\"HLAserialNumber\":4,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"GetObjectClassHandle\","
          L"\"HLAsuppliedArguments\":[{\"HLAargumentType\":53,"
          L"\"HLAargumentName\":\"Object class name\","
          L"\"HLAargumentValue\":\"HLAobjectRoot.Missing\"}],"
          L"\"HLAsuccessIndicator\":false,"
          L"\"HLAexception\":\"NameNotFound: The supplied object class name is not defined.\"}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Commit Region Modifications argument form",
    "[mom][encoding][service-reporting][unit][ddm][commit-region-modifications-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeRegionHandle;

  rti1516_2025::RegionHandleSet regions{
      makeRegionHandle(7U),
      makeRegionHandle(11U),
  };
  auto const supplied = MomServiceArgument{
      MomArgumentType::region_handle_set,
      L"Set of region designators",
      formatMomRegionHandleSet(regions),
  };

  REQUIRE(formatMomRegionHandleSet({}) == L"[]");
  REQUIRE(formatMomRegionHandleSet(regions) ==
          L"[\"RegionHandle(7)\",\"RegionHandle(11)\"]");
  REQUIRE(formatMomServiceArgumentRecord(supplied) ==
          L"{\"HLAargumentType\":43,\"HLAargumentName\":\"Set of region designators\","
          L"\"HLAargumentValue\":[\"RegionHandle(7)\",\"RegionHandle(11)\"]}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U,
              L"CommitRegionModifications",
              {supplied}) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"CommitRegionModifications\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":43,\"HLAargumentName\":\"Set of region designators\","
          L"\"HLAargumentValue\":[\"RegionHandle(7)\",\"RegionHandle(11)\"]}],"
              L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 attribute-region association list form",
    "[mom][encoding][service-reporting][unit][ddm][regional-object-attribute-association-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeRegionHandle;

  rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const associations{{
      rti1516_2025::AttributeHandleSet{
          makeAttributeHandle(7U), makeAttributeHandle(3U)},
      rti1516_2025::RegionHandleSet{
          makeRegionHandle(11U), makeRegionHandle(5U)},
  }};
  auto const supplied = MomServiceArgument{
      MomArgumentType::attribute_set_region_set_pair_list,
      L"Collection of attribute designator set and region designator set pairs",
      formatMomAttributeSetRegionSetPairList(associations),
  };

  REQUIRE(formatMomAttributeSetRegionSetPairList({}) == L"[]");
  REQUIRE(formatMomAttributeSetRegionSetPairList(associations) ==
      L"[{\"attributeHandleSet\":[\"AttributeHandle(3)\",\"AttributeHandle(7)\"],"
      L"\"regionHandleSet\":[\"RegionHandle(5)\",\"RegionHandle(11)\"]}]");
  REQUIRE(formatMomServiceArgumentRecord(supplied) ==
      L"{\"HLAargumentType\":4,\"HLAargumentName\":\"Collection of attribute designator set and region designator set pairs\","
      L"\"HLAargumentValue\":[{\"attributeHandleSet\":[\"AttributeHandle(3)\",\"AttributeHandle(7)\"],"
      L"\"regionHandleSet\":[\"RegionHandle(5)\",\"RegionHandle(11)\"]}]}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Delete Region argument form",
    "[mom][encoding][service-reporting][unit][ddm][delete-region-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeRegionHandle;

  auto const region = makeRegionHandle(23U);
  auto const supplied = MomServiceArgument{
      MomArgumentType::region_handle,
      L"Region designator",
      formatMomRegionHandle(region),
  };

  REQUIRE(formatMomRegionHandle(region) == L"\"RegionHandle(23)\"");
  REQUIRE(formatMomServiceArgumentRecord(supplied) ==
          L"{\"HLAargumentType\":42,\"HLAargumentName\":\"Region designator\","
          L"\"HLAargumentValue\":\"RegionHandle(23)\"}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U,
              L"DeleteRegion",
              {supplied}) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"DeleteRegion\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":42,\"HLAargumentName\":\"Region designator\","
          L"\"HLAargumentValue\":\"RegionHandle(23)\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Set Range Bounds argument forms",
    "[mom][encoding][service-reporting][unit][ddm][set-range-bounds-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeDimensionHandle;
  using rti1516_2025::umbra_binding_detail::makeRegionHandle;

  auto const region = makeRegionHandle(31U);
  auto const dimension = makeDimensionHandle(47U);
  std::vector<MomServiceArgument> const supplied{
      {MomArgumentType::region_handle,
       L"Region handle",
       formatMomRegionHandle(region)},
      {MomArgumentType::dimension_handle,
       L"Dimension handle",
       formatMomDimensionHandle(dimension)},
      {MomArgumentType::number, L"Range lower bound", formatMomNumber(L"3")},
      {MomArgumentType::number, L"Range upper bound", formatMomNumber(L"19")},
  };

  REQUIRE(formatMomDimensionHandle(dimension) == L"\"DimensionHandle(47)\"");
  REQUIRE(formatMomServiceArgumentRecord(supplied[2]) ==
          L"{\"HLAargumentType\":35,\"HLAargumentName\":\"Range lower bound\","
          L"\"HLAargumentValue\":3}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U,
              L"SetRangeBounds",
              supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"SetRangeBounds\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":42,\"HLAargumentName\":\"Region handle\","
          L"\"HLAargumentValue\":\"RegionHandle(31)\"},"
          L"{\"HLAargumentType\":10,\"HLAargumentName\":\"Dimension handle\","
          L"\"HLAargumentValue\":\"DimensionHandle(47)\"},"
          L"{\"HLAargumentType\":35,\"HLAargumentName\":\"Range lower bound\","
          L"\"HLAargumentValue\":3},"
          L"{\"HLAargumentType\":35,\"HLAargumentName\":\"Range upper bound\","
          L"\"HLAargumentValue\":19}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Create Region and Get Range Bounds return forms",
    "[mom][encoding][service-reporting][unit][ddm]"
    "[create-region-service-report][get-range-bounds-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeDimensionHandle;
  using rti1516_2025::umbra_binding_detail::makeRegionHandle;

  rti1516_2025::DimensionHandleSet const dimensions{
      makeDimensionHandle(47U),
      makeDimensionHandle(12U),
  };
  REQUIRE(formatMomDimensionHandleSet(dimensions) ==
          L"[\"DimensionHandle(12)\",\"DimensionHandle(47)\"]");
  REQUIRE(formatMomRangeBounds(3UL, 19UL) ==
          L"{\"lower\":3,\"upper\":19}");

  auto const createRegionReturn = MomServiceArgument{
      MomArgumentType::region_handle,
      L"Region designator",
      formatMomRegionHandle(makeRegionHandle(23U)),
  };
  auto const createRegionSupplied = MomServiceArgument{
      MomArgumentType::dimension_handle_set,
      L"Set of dimension designators",
      formatMomDimensionHandleSet(dimensions),
  };
  REQUIRE(formatMomServiceArgumentRecord(createRegionReturn) ==
          L"{\"HLAargumentType\":42,\"HLAargumentName\":\"Region designator\","
          L"\"HLAargumentValue\":\"RegionHandle(23)\"}");
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              0U,
              L"CreateRegion",
              {createRegionSupplied},
              createRegionReturn) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[{\"HLAargumentType\":42,"
          L"\"HLAargumentName\":\"Region designator\",\"HLAargumentValue\":"
          L"\"RegionHandle(23)\"}],\"HLAservice\":\"CreateRegion\","
          L"\"HLAsuppliedArguments\":[{\"HLAargumentType\":11,"
          L"\"HLAargumentName\":\"Set of dimension designators\","
          L"\"HLAargumentValue\":[\"DimensionHandle(12)\",\"DimensionHandle(47)\"]}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");

  auto const getRangeBoundsSupplied = std::vector<MomServiceArgument>{
      {MomArgumentType::region_handle,
       L"Region handle",
       formatMomRegionHandle(makeRegionHandle(23U))},
      {MomArgumentType::dimension_handle,
       L"Dimension handle",
       formatMomDimensionHandle(makeDimensionHandle(47U))},
  };
  auto const getRangeBoundsReturn = MomServiceArgument{
      MomArgumentType::range_bounds,
      L"Range bounds",
      formatMomRangeBounds(3UL, 19UL),
  };
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              1U,
              L"GetRangeBounds",
              getRangeBoundsSupplied,
              getRangeBoundsReturn) ==
          L"{\"HLAserialNumber\":1,\"HLAreturnedArgument\":[{\"HLAargumentType\":41,"
          L"\"HLAargumentName\":\"Range bounds\",\"HLAargumentValue\":"
          L"{\"lower\":3,\"upper\":19}}],\"HLAservice\":\"GetRangeBounds\","
          L"\"HLAsuppliedArguments\":[{\"HLAargumentType\":42,"
          L"\"HLAargumentName\":\"Region handle\",\"HLAargumentValue\":"
          L"\"RegionHandle(23)\"},{\"HLAargumentType\":10,"
          L"\"HLAargumentName\":\"Dimension handle\",\"HLAargumentValue\":"
          L"\"DimensionHandle(47)\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 support dimension lookup forms",
    "[mom][encoding][service-reporting][unit][support-services][ddm]"
    "[dimension-lookup-service-reports]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeDimensionHandle;
  using rti1516_2025::umbra_binding_detail::makeInteractionClassHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectClassHandle;
  using rti1516_2025::umbra_binding_detail::makeRegionHandle;

  auto const objectClass = makeObjectClassHandle(21U);
  auto const interactionClass = makeInteractionClassHandle(22U);
  auto const dimension = makeDimensionHandle(47U);
  auto const region = makeRegionHandle(23U);
  rti1516_2025::DimensionHandleSet const dimensions{
      makeDimensionHandle(12U), dimension};

  auto const objectClassSupplied = MomServiceArgument{
      MomArgumentType::object_class_handle,
      L"Object class handle",
      formatMomObjectClassHandle(objectClass),
  };
  auto const interactionClassSupplied = MomServiceArgument{
      MomArgumentType::interaction_class_handle,
      L"Interaction class handle",
      formatMomInteractionClassHandle(interactionClass),
  };
  auto const dimensionNameSupplied = MomServiceArgument{
      MomArgumentType::string,
      L"Dimension name",
      formatMomString(L"BarQuantity"),
  };
  auto const dimensionSupplied = MomServiceArgument{
      MomArgumentType::dimension_handle,
      L"Dimension handle",
      formatMomDimensionHandle(dimension),
  };
  auto const regionSupplied = MomServiceArgument{
      MomArgumentType::region_handle,
      L"Region handle",
      formatMomRegionHandle(region),
  };
  auto const dimensionSetReturned = MomServiceArgument{
      MomArgumentType::dimension_handle_set,
      L"A set of dimension handles",
      formatMomDimensionHandleSet(dimensions),
  };

  REQUIRE(formatMomSuccessfulServiceReportRecord(
              0U,
              L"GetAvailableDimensionsForObjectClass",
              {objectClassSupplied},
              dimensionSetReturned) ==
      L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[{\"HLAargumentType\":11,"
      L"\"HLAargumentName\":\"A set of dimension handles\",\"HLAargumentValue\":["
      L"\"DimensionHandle(12)\",\"DimensionHandle(47)\"]}],"
      L"\"HLAservice\":\"GetAvailableDimensionsForObjectClass\","
      L"\"HLAsuppliedArguments\":[{\"HLAargumentType\":36,"
      L"\"HLAargumentName\":\"Object class handle\","
      L"\"HLAargumentValue\":\"ObjectClassHandle(21)\"}],"
      L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");

  REQUIRE(formatMomSuccessfulServiceReportRecord(
              1U,
              L"GetAvailableDimensionsForInteractionClass",
              {interactionClassSupplied},
              dimensionSetReturned) ==
      L"{\"HLAserialNumber\":1,\"HLAreturnedArgument\":[{\"HLAargumentType\":11,"
      L"\"HLAargumentName\":\"A set of dimension handles\",\"HLAargumentValue\":["
      L"\"DimensionHandle(12)\",\"DimensionHandle(47)\"]}],"
      L"\"HLAservice\":\"GetAvailableDimensionsForInteractionClass\","
      L"\"HLAsuppliedArguments\":[{\"HLAargumentType\":27,"
      L"\"HLAargumentName\":\"Interaction class handle\","
      L"\"HLAargumentValue\":\"InteractionClassHandle(22)\"}],"
      L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");

  REQUIRE(formatMomSuccessfulServiceReportRecord(
              2U,
              L"GetDimensionHandle",
              {dimensionNameSupplied},
              dimensionSupplied) ==
      L"{\"HLAserialNumber\":2,\"HLAreturnedArgument\":[{\"HLAargumentType\":10,"
      L"\"HLAargumentName\":\"Dimension handle\",\"HLAargumentValue\":"
      L"\"DimensionHandle(47)\"}],\"HLAservice\":\"GetDimensionHandle\","
      L"\"HLAsuppliedArguments\":[{\"HLAargumentType\":53,"
      L"\"HLAargumentName\":\"Dimension name\",\"HLAargumentValue\":\"BarQuantity\"}],"
      L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");

  auto const dimensionNameReturned = MomServiceArgument{
      MomArgumentType::string,
      L"Dimension name",
      formatMomString(L"BarQuantity"),
  };
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              3U,
              L"GetDimensionName",
              {dimensionSupplied},
              dimensionNameReturned) ==
      L"{\"HLAserialNumber\":3,\"HLAreturnedArgument\":[{\"HLAargumentType\":53,"
      L"\"HLAargumentName\":\"Dimension name\",\"HLAargumentValue\":\"BarQuantity\"}],"
      L"\"HLAservice\":\"GetDimensionName\",\"HLAsuppliedArguments\":[{"
      L"\"HLAargumentType\":10,\"HLAargumentName\":\"Dimension handle\","
      L"\"HLAargumentValue\":\"DimensionHandle(47)\"}],"
      L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");

  auto const upperBoundReturned = MomServiceArgument{
      MomArgumentType::number,
      L"Dimension upper bound",
      formatMomNumber(L"25"),
  };
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              4U,
              L"GetDimensionUpperBound",
              {dimensionSupplied},
              upperBoundReturned) ==
      L"{\"HLAserialNumber\":4,\"HLAreturnedArgument\":[{\"HLAargumentType\":35,"
      L"\"HLAargumentName\":\"Dimension upper bound\",\"HLAargumentValue\":25}],"
      L"\"HLAservice\":\"GetDimensionUpperBound\",\"HLAsuppliedArguments\":[{"
      L"\"HLAargumentType\":10,\"HLAargumentName\":\"Dimension handle\","
      L"\"HLAargumentValue\":\"DimensionHandle(47)\"}],"
      L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");

  auto const dimensionsReturned = MomServiceArgument{
      MomArgumentType::dimension_handle_set,
      L"A set of dimensions",
      formatMomDimensionHandleSet(dimensions),
  };
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              5U,
              L"GetDimensionHandleSet",
              {regionSupplied},
              dimensionsReturned) ==
      L"{\"HLAserialNumber\":5,\"HLAreturnedArgument\":[{\"HLAargumentType\":11,"
      L"\"HLAargumentName\":\"A set of dimensions\",\"HLAargumentValue\":["
      L"\"DimensionHandle(12)\",\"DimensionHandle(47)\"]}],"
      L"\"HLAservice\":\"GetDimensionHandleSet\",\"HLAsuppliedArguments\":[{"
      L"\"HLAargumentType\":42,\"HLAargumentName\":\"Region handle\","
      L"\"HLAargumentValue\":\"RegionHandle(23)\"}],"
      L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 order and transportation lookup forms",
    "[mom][encoding][service-reporting][unit][support-services]"
    "[transportation-management][time-management][order-transportation-lookup-service-reports]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeTransportationTypeHandle;

  auto const reliable = makeTransportationTypeHandle(1U);
  auto const receive = rti1516_2025::RECEIVE;
  auto const timestamp = rti1516_2025::TIMESTAMP;
  auto const orderNameSupplied = MomServiceArgument{
      MomArgumentType::string,
      L"Order name",
      formatMomString(L"TimeStamp"),
  };
  auto const orderTypeSupplied = MomServiceArgument{
      MomArgumentType::order_type,
      L"Order type",
      formatMomOrderType(receive),
  };
  auto const transportationNameSupplied = MomServiceArgument{
      MomArgumentType::string,
      L"Transportation type name",
      formatMomString(L"HLAreliable"),
  };
  auto const transportationHandleSupplied = MomServiceArgument{
      MomArgumentType::transportation_type_handle,
      L"Transportation type handle",
      formatMomTransportationTypeHandle(reliable),
  };

  REQUIRE(formatMomSuccessfulServiceReportRecord(
              0U,
              L"GetOrderType",
              {orderNameSupplied},
              {MomArgumentType::order_type,
               L"Order type",
               formatMomOrderType(timestamp)}) ==
      L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[{\"HLAargumentType\":38,"
      L"\"HLAargumentName\":\"Order type\",\"HLAargumentValue\":\"TIMESTAMP\"}],"
      L"\"HLAservice\":\"GetOrderType\",\"HLAsuppliedArguments\":[{"
      L"\"HLAargumentType\":53,\"HLAargumentName\":\"Order name\","
      L"\"HLAargumentValue\":\"TimeStamp\"}],\"HLAsuccessIndicator\":true,"
      L"\"HLAexception\":null}");

  REQUIRE(formatMomSuccessfulServiceReportRecord(
              1U,
              L"GetOrderName",
              {orderTypeSupplied},
              {MomArgumentType::string,
               L"Order name",
               formatMomString(L"Receive")}) ==
      L"{\"HLAserialNumber\":1,\"HLAreturnedArgument\":[{\"HLAargumentType\":53,"
      L"\"HLAargumentName\":\"Order name\",\"HLAargumentValue\":\"Receive\"}],"
      L"\"HLAservice\":\"GetOrderName\",\"HLAsuppliedArguments\":[{"
      L"\"HLAargumentType\":38,\"HLAargumentName\":\"Order type\","
      L"\"HLAargumentValue\":\"RECEIVE\"}],\"HLAsuccessIndicator\":true,"
      L"\"HLAexception\":null}");

  REQUIRE(formatMomSuccessfulServiceReportRecord(
              2U,
              L"GetTransportationTypeHandle",
              {transportationNameSupplied},
              {MomArgumentType::transportation_type_handle,
               L"Transportation type handle",
               formatMomTransportationTypeHandle(reliable)}) ==
      L"{\"HLAserialNumber\":2,\"HLAreturnedArgument\":[{\"HLAargumentType\":59,"
      L"\"HLAargumentName\":\"Transportation type handle\","
      L"\"HLAargumentValue\":\"TransportationTypeHandle(1)\"}],"
      L"\"HLAservice\":\"GetTransportationTypeHandle\","
      L"\"HLAsuppliedArguments\":[{\"HLAargumentType\":53,"
      L"\"HLAargumentName\":\"Transportation type name\","
      L"\"HLAargumentValue\":\"HLAreliable\"}],"
      L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");

  REQUIRE(formatMomSuccessfulServiceReportRecord(
              3U,
              L"GetTransportationTypeName",
              {transportationHandleSupplied},
              {MomArgumentType::string,
               L"Transportation type name",
               formatMomString(L"HLAreliable")}) ==
      L"{\"HLAserialNumber\":3,\"HLAreturnedArgument\":[{\"HLAargumentType\":53,"
      L"\"HLAargumentName\":\"Transportation type name\","
      L"\"HLAargumentValue\":\"HLAreliable\"}],"
      L"\"HLAservice\":\"GetTransportationTypeName\","
      L"\"HLAsuppliedArguments\":[{\"HLAargumentType\":59,"
      L"\"HLAargumentName\":\"Transportation type handle\","
      L"\"HLAargumentValue\":\"TransportationTypeHandle(1)\"}],"
      L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 federate and object-class lookup forms",
    "[mom][encoding][service-reporting][unit][support-services]"
    "[federation-object-lookup-service-reports]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeFederateHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectClassHandle;

  auto const federate = makeFederateHandle(12U);
  auto const objectClass = makeObjectClassHandle(21U);
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              0U,
              L"GetFederateHandle",
              {{MomArgumentType::string,
                L"Federate name",
                formatMomString(L"owner")}},
              {MomArgumentType::federate_handle,
               L"Federate handle",
               formatMomFederateHandle(federate)}) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[{\"HLAargumentType\":15,"
          L"\"HLAargumentName\":\"Federate handle\",\"HLAargumentValue\":\"FederateHandle(12)\"}],"
          L"\"HLAservice\":\"GetFederateHandle\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":53,\"HLAargumentName\":\"Federate name\","
          L"\"HLAargumentValue\":\"owner\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              1U,
              L"GetFederateName",
              {{MomArgumentType::federate_handle,
                L"Federate handle",
                formatMomFederateHandle(federate)}},
              {MomArgumentType::string,
               L"Federate name",
               formatMomString(L"owner")}) ==
          L"{\"HLAserialNumber\":1,\"HLAreturnedArgument\":[{\"HLAargumentType\":53,"
          L"\"HLAargumentName\":\"Federate name\",\"HLAargumentValue\":\"owner\"}],"
          L"\"HLAservice\":\"GetFederateName\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":15,\"HLAargumentName\":\"Federate handle\","
          L"\"HLAargumentValue\":\"FederateHandle(12)\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              2U,
              L"GetObjectClassHandle",
              {{MomArgumentType::string,
                L"Object class name",
                formatMomString(L"HLAobjectRoot.Employee.Server")}},
              {MomArgumentType::object_class_handle,
               L"Object class handle",
               formatMomObjectClassHandle(objectClass)}) ==
          L"{\"HLAserialNumber\":2,\"HLAreturnedArgument\":[{\"HLAargumentType\":36,"
          L"\"HLAargumentName\":\"Object class handle\",\"HLAargumentValue\":\"ObjectClassHandle(21)\"}],"
          L"\"HLAservice\":\"GetObjectClassHandle\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":53,\"HLAargumentName\":\"Object class name\","
          L"\"HLAargumentValue\":\"HLAobjectRoot.Employee.Server\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              3U,
              L"GetObjectClassName",
              {{MomArgumentType::object_class_handle,
                L"Object class handle",
                formatMomObjectClassHandle(objectClass)}},
              {MomArgumentType::string,
               L"Object class name",
               formatMomString(L"HLAobjectRoot.Employee.Server")}) ==
          L"{\"HLAserialNumber\":3,\"HLAreturnedArgument\":[{\"HLAargumentType\":53,"
          L"\"HLAargumentName\":\"Object class name\",\"HLAargumentValue\":\"HLAobjectRoot.Employee.Server\"}],"
          L"\"HLAservice\":\"GetObjectClassName\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":36,\"HLAargumentName\":\"Object class handle\","
          L"\"HLAargumentValue\":\"ObjectClassHandle(21)\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 handle normalization forms",
    "[mom][encoding][service-reporting][unit][support-services][ddm]"
    "[handle-normalization-service-reports]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeFederateHandle;
  using rti1516_2025::umbra_binding_detail::makeInteractionClassHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectClassHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const serviceGroup = rti1516_2025::SUPPORT_SERVICES;
  auto const federate = makeFederateHandle(12U);
  auto const objectClass = makeObjectClassHandle(21U);
  auto const interactionClass = makeInteractionClassHandle(22U);
  auto const objectInstance = makeObjectInstanceHandle(23U);

  REQUIRE(formatMomServiceGroup(serviceGroup) == L"\"SUPPORT_SERVICES\"");
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              0U,
              L"NormalizeServiceGroup",
              {{MomArgumentType::service_group,
                L"Service group indicator",
                formatMomServiceGroup(serviceGroup)}},
              {MomArgumentType::number,
               L"Normalized value",
               formatMomNumber(L"6")}) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[{\"HLAargumentType\":35,"
          L"\"HLAargumentName\":\"Normalized value\",\"HLAargumentValue\":6}],"
          L"\"HLAservice\":\"NormalizeServiceGroup\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":50,\"HLAargumentName\":\"Service group indicator\","
          L"\"HLAargumentValue\":\"SUPPORT_SERVICES\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");

  auto const handleCases = std::vector<std::tuple<
      std::wstring,
      MomArgumentType,
      std::wstring,
      std::wstring>>{
      {L"NormalizeFederateHandle",
       MomArgumentType::federate_handle,
       L"Federate handle",
       formatMomFederateHandle(federate)},
      {L"NormalizeObjectClassHandle",
       MomArgumentType::object_class_handle,
       L"Object class handle",
       formatMomObjectClassHandle(objectClass)},
      {L"NormalizeInteractionClassHandle",
       MomArgumentType::interaction_class_handle,
       L"Interaction class handle",
       formatMomInteractionClassHandle(interactionClass)},
      {L"NormalizeObjectInstanceHandle",
       MomArgumentType::object_instance_handle,
       L"Object instance handle",
       formatMomObjectInstanceHandle(objectInstance)},
  };
  auto serialNumber = 1U;
  for (auto const& [service, suppliedType, suppliedName, suppliedValue] : handleCases) {
    auto const record = formatMomSuccessfulServiceReportRecord(
        serialNumber++,
        service,
        {{suppliedType, suppliedName, suppliedValue}},
        {MomArgumentType::number, L"Normalized value", formatMomNumber(L"7")});
    REQUIRE(record.find(L"\"HLAargumentType\":35") != std::wstring::npos);
    REQUIRE(record.find(L"\"HLAargumentName\":\"Normalized value\"") !=
            std::wstring::npos);
    REQUIRE(record.find(L"\"HLAargumentName\":\"" + suppliedName + L"\"") !=
            std::wstring::npos);
    REQUIRE(record.find(L"\"HLAargumentValue\":" + suppliedValue) !=
            std::wstring::npos);
  }
}

TEST_CASE(
    "MOM service-report files use the Federate Save Begun no-argument form",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[federate-save-begun]") {
  using namespace umbra::detail;

  // §4.21.1 and §4.21.2 both say None. Section 11.5.1 consequently requires
  // an empty supplied-argument list, while Table 5's successful-void record
  // retains its explicit [null] return form.
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(0U, L"FederateSaveBegun", {}) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"FederateSaveBegun\",\"HLAsuppliedArguments\":[],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 interaction-class and parameter lookup forms",
    "[mom][encoding][service-reporting][unit][support-services]"
    "[federation-interaction-parameter-lookup-service-reports]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeInteractionClassHandle;
  using rti1516_2025::umbra_binding_detail::makeParameterHandle;

  auto const interactionClass = makeInteractionClassHandle(21U);
  auto const parameter = makeParameterHandle(17U);
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              0U,
              L"GetInteractionClassHandle",
              {{MomArgumentType::string,
                L"Interaction class name",
                formatMomString(L"HLAinteractionRoot.ServerAction.TakeOrder")}},
              {MomArgumentType::interaction_class_handle,
               L"Interaction class handle",
               formatMomInteractionClassHandle(interactionClass)}) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[{\"HLAargumentType\":27,"
          L"\"HLAargumentName\":\"Interaction class handle\",\"HLAargumentValue\":\"InteractionClassHandle(21)\"}],"
          L"\"HLAservice\":\"GetInteractionClassHandle\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":53,\"HLAargumentName\":\"Interaction class name\","
          L"\"HLAargumentValue\":\"HLAinteractionRoot.ServerAction.TakeOrder\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              1U,
              L"GetInteractionClassName",
              {{MomArgumentType::interaction_class_handle,
                L"Interaction class handle",
                formatMomInteractionClassHandle(interactionClass)}},
              {MomArgumentType::string,
               L"Interaction class name",
               formatMomString(L"HLAinteractionRoot.ServerAction.TakeOrder")}) ==
          L"{\"HLAserialNumber\":1,\"HLAreturnedArgument\":[{\"HLAargumentType\":53,"
          L"\"HLAargumentName\":\"Interaction class name\",\"HLAargumentValue\":\"HLAinteractionRoot.ServerAction.TakeOrder\"}],"
          L"\"HLAservice\":\"GetInteractionClassName\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":27,\"HLAargumentName\":\"Interaction class handle\","
          L"\"HLAargumentValue\":\"InteractionClassHandle(21)\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              2U,
              L"GetParameterHandle",
              {{MomArgumentType::interaction_class_handle,
                L"Interaction class handle",
                formatMomInteractionClassHandle(interactionClass)},
               {MomArgumentType::string,
                L"Parameter name",
                formatMomString(L"orderId")}},
              {MomArgumentType::parameter_handle,
               L"Parameter handle",
               formatMomParameterHandle(parameter)}) ==
          L"{\"HLAserialNumber\":2,\"HLAreturnedArgument\":[{\"HLAargumentType\":39,"
          L"\"HLAargumentName\":\"Parameter handle\",\"HLAargumentValue\":\"ParameterHandle(17)\"}],"
          L"\"HLAservice\":\"GetParameterHandle\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":27,\"HLAargumentName\":\"Interaction class handle\","
          L"\"HLAargumentValue\":\"InteractionClassHandle(21)\"},{\"HLAargumentType\":53,"
          L"\"HLAargumentName\":\"Parameter name\",\"HLAargumentValue\":\"orderId\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              3U,
              L"GetParameterName",
              {{MomArgumentType::interaction_class_handle,
                L"Interaction class handle",
                formatMomInteractionClassHandle(interactionClass)},
               {MomArgumentType::parameter_handle,
                L"Parameter handle",
                formatMomParameterHandle(parameter)}},
              {MomArgumentType::string,
               L"Parameter name",
               formatMomString(L"orderId")}) ==
          L"{\"HLAserialNumber\":3,\"HLAreturnedArgument\":[{\"HLAargumentType\":53,"
          L"\"HLAargumentName\":\"Parameter name\",\"HLAargumentValue\":\"orderId\"}],"
          L"\"HLAservice\":\"GetParameterName\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":27,\"HLAargumentName\":\"Interaction class handle\","
          L"\"HLAargumentValue\":\"InteractionClassHandle(21)\"},{\"HLAargumentType\":39,"
          L"\"HLAargumentName\":\"Parameter handle\",\"HLAargumentValue\":\"ParameterHandle(17)\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 object, attribute, and update-rate lookup forms",
    "[mom][encoding][service-reporting][unit][support-services]"
    "[federate-object-attribute-update-rate-lookup-service-reports]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectClassHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectClass = makeObjectClassHandle(21U);
  auto const objectInstance = makeObjectInstanceHandle(23U);
  auto const attribute = makeAttributeHandle(7U);
  auto const maximumRate = formatMomNumber(std::to_wstring(30.0));

  REQUIRE(formatMomSuccessfulServiceReportRecord(
              0U,
              L"GetKnownObjectClassHandle",
              {{MomArgumentType::object_instance_handle,
                L"Object instance handle",
                formatMomObjectInstanceHandle(objectInstance)}},
              {MomArgumentType::object_class_handle,
               L"Object class handle",
               formatMomObjectClassHandle(objectClass)}) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[{\"HLAargumentType\":36,"
          L"\"HLAargumentName\":\"Object class handle\",\"HLAargumentValue\":\"ObjectClassHandle(21)\"}],"
          L"\"HLAservice\":\"GetKnownObjectClassHandle\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance handle\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(23)\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              1U,
              L"GetObjectInstanceHandle",
              {{MomArgumentType::string,
                L"Object instance name",
                formatMomString(L"lookup-service-object")}},
              {MomArgumentType::object_instance_handle,
               L"Object instance handle",
               formatMomObjectInstanceHandle(objectInstance)}) ==
          L"{\"HLAserialNumber\":1,\"HLAreturnedArgument\":[{\"HLAargumentType\":37,"
          L"\"HLAargumentName\":\"Object instance handle\",\"HLAargumentValue\":\"ObjectInstanceHandle(23)\"}],"
          L"\"HLAservice\":\"GetObjectInstanceHandle\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":53,\"HLAargumentName\":\"Object instance name\","
          L"\"HLAargumentValue\":\"lookup-service-object\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              2U,
              L"GetObjectInstanceName",
              {{MomArgumentType::object_instance_handle,
                L"Object instance handle",
                formatMomObjectInstanceHandle(objectInstance)}},
              {MomArgumentType::string,
               L"Object instance name",
               formatMomString(L"lookup-service-object")}) ==
          L"{\"HLAserialNumber\":2,\"HLAreturnedArgument\":[{\"HLAargumentType\":53,"
          L"\"HLAargumentName\":\"Object instance name\",\"HLAargumentValue\":\"lookup-service-object\"}],"
          L"\"HLAservice\":\"GetObjectInstanceName\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance handle\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(23)\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              3U,
              L"GetAttributeHandle",
              {{MomArgumentType::object_class_handle,
                L"Object class handle",
                formatMomObjectClassHandle(objectClass)},
               {MomArgumentType::string,
                L"Class attribute name",
                formatMomString(L"Efficiency")}},
              {MomArgumentType::attribute_handle,
               L"Class attribute handle",
               formatMomAttributeHandle(attribute)}) ==
          L"{\"HLAserialNumber\":3,\"HLAreturnedArgument\":[{\"HLAargumentType\":0,"
          L"\"HLAargumentName\":\"Class attribute handle\",\"HLAargumentValue\":\"AttributeHandle(7)\"}],"
          L"\"HLAservice\":\"GetAttributeHandle\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":36,\"HLAargumentName\":\"Object class handle\","
          L"\"HLAargumentValue\":\"ObjectClassHandle(21)\"},{\"HLAargumentType\":53,"
          L"\"HLAargumentName\":\"Class attribute name\",\"HLAargumentValue\":\"Efficiency\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              4U,
              L"GetAttributeName",
              {{MomArgumentType::object_class_handle,
                L"Object class handle",
                formatMomObjectClassHandle(objectClass)},
               {MomArgumentType::attribute_handle,
                L"Class attribute handle",
                formatMomAttributeHandle(attribute)}},
              {MomArgumentType::string,
               L"Class attribute name",
               formatMomString(L"Efficiency")}) ==
          L"{\"HLAserialNumber\":4,\"HLAreturnedArgument\":[{\"HLAargumentType\":53,"
          L"\"HLAargumentName\":\"Class attribute name\",\"HLAargumentValue\":\"Efficiency\"}],"
          L"\"HLAservice\":\"GetAttributeName\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":36,\"HLAargumentName\":\"Object class handle\","
          L"\"HLAargumentValue\":\"ObjectClassHandle(21)\"},{\"HLAargumentType\":0,"
          L"\"HLAargumentName\":\"Class attribute handle\",\"HLAargumentValue\":\"AttributeHandle(7)\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              5U,
              L"GetUpdateRateValue",
              {{MomArgumentType::string,
                L"Update rate name",
                formatMomString(L"High")}},
              {MomArgumentType::number,
               L"Maximum update rate value",
               maximumRate}) ==
          L"{\"HLAserialNumber\":5,\"HLAreturnedArgument\":[{\"HLAargumentType\":35,"
          L"\"HLAargumentName\":\"Maximum update rate value\",\"HLAargumentValue\":" +
              maximumRate +
          L"}],\"HLAservice\":\"GetUpdateRateValue\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":53,\"HLAargumentName\":\"Update rate name\","
          L"\"HLAargumentValue\":\"High\"}],\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulServiceReportRecord(
              6U,
              L"GetUpdateRateValueForAttribute",
              {{MomArgumentType::object_instance_handle,
                L"Object instance handle",
                formatMomObjectInstanceHandle(objectInstance)},
               {MomArgumentType::attribute_handle,
                L"Attribute handle",
                formatMomAttributeHandle(attribute)}},
              {MomArgumentType::number,
               L"Maximum update rate value",
               maximumRate}) ==
          L"{\"HLAserialNumber\":6,\"HLAreturnedArgument\":[{\"HLAargumentType\":35,"
          L"\"HLAargumentName\":\"Maximum update rate value\",\"HLAargumentValue\":" +
              maximumRate +
          L"}],\"HLAservice\":\"GetUpdateRateValueForAttribute\",\"HLAsuppliedArguments\":[{"
          L"\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance handle\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(23)\"},{\"HLAargumentType\":0,"
          L"\"HLAargumentName\":\"Attribute handle\",\"HLAargumentValue\":\"AttributeHandle(7)\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Federate Save Complete success-indicator forms",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[federate-save-complete][federate-save-not-complete]") {
  using namespace umbra::detail;

  // §4.22 is one service with one required Boolean. The official C++ binding
  // exposes its two selector values as federateSaveComplete() and
  // federateSaveNotComplete(), so both report forms retain one service name.
  auto const success = MomServiceArgument{
      MomArgumentType::boolean,
      L"Federate save-success indicator",
      formatMomBoolean(true),
  };
  auto const failure = MomServiceArgument{
      MomArgumentType::boolean,
      L"Federate save-success indicator",
      formatMomBoolean(false),
  };
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(0U, L"FederateSaveComplete", {success}) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"FederateSaveComplete\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":6,\"HLAargumentName\":\"Federate save-success indicator\","
          L"\"HLAargumentValue\":true}],\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(1U, L"FederateSaveComplete", {failure}) ==
          L"{\"HLAserialNumber\":1,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"FederateSaveComplete\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":6,\"HLAargumentName\":\"Federate save-success indicator\","
          L"\"HLAargumentValue\":false}],\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Abort Federation Save no-argument form",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[abort-federation-save]") {
  using namespace umbra::detail;

  // §4.24.1 and §4.24.2 both say None, so the Table 5 successful-void form
  // has no supplied arguments while retaining its explicit [null] return.
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(0U, L"AbortFederationSave", {}) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"AbortFederationSave\",\"HLAsuppliedArguments\":[],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Query Federation Save Status no-argument form",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[query-federation-save-status]") {
  using namespace umbra::detail;

  // §4.25.1 and §4.25.2 both say None. The status vector belongs to the
  // distinct §4.26 callback, not to this accepted service record.
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(0U, L"QueryFederationSaveStatus", {}) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"QueryFederationSaveStatus\",\"HLAsuppliedArguments\":[],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use Federation Save Status Response status-pair form",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[federation-save-status-response-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeFederateHandle;

  // Table 5 calls the C++ vector's representation
  // FederateHandleSaveStatusPairSet. Each array element is the explicit
  // handle/status record, with a quoted public SaveStatus spelling.
  rti1516_2025::FederateHandleSaveStatusPairVector const statuses{
      {makeFederateHandle(12U), rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE},
      {makeFederateHandle(14U), rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE},
  };
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::federate_handle_save_status_pair_set,
       L"List of joined federates and save status for each",
       formatMomFederateHandleSaveStatusPairVector(statuses)},
  };

  REQUIRE(formatMomSaveStatus(rti1516_2025::NO_SAVE_IN_PROGRESS) ==
          L"\"NO_SAVE_IN_PROGRESS\"");
  REQUIRE(formatMomSaveStatus(rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE) ==
          L"\"FEDERATE_INSTRUCTED_TO_SAVE\"");
  REQUIRE(formatMomSaveStatus(rti1516_2025::FEDERATE_SAVING) ==
          L"\"FEDERATE_SAVING\"");
  REQUIRE(formatMomSaveStatus(rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE) ==
          L"\"FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE\"");
  REQUIRE(formatMomFederateHandleSaveStatusPairVector(statuses) ==
          L"[{\"handle\":\"FederateHandle(12)\",\"status\":"
          L"\"FEDERATE_INSTRUCTED_TO_SAVE\"},{\"handle\":\"FederateHandle(14)\","
          L"\"status\":\"FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE\"}]");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              3U, L"FederationSaveStatusResponse", supplied) ==
          L"{\"HLAserialNumber\":3,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"FederationSaveStatusResponse\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":17,\"HLAargumentName\":"
          L"\"List of joined federates and save status for each\",\"HLAargumentValue\":"
          L"[{\"handle\":\"FederateHandle(12)\",\"status\":\"FEDERATE_INSTRUCTED_TO_SAVE\"},"
          L"{\"handle\":\"FederateHandle(14)\",\"status\":"
          L"\"FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE\"}]}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use Federation Restore Status Response descriptor form",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[federation-restore-status-response-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeFederateHandle;

  // The public C++ vector is the MIM's type-20 FederateRestoreStatusSet.
  // Table 5's collection-name typo and malformed second example are recorded
  // in RL-083; its valid singular record establishes these three fields.
  rti1516_2025::FederateRestoreStatusVector const statuses{
      {makeFederateHandle(12U), makeFederateHandle(0U), rti1516_2025::NO_RESTORE_IN_PROGRESS},
      {makeFederateHandle(14U),
       makeFederateHandle(17U),
       rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE},
  };
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::federate_restore_status_set,
       L"List of joined federates and restore status for each",
       formatMomFederateRestoreStatusVector(statuses)},
  };

  REQUIRE(formatMomRestoreStatus(rti1516_2025::NO_RESTORE_IN_PROGRESS) ==
          L"\"NO_RESTORE_IN_PROGRESS\"");
  REQUIRE(formatMomRestoreStatus(rti1516_2025::FEDERATE_RESTORE_REQUEST_PENDING) ==
          L"\"FEDERATE_RESTORE_REQUEST_PENDING\"");
  REQUIRE(formatMomRestoreStatus(rti1516_2025::FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN) ==
          L"\"FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN\"");
  REQUIRE(formatMomRestoreStatus(rti1516_2025::FEDERATE_PREPARED_TO_RESTORE) ==
          L"\"FEDERATE_PREPARED_TO_RESTORE\"");
  REQUIRE(formatMomRestoreStatus(rti1516_2025::FEDERATE_RESTORING) ==
          L"\"FEDERATE_RESTORING\"");
  REQUIRE(formatMomRestoreStatus(
              rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE) ==
          L"\"FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE\"");
  REQUIRE(formatMomFederateRestoreStatusVector(statuses) ==
          L"[{\"preRestoreHandle\":\"FederateHandle(12)\",\"postRestoreHandle\":"
          L"\"FederateHandle(invalid)\",\"status\":\"NO_RESTORE_IN_PROGRESS\"},{"
          L"\"preRestoreHandle\":\"FederateHandle(14)\",\"postRestoreHandle\":"
          L"\"FederateHandle(17)\",\"status\":"
          L"\"FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE\"}]");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              3U, L"FederationRestoreStatusResponse", supplied) ==
          L"{\"HLAserialNumber\":3,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"FederationRestoreStatusResponse\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":20,\"HLAargumentName\":"
          L"\"List of joined federates and restore status for each\",\"HLAargumentValue\":"
          L"[{\"preRestoreHandle\":\"FederateHandle(12)\",\"postRestoreHandle\":"
          L"\"FederateHandle(invalid)\",\"status\":\"NO_RESTORE_IN_PROGRESS\"},{"
          L"\"preRestoreHandle\":\"FederateHandle(14)\",\"postRestoreHandle\":"
          L"\"FederateHandle(17)\",\"status\":"
          L"\"FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE\"}]}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files preserve Request Federation Save timestamp overloads",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[request-federation-save]") {
  using namespace umbra::detail;

  // Section 4.19 has a required Federation save label and an optional
  // timestamp.  The C++ binding distinguishes absent and supplied timestamps
  // through two overloads, so Table 5 must retain the optional slot as Null
  // for the former and a type-31 LogicalTime for the latter.
  auto const label = MomServiceArgument{
      MomArgumentType::string,
      L"Federation save label",
      formatMomString(L"report-save"),
  };
  auto const omittedTimestamp = MomServiceArgument{
      MomArgumentType::null_value,
      L"Optional timestamp",
      formatMomNull(),
  };
  rti1516_2025::HLAinteger64Time timestamp{7};
  auto const suppliedTimestamp = MomServiceArgument{
      MomArgumentType::logical_time,
      L"Optional timestamp",
      formatMomLogicalTime(timestamp),
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U,
              L"RequestFederationSave",
              {label, omittedTimestamp}) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"RequestFederationSave\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Federation save label\","
          L"\"HLAargumentValue\":\"report-save\"},{\"HLAargumentType\":34,"
          L"\"HLAargumentName\":\"Optional timestamp\",\"HLAargumentValue\":null}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              1U,
              L"RequestFederationSave",
              {label, suppliedTimestamp}) ==
          L"{\"HLAserialNumber\":1,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"RequestFederationSave\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Federation save label\","
          L"\"HLAargumentValue\":\"report-save\"},{\"HLAargumentType\":31,"
          L"\"HLAargumentName\":\"Optional timestamp\",\"HLAargumentValue\":\"7\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files preserve Initiate Federate Save timestamp forms",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[initiate-federate-save-service-report]") {
  using namespace umbra::detail;

  // Section 4.20 preserves the original §4.19 save label and optional
  // timestamp at each recipient. Table 5 consequently keeps the omitted
  // timestamp position as Null and uses type 31 only for the supplied form.
  auto const label = MomServiceArgument{
      MomArgumentType::string,
      L"Federation save label",
      formatMomString(L"report-save"),
  };
  auto const omittedTimestamp = MomServiceArgument{
      MomArgumentType::null_value,
      L"Optional timestamp",
      formatMomNull(),
  };
  rti1516_2025::HLAinteger64Time timestamp{7};
  auto const suppliedTimestamp = MomServiceArgument{
      MomArgumentType::logical_time,
      L"Optional timestamp",
      formatMomLogicalTime(timestamp),
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U,
              L"InitiateFederateSave",
              {label, omittedTimestamp}) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"InitiateFederateSave\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Federation save label\","
          L"\"HLAargumentValue\":\"report-save\"},{\"HLAargumentType\":34,"
          L"\"HLAargumentName\":\"Optional timestamp\",\"HLAargumentValue\":null}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              1U,
              L"InitiateFederateSave",
              {label, suppliedTimestamp}) ==
          L"{\"HLAserialNumber\":1,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"InitiateFederateSave\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Federation save label\","
          L"\"HLAargumentValue\":\"report-save\"},{\"HLAargumentType\":31,"
          L"\"HLAargumentName\":\"Optional timestamp\",\"HLAargumentValue\":\"7\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files preserve Federation Saved result forms",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[federation-saved-service-report]") {
  using namespace umbra::detail;

  // §4.23 is one RTI-initiated service. Its C++ success/failure callbacks
  // share the Boolean result slot; only the failed form supplies Table 5's
  // type-48 SaveFailureReason in the otherwise optional second position.
  auto const success = std::vector<MomServiceArgument>{
      {MomArgumentType::boolean,
       L"Federation save-success indicator",
       formatMomBoolean(true)},
      {MomArgumentType::null_value,
       L"Optional failure reason",
       formatMomNull()},
  };
  auto const failure = std::vector<MomServiceArgument>{
      {MomArgumentType::boolean,
       L"Federation save-success indicator",
       formatMomBoolean(false)},
      {MomArgumentType::save_failure_reason,
       L"Optional failure reason",
       formatMomSaveFailureReason(rti1516_2025::SAVE_ABORTED)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              4U, L"FederationSaved", success) ==
          L"{\"HLAserialNumber\":4,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"FederationSaved\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":6,\"HLAargumentName\":\"Federation save-success indicator\","
          L"\"HLAargumentValue\":true},{\"HLAargumentType\":34,"
          L"\"HLAargumentName\":\"Optional failure reason\",\"HLAargumentValue\":null}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              6U, L"FederationSaved", failure) ==
          L"{\"HLAserialNumber\":6,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"FederationSaved\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":6,\"HLAargumentName\":\"Federation save-success indicator\","
          L"\"HLAargumentValue\":false},{\"HLAargumentType\":48,"
          L"\"HLAargumentName\":\"Optional failure reason\","
          L"\"HLAargumentValue\":\"SAVE_ABORTED\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Request Federation Restore label form",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[request-federation-restore]") {
  using namespace umbra::detail;

  // Section 4.27.1 supplies only the Federation save label, while §4.27.2
  // returns None.  The later confirm callback conveys whether a snapshot was
  // found; it does not change this successful-void request record.
  auto const label = MomServiceArgument{
      MomArgumentType::string,
      L"Federation save label",
      formatMomString(L"saved-state"),
  };
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              3U,
              L"RequestFederationRestore",
              {label}) ==
          L"{\"HLAserialNumber\":3,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"RequestFederationRestore\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Federation save label\","
          L"\"HLAargumentValue\":\"saved-state\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files preserve Confirm Federation Restoration Request result forms",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[confirm-federation-restoration-request-service-report]") {
  using namespace umbra::detail;

  // §4.28 is one RTI-initiated service.  The official C++ binding represents
  // the result through two callbacks, but both Table 5 records retain the
  // label and the required Boolean request-success indicator.
  auto const label = MomServiceArgument{
      MomArgumentType::string,
      L"Federation save label",
      formatMomString(L"saved-state"),
  };
  auto const success = std::vector<MomServiceArgument>{
      label,
      {MomArgumentType::boolean,
       L"Request-success indicator",
       formatMomBoolean(true)},
  };
  auto const failure = std::vector<MomServiceArgument>{
      label,
      {MomArgumentType::boolean,
       L"Request-success indicator",
       formatMomBoolean(false)},
  };
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              4U,
              L"ConfirmFederationRestorationRequest",
              success) ==
          L"{\"HLAserialNumber\":4,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"ConfirmFederationRestorationRequest\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Federation save label\","
          L"\"HLAargumentValue\":\"saved-state\"},{\"HLAargumentType\":6,"
          L"\"HLAargumentName\":\"Request-success indicator\",\"HLAargumentValue\":true}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              7U,
              L"ConfirmFederationRestorationRequest",
              failure) ==
          L"{\"HLAserialNumber\":7,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"ConfirmFederationRestorationRequest\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Federation save label\","
          L"\"HLAargumentValue\":\"saved-state\"},{\"HLAargumentType\":6,"
          L"\"HLAargumentName\":\"Request-success indicator\",\"HLAargumentValue\":false}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Federation Restore Begun no-argument form",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[federation-restore-begun-service-report]") {
  using namespace umbra::detail;

  // §4.29.1 and §4.29.2 both say None. The RTI-initiated record therefore
  // keeps Table 5's successful-void [null] return form and no supplied slots.
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(2U, L"FederationRestoreBegun", {}) ==
          L"{\"HLAserialNumber\":2,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"FederationRestoreBegun\",\"HLAsuppliedArguments\":[],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Initiate Federate Restore argument forms",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[initiate-federate-restore-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeFederateHandle;

  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::string,
       L"Federation save label",
       formatMomString(L"restore-snapshot")},
      {MomArgumentType::federate_handle,
       L"Joined federate designator",
       formatMomFederateHandle(makeFederateHandle(17U))},
      {MomArgumentType::string,
       L"Federate name",
       formatMomString(L"restore-subject")},
  };

  // §4.30.1 supplies the label, joined-federate designator, and federate
  // name in that order. §4.30.2 returns None, preserving Table 5's [null]
  // successful-void return form.
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              3U, L"InitiateFederateRestore", supplied) ==
          L"{\"HLAserialNumber\":3,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"InitiateFederateRestore\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Federation save label\","
          L"\"HLAargumentValue\":\"restore-snapshot\"},{\"HLAargumentType\":15,"
          L"\"HLAargumentName\":\"Joined federate designator\","
          L"\"HLAargumentValue\":\"FederateHandle(17)\"},{\"HLAargumentType\":53,"
          L"\"HLAargumentName\":\"Federate name\",\"HLAargumentValue\":\"restore-subject\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Federate Restore Complete success-indicator forms",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[federate-restore-complete][federate-restore-not-complete]") {
  using namespace umbra::detail;

  // §4.31 is one service with one required Boolean.  The official C++ binding
  // exposes its two selector values as federateRestoreComplete() and
  // federateRestoreNotComplete(), so both report forms retain one service
  // name and differ only in the Federate restore-success indicator.
  auto const success = MomServiceArgument{
      MomArgumentType::boolean,
      L"Federate restore-success indicator",
      formatMomBoolean(true),
  };
  auto const failure = MomServiceArgument{
      MomArgumentType::boolean,
      L"Federate restore-success indicator",
      formatMomBoolean(false),
  };
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              4U,
              L"FederateRestoreComplete",
              {success}) ==
          L"{\"HLAserialNumber\":4,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"FederateRestoreComplete\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":6,\"HLAargumentName\":\"Federate restore-success indicator\","
          L"\"HLAargumentValue\":true}],\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              6U,
              L"FederateRestoreComplete",
              {failure}) ==
          L"{\"HLAserialNumber\":6,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"FederateRestoreComplete\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":6,\"HLAargumentName\":\"Federate restore-success indicator\","
          L"\"HLAargumentValue\":false}],\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Abort Federation Restore no-argument form",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[abort-federation-restore]") {
  using namespace umbra::detail;

  // Section 4.33.1 and 4.33.2 both say None, so the Table 5 successful-void
  // form has no supplied arguments while retaining its explicit [null] return.
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(4U, L"AbortFederationRestore", {}) ==
          L"{\"HLAserialNumber\":4,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"AbortFederationRestore\",\"HLAsuppliedArguments\":[],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Query Federation Restore Status no-argument form",
    "[mom][encoding][service-reporting][unit][federation-management][save-restore]"
    "[query-federation-restore-status]") {
  using namespace umbra::detail;

  // Section 4.34.1 and 4.34.2 both say None. The later §4.35 status vector
  // belongs to the callback, not to this accepted successful-void request.
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U,
              L"QueryFederationRestoreStatus",
              {}) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"QueryFederationRestoreStatus\",\"HLAsuppliedArguments\":[],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Resign Federation Execution action form",
    "[mom][encoding][service-reporting][unit][federation-management]"
    "[resign-federation-execution]") {
  using namespace umbra::detail;

  // §4.12 supplies one ResignAction. Section 11.5.1 makes the descriptive
  // HLAargumentName implementation-dependent; use the corresponding Table 20
  // MOM parameter spelling while Table 5 fixes type 44 and the enum value.
  MomServiceArgument supplied{
      MomArgumentType::resign_action,
      L"HLAresignAction",
      formatMomResignAction(rti1516_2025::NO_ACTION),
  };
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U,
              L"ResignFederationExecution",
              {supplied}) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"ResignFederationExecution\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":44,\"HLAargumentName\":\"HLAresignAction\","
          L"\"HLAargumentValue\":\"NO_ACTION\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Federate Resigned reason form",
    "[mom][encoding][service-reporting][unit][federation-management]"
    "[federate-resigned-service-report]") {
  using namespace umbra::detail;

  // §4.13.1 supplies the reason text. Section 11.5.1 makes the argument-name
  // display implementation-dependent, so retain the source's descriptive
  // wording while Table 5 fixes the String argument encoding.
  MomServiceArgument supplied{
      MomArgumentType::string,
      L"Reason for resigning",
      formatMomString(L"embedded session control"),
  };
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              1U,
              L"FederateResigned",
              {supplied}) ==
          L"{\"HLAserialNumber\":1,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"FederateResigned\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Reason for resigning\","
          L"\"HLAargumentValue\":\"embedded session control\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Connection Lost fault-description form",
    "[mom][encoding][service-reporting][unit][federation-management]"
    "[connection-lost-service-report]") {
  using namespace umbra::detail;

  // §4.4.1 supplies the fault text. Section 11.5.1 makes the argument-name
  // display implementation-dependent, so retain the source's descriptive
  // wording while Table 5 fixes the String argument encoding.
  MomServiceArgument supplied{
      MomArgumentType::string,
      L"Fault description",
      formatMomString(L"embedded transport fault"),
  };
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              1U,
              L"ConnectionLost",
              {supplied}) ==
          L"{\"HLAserialNumber\":1,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"ConnectionLost\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Fault description\","
          L"\"HLAargumentValue\":\"embedded transport fault\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 LogicalTimeInterval argument form",
    "[mom][encoding][service-reporting][unit][time-management][time-role][modify-lookahead]") {
  using namespace umbra::detail;

  rti1516_2025::HLAinteger64Interval lookahead{7};
  MomServiceArgument supplied{
      MomArgumentType::logical_time_interval,
      L"Lookahead",
      formatMomLogicalTimeInterval(lookahead),
  };

  REQUIRE(formatMomServiceArgumentRecord(supplied) ==
          L"{\"HLAargumentType\":32,\"HLAargumentName\":\"Lookahead\","
          L"\"HLAargumentValue\":\"7\"}");

  rti1516_2025::HLAfloat64Interval floatLookahead{0.5};
  REQUIRE(formatMomLogicalTimeInterval(floatLookahead) == L"\"0.5\"");
}

TEST_CASE(
    "MOM service-report files use the Table 5 LogicalTime argument form",
    "[mom][encoding][service-reporting][unit][time-management][time-advance-request]"
    "[time-advance-request-available][next-message-request]"
    "[next-message-request-available][flush-queue-request]") {
  using namespace umbra::detail;

  rti1516_2025::HLAinteger64Time requestedTime{7};
  MomServiceArgument supplied{
      MomArgumentType::logical_time,
      L"Logical time",
      formatMomLogicalTime(requestedTime),
  };

  REQUIRE(formatMomServiceArgumentRecord(supplied) ==
          L"{\"HLAargumentType\":31,\"HLAargumentName\":\"Logical time\","
          L"\"HLAargumentValue\":\"7\"}");

  rti1516_2025::HLAfloat64Time floatTime{0.5};
  REQUIRE(formatMomLogicalTime(floatTime) == L"\"0.5\"");
}

TEST_CASE(
    "MOM service-report files use the Table 5 InteractionClassHandle and OrderType argument forms",
    "[mom][encoding][service-reporting][unit][time-management]"
    "[change-interaction-order-type]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeInteractionClassHandle;

  auto const interactionClass = makeInteractionClassHandle(2345U);
  REQUIRE(interactionClass.toString() == L"InteractionClassHandle(2345)");
  MomServiceArgument interactionClassArgument{
      MomArgumentType::interaction_class_handle,
      L"Interaction class designator",
      formatMomInteractionClassHandle(interactionClass),
  };
  REQUIRE(formatMomServiceArgumentRecord(interactionClassArgument) ==
          L"{\"HLAargumentType\":27,\"HLAargumentName\":"
          L"\"Interaction class designator\",\"HLAargumentValue\":"
          L"\"InteractionClassHandle(2345)\"}");

  MomServiceArgument orderTypeArgument{
      MomArgumentType::order_type,
      L"Order type",
      formatMomOrderType(rti1516_2025::TIMESTAMP),
  };
  REQUIRE(formatMomServiceArgumentRecord(orderTypeArgument) ==
          L"{\"HLAargumentType\":38,\"HLAargumentName\":\"Order type\","
          L"\"HLAargumentValue\":\"TIMESTAMP\"}");
  REQUIRE(formatMomOrderType(rti1516_2025::RECEIVE) == L"\"RECEIVE\"");
  REQUIRE_THROWS(formatMomOrderType(static_cast<rti1516_2025::OrderType>(0x7f)));
}

TEST_CASE(
    "MOM service-report files use the Table 5 TransportationTypeHandle argument form",
    "[mom][encoding][service-reporting][unit][object-management]"
    "[request-interaction-transportation-type-change]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeTransportationTypeHandle;

  auto const transportationType = makeTransportationTypeHandle(2U);
  REQUIRE(transportationType.toString() == L"TransportationTypeHandle(2)");
  MomServiceArgument supplied{
      MomArgumentType::transportation_type_handle,
      L"Transportation type",
      formatMomTransportationTypeHandle(transportationType),
  };
  REQUIRE(formatMomServiceArgumentRecord(supplied) ==
          L"{\"HLAargumentType\":59,\"HLAargumentName\":\"Transportation type\","
          L"\"HLAargumentValue\":\"TransportationTypeHandle(2)\"}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 ObjectInstanceHandle and AttributeHandleSet argument forms",
    "[mom][encoding][service-reporting][unit][object-management]"
    "[request-attribute-transportation-type-change]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  REQUIRE(objectInstance.toString() == L"ObjectInstanceHandle(345)");
  MomServiceArgument objectInstanceArgument{
      MomArgumentType::object_instance_handle,
      L"Object instance designator",
      formatMomObjectInstanceHandle(objectInstance),
  };
  REQUIRE(formatMomServiceArgumentRecord(objectInstanceArgument) ==
          L"{\"HLAargumentType\":37,\"HLAargumentName\":"
          L"\"Object instance designator\",\"HLAargumentValue\":"
          L"\"ObjectInstanceHandle(345)\"}");

  rti1516_2025::AttributeHandleSet const attributes{
      makeAttributeHandle(2U), makeAttributeHandle(1U)};
  MomServiceArgument attributeSetArgument{
      MomArgumentType::attribute_handle_set,
      L"Set of attribute designators",
      formatMomAttributeHandleSet(attributes),
  };
  REQUIRE(formatMomServiceArgumentRecord(attributeSetArgument) ==
          L"{\"HLAargumentType\":1,\"HLAargumentName\":"
          L"\"Set of attribute designators\",\"HLAargumentValue\":["
          L"\"AttributeHandle(1)\",\"AttributeHandle(2)\"]}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Update Attribute Values argument forms",
    "[mom][encoding][service-reporting][unit][object-management]"
    "[update-attribute-values]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  unsigned char const firstValueBytes[] = {0x01U, 0x02U};
  unsigned char const secondValueBytes[] = {0xffU, 0x00U};
  rti1516_2025::AttributeHandleValueMap values;
  values.emplace(
      makeAttributeHandle(2U),
      rti1516_2025::VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));
  values.emplace(
      makeAttributeHandle(1U),
      rti1516_2025::VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));

  REQUIRE(formatMomBinaryData(rti1516_2025::VariableLengthData{}) == L"\"\"");
  REQUIRE(formatMomAttributeHandleValueMap(values) ==
          L"{\"AttributeHandle(1)\":\"AQI=\",\"AttributeHandle(2)\":\"/wA=\"}");

  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::attribute_handle_value_map,
       L"Constrained set of attribute designator and value pairs",
       formatMomAttributeHandleValueMap(values)},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(rti1516_2025::VariableLengthData{})},
      {MomArgumentType::null_value,
       L"Optional timestamp",
       formatMomNull()},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"UpdateAttributeValues", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"UpdateAttributeValues\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":2,\"HLAargumentName\":"
          L"\"Constrained set of attribute designator and value pairs\","
          L"\"HLAargumentValue\":{\"AttributeHandle(1)\":\"AQI=\","
          L"\"AttributeHandle(2)\":\"/wA=\"}},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"\"},"
          L"{\"HLAargumentType\":34,\"HLAargumentName\":\"Optional timestamp\","
          L"\"HLAargumentValue\":null}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Send Interaction argument forms",
    "[mom][encoding][service-reporting][unit][object-management]"
    "[send-interaction]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeInteractionClassHandle;
  using rti1516_2025::umbra_binding_detail::makeParameterHandle;

  auto const interactionClass = makeInteractionClassHandle(345U);
  unsigned char const firstValueBytes[] = {0x01U, 0x02U};
  unsigned char const secondValueBytes[] = {0xffU, 0x00U};
  rti1516_2025::ParameterHandleValueMap values;
  values.emplace(
      makeParameterHandle(2U),
      rti1516_2025::VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));
  values.emplace(
      makeParameterHandle(1U),
      rti1516_2025::VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));

  REQUIRE(formatMomParameterHandleValueMap(values) ==
          L"{\"ParameterHandle(1)\":\"AQI=\",\"ParameterHandle(2)\":\"/wA=\"}");

  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       formatMomInteractionClassHandle(interactionClass)},
      {MomArgumentType::parameter_handle_value_map,
       L"Constrained set of interaction parameter designator and value pairs",
       formatMomParameterHandleValueMap(values)},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(rti1516_2025::VariableLengthData{})},
      {MomArgumentType::null_value,
       L"Optional timestamp",
       formatMomNull()},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"SendInteraction", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"SendInteraction\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":27,\"HLAargumentName\":\"Interaction class designator\","
          L"\"HLAargumentValue\":\"InteractionClassHandle(345)\"},"
          L"{\"HLAargumentType\":40,\"HLAargumentName\":"
          L"\"Constrained set of interaction parameter designator and value pairs\","
          L"\"HLAargumentValue\":{\"ParameterHandle(1)\":\"AQI=\","
          L"\"ParameterHandle(2)\":\"/wA=\"}},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"\"},"
          L"{\"HLAargumentType\":34,\"HLAargumentName\":\"Optional timestamp\","
          L"\"HLAargumentValue\":null}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Send Directed Interaction argument forms",
    "[mom][encoding][service-reporting][unit][object-management]"
    "[send-directed-interaction]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeInteractionClassHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;
  using rti1516_2025::umbra_binding_detail::makeParameterHandle;

  auto const interactionClass = makeInteractionClassHandle(345U);
  auto const objectInstance = makeObjectInstanceHandle(678U);
  unsigned char const firstValueBytes[] = {0x01U, 0x02U};
  unsigned char const secondValueBytes[] = {0xffU, 0x00U};
  rti1516_2025::ParameterHandleValueMap values;
  values.emplace(
      makeParameterHandle(2U),
      rti1516_2025::VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));
  values.emplace(
      makeParameterHandle(1U),
      rti1516_2025::VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));

  REQUIRE(formatMomParameterHandleValueMap(values) ==
          L"{\"ParameterHandle(1)\":\"AQI=\",\"ParameterHandle(2)\":\"/wA=\"}");

  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       formatMomInteractionClassHandle(interactionClass)},
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::parameter_handle_value_map,
       L"Constrained set of interaction parameter designator and value pairs",
       formatMomParameterHandleValueMap(values)},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(rti1516_2025::VariableLengthData{})},
      {MomArgumentType::null_value,
       L"Optional timestamp",
       formatMomNull()},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"SendDirectedInteraction", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"SendDirectedInteraction\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":27,\"HLAargumentName\":\"Interaction class designator\","
          L"\"HLAargumentValue\":\"InteractionClassHandle(345)\"},"
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(678)\"},"
          L"{\"HLAargumentType\":40,\"HLAargumentName\":"
          L"\"Constrained set of interaction parameter designator and value pairs\","
          L"\"HLAargumentValue\":{\"ParameterHandle(1)\":\"AQI=\","
          L"\"ParameterHandle(2)\":\"/wA=\"}},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"\"},"
          L"{\"HLAargumentType\":34,\"HLAargumentName\":\"Optional timestamp\","
          L"\"HLAargumentValue\":null}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Delete Object Instance argument forms",
    "[mom][encoding][service-reporting][unit][object-management]"
    "[delete-object-instance]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  unsigned char const tagBytes[] = {'r', 'e', 'p'};
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(
           rti1516_2025::VariableLengthData(tagBytes, sizeof(tagBytes)))},
      {MomArgumentType::null_value,
       L"Optional timestamp",
       formatMomNull()},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"DeleteObjectInstance", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"DeleteObjectInstance\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"cmVw\"},"
          L"{\"HLAargumentType\":34,\"HLAargumentName\":\"Optional timestamp\","
          L"\"HLAargumentValue\":null}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use Remove Object Instance descriptor form",
    "[mom][encoding][service-reporting][unit][object-management]"
    "[remove-object-instance-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeFederateHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  auto const producingFederate = makeFederateHandle(456U);
  unsigned char const tagBytes[] = {'r', 'e', 'p'};
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(
           rti1516_2025::VariableLengthData(tagBytes, sizeof(tagBytes)))},
      {MomArgumentType::order_type,
       L"Sent message order type",
       formatMomOrderType(rti1516_2025::RECEIVE)},
      {MomArgumentType::federate_handle,
       L"Producing joined federate designator",
       formatMomFederateHandle(producingFederate)},
      {MomArgumentType::null_value,
       L"Optional timestamp",
       formatMomNull()},
      {MomArgumentType::null_value,
       L"Optional receive message order type",
       formatMomNull()},
      {MomArgumentType::null_value,
       L"Optional message retraction designator",
       formatMomNull()},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"RemoveObjectInstance", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"RemoveObjectInstance\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"cmVw\"},"
          L"{\"HLAargumentType\":38,\"HLAargumentName\":\"Sent message order type\","
          L"\"HLAargumentValue\":\"RECEIVE\"},"
          L"{\"HLAargumentType\":15,\"HLAargumentName\":"
          L"\"Producing joined federate designator\","
          L"\"HLAargumentValue\":\"FederateHandle(456)\"},"
          L"{\"HLAargumentType\":34,\"HLAargumentName\":\"Optional timestamp\","
          L"\"HLAargumentValue\":null},"
          L"{\"HLAargumentType\":34,\"HLAargumentName\":"
          L"\"Optional receive message order type\",\"HLAargumentValue\":null},"
          L"{\"HLAargumentType\":34,\"HLAargumentName\":"
          L"\"Optional message retraction designator\",\"HLAargumentValue\":null}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use timestamped Remove Object Instance descriptor form",
    "[mom][encoding][service-reporting][unit][object-management][time-management]"
    "[timestamped-remove-object-instance-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeFederateHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  auto const producingFederate = makeFederateHandle(456U);
  unsigned char const tagBytes[] = {'r', 'e', 'p'};
  rti1516_2025::HLAinteger64Time timestamp{7};
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(
           rti1516_2025::VariableLengthData(tagBytes, sizeof(tagBytes)))},
      {MomArgumentType::order_type,
       L"Sent message order type",
       formatMomOrderType(rti1516_2025::TIMESTAMP)},
      {MomArgumentType::federate_handle,
       L"Producing joined federate designator",
       formatMomFederateHandle(producingFederate)},
      {MomArgumentType::logical_time,
       L"Optional timestamp",
       formatMomLogicalTime(timestamp)},
      {MomArgumentType::order_type,
       L"Optional receive message order type",
       formatMomOrderType(rti1516_2025::TIMESTAMP)},
      {MomArgumentType::message_retraction_handle,
       L"Optional message retraction designator",
       formatMomMessageRetractionHandle(42U)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              3U, L"RemoveObjectInstance", supplied) ==
          L"{\"HLAserialNumber\":3,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"RemoveObjectInstance\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"cmVw\"},"
          L"{\"HLAargumentType\":38,\"HLAargumentName\":\"Sent message order type\","
          L"\"HLAargumentValue\":\"TIMESTAMP\"},"
          L"{\"HLAargumentType\":15,\"HLAargumentName\":"
          L"\"Producing joined federate designator\","
          L"\"HLAargumentValue\":\"FederateHandle(456)\"},"
          L"{\"HLAargumentType\":31,\"HLAargumentName\":\"Optional timestamp\","
          L"\"HLAargumentValue\":\"7\"},"
          L"{\"HLAargumentType\":38,\"HLAargumentName\":"
          L"\"Optional receive message order type\",\"HLAargumentValue\":\"TIMESTAMP\"},"
          L"{\"HLAargumentType\":33,\"HLAargumentName\":"
          L"\"Optional message retraction designator\","
          L"\"HLAargumentValue\":\"MessageRetractionHandle<42>\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use Discover Object Instance descriptor form",
    "[mom][encoding][service-reporting][unit][object-management]"
    "[discover-object-instance-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeFederateHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectClassHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  auto const objectClass = makeObjectClassHandle(456U);
  auto const producingFederate = makeFederateHandle(12U);
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance handle",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::object_class_handle,
       L"Object class designator",
       formatMomObjectClassHandle(objectClass)},
      {MomArgumentType::string,
       L"Object instance name",
       formatMomString(L"report-object")},
      {MomArgumentType::federate_handle,
       L"Producing joined federate designator",
       formatMomFederateHandle(producingFederate)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"DiscoverObjectInstance", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"DiscoverObjectInstance\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance handle\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":36,\"HLAargumentName\":\"Object class designator\","
          L"\"HLAargumentValue\":\"ObjectClassHandle(456)\"},"
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Object instance name\","
          L"\"HLAargumentValue\":\"report-object\"},"
          L"{\"HLAargumentType\":15,"
          L"\"HLAargumentName\":\"Producing joined federate designator\","
          L"\"HLAargumentValue\":\"FederateHandle(12)\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use Provide Attribute Value Update descriptor form",
    "[mom][encoding][service-reporting][unit][object-management]"
    "[provide-attribute-value-update-service-report]"
    "[provide-attribute-value-update-class-service-report]"
    "[provide-attribute-value-update-regional-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  rti1516_2025::AttributeHandleSet const attributes{
      makeAttributeHandle(5U), makeAttributeHandle(4U)};
  unsigned char const tagBytes[] = {'r', 'e', 'p'};
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       formatMomAttributeHandleSet(attributes)},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(
           rti1516_2025::VariableLengthData(tagBytes, sizeof(tagBytes)))},
  };

  // §6.22.1 supplies exactly the object, attribute set, and propagated tag.
  // The Table 5 type-63 tag literal remains a file-text assertion only; the
  // bundled MIM's static UserSuppliedTag value differs (RL-077).
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              3U, L"ProvideAttributeValueUpdate", supplied) ==
          L"{\"HLAserialNumber\":3,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"ProvideAttributeValueUpdate\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":1,\"HLAargumentName\":\"Set of attribute designators\","
          L"\"HLAargumentValue\":[\"AttributeHandle(4)\",\"AttributeHandle(5)\"]},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"cmVw\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files preserve the empty Auto Provide tag descriptor form",
    "[mom][encoding][service-reporting][unit][object-management][auto-provide]"
    "[auto-provide-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  rti1516_2025::AttributeHandleSet const attributes{makeAttributeHandle(4U)};
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       formatMomAttributeHandleSet(attributes)},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(rti1516_2025::VariableLengthData())},
  };

  // §1.5 requires the tag to be present but zero-length when §6.22 is
  // RTI-invoked by Auto Provide rather than corresponding to a requester call.
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"ProvideAttributeValueUpdate", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"ProvideAttributeValueUpdate\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":1,\"HLAargumentName\":\"Set of attribute designators\","
          L"\"HLAargumentValue\":[\"AttributeHandle(4)\"]},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 InteractionClassHandleSet argument form",
    "[mom][encoding][service-reporting][unit][declaration-management]"
    "[publish-object-class-directed-interactions]"
    "[unpublish-object-class-directed-interactions]"
    "[subscribe-object-class-directed-interactions]"
    "[unsubscribe-object-class-directed-interactions]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeInteractionClassHandle;

  rti1516_2025::InteractionClassHandleSet const interactionClasses{
      makeInteractionClassHandle(2U), makeInteractionClassHandle(1U)};
  MomServiceArgument interactionClassSetArgument{
      MomArgumentType::interaction_class_handle_set,
      L"Set of interaction class designators",
      formatMomInteractionClassHandleSet(interactionClasses),
  };
  REQUIRE(formatMomServiceArgumentRecord(interactionClassSetArgument) ==
          L"{\"HLAargumentType\":28,\"HLAargumentName\":"
          L"\"Set of interaction class designators\",\"HLAargumentValue\":["
          L"\"InteractionClassHandle(1)\",\"InteractionClassHandle(2)\"]}");
}

TEST_CASE(
    "MOM service-report files use declaration relevance advisory descriptor forms",
    "[mom][encoding][service-reporting][unit][declaration-management]"
    "[declaration-relevance-advisory-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeInteractionClassHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectClassHandle;

  auto const objectClass = makeObjectClassHandle(456U);
  auto const interactionClass = makeInteractionClassHandle(789U);
  auto const objectClassArgument = std::vector<MomServiceArgument>{
      {MomArgumentType::object_class_handle,
       L"Object class designator",
       formatMomObjectClassHandle(objectClass)},
  };
  auto const interactionClassArgument = std::vector<MomServiceArgument>{
      {MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       formatMomInteractionClassHandle(interactionClass)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"StartRegistrationForObjectClass", objectClassArgument) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"StartRegistrationForObjectClass\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":36,\"HLAargumentName\":\"Object class designator\","
          L"\"HLAargumentValue\":\"ObjectClassHandle(456)\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              1U, L"StopRegistrationForObjectClass", objectClassArgument) ==
          L"{\"HLAserialNumber\":1,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"StopRegistrationForObjectClass\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":36,\"HLAargumentName\":\"Object class designator\","
          L"\"HLAargumentValue\":\"ObjectClassHandle(456)\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              2U, L"TurnInteractionsOn", interactionClassArgument) ==
          L"{\"HLAserialNumber\":2,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"TurnInteractionsOn\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":27,\"HLAargumentName\":\"Interaction class designator\","
          L"\"HLAargumentValue\":\"InteractionClassHandle(789)\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              3U, L"TurnInteractionsOff", interactionClassArgument) ==
          L"{\"HLAserialNumber\":3,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"TurnInteractionsOff\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":27,\"HLAargumentName\":\"Interaction class designator\","
          L"\"HLAargumentValue\":\"InteractionClassHandle(789)\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Change Attribute Order Type argument forms",
    "[mom][encoding][service-reporting][unit][object-management][time-management]"
    "[change-attribute-order-type]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  rti1516_2025::AttributeHandleSet const attributes{
      makeAttributeHandle(2U), makeAttributeHandle(1U)};
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       formatMomAttributeHandleSet(attributes)},
      {MomArgumentType::order_type,
       L"Order type",
       formatMomOrderType(rti1516_2025::TIMESTAMP)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"ChangeAttributeOrderType", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"ChangeAttributeOrderType\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":1,\"HLAargumentName\":\"Set of attribute designators\","
          L"\"HLAargumentValue\":[\"AttributeHandle(1)\",\"AttributeHandle(2)\"]},"
          L"{\"HLAargumentType\":38,\"HLAargumentName\":\"Order type\","
          L"\"HLAargumentValue\":\"TIMESTAMP\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Query Attribute Transportation Type argument forms",
    "[mom][encoding][service-reporting][unit][object-management][transportation-management]"
    "[query-attribute-transportation-type]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  auto const attribute = makeAttributeHandle(456U);
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::attribute_handle,
       L"Attribute designator",
       formatMomAttributeHandle(attribute)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"QueryAttributeTransportationType", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"QueryAttributeTransportationType\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":0,\"HLAargumentName\":\"Attribute designator\","
          L"\"HLAargumentValue\":\"AttributeHandle(456)\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Query Attribute Ownership argument forms",
    "[mom][encoding][service-reporting][unit][ownership-management]"
    "[query-attribute-ownership]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  rti1516_2025::AttributeHandleSet const attributes{
      makeAttributeHandle(2U), makeAttributeHandle(1U)};
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       formatMomAttributeHandleSet(attributes)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"QueryAttributeOwnership", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"QueryAttributeOwnership\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":1,\"HLAargumentName\":\"Set of attribute designators\","
          L"\"HLAargumentValue\":[\"AttributeHandle(1)\",\"AttributeHandle(2)\"]}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Unconditional Attribute Ownership Divestiture argument forms",
    "[mom][encoding][service-reporting][unit][ownership-management]"
    "[unconditional-attribute-ownership-divestiture]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  rti1516_2025::AttributeHandleSet const attributes{
      makeAttributeHandle(2U), makeAttributeHandle(1U)};
  unsigned char const tagBytes[] = {0x00U, 0xffU, 0x10U, 0xa5U};
  rti1516_2025::VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  REQUIRE(formatMomUserSuppliedTag(tag) == L"\"AP8QpQ==\"");
  REQUIRE(formatMomUserSuppliedTag(rti1516_2025::VariableLengthData{}) == L"\"\"");

  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       formatMomAttributeHandleSet(attributes)},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(tag)},
  };

  // The type-63 literal is Table 5's service-report example. It deliberately
  // remains a file-text assertion only: the companion standard MIM uses 60 for
  // UserSuppliedTag (RL-077).
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"UnconditionalAttributeOwnershipDivestiture", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"UnconditionalAttributeOwnershipDivestiture\","
          L"\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":1,\"HLAargumentName\":\"Set of attribute designators\","
          L"\"HLAargumentValue\":[\"AttributeHandle(1)\",\"AttributeHandle(2)\"]},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"AP8QpQ==\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Cancel Attribute Ownership Acquisition argument forms",
    "[mom][encoding][service-reporting][unit][ownership-management]"
    "[cancel-attribute-ownership-acquisition]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  rti1516_2025::AttributeHandleSet const attributes{
      makeAttributeHandle(2U), makeAttributeHandle(1U)};
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       formatMomAttributeHandleSet(attributes)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"CancelAttributeOwnershipAcquisition", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"CancelAttributeOwnershipAcquisition\","
          L"\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":1,\"HLAargumentName\":\"Set of attribute designators\","
          L"\"HLAargumentValue\":[\"AttributeHandle(1)\",\"AttributeHandle(2)\"]}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Attribute Ownership Acquisition argument forms",
    "[mom][encoding][service-reporting][unit][ownership-management]"
    "[attribute-ownership-acquisition]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  rti1516_2025::AttributeHandleSet const attributes{
      makeAttributeHandle(2U), makeAttributeHandle(1U)};
  unsigned char const tagBytes[] = {0x00U, 0xffU, 0x10U, 0xa5U};
  rti1516_2025::VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       formatMomAttributeHandleSet(attributes)},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(tag)},
  };

  // Table 5's literal type 63 for the Binary Data tag remains a private
  // file-text assertion only; the bundled MIM's UserSuppliedTag value differs
  // (RL-077).
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"AttributeOwnershipAcquisition", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"AttributeOwnershipAcquisition\","
          L"\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":1,\"HLAargumentName\":\"Set of attribute designators\","
          L"\"HLAargumentValue\":[\"AttributeHandle(1)\",\"AttributeHandle(2)\"]},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"AP8QpQ==\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Negotiated Attribute Ownership Divestiture argument forms",
    "[mom][encoding][service-reporting][unit][ownership-management]"
    "[negotiated-attribute-ownership-divestiture]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  rti1516_2025::AttributeHandleSet const attributes{
      makeAttributeHandle(2U), makeAttributeHandle(1U)};
  unsigned char const tagBytes[] = {0x00U, 0xffU, 0x10U, 0xa5U};
  rti1516_2025::VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       formatMomAttributeHandleSet(attributes)},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(tag)},
  };

  // §7.3.1 supplies object, attribute set, and tag; §7.3.2 returns None.
  // The Table 5 type-63 tag literal remains a private file-text assertion
  // because the bundled MIM's UserSuppliedTag value differs (RL-077).
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"NegotiatedAttributeOwnershipDivestiture", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"NegotiatedAttributeOwnershipDivestiture\","
          L"\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":1,\"HLAargumentName\":\"Set of attribute designators\","
          L"\"HLAargumentValue\":[\"AttributeHandle(1)\",\"AttributeHandle(2)\"]},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"AP8QpQ==\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Confirm Divestiture argument forms",
    "[mom][encoding][service-reporting][unit][ownership-management][confirm-divestiture]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  rti1516_2025::AttributeHandleSet const attributes{
      makeAttributeHandle(2U), makeAttributeHandle(1U)};
  unsigned char const tagBytes[] = {0x00U, 0xffU, 0x10U, 0xa5U};
  rti1516_2025::VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       formatMomAttributeHandleSet(attributes)},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(tag)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(0U, L"ConfirmDivestiture", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"ConfirmDivestiture\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":1,\"HLAargumentName\":\"Set of attribute designators\","
          L"\"HLAargumentValue\":[\"AttributeHandle(1)\",\"AttributeHandle(2)\"]},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"AP8QpQ==\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Synchronization Point Achieved argument forms",
    "[mom][encoding][service-reporting][unit][federation-management]"
    "[synchronization-point-achieved]") {
  using namespace umbra::detail;

  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::string,
       L"Synchronization point label",
       formatMomString(L"startup")},
      {MomArgumentType::boolean,
       L"Optional synchronization-success indicator",
       formatMomBoolean(false)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"SynchronizationPointAchieved", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"SynchronizationPointAchieved\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Synchronization point label\","
          L"\"HLAargumentValue\":\"startup\"},"
          L"{\"HLAargumentType\":6,\"HLAargumentName\":"
          L"\"Optional synchronization-success indicator\",\"HLAargumentValue\":false}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files preserve Register Federation Synchronization Point optional-set form",
    "[mom][encoding][service-reporting][unit][federation-management]"
    "[register-federation-synchronization-point]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeFederateHandle;

  unsigned char const tagBytes[] = {0x53U, 0x59U, 0x4eU, 0x43U};
  rti1516_2025::VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  rti1516_2025::FederateHandleSet const suppliedSet{
      makeFederateHandle(2U), makeFederateHandle(1U)};

  auto const suppliedWithoutSet = std::vector<MomServiceArgument>{
      {MomArgumentType::string,
       L"Synchronization point label",
       formatMomString(L"global")},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(tag)},
      {MomArgumentType::null_value,
       L"Optional set of joined federate designators",
       formatMomNull()},
  };
  auto const suppliedWithSet = std::vector<MomServiceArgument>{
      {MomArgumentType::string,
       L"Synchronization point label",
       formatMomString(L"explicit")},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(tag)},
      {MomArgumentType::federate_handle_set,
       L"Optional set of joined federate designators",
       formatMomFederateHandleSet(suppliedSet)},
  };

  // §4.14.1 supplies the optional set as the third argument. §11.5.1
  // requires the no-set overload to keep that position as Null, rather than
  // treating it as the semantically equivalent but actually supplied empty
  // FederateHandleSet.
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"RegisterFederationSynchronizationPoint", suppliedWithoutSet) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"RegisterFederationSynchronizationPoint\","
          L"\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Synchronization point label\","
          L"\"HLAargumentValue\":\"global\"},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"U1lOQw==\"},"
          L"{\"HLAargumentType\":34,\"HLAargumentName\":"
          L"\"Optional set of joined federate designators\",\"HLAargumentValue\":null}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              1U, L"RegisterFederationSynchronizationPoint", suppliedWithSet) ==
          L"{\"HLAserialNumber\":1,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"RegisterFederationSynchronizationPoint\","
          L"\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Synchronization point label\","
          L"\"HLAargumentValue\":\"explicit\"},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"U1lOQw==\"},"
          L"{\"HLAargumentType\":18,\"HLAargumentName\":"
          L"\"Optional set of joined federate designators\",\"HLAargumentValue\":["
          L"\"FederateHandle(1)\",\"FederateHandle(2)\"]}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use Announce Synchronization Point argument form",
    "[mom][encoding][service-reporting][unit][federation-management]"
    "[announce-synchronization-point-service-report]") {
  using namespace umbra::detail;

  unsigned char const tagBytes[] = {0x53U, 0x59U, 0x4eU, 0x43U};
  rti1516_2025::VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::string,
       L"Synchronization point label",
       formatMomString(L"startup")},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(tag)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"AnnounceSynchronizationPoint", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"AnnounceSynchronizationPoint\","
          L"\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Synchronization point label\","
          L"\"HLAargumentValue\":\"startup\"},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"U1lOQw==\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use Federation Synchronized argument form",
    "[mom][encoding][service-reporting][unit][federation-management]"
    "[federation-synchronized-service-report]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeFederateHandle;

  rti1516_2025::FederateHandleSet const failedToSyncSet{
      makeFederateHandle(2U), makeFederateHandle(1U)};
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::string,
       L"Synchronization point label",
       formatMomString(L"startup")},
      {MomArgumentType::federate_handle_set,
       L"Set of joined federate designators",
       formatMomFederateHandleSet(failedToSyncSet)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"FederationSynchronized", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"FederationSynchronized\","
          L"\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Synchronization point label\","
          L"\"HLAargumentValue\":\"startup\"},"
          L"{\"HLAargumentType\":18,\"HLAargumentName\":\"Set of joined federate designators\","
          L"\"HLAargumentValue\":[\"FederateHandle(1)\",\"FederateHandle(2)\"]}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use Confirm Synchronization Point Registration result forms",
    "[mom][encoding][service-reporting][unit][federation-management]"
    "[confirm-synchronization-point-registration-service-report]") {
  using namespace umbra::detail;

  // Section 4.15 supplies label, success indicator, and an optional failure
  // reason. Section 11.5.1 retains the unused optional position as Null;
  // Table 14/Table 5 assign the failure enumeration type 56 and quoted
  // spelling. Its display labels remain implementation-defined descriptions.
  auto const succeeded = std::vector<MomServiceArgument>{
      {MomArgumentType::string,
       L"Synchronization point label",
       formatMomString(L"startup")},
      {MomArgumentType::boolean,
       L"Registration-success indicator",
       formatMomBoolean(true)},
      {MomArgumentType::null_value,
       L"Optional failure reason",
       formatMomNull()},
  };
  auto const failed = std::vector<MomServiceArgument>{
      {MomArgumentType::string,
       L"Synchronization point label",
       formatMomString(L"startup")},
      {MomArgumentType::boolean,
       L"Registration-success indicator",
       formatMomBoolean(false)},
      {MomArgumentType::synchronization_point_failure_reason,
       L"Optional failure reason",
       formatMomSynchronizationPointFailureReason(
           rti1516_2025::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              1U, L"ConfirmSynchronizationPointRegistration", succeeded) ==
          L"{\"HLAserialNumber\":1,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"ConfirmSynchronizationPointRegistration\","
          L"\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Synchronization point label\","
          L"\"HLAargumentValue\":\"startup\"},"
          L"{\"HLAargumentType\":6,\"HLAargumentName\":\"Registration-success indicator\","
          L"\"HLAargumentValue\":true},"
          L"{\"HLAargumentType\":34,\"HLAargumentName\":\"Optional failure reason\","
          L"\"HLAargumentValue\":null}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              3U, L"ConfirmSynchronizationPointRegistration", failed) ==
          L"{\"HLAserialNumber\":3,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"ConfirmSynchronizationPointRegistration\","
          L"\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":53,\"HLAargumentName\":\"Synchronization point label\","
          L"\"HLAargumentValue\":\"startup\"},"
          L"{\"HLAargumentType\":6,\"HLAargumentName\":\"Registration-success indicator\","
          L"\"HLAargumentValue\":false},"
          L"{\"HLAargumentType\":56,\"HLAargumentName\":\"Optional failure reason\","
          L"\"HLAargumentValue\":\"SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Cancel Negotiated Attribute Ownership Divestiture argument forms",
    "[mom][encoding][service-reporting][unit][ownership-management]"
    "[cancel-negotiated-attribute-ownership-divestiture]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  rti1516_2025::AttributeHandleSet const attributes{
      makeAttributeHandle(2U), makeAttributeHandle(1U)};
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       formatMomAttributeHandleSet(attributes)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"CancelNegotiatedAttributeOwnershipDivestiture", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"CancelNegotiatedAttributeOwnershipDivestiture\","
          L"\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":1,\"HLAargumentName\":\"Set of attribute designators\","
          L"\"HLAargumentValue\":[\"AttributeHandle(1)\",\"AttributeHandle(2)\"]}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Attribute Ownership Acquisition If Available argument forms",
    "[mom][encoding][service-reporting][unit][ownership-management]"
    "[attribute-ownership-acquisition-if-available]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  rti1516_2025::AttributeHandleSet const attributes{
      makeAttributeHandle(2U), makeAttributeHandle(1U)};
  unsigned char const tagBytes[] = {0x00U, 0xffU, 0x10U, 0xa5U};
  rti1516_2025::VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       formatMomAttributeHandleSet(attributes)},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(tag)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"AttributeOwnershipAcquisitionIfAvailable", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"AttributeOwnershipAcquisitionIfAvailable\","
          L"\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":1,\"HLAargumentName\":\"Set of attribute designators\","
          L"\"HLAargumentValue\":[\"AttributeHandle(1)\",\"AttributeHandle(2)\"]},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"AP8QpQ==\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Attribute Ownership Release Denied argument forms",
    "[mom][encoding][service-reporting][unit][ownership-management]"
    "[attribute-ownership-release-denied]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle;

  auto const objectInstance = makeObjectInstanceHandle(345U);
  rti1516_2025::AttributeHandleSet const attributes{
      makeAttributeHandle(2U), makeAttributeHandle(1U)};
  unsigned char const tagBytes[] = {0x00U, 0xffU, 0x10U, 0xa5U};
  rti1516_2025::VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_instance_handle,
       L"Object instance designator",
       formatMomObjectInstanceHandle(objectInstance)},
      {MomArgumentType::attribute_handle_set,
       L"Set of attribute designators for which the joined federate is unwilling to divest ownership",
       formatMomAttributeHandleSet(attributes)},
      {MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       formatMomUserSuppliedTag(tag)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"AttributeOwnershipReleaseDenied", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"AttributeOwnershipReleaseDenied\","
          L"\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":37,\"HLAargumentName\":\"Object instance designator\","
          L"\"HLAargumentValue\":\"ObjectInstanceHandle(345)\"},"
          L"{\"HLAargumentType\":1,\"HLAargumentName\":\"Set of attribute designators for which the joined federate is unwilling to divest ownership\","
          L"\"HLAargumentValue\":[\"AttributeHandle(1)\",\"AttributeHandle(2)\"]},"
          L"{\"HLAargumentType\":63,\"HLAargumentName\":\"User-supplied tag\","
          L"\"HLAargumentValue\":\"AP8QpQ==\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Query Interaction Transportation Type argument forms",
    "[mom][encoding][service-reporting][unit][object-management][transportation-management]"
    "[query-interaction-transportation-type]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeFederateHandle;
  using rti1516_2025::umbra_binding_detail::makeInteractionClassHandle;

  auto const federate = makeFederateHandle(345U);
  auto const interactionClass = makeInteractionClassHandle(456U);
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::federate_handle,
       L"Federate designator",
       formatMomFederateHandle(federate)},
      {MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       formatMomInteractionClassHandle(interactionClass)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"QueryInteractionTransportationType", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"QueryInteractionTransportationType\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":15,\"HLAargumentName\":\"Federate designator\","
          L"\"HLAargumentValue\":\"FederateHandle(345)\"},"
          L"{\"HLAargumentType\":27,\"HLAargumentName\":\"Interaction class designator\","
          L"\"HLAargumentValue\":\"InteractionClassHandle(456)\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Change Default Attribute Order Type argument forms",
    "[mom][encoding][service-reporting][unit][object-management][time-management]"
    "[change-default-attribute-order-type]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectClassHandle;

  auto const objectClass = makeObjectClassHandle(456U);
  rti1516_2025::AttributeHandleSet const attributes{
      makeAttributeHandle(2U), makeAttributeHandle(1U)};
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_class_handle,
       L"Object class designator",
       formatMomObjectClassHandle(objectClass)},
      {MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       formatMomAttributeHandleSet(attributes)},
      {MomArgumentType::order_type,
       L"Order type",
       formatMomOrderType(rti1516_2025::TIMESTAMP)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"ChangeDefaultAttributeOrderType", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"ChangeDefaultAttributeOrderType\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":36,\"HLAargumentName\":\"Object class designator\","
          L"\"HLAargumentValue\":\"ObjectClassHandle(456)\"},"
          L"{\"HLAargumentType\":1,\"HLAargumentName\":\"Set of attribute designators\","
          L"\"HLAargumentValue\":[\"AttributeHandle(1)\",\"AttributeHandle(2)\"]},"
          L"{\"HLAargumentType\":38,\"HLAargumentName\":\"Order type\","
          L"\"HLAargumentValue\":\"TIMESTAMP\"}],\"HLAsuccessIndicator\":true,"
          L"\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 Change Default Attribute Transportation Type argument forms",
    "[mom][encoding][service-reporting][unit][object-management][transportation-management]"
    "[change-default-attribute-transportation-type]") {
  using namespace umbra::detail;
  using rti1516_2025::umbra_binding_detail::makeAttributeHandle;
  using rti1516_2025::umbra_binding_detail::makeObjectClassHandle;
  using rti1516_2025::umbra_binding_detail::makeTransportationTypeHandle;

  auto const objectClass = makeObjectClassHandle(456U);
  rti1516_2025::AttributeHandleSet const attributes{
      makeAttributeHandle(2U), makeAttributeHandle(1U)};
  auto const transportationType = makeTransportationTypeHandle(2U);
  auto const supplied = std::vector<MomServiceArgument>{
      {MomArgumentType::object_class_handle,
       L"Object class designator",
       formatMomObjectClassHandle(objectClass)},
      {MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       formatMomAttributeHandleSet(attributes)},
      {MomArgumentType::transportation_type_handle,
       L"Transportation type",
       formatMomTransportationTypeHandle(transportationType)},
  };

  REQUIRE(formatMomSuccessfulVoidServiceReportRecord(
              0U, L"ChangeDefaultAttributeTransportationType", supplied) ==
          L"{\"HLAserialNumber\":0,\"HLAreturnedArgument\":[null],"
          L"\"HLAservice\":\"ChangeDefaultAttributeTransportationType\",\"HLAsuppliedArguments\":["
          L"{\"HLAargumentType\":36,\"HLAargumentName\":\"Object class designator\","
          L"\"HLAargumentValue\":\"ObjectClassHandle(456)\"},"
          L"{\"HLAargumentType\":1,\"HLAargumentName\":\"Set of attribute designators\","
          L"\"HLAargumentValue\":[\"AttributeHandle(1)\",\"AttributeHandle(2)\"]},"
          L"{\"HLAargumentType\":59,\"HLAargumentName\":\"Transportation type\","
          L"\"HLAargumentValue\":\"TransportationTypeHandle(2)\"}],"
          L"\"HLAsuccessIndicator\":true,\"HLAexception\":null}");
}

TEST_CASE(
    "MOM service-report files use the Table 5 MessageRetractionHandle argument form",
    "[mom][encoding][service-reporting][unit][time-management][retract]") {
  using namespace umbra::detail;

  MomServiceArgument supplied{
      MomArgumentType::message_retraction_handle,
      L"MessageRetractionDesignator",
      formatMomMessageRetractionHandle(2345),
  };

  REQUIRE(formatMomServiceArgumentRecord(supplied) ==
          L"{\"HLAargumentType\":33,\"HLAargumentName\":\"MessageRetractionDesignator\","
          L"\"HLAargumentValue\":\"MessageRetractionHandle<2345>\"}");
}

TEST_CASE(
    "MOM service-report records use Table 5 ResignAction spellings",
    "[mom][encoding][service-reporting][unit][support-switches][resign-action]") {
  using namespace umbra::detail;

  REQUIRE(formatMomResignAction(rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES) ==
          L"\"UNCONDITIONALLY_DIVEST_ATTRIBUTES\"");
  REQUIRE(formatMomResignAction(rti1516_2025::DELETE_OBJECTS) ==
          L"\"DELETE_OBJECTS\"");
  REQUIRE(formatMomResignAction(rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS) ==
          L"\"CANCEL_PENDING_OWNERSHIP_ACQUISITIONS\"");
  REQUIRE(formatMomResignAction(rti1516_2025::DELETE_OBJECTS_THEN_DIVEST) ==
          L"\"DELETE_OBJECTS_THEN_DIVEST\"");
  REQUIRE(formatMomResignAction(rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST) ==
          L"\"CANCEL_THEN_DELETE_THEN_DIVEST\"");
  REQUIRE(formatMomResignAction(rti1516_2025::NO_ACTION) == L"\"NO_ACTION\"");
}
