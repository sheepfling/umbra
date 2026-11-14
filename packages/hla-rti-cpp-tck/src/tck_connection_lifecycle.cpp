#include "tck_main_scenarios.hpp"
#include "tck_service_report_file_support.hpp"

#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <fstream>
#include <future>
#include <optional>
#include <set>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioConnection(Options const& options, rti::CallbackModel model) {
  auto requireConfigurationResult = [](rti::ConfigurationResult const& result,
                                       std::string const& description) {
    require(
        result.additionalSettingsResult == rti::SETTINGS_IGNORED ||
            result.additionalSettingsResult == rti::SETTINGS_FAILED_TO_PARSE ||
            result.additionalSettingsResult == rti::SETTINGS_APPLIED,
        description + " returned an invalid additional-settings result code");
  };
  auto exerciseConnectOverload =
      [&](std::string const& description, auto const& connect, bool expectIgnoredSettings) {
        rti::RTIambassadorFactory factory;
        auto ambassador = factory.createRTIambassador();
        require(
            static_cast<bool>(ambassador),
            description + " did not create a standard RTIambassador");
        Recorder recorder;
        auto const result = connect(*ambassador, recorder);
        requireConfigurationResult(result, description);
        if (expectIgnoredSettings) {
          require(
              result.additionalSettingsResult == rti::SETTINGS_IGNORED,
              description + " did not report SETTINGS_IGNORED without additional settings");
        }
        ambassador->disconnect();
      };
  auto emptyConfiguration = rti::RtiConfiguration::createConfiguration();
  emptyConfiguration
      .withConfigurationName(L"")
      .withRtiAddress(L"")
      .withAdditionalSettings(L"");
  require(
      emptyConfiguration.configurationName().empty() &&
          emptyConfiguration.rtiAddress().empty() &&
          emptyConfiguration.additionalSettings().empty(),
      "explicit empty RtiConfiguration values did not remain empty");
  auto adapterConfiguration = rti::RtiConfiguration::createConfiguration();
  if (!options.configurationName.empty()) {
    adapterConfiguration.withConfigurationName(
        toWide(options.configurationName));
  }
  if (!options.rtiAddress.empty()) {
    adapterConfiguration.withRtiAddress(toWide(options.rtiAddress));
  }
  if (!options.additionalSettings.empty()) {
    adapterConfiguration.withAdditionalSettings(
        toWide(options.additionalSettings));
  }
  rti::HLAnoCredentials noCredentials;

  exerciseConnectOverload(
      "base connect overload",
      [model](rti::RTIambassador& ambassador, rti::FederateAmbassador& federate) {
        return ambassador.connect(federate, model);
      },
      true);
  exerciseConnectOverload(
      "configuration connect overload",
      [model, &emptyConfiguration](
          rti::RTIambassador& ambassador, rti::FederateAmbassador& federate) {
        return ambassador.connect(federate, model, emptyConfiguration);
      },
      true);
  exerciseConnectOverload(
      "credentials connect overload",
      [model, &noCredentials](
          rti::RTIambassador& ambassador, rti::FederateAmbassador& federate) {
        return ambassador.connect(federate, model, noCredentials);
      },
      true);
  exerciseConnectOverload(
      "configuration and credentials connect overload",
      [model, &emptyConfiguration, &noCredentials](
          rti::RTIambassador& ambassador, rti::FederateAmbassador& federate) {
        return ambassador.connect(
            federate, model, emptyConfiguration, noCredentials);
      },
      true);
  exerciseConnectOverload(
      "adapter configuration and HLAnoCredentials connect overload",
      [model, &adapterConfiguration, &noCredentials](
          rti::RTIambassador& ambassador, rti::FederateAmbassador& federate) {
        return ambassador.connect(
            federate, model, adapterConfiguration, noCredentials);
      },
      options.additionalSettings.empty());

  {
    rti::RTIambassadorFactory factory;
    auto ambassador = factory.createRTIambassador();
    require(
        static_cast<bool>(ambassador),
        "disconnect lifecycle check did not create a standard RTIambassador");
    Recorder recorder;
    auto const firstConnection = ambassador->connect(recorder, model);
    requireConfigurationResult(firstConnection, "first Connect before Disconnect");
    ambassador->disconnect();
    auto const secondConnection = ambassador->connect(recorder, model);
    requireConfigurationResult(secondConnection, "Connect after Disconnect");
    require(
        secondConnection.additionalSettingsResult == rti::SETTINGS_IGNORED,
        "Connect after Disconnect did not report SETTINGS_IGNORED");
    ambassador->disconnect();
  }

  {
    rti::RTIambassadorFactory factory;
    auto ambassador = factory.createRTIambassador();
    require(
        static_cast<bool>(ambassador),
        "unsupported callback-model check did not create a standard RTIambassador");
    Recorder recorder;
    requireException(
        [&] {
          static_cast<void>(ambassador->connect(
              recorder, static_cast<rti::CallbackModel>(-1)));
        },
        L"UnsupportedCallbackModel",
        "unsupported callback model");
    auto const result = ambassador->connect(recorder, model);
    requireConfigurationResult(result, "connect after unsupported callback model");
    require(
        result.additionalSettingsResult == rti::SETTINGS_IGNORED,
        "connect after unsupported callback model did not report SETTINGS_IGNORED");
    ambassador->disconnect();
  }

  {
    rti::RTIambassadorFactory factory;
    auto ambassador = factory.createRTIambassador();
    require(
        static_cast<bool>(ambassador),
        "pre-connect disconnect check did not create a standard RTIambassador");
    requireException(
        [&] { ambassador->disconnect(); },
        L"NotConnected",
        "disconnect before connect");
  }

  Session session(options, model, "connection");
  requireException(
      [&] { session.rtiAmbassador().listFederationExecutions(); },
      L"NotConnected",
      "listing federation executions before connect");
  requireException(
      [&] {
        session.rtiAmbassador().listFederationExecutionMembers(
            L"tck-not-connected-federation");
      },
      L"NotConnected",
      "listing federation members before connect");
  requireException(
      [&] {
        static_cast<void>(session.rtiAmbassador().getObjectClassHandle(
            options.objectClassName));
      },
      L"NotConnected",
      "looking up an object class before connect");
  requireException(
      [&] {
        session.rtiAmbassador().reserveObjectInstanceName(
            L"tck-not-connected-object");
      },
      L"NotConnected",
      "reserving an object name before connect");
  auto const baseResult = session.connect();
  requireConfigurationResult(baseResult, "base connect");
  requireException(
      [&] {
        static_cast<void>(session.rtiAmbassador().connect(session.recorder(), model));
      },
      L"AlreadyConnected",
      "duplicate base connect");
  session.rtiAmbassador().disableCallbacks();
  session.rtiAmbassador().enableCallbacks();
  require(
      !session.rtiAmbassador().evokeCallback(0.0),
      "an empty callback queue unexpectedly evoked a callback");
  require(
      !session.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.0),
      "an empty callback queue unexpectedly evoked multiple callbacks");
  session.disconnect();

  Session configured(options, model, "connection");
  auto const configuredResult = configured.connectWithConfiguration(emptyConfiguration);
  requireConfigurationResult(configuredResult, "empty-configuration connect");
  configured.disconnect();

  Session reconnect(options, model, "connection-reconnect");
  requireConfigurationResult(reconnect.connect(), "first reconnect-cycle connect");
  reconnect.disconnect();
  requireConfigurationResult(reconnect.connect(), "second reconnect-cycle connect");
  reconnect.disconnect();
}

void scenarioConnectionCallbackContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioConnection(options, model);
}

Clock::time_point waitForConnectionLossAndSignal(
    Session& lost,
    Options const& options,
    std::string const& description,
    rti::CallbackModel model) {
  if (!options.connectionLossReadyMarker.empty()) {
    std::ofstream readyMarker(
        options.connectionLossReadyMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(readyMarker),
        "Could not write the connection-loss readiness marker");
    readyMarker << "ready\n";
    require(
        static_cast<bool>(readyMarker),
        "Could not flush the connection-loss readiness marker");
  }

  bool outageConfirmed = options.connectionLossOutageMarker.empty();
  bool connectionLostBeforeOutageConfirmation = false;
  auto const outageDeadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (!outageConfirmed && Clock::now() < outageDeadline) {
    std::ifstream outageMarker(options.connectionLossOutageMarker);
    std::string status;
    if (outageMarker >> status && status == "stopped") {
      outageConfirmed = true;
      break;
    }
    connectionLostBeforeOutageConfirmation =
        connectionLostBeforeOutageConfirmation ||
        lost.recorder().hasConnectionLost();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      outageConfirmed,
      "Timed out waiting for the adapter's confirmed connection-loss outage marker");
  if (!options.connectionLossOutageMarker.empty()) {
    require(
        !connectionLostBeforeOutageConfirmation &&
            !lost.recorder().hasConnectionLost(),
        description +
            " connectionLost was reported before the adapter confirmed the outage");
  }

  auto const lossObservationStarted = Clock::now();
  auto const lossDeadline = lossObservationStarted +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  std::string lastEvokeFailure;
  while (Clock::now() < lossDeadline && !lost.recorder().hasConnectionLost()) {
    // HLA_EVOKED callbacks require the application's standard callback pump.
    // HLA_IMMEDIATE is checked without evocation so the scenario verifies the
    // selected model instead of accidentally servicing it through Evoke.
    if (model == rti::HLA_EVOKED) {
      try {
        static_cast<void>(lost.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.05));
      } catch (std::exception const& error) {
        // A provider may surface the socket fault on the Evoke operation
        // before placing the callback on the next callback turn.
        lastEvokeFailure = error.what();
      } catch (...) {
        lastEvokeFailure = "non-standard exception from EvokeMultipleCallbacks";
      }
    }
    if (!lost.recorder().hasConnectionLost()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  if (!lost.recorder().hasConnectionLost()) {
    auto failure = "Timed out waiting for " + description +
        " connectionLost callback";
    if (!lastEvokeFailure.empty()) {
      failure += "; last EvokeMultipleCallbacks failure: " + lastEvokeFailure;
    }
    throw std::runtime_error(std::move(failure));
  }
  require(
      !lost.recorder().connectionLostDescription().empty(),
      description + " connectionLost did not provide a fault description");
  if (!options.connectionLossMarker.empty()) {
    std::ofstream marker(
        options.connectionLossMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker),
        "Could not write the connection-loss completion marker");
    marker << "callback-ok\n";
    require(
        static_cast<bool>(marker),
        "Could not flush the connection-loss completion marker");
  }
  return lossObservationStarted;
}

void scenarioFederateReconnectTimeout(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.additionalSettings ==
          "FED_INT_HEART=0.02;FED_TIMEOUT_RECONNECT=0.25",
      "The current-process reconnect adapter did not provide its accelerated recovery settings");
  require(
      options.connectionLossServerManaged &&
          !options.connectionLossReadyMarker.empty() &&
          !options.connectionLossMarker.empty() &&
          !options.connectionLossOutageMarker.empty(),
      "Federate reconnect-timeout coverage requires an adapter-managed outage and callback markers");
  require(
      !options.mimFom.empty() && !options.logicalTimeImplementationName.empty(),
      "Federate reconnect-timeout coverage requires MIM and logical-time adapter inputs");
  Session session(options, model, "member");
  auto const connection = session.connect();
  require(
      connection.additionalSettingsResult == rti::SETTINGS_APPLIED,
      "Connect did not apply the provider-specific federate heartbeat settings");
  auto const federation = federationName(options, "federate-heartbeat");
  session.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  session.join(options.memberFederateName, options.federateType, federation);

  // The process fixture stops the RTI host only after the federate has joined
  // and this readiness signal is written. The helper then waits for its
  // outage-confirmation marker before starting the measured reconnect window.
  auto const reconnectStarted = waitForConnectionLossAndSignal(
      session,
      options,
      "FED_TIMEOUT_RECONNECT recovery window",
      model);
  require(
      Clock::now() - reconnectStarted >= std::chrono::milliseconds(200),
      "ConnectionLost was reported before the configured FED_TIMEOUT_RECONNECT window elapsed");
  require(
      Clock::now() - reconnectStarted < std::chrono::seconds(2),
      "ConnectionLost was reported after the configured FED_TIMEOUT_RECONNECT window");
  requireException(
      [&] {
        static_cast<void>(session.rtiAmbassador().listFederationExecutions());
      },
      L"NotConnected",
      "public service after FED_TIMEOUT_RECONNECT expires");
  session.abandonAfterConnectionLoss();
}

void writeFederateHeartbeatMarker(
    std::string const& markerPath,
    std::string const& value,
    std::string const& description) {
  std::ofstream marker(markerPath, std::ios::binary | std::ios::trunc);
  require(static_cast<bool>(marker), "Could not write " + description);
  marker << value << '\n';
  require(static_cast<bool>(marker), "Could not flush " + description);
}

