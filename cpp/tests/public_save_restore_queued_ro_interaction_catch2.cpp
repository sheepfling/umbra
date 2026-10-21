#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

#include <chrono>

namespace {

public_federation_restore_test_support::ScopedTemporaryDirectory
workingDirectoryTemporaryDirectory(std::string_view prefix) {
  auto const parent = std::filesystem::current_path();
  auto const ticks = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  auto const thread = static_cast<std::uint64_t>(
      std::hash<std::thread::id>{}(std::this_thread::get_id()));
  auto const identity = ticks ^ (thread + (ticks << 6U) + (ticks >> 2U));
  for (std::size_t attempt = 0U; attempt != 1024U; ++attempt) {
    auto const path = parent /
        (std::string(prefix) + "-" + std::to_string(identity) + "-" +
         std::to_string(attempt));
    std::error_code error;
    if (std::filesystem::create_directory(path, error)) {
      return public_federation_restore_test_support::ScopedTemporaryDirectory(path);
    }
    if (error && error != std::errc::file_exists) {
      throw std::filesystem::filesystem_error(
          "Unable to reserve a short working-directory test folder", path, error);
    }
  }
  throw std::runtime_error("Unable to reserve a unique working-directory test folder.");
}

TEST_CASE(
    "Public save and restore delivers queued receive-order interaction exactly once",
    "[integration][development-profile][federation-management][interaction-management]"
    "[save-restore][receive-order][public-save-queued-ro-interaction]"
    "[save-queued-ro-interaction-public][2025]"
    "[rti.service.send-interaction][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.initiate-federate-restore]"
    "[federate.callback.federation-restored][callback-evoked]") {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;

  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const federationName = nextFederationName();
  auto const interactionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "interaction-fdd-preferred-order-defaults-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{interactionFom};
  std::wstring const saveLabel = L"public-queued-ro-interaction";
  std::vector<unsigned char> const parameterBytes{0x52U, 0x4FU, 0x49U};
  std::vector<unsigned char> const tagBytes{0x53U, 0x41U, 0x56U, 0x45U};
  InteractionClassHandle interactionClass;
  ParameterHandle parameter;
  FederateHandle producerHandle;

  auto const runCallbacks = [](RTIambassador& first, RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeMultipleCallbacks(0.0, 0.0));
      static_cast<void>(second.evokeMultipleCallbacks(0.0, 0.0));
    }
  };

  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador producerReports;
    ReportingFederateAmbassador recipientReports;
    auto producer = makeRti();
    auto recipient = makeRti();
    auto configuration =
        configurationForServiceReportDirectory(reportDirectory.path());
    configuration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(recipient->connect(recipientReports, HLA_EVOKED));
    REQUIRE_NOTHROW(producer->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(producerHandle = producer->joinFederationExecution(
        L"queued-ro-producer", L"publisher", federationName));
    REQUIRE_NOTHROW(recipient->joinFederationExecution(
        L"queued-ro-recipient", L"subscriber", federationName));
    suppressDeclarationRelevanceAdvisories(*producer);
    suppressDeclarationRelevanceAdvisories(*recipient);

    interactionClass = producer->getInteractionClassHandle(
        L"HLAinteractionRoot.UmbraFddPreferredReceiveInteraction");
    auto const recipientInteractionClass = recipient->getInteractionClassHandle(
        L"HLAinteractionRoot.UmbraFddPreferredReceiveInteraction");
    parameter = producer->getParameterHandle(interactionClass, L"Value");
    auto const recipientParameter = recipient->getParameterHandle(
        recipientInteractionClass, L"Value");
    REQUIRE(interactionClass.isValid());
    REQUIRE(recipientInteractionClass.isValid());
    REQUIRE(parameter.isValid());
    REQUIRE(recipientParameter.isValid());
    REQUIRE_NOTHROW(producer->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(recipient->subscribeInteractionClass(
        recipientInteractionClass, true));

    runCallbacks(*producer, *recipient);
    producerReports.interactionReports.clear();
    recipientReports.interactionReports.clear();
    ParameterHandleValueMap parameters;
    parameters.emplace(
        parameter,
        VariableLengthData(parameterBytes.data(), parameterBytes.size()));
    VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    REQUIRE_NOTHROW(producer->sendInteraction(interactionClass, parameters, tag));
    REQUIRE(recipientReports.interactionReports.empty());

    REQUIRE_NOTHROW(producer->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(producer->federateSaveBegun());
    REQUIRE_NOTHROW(recipient->federateSaveBegun());
    REQUIRE_NOTHROW(producer->federateSaveComplete());
    REQUIRE_NOTHROW(recipient->federateSaveComplete());
    REQUIRE(recipientReports.interactionReports.empty());

    REQUIRE_NOTHROW(producer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(recipient->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(producer->disconnect());
    REQUIRE_NOTHROW(recipient->disconnect());
  }

  {
    auto const restoredRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry restoredScope(restoredRegistry);
    ReportingFederateAmbassador producerReports;
    ReportingFederateAmbassador recipientReports;
    auto producer = makeRti();
    auto recipient = makeRti();
    auto configuration =
        configurationForServiceReportDirectory(reportDirectory.path());
    configuration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(recipient->connect(recipientReports, HLA_EVOKED));
    REQUIRE_NOTHROW(producer->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE(producer->joinFederationExecution(
                L"queued-ro-producer", L"publisher", federationName) ==
            producerHandle);
    REQUIRE_NOTHROW(recipient->joinFederationExecution(
        L"queued-ro-recipient", L"subscriber", federationName));
    suppressDeclarationRelevanceAdvisories(*producer);
    suppressDeclarationRelevanceAdvisories(*recipient);

    auto const recipientInteractionClass = recipient->getInteractionClassHandle(
        L"HLAinteractionRoot.UmbraFddPreferredReceiveInteraction");
    auto const recipientParameter = recipient->getParameterHandle(
        recipientInteractionClass, L"Value");
    REQUIRE(recipientInteractionClass.isValid());
    REQUIRE(recipientParameter.isValid());
    REQUIRE_NOTHROW(producer->requestFederationRestore(saveLabel));
    runCallbacks(*producer, *recipient);
    REQUIRE(producerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(recipientReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(producerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(recipientReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE_NOTHROW(producer->federateRestoreComplete());
    REQUIRE_NOTHROW(recipient->federateRestoreComplete());
    runCallbacks(*producer, *recipient);
    REQUIRE(producerReports.federationRestoredReportCount == 1U);
    REQUIRE(recipientReports.federationRestoredReportCount == 1U);
    REQUIRE(recipientReports.interactionReports.size() == 1U);
    auto const& restored = recipientReports.interactionReports.front();
    REQUIRE(restored.interactionClass == recipientInteractionClass);
    REQUIRE(restored.producingFederate == producerHandle);
    REQUIRE(restored.parameterValues.size() == 1U);
    REQUIRE(restored.parameterValues.count(recipientParameter) == 1U);
    REQUIRE(variableLengthDataBytes(
                restored.parameterValues.at(recipientParameter)) == parameterBytes);
    REQUIRE(variableLengthDataBytes(restored.userSuppliedTag) == tagBytes);
    REQUIRE(restored.transportationType.isValid());
    REQUIRE_FALSE(restored.sentRegionsSupplied);
    runCallbacks(*producer, *recipient);
    REQUIRE(recipientReports.interactionReports.size() == 1U);

    REQUIRE_NOTHROW(producer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(recipient->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(producer->disconnect());
    REQUIRE_NOTHROW(recipient->disconnect());
  }
}

TEST_CASE(
    "Public restore discards pre-restore undelivered interaction and replays saved interaction once",
    "[integration][development-profile][federation-management][interaction-management]"
    "[save-restore][receive-order][restore-discard-pre-execution-undelivered-message]"
    "[save-queued-ro-interaction-public][2025]"
    "[rti.service.send-interaction][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.initiate-federate-restore]"
    "[federate.callback.federation-restored][callback-evoked]") {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;

  auto const saveDirectory = workingDirectoryTemporaryDirectory("s");
  auto const reportDirectory = workingDirectoryTemporaryDirectory("r");
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const federationName = nextFederationName();
  auto const interactionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "interaction-fdd-preferred-order-defaults-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{interactionFom};
  std::wstring const saveLabel = L"public-queued-ro-interaction-discard";
  std::vector<unsigned char> const savedParameterBytes{0x53U, 0x41U, 0x56U};
  std::vector<unsigned char> const savedTagBytes{0x53U, 0x41U, 0x56U, 0x45U};
  std::vector<unsigned char> const discardedParameterBytes{0x44U, 0x52U, 0x50U};
  std::vector<unsigned char> const discardedTagBytes{0x44U, 0x52U, 0x4FU, 0x50U};
  InteractionClassHandle interactionClass;
  ParameterHandle parameter;
  FederateHandle producerHandle;

  auto const runCallbacks = [](RTIambassador& first, RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeMultipleCallbacks(0.0, 0.0));
      static_cast<void>(second.evokeMultipleCallbacks(0.0, 0.0));
    }
  };

  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador producerReports;
    ReportingFederateAmbassador recipientReports;
    auto producer = makeRti();
    auto recipient = makeRti();
    auto configuration =
        configurationForServiceReportDirectory(reportDirectory.path());
    configuration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(recipient->connect(recipientReports, HLA_EVOKED));
    REQUIRE_NOTHROW(producer->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(producerHandle = producer->joinFederationExecution(
        L"queued-ro-discard-producer", L"publisher", federationName));
    REQUIRE_NOTHROW(recipient->joinFederationExecution(
        L"queued-ro-discard-recipient", L"subscriber", federationName));
    suppressDeclarationRelevanceAdvisories(*producer);
    suppressDeclarationRelevanceAdvisories(*recipient);

    interactionClass = producer->getInteractionClassHandle(
        L"HLAinteractionRoot.UmbraFddPreferredReceiveInteraction");
    auto const recipientInteractionClass = recipient->getInteractionClassHandle(
        L"HLAinteractionRoot.UmbraFddPreferredReceiveInteraction");
    parameter = producer->getParameterHandle(interactionClass, L"Value");
    auto const recipientParameter = recipient->getParameterHandle(
        recipientInteractionClass, L"Value");
    REQUIRE(interactionClass.isValid());
    REQUIRE(recipientInteractionClass.isValid());
    REQUIRE(parameter.isValid());
    REQUIRE(recipientParameter.isValid());
    REQUIRE_NOTHROW(producer->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(recipient->subscribeInteractionClass(
        recipientInteractionClass, true));

    runCallbacks(*producer, *recipient);
    producerReports.interactionReports.clear();
    recipientReports.interactionReports.clear();
    ParameterHandleValueMap savedParameters;
    savedParameters.emplace(
        parameter,
        VariableLengthData(savedParameterBytes.data(), savedParameterBytes.size()));
    VariableLengthData const savedTag(savedTagBytes.data(), savedTagBytes.size());
    REQUIRE_NOTHROW(producer->sendInteraction(
        interactionClass, savedParameters, savedTag));
    REQUIRE(recipientReports.interactionReports.empty());

    REQUIRE_NOTHROW(producer->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(producer->federateSaveBegun());
    REQUIRE_NOTHROW(recipient->federateSaveBegun());
    REQUIRE_NOTHROW(producer->federateSaveComplete());
    REQUIRE_NOTHROW(recipient->federateSaveComplete());
    REQUIRE(recipientReports.interactionReports.empty());

    ParameterHandleValueMap discardedParameters;
    discardedParameters.emplace(
        parameter,
        VariableLengthData(
            discardedParameterBytes.data(), discardedParameterBytes.size()));
    VariableLengthData const discardedTag(
        discardedTagBytes.data(), discardedTagBytes.size());
    REQUIRE_NOTHROW(producer->sendInteraction(
        interactionClass, discardedParameters, discardedTag));
    REQUIRE(recipientReports.interactionReports.empty());

    REQUIRE_NOTHROW(producer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(recipient->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(producer->disconnect());
    REQUIRE_NOTHROW(recipient->disconnect());
  }

  {
    auto const restoredRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry restoredScope(restoredRegistry);
    ReportingFederateAmbassador producerReports;
    ReportingFederateAmbassador recipientReports;
    auto producer = makeRti();
    auto recipient = makeRti();
    auto configuration =
        configurationForServiceReportDirectory(reportDirectory.path());
    configuration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(recipient->connect(recipientReports, HLA_EVOKED));
    REQUIRE_NOTHROW(producer->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE(producer->joinFederationExecution(
                L"queued-ro-discard-producer", L"publisher", federationName) ==
            producerHandle);
    REQUIRE_NOTHROW(recipient->joinFederationExecution(
        L"queued-ro-discard-recipient", L"subscriber", federationName));
    suppressDeclarationRelevanceAdvisories(*producer);
    suppressDeclarationRelevanceAdvisories(*recipient);

    auto const recipientInteractionClass = recipient->getInteractionClassHandle(
        L"HLAinteractionRoot.UmbraFddPreferredReceiveInteraction");
    auto const recipientParameter = recipient->getParameterHandle(
        recipientInteractionClass, L"Value");
    REQUIRE(recipientInteractionClass.isValid());
    REQUIRE(recipientParameter.isValid());
    REQUIRE_NOTHROW(producer->requestFederationRestore(saveLabel));
    runCallbacks(*producer, *recipient);
    REQUIRE(producerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(recipientReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(producerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(recipientReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE_NOTHROW(producer->federateRestoreComplete());
    REQUIRE_NOTHROW(recipient->federateRestoreComplete());
    runCallbacks(*producer, *recipient);
    REQUIRE(producerReports.federationRestoredReportCount == 1U);
    REQUIRE(recipientReports.federationRestoredReportCount == 1U);
    REQUIRE(recipientReports.interactionReports.size() == 1U);
    auto const& restored = recipientReports.interactionReports.front();
    REQUIRE(restored.interactionClass == recipientInteractionClass);
    REQUIRE(restored.producingFederate == producerHandle);
    REQUIRE(restored.parameterValues.size() == 1U);
    REQUIRE(restored.parameterValues.count(recipientParameter) == 1U);
    REQUIRE(variableLengthDataBytes(
                restored.parameterValues.at(recipientParameter)) == savedParameterBytes);
    REQUIRE(variableLengthDataBytes(restored.userSuppliedTag) == savedTagBytes);
    REQUIRE(restored.transportationType.isValid());
    REQUIRE_FALSE(restored.sentRegionsSupplied);
    runCallbacks(*producer, *recipient);
    REQUIRE(recipientReports.interactionReports.size() == 1U);

    REQUIRE_NOTHROW(producer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(recipient->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(producer->disconnect());
    REQUIRE_NOTHROW(recipient->disconnect());
  }
}

}  // namespace
