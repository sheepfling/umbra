#pragma once

#include "internal/federation/process_transport_session.hpp"

#include <stdexcept>

namespace umbra::test {

// A choreographed multi-session fixture must finish the reporting handshake on
// the failing session before switching to the next client's public request.
inline void serveProcessExceptionReport(
    detail::ProcessTransportSession& session,
    detail::ProcessTransportServiceDispatcher::Handler const& serviceHandler) {
  if (!detail::ProcessTransportServiceDispatcher::serveOne(
          session, [&](detail::TransportServiceMessage const& request) {
            if (request.operation !=
                detail::TransportServiceOperation::report_service_exception) {
              throw std::runtime_error(
                  "The public-service fixture expected an exception report.");
            }
            auto response = serviceHandler(request);
            if (response.status != detail::TransportServiceStatus::ok) {
              throw std::runtime_error(
                  "The public-service fixture rejected an exception report.");
            }
            return response;
          })) {
    throw std::runtime_error(
        "The public-service fixture lost an exception report.");
  }
}

// Public C++ services can report an exception through a separate, synchronous
// private request. Fixed-order service fixtures must acknowledge those reports
// without counting them as, or weakening assertions on, the next public service.
// The actual service handler still validates the report and applies its switch.
inline bool servePrimaryProcessRequest(
    detail::ProcessTransportSession& session,
    detail::ProcessTransportServiceDispatcher::Handler const& serviceHandler,
    detail::ProcessTransportServiceDispatcher::Handler const& primaryHandler) {
  for (;;) {
    bool primaryServed = false;
    if (!detail::ProcessTransportServiceDispatcher::serveOne(
            session, [&](detail::TransportServiceMessage const& request) {
              if (request.operation ==
                  detail::TransportServiceOperation::report_service_exception) {
                auto response = serviceHandler(request);
                if (response.status != detail::TransportServiceStatus::ok) {
                  throw std::runtime_error(
                      "The public-service fixture rejected an exception report.");
                }
                return response;
              }
              primaryServed = true;
              return primaryHandler(request);
            })) {
      return false;
    }
    if (primaryServed) {
      return true;
    }
  }
}

inline bool servePrimaryProcessRequest(
    detail::ProcessTransportSession& session,
    detail::ProcessTransportServiceDispatcher::Handler const& serviceHandler) {
  return servePrimaryProcessRequest(session, serviceHandler, serviceHandler);
}

}  // namespace umbra::test
