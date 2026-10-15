#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded public fresh-registry restore preserves two reserved object names through named registration",
    "[integration][development-profile][2025][federation-management]"
    "[object-management][save-restore][durable-save][filesystem][process-restart]"
    "[public-fresh-registry-object-name-reservation]"
    "[rti.service.reserve-object-instance-name]"
    "[rti.service.release-object-instance-name]"
    "[rti.service.register-object-instance]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete]"
    "[federate.callback.object-instance-name-reservation-succeeded]"
    "[federate.callback.object-instance-name-reservation-failed]") {
  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();
  std::wstring const firstName = L"saved-reserved-first";
  std::wstring const secondName = L"saved-reserved-second";
  std::wstring const saveLabel = L"two-reserved-object-names";
  FederateHandle sourceHandle;

  auto const drain = [](RTIambassador& rti) {
    for (int pass = 0; pass != 16; ++pass) {
      static_cast<void>(rti.evokeCallback(0.0));
    }
  };

  {
    auto registry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr, saveStore);
    ScopedEmbeddedFederationRegistry scope(registry);
    ReportingFederateAmbassador reports;
    auto rti = makeRti();
    REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
    REQUIRE_NOTHROW(rti->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceHandle = rti->joinFederationExecution(
        L"name-owner", L"publisher", federationName));

    REQUIRE_NOTHROW(rti->reserveObjectInstanceName(firstName));
    REQUIRE_NOTHROW(rti->reserveObjectInstanceName(secondName));
    drain(*rti);
    REQUIRE(reports.objectInstanceNameReservationSucceededReports.size() == 2U);
    REQUIRE(reports.objectInstanceNameReservationFailedReports.empty());

    REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(rti->federateSaveBegun());
    REQUIRE_NOTHROW(rti->federateSaveComplete());
    drain(*rti);
    REQUIRE(reports.federationSavedReportCount == 1U);
    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.reservedObjectInstanceNames.size() == 2U);

    REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(rti->disconnect());
  }

  {
    auto registry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr, saveStore);
    ScopedEmbeddedFederationRegistry scope(registry);
    ReportingFederateAmbassador reports;
    auto rti = makeRti();
    REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
    REQUIRE_NOTHROW(rti->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    FederateHandle freshHandle;
    REQUIRE_NOTHROW(freshHandle = rti->joinFederationExecution(
        L"name-owner", L"publisher", federationName));
    REQUIRE(freshHandle == sourceHandle);

    REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
    drain(*rti);
    REQUIRE(reports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(rti->federateRestoreComplete());
    drain(*rti);
    REQUIRE(reports.federationRestoredReportCount == 1U);

    REQUIRE_NOTHROW(rti->reserveObjectInstanceName(firstName));
    REQUIRE_NOTHROW(rti->reserveObjectInstanceName(secondName));
    drain(*rti);
    REQUIRE(reports.objectInstanceNameReservationFailedReports.size() == 2U);
    REQUIRE(reports.objectInstanceNameReservationSucceededReports.empty());

    auto const objectClass = rti->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const attribute = rti->getAttributeHandle(
        objectClass, fixture_hla::fixture::reliable_child);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE_NOTHROW(rti->publishObjectClassAttributes(
        objectClass, AttributeHandleSet{attribute}));
    ObjectInstanceHandle registered;
    REQUIRE_NOTHROW(registered =
        rti->registerObjectInstance(objectClass, firstName));
    REQUIRE(registered.isValid());
    REQUIRE(rti->getObjectInstanceName(registered) == firstName);
    REQUIRE_THROWS_AS(rti->releaseObjectInstanceName(firstName),
                      rti1516_2025::ObjectInstanceNameNotReserved);

    REQUIRE_NOTHROW(rti->releaseObjectInstanceName(secondName));
    REQUIRE_NOTHROW(rti->reserveObjectInstanceName(secondName));
    drain(*rti);
    REQUIRE(reports.objectInstanceNameReservationSucceededReports.size() == 1U);
    REQUIRE(reports.objectInstanceNameReservationSucceededReports.front()
                .objectInstanceName == secondName);

    REQUIRE_NOTHROW(rti->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(rti->disconnect());
  }
}
