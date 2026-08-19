#include "internal/runtime_instrumentation_output.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace umbra::detail {
namespace {

using OperationKey = std::pair<InstrumentationLayer, std::string>;

struct PreviousOperation final {
  std::uint64_t calls = 0;
  std::uint64_t successes = 0;
  std::uint64_t failures = 0;
  std::uint64_t exceptions = 0;
  std::uint64_t totalDurationNanoseconds = 0;
};

std::uint64_t delta(std::uint64_t current, std::uint64_t previous) noexcept {
  return current >= previous ? current - previous : current;
}

void writeJsonString(std::ostream& output, std::string_view value) {
  static constexpr std::array<char, 16> digits{
      '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  output.put('"');
  for (unsigned char character : value) {
    switch (character) {
      case '"':
        output << "\\\"";
        break;
      case '\\':
        output << "\\\\";
        break;
      case '\b':
        output << "\\b";
        break;
      case '\f':
        output << "\\f";
        break;
      case '\n':
        output << "\\n";
        break;
      case '\r':
        output << "\\r";
        break;
      case '\t':
        output << "\\t";
        break;
      default:
        if (character < 0x20) {
          output << "\\u00" << digits[character >> 4] << digits[character & 0x0F];
        } else {
          output.put(static_cast<char>(character));
        }
        break;
    }
  }
  output.put('"');
}

void writeCsvField(std::ostream& output, std::string_view value) {
  bool const needsQuotes = value.find_first_of(",\"\r\n") != std::string_view::npos;
  if (!needsQuotes) {
    output << value;
    return;
  }
  output.put('"');
  for (char character : value) {
    if (character == '"') {
      output.put('"');
    }
    output.put(character);
  }
  output.put('"');
}

void writeSnapshotCsvHeader(std::ostream& output) {
  output << "layer,operation,calls,successes,failures,exceptions,"
            "total_duration_ns,min_duration_ns,max_duration_ns,"
            "active_calls,peak_active_calls\n";
}

void writeSampleCsvHeader(std::ostream& output) {
  output << "sample_index,elapsed_ms,wall_time_unix_ms,layer,operation,"
            "calls,delta_calls,successes,delta_successes,failures,"
            "delta_failures,exceptions,delta_exceptions,total_duration_ns,"
            "delta_duration_ns,min_duration_ns,max_duration_ns,active_calls,"
            "peak_active_calls\n";
}

void writeSnapshotCsvRow(
    std::ostream& output,
    InstrumentationOperationSnapshot const& operation) {
  writeCsvField(output, toString(operation.layer));
  output.put(',');
  writeCsvField(output, operation.name);
  output << ',' << operation.calls
         << ',' << operation.successes
         << ',' << operation.failures
         << ',' << operation.exceptions
         << ',' << operation.totalDurationNanoseconds
         << ',' << operation.minimumDurationNanoseconds
         << ',' << operation.maximumDurationNanoseconds
         << ',' << operation.activeCalls
         << ',' << operation.peakActiveCalls
         << '\n';
}

void writeSampleCsvRows(
    std::ostream& output,
    RuntimeInstrumentationSnapshot const& snapshot,
    std::map<OperationKey, PreviousOperation> const& previous,
    std::uint64_t sampleIndex,
    std::uint64_t elapsedMilliseconds,
    std::uint64_t wallTimeUnixMilliseconds) {
  for (auto const& operation : snapshot.operations) {
    auto const key = std::make_pair(operation.layer, operation.name);
    auto const found = previous.find(key);
    PreviousOperation const prior = found == previous.end()
        ? PreviousOperation{}
        : found->second;

    output << sampleIndex << ','
           << elapsedMilliseconds << ','
           << wallTimeUnixMilliseconds << ',';
    writeCsvField(output, toString(operation.layer));
    output.put(',');
    writeCsvField(output, operation.name);
    output << ',' << operation.calls
           << ',' << delta(operation.calls, prior.calls)
           << ',' << operation.successes
           << ',' << delta(operation.successes, prior.successes)
           << ',' << operation.failures
           << ',' << delta(operation.failures, prior.failures)
           << ',' << operation.exceptions
           << ',' << delta(operation.exceptions, prior.exceptions)
           << ',' << operation.totalDurationNanoseconds
           << ',' << delta(operation.totalDurationNanoseconds, prior.totalDurationNanoseconds)
           << ',' << operation.minimumDurationNanoseconds
           << ',' << operation.maximumDurationNanoseconds
           << ',' << operation.activeCalls
           << ',' << operation.peakActiveCalls
           << '\n';
  }
}

std::map<OperationKey, PreviousOperation> previousOperations(
    RuntimeInstrumentationSnapshot const& snapshot) {
  std::map<OperationKey, PreviousOperation> result;
  for (auto const& operation : snapshot.operations) {
    result.emplace(
        std::make_pair(operation.layer, operation.name),
        PreviousOperation{
            operation.calls,
            operation.successes,
            operation.failures,
            operation.exceptions,
            operation.totalDurationNanoseconds});
  }
  return result;
}

std::uint64_t unixMilliseconds() noexcept {
  auto const now = std::chrono::system_clock::now().time_since_epoch();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

}  // namespace

std::string_view toString(InstrumentationLayer layer) noexcept {
  switch (layer) {
    case InstrumentationLayer::rti_ambassador:
      return "rti_ambassador";
    case InstrumentationLayer::federation_registry:
      return "federation_registry";
    case InstrumentationLayer::callback_dispatch:
      return "callback_dispatch";
    case InstrumentationLayer::federate_ambassador:
      return "federate_ambassador";
    case InstrumentationLayer::transport:
      return "transport";
    case InstrumentationLayer::service_reporting:
      return "service_reporting";
  }
  return "unknown";
}

void writeRuntimeInstrumentationConsole(
    std::ostream& output,
    RuntimeInstrumentationSnapshot const& snapshot) {
  output << "Runtime instrumentation (next correlation id: "
         << snapshot.nextCorrelationId << ")\n";
  output << std::left
         << std::setw(22) << "layer"
         << std::setw(42) << "operation"
         << std::right
         << std::setw(10) << "calls"
         << std::setw(10) << "success"
         << std::setw(10) << "failure"
         << std::setw(10) << "except"
         << std::setw(10) << "active"
         << std::setw(10) << "peak"
         << std::setw(12) << "avg_ms"
         << std::setw(12) << "min_ms"
         << std::setw(12) << "max_ms"
         << '\n';
  output << std::string(138, '-') << '\n';

  auto const oldFlags = output.flags();
  auto const oldPrecision = output.precision();
  output << std::fixed << std::setprecision(3);
  for (auto const& operation : snapshot.operations) {
    double const averageMilliseconds = operation.calls == 0
        ? 0.0
        : static_cast<double>(operation.totalDurationNanoseconds) /
            static_cast<double>(operation.calls) / 1'000'000.0;
    output << std::left
           << std::setw(22) << toString(operation.layer)
           << std::setw(42) << operation.name
           << std::right
           << std::setw(10) << operation.calls
           << std::setw(10) << operation.successes
           << std::setw(10) << operation.failures
           << std::setw(10) << operation.exceptions
           << std::setw(10) << operation.activeCalls
           << std::setw(10) << operation.peakActiveCalls
           << std::setw(12) << averageMilliseconds
           << std::setw(12) << static_cast<double>(operation.minimumDurationNanoseconds) / 1'000'000.0
           << std::setw(12) << static_cast<double>(operation.maximumDurationNanoseconds) / 1'000'000.0
           << '\n';
  }
  output.flags(oldFlags);
  output.precision(oldPrecision);
}

void writeRuntimeInstrumentationJson(
    std::ostream& output,
    RuntimeInstrumentationSnapshot const& snapshot) {
  output << "{\n  \"next_correlation_id\": "
         << snapshot.nextCorrelationId << ",\n  \"operations\": [";
  if (!snapshot.operations.empty()) {
    output << '\n';
  }
  for (std::size_t index = 0; index < snapshot.operations.size(); ++index) {
    auto const& operation = snapshot.operations[index];
    output << "    {\n      \"layer\": ";
    writeJsonString(output, toString(operation.layer));
    output << ",\n      \"operation\": ";
    writeJsonString(output, operation.name);
    output << ",\n      \"calls\": " << operation.calls
           << ",\n      \"successes\": " << operation.successes
           << ",\n      \"failures\": " << operation.failures
           << ",\n      \"exceptions\": " << operation.exceptions
           << ",\n      \"total_duration_ns\": " << operation.totalDurationNanoseconds
           << ",\n      \"min_duration_ns\": " << operation.minimumDurationNanoseconds
           << ",\n      \"max_duration_ns\": " << operation.maximumDurationNanoseconds
           << ",\n      \"active_calls\": " << operation.activeCalls
           << ",\n      \"peak_active_calls\": " << operation.peakActiveCalls
           << "\n    }";
    if (index + 1 != snapshot.operations.size()) {
      output.put(',');
    }
    output.put('\n');
  }
  output << "  ]\n}\n";
}

void writeRuntimeInstrumentationCsv(
    std::ostream& output,
    RuntimeInstrumentationSnapshot const& snapshot) {
  writeSnapshotCsvHeader(output);
  for (auto const& operation : snapshot.operations) {
    writeSnapshotCsvRow(output, operation);
  }
}

void writeRuntimeInstrumentation(
    std::ostream& output,
    RuntimeInstrumentationSnapshot const& snapshot,
    RuntimeInstrumentationOutputFormat format) {
  switch (format) {
    case RuntimeInstrumentationOutputFormat::console:
      writeRuntimeInstrumentationConsole(output, snapshot);
      return;
    case RuntimeInstrumentationOutputFormat::json:
      writeRuntimeInstrumentationJson(output, snapshot);
      return;
    case RuntimeInstrumentationOutputFormat::csv:
      writeRuntimeInstrumentationCsv(output, snapshot);
      return;
  }
}

RuntimeInstrumentationCsvSampler::RuntimeInstrumentationCsvSampler(
    std::shared_ptr<RuntimeInstrumentation> instrumentation,
    std::filesystem::path outputPath,
    std::chrono::milliseconds interval)
    : instrumentation_(std::move(instrumentation)),
      outputPath_(std::move(outputPath)),
      interval_(std::max(interval, std::chrono::milliseconds(1))) {}

RuntimeInstrumentationCsvSampler::RuntimeInstrumentationCsvSampler(
    RuntimeInstrumentationSnapshotProvider snapshotProvider,
    std::filesystem::path outputPath,
    std::chrono::milliseconds interval)
    : snapshotProvider_(std::move(snapshotProvider)),
      outputPath_(std::move(outputPath)),
      interval_(std::max(interval, std::chrono::milliseconds(1))) {}

RuntimeInstrumentationCsvSampler::~RuntimeInstrumentationCsvSampler() {
  stop();
}

void RuntimeInstrumentationCsvSampler::start() {
  std::scoped_lock lock(mutex_);
  if (running_) {
    return;
  }
  if (!instrumentation_ && !snapshotProvider_) {
    throw std::invalid_argument("runtime instrumentation sampler needs a runtime");
  }
  output_.open(outputPath_, std::ios::out | std::ios::trunc);
  if (!output_) {
    throw std::runtime_error(
        "could not open runtime instrumentation CSV output: " +
        outputPath_.string());
  }
  stopRequested_ = false;
  running_ = true;
  try {
    worker_ = std::thread(&RuntimeInstrumentationCsvSampler::run, this);
  } catch (...) {
    running_ = false;
    output_.close();
    throw;
  }
}

void RuntimeInstrumentationCsvSampler::stop() noexcept {
  {
    std::scoped_lock lock(mutex_);
    if (!running_ && !worker_.joinable()) {
      return;
    }
    stopRequested_ = true;
  }
  wakeup_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }
  std::scoped_lock lock(mutex_);
  running_ = false;
  if (output_.is_open()) {
    output_.flush();
    output_.close();
  }
}

bool RuntimeInstrumentationCsvSampler::running() const noexcept {
  std::scoped_lock lock(mutex_);
  return running_;
}

void RuntimeInstrumentationCsvSampler::run() noexcept {
  try {
    auto const startedAt = RuntimeInstrumentation::Clock::now();
    std::map<OperationKey, PreviousOperation> previous;
    std::uint64_t sampleIndex = 0;
    writeSampleCsvHeader(output_);

    while (true) {
      auto const now = RuntimeInstrumentation::Clock::now();
      auto const elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
          now - startedAt);
      auto const snapshot = snapshotProvider_
          ? snapshotProvider_()
          : instrumentation_->snapshot();
      writeSampleCsvRows(
          output_,
          snapshot,
          previous,
          sampleIndex++,
          static_cast<std::uint64_t>(std::max(elapsed.count(), std::int64_t{0})),
          unixMilliseconds());
      output_.flush();
      previous = previousOperations(snapshot);

      std::unique_lock lock(mutex_);
      if (wakeup_.wait_for(lock, interval_, [this] { return stopRequested_; })) {
        break;
      }
    }
  } catch (...) {
    // Diagnostics must never terminate the RTI process if a snapshot or
    // output allocation fails on the sampler thread.
  }

  std::scoped_lock lock(mutex_);
  running_ = false;
}

}  // namespace umbra::detail
