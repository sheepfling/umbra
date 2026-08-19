#include <catch2/catch_test_macros.hpp>

#include "internal/service_report_store.hpp"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <future>
#include <iterator>
#include <set>
#include <thread>

namespace {

std::filesystem::path temporaryDirectory() {
  static std::atomic_uint64_t next{0};
  return std::filesystem::temp_directory_path() /
      ("umbra-service-report-store-" + std::to_string(++next));
}

umbra::detail::JoinedFederateReportDescriptor descriptor() {
  return {L"federation / unsafe", L"observer name", 7U, 42U, L"{\"initial\":true}"};
}

}  // namespace

TEST_CASE("Filesystem service-report stores allocate stable unique files and initial records", "[mom][service-report-store]") {
  auto const directory = temporaryDirectory();
  umbra::detail::FilesystemServiceReportStore store(directory);
  auto first = store.createForJoinedFederate(descriptor());
  auto second = store.createForJoinedFederate(descriptor());

  REQUIRE(first->location().is_absolute());
  REQUIRE(first->location().parent_path() == std::filesystem::absolute(directory));
  REQUIRE(first->location() != second->location());
  REQUIRE(std::filesystem::exists(first->location()));
  first->append(L"{\"record\":1}");
  std::ifstream input(first->location(), std::ios::binary);
  std::string contents{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  REQUIRE(contents == "{\"initial\":true}{\"record\":1}");

  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE("Filesystem service-report stores preserve Unicode log text as UTF-8", "[mom][service-report-store]") {
  auto const directory = temporaryDirectory();
  auto record = descriptor();
  record.initialRecord = L"{\"text\":\"\u00E9\"}";
  umbra::detail::FilesystemServiceReportStore store(directory);
  auto writer = store.createForJoinedFederate(record);
  std::ifstream input(writer->location(), std::ios::binary);
  std::string contents{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  REQUIRE(contents == "{\"text\":\"\xC3\xA9\"}");
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem service-report writers do not recreate a missing joined-federate file",
    "[mom][service-report-store]") {
  auto const directory = temporaryDirectory();
  umbra::detail::FilesystemServiceReportStore store(directory);
  auto writer = store.createForJoinedFederate(descriptor());
  auto const location = writer->location();

  REQUIRE(std::filesystem::remove(location));
  REQUIRE_FALSE(std::filesystem::exists(location));
  REQUIRE_THROWS(writer->append(L"{\"record\":2}"));
  REQUIRE_FALSE(std::filesystem::exists(location));

  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem service-report stores reserve unique files under concurrent join pressure",
    "[mom][service-report-store][concurrency]") {
  auto const directory = temporaryDirectory();
  umbra::detail::FilesystemServiceReportStore store(directory);
  constexpr std::size_t writerCount = 24;
  std::atomic_bool mayCreate{false};
  std::vector<std::future<std::unique_ptr<umbra::detail::ServiceReportWriter>>> pending;
  pending.reserve(writerCount);
  for (std::size_t index = 0; index < writerCount; ++index) {
    pending.push_back(std::async(std::launch::async, [&store, &mayCreate] {
      while (!mayCreate.load(std::memory_order_acquire)) {
        std::this_thread::yield();
      }
      return store.createForJoinedFederate(descriptor());
    }));
  }
  mayCreate.store(true, std::memory_order_release);

  std::set<std::filesystem::path> locations;
  for (auto& future : pending) {
    auto writer = future.get();
    REQUIRE(writer);
    REQUIRE(locations.insert(writer->location()).second);
    std::ifstream input(writer->location(), std::ios::binary);
    std::string contents{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    REQUIRE(contents == "{\"initial\":true}");
  }
  REQUIRE(locations.size() == writerCount);

  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem service-report writers preserve whole records under concurrent appends",
    "[mom][service-report-store][concurrency]") {
  auto const directory = temporaryDirectory();
  umbra::detail::FilesystemServiceReportStore store(directory);
  auto writer = store.createForJoinedFederate(descriptor());
  constexpr std::size_t recordCount = 24U;
  std::atomic_bool mayAppend{false};
  std::vector<std::future<void>> pending;
  pending.reserve(recordCount);
  for (std::size_t index = 0U; index < recordCount; ++index) {
    pending.push_back(std::async(std::launch::async, [&writer, &mayAppend, index] {
      while (!mayAppend.load(std::memory_order_acquire)) {
        std::this_thread::yield();
      }
      writer->append(L"{\"record\":" + std::to_wstring(index) + L"}");
    }));
  }
  mayAppend.store(true, std::memory_order_release);
  for (auto& future : pending) {
    REQUIRE_NOTHROW(future.get());
  }

  std::ifstream input(writer->location(), std::ios::binary);
  std::string contents{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  constexpr std::string_view initialRecord{"{\"initial\":true}"};
  REQUIRE(contents.starts_with(initialRecord));
  auto remaining = contents.substr(initialRecord.size());
  std::set<std::string> observedRecords;
  while (!remaining.empty()) {
    auto const recordEnd = remaining.find('}');
    REQUIRE(recordEnd != std::string::npos);
    observedRecords.insert(remaining.substr(0U, recordEnd + 1U));
    remaining.erase(0U, recordEnd + 1U);
  }
  REQUIRE(observedRecords.size() == recordCount);
  for (std::size_t index = 0U; index < recordCount; ++index) {
    REQUIRE(observedRecords.contains("{\"record\":" + std::to_string(index) + "}"));
  }

  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE("Memory service-report stores are explicit test seams only", "[mom][service-report-store]") {
  umbra::detail::MemoryServiceReportStore store;
  auto writer = store.createForJoinedFederate(descriptor());
  writer->append(L"{\"record\":2}");
  REQUIRE(writer->location().empty());
  REQUIRE(store.records() == std::vector<std::wstring>{L"{\"initial\":true}", L"{\"record\":2}"});
}

TEST_CASE(
    "Service-report stores expose internal creation and append timing",
    "[mom][service-report-store][instrumentation]") {
  auto const directory = temporaryDirectory();
  auto instrumentation = std::make_shared<umbra::detail::RuntimeInstrumentation>();
  umbra::detail::FilesystemServiceReportStore store(directory, instrumentation);
  auto writer = store.createForJoinedFederate(descriptor());
  writer->append(L"{\"record\":3}");

  auto const snapshot = instrumentation->snapshot();
  auto callsFor = [&snapshot](std::string_view name) {
    auto const found = std::find_if(
        snapshot.operations.begin(),
        snapshot.operations.end(),
        [name](umbra::detail::InstrumentationOperationSnapshot const& operation) {
          return operation.layer == umbra::detail::InstrumentationLayer::service_reporting &&
              operation.name == name;
        });
    return found == snapshot.operations.end() ? 0U : found->calls;
  };

  REQUIRE(callsFor("create_writer") == 1U);
  REQUIRE(callsFor("file_append") == 1U);

  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}
