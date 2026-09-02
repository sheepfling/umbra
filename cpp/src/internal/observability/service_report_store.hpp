#pragma once

#include "internal/observability/runtime_instrumentation.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace umbra::detail {

struct JoinedFederateReportDescriptor {
  std::wstring federationName;
  std::wstring federateName;
  std::uint64_t federateId = 0;
  std::uint64_t joinIdentifier = 0;
  std::wstring initialRecord;
};

class ServiceReportWriter {
 public:
  virtual ~ServiceReportWriter() = default;
  [[nodiscard]] virtual std::filesystem::path location() const = 0;
  virtual void append(std::wstring_view encodedRecord) = 0;
};

class ServiceReportStore {
 public:
  virtual ~ServiceReportStore() = default;
  [[nodiscard]] virtual std::unique_ptr<ServiceReportWriter> createForJoinedFederate(
      JoinedFederateReportDescriptor const& descriptor) = 0;
};

// Embedded-profile production store. The directory is an RTI configuration
// decision; a failure to create/write a report is surfaced to the caller and
// never downgraded to an in-memory destination.
class FilesystemServiceReportStore final : public ServiceReportStore {
 public:
  explicit FilesystemServiceReportStore(
      std::filesystem::path directory,
      std::shared_ptr<RuntimeInstrumentation> instrumentation = {});
  [[nodiscard]] std::unique_ptr<ServiceReportWriter> createForJoinedFederate(
      JoinedFederateReportDescriptor const& descriptor) override;

 private:
  std::filesystem::path directory_;
  std::shared_ptr<RuntimeInstrumentation> instrumentation_;
};

// Test-only store. It is deliberately not selected from production
// configuration and has no filesystem-like advertised URI.
class MemoryServiceReportStore final : public ServiceReportStore {
 public:
  explicit MemoryServiceReportStore(
      std::shared_ptr<RuntimeInstrumentation> instrumentation = {});

  [[nodiscard]] std::unique_ptr<ServiceReportWriter> createForJoinedFederate(
      JoinedFederateReportDescriptor const& descriptor) override;
  [[nodiscard]] std::vector<std::wstring> const& records() const noexcept;
  // Returns a synchronized copy for tests that intentionally exercise the
  // store from more than one callback/thread. The reference-returning
  // records() accessor remains for quiescent, single-threaded assertions.
  [[nodiscard]] std::vector<std::wstring> snapshotRecords() const;

 private:
  std::vector<std::wstring> records_;
  mutable std::mutex recordsMutex_;
  std::shared_ptr<RuntimeInstrumentation> instrumentation_;
};

}  // namespace umbra::detail
