#include "internal/federation/federation_save_commit_store.hpp"

#include "internal/runtime/utf8_string.hpp"

#include <algorithm>
#include <atomic>
#include <charconv>
#include <cerrno>
#include <cctype>
#include <fstream>
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
  result.reserve(value.size());
  for (wchar_t const character : value) {
    if ((character >= L'a' && character <= L'z') ||
        (character >= L'A' && character <= L'Z') ||
        (character >= L'0' && character <= L'9') || character == L'_' ||
        character == L'-') {
      result.push_back(static_cast<char>(character));
    } else {
      result.push_back('_');
    }
  }
  constexpr std::size_t maximumComponentLength = 64U;
  if (result.size() > maximumComponentLength) {
    result.resize(maximumComponentLength);
  }
  return result.empty() ? std::string{"federation-save"} : result;
}

std::string utf8(std::wstring_view value) {
  auto const result = utf8FromWide(value);
  if (!result) {
    throw std::runtime_error(
        "An Umbra federation-save commit field is not valid Unicode text.");
  }
  return *result;
}

std::string jsonString(std::wstring_view value) {
  auto const encoded = utf8(value);
  std::string result;
  result.reserve(encoded.size() + 2U);
  result.push_back('"');
  for (unsigned char const character : encoded) {
    switch (character) {
      case '"':
        result += "\\\"";
        break;
      case '\\':
        result += "\\\\";
        break;
      case '\b':
        result += "\\b";
        break;
      case '\f':
        result += "\\f";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        if (character < 0x20U) {
          constexpr char hexadecimal[] = "0123456789abcdef";
          result += "\\u00";
          result.push_back(hexadecimal[(character >> 4U) & 0x0fU]);
          result.push_back(hexadecimal[character & 0x0fU]);
        } else {
          result.push_back(static_cast<char>(character));
        }
        break;
    }
  }
  result.push_back('"');
  return result;
}

// State-image payloads are canonical ASCII today, but keep this encoder
// byte-oriented so a future image version can carry arbitrary UTF-8 without
// weakening the surrounding JSON envelope.
std::string jsonRawString(std::string_view encoded) {
  constexpr char hexadecimal[] = "0123456789abcdef";
  std::string result;
  result.reserve(encoded.size() + 2U);
  result.push_back('"');
  for (unsigned char const character : encoded) {
    switch (character) {
      case '"':
        result += "\\\"";
        break;
      case '\\':
        result += "\\\\";
        break;
      case '\b':
        result += "\\b";
        break;
      case '\f':
        result += "\\f";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        if (character < 0x20U) {
          result += "\\u00";
          result.push_back(hexadecimal[(character >> 4U) & 0x0fU]);
          result.push_back(hexadecimal[character & 0x0fU]);
        } else {
          result.push_back(static_cast<char>(character));
        }
        break;
    }
  }
  result.push_back('"');
  return result;
}

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
    throw std::runtime_error(
        "Unable to reserve an Umbra federation-save manifest.");
  }
  if (::CloseHandle(file) == 0) {
    std::error_code ignored;
    std::filesystem::remove(location, ignored);
    throw std::runtime_error(
        "Unable to finalize an Umbra federation-save manifest reservation.");
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
    throw std::runtime_error(
        "Unable to reserve an Umbra federation-save manifest.");
  }
  if (::close(descriptor) != 0) {
    std::error_code ignored;
    std::filesystem::remove(location, ignored);
    throw std::runtime_error(
        "Unable to finalize an Umbra federation-save manifest reservation.");
  }
#endif
  return true;
}

std::string manifestFor(FederationSaveCommitDescriptor const& descriptor) {
  std::string result;
  result += "{\n";
  result += "  \"format\": \"umbra-federation-save-commit/v1\",\n";
  result += "  \"federation\": ";
  result += jsonString(descriptor.federationName);
  result += ",\n  \"label\": ";
  result += jsonString(descriptor.label);
  result += ",\n  \"logicalTimeImplementation\": ";
  result += jsonString(descriptor.logicalTimeImplementationName);
  result += ",\n  \"timed\": ";
  result += descriptor.timed ? "true" : "false";
  result += ",\n  \"stateImage\": ";
  result += jsonRawString(descriptor.stateImage);
  result += ",\n  \"memberFederateIds\": [";
  for (std::size_t index = 0U; index < descriptor.memberFederateIds.size(); ++index) {
    if (index != 0U) {
      result += ", ";
    }
    result += std::to_string(descriptor.memberFederateIds[index]);
  }
  result += "]\n}\n";
  return result;
}

