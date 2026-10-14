#pragma once

#include "internal/federation/process_transport_session.hpp"

#include <stdexcept>

namespace umbra::test {

template <typename ServeExpected, typename Session, typename Handler>
inline void serveExpectedProcessServiceReportPair(
    ServeExpected& serveExpected,
    Session& session,
    Handler& serviceHandler,
    detail::TransportServiceOperation serviceOperation,
    detail::TransportServiceOperation reportOperation,
    char const* description) {
  serveExpected(session, serviceHandler, serviceOperation, description);
  serveExpected(session, serviceHandler, reportOperation, description);
}

template <typename ServeExpected, typename Session, typename Handler>
inline void serveExpectedProcessSuccessfulVoidService(
    ServeExpected& serveExpected,
    Session& session,
    Handler& serviceHandler,
    detail::TransportServiceOperation serviceOperation,
    char const* description) {
  serveExpectedProcessServiceReportPair(
      serveExpected,
      session,
      serviceHandler,
      serviceOperation,
      detail::TransportServiceOperation::report_successful_void_service_invocation,
      description);
}

template <typename ServeExpected, typename Session, typename Handler>
inline void serveExpectedProcessSuccessfulVoidService(
    ServeExpected& serveExpected,
    Session& session,
    Handler& serviceHandler,
    detail::TransportServiceOperation serviceOperation) {
  serveExpected(session, serviceHandler, serviceOperation);
  serveExpected(
      session,
      serviceHandler,
      detail::TransportServiceOperation::report_successful_void_service_invocation);
}

template <typename ServeExpected, typename Session, typename Handler>
inline void serveExpectedProcessSuccessfulService(
    ServeExpected& serveExpected,
    Session& session,
    Handler& serviceHandler,
    detail::TransportServiceOperation serviceOperation,
    char const* description) {
  serveExpectedProcessServiceReportPair(
      serveExpected,
      session,
      serviceHandler,
      serviceOperation,
      detail::TransportServiceOperation::report_successful_service_invocation,
      description);
}

// Fixed-order multi-federate fixtures must acknowledge synchronous report
// sidebands on the owning session before advancing to another operation.
template <typename Session, typename Handler, typename ExpectedResponseHandler>
inline void serveExpectedPrimaryProcessRequest(
    Session& session,
    Handler const& serviceHandler,
    detail::TransportServiceOperation expectedOperation,
    char const* description,
    ExpectedResponseHandler&& expectedResponseHandler) {
  for (;;) {
    bool expectedServed = false;
    if (!detail::ProcessTransportServiceDispatcher::serveOne(
            session, [&](detail::TransportServiceMessage const& request) {
              bool const isServiceReport =
                  request.operation ==
                      detail::TransportServiceOperation::report_service_exception ||
                  request.operation ==
                      detail::TransportServiceOperation::report_failed_service_invocation ||
                  request.operation ==
                      detail::TransportServiceOperation::report_successful_service_invocation ||
                  request.operation ==
                      detail::TransportServiceOperation::report_successful_void_service_invocation ||
                  request.operation ==
                      detail::TransportServiceOperation::recheck_exception_report;
              if (isServiceReport && request.operation != expectedOperation) {
                auto response = serviceHandler(request);
                if (response.status != detail::TransportServiceStatus::ok) {
                  throw std::runtime_error(
                      "The process-service fixture rejected a report sideband.");
                }
                return response;
              }
              if (request.operation != expectedOperation) {
                throw std::runtime_error(description);
              }
              auto response = serviceHandler(request);
              if (response.status != detail::TransportServiceStatus::ok) {
                throw std::runtime_error(description);
              }
              expectedResponseHandler(response);
              expectedServed = true;
              return response;
            })) {
      throw std::runtime_error(description);
    }
    if (expectedServed) {
      return;
    }
  }
}

template <typename Session, typename Handler>
inline void serveExpectedPrimaryProcessRequest(
    Session& session,
    Handler const& serviceHandler,
    detail::TransportServiceOperation expectedOperation,
    char const* description) {
  serveExpectedPrimaryProcessRequest(
      session,
      serviceHandler,
      expectedOperation,
      description,
      [](detail::TransportServiceMessage const&) {});
}

}  // namespace umbra::test
