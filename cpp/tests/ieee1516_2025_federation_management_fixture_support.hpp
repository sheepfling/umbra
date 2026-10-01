#pragma once
#include "ieee1516_2025_federation_management_test_support.hpp"

namespace {
std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
         "third_party" /
         "ieee1516.2-2025" /
         "resources" /
         relativePath;
}

class ScopedTemporaryFile final {
 public:
  explicit ScopedTemporaryFile(
      std::filesystem::path path,
      std::filesystem::path reservationPath = {})
      : path_(std::move(path)), reservationPath_(std::move(reservationPath)) {}

  ScopedTemporaryFile(ScopedTemporaryFile const&) = delete;
  ScopedTemporaryFile& operator=(ScopedTemporaryFile const&) = delete;
  ScopedTemporaryFile(ScopedTemporaryFile&& other) noexcept
      : path_(std::move(other.path_)),
        reservationPath_(std::move(other.reservationPath_)) {
    other.path_.clear();
    other.reservationPath_.clear();
  }
  ScopedTemporaryFile& operator=(ScopedTemporaryFile&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    std::error_code ignored;
    std::filesystem::remove(path_, ignored);
    std::filesystem::remove_all(reservationPath_, ignored);
    path_ = std::move(other.path_);
    reservationPath_ = std::move(other.reservationPath_);
    other.path_.clear();
    other.reservationPath_.clear();
    return *this;
  }

  ~ScopedTemporaryFile() {
    std::error_code ignored;
    std::filesystem::remove(path_, ignored);
    std::filesystem::remove_all(reservationPath_, ignored);
  }

  [[nodiscard]] std::filesystem::path const& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
  std::filesystem::path reservationPath_;
};

// Catch2 launches focused cases in separate processes. A process-local
// counter is therefore not enough to make a generated FOM path unique: two
// workers can both create "...-1.xml" and truncate one another's input. Use
// an atomically-created sibling directory as a reservation token, then keep
// that token alive with the returned temporary file until the fixture dies.
ScopedTemporaryFile reserveTemporaryFomPath(
    std::string_view prefix,
    std::string_view extension) {
  static std::atomic_uint64_t counter{0};
  auto const tick = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count();
  auto const threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());
  auto const parent = std::filesystem::temp_directory_path();
  for (std::size_t attempt = 0U; attempt != 1024U; ++attempt) {
    auto const serial = ++counter;
    auto const stem = std::string(prefix) + "-" + std::to_string(tick) + "-" +
        std::to_string(threadId) + "-" + std::to_string(serial) + "-" +
        std::to_string(attempt);
    auto const path = parent / (stem + std::string(extension));
    auto const reservationPath = parent / (stem + ".reservation");
    std::error_code error;
    if (std::filesystem::create_directory(reservationPath, error)) {
      return ScopedTemporaryFile(path, reservationPath);
    }
    if (error && error != std::errc::file_exists) {
      throw std::filesystem::filesystem_error(
          "Unable to reserve a temporary FOM path", reservationPath, error);
    }
  }
  throw std::runtime_error("Unable to reserve a unique temporary FOM path.");
}

class ScopedTemporaryDirectory final {
 public:
  explicit ScopedTemporaryDirectory(std::filesystem::path path) : path_(std::move(path)) {}

  ScopedTemporaryDirectory(ScopedTemporaryDirectory const&) = delete;
  ScopedTemporaryDirectory& operator=(ScopedTemporaryDirectory const&) = delete;

  ~ScopedTemporaryDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }

  [[nodiscard]] std::filesystem::path const& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

ScopedTemporaryFile nrgEnabledRestaurantModule() {
  std::ifstream input(resourcePath("examples/RestaurantFOMmodule-2025.xml"), std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};
  std::string const marker = "</switches>";
  auto const switchesPosition = fomText.find(marker);
  REQUIRE(switchesPosition != std::string::npos);
  fomText.insert(
      switchesPosition,
      "        <nonRegulatedGrant isEnabled=\"true\"/>\n    ");

  auto temporary = reserveTemporaryFomPath("umbra-nrg-scheduler", ".xml");
  auto const path = temporary.path();
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << fomText;
  REQUIRE(output.good());
  return temporary;
}

