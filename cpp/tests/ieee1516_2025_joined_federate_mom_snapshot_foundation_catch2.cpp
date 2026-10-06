#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded joins retain an unpublished RTI-owned joined-federate MOM snapshot",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[mom-object-foundation][service-reporting]"
    "[joined-federate-mom-snapshot-foundation]") {
  using rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador;
  using namespace public_federation_restore_test_support;

  TestFederateAmbassador reports;
  auto rti = std::make_unique<UmbraRtiAmbassador>();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  RtiConfiguration configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  auto const firstFederate = rti->joinFederationExecution(
      L"mom-snapshot-subject", L"observer", federationName);
  auto firstSnapshot = rti->joinedFederateMomObjectSnapshotForTesting();
  REQUIRE(firstSnapshot);
  REQUIRE(firstSnapshot->objectInstanceHandle != 0U);
  REQUIRE(firstSnapshot->joinedFederateId != 0U);
  REQUIRE(firstSnapshot->objectClassHandle != 0U);
  REQUIRE(firstSnapshot->immutableFederatePoint.specificationCommitted);
  REQUIRE(firstSnapshot->immutableFederatePoint.dimensionHandles.size() == 1U);
  REQUIRE(firstSnapshot->immutableFederatePoint.committedRangeBounds.size() == 1U);
  auto const& pointRange = firstSnapshot->immutableFederatePoint.committedRangeBounds.begin()->second;
  REQUIRE(pointRange.upperBound == pointRange.lowerBound + 1U);

  // The seven Table 8 direct initial values are present as official MIM wire
  // encodings, including the public FederateHandle encoding and the exact
  // filesystem pathname. The empty dynamic module-designator array is the
  // correct value because this Join did not contribute an additional FOM.
  REQUIRE(firstSnapshot->initialAttributeValues.size() == 7U);
  auto containsEncodedValue = [&](VariableLengthData const& expected) {
    auto const expectedBytes = variableLengthDataBytes(expected);
    return std::any_of(
        firstSnapshot->initialAttributeValues.begin(),
        firstSnapshot->initialAttributeValues.end(),
        [&](auto const& attribute) {
          return variableLengthDataBytes(attribute.second) == expectedBytes;
        });
  };
  auto files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  REQUIRE(files.front().is_absolute());
  REQUIRE(files.front().parent_path() ==
          std::filesystem::absolute(directory.path()).lexically_normal());
  REQUIRE(containsEncodedValue(firstFederate.encode()));
  REQUIRE(containsEncodedValue(
      rti1516_2025::HLAunicodeString{L"mom-snapshot-subject"}.encode()));
  REQUIRE(containsEncodedValue(rti1516_2025::HLAunicodeString{L"observer"}.encode()));
  REQUIRE(containsEncodedValue(
      rti1516_2025::HLAunicodeString{L"umbra-embedded"}.encode()));
  REQUIRE(containsEncodedValue(rti1516_2025::HLAunicodeString{L"Umbra 0.1.0"}.encode()));
  REQUIRE(containsEncodedValue(rti1516_2025::HLAunicodeString{files.front().wstring()}.encode()));
  REQUIRE(containsEncodedValue(rti1516_2025::HLAinteger32BE{0}.encode()));

  // Switch changes gate later reporting only. They cannot replace the object
  // identity or its already allocated report-file value.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  auto const afterEnable = rti->joinedFederateMomObjectSnapshotForTesting();
  REQUIRE(afterEnable);
  REQUIRE(afterEnable->objectInstanceHandle == firstSnapshot->objectInstanceHandle);
  auto sameInitialValues = [](auto const& left, auto const& right) {
    if (left.size() != right.size()) {
      return false;
    }
    auto leftValue = left.begin();
    auto rightValue = right.begin();
    for (; leftValue != left.end(); ++leftValue, ++rightValue) {
      if (leftValue->first != rightValue->first ||
          variableLengthDataBytes(leftValue->second) !=
              variableLengthDataBytes(rightValue->second)) {
        return false;
      }
    }
    return true;
  };
  REQUIRE(sameInitialValues(
      afterEnable->initialAttributeValues,
      firstSnapshot->initialAttributeValues));

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_FALSE(rti->joinedFederateMomObjectSnapshotForTesting());
  auto const secondFederate = rti->joinFederationExecution(
      L"mom-snapshot-subject", L"observer", federationName);
  auto const secondSnapshot = rti->joinedFederateMomObjectSnapshotForTesting();
  REQUIRE(secondSnapshot);
  REQUIRE(secondFederate != firstFederate);
  REQUIRE(secondSnapshot->objectInstanceHandle != firstSnapshot->objectInstanceHandle);
  files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 2U);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
