#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace umbra::detail {

enum class FomModuleKind {
  fom,
  mim,
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
