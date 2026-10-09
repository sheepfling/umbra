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

#include <RTI/auth/HLAplainTextPassword.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <initializer_list>
#include <set>
#include <tuple>
#include <vector>

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
