#include "internal/libxml2_fom_document.hpp"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlschemas.h>

#include <cstdint>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace umbra::detail {
namespace {

constexpr char kHla2025Namespace[] = "http://standards.ieee.org/IEEE1516-2025";

struct ParserContextDeleter {
  void operator()(xmlParserCtxt* context) const noexcept {
    xmlFreeParserCtxt(context);
  }
};

struct SchemaParserContextDeleter {
  void operator()(xmlSchemaParserCtxt* context) const noexcept {
    xmlSchemaFreeParserCtxt(context);
  }
};

struct SchemaDeleter {
  void operator()(xmlSchema* schema) const noexcept {
    xmlSchemaFree(schema);
  }
};

struct SchemaValidationContextDeleter {
  void operator()(xmlSchemaValidCtxt* context) const noexcept {
    xmlSchemaFreeValidCtxt(context);
  }
};

class ErrorCollector final {
 public:
  static void collect(void* userData, xmlError const* error) {
    if (userData == nullptr || error == nullptr || error->message == nullptr) {
      return;
    }

    auto* collector = static_cast<ErrorCollector*>(userData);
    std::string message(error->message);
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
      message.pop_back();
    }
    if (message.empty()) {
      return;
    }
    if (!collector->message_.empty()) {
      collector->message_ += '\n';
    }
    collector->message_ += message;
  }

  [[nodiscard]] std::string const& message() const noexcept {
    return message_;
  }

 private:
  std::string message_;
};

xmlParserErrors rejectExternalXmlResource(
    void*,
    char const*,
    char const*,
    xmlResourceType,
    xmlParserInputFlags,
    xmlParserInput**) {
  return XML_IO_LOAD_ERROR;
}

std::string pathAsUtf8(std::filesystem::path const& path) {
  auto const encoded = path.u8string();
  std::string result;
  result.reserve(encoded.size());
  for (auto const byte : encoded) {
    result.push_back(static_cast<char>(byte));
  }
  return result;
}

FomValidationResult failure(FomValidationStatus status, std::string diagnostics) {
  return {status, std::nullopt, std::move(diagnostics)};
}

FomValidationResult resolveRegularFile(
    std::filesystem::path const& source,
    std::filesystem::path& canonicalDestination,
    std::string const& role) {
  std::error_code error;
  if (!std::filesystem::is_regular_file(source, error)) {
    if (error) {
      if (error == std::errc::no_such_file_or_directory ||
          error == std::errc::not_a_directory) {
        return failure(FomValidationStatus::source_not_found, "Cannot find " + role + " source.");
      }
      return failure(
          FomValidationStatus::source_unreadable,
          "Cannot inspect " + role + " source: " + error.message());
    }
    return failure(FomValidationStatus::source_not_found, "Cannot find " + role + " source.");
  }

  canonicalDestination = std::filesystem::canonical(source, error);
  if (error) {
    return failure(
        FomValidationStatus::source_unreadable,
        "Cannot canonicalize " + role + " source: " + error.message());
  }
  return {FomValidationStatus::valid, std::nullopt, {}};
}

FomValidationResult readLocalXmlSource(
    std::filesystem::path const& source,
    std::vector<char>& contents) {
  std::error_code error;
  auto const byteCount = std::filesystem::file_size(source, error);
  if (error) {
    return failure(
        FomValidationStatus::source_unreadable,
        "Cannot determine FOM source size: " + error.message());
  }
  if (byteCount > static_cast<std::uintmax_t>(std::numeric_limits<int>::max())) {
    return failure(FomValidationStatus::source_unreadable, "The FOM source is too large to parse.");
  }

  std::ifstream input(source, std::ios::binary);
  if (!input) {
    return failure(FomValidationStatus::source_unreadable, "Cannot open the FOM source for reading.");
  }
  contents.resize(static_cast<std::size_t>(byteCount));
  if (!contents.empty()) {
    input.read(contents.data(), static_cast<std::streamsize>(contents.size()));
  }
  if (!input && !input.eof()) {
    return failure(FomValidationStatus::source_unreadable, "Cannot read the complete FOM source.");
  }
  if (input.gcount() != static_cast<std::streamsize>(contents.size())) {
    return failure(FomValidationStatus::source_unreadable, "Cannot read the complete FOM source.");
  }
  return {FomValidationStatus::valid, std::nullopt, {}};
}

bool hasExpectedRoot(xmlDoc const* document) {
  xmlNode const* root = xmlDocGetRootElement(const_cast<xmlDoc*>(document));
  return root != nullptr && xmlStrEqual(root->name, BAD_CAST "objectModel") != 0 &&
         root->ns != nullptr && root->ns->href != nullptr &&
         xmlStrEqual(root->ns->href, BAD_CAST kHla2025Namespace) != 0;
}

}  // namespace

void LibXml2DocumentDeleter::operator()(xmlDoc* document) const noexcept {
  xmlFreeDoc(document);
}

