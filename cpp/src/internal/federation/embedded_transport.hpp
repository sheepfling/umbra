#pragma once

#include "internal/observability/runtime_instrumentation.hpp"

#include <RTI/RTIambassador.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace umbra::detail {

// Private transport contract used by the runtime adapter. Implementations own
// connection state and report a transport fault exactly once. The embedded
// loopback endpoint below is the development-profile implementation; a future
// socket/IPC endpoint can implement this contract without changing the public
// IEEE binding adapter.
class TransportConnection {
 public:
  using FailureHandler = std::function<void(std::wstring)>;
  using ForcedResignationHandler = std::function<bool(std::wstring)>;

  virtual ~TransportConnection() = default;

  virtual void close() noexcept = 0;
  virtual void fail(std::wstring faultDescription) = 0;
  [[nodiscard]] virtual bool forceFederateResignation(
      std::wstring reasonForResign) = 0;
  [[nodiscard]] virtual bool open() const noexcept = 0;
  [[nodiscard]] virtual void* owner() const noexcept = 0;
};

class EmbeddedTransportConnection final : public TransportConnection {
 public:
  using FailureHandler = TransportConnection::FailureHandler;
  using ForcedResignationHandler = TransportConnection::ForcedResignationHandler;

  EmbeddedTransportConnection(
      void* owner,
      FailureHandler failureHandler,
      ForcedResignationHandler forcedResignationHandler,
      std::shared_ptr<RuntimeInstrumentation> instrumentation = {});

  EmbeddedTransportConnection(EmbeddedTransportConnection const&) = delete;
  EmbeddedTransportConnection& operator=(EmbeddedTransportConnection const&) = delete;

  ~EmbeddedTransportConnection() override;

  // Graceful endpoint shutdown suppresses the fault callback. It is used by
  // the normal Disconnect path and while the owning ambassador is destroyed.
  void close() noexcept override;

  // Closes the endpoint as a transport fault and invokes the handler outside
  // the endpoint mutex. Repeated faults are ignored.
  void fail(std::wstring faultDescription) override;

  // Requests a private RTI-side membership revocation while the endpoint stays
  // connected. The handler may reject it when the owner is no longer joined.
  // This is a control-plane seam for future session/admin logic; it is not a
  // federate-invoked operation or a public Umbra API.
  [[nodiscard]] bool forceFederateResignation(
      std::wstring reasonForResign) override;

  [[nodiscard]] bool open() const noexcept override;
  [[nodiscard]] void* owner() const noexcept override;

 private:
  mutable std::mutex mutex_;
  void* owner_ = nullptr;
  FailureHandler failureHandler_;
  ForcedResignationHandler forcedResignationHandler_;
  std::shared_ptr<RuntimeInstrumentation> instrumentation_;
  bool open_ = true;
};

// Process-local endpoint directory used by the embedded backend. It is a
// transport seam, not an Umbra public API. The testing fault hook below lets
// Catch2 drive the same endpoint failure path that a future socket/IPC
// backend will use when a connection actually breaks.
class EmbeddedTransportHub final {
 public:
  EmbeddedTransportHub() = default;

  std::shared_ptr<TransportConnection> connect(
      void* owner,
      EmbeddedTransportConnection::FailureHandler failureHandler,
      EmbeddedTransportConnection::ForcedResignationHandler forcedResignationHandler,
      std::shared_ptr<RuntimeInstrumentation> instrumentation = {});

  void disconnect(void* owner) noexcept;

  bool fail(void* owner, std::wstring faultDescription);

  bool forceFederateResignation(void* owner, std::wstring reasonForResign);

 private:
  mutable std::mutex mutex_;
  std::unordered_map<void*, std::weak_ptr<TransportConnection>> connections_;
};

EmbeddedTransportHub& embeddedTransportHub() noexcept;

// Deliberately internal: this is a deterministic transport fault source for
// integration tests, not a second public RTI operation.
bool failEmbeddedTransportConnectionForTesting(
    rti1516_2025::RTIambassador& owner,
    std::wstring faultDescription);

// Deliberately internal: a deterministic RTI-side membership-control source
// for integration tests. It leaves a valid connection open and will later be
// replaced by real session watchdog or administration control input.
bool forceEmbeddedFederateResignationForTesting(
    rti1516_2025::RTIambassador& owner,
    std::wstring reasonForResign);

}  // namespace umbra::detail