void waitForFederateHeartbeatMarker(
    Session& session,
    std::string const& markerPath,
    std::string const& expectedValue,
    std::string const& description,
    Clock::time_point deadline) {
  std::string lastMarkerValue;
  while (Clock::now() < deadline) {
    std::ifstream marker(markerPath, std::ios::binary);
    std::string value;
    marker >> value;
    if (marker) {
      lastMarkerValue = std::move(value);
    }
    if (lastMarkerValue == expectedValue) {
      return;
    }
    require(
        !session.recorder().hasConnectionLost(),
        "The federate lost its connection before " + description);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  auto message = "Timed out waiting for " + description;
  if (!lastMarkerValue.empty()) {
    message += " (last marker value: " + lastMarkerValue + ")";
  }
  require(false, std::move(message));
}

void scenarioFederateHeartbeatTimeoutRecovery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.additionalSettings ==
          "FED_INT_HEART=2;FED_TIMEOUT_HEART=2.5;FED_TIMEOUT_RECONNECT=4",
      "The current-process heartbeat-timeout adapter did not provide its accelerated recovery settings");
  require(
      options.federateHeartbeatRecoveryServerManaged &&
          !options.federateHeartbeatTimeoutReadyMarker.empty() &&
          !options.federateHeartbeatInactiveMarker.empty() &&
          !options.federateHeartbeatResumedMarker.empty(),
      "Federate-heartbeat recovery requires adapter-managed inactivity and recovery markers");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Federate-heartbeat recovery coverage requires a logical-time implementation");

  Session session(options, model, "member");
  auto const connection = session.connect();
  require(
      connection.additionalSettingsResult == rti::SETTINGS_APPLIED,
      "Connect did not apply the provider-specific federate heartbeat settings");
  auto const federation = federationName(options, "federate-heartbeat-recovery");
  session.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  session.join(options.memberFederateName, options.federateType, federation);
  auto const memberHandle = session.federateHandle();

  writeFederateHeartbeatMarker(
      options.federateHeartbeatTimeoutReadyMarker,
      "ready",
      "the heartbeat-timeout readiness marker");
  auto const timeoutStarted = Clock::now();
  auto const recoveryDeadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  if (model == rti::HLA_EVOKED) {
    waitForFederateHeartbeatMarker(
        session,
        options.federateHeartbeatInactiveMarker,
        "inactive",
        "the RTI inactive-session timeout transition",
        recoveryDeadline);
    auto const timeoutElapsed = Clock::now() - timeoutStarted;
    require(
        timeoutElapsed >= std::chrono::milliseconds(400),
        "The RTI declared the process session inactive before its configured timeout could expire");
    waitForFederateHeartbeatMarker(
        session,
        options.federateHeartbeatResumedMarker,
        "resumed",
        "the server-side session resumption after heartbeat timeout",
        recoveryDeadline);
  } else {
    // HLA_IMMEDIATE continuously polls the RTI for callbacks using
    // receive_interaction service requests. That is peer activity, so it is
    // not an offline-federate case and must not be expired by RTI_TIMEOUT_HEART.
    auto const activeObservationDeadline =
        Clock::now() + std::chrono::milliseconds(1500);
    while (Clock::now() < activeObservationDeadline) {
      require(
          !session.recorder().hasConnectionLost(),
          "An active immediate-callback session reported connectionLost");
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::ifstream inactiveMarker(
        options.federateHeartbeatInactiveMarker, std::ios::binary);
    std::string inactiveValue;
    inactiveMarker >> inactiveValue;
    require(
        !inactiveMarker || inactiveValue != "inactive",
        "The RTI expired an immediate-callback session that was still polling for callbacks");
  }
  require(
      !session.recorder().hasConnectionLost(),
      "The recoverable heartbeat interruption produced a connectionLost callback");

  auto const serviceDeadline = Clock::now() + std::chrono::seconds(3);
  bool publicServiceRecovered = false;
  while (Clock::now() < serviceDeadline && !publicServiceRecovered) {
    if (session.recorder().hasConnectionLost()) {
      break;
    }
    try {
      session.rtiAmbassador().listFederationExecutions();
      if (model == rti::HLA_EVOKED) {
        static_cast<void>(
            session.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.05));
      }
      require(
          !session.recorder().hasConnectionLost(),
          "The public service recovery attempt reported connectionLost");
      publicServiceRecovered = session.recorder().listsFederation(
          federation,
          options.logicalTimeImplementationName);
    } catch (std::exception const&) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
  require(
      publicServiceRecovered,
      "A public listFederationExecutions service did not work after heartbeat-timeout recovery");
  require(
      !session.recorder().hasConnectionLost(),
      "The public service recovery attempt reported connectionLost");
  require(
      session.rtiAmbassador().getFederateHandle(options.memberFederateName) ==
          memberHandle,
      "The RTI did not preserve the joined federate identity across the silent-session timeout and reconnect");
  session.resign(rti::NO_ACTION);
  session.rtiAmbassador().destroyFederationExecution(federation);
  session.disconnect();
}

void scenarioFederateMissingRtiMessageTimeoutRecovers(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_EVOKED,
      "Missing-RTI-message timeout coverage requires an idle HLA_EVOKED federate");
  require(
      options.additionalSettings ==
          "FED_INT_HEART=0.05;FED_TIMEOUT_HEART=0.4;FED_TIMEOUT_RECONNECT=4",
      "The missing-RTI-message timeout adapter did not provide its accelerated settings");
  require(
      options.federateMissingRtiMessageTimeoutServerManaged,
      "Missing-RTI-message timeout coverage requires its adapter-managed heartbeat fixture");
  require(
      !options.results.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Missing-RTI-message timeout coverage requires result and logical-time configuration");

  auto const markerDirectory = std::filesystem::path(options.results).parent_path();
  auto const readyMarker =
      (markerDirectory / "federate-missing-rti-message-timeout-ready.ok").string();
  auto const suppressedMarker =
      (markerDirectory / "federate-missing-rti-message-timeout-reply-suppressed.ok").string();
  auto const resumedMarker =
      (markerDirectory / "federate-missing-rti-message-timeout-resumed.ok").string();

  Session session(options, model, "member");
  auto const connection = session.connect();
  require(
      connection.additionalSettingsResult == rti::SETTINGS_APPLIED,
      "Connect did not apply the provider-specific missing-RTI-message timeout");
  auto const federation =
      federationName(options, "federate-missing-rti-message-timeout");
  session.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  session.join(options.memberFederateName, options.federateType, federation);
  auto const memberHandle = session.federateHandle();

  writeFederateHeartbeatMarker(
      readyMarker,
      "ready",
      "the missing-RTI-message timeout readiness marker");
  auto const deadline =
      Clock::now() + std::chrono::milliseconds(options.timeoutMilliseconds);
  waitForFederateHeartbeatMarker(
      session,
      suppressedMarker,
      "suppressed",
      "the adapter to suppress an RTI heartbeat reply",
      deadline);
  auto const firstReplySuppressed = Clock::now();
  waitForFederateHeartbeatMarker(
      session,
      resumedMarker,
      "resumed",
      "the configured missing-RTI-message timeout to trigger session recovery",
      deadline);
  auto const recoveryDelay = Clock::now() - firstReplySuppressed;
  require(
      recoveryDelay >= std::chrono::milliseconds(250),
      "The RTI resumed the session before the configured 400 ms missing-message timeout could expire");
  require(
      recoveryDelay < std::chrono::milliseconds(3500),
      "The RTI did not resume the session within the configured reconnect window");

  require(
      !session.recorder().hasConnectionLost(),
      "A recoverable missing-RTI-message timeout produced connectionLost");
  require(
      session.rtiAmbassador().getFederateHandle(options.memberFederateName) ==
          memberHandle,
      "The federate identity changed across missing-RTI-message timeout recovery");
  session.rtiAmbassador().listFederationExecutions();
  auto const serviceDeadline = Clock::now() + std::chrono::seconds(1);
  while (!session.recorder().listsFederation(
             federation,
             options.logicalTimeImplementationName) &&
         Clock::now() < serviceDeadline) {
    static_cast<void>(
        session.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.05));
  }
  require(
      session.recorder().listsFederation(
          federation,
          options.logicalTimeImplementationName),
      "The public federation-list service did not work after missing-RTI-message recovery");
  require(
      !session.recorder().hasConnectionLost(),
      "The recovered public session reported connectionLost");

  session.resign(rti::NO_ACTION);
  session.rtiAmbassador().destroyFederationExecution(federation);
  session.disconnect();
}

void scenarioFederateIdleHeartbeatPreservesSession(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.additionalSettings ==
          "FED_INT_HEART=0.1;FED_TIMEOUT_HEART=2;FED_TIMEOUT_RECONNECT=0.25",
      "The idle-heartbeat adapter did not provide its accelerated heartbeat settings");
  require(
      !options.results.empty(),
      "Idle-heartbeat coverage requires an adapter results path for coordination markers");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Idle-heartbeat coverage requires a logical-time implementation");
  auto const markerDirectory = std::filesystem::path(options.results).parent_path();
  auto const readyMarker =
      (markerDirectory / "federate-idle-heartbeat-ready.ok").string();
  auto const observedMarker =
      (markerDirectory / "federate-idle-heartbeat-observed.ok").string();
  auto const repeatedMarkerPath =
      markerDirectory / "federate-idle-heartbeat-repeated.ok";
  auto const inactiveMarkerPath =
      (markerDirectory / "federate-idle-heartbeat-inactive.ok").string();

  Session session(options, model, "member");
  auto const connection = session.connect();
  require(
      options.rtiAddress.rfind("tcp://", 0U) == 0U,
      "TCP transport coverage requires an adapter-supplied tcp:// RTI address");
  require(
      connection.addressUsed,
      "Connect did not use the adapter-supplied TCP RTI address");
  require(
      connection.additionalSettingsResult == rti::SETTINGS_APPLIED,
      "Connect did not apply the provider-specific idle-heartbeat interval");
  auto const federation = federationName(options, "federate-idle-heartbeat");
  session.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  session.join(options.memberFederateName, options.federateType, federation);
  auto const idleStarted = Clock::now();

  writeFederateHeartbeatMarker(
      readyMarker,
      "ready",
      "the idle-heartbeat readiness marker");
  auto const observationDeadline = idleStarted + std::chrono::milliseconds(1500);
  if (model == rti::HLA_EVOKED) {
    // No RTIambassador service or callback-poll request is issued during this
    // branch's idle window. The adapter's server-side hook records only
    // received heartbeat control frames.
    waitForFederateHeartbeatMarker(
        session,
        observedMarker,
        "heartbeat",
        "a heartbeat from the otherwise-idle evoked federate",
        observationDeadline);
  }

  while (Clock::now() < observationDeadline) {
    require(
        !session.recorder().hasConnectionLost(),
        "The idle federate lost its public RTI connection during the heartbeat window");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  if (model == rti::HLA_EVOKED) {
    std::ifstream repeatedMarker(repeatedMarkerPath, std::ios::binary);
    std::string repeatedValue;
    repeatedMarker >> repeatedValue;
    require(
        repeatedMarker && repeatedValue == "heartbeat",
        "The idle evoked federate did not send a second heartbeat during the observation window");
  }
  std::ifstream inactiveMarker(
      inactiveMarkerPath, std::ios::binary);
  std::string inactiveValue;
  inactiveMarker >> inactiveValue;
  require(
      !inactiveMarker || inactiveValue != "inactive",
      "The idle federate exceeded the RTI heartbeat timeout despite the configured interval");

  std::atomic<std::size_t> federationReportCount{0U};
  std::promise<void> firstReportEntered;
  auto firstReportEnteredSignal = firstReportEntered.get_future();
  std::promise<void> releaseFirstReport;
  auto releaseFirstReportSignal = releaseFirstReport.get_future().share();
  session.recorder().setFederationExecutionsReportHook([&] {
    auto const reportIndex = federationReportCount.fetch_add(1U);
    if (model == rti::HLA_EVOKED && reportIndex == 0U) {
      firstReportEntered.set_value();
      releaseFirstReportSignal.wait();
    }
  });
  session.rtiAmbassador().listFederationExecutions();

  std::exception_ptr evokeFailure;
  std::thread evoker;
  bool callbackWasHeld = false;
  if (model == rti::HLA_EVOKED) {
    evoker = std::thread([&] {
      try {
        static_cast<void>(session.rtiAmbassador().evokeCallback(0.25));
      } catch (...) {
        evokeFailure = std::current_exception();
      }
    });
    callbackWasHeld =
        firstReportEnteredSignal.wait_for(std::chrono::seconds(3)) ==
        std::future_status::ready;
  }
  std::exception_ptr concurrentServiceFailure;
  std::promise<void> concurrentServiceCompleted;
  auto concurrentServiceCompletedSignal =
      concurrentServiceCompleted.get_future();
  std::thread concurrentServiceSender;
  bool serviceCompletedWhileCallbackHeld = false;
  if (callbackWasHeld) {
    concurrentServiceSender = std::thread([&] {
      try {
        session.rtiAmbassador().listFederationExecutions();
      } catch (...) {
        concurrentServiceFailure = std::current_exception();
      }
      concurrentServiceCompleted.set_value();
    });
    serviceCompletedWhileCallbackHeld =
        concurrentServiceCompletedSignal.wait_for(std::chrono::seconds(3)) ==
        std::future_status::ready;
  }

  releaseFirstReport.set_value();
  if (evoker.joinable()) {
    evoker.join();
  }
  if (concurrentServiceSender.joinable()) {
    concurrentServiceSender.join();
  }

  auto requireThreadSuccess = [](std::exception_ptr const& failure,
                                 std::string const& operation) {
    if (failure == nullptr) {
      return;
    }
    try {
      std::rethrow_exception(failure);
    } catch (std::exception const& error) {
      require(false, operation + " failed: " + error.what());
    } catch (...) {
      require(false, operation + " failed unexpectedly");
    }
  };
  if (model == rti::HLA_EVOKED) {
    require(
        callbackWasHeld,
        "List Federation Executions did not enter its first report callback");
    require(
        serviceCompletedWhileCallbackHeld,
        "the federate could not send a second service while its RTI report callback was outstanding");
    requireThreadSuccess(evokeFailure, "evoked report callback");
    requireThreadSuccess(concurrentServiceFailure, "overlapping List Federation Executions");
    static_cast<void>(session.rtiAmbassador().evokeCallback(0.25));
    require(
        federationReportCount.load() >= 2U,
        "The evoked callback model did not deliver the second federation-list report");
  } else {
    require(
        federationReportCount.load() == 1U,
        "List Federation Executions did not deliver its immediate report callback");
  }
  session.recorder().setFederationExecutionsReportHook({});
  require(
      session.recorder().listsFederation(
          federation,
          options.logicalTimeImplementationName),
      "the public federation-list service did not succeed while the same TCP session had an outstanding RTI callback");

  session.resign(rti::NO_ACTION);
  session.rtiAmbassador().destroyFederationExecution(federation);
  session.disconnect();
}

