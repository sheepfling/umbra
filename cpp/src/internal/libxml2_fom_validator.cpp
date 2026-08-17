#include "internal/libxml2_fom_validator.hpp"

#include "internal/libxml2_fom_document.hpp"

namespace umbra::detail {

FomValidationResult LibXml2FomValidator::validate(FomValidationRequest const& request) const {
  LibXml2ValidatedFomDocument document;
  return loadValidatedLibXml2FomDocument(request, document);
}

}  // namespace umbra::detail
