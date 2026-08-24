#include <catch2/catch_test_macros.hpp>

#include "internal/observability/runtime_instrumentation_output.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

namespace {

using namespace std::chrono_literals;
using umbra::detail::InstrumentationLayer;
using umbra::detail::RuntimeInstrumentation;
using umbra::detail::RuntimeInstrumentationCsvSampler;
using umbra::detail::RuntimeInstrumentationOutputFormat;
using umbra::detail::writeRuntimeInstrumentationConsole;
using umbra::detail::writeRuntimeInstrumentationCsv;
using umbra::detail::writeRuntimeInstrumentationJson;
using umbra::detail::writeRuntimeInstrumentation;

std::filesystem::path temporaryOutputPath() {
  static std::atomic_uint64_t next{0};
  return std::filesystem::temp_directory_path() /
      ("umbra-runtime-instrumentation-" + std::to_string(++next) + ".csv");
}

}  // namespace

TEST_CASE(
    "Runtime instrumentation snapshots have console JSON and CSV writers",
    "[unit][kernel][instrumentation][output][foundation]") {
  auto instrumentation = std::make_shared<RuntimeInstrumentation>();
  {
    auto scope = instrumentation->begin(
        InstrumentationLayer::rti_ambassador,
        "operation,with\"delimiters");
    scope.complete();
  }

  auto const snapshot = instrumentation->snapshot();
  std::ostringstream console;
  std::ostringstream json;
  std::ostringstream csv;
  std::ostringstream selected;
  writeRuntimeInstrumentationConsole(console, snapshot);
  writeRuntimeInstrumentationJson(json, snapshot);
  writeRuntimeInstrumentationCsv(csv, snapshot);
  writeRuntimeInstrumentation(
      selected,
      snapshot,
      RuntimeInstrumentationOutputFormat::json);

  REQUIRE(console.str().find("rti_ambassador") != std::string::npos);
  REQUIRE(console.str().find("operation,with\"delimiters") != std::string::npos);
  REQUIRE(json.str().find("\"layer\": \"rti_ambassador\"") != std::string::npos);
  REQUIRE(json.str().find("operation,with\\\"delimiters") != std::string::npos);
  REQUIRE(selected.str() == json.str());
  REQUIRE(csv.str().find(
      "layer,operation,calls,successes,failures,exceptions") == 0);
  std::string const expectedCsv =
      "rti_ambassador,\"operation,with\"\"delimiters\"";
  REQUIRE(csv.str().find(expectedCsv) != std::string::npos);
}

TEST_CASE(
    "Runtime instrumentation CSV sampler records cumulative and interval counters",
    "[unit][kernel][instrumentation][output][foundation]") {
  auto instrumentation = std::make_shared<RuntimeInstrumentation>();
  {
    auto scope = instrumentation->begin(
        InstrumentationLayer::callback_dispatch,
        "sampled");
    scope.complete();
  }

  auto const outputPath = temporaryOutputPath();
  RuntimeInstrumentationCsvSampler sampler(instrumentation, outputPath, 10ms);
  sampler.start();
  REQUIRE(sampler.running());
  // Allow the worker to publish its initial snapshot before introducing the
  // second call; this keeps the delta assertion independent of thread start
  // scheduling on slower Windows runners.
  std::this_thread::sleep_for(75ms);
  {
    auto scope = instrumentation->begin(
        InstrumentationLayer::callback_dispatch,
        "sampled");
    scope.complete();
  }
  std::this_thread::sleep_for(75ms);
  sampler.stop();
  REQUIRE_FALSE(sampler.running());

  std::ifstream input(outputPath, std::ios::binary);
  std::string contents{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};
  REQUIRE(contents.find("sample_index,elapsed_ms,wall_time_unix_ms") == 0);
  REQUIRE(contents.find(",callback_dispatch,sampled,1,1,") != std::string::npos);
  REQUIRE(contents.find(",callback_dispatch,sampled,2,1,") != std::string::npos);

  std::error_code ignored;
  std::filesystem::remove(outputPath, ignored);
}
