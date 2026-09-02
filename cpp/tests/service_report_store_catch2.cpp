#include <catch2/catch_test_macros.hpp>

#include "internal/observability/service_report_store.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <umbra/embedded_profile_configuration.hpp>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <future>
#include <iterator>
#include <set>
#include <thread>
#include <vector>

namespace {

std::filesystem::path temporaryDirectory() {
  static std::atomic_uint64_t next{0};
  return std::filesystem::temp_directory_path() /
      ("umbra-service-report-store-" + std::to_string(++next));
}

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The service-report lifecycle test requires the Umbra source directory."
#endif

std::vector<std::filesystem::path> serviceReportFiles(
    std::filesystem::path const& directory) {
  std::vector<std::filesystem::path> files;
  std::error_code error;
  if (!std::filesystem::exists(directory, error) || error) {
    return files;
  }
  for (auto const& entry : std::filesystem::directory_iterator(directory, error)) {
    if (error) {
      break;
    }
    if (entry.is_regular_file(error) && !error) {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());
  return files;
}

std::string readTextFile(std::filesystem::path const& path) {
  std::ifstream input(path, std::ios::binary);
  REQUIRE(input.good());
  return {
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};
}

std::wstring nextLifecycleFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"service-report-lifecycle-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

umbra::detail::JoinedFederateReportDescriptor descriptor() {
  return {L"federation / unsafe", L"observer name", 7U, 42U, L"{\"initial\":true}"};
}

}  // namespace

TEST_CASE(
    "Embedded service-report files are allocated at join and remain stable across switch cycles and rejoin",
    "[integration][development-profile][federation-management][mom][service-reporting]"
    "[service-report-file][service-report-store][filesystem][join-lifecycle][2025]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]") {
  using rti1516_2025::HLA_EVOKED;
  using rti1516_2025::NO_ACTION;

  auto const directory = temporaryDirectory();
  auto const federationName = nextLifecycleFederationName();
  std::error_code staleDirectory;
  std::filesystem::remove_all(directory, staleDirectory);
  auto const fomModule =
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" /
      "examples" / "RestaurantFOMmodule-2025.xml";
  auto configuration = umbra::embedded::makeEmbeddedRtiConfiguration(
      umbra::embedded::ServiceReportConfiguration{directory});
  configuration.withRtiAddress(L"in-process");

  rti1516_2025::NullFederateAmbassador federate;
  rti1516_2025::RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule.wstring(),
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"service-report-lifecycle-federate",
      L"lifecycle",
      federationName));

  auto const filesAtJoin = serviceReportFiles(directory);
  REQUIRE(filesAtJoin.size() == 1U);
  auto const reportFile = filesAtJoin.front();
  REQUIRE(std::filesystem::absolute(reportFile).lexically_normal() ==
          reportFile.lexically_normal());
  auto const initialText = readTextFile(reportFile);
  REQUIRE(initialText.find("\"HLAfederateName\":\"service-report-lifecycle-federate\"") !=
          std::string::npos);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE(serviceReportFiles(directory) == filesAtJoin);
  auto const enabledText = readTextFile(reportFile);
  REQUIRE(enabledText.size() >= initialText.size());

  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE(serviceReportFiles(directory) == filesAtJoin);
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE(serviceReportFiles(directory) == filesAtJoin);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule.wstring(),
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"service-report-lifecycle-federate",
      L"lifecycle",
      federationName));
  auto const filesAfterRejoin = serviceReportFiles(directory);
  REQUIRE(filesAfterRejoin.size() == 2U);
  REQUIRE(std::find(filesAfterRejoin.begin(), filesAfterRejoin.end(), reportFile) !=
          filesAfterRejoin.end());
  REQUIRE(std::any_of(
      filesAfterRejoin.begin(),
      filesAfterRejoin.end(),
      [&](auto const& candidate) { return candidate != reportFile; }));

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem service-report stores allocate stable unique files and initial records",
    "[mom][service-report-store][service-reporting][unit]") {
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

TEST_CASE(
    "Filesystem service-report stores preserve Unicode log text as UTF-8",
    "[mom][service-report-store][service-reporting][unit]") {
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
    "Filesystem service-report stores remove a reserved file when the initial record is invalid",
    "[mom][service-report-store][service-reporting][filesystem-rollback][unit]") {
  auto const directory = temporaryDirectory();
  std::error_code ignored;
  // CTest can launch this focused case in a fresh process, so the helper's
  // process-local sequence is not sufficient to distinguish a prior failed
  // run. Start from a clean test-only directory before asserting rollback.
  std::filesystem::remove_all(directory, ignored);
  auto record = descriptor();
  // A lone surrogate is not a Unicode scalar value on either the Windows
  // UTF-16 or POSIX wide-string representation. Initial-record conversion
  // must fail after reservation without leaving a misleading empty file.
  record.initialRecord = std::wstring{L"{\"text\":\""} +
      std::wstring(1U, static_cast<wchar_t>(0xD800U)) + L"\"}";
  umbra::detail::FilesystemServiceReportStore store(directory);

  REQUIRE_THROWS(store.createForJoinedFederate(record));
  REQUIRE(std::filesystem::exists(directory));
  REQUIRE(std::filesystem::is_empty(directory));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem service-report writers do not recreate a missing joined-federate file",
    "[mom][service-report-store][service-reporting][unit]") {
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
    "[mom][service-report-store][service-reporting][concurrency][unit]") {
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
    "[mom][service-report-store][service-reporting][concurrency][unit]") {
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

TEST_CASE(
    "Memory service-report stores are explicit test seams only",
    "[mom][service-report-store][service-reporting][unit]") {
  umbra::detail::MemoryServiceReportStore store;
  auto writer = store.createForJoinedFederate(descriptor());
  writer->append(L"{\"record\":2}");
  REQUIRE(writer->location().empty());
  REQUIRE(store.records() == std::vector<std::wstring>{L"{\"initial\":true}", L"{\"record\":2}"});
}

TEST_CASE(
    "Memory service-report writers serialize concurrent test appends",
    "[mom][service-report-store][service-reporting][concurrency][unit]") {
  umbra::detail::MemoryServiceReportStore store;
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

  auto snapshot = store.snapshotRecords();
  REQUIRE(snapshot.size() == recordCount + 1U);
  REQUIRE(snapshot.front() == L"{\"initial\":true}");
  std::set<std::wstring> observedRecords(snapshot.begin() + 1, snapshot.end());
  REQUIRE(observedRecords.size() == recordCount);
  for (std::size_t index = 0U; index < recordCount; ++index) {
    REQUIRE(observedRecords.contains(L"{\"record\":" + std::to_wstring(index) + L"}"));
  }
}

TEST_CASE(
    "Service-report stores expose internal creation and append timing",
    "[mom][service-report-store][service-reporting][instrumentation][unit]") {
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