void scenarioConnectionLoss(Options const& options, rti::CallbackModel model) {
  require(
      options.connectionLossServerManaged,
      "Connection-loss cleanup requires an adapter-managed fault fixture");
  require(
      !options.mimFom.empty() && !options.logicalTimeImplementationName.empty(),
      "Connection-loss save reporting requires MIM and logical-time configuration");
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "connection-loss");
  owner.connect();
  member.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  // The process adapter owns these endpoint identities and the coordinator's
  // process fixture resolves joined members by the same configured names.
  auto const ownerName = options.ownerFederateName;
  auto const memberName = options.memberFederateName;
  owner.join(ownerName, options.federateType, federation);
  member.join(memberName, options.federateType, federation);

  auto const federateMomClass = member.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateNameAttribute = member.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAfederateName");
  auto const reportFileAttribute = member.rtiAmbassador().getAttributeHandle(
      federateMomClass,
      L"HLAreportServiceFile");
  require(
      federateMomClass.isValid() && federateNameAttribute.isValid() &&
          reportFileAttribute.isValid(),
      "Standard Federate MOM did not expose HLAreportServiceFile during connection loss");
  member.rtiAmbassador().subscribeObjectClassAttributes(
      federateMomClass,
      {federateNameAttribute, reportFileAttribute},
      true,
      L"");
  // A process transport may have pushed the static reflection while the
  // subscription service was returning. This lookup gives the reader an
  // ordinary public-service boundary to buffer that event; delivery is still
  // observed through each selected callback model below.
  static_cast<void>(member.rtiAmbassador().getObjectClassName(federateMomClass));
  std::optional<std::filesystem::path> ownerReportFile;
  waitForSessions(
      {&member, &owner},
      [&] {
        ownerReportFile = service_report_file_support::advertisedServiceReportFile(
            member.recorder(),
            federateNameAttribute,
            reportFileAttribute,
            ownerName);
        return ownerReportFile.has_value();
      },
      options,
      "survivor HLAreportServiceFile MOM reflection");
  require(
      std::filesystem::is_regular_file(*ownerReportFile),
      "Surviving federate HLAreportServiceFile did not name an existing file");
  owner.rtiAmbassador().setServiceReportingSwitch(true);
  owner.rtiAmbassador().setSendServiceReportsToFileSwitch(true);

  auto const saveLabel =
      L"service-report-rti-detected-failure-on-connection-loss";
  auto const ownerSaveInitiationsBefore = owner.federateSaveInitiations().size();
  auto const memberSaveInitiationsBefore = member.federateSaveInitiations().size();
  owner.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions(
      {&owner, &member},
      [&] {
        return owner.federateSaveInitiations().size() >
                ownerSaveInitiationsBefore &&
            member.federateSaveInitiations().size() >
                memberSaveInitiationsBefore;
      },
      options,
      "save initiation before RTI-detected connection loss");
  owner.rtiAmbassador().federateSaveBegun();
  member.rtiAmbassador().federateSaveBegun();
  auto const ownerFailuresBefore = owner.federationNotSavedReasons().size();
  auto const beforeConnectionLoss =
      service_report_file_support::readServiceReportFile(*ownerReportFile);

  waitForConnectionLossAndSignal(
      member,
      options,
      "connection loss during an active federation save",
      model);
  auto const afterConnectionLoss =
      service_report_file_support::readServiceReportFile(*ownerReportFile);
  auto const savedRecord = afterConnectionLoss.find(
      "\"HLAservice\":\"FederationSaved\"",
      beforeConnectionLoss.size());
  auto const failureReason = afterConnectionLoss.find(
      "\"HLAargumentType\":48,\"HLAargumentName\":\"Optional failure reason\","
      "\"HLAargumentValue\":\"RTI_DETECTED_FAILURE_DURING_SAVE\"",
      savedRecord);
  require(
      savedRecord != std::string::npos && failureReason != std::string::npos,
      "Surviving federate's HLAreportServiceFile omitted type-48 RTI_DETECTED_FAILURE_DURING_SAVE");
  if (model == rti::HLA_EVOKED) {
    require(
        owner.federationNotSavedReasons().size() == ownerFailuresBefore,
        "Evoked federationNotSaved callback ran before the service-report file inspection");
  }
  waitForSessions(
      {&owner},
      [&] {
        return owner.federationNotSavedReasons().size() > ownerFailuresBefore;
      },
      options,
      "RTI_DETECTED_FAILURE_DURING_SAVE callback");
  require(
      owner.federationNotSavedReasons().back() ==
          rti::RTI_DETECTED_FAILURE_DURING_SAVE,
      "Surviving federate did not receive RTI_DETECTED_FAILURE_DURING_SAVE");

  owner.rtiAmbassador().listFederationExecutionMembers(federation);
  waitFor(owner, [&] {
    return owner.recorder().listsMembers(
        federation, {options.ownerFederateName}, 1U);
  }, options, "the disconnected federate is no longer joined");
  requireException(
      [&] {
        static_cast<void>(member.rtiAmbassador().listFederationExecutions());
      },
      L"NotConnected",
      "listing federation executions after connection loss");

  std::wstring const survivorSynchronizationLabel =
      L"connection-loss-survivor-synchronization";
  std::vector<std::uint8_t> const synchronizationTag{0x4cU, 0x4fU, 0x53U};
  auto const registrationsBefore =
      owner.synchronizationPointRegistrations().size();
  auto const announcementsBefore =
      owner.synchronizationPointAnnouncements().size();
  owner.rtiAmbassador().registerFederationSynchronizationPoint(
      survivorSynchronizationLabel,
      rti::VariableLengthData(
          synchronizationTag.data(), synchronizationTag.size()));
  waitFor(
      owner,
      [&] {
        return owner.synchronizationPointRegistrations().size() >
                registrationsBefore &&
            owner.synchronizationPointAnnouncements().size() >
                announcementsBefore;
      },
      options,
      "surviving-member synchronization registration after connection loss");
  auto const registration =
      owner.synchronizationPointRegistrations().back();
  auto const announcement =
      owner.synchronizationPointAnnouncements().back();
  require(
      registration.succeeded &&
          registration.label == survivorSynchronizationLabel,
      "surviving member could not register a synchronization point after connection loss");
  require(
      announcement.label == survivorSynchronizationLabel &&
          announcement.tag == synchronizationTag,
      "surviving member received an incorrect synchronization-point announcement");
  auto const synchronizedBefore = owner.federationSynchronized().size();
  owner.rtiAmbassador().synchronizationPointAchieved(
      survivorSynchronizationLabel);
  waitFor(
      owner,
      [&] {
        return owner.federationSynchronized().size() > synchronizedBefore;
      },
      options,
      "surviving-member federation synchronization after connection loss");
  auto const completion = owner.federationSynchronized().back();
  require(
      completion.label == survivorSynchronizationLabel &&
          completion.failedToSyncSet.empty(),
      "federation synchronization did not complete as if the lost federate had resigned");

  rti::InteractionClassHandle interactionClass =
      owner.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  require(interactionClass.isValid(), "The surviving member could not resolve an interaction class");
  owner.rtiAmbassador().sendInteraction(
      interactionClass, rti::ParameterHandleValueMap{}, rti::VariableLengthData{});
  owner.rtiAmbassador().setServiceReportingSwitch(false);
  owner.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  owner.resign(rti::NO_ACTION);
  owner.disconnect();
}

void scenarioConnectionLossContract(Options const& options, rti::CallbackModel model) {
  scenarioConnectionLoss(options, model);
}

void scenarioConnectionFaultJoinResponseReplay(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.messageHistoryRecoveryServerManaged,
      "Join-response replay requires an adapter-managed process fault fixture");
  Session owner(options, model, "owner");
  Session recovering(options, model, "member");
  auto const federation = federationName(options, "connection-fault-replay");
  auto const recoveringName = options.memberFederateName;

  owner.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  recovering.connect();
  recovering.join(recoveringName, options.federateType, federation);
  auto const recoveredHandle = recovering.rtiAmbassador().getFederateHandle(
      recoveringName);
  require(
      recoveredHandle == recovering.federateHandle(),
      "Join response replay returned a handle that did not identify the joined federate");
  recovering.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  recovering.disconnect();
  owner.disconnect();
  if (!options.messageHistoryRecoveryMarker.empty()) {
    std::ofstream marker(
        options.messageHistoryRecoveryMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker) && marker << "recovered\n",
        "Could not write the message-history recovery completion marker");
  }
}

void scenarioConnectionFaultFederateRequestReplay(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.additionalSettings ==
          "FED_INT_HEART=0.02;FED_TIMEOUT_RECONNECT=0.25",
      "Federate request replay requires accelerated heartbeat settings");
  require(
      options.messageHistoryRecoveryServerManaged,
      "Federate request replay requires an adapter-managed process fault fixture");
  Session owner(options, model, "owner");
  Session recovering(options, model, "member");
  auto const federation = federationName(options, "federate-request-replay");
  auto const recoveringName = options.memberFederateName;

  auto const ownerConnection = owner.connect();
  require(
      ownerConnection.additionalSettingsResult == rti::SETTINGS_APPLIED,
      "The owner Connect did not apply the accelerated heartbeat settings");
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const recoveringConnection = recovering.connect();
  require(
      recoveringConnection.additionalSettingsResult == rti::SETTINGS_APPLIED,
      "The member Connect did not apply the accelerated heartbeat settings");
  recovering.join(recoveringName, options.federateType, federation);
  auto const recoveredHandle = recovering.rtiAmbassador().getFederateHandle(
      recoveringName);
  require(
      recoveredHandle == recovering.federateHandle(),
      "Replayed federate request returned a handle for a different federate");

  auto const replayedResponseHandle = recovering.rtiAmbassador().getFederateHandle(
      recoveringName);
  require(
      replayedResponseHandle == recovering.federateHandle(),
      "Replayed federate service response returned a handle for a different federate");

  recovering.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  recovering.disconnect();
  owner.disconnect();
  if (!options.messageHistoryRecoveryMarker.empty()) {
    std::ofstream marker(
        options.messageHistoryRecoveryMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker) && marker << "recovered\n",
        "Could not write the federate-request recovery completion marker");
  }
}

