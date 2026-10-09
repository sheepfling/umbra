#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/auth/HLAnoCredentials.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <umbra/embedded_profile_configuration.hpp>

#include "ieee1516_2025_connection_process_test_support.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "tests/ieee1516_2025_connection_process_service_report_interaction_helpers.hpp"

TEST_CASE(
    "RTIambassador delivers failed process dimension-upper-bound reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-upper-bound-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-upper-bound]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionUpperBound,
      ProcessFailedServiceReportHandleState::BindingInvalid);
}

TEST_CASE(
    "RTIambassador delivers failed process dimension-upper-bound unknown-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-upper-bound-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-upper-bound]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionUpperBound,
      ProcessFailedServiceReportHandleState::FederationUnknown);
}

TEST_CASE(
    "RTIambassador delivers failed process dimension-name unknown-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-name-unknown-handle-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionName,
      ProcessFailedServiceReportHandleState::FederationUnknown);
}

TEST_CASE(
    "RTIambassador delivers failed process GetAvailableDimensionsForObjectClass reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-dimensions-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForObjectClass,
      ProcessFailedServiceReportHandleState::BindingInvalid);
}

TEST_CASE(
    "RTIambassador delivers failed process GetAvailableDimensionsForInteractionClass reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-interaction-dimensions-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForInteractionClass,
      ProcessFailedServiceReportHandleState::BindingInvalid);
}

TEST_CASE(
    "RTIambassador delivers failed process GetAvailableDimensionsForInteractionClass unknown-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-interaction-dimensions-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForInteractionClass,
      ProcessFailedServiceReportHandleState::FederationUnknown);
}

TEST_CASE(
    "RTIambassador delivers failed process GetAvailableDimensionsForObjectClass unknown-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-dimensions-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForObjectClass,
      ProcessFailedServiceReportHandleState::FederationUnknown);
}

TEST_CASE(
    "RTIambassador delivers failed process GetObjectClassHandle unknown-name reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-object-class-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-object-class-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetObjectClassHandleNameNotFound,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador delivers failed process GetObjectClassName invalid-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-object-class-name-invalid-handle-interaction]"
    "[process-service-report-object-class-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-object-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetObjectClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::FederationUnknown);
}

TEST_CASE(
    "RTIambassador delivers failed process GetObjectClassName malformed-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-object-class-name-malformed-handle-interaction]"
    "[process-service-report-object-class-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-object-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetObjectClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::BindingInvalid);
}

TEST_CASE(
    "RTIambassador delivers failed process GetInteractionClassName invalid-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-interaction-class-name-invalid-handle-interaction]"
    "[process-service-report-interaction-class-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetInteractionClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::FederationUnknown);
}

TEST_CASE(
    "RTIambassador delivers failed process GetInteractionClassName malformed-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-interaction-class-name-malformed-handle-interaction]"
    "[process-service-report-interaction-class-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetInteractionClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::BindingInvalid);
}

TEST_CASE(
    "RTIambassador delivers failed process GetInteractionClassHandle unknown-name reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-interaction-class-handle-name-not-found-interaction]"
    "[process-service-report-interaction-class-name-lookup-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle][rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetInteractionClassHandleNameNotFound,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador delivers failed process GetTransportationTypeHandle invalid-name reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-transportation-type-handle-invalid-name-interaction]"
    "[process-service-report-transportation-type-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeHandleInvalidName,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador delivers failed process GetTransportationTypeName federation-unknown-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-transportation-type-name-federation-unknown-handle-interaction]"
    "[process-service-report-transportation-type-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-name]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeNameInvalidHandle,
      ProcessFailedServiceReportHandleState::FederationUnknown);
}

TEST_CASE(
    "RTIambassador delivers failed process GetTransportationTypeName malformed-handle reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][object-management]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-transportation-type-name-malformed-handle-interaction]"
    "[process-service-report-transportation-type-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-name]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeNameInvalidHandle,
      ProcessFailedServiceReportHandleState::BindingInvalid);
}

