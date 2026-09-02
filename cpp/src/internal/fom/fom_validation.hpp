#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace umbra::detail {

// The model edition is deliberately separate from the IEEE 1516.1 binding
// selected by a caller.  A 2025 binding may ingest a 2010 model in the
// compatibility profile, but that does not turn the runtime into a 2010
// binding or make the two standards interchangeable.
enum class FomStandardEdition {
  ieee1516_2010,
  ieee1516_2025,
};

inline constexpr std::string_view fomNamespace(FomStandardEdition edition) noexcept {
  switch (edition) {
    case FomStandardEdition::ieee1516_2010:
      return "http://standards.ieee.org/IEEE1516-2010";
    case FomStandardEdition::ieee1516_2025:
      return "http://standards.ieee.org/IEEE1516-2025";
  }
  return {};
}

inline constexpr std::wstring_view fomDifSchemaDesignator(
    FomStandardEdition edition) noexcept {
  switch (edition) {
    case FomStandardEdition::ieee1516_2010:
      return L"IEEE1516-DIF-2010.xsd";
    case FomStandardEdition::ieee1516_2025:
      return L"IEEE1516-DIF-2025.xsd";
  }
  return {};
}

inline constexpr std::wstring_view fomFddSchemaDesignator(
    FomStandardEdition edition) noexcept {
  switch (edition) {
    case FomStandardEdition::ieee1516_2010:
      return L"IEEE1516-FDD-2010.xsd";
    case FomStandardEdition::ieee1516_2025:
      return L"IEEE1516-FDD-2025.xsd";
  }
  return {};
}

enum class FomModuleKind {
  fom,
  mim,
};

// Source compatibility is deliberately separate from the selected IEEE model
// edition.  The RPR option is a narrow, opt-in normalization of a known
// 2010-era source defect; it is not a relaxed 2025 schema or a license to
// rewrite arbitrary FOM XML.
enum class FomSourceCompatibility {
  strict,
  rpr_2010,
};

// This is the only FOM-shaped value the federation registry may accept. A
// future XML/XSD backend creates it after parsing, schema validation, and
// module-level checks succeed. The designator is the exact supplied service
// argument (or HLAstandardMIM); sourcePath is its separately canonicalized
// local resolution. Callers must not use it as a well-formed-XML marker or
// construct it from an unchecked public service argument.
struct PrevalidatedFomModule {
  std::wstring designator;
  std::filesystem::path sourcePath;
  std::filesystem::path schemaPath;
  FomModuleKind kind = FomModuleKind::fom;
  std::wstring schemaDesignator;
  // UTF-16 view of the validated module's canonical UTF-8 XML serialization.
  // Retaining the content at validation time lets later MOM requests report
  // the module even if its original filesystem path is changed or removed;
  // it is not a second validation or composition input.
  std::wstring contents;
  FomStandardEdition standardEdition = FomStandardEdition::ieee1516_2025;
  FomSourceCompatibility sourceCompatibility = FomSourceCompatibility::strict;
  std::vector<std::string> warnings;
};

enum class FomValidationStatus {
  valid,
  source_not_found,
  source_unreadable,
  source_parse_error,
  invalid_model,
  validator_failure,
};

struct FomValidationRequest {
  std::filesystem::path sourcePath;
  std::filesystem::path schemaPath;
  FomModuleKind moduleKind = FomModuleKind::fom;
  std::wstring designator;
  std::wstring schemaDesignator;
  FomStandardEdition standardEdition = FomStandardEdition::ieee1516_2025;
  FomSourceCompatibility sourceCompatibility = FomSourceCompatibility::strict;
};

struct FomValidationResult {
  FomValidationStatus status = FomValidationStatus::validator_failure;
  std::optional<PrevalidatedFomModule> module;
  std::string diagnostics;
};

class FomValidator {
 public:
  virtual ~FomValidator() = default;

  [[nodiscard]] virtual FomValidationResult validate(
      FomValidationRequest const& request) const = 0;
};

}  // namespace umbra::detail