void scenarioConnectionFaultRtiClosedResumeFails(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.additionalSettings ==
          "FED_INT_HEART=0.02;FED_TIMEOUT_RECONNECT=0.25",
      "RTI-closed resume failure requires accelerated federate heartbeat settings");
  require(
      options.messageHistoryRecoveryServerManaged &&
          !options.messageHistoryRecoveryMarker.empty() &&
          !options.messageHistoryResumeFailureTriggerMarker.empty(),
      "RTI-closed resume failure requires an adapter-managed process fault fixture");
  Session owner(options, model, "owner");
  Session recovering(options, model, "member");
  auto const federation = federationName(
      options,
      "connection-fault-rti-closed-resume-fails");
  auto const recoveringName = options.memberFederateName;

  auto const ownerConnection = owner.connect();
  require(
      ownerConnection.additionalSettingsResult == rti::SETTINGS_APPLIED,
      "The owner Connect did not apply the accelerated heartbeat settings");
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  auto const recoveringConnection = recovering.connect();
  require(
      recoveringConnection.additionalSettingsResult == rti::SETTINGS_APPLIED,
      "The member Connect did not apply the accelerated heartbeat settings");
  recovering.join(recoveringName, options.federateType, federation);
  {
    std::ofstream trigger(
        options.messageHistoryResumeFailureTriggerMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(trigger) && trigger << "invalidate-next-data-frame\n",
        "Could not signal the adapter to invalidate the first post-join data frame");
  }

  bool triggerCallMade = false;
  auto const connectionLostDeadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (
      std::chrono::steady_clock::now() < connectionLostDeadline &&
      !recovering.recorder().hasConnectionLost()) {
    try {
      if (!triggerCallMade) {
        triggerCallMade = true;
        if (model == rti::HLA_EVOKED) {
          static_cast<void>(
              recovering.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.05));
        } else {
          static_cast<void>(
              recovering.rtiAmbassador().getFederateHandle(recoveringName));
        }
      } else {
        recovering.pump();
      }
    } catch (rti::Exception const&) {
      // In immediate mode, the first synchronous service can report that the
      // recovery window expired before the ConnectionLost callback is seen.
    } catch (std::exception const&) {
      // Callback pumping can return normally while dispatching ConnectionLost;
      // the later NotConnected service check is the public failure assertion.
    }
    if (!recovering.recorder().hasConnectionLost()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  require(
      recovering.recorder().hasConnectionLost(),
      "Timed out waiting for ConnectionLost after RTI-closed session resume failure");
  require(
      !recovering.recorder().connectionLostDescription().empty(),
      "RTI-closed session resume failure omitted the ConnectionLost description");
  requireException(
      [&] {
        static_cast<void>(
            recovering.rtiAmbassador().listFederationExecutions());
      },
      L"NotConnected",
      "public service after RTI-closed session resume failure");

  owner.resign(rti::NO_ACTION);
  owner.disconnect();
  recovering.abandonAfterConnectionLoss();
  std::ofstream marker(
      options.messageHistoryRecoveryMarker,
      std::ios::binary | std::ios::trunc);
  require(
      static_cast<bool>(marker) && marker << "resume-failed\n",
      "Could not write the RTI-closed resume-failure completion marker");
}

void scenarioConnectionFaultRegistrationResponseReplay(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.messageHistoryRecoveryServerManaged,
      "RegisterObjectInstance response replay requires an adapter-managed process fault fixture");
  Session owner(options, model, "owner");
  Session recovering(options, model, "member");
  auto const federation = federationName(options, "registration-response-replay");

  owner.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  recovering.connect();
  recovering.join(options.memberFederateName, options.federateType, federation);
  auto const objectClass =
      recovering.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const attribute = recovering.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "RegisterObjectInstance response replay could not resolve its published class and attribute");
  recovering.rtiAmbassador().publishObjectClassAttributes(
      objectClass,
      rti::AttributeHandleSet{attribute});

  auto const objectName = federation + L"-registration-response-replay";
  recovering.rtiAmbassador().reserveObjectInstanceName(objectName);
  waitFor(
      recovering,
      [&] { return recovering.recorder().reservationSucceeded(objectName); },
      options,
      "RegisterObjectInstance response-replay object-name reservation");

  auto const recoveredObject =
      recovering.rtiAmbassador().registerObjectInstance(objectClass, objectName);
  require(
      recoveredObject.isValid(),
      "RegisterObjectInstance response replay returned an invalid object handle");
  require(
      recovering.rtiAmbassador().getObjectInstanceName(recoveredObject) == objectName,
      "RegisterObjectInstance response replay returned a handle for another object name");
  require(
      recovering.rtiAmbassador().getObjectInstanceHandle(objectName) == recoveredObject,
      "RegisterObjectInstance response replay did not preserve the registered object's stable handle");

  recovering.resign(rti::DELETE_OBJECTS_THEN_DIVEST);
  owner.resign(rti::NO_ACTION);
  recovering.disconnect();
  owner.disconnect();
  if (!options.messageHistoryRecoveryMarker.empty()) {
    std::ofstream marker(
        options.messageHistoryRecoveryMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker) && marker << "recovered\n",
        "Could not write the RegisterObjectInstance response-recovery completion marker");
  }
}

void scenarioConnectionFaultCreateFederationResponseReplay(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.messageHistoryRecoveryServerManaged,
      "CreateFederationExecutionWithMIM response replay requires an adapter-managed process fault fixture");
  Session owner(options, model, "owner");
  auto const federation = federationName(options, "create-federation-response-replay");

  owner.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  auto const joinedHandle = owner.rtiAmbassador().getFederateHandle(
      options.ownerFederateName);
  require(
      joinedHandle.isValid() && joinedHandle == owner.federateHandle(),
      "The replayed federation-creation response did not leave an execution that accepted Join");

  owner.resign(rti::NO_ACTION);
  owner.disconnect();
  if (!options.messageHistoryRecoveryMarker.empty()) {
    std::ofstream marker(
        options.messageHistoryRecoveryMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker) && marker << "recovered\n",
        "Could not write the CreateFederationExecutionWithMIM response-recovery completion marker");
  }
}

void scenarioConnectionFaultReservationResponseReplay(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.messageHistoryRecoveryServerManaged,
      "ReserveObjectInstanceName response replay requires an adapter-managed process fault fixture");
  Session owner(options, model, "owner");
  auto const federation = federationName(options, "reservation-response-replay");

  owner.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const objectClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const attribute = owner.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "ReserveObjectInstanceName response replay could not resolve its published class and attribute");
  owner.rtiAmbassador().publishObjectClassAttributes(
      objectClass,
      rti::AttributeHandleSet{attribute});

  auto const objectName = federation + L"-reservation-response-replay";
  owner.recorder().clearReservations();
  owner.rtiAmbassador().reserveObjectInstanceName(objectName);
  waitFor(
      owner,
      [&] { return owner.recorder().reservationSucceeded(objectName); },
      options,
      "reservation-succeeded callback after response replay");
  auto const waitForReservationCallbacksToSettle = [&] {
    auto const deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(20);
    while (std::chrono::steady_clock::now() < deadline) {
      owner.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  };
  waitForReservationCallbacksToSettle();
  require(
      owner.recorder().reservationSucceededCount(objectName) == 1U,
      "ReserveObjectInstanceName response replay delivered other than one success callback");

  auto const object =
      owner.rtiAmbassador().registerObjectInstance(objectClass, objectName);
  require(object.isValid(), "Named registration after reservation replay returned an invalid handle");
  require(
      owner.rtiAmbassador().getObjectInstanceName(object) == objectName &&
          owner.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "Named registration did not consume the replayed reservation with a stable object identity");
  waitForReservationCallbacksToSettle();
  require(
      owner.recorder().reservationSucceededCount(objectName) == 1U,
      "ReserveObjectInstanceName replay produced a duplicate success callback");

  owner.resign(rti::DELETE_OBJECTS_THEN_DIVEST);
  owner.disconnect();
  if (!options.messageHistoryRecoveryMarker.empty()) {
    std::ofstream marker(
        options.messageHistoryRecoveryMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker) && marker << "recovered\n",
        "Could not write the ReserveObjectInstanceName response-recovery completion marker");
  }
}

void scenarioConnectionFaultDeleteObjectResponseReplay(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.messageHistoryRecoveryServerManaged,
      "DeleteObjectInstance response replay requires an adapter-managed process fault fixture");
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "delete-object-response-replay");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const memberClass =
      member.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const memberAttribute = member.rtiAmbassador().getAttributeHandle(
      memberClass,
      options.attributeName);
  require(
      ownerClass.isValid() && ownerAttribute.isValid() && memberClass.isValid() &&
          memberAttribute.isValid(),
      "DeleteObjectInstance response replay could not resolve the object class and attribute");
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      rti::AttributeHandleSet{ownerAttribute});
  member.rtiAmbassador().subscribeObjectClassAttributes(
      memberClass,
      rti::AttributeHandleSet{memberAttribute},
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  auto const objectName = owner.rtiAmbassador().getObjectInstanceName(object);
  waitFor(
      member,
      [&] { return member.recorder().discovery().present; },
      options,
      "object discovery before DeleteObjectInstance response loss");
  auto const discovered = member.recorder().discovery();
  require(
      object.isValid() && discovered.object == object && discovered.name == objectName,
      "DeleteObjectInstance response replay setup did not discover the registered object");

  std::vector<std::uint8_t> const deletionTagBytes{0xA4U, 0xB5U, 0xC6U};
  rti::VariableLengthData const deletionTag(
      deletionTagBytes.data(), deletionTagBytes.size());
  member.recorder().clearRemovals();
  owner.rtiAmbassador().deleteObjectInstance(object, deletionTag);
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getObjectInstanceHandle(objectName));
      },
      L"ObjectInstanceNotKnown",
      "owner object lookup after replayed DeleteObjectInstance");

  waitFor(
      member,
      [&] { return !member.recorder().removals().empty(); },
      options,
      "remote object-removal callback after DeleteObjectInstance response replay");
  auto const removals = member.recorder().removals();
  require(
      removals.size() == 1U && removals.front().object == object &&
          removals.front().tag == deletionTagBytes &&
          removals.front().producer == owner.federateHandle(),
      "DeleteObjectInstance response replay duplicated or changed the removal notification");
  requireException(
      [&] {
        static_cast<void>(member.rtiAmbassador().getObjectInstanceHandle(objectName));
      },
      L"ObjectInstanceNotKnown",
      "member object lookup after replayed DeleteObjectInstance");

  member.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  member.disconnect();
  owner.disconnect();
  if (!options.messageHistoryRecoveryMarker.empty()) {
    std::ofstream marker(
        options.messageHistoryRecoveryMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker) && marker << "recovered\n",
        "Could not write the DeleteObjectInstance response-recovery completion marker");
  }
}

void scenarioConnectionFaultAttributeUpdateResponseReplay(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.messageHistoryRecoveryServerManaged,
      "UpdateAttributeValues response replay requires an adapter-managed process fault fixture");
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "attribute-update-response-replay");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const memberClass =
      member.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const memberAttribute = member.rtiAmbassador().getAttributeHandle(
      memberClass,
      options.attributeName);
  require(
      ownerClass.isValid() && ownerAttribute.isValid() && memberClass.isValid() &&
          memberAttribute.isValid(),
      "UpdateAttributeValues response replay could not resolve the object class and attribute");
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      rti::AttributeHandleSet{ownerAttribute});
  member.rtiAmbassador().subscribeObjectClassAttributes(
      memberClass,
      rti::AttributeHandleSet{memberAttribute},
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  waitFor(
      member,
      [&] { return member.recorder().discovery().present; },
      options,
      "object discovery before UpdateAttributeValues response loss");
  require(
      object.isValid() && member.recorder().discovery().object == object,
      "UpdateAttributeValues response replay setup did not discover the registered object");

  std::vector<std::uint8_t> const firstValue{0x21U, 0x22U, 0x23U};
  std::vector<std::uint8_t> const firstTag{0xA1U, 0xA2U};
  rti::AttributeHandleValueMap const firstValues{{
      ownerAttribute,
      rti::VariableLengthData(firstValue.data(), firstValue.size())}};
  owner.rtiAmbassador().updateAttributeValues(
      object,
      firstValues,
      rti::VariableLengthData(firstTag.data(), firstTag.size()));

  std::vector<std::uint8_t> const fenceValue{0x31U, 0x32U};
  std::vector<std::uint8_t> const fenceTag{0xB1U};
  rti::AttributeHandleValueMap const fenceValues{{
      ownerAttribute,
      rti::VariableLengthData(fenceValue.data(), fenceValue.size())}};
  owner.rtiAmbassador().updateAttributeValues(
      object,
      fenceValues,
      rti::VariableLengthData(fenceTag.data(), fenceTag.size()));

  waitFor(
      member,
      [&] { return member.recorder().reflections().size() >= 2U; },
      options,
      "UpdateAttributeValues replay and ordered callback-fence reflections");
  auto const reflections = member.recorder().reflections();
  auto const hasExpectedReflection = [&](ReflectionRecord const& reflection,
                                         std::vector<std::uint8_t> const& expectedValue,
                                         std::vector<std::uint8_t> const& expectedTag) {
    auto const value = reflection.values.find(memberAttribute);
    return reflection.object == object && value != reflection.values.end() &&
        copyBytes(value->second) == expectedValue && reflection.tag == expectedTag &&
        reflection.producer == owner.federateHandle();
  };
  require(
      reflections.size() == 2U &&
          hasExpectedReflection(reflections[0], firstValue, firstTag) &&
          hasExpectedReflection(reflections[1], fenceValue, fenceTag),
      "UpdateAttributeValues response replay lost, duplicated, reordered, or changed an attribute reflection");

  member.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  member.disconnect();
  owner.disconnect();
  if (!options.messageHistoryRecoveryMarker.empty()) {
    std::ofstream marker(
        options.messageHistoryRecoveryMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker) && marker << "recovered\n",
        "Could not write the UpdateAttributeValues response-recovery completion marker");
  }
}