ScopedTemporaryFile knownClassEnabledRestaurantModule() {
  std::ifstream input(resourcePath("examples/RestaurantFOMmodule-2025.xml"), std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};
  auto const marker = std::string{"</switches>"};
  auto const switchesPosition = fomText.find(marker);
  REQUIRE(switchesPosition != std::string::npos);
  fomText.insert(
      switchesPosition,
      "        <advisoriesUseKnownClass isEnabled=\"true\"/>\n    ");

  auto temporary = reserveTemporaryFomPath("umbra-known-class-advisories", ".xml");
  auto const path = temporary.path();
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output.write(fomText.data(), static_cast<std::streamsize>(fomText.size()));
  REQUIRE(output.good());
  return temporary;
}

ScopedTemporaryFile lowRateAttributeUpdatePasselModule() {
  auto const source = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "attribute-update-passel-fom.xml";
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};
  auto const marker = std::string{"</objects>"};
  auto const insertion = fomText.find(marker);
  REQUIRE(insertion != std::string::npos);
  fomText.insert(
      insertion + marker.size(),
      "\n    <updateRates>\n"
       "        <updateRate>\n"
       "            <name>Low</name>\n"
       "            <rate>0.2</rate>\n"
       "        </updateRate>\n"
       "        <updateRate>\n"
       "            <name>High</name>\n"
       "            <rate>30</rate>\n"
       "        </updateRate>\n"
       "    </updateRates>");

  auto temporary = reserveTemporaryFomPath(
      "umbra-update-rate-attribute-passel", ".xml");
  auto const path = temporary.path();
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output.write(fomText.data(), static_cast<std::streamsize>(fomText.size()));
  REQUIRE(output.good());
  return temporary;
}

ScopedTemporaryFile lowRateRegionalAttributeUpdateModule() {
  auto const source = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "two-dimensional-regional-interaction-fom.xml";
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};

  // The regional fixture deliberately starts with the standard reliable
  // transport.  This lane changes only the object attribute to best-effort so
  // the FDD-defined Low rate can be observed at the reflection boundary.
  auto const reliableMarker = std::string{
      "                    <transportation>HLAreliable</transportation>"};
  auto const transportationPosition = fomText.find(reliableMarker);
  REQUIRE(transportationPosition != std::string::npos);
  fomText.replace(
      transportationPosition,
      reliableMarker.size(),
      "                    <transportation>HLAbestEffort</transportation>");

  // DIF orders updateRates after dimensions; unlike the smaller object-only
  // fixture, this regional file also declares interactions before its
  // dimensions, so insert at the schema-defined position.
  auto const dimensionsMarker = std::string{"</dimensions>"};
  auto const insertion = fomText.rfind(dimensionsMarker);
  REQUIRE(insertion != std::string::npos);
  fomText.insert(
      insertion + dimensionsMarker.size(),
      "\n    <updateRates>\n"
      "        <updateRate>\n"
      "            <name>Low</name>\n"
      "            <rate>0.2</rate>\n"
      "        </updateRate>\n"
      "        <updateRate>\n"
      "            <name>High</name>\n"
      "            <rate>30</rate>\n"
      "        </updateRate>\n"
      "    </updateRates>");

  auto temporary = reserveTemporaryFomPath(
      "umbra-update-rate-regional-attribute", ".xml");
  auto const path = temporary.path();
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output.write(fomText.data(), static_cast<std::streamsize>(fomText.size()));
  REQUIRE(output.good());
  return temporary;
}

ScopedTemporaryFile mixedRateAttributeUpdatePasselModule() {
  auto const lowRateModule = lowRateAttributeUpdatePasselModule();
  std::ifstream input(lowRateModule.path(), std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};

  // Keep the shared fixture focused on the existing transport cases.  This
  // lane adds a second best-effort attribute so one explicit Low subscription
  // can be exercised beside an independent HLAdefault subscription.
  auto const marker = std::string{
      "                    <attribute>\n"
      "                        <name>UnownedChild</name>"};
  auto const insertion = fomText.find(marker);
  REQUIRE(insertion != std::string::npos);
  fomText.insert(
      insertion,
      "                    <attribute>\n"
      "                        <name>BestEffortChild</name>\n"
      "                        <dataType>HLAunicodeString</dataType>\n"
      "                        <updateType>Static</updateType>\n"
      "                        <updateCondition>Fixture update.</updateCondition>\n"
      "                        <valueRequired>false</valueRequired>\n"
      "                        <ownership>DivestAcquire</ownership>\n"
      "                        <sharing>PublishSubscribe</sharing>\n"
      "                        <transportation>HLAbestEffort</transportation>\n"
      "                        <order>Receive</order>\n"
      "                        <semantics>Second best-effort attribute for mixed update-rate routing.</semantics>\n"
      "                    </attribute>\n");

  auto temporary = reserveTemporaryFomPath(
      "umbra-mixed-update-rate-attribute-passel", ".xml");
  auto const path = temporary.path();
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output.write(fomText.data(), static_cast<std::streamsize>(fomText.size()));
  REQUIRE(output.good());
  return temporary;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0};
  return L"umbra-catch2-federation-" + std::to_wstring(++counter);
}