std::size_t skipWhitespace(std::string const& contents, std::size_t position) {
  while (position < contents.size() &&
         std::isspace(static_cast<unsigned char>(contents[position])) != 0) {
    ++position;
  }
  return position;
}

std::string parseJsonString(
    std::string const& contents,
    std::size_t& position) {
  if (position >= contents.size() || contents[position] != '"') {
    throw std::runtime_error(
        "Malformed Umbra federation-save manifest string.");
  }
  ++position;
  std::string result;
  while (position < contents.size()) {
    auto const character = static_cast<unsigned char>(contents[position++]);
    if (character == '"') {
      return result;
    }
    if (character != '\\') {
      result.push_back(static_cast<char>(character));
      continue;
    }
    if (position >= contents.size()) {
      break;
    }
    auto const escaped = static_cast<unsigned char>(contents[position++]);
    switch (escaped) {
      case '"':
      case '\\':
      case '/':
        result.push_back(static_cast<char>(escaped));
        break;
      case 'b':
        result.push_back('\b');
        break;
      case 'f':
        result.push_back('\f');
        break;
      case 'n':
        result.push_back('\n');
        break;
      case 'r':
        result.push_back('\r');
        break;
      case 't':
        result.push_back('\t');
        break;
      case 'u': {
        if (position + 4U > contents.size()) {
          throw std::runtime_error(
              "Malformed Umbra federation-save manifest Unicode escape.");
        }
        unsigned int codePoint = 0U;
        for (std::size_t digit = 0U; digit < 4U; ++digit) {
          auto const value = static_cast<unsigned char>(contents[position++]);
          codePoint <<= 4U;
          if (value >= '0' && value <= '9') {
            codePoint += value - '0';
          } else if (value >= 'a' && value <= 'f') {
            codePoint += value - 'a' + 10U;
          } else if (value >= 'A' && value <= 'F') {
            codePoint += value - 'A' + 10U;
          } else {
            throw std::runtime_error(
                "Malformed Umbra federation-save manifest Unicode escape.");
          }
        }
        if (codePoint > 0x7fU) {
          throw std::runtime_error(
              "Unsupported escaped Unicode in Umbra federation-save manifest.");
        }
        result.push_back(static_cast<char>(codePoint));
        break;
      }
      default:
        throw std::runtime_error(
            "Malformed Umbra federation-save manifest escape.");
    }
  }
  throw std::runtime_error(
      "Unterminated Umbra federation-save manifest string.");
}

std::string jsonStringField(
    std::string const& contents,
    std::string_view key) {
  auto const marker = std::string{"\""} + std::string{key} + "\":";
  auto const markerPosition = contents.find(marker);
  if (markerPosition == std::string::npos) {
    throw std::runtime_error(
        "Missing field in Umbra federation-save manifest.");
  }
  auto position = skipWhitespace(contents, markerPosition + marker.size());
  return parseJsonString(contents, position);
}

std::string jsonStringFieldOptional(
    std::string const& contents,
    std::string_view key) {
  auto const marker = std::string{"\""} + std::string{key} + "\":";
  auto const markerPosition = contents.find(marker);
  if (markerPosition == std::string::npos) {
    return {};
  }
  auto position = skipWhitespace(contents, markerPosition + marker.size());
  return parseJsonString(contents, position);
}

bool jsonBooleanField(
    std::string const& contents,
    std::string_view key) {
  auto const marker = std::string{"\""} + std::string{key} + "\":";
  auto const markerPosition = contents.find(marker);
  if (markerPosition == std::string::npos) {
    throw std::runtime_error(
        "Missing boolean field in Umbra federation-save manifest.");
  }
  auto const position = skipWhitespace(contents, markerPosition + marker.size());
  if (contents.compare(position, 4U, "true") == 0) {
    return true;
  }
  if (contents.compare(position, 5U, "false") == 0) {
    return false;
  }
  throw std::runtime_error(
      "Malformed boolean field in Umbra federation-save manifest.");
}