void scenarioConnectionFaultSendInteractionResponseReplay(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.messageHistoryRecoveryServerManaged,
      "SendInteraction response replay requires an adapter-managed process fault fixture");
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "send-interaction-response-replay");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto const ownerInteraction = owner.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const ownerParameter = owner.rtiAmbassador().getParameterHandle(
      ownerInteraction,
      options.parameterName);
  auto const memberInteraction = member.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const memberParameter = member.rtiAmbassador().getParameterHandle(
      memberInteraction,
      options.parameterName);
  require(
      ownerInteraction.isValid() && ownerParameter.isValid() &&
          memberInteraction.isValid() && memberParameter.isValid(),
      "SendInteraction response replay could not resolve the interaction and parameter");
  owner.rtiAmbassador().publishInteractionClass(ownerInteraction);
  member.rtiAmbassador().subscribeInteractionClass(memberInteraction);

  std::vector<std::uint8_t> const firstValue{0x61U, 0x62U, 0x63U};
  std::vector<std::uint8_t> const firstTag{0xC1U, 0xC2U};
  rti::ParameterHandleValueMap const firstParameters{{
      ownerParameter,
      rti::VariableLengthData(firstValue.data(), firstValue.size())}};
  owner.rtiAmbassador().sendInteraction(
      ownerInteraction,
      firstParameters,
      rti::VariableLengthData(firstTag.data(), firstTag.size()));

  std::vector<std::uint8_t> const fenceValue{0x71U, 0x72U};
  std::vector<std::uint8_t> const fenceTag{0xD1U};
  rti::ParameterHandleValueMap const fenceParameters{{
      ownerParameter,
      rti::VariableLengthData(fenceValue.data(), fenceValue.size())}};
  owner.rtiAmbassador().sendInteraction(
      ownerInteraction,
      fenceParameters,
      rti::VariableLengthData(fenceTag.data(), fenceTag.size()));

  waitFor(
      member,
      [&] { return member.recorder().interactions().size() >= 2U; },
      options,
      "SendInteraction replay and ordered callback-fence interactions");
  auto const interactions = member.recorder().interactions();
  auto const hasExpectedInteraction = [&](InteractionRecord const& record,
                                          std::vector<std::uint8_t> const& expectedValue,
                                          std::vector<std::uint8_t> const& expectedTag) {
    auto const value = record.parameters.find(memberParameter);
    return record.interaction == memberInteraction &&
        value != record.parameters.end() && copyBytes(value->second) == expectedValue &&
        record.tag == expectedTag && record.producer == owner.federateHandle();
  };
  require(
      interactions.size() == 2U &&
          hasExpectedInteraction(interactions[0], firstValue, firstTag) &&
          hasExpectedInteraction(interactions[1], fenceValue, fenceTag),
      "SendInteraction response replay lost, duplicated, reordered, or changed a received interaction");

  member.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  member.disconnect();
  owner.disconnect();
  if (!options.messageHistoryRecoveryMarker.empty()) {
    std::ofstream marker(
        options.messageHistoryRecoveryMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker) && marker << "recovered\n",
        "Could not write the SendInteraction response-recovery completion marker");
  }
}

void scenarioConnectionFaultTimeRegulationResponseReplay(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.messageHistoryRecoveryServerManaged,
      "EnableTimeRegulation response replay requires an adapter-managed process fault fixture");
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "time-regulation-response-replay");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto const time = makeTimeContext(owner);
  require(
      time.factory->getName() == options.logicalTimeImplementationName,
      "EnableTimeRegulation response replay selected an unexpected logical-time factory");
  owner.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(
      owner,
      [&] { return owner.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Time Regulation Enabled callback after response replay");
  auto const enabled = owner.recorder().timeRegulationEnabled();
  require(
      enabled.size() == 1U &&
          enabled.front().encoded == encodeTime(*time.initial),
      "EnableTimeRegulation response replay lost or duplicated the callback or changed its time");

  auto queriedTime = time.factory->makeInitial();
  require(queriedTime != nullptr,
          "EnableTimeRegulation response replay could not allocate a query time");
  owner.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime,
      *time.initial,
      "Query Logical Time after EnableTimeRegulation response replay");
  if (model == rti::HLA_EVOKED) {
    while (owner.rtiAmbassador().evokeCallback(0.0)) {
    }
  }
  require(
      owner.recorder().timeRegulationEnabled().size() == 1U,
      "EnableTimeRegulation response replay duplicated Time Regulation Enabled after a subsequent time service");

  member.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  member.disconnect();
  owner.disconnect();
  if (!options.messageHistoryRecoveryMarker.empty()) {
    std::ofstream marker(
        options.messageHistoryRecoveryMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker) && marker << "recovered\n",
        "Could not write the EnableTimeRegulation response-recovery completion marker");
  }
}

void scenarioConnectionFaultTimeConstrainedResponseReplay(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.messageHistoryRecoveryServerManaged,
      "EnableTimeConstrained response replay requires an adapter-managed process fault fixture");
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "time-constrained-response-replay");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto const time = makeTimeContext(owner);
  require(
      time.factory->getName() == options.logicalTimeImplementationName,
      "EnableTimeConstrained response replay selected an unexpected logical-time factory");
  owner.rtiAmbassador().enableTimeConstrained();
  waitFor(
      owner,
      [&] { return owner.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Time Constrained Enabled callback after response replay");
  auto const enabled = owner.recorder().timeConstrainedEnabled();
  require(
      enabled.size() == 1U && enabled.front().encoded == encodeTime(*time.initial),
      "EnableTimeConstrained response replay lost or duplicated the callback or changed its time");

  auto queriedTime = time.factory->makeInitial();
  require(queriedTime != nullptr,
          "EnableTimeConstrained response replay could not allocate a query time");
  owner.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime,
      *time.initial,
      "Query Logical Time after EnableTimeConstrained response replay");
  if (model == rti::HLA_EVOKED) {
    while (owner.rtiAmbassador().evokeCallback(0.0)) {
    }
  }
  require(
      owner.recorder().timeConstrainedEnabled().size() == 1U,
      "EnableTimeConstrained response replay duplicated Time Constrained Enabled after a subsequent time service");

  member.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  member.disconnect();
  owner.disconnect();
  if (!options.messageHistoryRecoveryMarker.empty()) {
    std::ofstream marker(
        options.messageHistoryRecoveryMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker) && marker << "recovered\n",
        "Could not write the EnableTimeConstrained response-recovery completion marker");
  }
}

void scenarioConnectionFaultTimeAdvanceRequestResponseReplay(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.messageHistoryRecoveryServerManaged,
      "TimeAdvanceRequest response replay requires an adapter-managed process fault fixture");
  Session regulator(options, model, "owner");
  Session constrained(options, model, "member");
  auto const federation = federationName(
      options,
      "time-advance-request-response-replay");
  connectAndJoin(regulator, constrained, options, federation, options.fom);

  auto const time = makeTimeContext(regulator);
  require(
      time.factory->getName() == options.logicalTimeImplementationName,
      "TimeAdvanceRequest response replay selected an unexpected logical-time factory");
  regulator.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "TimeAdvanceRequest response replay regulation callback");
  auto const target = timeAfter(
      *time.factory,
      *time.initial,
      *time.epsilon,
      4U);
  regulator.rtiAmbassador().timeAdvanceRequest(*target);

  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "Time Advance Grant callback after TimeAdvanceRequest response replay");
  auto const grants = regulator.recorder().timeAdvanceGrants();
  require(
      grants.size() == 1U && grants.front().encoded == encodeTime(*target),
      "TimeAdvanceRequest response replay lost, duplicated, or changed a requested-time grant");

  auto logicalTime = time.factory->makeInitial();
  require(
      logicalTime != nullptr,
      "TimeAdvanceRequest response replay could not allocate query times");
  regulator.rtiAmbassador().queryLogicalTime(*logicalTime);
  requireTimeEquals(
      *logicalTime,
      *target,
      "Query Logical Time after TimeAdvanceRequest response replay");
  if (model == rti::HLA_EVOKED) {
    while (regulator.rtiAmbassador().evokeCallback(0.0)) {
    }
  }
  require(
      regulator.recorder().timeAdvanceGrants().size() == 1U,
      "TimeAdvanceRequest response replay duplicated a Time Advance Grant");

  constrained.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  constrained.disconnect();
  regulator.disconnect();
  if (!options.messageHistoryRecoveryMarker.empty()) {
    std::ofstream marker(
        options.messageHistoryRecoveryMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker) && marker << "recovered\n",
        "Could not write the TimeAdvanceRequest response-recovery completion marker");
  }
}

void scenarioConnectionFaultTimeAdvanceRequestAvailableResponseReplay(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.messageHistoryRecoveryServerManaged,
      "TimeAdvanceRequestAvailable response replay requires an adapter-managed process fault fixture");
  Session regulator(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(
      options,
      "time-advance-request-available-response-replay");
  connectAndJoin(regulator, peer, options, federation, options.fom);

  auto const time = makeTimeContext(regulator);
  require(
      time.factory->getName() == options.logicalTimeImplementationName,
      "TimeAdvanceRequestAvailable response replay selected an unexpected logical-time factory");
  regulator.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "TimeAdvanceRequestAvailable response replay regulation callback");
  auto const regulationCallbacks = regulator.recorder().timeRegulationEnabled();
  require(
      regulationCallbacks.size() == 1U &&
          regulationCallbacks.front().encoded == encodeTime(*time.initial),
      "TimeAdvanceRequestAvailable response replay did not establish regulation exactly once at the initial time");

  auto const target = timeAfter(
      *time.factory,
      *time.initial,
      *time.epsilon,
      4U);
  regulator.rtiAmbassador().timeAdvanceRequestAvailable(*target);

  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "Time Advance Grant callback after TimeAdvanceRequestAvailable response replay");
  auto const grants = regulator.recorder().timeAdvanceGrants();
  require(
      grants.size() == 1U && grants.front().encoded == encodeTime(*target),
      "TimeAdvanceRequestAvailable response replay lost, duplicated, or changed the requested-time grant");

  auto logicalTime = time.factory->makeInitial();
  require(
      logicalTime != nullptr,
      "TimeAdvanceRequestAvailable response replay could not allocate a query time");
  regulator.rtiAmbassador().queryLogicalTime(*logicalTime);
  requireTimeEquals(
      *logicalTime,
      *target,
      "Query Logical Time after TimeAdvanceRequestAvailable response replay");
  if (model == rti::HLA_EVOKED) {
    while (regulator.rtiAmbassador().evokeCallback(0.0)) {
    }
  }
  require(
      regulator.recorder().timeAdvanceGrants().size() == 1U,
      "TimeAdvanceRequestAvailable response replay duplicated a Time Advance Grant");

  peer.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  peer.disconnect();
  regulator.disconnect();
  if (!options.messageHistoryRecoveryMarker.empty()) {
    std::ofstream marker(
        options.messageHistoryRecoveryMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker) && marker << "recovered\n",
        "Could not write the TimeAdvanceRequestAvailable response-recovery completion marker");
  }
}

