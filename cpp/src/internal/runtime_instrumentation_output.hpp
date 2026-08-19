#pragma once

#include "internal/runtime_instrumentation.hpp"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <ostream>
#include <thread>

namespace umbra::detail {

enum class RuntimeInstrumentationOutputFormat {
  console,
  json,
  csv,
};

[[nodiscard]] std::string_view toString(InstrumentationLayer layer) noexcept;

using RuntimeInstrumentationSnapshotProvider =
    std::function<RuntimeInstrumentationSnapshot()>;

void writeRuntimeInstrumentationConsole(
    std::ostream& output,
    RuntimeInstrumentationSnapshot const& snapshot);

void writeRuntimeInstrumentationJson(
    std::ostream& output,
    RuntimeInstrumentationSnapshot const& snapshot);

void writeRuntimeInstrumentationCsv(
    std::ostream& output,
    RuntimeInstrumentationSnapshot const& snapshot);

void writeRuntimeInstrumentation(
    std::ostream& output,
    RuntimeInstrumentationSnapshot const& snapshot,
    RuntimeInstrumentationOutputFormat format);

// Samples a runtime from a worker thread. Snapshotting and all output I/O are
// intentionally outside RTI service, registry, and callback execution paths.
class RuntimeInstrumentationCsvSampler final {
 public:
  RuntimeInstrumentationCsvSampler(
      std::shared_ptr<RuntimeInstrumentation> instrumentation,
      std::filesystem::path outputPath,
      std::chrono::milliseconds interval = std::chrono::seconds(1));

  RuntimeInstrumentationCsvSampler(
      RuntimeInstrumentationSnapshotProvider snapshotProvider,
      std::filesystem::path outputPath,
      std::chrono::milliseconds interval = std::chrono::seconds(1));

  RuntimeInstrumentationCsvSampler(
      RuntimeInstrumentationCsvSampler const&) = delete;
  RuntimeInstrumentationCsvSampler& operator=(
      RuntimeInstrumentationCsvSampler const&) = delete;

  ~RuntimeInstrumentationCsvSampler();

  // Opens/truncates the destination and starts sampling. Calling start while
  // already running is harmless.
  void start();

  // Stops sampling, flushes completed rows, and closes the file.
  void stop() noexcept;

  [[nodiscard]] bool running() const noexcept;

 private:
  void run() noexcept;

  std::shared_ptr<RuntimeInstrumentation> instrumentation_;
  RuntimeInstrumentationSnapshotProvider snapshotProvider_;
  std::filesystem::path outputPath_;
  std::chrono::milliseconds interval_;
  mutable std::mutex mutex_;
  std::condition_variable wakeup_;
  std::ofstream output_;
  std::thread worker_;
  bool stopRequested_ = false;
  bool running_ = false;
};

}  // namespace umbra::detail
