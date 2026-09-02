#include "internal/observability/service_report_store.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstddef>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string_view>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace umbra::detail {
namespace {

std::string safeComponent(std::wstring const& value) {
  std::string result;
  for (wchar_t const character : value) {
    if ((character >= L'a' && character <= L'z') ||
        (character >= L'A' && character <= L'Z') ||
        (character >= L'0' && character <= L'9') || character == L'_' || character == L'-') {
      result.push_back(static_cast<char>(character));
    } else {
      result.push_back('_');
    }
  }
  // The human-readable pieces are only diagnostic. Keep them bounded so an
  // unusually long federation or federate name cannot consume the platform's
  // filename budget; the RTI-owned join identifier remains the uniqueness
  // component.
  constexpr std::size_t maximumComponentLength = 64;
  if (result.size() > maximumComponentLength) {
    result.resize(maximumComponentLength);
  }
  return result.empty() ? std::string{"joined-federate"} : result;
}

std::string utf8(std::wstring_view value) {
  auto const result = utf8FromWide(value);
  if (!result) {
    throw std::runtime_error("An Umbra service-report record is not valid Unicode text.");
  }
  return *result;
}

// Reserve a path atomically before opening an iostream. std::ofstream with
// truncation can overwrite an independently selected same name; the standard
// requires each joined federate's report file to be unique. The native create
// primitives provide the necessary no-replacement decision on the real
// filesystem, including across Umbra processes that happen to share a report
// directory.
bool reserveNewFile(std::filesystem::path const& location) {
#if defined(_WIN32)
  auto const file = ::CreateFileW(
      location.c_str(),
      GENERIC_WRITE,
      0,
      nullptr,
      CREATE_NEW,
      FILE_ATTRIBUTE_NORMAL,
      nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    auto const error = ::GetLastError();
    if (error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS) {
      return false;
    }
    throw std::runtime_error("Unable to reserve an Umbra service-report file.");
  }
  if (::CloseHandle(file) == 0) {
    std::error_code ignored;
    std::filesystem::remove(location, ignored);
    throw std::runtime_error("Unable to finalize an Umbra service-report file reservation.");
  }
#else
  auto const descriptor = ::open(
      location.c_str(),
      O_WRONLY | O_CREAT | O_EXCL,
      S_IRUSR | S_IWUSR);
  if (descriptor < 0) {
    if (errno == EEXIST) {
      return false;
    }
    throw std::runtime_error("Unable to reserve an Umbra service-report file.");
  }
  if (::close(descriptor) != 0) {
    std::error_code ignored;
    std::filesystem::remove(location, ignored);
    throw std::runtime_error("Unable to finalize an Umbra service-report file reservation.");
  }
#endif
  return true;
}

void appendUtf8ToExistingFile(
    std::filesystem::path const& location,
    std::string_view contents,
    char const* failureDescription) {
#if defined(_WIN32)
  auto const file = ::CreateFileW(
      location.c_str(),
      FILE_APPEND_DATA,
      FILE_SHARE_READ,
      nullptr,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    throw std::runtime_error(failureDescription);
  }
  try {
    std::size_t offset = 0U;
    while (offset < contents.size()) {
      auto const remaining = contents.size() - offset;
      auto const requested = static_cast<DWORD>(std::min<std::size_t>(
          remaining,
          static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())));
      DWORD written = 0U;
      if (::WriteFile(file, contents.data() + offset, requested, &written, nullptr) == 0 ||
          written != requested) {
        throw std::runtime_error(failureDescription);
      }
      offset += written;
    }
  } catch (...) {
    static_cast<void>(::CloseHandle(file));
    throw;
  }
  if (::CloseHandle(file) == 0) {
    throw std::runtime_error(failureDescription);
  }
#else
  auto const descriptor = ::open(location.c_str(), O_WRONLY | O_APPEND);
  if (descriptor < 0) {
    throw std::runtime_error(failureDescription);
  }
  try {
    std::size_t offset = 0U;
    while (offset < contents.size()) {
      auto const remaining = contents.size() - offset;
      auto const requested = std::min<std::size_t>(
          remaining,
          static_cast<std::size_t>(std::numeric_limits<ssize_t>::max()));
      auto const written = ::write(descriptor, contents.data() + offset, requested);
      if (written < 0 && errno == EINTR) {
        continue;
      }
      if (written <= 0) {
        throw std::runtime_error(failureDescription);
      }
      offset += static_cast<std::size_t>(written);
    }
  } catch (...) {
    static_cast<void>(::close(descriptor));
    throw;
  }
  if (::close(descriptor) != 0) {
    throw std::runtime_error(failureDescription);
  }
#endif
}

void writeInitialRecord(std::filesystem::path const& location, std::wstring_view initialRecord) {
  // The file has already been atomically reserved empty by this store. Open
  // only that existing file, so even a concurrent external deletion cannot
  // turn initial-record writing into an implicit replacement operation.
  appendUtf8ToExistingFile(
      location,
      utf8(initialRecord),
      "Unable to write the Umbra service-report initial record.");
}