namespace {

void runConnectionFaultNextMessageRequestResponseReplay(
    Options const& options,
    rti::CallbackModel model,
    bool requestAvailable) {
  std::string const requestName = requestAvailable
      ? "NextMessageRequestAvailable"
      : "NextMessageRequest";
  std::string const federationSuffix = requestAvailable
      ? "next-message-request-available-response-replay"
      : "next-message-request-response-replay";
  require(
      options.messageHistoryRecoveryServerManaged,
      requestName + " response replay requires an adapter-managed process fault fixture");
  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      federationSuffix);
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && publisherParameter.isValid() &&
          receiverInteraction.isValid() && receiverParameter.isValid() &&
          publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter,
      requestName + " response replay could not resolve consistent interaction handles");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  enableTimestampedRoles(
      publisher,
      receiver,
      publisherTime,
      receiverTime,
      options,
      federationSuffix);
  publisher.recorder().clearTimeCallbacks();
  receiver.recorder().clearTimeCallbacks();
  publisher.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const requestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      3U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  // TAR(2) with unit lookahead makes time 3 the inclusive NMRA/GALT edge.
  requireTimeEquals(
      *requestBoundary,
      *messageTime,
      requestName + " response replay request must include the queued message exactly at its time boundary");
  std::vector<std::uint8_t> const valueBytes{0x4EU, 0x4DU, 0x52U};
  std::vector<std::uint8_t> const tagBytes{0x52U, 0x50U, 0x4CU};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      requestName + " response replay send returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      requestName + " response replay delivered queued TSO before the request");

  if (requestAvailable) {
    receiver.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  } else {
    receiver.rtiAmbassador().nextMessageRequest(*requestBoundary);
  }
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedInteractions().size() >= 1U;
      },
      options,
      requestName + " response replay delivery and grant");

  auto const grants = receiver.recorder().timeAdvanceGrants();
  auto const interactions = receiver.recorder().timedInteractions();
  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U &&
          grants.size() == 1U && interactions.size() == 1U,
      requestName + " response replay lost or duplicated a grant or TSO callback");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      requestName + " response replay granted before delivering the queued TSO");
  auto const received = interactions.front();
  require(
      received.interaction == receiverInteraction &&
          received.parameters.size() == 1U &&
          received.parameters.count(receiverParameter) == 1U &&
          copyBytes(received.parameters.at(receiverParameter)) == valueBytes &&
          received.tag == tagBytes &&
          received.producer == publisher.federateHandle() &&
          received.time == encodeTime(*messageTime) &&
          received.sentOrder == rti::TIMESTAMP &&
          received.receivedOrder == rti::TIMESTAMP &&
          received.retractionPresent &&
          received.retraction == copyBytes(retraction.encode()),
      requestName + " response replay changed the queued TSO callback");
  require(
      grants.front().encoded == encodeTime(*messageTime),
      requestName + " response replay granted at the wrong logical time");

  auto logicalTime = receiverTime.factory->makeInitial();
  require(
      logicalTime != nullptr,
      requestName + " response replay could not allocate a query time");
  receiver.rtiAmbassador().queryLogicalTime(*logicalTime);
  requireTimeEquals(
      *logicalTime,
      *messageTime,
      "Query Logical Time after " + requestName + " response replay");
  if (model == rti::HLA_EVOKED) {
    while (receiver.rtiAmbassador().evokeCallback(0.0)) {
    }
  }
  require(
      receiver.recorder().timeAdvanceGrants().size() == 1U &&
          receiver.recorder().timedInteractions().size() == 1U,
      requestName + " response replay duplicated callbacks after evoking pending events");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  receiver.disconnect();
  publisher.disconnect();
  if (!options.messageHistoryRecoveryMarker.empty()) {
    std::ofstream marker(
        options.messageHistoryRecoveryMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker) && marker << "recovered\n",
        "Could not write the " + requestName + " response-recovery completion marker");
  }
}

}  // namespace

void scenarioConnectionFaultNextMessageRequestResponseReplay(
    Options const& options,
    rti::CallbackModel model) {
  runConnectionFaultNextMessageRequestResponseReplay(options, model, false);
}

void scenarioConnectionFaultNextMessageRequestAvailableResponseReplay(
    Options const& options,
    rti::CallbackModel model) {
  runConnectionFaultNextMessageRequestResponseReplay(options, model, true);
}

void scenarioConnectionFaultInactiveSessionPurge(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.messageHistoryRecoveryServerManaged &&
          !options.messageHistorySessionPurgeTriggerMarker.empty() &&
          !options.messageHistoryRecoveryMarker.empty(),
      "Inactive-session purge requires an adapter-managed transport-fault fixture");
  bool writeCompletionMarker = false;
  struct CompletionMarker final {
    Options const& options;
    bool const& shouldWrite;

    ~CompletionMarker() {
      if (!shouldWrite) {
        return;
      }
      std::ofstream completion(
          options.messageHistoryRecoveryMarker,
          std::ios::binary | std::ios::trunc);
      if (completion) {
        completion << "recovered\n";
      }
    }
  } completionMarker{options, writeCompletionMarker};
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(
      options,
      "connection-fault-inactive-session-purge");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto const memberName = options.memberFederateName;
  auto const memberHandle = member.federateHandle();
  require(
      owner.rtiAmbassador().getFederateHandle(memberName) == memberHandle,
      "The member was not visible before the inactive-session purge fault");

  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const memberClass =
      member.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const memberAttribute = member.rtiAmbassador().getAttributeHandle(
      memberClass,
      options.attributeName);
  require(
      ownerClass.isValid() && memberClass.isValid() &&
          ownerAttribute.isValid() && memberAttribute.isValid(),
      "Inactive-session purge automatic-resign setup returned invalid object handles");
  owner.rtiAmbassador().subscribeObjectClassAttributes(
      ownerClass, rti::AttributeHandleSet{ownerAttribute}, true, L"");
  member.rtiAmbassador().publishObjectClassAttributes(
      memberClass, rti::AttributeHandleSet{memberAttribute});
  auto const memberObject =
      member.rtiAmbassador().registerObjectInstance(memberClass);
  require(
      memberObject.isValid(),
      "Inactive-session purge automatic-resign object registration returned an invalid handle");
  waitFor(
      owner,
      [&] { return owner.recorder().hasDiscovery(memberObject); },
      options,
      "inactive-session purge automatic-resign object discovery");
  member.rtiAmbassador().setAutomaticResignDirective(
      rti::DELETE_OBJECTS_THEN_DIVEST);
  require(
      member.rtiAmbassador().getAutomaticResignDirective() ==
          rti::DELETE_OBJECTS_THEN_DIVEST,
      "Inactive-session purge automatic-resign directive did not round-trip");
  owner.recorder().clearRemovals();

  {
    std::ofstream trigger(
        options.messageHistorySessionPurgeTriggerMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(trigger) && trigger << "drop-member-transport\n",
        "Could not signal the adapter to drop the member transport");
  }

  bool purged = false;
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (std::chrono::steady_clock::now() < deadline) {
    try {
      auto const observed = owner.rtiAmbassador().getFederateHandle(memberName);
      require(
          observed == memberHandle,
          "The inactive-session purge changed the member's stable handle");
    } catch (rti::NameNotFound const&) {
      purged = true;
      break;
    }
    owner.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  require(
      purged,
      "The RTI did not remove the member after its inactive session purge timeout");
  waitFor(
      owner,
      [&] {
        return owner.recorder().hasRemoval(memberObject, {}, memberHandle);
      },
      options,
      "inactive-session purge automatic-resign object removal");

  owner.resign(rti::NO_ACTION);
  owner.disconnect();
  member.destroyAfterConnectionLoss();
  writeCompletionMarker = true;
}

void scenarioConnectionLossAutomaticUnconditionalDivestiture(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.connectionLossServerManaged,
      "Automatic connection-loss divestiture requires an adapter-managed fault fixture");
  Session survivor(options, model, "owner");
  Session lost(options, model, "member");
  auto const federation = federationName(
      options,
      "connection-loss-automatic-unconditional-divestiture");
  connectAndJoin(survivor, lost, options, federation, options.fom);

  auto const lostClass =
      lost.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const survivorClass =
      survivor.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const lostAttribute =
      lost.rtiAmbassador().getAttributeHandle(lostClass, options.attributeName);
  auto const survivorAttribute = survivor.rtiAmbassador().getAttributeHandle(
      survivorClass,
      options.attributeName);
  auto const lostPrivilege = lost.rtiAmbassador().getAttributeHandle(
      lostClass,
      L"HLAprivilegeToDeleteObject");
  auto const survivorPrivilege = survivor.rtiAmbassador().getAttributeHandle(
      survivorClass,
      L"HLAprivilegeToDeleteObject");
  require(
      lostClass.isValid() && survivorClass.isValid(),
      "automatic connection-loss divestiture object-class lookup returned an invalid handle");
  require(
      lostAttribute.isValid() && survivorAttribute.isValid() &&
          lostPrivilege.isValid() && survivorPrivilege.isValid(),
      "automatic connection-loss divestiture attribute lookup returned an invalid handle");

  rti::AttributeHandleSet const lostAttributes{lostAttribute};
  rti::AttributeHandleSet const survivorAttributes{survivorAttribute};
  rti::AttributeHandleSet const expectedAssumption{
      survivorAttribute,
      survivorPrivilege};
  lost.rtiAmbassador().publishObjectClassAttributes(lostClass, lostAttributes);
  survivor.rtiAmbassador().publishObjectClassAttributes(
      survivorClass,
      survivorAttributes);
  survivor.rtiAmbassador().subscribeObjectClassAttributes(
      survivorClass,
      survivorAttributes,
      true,
      L"");

  auto const object = lost.rtiAmbassador().registerObjectInstance(lostClass);
  require(
      object.isValid(),
      "automatic connection-loss divestiture registration returned an invalid handle");
  auto const objectName = lost.rtiAmbassador().getObjectInstanceName(object);
  waitFor(
      survivor,
      [&] { return survivor.recorder().hasDiscovery(object); },
      options,
      "automatic connection-loss divestiture object discovery");

  lost.rtiAmbassador().setAutomaticResignDirective(
      rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  require(
      lost.rtiAmbassador().getAutomaticResignDirective() ==
          rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES,
      "automatic connection-loss divestiture directive did not round-trip");
  survivor.recorder().clearOwnershipRecords();

  waitForConnectionLossAndSignal(
      lost,
      options,
      "automatic unconditional-divestiture",
      model);
  waitFor(
      survivor,
      [&] { return survivor.recorder().ownershipAssumption().has_value(); },
      options,
      "automatic unconditional-divestiture ownership assumption");
  auto const assumption = survivor.recorder().ownershipAssumption();
  require(
      assumption->object == object &&
          assumption->attributes == expectedAssumption && assumption->tag.empty(),
      "automatic unconditional-divestiture offered the wrong ownership set or tag");
  require(
      survivor.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "automatic unconditional-divestiture removed the retained object");
  require(
      !survivor.rtiAmbassador().isAttributeOwnedByFederate(
          object,
          survivorAttribute) &&
          !survivor.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              survivorPrivilege),
      "automatic unconditional-divestiture left the survivor owning released attributes");

  survivor.resign(rti::NO_ACTION);
  survivor.disconnect();
}

void scenarioConnectionLossAutomaticUnconditionalDivestitureContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioConnectionLossAutomaticUnconditionalDivestiture(options, model);
}

void scenarioConnectionLossAutomaticDeleteThenDivest(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.connectionLossServerManaged,
      "Automatic connection-loss delete-then-divest requires an adapter-managed fault fixture");
  Session survivor(options, model, "owner");
  Session lost(options, model, "member");
  auto const federation = federationName(
      options,
      "connection-loss-automatic-delete-then-divest");
  connectAndJoin(survivor, lost, options, federation, options.fom);

  auto const survivorClass =
      survivor.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const lostClass =
      lost.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const survivorAttribute = survivor.rtiAmbassador().getAttributeHandle(
      survivorClass, options.attributeName);
  auto const lostAttribute = lost.rtiAmbassador().getAttributeHandle(
      lostClass, options.attributeName);
  require(
      survivorClass.isValid() && lostClass.isValid() &&
          survivorAttribute.isValid() && lostAttribute.isValid(),
      "automatic delete-then-divest setup returned an invalid class or attribute handle");

  rti::AttributeHandleSet const survivorAttributes{survivorAttribute};
  rti::AttributeHandleSet const lostAttributes{lostAttribute};
  survivor.rtiAmbassador().publishObjectClassAttributes(
      survivorClass, survivorAttributes);
  lost.rtiAmbassador().publishObjectClassAttributes(lostClass, lostAttributes);
  survivor.rtiAmbassador().subscribeObjectClassAttributes(
      survivorClass, survivorAttributes, true, L"");
  lost.rtiAmbassador().subscribeObjectClassAttributes(
      lostClass, lostAttributes, true, L"");

  auto const retainedObject =
      survivor.rtiAmbassador().registerObjectInstance(survivorClass);
  auto const retainedName =
      survivor.rtiAmbassador().getObjectInstanceName(retainedObject);
  require(retainedObject.isValid(), "automatic delete-then-divest retained object is invalid");
  waitFor(
      lost,
      [&] { return lost.recorder().hasDiscovery(retainedObject); },
      options,
      "automatic delete-then-divest retained-object discovery");
  survivor.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      retainedObject, survivorAttributes, rti::VariableLengthData{});
  std::vector<std::uint8_t> const acquisitionTagBytes{0xD4U, 0x25U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  lost.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      retainedObject, lostAttributes, acquisitionTag);
  waitFor(
      lost,
      [&] { return lost.recorder().ownershipAcquisition().has_value(); },
      options,
      "automatic delete-then-divest setup acquisition");
  auto const acquisition = lost.recorder().ownershipAcquisition();
  require(
      acquisition->object == retainedObject &&
          acquisition->attributes == lostAttributes &&
          acquisition->tag == acquisitionTagBytes &&
          lost.rtiAmbassador().isAttributeOwnedByFederate(
              retainedObject, lostAttribute),
      "automatic delete-then-divest did not establish lost-member ownership of the retained attribute");

  auto const deletedObject =
      lost.rtiAmbassador().registerObjectInstance(lostClass);
  require(
      deletedObject.isValid() && deletedObject != retainedObject,
      "automatic delete-then-divest departing object is invalid or duplicated");
  waitFor(
      survivor,
      [&] { return survivor.recorder().hasDiscovery(deletedObject); },
      options,
      "automatic delete-then-divest departing-object discovery");
  lost.rtiAmbassador().setAutomaticResignDirective(
      rti::DELETE_OBJECTS_THEN_DIVEST);
  require(
      lost.rtiAmbassador().getAutomaticResignDirective() ==
          rti::DELETE_OBJECTS_THEN_DIVEST,
      "automatic delete-then-divest directive did not round-trip");

  survivor.recorder().clearOwnershipRecords();
  survivor.recorder().clearRemovals();
  auto const lostFederate = lost.federateHandle();
  waitForConnectionLossAndSignal(
      lost, options, "automatic delete-then-divest", model);
  waitFor(
      survivor,
      [&] {
        return survivor.recorder().ownershipAssumption().has_value() &&
            survivor.recorder().hasRemoval(deletedObject, {}, lostFederate);
      },
      options,
      "automatic delete-then-divest survivor callbacks");
  auto const assumption = survivor.recorder().ownershipAssumption();
  require(
      assumption->object == retainedObject &&
          assumption->attributes == survivorAttributes &&
          assumption->tag.empty(),
      "automatic delete-then-divest offered the wrong retained attribute");
  auto const removals = survivor.recorder().removals();
  require(
      removals.size() == 1U && removals.front().object == deletedObject &&
          removals.front().tag.empty() &&
          removals.front().producer == lostFederate,
      "automatic delete-then-divest did not remove exactly the departing object");
  require(
      survivor.rtiAmbassador().getObjectInstanceHandle(retainedName) ==
          retainedObject &&
          !survivor.rtiAmbassador().isAttributeOwnedByFederate(
              retainedObject, survivorAttribute),
      "automatic delete-then-divest removed the retained object or left its attribute owned");
  survivor.resign(rti::NO_ACTION);
  survivor.disconnect();
}