ScopedTemporaryDirectory temporaryServiceReportDirectory() {
  static std::atomic_uint64_t counter{0};
  auto const parent = std::filesystem::temp_directory_path();
  // Catch2 discovery runs cases in independent processes, so a process-local
  // counter alone can make two focused filesystem lanes race for the same
  // directory. Reserve the path atomically before handing it to the profile.
  for (std::size_t attempt = 0U; attempt != 1024U; ++attempt) {
    auto const path = parent /
        ("umbra-service-report-lifecycle-" + std::to_string(++counter));
    std::error_code error;
    if (std::filesystem::create_directory(path, error)) {
      return ScopedTemporaryDirectory(path);
    }
    if (error && error != std::errc::file_exists) {
      throw std::filesystem::filesystem_error(
          "Unable to reserve a temporary service-report directory", path, error);
    }
  }
  throw std::runtime_error("Unable to reserve a unique temporary service-report directory.");
}

ScopedTemporaryDirectory temporaryFederationSaveDirectory() {
  static std::atomic_uint64_t counter{0};
  auto const parent = std::filesystem::temp_directory_path();
  // Catch2 may execute focused cases in separate worker processes. Include a
  // process-run component so two workers cannot both reserve the same
  // process-local counter value while sharing the system temporary directory.
  auto const steadyTicks = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  auto const wallTicks = static_cast<std::uint64_t>(
      std::chrono::system_clock::now().time_since_epoch().count());
  auto const processRun = steadyTicks ^ (wallTicks + (steadyTicks << 6U) +
                                         (steadyTicks >> 2U)) ^
      static_cast<std::uint64_t>(std::hash<std::thread::id>{}(
          std::this_thread::get_id()));
  for (std::size_t attempt = 0U; attempt != 1024U; ++attempt) {
    auto const path = parent /
        ("umbra-public-federation-save-" + std::to_string(processRun) + "-" +
         std::to_string(++counter));
    std::error_code error;
    if (std::filesystem::create_directory(path, error)) {
      return ScopedTemporaryDirectory(path);
    }
    if (error && error != std::errc::file_exists) {
      throw std::filesystem::filesystem_error(
          "Unable to reserve a temporary federation-save directory", path, error);
    }
  }
  throw std::runtime_error("Unable to reserve a unique temporary federation-save directory.");
}

class ScopedEmbeddedFederationRegistry final {
 public:
  explicit ScopedEmbeddedFederationRegistry(
      std::shared_ptr<umbra::detail::EmbeddedFederationRegistry> replacement)
      : previous_(
            rti1516_2025::umbra_binding_detail::replaceEmbeddedFederationRegistryForTesting(
                std::move(replacement))) {}

  ScopedEmbeddedFederationRegistry(ScopedEmbeddedFederationRegistry const&) = delete;
  ScopedEmbeddedFederationRegistry& operator=(ScopedEmbeddedFederationRegistry const&) = delete;

  ~ScopedEmbeddedFederationRegistry() noexcept {
    if (previous_) {
      static_cast<void>(
          rti1516_2025::umbra_binding_detail::replaceEmbeddedFederationRegistryForTesting(
              std::move(previous_)));
    }
  }

 private:
  std::shared_ptr<umbra::detail::EmbeddedFederationRegistry> previous_;
};

[[nodiscard]] RtiConfiguration configurationForServiceReportDirectory(
    std::filesystem::path const& directory) {
  return umbra::embedded::makeEmbeddedRtiConfiguration(
      umbra::embedded::ServiceReportConfiguration{directory});
}