class FilesystemServiceReportWriter final : public ServiceReportWriter {
 public:
  explicit FilesystemServiceReportWriter(
      std::filesystem::path location,
      std::shared_ptr<RuntimeInstrumentation> instrumentation)
      : location_(std::move(location)), instrumentation_(std::move(instrumentation)) {}
  [[nodiscard]] std::filesystem::path location() const override { return location_; }
  void append(std::wstring_view encodedRecord) override {
    auto instrumentationScope = instrumentation_
        ? instrumentation_->begin(
              InstrumentationLayer::service_reporting,
              "file_append")
        : RuntimeInstrumentation::Scope{};
    // One joined-federate writer may eventually be reached by more than one
    // service wrapper. Preserve each brace-delimited record as one contiguous
    // append in this process rather than allowing writes to interleave. An
    // OPEN_EXISTING/O_APPEND operation deliberately fails if somebody has
    // removed the allocated report path; an append must not recreate or
    // replace the joined-federate's mandated logical file.
    std::scoped_lock lock(mutex_);
    appendUtf8ToExistingFile(
        location_, utf8(encodedRecord), "Unable to append an Umbra service-report record.");
  }
 private:
  std::mutex mutex_;
  std::filesystem::path location_;
  std::shared_ptr<RuntimeInstrumentation> instrumentation_;
};

class MemoryServiceReportWriter final : public ServiceReportWriter {
 public:
  explicit MemoryServiceReportWriter(
      std::vector<std::wstring>& records,
      std::mutex& recordsMutex,
      std::shared_ptr<RuntimeInstrumentation> instrumentation)
      : records_(records), recordsMutex_(recordsMutex), instrumentation_(std::move(instrumentation)) {}
  [[nodiscard]] std::filesystem::path location() const override { return {}; }
  void append(std::wstring_view encodedRecord) override {
    auto instrumentationScope = instrumentation_
        ? instrumentation_->begin(
              InstrumentationLayer::service_reporting,
              "memory_append")
        : RuntimeInstrumentation::Scope{};
    std::scoped_lock lock(recordsMutex_);
    records_.emplace_back(encodedRecord);
  }
 private:
  std::vector<std::wstring>& records_;
  std::mutex& recordsMutex_;
  std::shared_ptr<RuntimeInstrumentation> instrumentation_;
};

}  // namespace

FilesystemServiceReportStore::FilesystemServiceReportStore(
    std::filesystem::path directory,
    std::shared_ptr<RuntimeInstrumentation> instrumentation)
    : directory_(std::move(directory)), instrumentation_(std::move(instrumentation)) {}

std::unique_ptr<ServiceReportWriter> FilesystemServiceReportStore::createForJoinedFederate(
    JoinedFederateReportDescriptor const& descriptor) {
  auto instrumentationScope = instrumentation_
      ? instrumentation_->begin(
            InstrumentationLayer::service_reporting,
            "create_writer")
      : RuntimeInstrumentation::Scope{};
  std::error_code error;
  std::filesystem::create_directories(directory_, error);
  if (error) {
    throw std::runtime_error("Unable to create the Umbra service-report directory.");
  }
  static std::atomic_uint64_t nextUnique{1};
  constexpr std::size_t maximumReservationAttempts = 1024;
  for (std::size_t attempt = 0; attempt < maximumReservationAttempts; ++attempt) {
    auto const filename = "umbra-service-report-" + safeComponent(descriptor.federationName) + "-" +
        safeComponent(descriptor.federateName) + "-" + std::to_string(descriptor.federateId) + "-" +
        std::to_string(descriptor.joinIdentifier) + "-" + std::to_string(nextUnique.fetch_add(1)) + ".log";
    auto const location = std::filesystem::absolute(directory_ / filename).lexically_normal();
    if (!reserveNewFile(location)) {
      continue;
    }
    try {
      writeInitialRecord(location, descriptor.initialRecord);
      return std::make_unique<FilesystemServiceReportWriter>(
          location,
          instrumentation_);
    } catch (...) {
      // This store created the pathname exclusively, so cleanup cannot delete
      // another joined federate's report. Failure still escapes to the caller;
      // cleanup is only to avoid presenting a partial report as a valid file.
      std::error_code ignored;
      std::filesystem::remove(location, ignored);
      throw;
    }
  }
  throw std::runtime_error("Unable to allocate a unique Umbra service-report file.");
}

MemoryServiceReportStore::MemoryServiceReportStore(
    std::shared_ptr<RuntimeInstrumentation> instrumentation)
    : instrumentation_(std::move(instrumentation)) {}

std::unique_ptr<ServiceReportWriter> MemoryServiceReportStore::createForJoinedFederate(
    JoinedFederateReportDescriptor const& descriptor) {
  auto instrumentationScope = instrumentation_
      ? instrumentation_->begin(
            InstrumentationLayer::service_reporting,
            "create_writer")
      : RuntimeInstrumentation::Scope{};
  {
    std::scoped_lock lock(recordsMutex_);
    records_.push_back(descriptor.initialRecord);
  }
  return std::make_unique<MemoryServiceReportWriter>(records_, recordsMutex_, instrumentation_);
}

std::vector<std::wstring> const& MemoryServiceReportStore::records() const noexcept { return records_; }

std::vector<std::wstring> MemoryServiceReportStore::snapshotRecords() const {
  std::scoped_lock lock(recordsMutex_);
  return records_;
}

}  // namespace umbra::detail