TEST_CASE(
    "RTIambassador delivers failed process GetOrderName invalid-enum reports through the MOM interaction",
    "[integration][foundation][time-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-order-name-invalid-enum-interaction]"
    "[process-service-report-order-name-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-order-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetOrderNameInvalidType,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetOrderName invalid enum",
    "[integration][development-profile][foundation][time-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-order-name-invalid-enum-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-order-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetOrderNameInvalidType,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetObjectClassName",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][object-management][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-object-class-name-invalid-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-object-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetObjectClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetObjectClassName with malformed handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][object-management][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-object-class-name-malformed-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-object-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetObjectClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::BindingInvalid,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetInteractionClassName",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][object-management][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-interaction-class-name-invalid-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetInteractionClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetInteractionClassName with malformed handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][object-management][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-interaction-class-name-malformed-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetInteractionClassNameInvalidHandle,
      ProcessFailedServiceReportHandleState::BindingInvalid,
      true);
}

TEST_CASE(
    "RTIambassador delivers successful process SendInteraction reports through the MOM interaction with file reporting disabled",
    "[integration][development-profile][foundation][interaction-management][mom]"
    "[service-reporting][service-report-interaction][service-success][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-success-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction]"
    "[rti.service.subscribe-interaction-class][rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][federate.callback.receive-interaction]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::SendInteractionSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador delivers successful process GetOrderName reports through the MOM interaction with file reporting disabled",
    "[integration][development-profile][foundation][time-management][mom]"
    "[service-reporting][service-report-interaction][service-success][support-services]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-order-name-success-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-order-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetOrderNameSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador delivers successful process GetOrderType reports through the MOM interaction with file reporting disabled",
    "[integration][development-profile][foundation][time-management][mom]"
    "[service-reporting][service-report-interaction][service-success][support-services]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-order-type-success-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.get-order-type]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetOrderTypeSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful SendInteraction",
    "[integration][development-profile][foundation][interaction-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch][rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::SendInteractionSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador delivers successful process SendInteractionWithRegions reports through the MOM interaction with file reporting disabled",
    "[integration][development-profile][foundation][interaction-management][data-distribution-management][mom]"
    "[service-reporting][service-report-interaction][service-success][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-with-regions-success-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch][rti.service.set-service-reporting-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-dimension-handle][rti.service.create-region]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction-with-regions]"
    "[rti.service.subscribe-interaction-class][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::SendInteractionWithRegionsSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful SendInteractionWithRegions",
    "[integration][development-profile][foundation][interaction-management][data-distribution-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction][service-success][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-with-regions-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch][rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-dimension-handle][rti.service.create-region]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction-with-regions]"
    "[rti.service.subscribe-interaction-class][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::SendInteractionWithRegionsSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador delivers failed process SendInteractionWithRegions reports through the MOM interaction with file reporting disabled",
    "[integration][development-profile][foundation][interaction-management][data-distribution-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-with-regions-invalid-region-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch][rti.service.set-service-reporting-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction-with-regions]"
    "[rti.service.get-transportation-type-handle][rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::
          SendInteractionWithRegionsInvalidRegion,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador records invalid RegionHandle failures from process SendInteractionWithRegions in the selected service-report file",
    "[integration][development-profile][foundation][interaction-management][data-distribution-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction][service-failure][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-with-regions-invalid-region-failure-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch][rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction-with-regions]"
    "[rti.service.get-transportation-type-handle][rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::
          SendInteractionWithRegionsInvalidRegion,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador records InteractionParameterNotDefined failures from process SendInteractionWithRegions in the selected service-report file",
    "[integration][development-profile][foundation][interaction-management][data-distribution-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction][service-failure][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-with-regions-invalid-parameter-failure-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch][rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-dimension-handle][rti.service.create-region]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction-with-regions]"
    "[rti.service.get-transportation-type-handle][rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::
          SendInteractionWithRegionsInvalidParameter,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador delivers InteractionParameterNotDefined failures from process SendInteractionWithRegions through the MOM interaction with file reporting disabled",
    "[integration][development-profile][foundation][interaction-management][data-distribution-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-send-interaction-with-regions-invalid-parameter-failure-interaction]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch][rti.service.set-service-reporting-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-dimension-handle][rti.service.create-region]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.publish-interaction-class][rti.service.send-interaction-with-regions]"
    "[rti.service.get-transportation-type-handle][rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::
          SendInteractionWithRegionsInvalidParameter,
      ProcessFailedServiceReportHandleState::NotApplicable);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM when file reporting is enabled for GetOrderName",
    "[integration][development-profile][foundation][time-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-order-name-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-order-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetOrderNameSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM when file reporting is enabled for GetOrderType",
    "[integration][development-profile][foundation][time-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-order-type-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-order-type]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetOrderTypeSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetTransportationTypeName",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-transportation-type-name-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeNameInvalidHandle,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetTransportationTypeName with malformed handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-transportation-type-name-malformed-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeNameInvalidHandle,
      ProcessFailedServiceReportHandleState::BindingInvalid,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetTransportationTypeHandle invalid name",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-transportation-type-handle-invalid-name-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeHandleInvalidName,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetInteractionClassHandle unknown name",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][object-management][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-interaction-class-handle-name-not-found-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetInteractionClassHandleNameNotFound,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetTransportationTypeName with federation-unknown handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-transportation-type-name-federation-unknown-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeNameInvalidHandle,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful GetTransportationTypeName",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][transport][process-boundary][public-endpoint]"
    "[2025][process-service-report-transportation-type-name-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-transportation-type-name]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetTransportationTypeNameSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetDimensionUpperBound with binding-invalid handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-upper-bound-invalid-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-upper-bound]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionUpperBound,
      ProcessFailedServiceReportHandleState::BindingInvalid,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetDimensionUpperBound with federation-unknown handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-upper-bound-unknown-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-upper-bound]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionUpperBound,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetDimensionName with binding-invalid handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-name-binding-invalid-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionName,
      ProcessFailedServiceReportHandleState::BindingInvalid,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetAvailableDimensionsForObjectClass with binding-invalid handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-dimensions-invalid-object-class-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForObjectClass,
      ProcessFailedServiceReportHandleState::BindingInvalid,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetAvailableDimensionsForInteractionClass with binding-invalid handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-interaction-dimensions-invalid-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForInteractionClass,
      ProcessFailedServiceReportHandleState::BindingInvalid,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetAvailableDimensionsForInteractionClass with federation-unknown handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-interaction-dimensions-unknown-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForInteractionClass,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetAvailableDimensionsForObjectClass with federation-unknown handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-dimensions-unknown-object-class-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForObjectClass,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful GetAvailableDimensionsForObjectClass",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-dimensions-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-object-class-handle][rti.service.get-dimension-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForObjectClass,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful GetAvailableDimensionsForInteractionClass",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-available-interaction-dimensions-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-interaction-class-handle][rti.service.get-dimension-handle]"
    "[rti.service.get-parameter-handle][rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetAvailableDimensionsForInteractionClass,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful GetDimensionName",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-name-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-name][rti.service.get-dimension-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionName,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful GetObjectClassName",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][object-management][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-object-class-name-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-object-class-name][rti.service.get-object-class-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetObjectClassNameSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful GetInteractionClassName",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][object-management][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[process-service-report-interaction-class-name-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-interaction-class-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetInteractionClassNameSuccess,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for failed GetDimensionName with federation-unknown handle",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-failure][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-name-unknown-handle-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-name]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionName,
      ProcessFailedServiceReportHandleState::FederationUnknown,
      true);
}

TEST_CASE(
    "RTIambassador selects the process service-report file instead of MOM for successful GetDimensionUpperBound",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[service-reporting][service-report-file][service-report-interaction]"
    "[service-success][support-services][data-distribution-management][ddm]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-upper-bound-success-file-destination-selection]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-upper-bound][rti.service.get-dimension-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback][rti.service.resign-federation-execution]"
    "[rti.service.disconnect][federate.callback.receive-interaction]") {
  runProcessServiceReportInteraction(
      ProcessServiceReportInteractionOperation::GetDimensionUpperBound,
      ProcessFailedServiceReportHandleState::NotApplicable,
      true);
}
#endif