std::vector<std::uint64_t> jsonMemberIdsField(std::string const& contents) {
  constexpr std::string_view key = "\"memberFederateIds\":";
  auto const markerPosition = contents.find(key);
  if (markerPosition == std::string::npos) {
    throw std::runtime_error(
        "Missing memberFederateIds in Umbra federation-save manifest.");
  }
  auto position = skipWhitespace(contents, markerPosition + key.size());
  if (position >= contents.size() || contents[position] != '[') {
    throw std::runtime_error(
        "Malformed memberFederateIds in Umbra federation-save manifest.");
  }
  ++position;
  std::vector<std::uint64_t> result;
  while (true) {
    position = skipWhitespace(contents, position);
    if (position >= contents.size()) {
      throw std::runtime_error(
          "Unterminated memberFederateIds in Umbra federation-save manifest.");
    }
    if (contents[position] == ']') {
      ++position;
      return result;
    }
    std::uint64_t value = 0U;
    auto const* begin = contents.data() + position;
    auto const* end = contents.data() + contents.size();
    auto const parsed = std::from_chars(begin, end, value);
    if (parsed.ec != std::errc{} || parsed.ptr == begin) {
      throw std::runtime_error(
          "Malformed memberFederateIds value in Umbra federation-save manifest.");
    }
    result.push_back(value);
    position = static_cast<std::size_t>(parsed.ptr - contents.data());
    position = skipWhitespace(contents, position);
    if (position >= contents.size()) {
      throw std::runtime_error(
          "Unterminated memberFederateIds in Umbra federation-save manifest.");
    }
    if (contents[position] == ',') {
      ++position;
      continue;
    }
    if (contents[position] == ']') {
      ++position;
      return result;
    }
    throw std::runtime_error(
        "Malformed memberFederateIds separator in Umbra federation-save manifest.");
  }
}

