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