FomValidationResult loadValidatedLibXml2FomDocument(
    FomValidationRequest const& request,
    LibXml2ValidatedFomDocument& destination) {
  destination = {};

  std::filesystem::path canonicalSource;
  if (auto const result = resolveRegularFile(request.sourcePath, canonicalSource, "FOM");
      result.status != FomValidationStatus::valid) {
    return result;
  }

  std::filesystem::path canonicalSchema;
  if (auto const result = resolveRegularFile(request.schemaPath, canonicalSchema, "schema");
      result.status != FomValidationStatus::valid) {
    return failure(
        FomValidationStatus::validator_failure,
        result.diagnostics.empty() ? "The selected schema is unavailable." : result.diagnostics);
  }

  std::vector<char> sourceBytes;
  if (auto const result = readLocalXmlSource(canonicalSource, sourceBytes);
      result.status != FomValidationStatus::valid) {
    return result;
  }

  ErrorCollector parseErrors;
  std::unique_ptr<xmlParserCtxt, ParserContextDeleter> parser(xmlNewParserCtxt());
  if (!parser) {
    return failure(FomValidationStatus::validator_failure, "Cannot allocate an XML parser context.");
  }
  xmlCtxtSetErrorHandler(parser.get(), ErrorCollector::collect, &parseErrors);
  xmlCtxtSetResourceLoader(parser.get(), rejectExternalXmlResource, nullptr);
  std::unique_ptr<xmlDoc, LibXml2DocumentDeleter> document(xmlCtxtReadMemory(
      parser.get(),
      sourceBytes.empty() ? "" : sourceBytes.data(),
      static_cast<int>(sourceBytes.size()),
      pathAsUtf8(canonicalSource).c_str(),
      nullptr,
      XML_PARSE_NONET | XML_PARSE_NO_XXE | XML_PARSE_NOERROR | XML_PARSE_NOWARNING));
  if (!document) {
    return failure(
        FomValidationStatus::source_parse_error,
        parseErrors.message().empty() ? "The FOM XML document cannot be parsed." : parseErrors.message());
  }

  if (xmlGetIntSubset(document.get()) != nullptr || document->extSubset != nullptr) {
    return failure(FomValidationStatus::invalid_model, "DTD declarations are not permitted in an Umbra FOM.");
  }
  if (!hasExpectedRoot(document.get())) {
    return failure(
        FomValidationStatus::invalid_model,
        "The FOM root must be objectModel in the IEEE 1516-2025 namespace.");
  }

  ErrorCollector schemaErrors;
  std::unique_ptr<xmlSchemaParserCtxt, SchemaParserContextDeleter> schemaParser(
      xmlSchemaNewParserCtxt(pathAsUtf8(canonicalSchema).c_str()));
  if (!schemaParser) {
    return failure(FomValidationStatus::validator_failure, "Cannot allocate an XML schema parser context.");
  }
  xmlSchemaSetParserStructuredErrors(schemaParser.get(), ErrorCollector::collect, &schemaErrors);
  std::unique_ptr<xmlSchema, SchemaDeleter> schema(xmlSchemaParse(schemaParser.get()));
  if (!schema) {
    return failure(
        FomValidationStatus::validator_failure,
        schemaErrors.message().empty() ? "The selected XML schema cannot be parsed."
                                       : schemaErrors.message());
  }

  std::unique_ptr<xmlSchemaValidCtxt, SchemaValidationContextDeleter> validationContext(
      xmlSchemaNewValidCtxt(schema.get()));
  if (!validationContext) {
    return failure(FomValidationStatus::validator_failure, "Cannot allocate an XML schema validation context.");
  }
  xmlSchemaSetValidStructuredErrors(
      validationContext.get(), ErrorCollector::collect, &schemaErrors);
  int const validationResult = xmlSchemaValidateDoc(validationContext.get(), document.get());
  if (validationResult != 0) {
    return failure(
        validationResult > 0 ? FomValidationStatus::invalid_model
                             : FomValidationStatus::validator_failure,
        schemaErrors.message().empty() ? "The FOM does not validate against the selected schema."
                                       : schemaErrors.message());
  }

  std::wstring designator = request.designator;
  if (designator.empty()) {
    // Standalone validator callers may omit a designator. Federation-service
    // coordination must always populate it from the exact API argument.
    designator = canonicalSource.generic_wstring();
  }
  std::wstring schemaDesignator = request.schemaDesignator;
  if (schemaDesignator.empty()) {
    schemaDesignator = canonicalSchema.filename().wstring();
  }

  destination.module = {
      std::move(designator),
      canonicalSource,
      canonicalSchema,
      request.moduleKind,
      std::move(schemaDesignator),
  };
  destination.document = std::move(document);
  return {FomValidationStatus::valid, destination.module, {}};
}

}  // namespace umbra::detail
