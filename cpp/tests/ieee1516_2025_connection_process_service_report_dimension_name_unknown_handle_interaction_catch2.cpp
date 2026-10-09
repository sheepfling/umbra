#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <array>
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
#include <stdexcept>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/auth/HLAnoCredentials.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <umbra/embedded_profile_configuration.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "ieee1516_2025_connection_process_test_support.hpp"


#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "tests/ieee1516_2025_connection_process_service_report_interaction_helpers.hpp"
TEST_CASE(
    "RTIambassador delivers failed process dimension-name reports through the MOM interaction",
    "[integration][foundation][federation-management][mom][service-report-interaction]"
    "[service-reporting][service-failure][support-services][data-distribution-management]"
    "[ddm][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-dimension-name-failure-interaction]"
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

#endif
#endif