FederationSaveCommitDescriptor parseManifest(std::filesystem::path const& location) {
  std::ifstream input(location, std::ios::binary);
  if (!input) {
    throw std::runtime_error(
        "Unable to read an Umbra federation-save manifest.");
  }
  std::string contents{
      std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  if (input.bad()) {
    throw std::runtime_error(
        "Unable to finish reading an Umbra federation-save manifest.");
  }
  if (jsonStringField(contents, "format") !=
      "umbra-federation-save-commit/v1") {
    throw std::runtime_error(
        "Unsupported Umbra federation-save manifest version.");
  }
  auto const federation = wideFromUtf8(jsonStringField(contents, "federation"));
  auto const label = wideFromUtf8(jsonStringField(contents, "label"));
  auto const implementation = wideFromUtf8(
      jsonStringField(contents, "logicalTimeImplementation"));
  auto const stateImage = jsonStringFieldOptional(contents, "stateImage");
  if (!federation || !label || !implementation) {
    throw std::runtime_error(
        "Invalid UTF-8 in Umbra federation-save manifest.");
  }
  return FederationSaveCommitDescriptor{
      *federation,
      *label,
      *implementation,
      jsonMemberIdsField(contents),
      jsonBooleanField(contents, "timed"),
      stateImage};
}

void writeManifest(
    std::filesystem::path const& temporary,
    std::filesystem::path const& final,
    std::string const& contents) {
  try {
    std::ofstream output(temporary, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!output) {
      throw std::runtime_error(
          "Unable to open an Umbra federation-save manifest.");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.flush();
    if (!output) {
      throw std::runtime_error(
          "Unable to write an Umbra federation-save manifest.");
    }
    output.close();
    if (!output) {
      throw std::runtime_error(
          "Unable to close an Umbra federation-save manifest.");
    }
    std::error_code error;
    std::filesystem::rename(temporary, final, error);
    if (error) {
      throw std::runtime_error(
          "Unable to publish an Umbra federation-save manifest.");
    }
  } catch (...) {
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    std::filesystem::remove(final, ignored);
    throw;
  }
}

}  // namespace

void MemoryFederationSaveCommitStore::commit(
    FederationSaveCommitDescriptor const& descriptor) {
  std::scoped_lock lock(mutex_);
  commits_.push_back(descriptor);
}

std::optional<FederationSaveCommitDescriptor>
MemoryFederationSaveCommitStore::load(
    std::wstring const& federationName,
    std::wstring const& label) const {
  std::scoped_lock lock(mutex_);
  for (auto iterator = commits_.rbegin(); iterator != commits_.rend(); ++iterator) {
    if (iterator->federationName == federationName && iterator->label == label) {
      return *iterator;
    }
  }
  return std::nullopt;
}

std::vector<FederationSaveCommitDescriptor>
MemoryFederationSaveCommitStore::snapshotCommits() const {
  std::scoped_lock lock(mutex_);
  return commits_;
}

FilesystemFederationSaveCommitStore::FilesystemFederationSaveCommitStore(
    std::filesystem::path directory)
    : directory_(std::move(directory)) {}

void FilesystemFederationSaveCommitStore::commit(
    FederationSaveCommitDescriptor const& descriptor) {
  std::error_code error;
  std::filesystem::create_directories(directory_, error);
  if (error || !std::filesystem::is_directory(directory_, error) || error) {
    throw std::runtime_error(
        "Unable to create or access the Umbra federation-save directory.");
  }

  static std::atomic_uint64_t nextUnique{1U};
  constexpr std::size_t maximumReservationAttempts = 1024U;
  auto const stem = "umbra-federation-save-" + safeComponent(descriptor.federationName) +
      "-" + safeComponent(descriptor.label);
  // Render before reserving a pathname so malformed Unicode cannot leave a
  // misleading temporary manifest behind.
  auto const contents = manifestFor(descriptor);
  for (std::size_t attempt = 0U; attempt < maximumReservationAttempts; ++attempt) {
    auto const sequence = nextUnique.fetch_add(1U, std::memory_order_relaxed);
    auto sequenceText = std::to_string(sequence);
    if (sequenceText.size() < 20U) {
      sequenceText.insert(0U, 20U - sequenceText.size(), '0');
    }
    auto const final = std::filesystem::absolute(
                           directory_ / (stem + "-" + sequenceText + ".json"))
                           .lexically_normal();
    auto temporary = final;
    temporary += ".tmp";
    if (!reserveNewFile(temporary)) {
      continue;
    }
    writeManifest(temporary, final, contents);
    return;
  }
  throw std::runtime_error(
      "Unable to allocate a unique Umbra federation-save manifest.");
}

std::optional<FederationSaveCommitDescriptor>
FilesystemFederationSaveCommitStore::load(
    std::wstring const& federationName,
    std::wstring const& label) const {
  std::error_code error;
  if (!std::filesystem::exists(directory_, error)) {
    if (error) {
      throw std::runtime_error(
          "Unable to inspect the Umbra federation-save directory.");
    }
    return std::nullopt;
  }
  if (!std::filesystem::is_directory(directory_, error) || error) {
    throw std::runtime_error(
        "The Umbra federation-save path is not a directory.");
  }

  auto const prefix = "umbra-federation-save-" + safeComponent(federationName) +
      "-" + safeComponent(label) + "-";
  std::vector<std::filesystem::path> candidates;
  std::filesystem::directory_iterator entries(directory_, error);
  if (error) {
    throw std::runtime_error(
        "Unable to enumerate the Umbra federation-save directory.");
  }
  for (auto const& entry : entries) {
    if (!entry.is_regular_file(error) || error) {
      error.clear();
      continue;
    }
    auto const filename = entry.path().filename().string();
    if (entry.path().extension() == ".json" && filename.starts_with(prefix)) {
      candidates.push_back(entry.path());
    }
  }
  if (candidates.empty()) {
    return std::nullopt;
  }
  std::sort(
      candidates.begin(),
      candidates.end(),
      [](std::filesystem::path const& first, std::filesystem::path const& second) {
        return first.filename().string() < second.filename().string();
      });
  auto descriptor = parseManifest(candidates.back());
  if (descriptor.federationName != federationName || descriptor.label != label) {
    throw std::runtime_error(
        "Federation-save manifest identity does not match its filename.");
  }
  return descriptor;
}

std::filesystem::path FilesystemFederationSaveCommitStore::directory() const {
  return directory_;
}

}  // namespace umbra::detail
