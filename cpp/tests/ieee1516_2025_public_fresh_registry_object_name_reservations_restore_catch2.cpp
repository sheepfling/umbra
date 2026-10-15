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

TEST_CASE(
    "Embedded public immediate fresh-registry restore keeps an object-name reservation with its federate",
    "[integration][development-profile][2025][federation-management]"
    "[object-management][save-restore][durable-save][filesystem][process-restart]"
    "[public-immediate-fresh-registry-object-name-ownership]"
    "[callback-immediate][multi-federate]"
    "[rti.service.reserve-object-instance-name]"
    "[rti.service.release-object-instance-name]"
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
  std::wstring const reservedName = L"restored-owned-reservation";
  std::wstring const saveLabel = L"immediate-owned-reservation";
  FederateHandle sourceOwnerHandle;
  FederateHandle sourceOtherHandle;

  auto const flushImmediate = [](RTIambassador& rti) {
    REQUIRE(rti.getObjectClassHandle(
                fixture_hla::fom::attribute_fixture_child).isValid());
  };

  {
    auto registry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr, saveStore);
    ScopedEmbeddedFederationRegistry scope(registry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador otherReports;
    auto owner = makeRti();
    auto other = makeRti();
    REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
    REQUIRE_NOTHROW(other->connect(otherReports, rti1516_2025::HLA_IMMEDIATE));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
        L"name-owner", L"publisher", federationName));
    REQUIRE_NOTHROW(sourceOtherHandle = other->joinFederationExecution(
        L"name-other", L"subscriber", federationName));

    REQUIRE_NOTHROW(owner->reserveObjectInstanceName(reservedName));
    flushImmediate(*owner);
    REQUIRE(ownerReports.objectInstanceNameReservationSucceededReports.size() == 1U);
    REQUIRE(ownerReports.objectInstanceNameReservationSucceededReports.front()
                .objectInstanceName == reservedName);
    REQUIRE_NOTHROW(other->reserveObjectInstanceName(reservedName));
    flushImmediate(*other);
    REQUIRE(otherReports.objectInstanceNameReservationFailedReports.size() == 1U);
    REQUIRE(otherReports.objectInstanceNameReservationFailedReports.front()
                .objectInstanceName == reservedName);

    REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(other->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(other->federateSaveComplete());
    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.reservedObjectInstanceNames.size() == 1U);
    REQUIRE(image.reservedObjectInstanceNames.front().objectInstanceName ==
            reservedName);
    auto const ownerValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            sourceOwnerHandle);
    REQUIRE(ownerValue.has_value());
    REQUIRE(image.reservedObjectInstanceNames.front().federateId == *ownerValue);

    REQUIRE_NOTHROW(other->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(other->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  {
    auto registry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr, saveStore);
    ScopedEmbeddedFederationRegistry scope(registry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador otherReports;
    auto owner = makeRti();
    auto other = makeRti();
    REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
    REQUIRE_NOTHROW(other->connect(otherReports, rti1516_2025::HLA_IMMEDIATE));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    FederateHandle freshOwnerHandle;
    FederateHandle freshOtherHandle;
    REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
        L"name-owner", L"publisher", federationName));
    REQUIRE_NOTHROW(freshOtherHandle = other->joinFederationExecution(
        L"name-other", L"subscriber", federationName));
    REQUIRE(freshOwnerHandle == sourceOwnerHandle);
    REQUIRE(freshOtherHandle == sourceOtherHandle);

    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    flushImmediate(*owner);
    flushImmediate(*other);
    REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(other->federateRestoreComplete());

    REQUIRE_THROWS_AS(other->releaseObjectInstanceName(reservedName),
                      rti1516_2025::ObjectInstanceNameNotReserved);
    REQUIRE_NOTHROW(other->reserveObjectInstanceName(reservedName));
    flushImmediate(*other);
    REQUIRE(otherReports.objectInstanceNameReservationFailedReports.size() == 1U);
    REQUIRE(otherReports.objectInstanceNameReservationSucceededReports.empty());

    REQUIRE_NOTHROW(owner->releaseObjectInstanceName(reservedName));
    REQUIRE_NOTHROW(other->reserveObjectInstanceName(reservedName));
    flushImmediate(*other);
    REQUIRE(otherReports.objectInstanceNameReservationSucceededReports.size() == 1U);
    REQUIRE(otherReports.objectInstanceNameReservationSucceededReports.front()
                .objectInstanceName == reservedName);
    REQUIRE_THROWS_AS(owner->releaseObjectInstanceName(reservedName),
                      rti1516_2025::ObjectInstanceNameNotReserved);
    REQUIRE_NOTHROW(other->releaseObjectInstanceName(reservedName));

    REQUIRE_NOTHROW(other->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(other->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
}

TEST_CASE(
    "Embedded public fresh-registry restore partitions Reserve Multiple Names against saved foreign ownership",
    "[integration][development-profile][2025][federation-management]"
    "[object-management][save-restore][durable-save][filesystem][process-restart]"
    "[public-fresh-registry-reserve-multiple-name-partition][multi-federate]"
    "[rti.service.reserve-multiple-object-instance-names]"
    "[rti.service.release-object-instance-name]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete]"
    "[federate.callback.multiple-object-instance-name-reservation-succeeded]"
    "[federate.callback.multiple-object-instance-name-reservation-failed]") {
  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();
  std::wstring const ownedName = L"saved-multiple-owned";
  std::wstring const untouchedName = L"saved-multiple-untouched";
  std::wstring const availableName = L"saved-multiple-available";
  std::wstring const saveLabel = L"multiple-reservation-foreign-owner";
  FederateHandle sourceOwnerHandle;
  FederateHandle sourceOtherHandle;

  auto const drain = [](RTIambassador& rti) {
    for (int pass = 0; pass != 16; ++pass) {
      static_cast<void>(rti.evokeCallback(0.0));
    }
  };

  {
    auto registry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr, saveStore);
    ScopedEmbeddedFederationRegistry scope(registry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador otherReports;
    auto owner = makeRti();
    auto other = makeRti();
    REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
    REQUIRE_NOTHROW(other->connect(otherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
        L"multiple-owner", L"publisher", federationName));
    REQUIRE_NOTHROW(sourceOtherHandle = other->joinFederationExecution(
        L"multiple-other", L"subscriber", federationName));

    std::set<std::wstring> const ownedNames{ownedName, untouchedName};
    REQUIRE_NOTHROW(owner->reserveMultipleObjectInstanceNames(ownedNames));
    drain(*owner);
    REQUIRE(ownerReports.multipleObjectInstanceNameReservationSucceededReports.size() == 1U);
    REQUIRE(ownerReports.multipleObjectInstanceNameReservationSucceededReports.front()
                .objectInstanceNames == ownedNames);
    REQUIRE(ownerReports.multipleObjectInstanceNameReservationFailedReports.empty());

    REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(other->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_FALSE(saveStore->load(federationName, saveLabel).has_value());
    drain(*owner);
    drain(*other);
    REQUIRE(ownerReports.federationSavedReportCount == 0U);
    REQUIRE(otherReports.federationSavedReportCount == 0U);
    REQUIRE_NOTHROW(other->federateSaveComplete());
    drain(*owner);
    drain(*other);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);
    REQUIRE(otherReports.federationSavedReportCount == 1U);
    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.reservedObjectInstanceNames.size() == 2U);
    auto const ownerValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            sourceOwnerHandle);
    REQUIRE(ownerValue.has_value());
    for (auto const& reservation : image.reservedObjectInstanceNames) {
      REQUIRE(reservation.federateId == *ownerValue);
      REQUIRE(ownedNames.contains(reservation.objectInstanceName));
    }

    REQUIRE_NOTHROW(other->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(other->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  {
    auto registry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr, saveStore);
    ScopedEmbeddedFederationRegistry scope(registry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador otherReports;
    auto owner = makeRti();
    auto other = makeRti();
    REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
    REQUIRE_NOTHROW(other->connect(otherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    FederateHandle freshOwnerHandle;
    FederateHandle freshOtherHandle;
    REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
        L"multiple-owner", L"publisher", federationName));
    REQUIRE_NOTHROW(freshOtherHandle = other->joinFederationExecution(
        L"multiple-other", L"subscriber", federationName));
    REQUIRE(freshOwnerHandle == sourceOwnerHandle);
    REQUIRE(freshOtherHandle == sourceOtherHandle);

    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    drain(*owner);
    drain(*other);
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(other->federateRestoreComplete());
    drain(*owner);
    drain(*other);

    REQUIRE_NOTHROW(other->reserveMultipleObjectInstanceNames(
        std::set<std::wstring>{ownedName, availableName}));
    drain(*other);
    REQUIRE(otherReports.multipleObjectInstanceNameReservationSucceededReports.size() == 1U);
    REQUIRE(otherReports.multipleObjectInstanceNameReservationSucceededReports.front()
                .objectInstanceNames == std::set<std::wstring>{availableName});
    REQUIRE(otherReports.multipleObjectInstanceNameReservationFailedReports.size() == 1U);
    REQUIRE(otherReports.multipleObjectInstanceNameReservationFailedReports.front()
                .objectInstanceNames == std::set<std::wstring>{ownedName});
    REQUIRE(otherReports.objectInstanceNameReservationSucceededReports.empty());
    REQUIRE(otherReports.objectInstanceNameReservationFailedReports.empty());
    REQUIRE_THROWS_AS(other->releaseObjectInstanceName(ownedName),
                      rti1516_2025::ObjectInstanceNameNotReserved);
    REQUIRE_NOTHROW(other->releaseObjectInstanceName(availableName));
    REQUIRE_NOTHROW(owner->releaseObjectInstanceName(ownedName));
    REQUIRE_NOTHROW(owner->releaseObjectInstanceName(untouchedName));

    REQUIRE_NOTHROW(other->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(other->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
}