class FailingServiceReportStore final : public umbra::detail::ServiceReportStore {
 public:
  [[nodiscard]] std::unique_ptr<umbra::detail::ServiceReportWriter> createForJoinedFederate(
      umbra::detail::JoinedFederateReportDescriptor const& descriptor) override {
    static_cast<void>(descriptor);
    ++createCalls;
    throw std::runtime_error("intentional service-report store failure");
  }

  std::size_t createCalls = 0U;
};

class FailingAppendServiceReportWriter final : public umbra::detail::ServiceReportWriter {
 public:
  explicit FailingAppendServiceReportWriter(std::size_t& appendCalls)
      : appendCalls_(appendCalls) {}

  [[nodiscard]] std::filesystem::path location() const override { return {}; }

  void append(std::wstring_view encodedRecord) override {
    static_cast<void>(encodedRecord);
    ++appendCalls_;
    throw std::runtime_error("intentional service-report append failure");
  }

 private:
  std::size_t& appendCalls_;
};

class FailingAppendServiceReportStore final : public umbra::detail::ServiceReportStore {
 public:
  [[nodiscard]] std::unique_ptr<umbra::detail::ServiceReportWriter> createForJoinedFederate(
      umbra::detail::JoinedFederateReportDescriptor const& descriptor) override {
    initialRecords.push_back(descriptor.initialRecord);
    return std::make_unique<FailingAppendServiceReportWriter>(appendCalls);
  }

  std::vector<std::wstring> initialRecords;
  std::size_t appendCalls = 0U;
};

std::vector<std::filesystem::path> serviceReportFiles(
    std::filesystem::path const& directory) {
  std::vector<std::filesystem::path> files;
  for (auto const& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file()) {
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

std::string joinedFederateReportDesignator(std::filesystem::path const& reportFile) {
  auto const text = readTextFile(reportFile);
  constexpr std::string_view marker{"\"HLAfederateHandle\":\""};
  auto const begin = text.find(marker);
  REQUIRE(begin != std::string::npos);
  auto const valueBegin = begin + marker.size();
  auto const valueEnd = text.find('"', valueBegin);
  REQUIRE(valueEnd != std::string::npos);
  return text.substr(valueBegin, valueEnd - valueBegin);
}

std::string initiateFederateRestoreServiceReportRecord(
    std::uint32_t serialNumber,
    std::string const& label,
    std::string const& designator,
    std::string const& federateName) {
  return std::string{R"({"HLAserialNumber":)"} + std::to_string(serialNumber) +
      R"(,"HLAreturnedArgument":[null],"HLAservice":"InitiateFederateRestore","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":")" +
      label +
      R"("},{"HLAargumentType":15,"HLAargumentName":"Joined federate designator","HLAargumentValue":")" +
      designator +
      R"("},{"HLAargumentType":53,"HLAargumentName":"Federate name","HLAargumentValue":")" +
      federateName + R"("}],"HLAsuccessIndicator":true,"HLAexception":null})";
}

std::vector<unsigned char> variableLengthDataBytes(VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

std::optional<std::vector<std::wstring>> decodeHlaUnicodeStringList(
    VariableLengthData const& encoded) {
  try {
    auto const rawBytes = variableLengthDataBytes(encoded);
    std::vector<rti1516_2025::Octet> bytes(rawBytes.begin(), rawBytes.end());
    rti1516_2025::HLAinteger32BE count;
    auto index = count.decodeFrom(bytes, 0U);
    auto const elementCount = count.get();
    if (elementCount < 0) {
      return std::nullopt;
    }
    std::vector<std::wstring> result;
    result.reserve(static_cast<std::size_t>(elementCount));
    for (rti1516_2025::Integer32 element = 0; element < elementCount; ++element) {
      auto const remainder = index % 4U;
      if (remainder != 0U) {
        index += 4U - remainder;
      }
      rti1516_2025::HLAunicodeString designator;
      index = designator.decodeFrom(bytes, index);
      result.push_back(designator.get());
    }
    if (index != bytes.size()) {
      return std::nullopt;
    }
    return result;
  } catch (rti1516_2025::EncoderException const&) {
    return std::nullopt;
  }
}

}  // namespace
