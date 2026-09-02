#pragma once

#include "internal/fom/fom_validation.hpp"

#include <libxml/tree.h>

#include <memory>

namespace umbra::detail {

struct LibXml2DocumentDeleter {
  void operator()(xmlDoc* document) const noexcept;
};

// The XML document is retained only inside the opt-in libxml2 backend. A
// caller receives its validated descriptor through FomValidationResult, never
// an XML object that could escape into the RTI's public binding.
struct LibXml2ValidatedFomDocument {
  std::unique_ptr<xmlDoc, LibXml2DocumentDeleter> document;
  PrevalidatedFomModule module;
};

// Opens one explicitly selected local FOM/MIM source, rejects external XML
// resources and DTD declarations, validates it against the explicitly
// selected local schema, and retains the validated XML tree (including any
// explicitly selected source-compatibility normalization) for a private
// caller such as the module-composition preflight.
[[nodiscard]] FomValidationResult loadValidatedLibXml2FomDocument(
    FomValidationRequest const& request,
    LibXml2ValidatedFomDocument& destination);

}  // namespace umbra::detail