void scenarioConnectionLossAutomaticDeleteThenDivestContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioConnectionLossAutomaticDeleteThenDivest(options, model);
}

void scenarioConnectionLossAutomaticCancelPendingAcquisition(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.connectionLossServerManaged,
      "Automatic connection-loss acquisition cancellation requires an adapter-managed fault fixture");
  Session owner(options, model, "owner");
  Session lost(options, model, "member");
  Session survivor(options, model, "survivor");
  auto const federation = federationName(
      options,
      "connection-loss-automatic-cancel-pending-acquisition");
  connectAndJoin(owner, lost, options, federation, options.fom);
  survivor.connect();
  survivor.join(
      options.memberFederateName + L"-survivor",
      options.federateType,
      federation);

  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const lostClass =
      lost.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const survivorClass =
      survivor.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute =
      owner.rtiAmbassador().getAttributeHandle(ownerClass, options.attributeName);
  auto const lostAttribute =
      lost.rtiAmbassador().getAttributeHandle(lostClass, options.attributeName);
  auto const survivorAttribute = survivor.rtiAmbassador().getAttributeHandle(
      survivorClass,
      options.attributeName);
  require(
      ownerClass.isValid() && lostClass.isValid() && survivorClass.isValid(),
      "automatic acquisition-cancellation object-class lookup returned an invalid handle");
  require(
      ownerAttribute.isValid() && lostAttribute.isValid() && survivorAttribute.isValid(),
      "automatic acquisition-cancellation attribute lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const lostAttributes{lostAttribute};
  rti::AttributeHandleSet const survivorAttributes{survivorAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  lost.rtiAmbassador().publishObjectClassAttributes(lostClass, lostAttributes);
  lost.rtiAmbassador().subscribeObjectClassAttributes(
      lostClass,
      lostAttributes,
      true,
      L"");
  survivor.rtiAmbassador().publishObjectClassAttributes(
      survivorClass,
      survivorAttributes);
  survivor.rtiAmbassador().subscribeObjectClassAttributes(
      survivorClass,
      survivorAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "automatic acquisition-cancellation registration returned an invalid handle");
  waitFor(
      lost,
      [&] { return lost.recorder().hasDiscovery(object); },
      options,
      "automatic acquisition-cancellation lost-member discovery");
  require(
      !lost.rtiAmbassador().isAttributeOwnedByFederate(object, lostAttribute),
      "automatic acquisition-cancellation lost member unexpectedly owns the attribute");
  waitFor(
      survivor,
      [&] { return survivor.recorder().hasDiscovery(object); },
      options,
      "automatic acquisition-cancellation survivor discovery");
  require(
      !survivor.rtiAmbassador().isAttributeOwnedByFederate(object, survivorAttribute),
      "automatic acquisition-cancellation survivor unexpectedly owns the attribute");

  std::vector<std::uint8_t> const acquisitionTagBytes{0xD6U, 0x25U};
  std::vector<std::uint8_t> const divestitureTagBytes{0xD7U, 0x25U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(),
      divestitureTagBytes.size());
  lost.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      lostAttributes,
      acquisitionTag);
  require(
      !owner.recorder().ownershipReleaseRequest().has_value(),
      "automatic acquisition-cancellation queued an owner release callback too early");
  lost.rtiAmbassador().setAutomaticResignDirective(
      rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
  require(
      lost.rtiAmbassador().getAutomaticResignDirective() ==
          rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS,
      "automatic acquisition-cancellation directive did not round-trip");
  survivor.recorder().clearOwnershipRecords();

  waitForConnectionLossAndSignal(
      lost,
      options,
      "automatic pending-acquisition cancellation",
      model);
  for (int pass = 0; pass != 8; ++pass) {
    owner.pump();
  }
  require(
      !owner.recorder().ownershipReleaseRequest().has_value(),
      "automatic pending-acquisition cancellation delivered stale owner release work");

  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object,
      ownerAttributes,
      divestitureTag);
  waitFor(
      survivor,
      [&] { return survivor.recorder().ownershipAssumption().has_value(); },
      options,
      "automatic acquisition-cancellation survivor assumption");
  auto const assumption = survivor.recorder().ownershipAssumption();
  require(
      assumption->object == object &&
          assumption->attributes == survivorAttributes &&
          assumption->tag == divestitureTagBytes,
      "automatic acquisition-cancellation offered the wrong surviving ownership metadata");
  require(
      !survivor.rtiAmbassador().isAttributeOwnedByFederate(
          object,
          survivorAttribute),
      "automatic acquisition-cancellation transferred ownership before acquisition");

  survivor.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.disconnect();
  survivor.disconnect();
}

void scenarioConnectionLossAutomaticCancelPendingAcquisitionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioConnectionLossAutomaticCancelPendingAcquisition(options, model);
}

void scenarioConnectionUnknownConfigurationFallback(
    Options const& options,
    rti::CallbackModel model) {
  static_cast<void>(options);
  rti::RTIambassadorFactory factory;
  auto ambassador = factory.createRTIambassador();
  require(
      static_cast<bool>(ambassador),
      "unknown-configuration fallback did not create a standard RTIambassador");

  auto const nonce = std::chrono::steady_clock::now().time_since_epoch().count();
  auto const unknownName = std::wstring(L"hla-rti-tck-unknown-configuration-") +
      std::to_wstring(nonce);
  auto configuration = rti::RtiConfiguration::createConfiguration();
  configuration.withConfigurationName(unknownName);
  Recorder recorder;
  auto const result = ambassador->connect(recorder, model, configuration);
  require(
      result.additionalSettingsResult == rti::SETTINGS_IGNORED ||
          result.additionalSettingsResult == rti::SETTINGS_FAILED_TO_PARSE ||
          result.additionalSettingsResult == rti::SETTINGS_APPLIED,
      "unknown configuration returned an invalid additional-settings result code");
  ambassador->disconnect();
}

void scenarioFederateLostMomReportPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.connectionLossServerManaged,
      "Federate-lost MOM reporting requires an adapter-managed fault fixture");
  require(
      !options.fom.empty(),
      "Federate-lost MOM reporting requires an adapter-supplied application FOM module");
  require(
      !options.mimFom.empty(),
      "Federate-lost MOM reporting requires an adapter-supplied standard MIM");
  require(
      !options.interactionClassName.empty(),
      "Federate-lost TSO cutoff reporting requires an adapter-supplied interaction");
  auto withStage = [](char const* stage, auto&& operation) -> decltype(operation()) {
    try {
      return operation();
    } catch (rti::Exception const& error) {
      throw std::runtime_error(
          std::string("HLAreportFederateLost ") + stage + ": " +
          toNarrow(error.name()) + ": " + toNarrow(error.what()));
    } catch (std::exception const& error) {
      throw std::runtime_error(
          std::string("HLAreportFederateLost ") + stage + ": " + error.what());
    }
  };

  Session observer(options, model, "owner");
  Session lost(options, model, "member");
  auto const federation = federationName(options, "federate-lost-mom-report");
  auto const observerConnection = withStage(
      "observer connect failed", [&] { return observer.connect(); });
  auto const lostConnection = withStage(
      "lost-member connect failed", [&] { return lost.connect(); });
  require(
      observerConnection.addressUsed && lostConnection.addressUsed,
      "The adapter rtiAddress was not selected for both federates: " +
          options.rtiAddress);
  // The application module supplies the non-empty FOM module list required
  // for a valid federation create; the tested interaction remains defined by
  // that module and the standard MIM supplies the loss-report class.
  withStage("federation creation failed", [&] {
    observer.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  });

  auto const observerName = options.ownerFederateName;
  auto const lostName = options.memberFederateName;
  withStage("observer join failed", [&] {
    observer.join(observerName, options.federateType, federation);
  });
  withStage("lost-member join failed", [&] {
    lost.join(lostName, options.federateType, federation);
  });
  auto const observerFederate = observer.federateHandle();
  auto const lostFederate = lost.federateHandle();

  auto const federationMomClass = withStage(
      "federation MOM class lookup failed", [&] {
        return observer.rtiAmbassador().getObjectClassHandle(
            L"HLAobjectRoot.HLAmanager.HLAfederation");
      });
  auto const federatesInFederationAttribute = withStage(
      "federation membership attribute lookup failed", [&] {
        return observer.rtiAmbassador().getAttributeHandle(
            federationMomClass, L"HLAfederatesInFederation");
      });
  require(
      federationMomClass.isValid() &&
          federatesInFederationAttribute.isValid(),
      "Federation MOM membership lookup returned an invalid handle");
  withStage("federation membership subscription failed", [&] {
    observer.rtiAmbassador().subscribeObjectClassAttributes(
        federationMomClass,
        rti::AttributeHandleSet{federatesInFederationAttribute},
        true);
  });
  waitFor(
      observer,
      [&] {
        auto const discovery = observer.recorder().discovery();
        return discovery.present &&
            discovery.objectClass == federationMomClass &&
            discovery.name == L"HLAfederation";
      },
      options,
      "federation MOM membership object discovery");
  auto const federationMomObject = observer.recorder().discovery().object;
  auto const matchingMembershipReflections = [&] {
    auto const reflections = observer.recorder().reflections();
    return static_cast<std::size_t>(std::count_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.object == federationMomObject &&
              reflection.values.count(federatesInFederationAttribute) == 1U;
        }));
  };
  auto const latestMembership = [&]()
      -> std::optional<std::vector<rti::FederateHandle>> {
    auto const reflections = observer.recorder().reflections();
    auto const reflection = std::find_if(
        reflections.rbegin(),
        reflections.rend(),
        [&](ReflectionRecord const& candidate) {
          return candidate.object == federationMomObject &&
              candidate.values.count(federatesInFederationAttribute) == 1U;
        });
    if (reflection == reflections.rend()) {
      return std::nullopt;
    }
    auto const raw = copyBytes(
        reflection->values.at(federatesInFederationAttribute));
    std::vector<rti::Octet> bytes(raw.begin(), raw.end());
    rti::HLAinteger32BE count;
    auto index = count.decodeFrom(bytes, 0U);
    require(
        count.get() >= 0,
        "HLAfederatesInFederation encoded a negative element count");
    std::vector<rti::FederateHandle> result;
    result.reserve(static_cast<std::size_t>(count.get()));
    for (rti::Integer32 element = 0; element < count.get(); ++element) {
      auto const referenceStart = index;
      rti::HLAinteger32BE encodedReferenceLength;
      index = encodedReferenceLength.decodeFrom(bytes, index);
      require(
          encodedReferenceLength.get() == 9 &&
              index + 9U <= bytes.size(),
          "HLAfederatesInFederation encoded a malformed HLAfederateReference");
      result.push_back(observer.rtiAmbassador().decodeFederateHandle(
          rti::VariableLengthData(
              bytes.data() + referenceStart,
              index + 9U - referenceStart)));
      index += 9U;
    }
    require(
        index == bytes.size(),
        "HLAfederatesInFederation encoded trailing bytes");
    return result;
  };
  auto const reportClass = withStage("interaction-class lookup failed", [&] {
    return observer.rtiAmbassador().getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFederateLost");
  });
  require(reportClass.isValid(), "HLAreportFederateLost lookup returned an invalid handle");
  auto const federateParameter = withStage("HLAfederate parameter lookup failed", [&] {
    return observer.rtiAmbassador().getParameterHandle(reportClass, L"HLAfederate");
  });
  auto const nameParameter = withStage("HLAfederateName parameter lookup failed", [&] {
    return observer.rtiAmbassador().getParameterHandle(reportClass, L"HLAfederateName");
  });
  auto const timeParameter = withStage("HLAtimeStamp parameter lookup failed", [&] {
    return observer.rtiAmbassador().getParameterHandle(reportClass, L"HLAtimeStamp");
  });
  auto const faultParameter = withStage("HLAfaultDescription parameter lookup failed", [&] {
    return observer.rtiAmbassador().getParameterHandle(reportClass, L"HLAfaultDescription");
  });
  require(
      federateParameter.isValid() && nameParameter.isValid() &&
          timeParameter.isValid() && faultParameter.isValid(),
      "HLAreportFederateLost parameter lookup returned an invalid handle");
  withStage("report subscription failed", [&] {
    observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  });

  auto const interaction = withStage("application interaction lookup failed", [&] {
    return lost.rtiAmbassador().getInteractionClassHandle(
        options.interactionClassName);
  });
  require(interaction.isValid(),
          "Federate-lost TSO cutoff could not resolve the adapter application interaction");
  withStage("application interaction publication failed", [&] {
    lost.rtiAmbassador().publishInteractionClass(interaction);
    lost.rtiAmbassador().changeInteractionOrderType(interaction, rti::TIMESTAMP);
  });
  withStage("application interaction subscription failed", [&] {
    observer.rtiAmbassador().subscribeInteractionClass(interaction, true);
  });

  auto lostTime = makeTimeContext(lost);
  auto observerTime = makeTimeContext(observer);
  enableTimestampedRoles(
      lost,
      observer,
      lostTime,
      observerTime,
      options,
      "HLAreportFederateLost TSO cutoff");
  withStage("automatic resign directive setup failed", [&] {
    lost.rtiAmbassador().setAutomaticResignDirective(rti::NO_ACTION);
  });
  withStage("asynchronous delivery enablement failed", [&] {
    observer.rtiAmbassador().enableAsynchronousDelivery();
  });
  auto const cutoff = timeAfter(
      *lostTime.factory,
      *lostTime.initial,
      *lostTime.epsilon,
      6U);
  auto const observerCutoff = timeAfter(
      *observerTime.factory,
      *observerTime.initial,
      *observerTime.epsilon,
      6U);
  requireTimeEquals(
      *observerCutoff,
      *cutoff,
      "HLAreportFederateLost cutoff members' request times");
  std::vector<std::uint8_t> const tagBytes{0x43U, 0x55U, 0x54U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = withStage("timestamped send failed", [&] {
    return lost.rtiAmbassador().sendInteraction(
        interaction,
        rti::ParameterHandleValueMap{},
        tag,
        *cutoff);
  });
  require(retraction.isValid(), "Federate-lost TSO cutoff send returned an invalid retraction handle");
  withStage("survivor time advance request failed", [&] {
    observer.rtiAmbassador().timeAdvanceRequest(*observerCutoff);
  });
  withStage("lost-member time advance request failed", [&] {
    lost.rtiAmbassador().timeAdvanceRequest(*cutoff);
  });
  waitFor(
      lost,
      [&] { return !lost.recorder().timeAdvanceGrants().empty(); },
      options,
      "lost-member Time Advance Grant before fault injection");
  auto const lostGrants = lost.recorder().timeAdvanceGrants();
  require(
      lostGrants.size() == 1U && lostGrants.front().encoded == encodeTime(*cutoff),
      "Lost-member time advance did not reach the timestamped cutoff");

  auto const membershipReflectionsBeforeLoss =
      matchingMembershipReflections();
  if (!options.connectionLossReadyMarker.empty()) {
    std::ofstream readyMarker(
        options.connectionLossReadyMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(readyMarker),
        "Could not write the connection-loss readiness marker");
    readyMarker << "ready\n";
    require(
        static_cast<bool>(readyMarker),
        "Could not flush the connection-loss readiness marker");
  }
  auto const lossDeadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (std::chrono::steady_clock::now() < lossDeadline &&
         (!lost.recorder().hasConnectionLost() ||
          !observer.recorder().interaction().present ||
          observer.recorder().timedInteractions().empty() ||
          observer.recorder().timeAdvanceGrants().empty())) {
    try {
      static_cast<void>(
          lost.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.05));
    } catch (...) {
      // The faulted member's socket error may be raised before ConnectionLost.
    }
    static_cast<void>(
        observer.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.05));
    if (!lost.recorder().hasConnectionLost() ||
        !observer.recorder().interaction().present ||
        observer.recorder().timedInteractions().empty() ||
        observer.recorder().timeAdvanceGrants().empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  require(
      lost.recorder().hasConnectionLost(),
      "Timed out waiting for the lost-member connectionLost callback");
  require(
      !lost.recorder().connectionLostDescription().empty(),
      "Lost-member connectionLost did not provide a fault description");
  require(
      observer.recorder().interaction().present,
      "Timed out waiting for the subscribed HLAreportFederateLost callback");
  if (!options.connectionLossMarker.empty()) {
    std::ofstream marker(
        options.connectionLossMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker),
        "Could not write the connection-loss completion marker");
    marker << "callback-ok\n";
    require(
        static_cast<bool>(marker),
        "Could not flush the connection-loss completion marker");
  }

  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass &&
          report.parameters.size() == 4U &&
          report.parameters.count(federateParameter) == 1U &&
          report.parameters.count(nameParameter) == 1U &&
          report.parameters.count(timeParameter) == 1U &&
          report.parameters.count(faultParameter) == 1U,
      "HLAreportFederateLost callback returned the wrong identity or parameter set");
  auto const reliable = withStage("HLAreliable lookup failed", [&] {
    return observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  });
  require(
      report.transportation == reliable,
      "HLAreportFederateLost callback did not use reliable transportation");
  auto const reportedFederate = withStage("federate-handle decoding failed", [&] {
    return observer.rtiAmbassador().decodeFederateHandle(
        report.parameters.at(federateParameter));
  });
  require(
      reportedFederate == lostFederate,
      "HLAreportFederateLost identified the wrong joined federate");
  rti::HLAunicodeString reportedName;
  withStage("federate-name decoding failed", [&] {
    reportedName.decode(report.parameters.at(nameParameter));
  });
  require(
      reportedName.get() == lostName,
      "HLAreportFederateLost returned the wrong federate name");
  auto timeFactory = withStage("logical-time factory creation failed", [&] {
    return rti::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
        options.logicalTimeImplementationName);
  });
  require(timeFactory != nullptr, "The adapter logical-time factory was unavailable");
  auto reportedTime = withStage("logical timestamp decoding failed", [&] {
    return timeFactory->decodeLogicalTime(report.parameters.at(timeParameter));
  });
  require(
      reportedTime != nullptr && !reportedTime->toString().empty(),
      "HLAreportFederateLost did not encode a standard logical timestamp");
  requireTimeEquals(
      *reportedTime,
      *cutoff,
      "HLAreportFederateLost HLAtimeStamp versus the lost member's cutoff");
  rti::HLAunicodeString faultDescription;
  withStage("fault-description decoding failed", [&] {
    faultDescription.decode(report.parameters.at(faultParameter));
  });

  auto const timedInteractions = observer.recorder().timedInteractions();
  auto const survivorGrants = observer.recorder().timeAdvanceGrants();
  require(
      timedInteractions.size() == 1U && survivorGrants.size() == 1U,
      "Federate loss lost or duplicated the cutoff TSO callback or survivor grant");
  auto const& timed = timedInteractions.front();
  require(
      timed.interaction == interaction &&
          timed.parameters.empty() &&
          timed.tag == tagBytes &&
          timed.producer == lostFederate &&
          timed.time == encodeTime(*cutoff) &&
          timed.sentOrder == rti::TIMESTAMP &&
          timed.receivedOrder == rti::TIMESTAMP &&
          timed.retractionPresent &&
          timed.retraction == copyBytes(retraction.encode()),
      "Federate-loss cutoff changed the queued timestamped interaction");
  require(
      survivorGrants.front().encoded == encodeTime(*cutoff),
      "Federate loss granted the survivor at the wrong cutoff time");
  require(
      timed.callbackSequence < survivorGrants.front().callbackSequence,
      "The survivor received its cutoff grant before the lost member's TSO message");

  waitFor(
      observer,
      [&] {
        return matchingMembershipReflections() >
            membershipReflectionsBeforeLoss;
      },
      options,
      "post-loss HLAfederatesInFederation conditional update");
  auto const membershipAfterLoss = latestMembership();
  require(
      membershipAfterLoss && membershipAfterLoss->size() == 1U &&
          membershipAfterLoss->front() == observerFederate,
      "Federation MOM membership update retained the automatically resigned federate");

  withStage("post-report membership query failed", [&] {
    observer.rtiAmbassador().listFederationExecutionMembers(federation);
  });
  waitFor(
      observer,
      [&] {
        return observer.recorder().listsMembers(
            federation, {observerName}, 1U);
      },
      options,
      "automatic resignation of the reported lost federate");

  auto const postLossSynchronizationLabel =
      federation + L"-post-loss-survivor-synchronization";
  std::vector<std::uint8_t> synchronizationTagBytes{0x53U, 0x59U, 0x4EU};
  rti::VariableLengthData const synchronizationTag(
      synchronizationTagBytes.data(), synchronizationTagBytes.size());
  withStage("survivor synchronization registration failed", [&] {
    observer.rtiAmbassador().registerFederationSynchronizationPoint(
        postLossSynchronizationLabel, synchronizationTag);
  });
  waitFor(
      observer,
      [&] {
        auto const registrations =
            observer.synchronizationPointRegistrations();
        auto const announcements =
            observer.synchronizationPointAnnouncements();
        return std::any_of(
                   registrations.begin(),
                   registrations.end(),
                   [&](SynchronizationRegistrationRecord const& record) {
                     return record.label == postLossSynchronizationLabel;
                   }) &&
            std::any_of(
                announcements.begin(),
                announcements.end(),
                [&](SynchronizationAnnouncementRecord const& record) {
                  return record.label == postLossSynchronizationLabel;
                });
      },
      options,
      "single-survivor synchronization registration and announcement");
  auto const registrations = observer.synchronizationPointRegistrations();
  auto const registration = std::find_if(
      registrations.begin(),
      registrations.end(),
      [&](SynchronizationRegistrationRecord const& record) {
        return record.label == postLossSynchronizationLabel;
      });
  require(
      registration != registrations.end() && registration->succeeded &&
          !registration->failureReason.has_value(),
      "The survivor's synchronization registration did not succeed");
  auto const announcements = observer.synchronizationPointAnnouncements();
  auto const announcement = std::find_if(
      announcements.begin(),
      announcements.end(),
      [&](SynchronizationAnnouncementRecord const& record) {
        return record.label == postLossSynchronizationLabel;
      });
  require(
      announcement != announcements.end() &&
          announcement->tag == synchronizationTagBytes,
      "The survivor's synchronization announcement changed its tag");
  withStage("survivor synchronization achievement failed", [&] {
    observer.rtiAmbassador().synchronizationPointAchieved(
        postLossSynchronizationLabel);
  });
  waitFor(
      observer,
      [&] {
        auto const completions = observer.federationSynchronized();
        return std::any_of(
            completions.begin(),
            completions.end(),
            [&](FederationSynchronizedRecord const& record) {
              return record.label == postLossSynchronizationLabel;
            });
      },
      options,
      "single-survivor Federation Synchronized callback");
  auto const completions = observer.federationSynchronized();
  auto const completion = std::find_if(
      completions.begin(),
      completions.end(),
      [&](FederationSynchronizedRecord const& record) {
        return record.label == postLossSynchronizationLabel;
      });
  require(
      completion != completions.end() && completion->failedToSyncSet.empty(),
      "The survivor's synchronization completed with an unexpected failed set");

  try {
    lost.disconnect();
  } catch (...) {
    // The connection-loss callback may follow an implementation's forced disconnect.
  }
  withStage("observer resignation failed", [&] {
    observer.resign(rti::NO_ACTION);
  });
  withStage("federation destruction failed", [&] {
    observer.rtiAmbassador().destroyFederationExecution(federation);
  });
  withStage("observer disconnect failed", [&] { observer.disconnect(); });
}

void scenarioFederateLostMomReportContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateLostMomReportPortable(options, model);
}

std::vector<std::pair<std::string, rti::CallbackModel>> callbackModels(Options const& options) {
  if (options.callbackModel == "evoked") {
    return {{"evoked", rti::HLA_EVOKED}};
  }
  if (options.callbackModel == "immediate") {
    return {{"immediate", rti::HLA_IMMEDIATE}};
  }
  return {{"evoked", rti::HLA_EVOKED}, {"immediate", rti::HLA_IMMEDIATE}};
}
}  // namespace hla_rti_cpp_tck::detail
