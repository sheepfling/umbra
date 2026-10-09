#include "internal/fom/libxml2_fom_composer_semantics.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/fom/hla_names.hpp"

#include <libxml/valid.h>

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <exception>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace umbra::detail {
namespace {

bool asciiCaseInsensitiveEquals(std::string_view value, std::string_view expected) {
  if (value.size() != expected.size()) {
    return false;
  }
  for (std::size_t index = 0; index < value.size(); ++index) {
    auto const left = static_cast<unsigned char>(value[index]);
    auto const right = static_cast<unsigned char>(expected[index]);
    if (std::tolower(left) != std::tolower(right)) {
      return false;
    }
  }
  return true;
}

bool hasAsciiCaseInsensitivePrefix(std::string_view value, std::string_view prefix) {
  if (value.size() < prefix.size()) {
    return false;
  }
  return asciiCaseInsensitiveEquals(value.substr(0, prefix.size()), prefix);
}

bool isHlaNamedDeclaration(std::string_view localName) {
  using namespace fom_schema::element;
  return localName == object_class || localName == interaction_class ||
         localName == attribute || localName == parameter ||
         dataTypeKindForElement(localName).has_value() || localName == dimension ||
         localName == transportation || localName == synchronization_point ||
         localName == field || localName == alternative || localName == update_rate ||
         localName == enumerator;
}

bool validateHlaNamePart(
    std::string_view value,
    std::string const &path,
    std::string_view valueKind,
    bool allowReservedHlaNames,
    bool allowNaMarker,
    std::string &diagnostics) {
  if (value.empty()) {
    diagnostics = "HLA " + std::string(valueKind) + " at " + path +
                  " must contain a non-empty XML name.";
    return false;
  }

  // XML NCName is the authoritative character/leading-character rule.  The
  // HLA 3.3.1 convention is stricter than XML in two ways below (periods are
  // reserved for qualified class paths and colons are discouraged/reserved).
  std::string const ownedValue(value);
  if (xmlValidateNCName(
          reinterpret_cast<xmlChar const *>(ownedValue.c_str()),
          0) != 0) {
    diagnostics = "HLA " + std::string(valueKind) + " " +
                  quoteDiagnosticString(value) + " at " + path +
                  " is not a valid XML name under HLA 3.3.1.";
    return false;
  }
  if (value.find('.') != std::string_view::npos) {
    diagnostics = "HLA " + std::string(valueKind) + " " +
                  quoteDiagnosticString(value) + " at " + path +
                  " cannot contain a period; periods are reserved for qualified class names.";
    return false;
  }
  if (asciiCaseInsensitiveEquals(value, fom_schema::value::not_applicable_name) && !allowNaMarker) {
    diagnostics = "HLA " + std::string(valueKind) + " " +
                  quoteDiagnosticString(value) + " at " + path +
                  " is reserved for the NA marker and cannot be a user-defined name.";
    return false;
  }
  if (hasAsciiCaseInsensitivePrefix(value, fom_schema::value::reserved_hla_prefix) &&
      !allowReservedHlaNames) {
    diagnostics = "HLA " + std::string(valueKind) + " " +
                  quoteDiagnosticString(value) + " at " + path +
                  " uses the reserved HLA prefix and cannot be a user-defined name.";
    return false;
  }
  return true;
}

bool validateQualifiedHlaName(
    std::string_view value,
    std::string const &path,
    std::string_view valueKind,
    bool allowReservedHlaNames,
    std::set<std::string> const *knownReservedHlaNames,
    std::string &diagnostics) {
  std::size_t segmentStart = 0;
  while (segmentStart <= value.size()) {
    auto const separator = value.find('.', segmentStart);
    auto const segment = value.substr(
        segmentStart,
        separator == std::string_view::npos ? value.size() - segmentStart
                                             : separator - segmentStart);
    // A qualified class path may use the standard HLA root (and, for the
    // MIM, other standard HLA identifiers), while each user-defined segment
    // still follows the ordinary NCName restrictions.
    bool const segmentAllowsReserved =
        allowReservedHlaNames ||
        (knownReservedHlaNames != nullptr && knownReservedHlaNames->contains(std::string(segment))) ||
        segment == umbra::detail::hla::utf8::fom::object_root ||
        segment == umbra::detail::hla::utf8::fom::interaction_root;
    if (!validateHlaNamePart(
            segment,
            path,
            valueKind,
            segmentAllowsReserved,
            false,
            diagnostics)) {
      return false;
    }
    if (separator == std::string_view::npos) {
      return true;
    }
    segmentStart = separator + 1U;
  }
  return false;
}

bool validateHlaNames(
    SemanticNode const &root,
    FomModuleKind moduleKind,
    std::set<std::string> const *knownReservedHlaNames,
    std::string &diagnostics) {
  // Individual DIF modules are intentionally allowed to carry standard HLA
  // names that are declared by a separately supplied MIM.  The merged pass
  // below supplies the MIM-derived set and rejects an unknown HLA-prefixed
  // user name.  A standalone MIM is, by definition, the source of those
  // reserved names.
  bool const allowReservedHlaNames =
      moduleKind == FomModuleKind::mim || knownReservedHlaNames == nullptr;
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid) {
      return;
    }
    if (node.localName == fom_schema::element::directed_interaction) {
      auto const *name = firstChildNamed(node, fom_schema::element::name);
      if (name == nullptr) {
        return;
      }
      valid = validateQualifiedHlaName(
          name->text,
          childPath(path, *name),
          "directed-interaction name",
          allowReservedHlaNames,
          knownReservedHlaNames,
          diagnostics);
      return;
    }
    if (isHlaNamedDeclaration(node.localName)) {
      auto const *name = firstChildNamed(node, fom_schema::element::name);
      if (name == nullptr) {
        return;
      }
      bool const isStandardRoot =
          name->text == umbra::detail::hla::utf8::fom::object_root ||
          name->text == umbra::detail::hla::utf8::fom::interaction_root;
      valid = validateHlaNamePart(
          name->text,
          childPath(path, *name),
          node.localName == fom_schema::element::field ? "field name" : "name",
          allowReservedHlaNames || isStandardRoot ||
              (knownReservedHlaNames != nullptr && knownReservedHlaNames->contains(name->text)),
          node.localName == fom_schema::element::enumerator,
          diagnostics);
      return;
    }
    if (node.localName == fom_schema::element::note) {
      auto const *label = firstChildNamed(node, fom_schema::element::label);
      if (label == nullptr) {
        return;
      }
      valid = validateHlaNamePart(
          label->text,
          childPath(path, *label),
          "note label",
          allowReservedHlaNames,
          false,
          diagnostics);
    }
  });
  return valid;
}

// IEEE 1516.2-2025 Table 1 specifies the object-model modification date as
// the lexical form YYYY-MM-DD.  The DIF schema intentionally uses xs:date,
// which also admits an optional timezone suffix.  Keep the schema responsible
// for calendar validity, but enforce the stricter OMT presentation here so a
// schema-valid value such as 2025-02-10Z does not silently enter a composed
// FDD.  DIF modules may be incomplete, so an omitted date remains acceptable
// until a completed OMT/FDD supplies one.
bool validModificationDateLexical(std::string_view value) {
  if (value.size() != 10U || value[4] != '-' || value[7] != '-') {
    return false;
  }
  for (std::size_t index = 0; index < value.size(); ++index) {
    if (index == 4U || index == 7U) {
      continue;
    }
    auto const character = static_cast<unsigned char>(value[index]);
    if (character < static_cast<unsigned char>('0') ||
        character > static_cast<unsigned char>('9')) {
      return false;
    }
  }
  return true;
}

bool validateModificationDate(
    xmlNode const *modelIdentification,
    std::string &diagnostics) {
  if (modelIdentification == nullptr) {
    return true;
  }
  std::string const value = directChildText(modelIdentification, fom_schema::element::modification_date);
  if (value.empty() || validModificationDateLexical(value)) {
    return true;
  }
  diagnostics = "Object-model modification date " + quoteDiagnosticString(value) +
                " at objectModel/modelIdentification/modificationDate must use YYYY-MM-DD format.";
  return false;
}

void collectReservedHlaNames(
    SemanticNode const &root,
    std::set<std::string> &reservedNames) {
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &) {
    auto collect = [&](std::string_view value) {
      std::size_t segmentStart = 0;
      while (segmentStart <= value.size()) {
        auto const separator = value.find('.', segmentStart);
        auto const segment = value.substr(
            segmentStart,
            separator == std::string_view::npos ? value.size() - segmentStart
                                                 : separator - segmentStart);
        if (hasAsciiCaseInsensitivePrefix(segment, fom_schema::value::reserved_hla_prefix)) {
          reservedNames.emplace(segment);
        }
        if (separator == std::string_view::npos) {
          break;
        }
        segmentStart = separator + 1U;
      }
    };

    if (node.localName == fom_schema::element::directed_interaction) {
      if (auto const *name = firstChildNamed(node, fom_schema::element::name); name != nullptr) {
        collect(name->text);
      }
    } else if (isHlaNamedDeclaration(node.localName)) {
      if (auto const *name = firstChildNamed(node, fom_schema::element::name); name != nullptr) {
        collect(name->text);
      }
    } else if (node.localName == fom_schema::element::note) {
      if (auto const *label = firstChildNamed(node, fom_schema::element::label); label != nullptr) {
        collect(label->text);
      }
    }
  });
}

DataTypeDeclarationKinds dataTypeDeclarationKinds(SemanticNode const &root) {
  DataTypeDeclarationKinds kinds;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &) {
    auto const kind = dataTypeKindForElement(node.localName);
    if (!kind.has_value()) {
      return;
    }
    std::string const name = identityValue(node, fom_schema::identity::name_prefix);
    if (!name.empty()) {
      kinds.insert_or_assign(name, *kind);
    }
  });
  return kinds;
}

bool validateDataTypeKinds(SemanticNode const &root, std::string &diagnostics) {
  std::map<std::string, std::pair<DataTypeKind, std::string>> seen;
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    auto const kind = dataTypeKindForElement(node.localName);
    if (!valid || !kind.has_value()) {
      return;
    }
    std::string const name = identityValue(node, fom_schema::identity::name_prefix);
    if (name.empty()) {
      return;
    }
    auto const [existing, inserted] = seen.emplace(name, std::make_pair(*kind, path));
    if (!inserted && existing->second.first != *kind) {
      diagnostics = "Data type " + quoteDiagnosticString(name) + " has incompatible definitions at " +
                    existing->second.second + " and " + path + ".";
      valid = false;
    }
  });
  return valid;
}

bool validateDataTypeReferences(SemanticNode const &root, std::string &diagnostics) {
  std::set<std::string> declaredNames;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &) {
    // The 2025 OMT schema's dataTypeKey includes every data-type family,
    // including basicData. Keep complete-model name resolution aligned with
    // that key; narrower table-specific predicates below decide whether a
    // resolved basic-data name is valid for an individual table column.
    if (!isDataTypeDeclaration(node.localName)) {
      return;
    }
    std::string const name = identityValue(node, fom_schema::identity::name_prefix);
    if (!name.empty()) {
      declaredNames.insert(name);
    }
  });

  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid || node.localName != fom_schema::element::data_type || node.text.empty() ||
        node.text == fom_schema::value::not_applicable) {
      return;
    }
    if (!declaredNames.contains(node.text)) {
      diagnostics = "Data type reference " + quoteDiagnosticString(node.text) + " at " + path +
                    " is not declared in the composed model.";
      valid = false;
    }
  });
  return valid;
}

bool isObjectAttributeOrInteractionParameterDataTypeKind(DataTypeKind kind) {
  return kind != DataTypeKind::basic;
}

bool validateObjectAttributeAndInteractionParameterDataTypeKinds(
    SemanticNode const &root,
    std::string &diagnostics) {
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid || (node.localName != fom_schema::element::attribute &&
                   node.localName != fom_schema::element::parameter)) {
      return;
    }
    std::string const dataType = scalarChildValue(node, fom_schema::element::data_type);
    if (dataType.empty() || dataType == fom_schema::value::not_applicable) {
      return;
    }
    auto const kind = kinds.find(dataType);
    // The general data-type resolver runs first and reports a missing name.
    // This table-specific rule classifies an already declared name. HLAtoken
    // requires no name special case: the standard MIM declares it as arrayData.
    if (kind == kinds.end() || isObjectAttributeOrInteractionParameterDataTypeKind(kind->second)) {
      return;
    }
    std::string const owner = node.localName == fom_schema::element::attribute
                                  ? "Object attribute"
                                  : "Interaction parameter";
    diagnostics = owner + " data type " + quoteDiagnosticString(dataType) + " at " + path +
                  " is not permitted; it must name a simple, enumerated, reference, array, "
                  "fixed-record, or variant-record data type (or NA).";
    valid = false;
  });
  return valid;
}

bool validateAttributeNaCompanionFields(SemanticNode const &root, std::string &diagnostics) {
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid || node.localName != fom_schema::element::attribute ||
        scalarChildValue(node, fom_schema::element::data_type) != fom_schema::value::not_applicable) {
      return;
    }

    std::string const attributeName = identityValue(node, fom_schema::identity::name_prefix);
    auto reject = [&](std::string_view fieldName, std::string_view expectation) {
      diagnostics = "Attribute " + quoteDiagnosticString(attributeName) + " at " + path +
                    " has data type NA but its " + std::string(fieldName) + " " +
                    std::string(expectation) + ".";
      valid = false;
    };

    // DIF permits incomplete rows at the module boundary. Check a companion
    // only when the merged row actually supplies it, rather than manufacturing
    // a missing table value. A completed FDD separately requires transportation
    // and order before materialization.
    if (auto const *transportation = firstChildNamed(node, fom_schema::element::transportation);
        transportation != nullptr && transportation->text == fom_schema::value::not_applicable) {
      reject(fom_schema::element::transportation, "must name a non-NA transportation type");
      return;
    }
    if (auto const *order = firstChildNamed(node, fom_schema::element::order);
        order != nullptr && order->text == fom_schema::value::not_applicable) {
      reject(fom_schema::element::order, "must name a non-NA order type");
      return;
    }
    if (auto const *updateType = firstChildNamed(node, fom_schema::element::update_type);
        updateType != nullptr && updateType->text != fom_schema::value::not_applicable) {
      reject("update type", "must be NA");
      return;
    }
    if (auto const *updateCondition = firstChildNamed(node, fom_schema::element::update_condition);
        updateCondition != nullptr && updateCondition->text != fom_schema::value::not_applicable) {
      reject("update condition", "must be NA");
    }
  });
  return valid;
}

bool validateDynamicAttributeUpdateConditions(SemanticNode const &root, std::string &diagnostics) {
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid || node.localName != fom_schema::element::attribute) {
      return;
    }
    auto const *updateType = firstChildNamed(node, fom_schema::element::update_type);
    if (updateType == nullptr ||
        (updateType->text != fom_schema::value::conditional_update &&
         updateType->text != fom_schema::value::periodic_update)) {
      return;
    }
    auto const *updateCondition = firstChildNamed(node, fom_schema::element::update_condition);
    // DIF permits incomplete attribute rows at the module boundary. Once a
    // Conditional or Periodic row supplies its condition, the direct table
    // predicate requires actual text. This deliberately does not interpret
    // the conflicting Static/NA direction or parse a periodic-rate grammar.
    if (updateCondition == nullptr ||
        (!updateCondition->text.empty() &&
         updateCondition->text != fom_schema::value::not_applicable)) {
      return;
    }
    diagnostics = "Attribute " + quotedIdentityValue(node, fom_schema::identity::name_prefix) + " at " + path +
                  " has " + updateType->text +
                  " update type but its supplied update condition must contain non-NA text.";
    valid = false;
  });
  return valid;
}

bool validateUnsharedAttributeValueRequirement(SemanticNode const &root, std::string &diagnostics) {
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid || node.localName != fom_schema::element::attribute) {
      return;
    }
    auto const *sharing = firstChildNamed(node, fom_schema::element::sharing);
    auto const *valueRequired = firstChildNamed(node, fom_schema::element::value_required);
    // DIF permits partial attribute rows. Only apply this Table 10 predicate
    // once the supplied merged row explicitly says the attribute is neither
    // published nor subscribed and also supplies its Value Required field.
    if (sharing == nullptr || sharing->text != fom_schema::value::neither_sharing ||
        valueRequired == nullptr || valueRequired->text == fom_schema::value::false_literal) {
      return;
    }
    diagnostics = "Attribute " + quotedIdentityValue(node, fom_schema::identity::name_prefix) + " at " + path +
                  " is neither published nor subscribed but its value-required field must be false.";
    valid = false;
  });
  return valid;
}

bool validateArrayElementDataTypeKinds(SemanticNode const &root, std::string &diagnostics) {
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid || node.localName != fom_schema::element::array_data) {
      return;
    }
    std::string const dataType = scalarChildValue(node, fom_schema::element::data_type);
    // DIF deliberately permits incomplete rows. Keep an omitted element type
    // and the established NA no-type marker representable; this completed-model
    // rule classifies only a supplied named declaration.
    if (dataType.empty() || dataType == fom_schema::value::not_applicable) {
      return;
    }
    auto const kind = kinds.find(dataType);
    // The general data-type resolver runs first and reports a missing name.
    // Table 35 restricts an array element to a name from another data-type
    // table, which excludes a raw basic-data representation.
    if (kind == kinds.end() || isObjectAttributeOrInteractionParameterDataTypeKind(kind->second)) {
      return;
    }
    diagnostics = "Array data type " +
                  quotedIdentityValue(node, fom_schema::identity::name_prefix) + " element type " +
                  quoteDiagnosticString(dataType) +
                  " at " + path +
                  " is not permitted; it must name a simple, enumerated, reference, array, "
                  "fixed-record, or variant-record data type (or NA).";
    valid = false;
  });
  return valid;
}

bool validateFixedRecordFieldAndVariantRecordAlternativeDataTypeKinds(
    SemanticNode const &root,
    std::string &diagnostics) {
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);
  bool valid = true;
  auto validateMember = [&](SemanticNode const &record,
                            SemanticNode const &member,
                            std::string const &memberPath,
                            std::string_view recordLabel,
                            std::string_view memberLabel) {
    std::string const dataType = scalarChildValue(member, fom_schema::element::data_type);
    // DIF deliberately permits incomplete rows. This completed-model rule
    // classifies only a supplied named declaration.
    if (dataType.empty() || dataType == fom_schema::value::not_applicable) {
      return true;
    }
    auto const kind = kinds.find(dataType);
    // The general resolver runs first and reports an unresolved name. The
    // fixed-record Field Type and variant-record Alternative Type columns
    // accept names from data-type tables, not a raw basic-data representation.
    if (kind == kinds.end() || isObjectAttributeOrInteractionParameterDataTypeKind(kind->second)) {
      return true;
    }
    diagnostics = std::string(recordLabel) + " " +
                  quotedIdentityValue(record, fom_schema::identity::name_prefix) + " " +
                  std::string(memberLabel) + " " +
                  quotedIdentityValue(member, fom_schema::identity::name_prefix) +
                  " data type " + quoteDiagnosticString(dataType) + " at " + memberPath +
                  " is not permitted; it must name a simple, enumerated, reference, array, "
                  "fixed-record, or variant-record data type (or NA).";
    return false;
  };

  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid || (node.localName != fom_schema::element::fixed_record_data &&
                   node.localName != fom_schema::element::variant_record_data)) {
      return;
    }
    std::string_view const memberName =
        node.localName == fom_schema::element::fixed_record_data
            ? fom_schema::element::field
            : fom_schema::element::alternative;
    std::string_view const recordLabel =
        node.localName == fom_schema::element::fixed_record_data
            ? "Fixed-record data type"
            : "Variant-record data type";
    std::string_view const memberLabel =
        node.localName == fom_schema::element::fixed_record_data
            ? fom_schema::element::field
            : fom_schema::element::alternative;
    for (auto const & [key, member] : node.children) {
      (void)key;
      if (member.localName != memberName) {
        continue;
      }
      if (!validateMember(
              node,
              member,
              childPath(path, member),
              recordLabel,
              memberLabel)) {
        valid = false;
        return;
      }
    }
  });
  return valid;
}

bool validateVariantRecordDiscriminantDataTypeKinds(
    SemanticNode const &root,
    std::string &diagnostics) {
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid || node.localName != fom_schema::element::variant_record_data) {
      return;
    }
    auto const *discriminantType = firstChildNamed(node, fom_schema::element::data_type);
    // DIF deliberately permits an incomplete record declaration. Once the
    // discriminant type is supplied, the 2025 variant-record table requires
    // an enumerated-data declaration; unlike several other table columns,
    // that rule provides no NA marker path.
    if (discriminantType == nullptr || discriminantType->text.empty()) {
      return;
    }
    auto const kind = kinds.find(discriminantType->text);
    // The general resolver runs first and reports an unresolved non-NA name.
    // Keep NA here so this narrower predicate can reject it explicitly.
    if (discriminantType->text != fom_schema::value::not_applicable && kind == kinds.end()) {
      return;
    }
    if (kind != kinds.end() && kind->second == DataTypeKind::enumerated) {
      return;
    }
    diagnostics = "Variant-record data type " +
                  quotedIdentityValue(node, fom_schema::identity::name_prefix) +
                  " discriminant type " + quoteDiagnosticString(discriminantType->text) + " at " +
                  childPath(path, *discriminantType) +
                  " is not permitted; it must name an enumerated data type.";
    valid = false;
  });
  return valid;
}

bool validateDimensionInputDataTypeKinds(SemanticNode const &root, std::string &diagnostics) {
  auto const *dimensions = firstChildNamed(root, fom_schema::element::dimensions);
  if (dimensions == nullptr) {
    return true;
  }

  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);
  std::string const dimensionsPath = childPath(fom_schema::element::object_model, *dimensions);
  for (auto const & [key, dimension] : dimensions->children) {
    (void)key;
    if (dimension.localName != fom_schema::element::dimension) {
      continue;
    }
    std::string const dimensionPath = childPath(dimensionsPath, dimension);
    for (auto const & [inputTypesKey, inputDataTypes] : dimension.children) {
      (void)inputTypesKey;
      if (inputDataTypes.localName != fom_schema::element::input_data_types) {
        continue;
      }
      std::string const inputTypesPath = childPath(dimensionPath, inputDataTypes);
      for (auto const & [dataTypeKey, dataType] : inputDataTypes.children) {
        (void)dataTypeKey;
        if (dataType.localName != fom_schema::element::data_type || dataType.text.empty() ||
            dataType.text == fom_schema::value::not_applicable) {
          continue;
        }
        auto const kind = kinds.find(dataType.text);
        // The general resolver runs first and reports a missing name. The
        // Dimension Input data type column refers to the 4.14 data-type
        // tables, so a raw basic-data representation is not a permitted name.
        if (kind == kinds.end() || isObjectAttributeOrInteractionParameterDataTypeKind(kind->second)) {
          continue;
        }
        diagnostics = "Dimension " +
                      quotedIdentityValue(dimension, fom_schema::identity::name_prefix) +
                      " input data type " + quoteDiagnosticString(dataType.text) + " at " +
                      childPath(inputTypesPath, dataType) +
                      " is not permitted; it must name a simple, enumerated, reference, array, "
                      "fixed-record, or variant-record data type (or NA).";
        return false;
      }
    }
  }
  return true;
}

bool validateDimensionInputDataTypeNaExclusivity(
    SemanticNode const &root,
    std::string &diagnostics) {
  auto const *dimensions = firstChildNamed(root, fom_schema::element::dimensions);
  if (dimensions == nullptr) {
    return true;
  }

  std::string const dimensionsPath = childPath(fom_schema::element::object_model, *dimensions);
  for (auto const & [key, dimension] : dimensions->children) {
    (void)key;
    if (dimension.localName != fom_schema::element::dimension) {
      continue;
    }
    auto const *inputDataTypes = firstChildNamed(dimension, fom_schema::element::input_data_types);
    if (inputDataTypes == nullptr) {
      continue;
    }

    bool hasNaMarker = false;
    bool hasNamedInputType = false;
    for (auto const & [inputTypeKey, inputDataType] : inputDataTypes->children) {
      (void)inputTypeKey;
      if (inputDataType.localName != fom_schema::element::data_type || inputDataType.text.empty()) {
        continue;
      }
      if (inputDataType.text == fom_schema::value::not_applicable) {
        hasNaMarker = true;
      } else {
        hasNamedInputType = true;
      }
    }

    // Table 14 uses NA for the mutually exclusive no-suitable-named-type
    // branch. The DIF maps the Input data type column to a sequence, so keep
    // the no-type marker exclusive rather than allowing it beside a chosen
    // table-defined data type.
    if (hasNaMarker && hasNamedInputType) {
      diagnostics = "Dimension " +
                    quotedIdentityValue(dimension, fom_schema::identity::name_prefix) + " at " +
                    childPath(dimensionsPath, dimension) +
                    " mixes the NA input data type marker with named input data types.";
      return false;
    }
  }
  return true;
}

bool validateDimensionInputDataTypeDescriptions(SemanticNode const &root, std::string &diagnostics) {
  auto const *dimensions = firstChildNamed(root, fom_schema::element::dimensions);
  if (dimensions == nullptr) {
    return true;
  }

  std::string const dimensionsPath = childPath(fom_schema::element::object_model, *dimensions);
  for (auto const & [key, dimension] : dimensions->children) {
    (void)key;
    if (dimension.localName != fom_schema::element::dimension) {
      continue;
    }
    auto const *inputDataTypes = firstChildNamed(dimension, fom_schema::element::input_data_types);
    auto const *inputDataDescription =
        firstChildNamed(dimension, fom_schema::element::input_data_description);
    // Both fields are required by the DIF schema for a completed Dimension
    // entry. Preserve defensive behavior if an unchecked SemanticNode reaches
    // this private preflight.
    if (inputDataTypes == nullptr || inputDataDescription == nullptr) {
      continue;
    }

    bool hasNamedInputType = false;
    for (auto const & [inputTypeKey, inputDataType] : inputDataTypes->children) {
      (void)inputTypeKey;
      if (inputDataType.localName == fom_schema::element::data_type &&
          !inputDataType.text.empty() &&
          inputDataType.text != fom_schema::value::not_applicable) {
        hasNamedInputType = true;
        break;
      }
    }
    // Table 14 permits the Input data type cell to be NA only when the third
    // column supplies an unambiguous textual description. In DIF that table
    // cell is a sequence, and the supplied official extension represents NA
    // with an empty inputDataTypes element. Treat that form and the existing
    // explicit dataType=NA marker equivalently, without trying to decide
    // whether a suitable named type exists.
    if (!hasNamedInputType &&
        (inputDataDescription->text.empty() ||
         inputDataDescription->text == fom_schema::value::not_applicable)) {
      diagnostics = "Dimension " +
                    quotedIdentityValue(dimension, fom_schema::identity::name_prefix) + " at " +
                    childPath(dimensionsPath, dimension) +
                    " has no named input data type but its input data description must be non-NA text.";
      return false;
    }
  }
  return true;
}

bool isStandardInstanceIdentifierAttribute(std::string_view name);

bool validateDataTypeRepresentationReferences(
    SemanticNode const &root,
    std::string &diagnostics) {
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);

  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid ||
        (node.localName != fom_schema::element::simple_data &&
         node.localName != fom_schema::element::enumerated_data &&
         node.localName != fom_schema::element::reference_data_type)) {
      return;
    }
    // The two standard instance identifiers are handled by the dedicated
    // special-reference rule below; their implicit attribute rows do not use
    // the ordinary reference-data representation predicate.
    if (node.localName == fom_schema::element::reference_data_type &&
        isStandardInstanceIdentifierAttribute(
            scalarChildValue(node, fom_schema::element::referenced_attribute))) {
      return;
    }
    std::string const representation = scalarChildValue(node, fom_schema::element::representation);
    if (representation.empty()) {
      return;
    }
    auto const kind = kinds.find(representation);
    if (kind == kinds.end()) {
      diagnostics = "Representation " + quoteDiagnosticString(representation) + " at " + path +
                    " is not declared in the composed data-type model.";
      valid = false;
      return;
    }

    if (node.localName == fom_schema::element::reference_data_type &&
        (kind->second == DataTypeKind::basic || kind->second == DataTypeKind::reference)) {
      diagnostics = "Reference data type " +
                    quotedIdentityValue(node, fom_schema::identity::name_prefix) +
                    " representation " + quoteDiagnosticString(representation) + " at " + path +
                    " must name a simple, enumerated, array, fixed-record, or variant-record "
                    "data type.";
      valid = false;
      return;
    }

    if (node.localName == fom_schema::element::enumerated_data &&
        kind->second != DataTypeKind::basic) {
      diagnostics = "Enumerated data type " +
                    quotedIdentityValue(node, fom_schema::identity::name_prefix) +
                    " representation " + quoteDiagnosticString(representation) + " at " + path +
                    " must name a basic-data representation.";
      valid = false;
      return;
    }

    // The 2025 source table describes simple-data representations as
    // basic-data rows, but the official MIM/Restaurant DIF pair uses
    // HLAboolean (an enumerated data type) as a simple-data representation.
    // Keep simple-data representation checking at name resolution under the
    // reviewed RL-009 interpretation. The enumerated-data table has its own
    // unambiguous basic-data predicate above.
  });
  return valid;
}

bool isTimeRepresentationDataTypeKind(DataTypeKind kind) {
  return kind == DataTypeKind::simple || kind == DataTypeKind::enumerated ||
         kind == DataTypeKind::array || kind == DataTypeKind::fixed_record ||
         kind == DataTypeKind::variant_record;
}

bool validateTimeRepresentationDataTypeKinds(SemanticNode const &root, std::string &diagnostics) {
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid || (node.localName != fom_schema::element::logical_time &&
                   node.localName != fom_schema::element::logical_time_interval)) {
      return;
    }
    std::string const dataType = scalarChildValue(node, fom_schema::element::data_type);
    if (dataType.empty() || dataType == fom_schema::value::not_applicable) {
      return;
    }
    auto const kind = kinds.find(dataType);
    // The general data-type resolver runs first and reports a missing name.
    // This narrower 2025 table rule only classifies an already declared name.
    if (kind == kinds.end() || isTimeRepresentationDataTypeKind(kind->second)) {
      return;
    }
    diagnostics = "Time representation data type " + quoteDiagnosticString(dataType) + " at " + path +
                  " is not permitted; it must name a simple, enumerated, array, fixed-record, or "
                  "variant-record data type (or NA).";
    valid = false;
  });
  return valid;
}

bool isTagDataTypeOwner(std::string_view localName) {
  return localName == fom_schema::element::update_reflect_tag ||
         localName == fom_schema::element::send_receive_tag ||
         localName == fom_schema::element::delete_remove_tag ||
         localName == fom_schema::element::divestiture_request_tag ||
         localName == fom_schema::element::divestiture_completion_tag ||
         localName == fom_schema::element::acquisition_request_tag ||
         localName == fom_schema::element::request_update_tag ||
         localName == fom_schema::element::synchronization_point;
}

bool isTagDataTypeKind(DataTypeKind kind) {
  return isObjectAttributeOrInteractionParameterDataTypeKind(kind);
}

bool validateTagDataTypeKinds(SemanticNode const &root, std::string &diagnostics) {
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid || !isTagDataTypeOwner(node.localName)) {
      return;
    }
    std::string const dataType = scalarChildValue(node, fom_schema::element::data_type);
    if (dataType.empty() || dataType == fom_schema::value::not_applicable) {
      return;
    }
    auto const kind = kinds.find(dataType);
    // The general data-type resolver runs first and reports a missing name.
    // This table-specific rule only classifies an already declared name.
    if (kind == kinds.end() || isTagDataTypeKind(kind->second)) {
      return;
    }
    diagnostics = "Tag data type " + quoteDiagnosticString(dataType) + " at " + path +
                  " is not permitted; it must name a simple, enumerated, reference, array, "
                  "fixed-record, or variant-record data type (or NA).";
    valid = false;
  });
  return valid;
}

struct ObjectClassReferenceDefinition {
  std::string parentName;
  std::map<std::string, std::string> declaredAttributeTypes;
};

using ObjectClassReferenceDefinitions = std::map<std::string, ObjectClassReferenceDefinition>;

void collectObjectClassReferenceDefinitions(
    SemanticNode const &parent,
    std::string const &parentName,
    ObjectClassReferenceDefinitions &definitions) {
  for (auto const & [key, child] : parent.children) {
    (void)key;
      if (child.localName != fom_schema::element::object_class) {
      continue;
    }
    std::string const shortName = identityValue(child, fom_schema::identity::name_prefix);
    if (shortName.empty()) {
      continue;
    }
    std::string const qualifiedName =
        parentName.empty() ? shortName : parentName + "." + shortName;
    ObjectClassReferenceDefinition definition{parentName, {}};
    for (auto const & [childKey, attribute] : child.children) {
      (void)childKey;
      if (attribute.localName != fom_schema::element::attribute) {
        continue;
      }
      std::string const attributeName = identityValue(attribute, fom_schema::identity::name_prefix);
      if (!attributeName.empty()) {
        definition.declaredAttributeTypes.emplace(
            attributeName,
            scalarChildValue(attribute, fom_schema::element::data_type));
      }
    }
    definitions.insert_or_assign(qualifiedName, std::move(definition));
    collectObjectClassReferenceDefinitions(child, qualifiedName, definitions);
  }
}

ObjectClassReferenceDefinitions objectClassReferenceDefinitions(SemanticNode const &root) {
  ObjectClassReferenceDefinitions definitions;
  if (auto const *objects = firstChildNamed(root, fom_schema::element::objects); objects != nullptr) {
    collectObjectClassReferenceDefinitions(*objects, {}, definitions);
  }
  return definitions;
}

bool validateReferenceDataTypeClassReferences(SemanticNode const &root, std::string &diagnostics) {
  ObjectClassReferenceDefinitions const classes = objectClassReferenceDefinitions(root);

  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid || node.localName != fom_schema::element::reference_data_type) {
      return;
    }
    std::string const referencedClass = scalarChildValue(node, fom_schema::element::reference_class);
    if (!referencedClass.empty() && !classes.contains(referencedClass)) {
      diagnostics = "Reference data type " +
                    quotedIdentityValue(node, fom_schema::identity::name_prefix) +
                    " refers to object class " + quoteDiagnosticString(referencedClass) + " at " + path +
                    ", but that class is not declared in the composed model.";
      valid = false;
    }
  });
  return valid;
}

bool isStandardInstanceIdentifierAttribute(std::string_view name) {
  return name == umbra::detail::hla::utf8::fom::object_instance_name || name == umbra::detail::hla::utf8::fom::object_instance_handle;
}

std::optional<std::string> referencedAttributeDataType(
    ObjectClassReferenceDefinitions const &classes,
    std::string className,
    std::string const &attributeName) {
  while (!className.empty()) {
    auto const classDefinition = classes.find(className);
    if (classDefinition == classes.end()) {
      return std::nullopt;
    }
    auto const attribute = classDefinition->second.declaredAttributeTypes.find(attributeName);
    if (attribute != classDefinition->second.declaredAttributeTypes.end()) {
      return attribute->second;
    }
    className = classDefinition->second.parentName;
  }
  return std::nullopt;
}

bool validateReferenceDataTypeAttributeReferences(SemanticNode const &root, std::string &diagnostics) {
  ObjectClassReferenceDefinitions const classes = objectClassReferenceDefinitions(root);
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);

  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid || node.localName != fom_schema::element::reference_data_type) {
      return;
    }
    std::string const referencedClass = scalarChildValue(node, fom_schema::element::reference_class);
    std::string const referencedAttribute =
        scalarChildValue(node, fom_schema::element::referenced_attribute);
    if (referencedClass.empty() || referencedAttribute.empty()) {
      return;
    }
    if (isStandardInstanceIdentifierAttribute(referencedAttribute)) {
      // IEEE 1516.2 gives the two standard instance identifiers special
      // semantics rather than requiring an ordinary attribute row. Preserve
      // that exception, but still enforce their standardized representations.
      std::string const expectedRepresentation =
          referencedAttribute == umbra::detail::hla::utf8::fom::object_instance_name ? umbra::detail::hla::utf8::fom::unicode_string
                                                            : umbra::detail::hla::utf8::fom::object_instance_handle;
      if (scalarChildValue(node, fom_schema::element::representation) != expectedRepresentation) {
        diagnostics = "Reference data type " +
                      quotedIdentityValue(node, fom_schema::identity::name_prefix) +
                      " representation must be " + quoteDiagnosticString(expectedRepresentation) +
                      " for " + quoteDiagnosticString(referencedAttribute) + " at " + path + ".";
        valid = false;
        return;
      }
      auto const representation = kinds.find(expectedRepresentation);
      if (representation == kinds.end()) {
        diagnostics = "Standard instance identifier representation " +
                      quoteDiagnosticString(expectedRepresentation) + " for " +
                      quoteDiagnosticString(referencedAttribute) + " at " + path +
                      " is not declared in the composed data-type model.";
        valid = false;
      }
      return;
    }
    auto const attributeType =
        referencedAttributeDataType(classes, referencedClass, referencedAttribute);
    if (!attributeType.has_value()) {
      diagnostics = "Reference data type " +
                    quotedIdentityValue(node, fom_schema::identity::name_prefix) +
                    " refers to attribute " + quoteDiagnosticString(referencedAttribute) +
                    " of object class " + quoteDiagnosticString(referencedClass) + " at " + path +
                    ", but that attribute is not declared by the class or an ancestor in the composed model.";
      valid = false;
      return;
    }
    std::string const representation = scalarChildValue(node, fom_schema::element::representation);
    if (!representation.empty() && representation != *attributeType) {
      diagnostics = "Reference data type " +
                    quotedIdentityValue(node, fom_schema::identity::name_prefix) +
                    " representation " + quoteDiagnosticString(representation) +
                    " does not match referenced attribute " + quoteDiagnosticString(referencedAttribute) +
                    " data type " + quoteDiagnosticString(*attributeType) + " at " + path + ".";
      valid = false;
    }
  });
  return valid;
}

void collectInteractionClassNames(
    SemanticNode const &parent,
    std::string const &parentName,
    std::set<std::string> &names) {
  for (auto const & [key, child] : parent.children) {
    (void)key;
    if (child.localName != fom_schema::element::interaction_class) {
      continue;
    }
    std::string const shortName = identityValue(child, fom_schema::identity::name_prefix);
    if (shortName.empty()) {
      continue;
    }
    std::string const qualifiedName =
        parentName.empty() ? shortName : parentName + "." + shortName;
    names.insert(qualifiedName);
    collectInteractionClassNames(child, qualifiedName, names);
  }
}

bool validateDirectedInteractionReferences(SemanticNode const &root, std::string &diagnostics) {
  std::set<std::string> interactionNames;
  if (auto const *interactions = firstChildNamed(root, fom_schema::element::interactions);
      interactions != nullptr) {
    collectInteractionClassNames(*interactions, {}, interactionNames);
  }

  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid || node.localName != fom_schema::element::directed_interaction) {
      return;
    }
    std::string const interactionName = scalarChildValue(node, fom_schema::element::name);
    if (!interactionName.empty() && !interactionNames.contains(interactionName)) {
      diagnostics = "Directed interaction " + quoteDiagnosticString(interactionName) + " at " + path +
                    " is not declared in the composed interaction hierarchy.";
      valid = false;
    }
  });
  return valid;
}

bool validateAvailableDimensionReferences(SemanticNode const &root, std::string &diagnostics) {
  std::set<std::string> dimensionNames;
  if (auto const *dimensions = firstChildNamed(root, fom_schema::element::dimensions);
      dimensions != nullptr) {
    for (auto const & [key, dimension] : dimensions->children) {
      (void)key;
      if (dimension.localName != fom_schema::element::dimension) {
        continue;
      }
      std::string const name = identityValue(dimension, fom_schema::identity::name_prefix);
      if (!name.empty()) {
        dimensionNames.insert(name);
      }
    }
  }

  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const &node, std::string const &path) {
    if (!valid || node.localName != fom_schema::element::dimensions) {
      return;
    }
    for (auto const & [key, dimension] : node.children) {
      (void)key;
      // IEEE 1516.2-2025's OMT dimensionRef keyref selects only the scalar
      // dimension values below object and interaction classes. Top-level
      // dimension declarations carry a name identity and are not references.
      if (dimension.localName != fom_schema::element::dimension || !dimension.identity.empty() ||
          dimension.text.empty()) {
        continue;
      }
      if (!dimensionNames.contains(dimension.text)) {
        diagnostics = "Available dimension " + quoteDiagnosticString(dimension.text) + " at " +
                      childPath(path, dimension) +
                      " is not declared in the composed dimension table.";
        valid = false;
        return;
      }
    }
  });
  return valid;
}


struct DimensionValueRange {
  unsigned long lower = 0;
  unsigned long upper = 0;
};

std::optional<unsigned long> parseDimensionValueInteger(std::string_view value) {
  if (value.empty()) {
    return std::nullopt;
  }
  unsigned long parsed = 0;
  auto const result = std::from_chars(value.data(), value.data() + value.size(), parsed, 10);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) {
    return std::nullopt;
  }
  return parsed;
}

std::optional<DimensionValueRange> parseDimensionValueRange(
    std::string_view value,
    unsigned long dimensionUpperBound) {
  if (value == fom_schema::value::excluded_dimension_value) {
    return DimensionValueRange{};
  }

  if (auto const point = parseDimensionValueInteger(value); point.has_value()) {
    if (*point == std::numeric_limits<unsigned long>::max()) {
      return std::nullopt;
    }
    return DimensionValueRange{*point, *point + 1};
  }

  if (value.size() < 3 || value.front() != '[' || value.back() != ')') {
    return std::nullopt;
  }
  std::string_view body = value.substr(1, value.size() - 2);
  auto const separator = body.find("..");
  auto const lower = parseDimensionValueInteger(
      separator == std::string_view::npos ? body : body.substr(0, separator));
  if (!lower.has_value()) {
    return std::nullopt;
  }
  if (separator == std::string_view::npos) {
    return DimensionValueRange{*lower, dimensionUpperBound};
  }
  auto const upper = parseDimensionValueInteger(body.substr(separator + 2));
  if (!upper.has_value()) {
    return std::nullopt;
  }
  return DimensionValueRange{*lower, *upper};
}

bool validateDimensionDefaultValues(SemanticNode const& root, std::string& diagnostics) {
  auto const* dimensions = firstChildNamed(root, fom_schema::element::dimensions);
  if (dimensions == nullptr) {
    return true;
  }

  bool valid = true;
  for (auto const& [key, dimension] : dimensions->children) {
    (void)key;
    if (!valid || dimension.localName != fom_schema::element::dimension) {
      continue;
    }
    std::string const value = scalarChildValue(dimension, fom_schema::element::value);
    if (value.empty() || value == fom_schema::value::excluded_dimension_value) {
      continue;
    }

    std::string const upperBoundText = scalarChildValue(dimension, fom_schema::element::upper_bound);
    auto const upperBound = parseDimensionValueInteger(upperBoundText);
    auto const path = childPath("objectModel/dimensions", dimension);
    if (!upperBound.has_value() || *upperBound == 0) {
      diagnostics = "Dimension " + quotedIdentityValue(dimension, fom_schema::identity::name_prefix) + " at " + path +
                    " supplies a default value but no positive upper bound.";
      valid = false;
      continue;
    }

    auto const range = parseDimensionValueRange(value, *upperBound);
    if (!range.has_value() || range->lower >= range->upper ||
        range->upper > *upperBound) {
      diagnostics = "Dimension " + quotedIdentityValue(dimension, fom_schema::identity::name_prefix) + " value " +
                    quoteDiagnosticString(value) +
                    " at " + path + " must be a nonnegative integer subrange of [0, " +
                    upperBoundText + ").";
      valid = false;
    }
  }
  return valid;
}

bool validateUpdateRateValues(SemanticNode const& root, std::string& diagnostics) {
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != fom_schema::element::update_rate) {
      return;
    }
    std::string const rateText = scalarChildValue(node, fom_schema::element::rate);
    if (rateText.empty()) {
      // DIF permits an incomplete update-rate row; do not invent a value.
      return;
    }
    try {
      std::size_t consumed = 0;
      double const rate = std::stod(rateText, &consumed);
      if (consumed != rateText.size() || !std::isfinite(rate) || rate <= 0.0) {
        diagnostics = "Update rate " + quotedIdentityValue(node, fom_schema::identity::name_prefix) + " at " + path +
                      " must be a decimal value greater than zero.";
        valid = false;
      }
    } catch (std::exception const&) {
      diagnostics = "Update rate " + quotedIdentityValue(node, fom_schema::identity::name_prefix) + " at " + path +
                    " must be a decimal value greater than zero.";
      valid = false;
    }
  });
  return valid;
}

bool validateTransportationReferences(SemanticNode const& root, std::string& diagnostics) {
  std::set<std::string> transportationNames;
  if (auto const* transportations = firstChildNamed(root, fom_schema::element::transportations);
      transportations != nullptr) {
    for (auto const& [key, transportation] : transportations->children) {
      (void)key;
      if (transportation.localName != fom_schema::element::transportation) {
        continue;
      }
      std::string const name = identityValue(transportation, fom_schema::identity::name_prefix);
      if (!name.empty()) {
        transportationNames.insert(name);
      }
    }
  }

  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const& node, std::string const& path) {
    // IEEE 1516.2-2025 OMT's transportationRef keyref selects attributes and
    // interaction classes, whose transportation fields resolve through the
    // completed top-level transportation table.
    if (!valid || (node.localName != fom_schema::element::attribute &&
                   node.localName != fom_schema::element::interaction_class)) {
      return;
    }
    std::string const transportation = scalarChildValue(node, fom_schema::element::transportation);
    if (!transportation.empty() && !transportationNames.contains(transportation)) {
      diagnostics = "Transportation " + quoteDiagnosticString(transportation) + " at " + path +
                    " is not declared in the composed transportation table.";
      valid = false;
    }
  });
  return valid;
}

bool validateStandardRootClassHierarchies(SemanticNode const& root, std::string& diagnostics) {
  auto validate = [&](std::string_view sectionName,
                      std::string_view classElementName,
                      std::string_view requiredRootName,
                      std::string_view hierarchyKind) {
    auto const* section = firstChildNamed(root, sectionName);
    if (section == nullptr) {
      // DIF permits incomplete modules. A completed model that supplies this
      // table must, however, express its classes under the standard root.
      return true;
    }

    for (auto const& [key, classDefinition] : section->children) {
      (void)key;
      if (classDefinition.localName != classElementName) {
        continue;
      }
      std::string const name = identityValue(classDefinition, fom_schema::identity::name_prefix);
      if (name == requiredRootName) {
        continue;
      }
      diagnostics = std::string(hierarchyKind) + " class " + quoteDiagnosticString(name) + " at " +
                    childPath(
                        std::string(fom_schema::element::object_model) + "/" + std::string(sectionName),
                        classDefinition) +
                    " is not nested below " + quoteDiagnosticString(requiredRootName) +
                    "; the completed hierarchy must be rooted by " +
                    quoteDiagnosticString(requiredRootName) + ".";
      return false;
    }
    return true;
  };

  return validate(
             fom_schema::element::objects,
             fom_schema::element::object_class,
             umbra::detail::hla::utf8::fom::object_root,
             "Object") &&
         validate(
             fom_schema::element::interactions,
             fom_schema::element::interaction_class,
             umbra::detail::hla::utf8::fom::interaction_root,
             "Interaction");
}

bool validateInheritedObjectClassAttributeNames(SemanticNode const& root, std::string& diagnostics) {
  ObjectClassReferenceDefinitions const classes = objectClassReferenceDefinitions(root);
  for (auto const& [className, definition] : classes) {
    for (auto const& [attributeName, attributeType] : definition.declaredAttributeTypes) {
      (void)attributeType;
      std::string parentName = definition.parentName;
      while (!parentName.empty()) {
        auto const parent = classes.find(parentName);
        if (parent == classes.end()) {
          break;
        }
        if (parent->second.declaredAttributeTypes.contains(attributeName)) {
          diagnostics = "Object class " + quoteDiagnosticString(className) +
                        " duplicates inherited attribute " + quoteDiagnosticString(attributeName) +
                        " from " + quoteDiagnosticString(parentName) + ".";
          return false;
        }
        parentName = parent->second.parentName;
      }
    }
  }
  return true;
}

struct InteractionClassReferenceDefinition {
  std::string parentName;
  std::set<std::string> declaredParameterNames;
};

using InteractionClassReferenceDefinitions =
    std::map<std::string, InteractionClassReferenceDefinition>;

void collectInteractionClassReferenceDefinitions(
    SemanticNode const& parent,
    std::string const& parentName,
    InteractionClassReferenceDefinitions& definitions) {
  for (auto const& [key, child] : parent.children) {
    (void)key;
    if (child.localName != fom_schema::element::interaction_class) {
      continue;
    }
    std::string const shortName = identityValue(child, fom_schema::identity::name_prefix);
    if (shortName.empty()) {
      continue;
    }
    std::string const qualifiedName =
        parentName.empty() ? shortName : parentName + "." + shortName;
    InteractionClassReferenceDefinition definition{parentName, {}};
    for (auto const& [childKey, parameter] : child.children) {
      (void)childKey;
      if (parameter.localName != fom_schema::element::parameter) {
        continue;
      }
      std::string const parameterName = identityValue(parameter, fom_schema::identity::name_prefix);
      if (!parameterName.empty()) {
        definition.declaredParameterNames.insert(parameterName);
      }
    }
    definitions.insert_or_assign(qualifiedName, std::move(definition));
    collectInteractionClassReferenceDefinitions(child, qualifiedName, definitions);
  }
}

InteractionClassReferenceDefinitions interactionClassReferenceDefinitions(SemanticNode const& root) {
  InteractionClassReferenceDefinitions definitions;
  if (auto const* interactions = firstChildNamed(root, fom_schema::element::interactions);
      interactions != nullptr) {
    collectInteractionClassReferenceDefinitions(*interactions, {}, definitions);
  }
  return definitions;
}

bool validateInheritedInteractionClassParameterNames(
    SemanticNode const& root,
    std::string& diagnostics) {
  InteractionClassReferenceDefinitions const classes = interactionClassReferenceDefinitions(root);
  for (auto const& [className, definition] : classes) {
    for (std::string const& parameterName : definition.declaredParameterNames) {
      std::string parentName = definition.parentName;
      while (!parentName.empty()) {
        auto const parent = classes.find(parentName);
        if (parent == classes.end()) {
          break;
        }
        if (parent->second.declaredParameterNames.contains(parameterName)) {
          diagnostics = "Interaction class " + quoteDiagnosticString(className) +
                        " duplicates inherited parameter " + quoteDiagnosticString(parameterName) +
                        " from " + quoteDiagnosticString(parentName) + ".";
          return false;
        }
        parentName = parent->second.parentName;
      }
    }
  }
  return true;
}

bool validateEnumeratedValues(SemanticNode const& root, std::string& diagnostics) {
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != fom_schema::element::enumerated_data) {
      return;
    }
    std::map<std::string, std::string> valueOwners;
    for (auto const& [key, enumerator] : node.children) {
      (void)key;
      if (enumerator.localName != fom_schema::element::enumerator) {
        continue;
      }
      std::string const enumeratorName = identityValue(enumerator, fom_schema::identity::name_prefix);
      for (auto const& [valueKey, value] : enumerator.children) {
        (void)valueKey;
        if (value.localName != fom_schema::element::value || value.text.empty()) {
          continue;
        }
        auto const [existing, inserted] = valueOwners.emplace(value.text, enumeratorName);
        if (!inserted && existing->second != enumeratorName) {
          diagnostics = "Enumerated data type " +
                        quotedIdentityValue(node, fom_schema::identity::name_prefix) +
                        " assigns value " + quoteDiagnosticString(value.text) + " to both " +
                        quoteDiagnosticString(existing->second) + " and " +
                        quoteDiagnosticString(enumeratorName) + " at " + path + ".";
          valid = false;
          return;
        }
      }
    }
  });
  return valid;
}

std::string trimAsciiWhitespace(std::string value) {
  auto const isWhitespace = [](unsigned char character) {
    return std::isspace(character) != 0;
  };
  while (!value.empty() && isWhitespace(static_cast<unsigned char>(value.front()))) {
    value.erase(value.begin());
  }
  while (!value.empty() && isWhitespace(static_cast<unsigned char>(value.back()))) {
    value.pop_back();
  }
  return value;
}

std::vector<SemanticNode const*> variantDiscriminantEnumeratorNodes(
    SemanticNode const& alternative) {
  std::vector<SemanticNode const*> enumerators;
  for (SemanticNode const* child : childrenInDeclarationOrder(alternative)) {
    if (child->localName == fom_schema::element::enumerator) {
      enumerators.push_back(child);
    }
  }
  return enumerators;
}

struct VariantDiscriminantEnumeratorComponent {
  std::string first;
  std::optional<std::string> last;
};

std::vector<VariantDiscriminantEnumeratorComponent> variantDiscriminantEnumeratorComponents(
    std::string expression) {
  std::vector<VariantDiscriminantEnumeratorComponent> components;
  expression = trimAsciiWhitespace(std::move(expression));
  std::size_t componentStart = 0;
  while (componentStart <= expression.size()) {
    std::size_t const comma = expression.find(',', componentStart);
    std::string const component = trimAsciiWhitespace(
        expression.substr(
            componentStart,
            comma == std::string::npos ? std::string::npos : comma - componentStart));
    if (component.empty()) {
      return {};
    }
    if (component.size() >= 2 && component.front() == '[' && component.back() == ']') {
      std::string const range = trimAsciiWhitespace(component.substr(1, component.size() - 2));
      std::size_t const separator = range.find("..");
      if (separator == std::string::npos) {
        return {};
      }
      components.push_back({
          trimAsciiWhitespace(range.substr(0, separator)),
          trimAsciiWhitespace(range.substr(separator + 2)),
      });
    } else {
      components.push_back({component, std::nullopt});
    }
    if (comma == std::string::npos) {
      break;
    }
    componentStart = comma + 1;
  }
  return components;
}

bool validateVariantRecordDiscriminantEnumeratorSyntax(
    SemanticNode const& root,
    std::string& diagnostics) {
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != fom_schema::element::variant_record_data) {
      return;
    }

    std::string const recordName = identityValue(node, fom_schema::identity::name_prefix);
    std::string hlaOtherPath;
    for (SemanticNode const* alternative : childrenInDeclarationOrder(node)) {
      if (alternative->localName != fom_schema::element::alternative) {
        continue;
      }
      for (SemanticNode const* enumerator : variantDiscriminantEnumeratorNodes(*alternative)) {
        // DIF deliberately permits an incomplete alternative. This bounded
        // completed-model rule checks only a supplied discriminant-enumerator
        // field.
        if (enumerator->text.empty()) {
          continue;
        }
        std::string const enumeratorPath =
            childPath(childPath(path, *alternative), *enumerator);
        std::string const expression = trimAsciiWhitespace(enumerator->text);
        if (expression.empty()) {
          continue;
        }

        if (expression == fom_schema::value::hla_other_enumerator) {
          if (!hlaOtherPath.empty()) {
            diagnostics = "Variant-record data type " + quoteDiagnosticString(recordName) +
                          " uses HLAother more than once at " + enumeratorPath +
                          "; the first occurrence is at " + hlaOtherPath + ".";
            valid = false;
            return;
          }
          hlaOtherPath = enumeratorPath;
          continue;
        }

        std::size_t componentStart = 0;
        while (componentStart <= expression.size()) {
          std::size_t const comma = expression.find(',', componentStart);
          std::string const component = trimAsciiWhitespace(
              expression.substr(
                  componentStart,
                  comma == std::string::npos ? std::string::npos : comma - componentStart));
          if (component.empty()) {
            diagnostics = "Variant-record data type " + quoteDiagnosticString(recordName) +
                          " has an empty discriminant-enumerator component at " + enumeratorPath + ".";
            valid = false;
            return;
          }
          if (component == fom_schema::value::hla_other_enumerator) {
            diagnostics = "Variant-record data type " + quoteDiagnosticString(recordName) +
                          " must use HLAother as the complete discriminant-enumerator field at " +
                          enumeratorPath + ".";
            valid = false;
            return;
          }

          bool const startsRange = component.starts_with('[');
          bool const endsRange = component.ends_with(']');
          if (startsRange != endsRange) {
            diagnostics = "Variant-record data type " + quoteDiagnosticString(recordName) +
                          " has a malformed discriminant-enumerator range at " + enumeratorPath + ".";
            valid = false;
            return;
          }
          if (startsRange) {
            std::string const range =
                trimAsciiWhitespace(component.substr(1, component.size() - 2));
            std::size_t const separator = range.find("..");
            if (separator == std::string::npos ||
                range.find("..", separator + 2) != std::string::npos ||
                range.find('[') != std::string::npos ||
                range.find(']') != std::string::npos ||
                trimAsciiWhitespace(range.substr(0, separator)).empty() ||
                trimAsciiWhitespace(range.substr(separator + 2)).empty()) {
              diagnostics = "Variant-record data type " + quoteDiagnosticString(recordName) +
                            " has a malformed discriminant-enumerator range at " + enumeratorPath + ".";
              valid = false;
              return;
            }
          } else if (component.find("..") != std::string::npos ||
                     component.find('[') != std::string::npos ||
                     component.find(']') != std::string::npos) {
            diagnostics = "Variant-record data type " + quoteDiagnosticString(recordName) +
                          " has a malformed discriminant-enumerator range at " + enumeratorPath + ".";
            valid = false;
            return;
          }

          if (comma == std::string::npos) {
            break;
          }
          componentStart = comma + 1;
        }
      }
    }
  });
  return valid;
}

bool validateVariantRecordAlternatives(SemanticNode const& root, std::string& diagnostics) {
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != fom_schema::element::variant_record_data) {
      return;
    }
    bool const isExtendable =
        scalarChildValue(node, fom_schema::element::encoding) ==
            fom_schema::value::extendable_variant_record_encoding;
    std::map<std::string, std::string> enumeratorOwners;
    for (SemanticNode const* alternative : childrenInDeclarationOrder(node)) {
      if (alternative->localName != fom_schema::element::alternative) {
        continue;
      }
      std::string const alternativeName = identityValue(*alternative, fom_schema::identity::name_prefix);
      for (SemanticNode const* enumerator : variantDiscriminantEnumeratorNodes(*alternative)) {
        if (enumerator->text.empty()) {
          continue;
        }
        std::string const expression = trimAsciiWhitespace(enumerator->text);
        if (isExtendable && expression == fom_schema::value::hla_other_enumerator) {
          diagnostics = "Extendable variant record " +
                        quotedIdentityValue(node, fom_schema::identity::name_prefix) +
                        " uses the prohibited HLAother alternative at " + path + ".";
          valid = false;
          return;
        }
        auto const [existing, inserted] = enumeratorOwners.emplace(expression, alternativeName);
        if (!inserted && existing->second != alternativeName) {
          diagnostics = "Variant record " +
                        quotedIdentityValue(node, fom_schema::identity::name_prefix) +
                        " assigns enumerator " + quoteDiagnosticString(expression) +
                        " to both " + quoteDiagnosticString(existing->second) + " and " +
                        quoteDiagnosticString(alternativeName) +
                        " at " + path + ".";
          valid = false;
          return;
        }
      }
    }
  });
  return valid;
}

struct EnumeratedDataTypeEnumerators {
  std::vector<std::string> declarationOrder;
  std::map<std::string, std::size_t> declarationIndex;
};

using EnumeratedDataTypeEnumeratorNames =
    std::map<std::string, EnumeratedDataTypeEnumerators>;

EnumeratedDataTypeEnumeratorNames enumeratedDataTypeEnumeratorNames(SemanticNode const& root) {
  EnumeratedDataTypeEnumeratorNames names;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const& node, std::string const&) {
    if (node.localName != fom_schema::element::enumerated_data) {
      return;
    }
    std::string const dataTypeName = identityValue(node, fom_schema::identity::name_prefix);
    if (dataTypeName.empty()) {
      return;
    }
    auto& enumerators = names[dataTypeName];
    for (SemanticNode const* enumerator : childrenInDeclarationOrder(node)) {
      if (enumerator->localName != fom_schema::element::enumerator) {
        continue;
      }
      std::string const enumeratorName = identityValue(*enumerator, fom_schema::identity::name_prefix);
      if (enumeratorName.empty()) {
        continue;
      }
      auto const [existing, inserted] = enumerators.declarationIndex.emplace(
          enumeratorName,
          enumerators.declarationOrder.size());
      (void)existing;
      if (inserted) {
        enumerators.declarationOrder.push_back(enumeratorName);
      }
    }
  });
  return names;
}

bool validateVariantRecordDiscriminantEnumeratorMembership(
    SemanticNode const& root,
    std::string& diagnostics) {
  EnumeratedDataTypeEnumeratorNames const names = enumeratedDataTypeEnumeratorNames(root);
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != fom_schema::element::variant_record_data) {
      return;
    }

    std::string const dataType = scalarChildValue(node, fom_schema::element::data_type);
    auto const namedEnumerators = names.find(dataType);
    // This predicate follows the general resolver and discriminant-type
    // category check. If the selected enumeration has no supplied enumerators,
    // retain incomplete DIF without inventing a closed member set.
    if (dataType.empty() || namedEnumerators == names.end() ||
        namedEnumerators->second.declarationOrder.empty()) {
      return;
    }

    std::string const recordName = identityValue(node, fom_schema::identity::name_prefix);
    auto validateMember = [&](std::string const& member, std::string const& enumeratorPath) {
      if (namedEnumerators->second.declarationIndex.contains(member)) {
        return true;
      }
      diagnostics = "Variant-record data type " + quoteDiagnosticString(recordName) +
                    " uses discriminant enumerator " + quoteDiagnosticString(member) +
                    " at " + enumeratorPath +
                    " that is not declared by discriminant type " + quoteDiagnosticString(dataType) + ".";
      return false;
    };

    for (SemanticNode const* alternative : childrenInDeclarationOrder(node)) {
      if (alternative->localName != fom_schema::element::alternative) {
        continue;
      }
      for (SemanticNode const* enumerator : variantDiscriminantEnumeratorNodes(*alternative)) {
        if (enumerator->text.empty()) {
          continue;
        }
        std::string const expression = trimAsciiWhitespace(enumerator->text);
        // The lexical predicate has already accepted the grammar. HLAother is
        // not an enumerator name and therefore has no direct membership check.
        if (expression.empty() || expression == fom_schema::value::hla_other_enumerator) {
          continue;
        }
        std::string const enumeratorPath =
            childPath(childPath(path, *alternative), *enumerator);
        for (VariantDiscriminantEnumeratorComponent const& component :
             variantDiscriminantEnumeratorComponents(expression)) {
          if (component.last.has_value()) {
            if (!validateMember(component.first, enumeratorPath) ||
                !validateMember(*component.last, enumeratorPath)) {
              valid = false;
              return;
            }
          } else if (!validateMember(component.first, enumeratorPath)) {
            valid = false;
            return;
          }
        }
      }
    }
  });
  return valid;
}

bool validateVariantRecordDiscriminantEnumeratorAssignments(
    SemanticNode const& root,
    std::string& diagnostics) {
  EnumeratedDataTypeEnumeratorNames const enumerators =
      enumeratedDataTypeEnumeratorNames(root);
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != fom_schema::element::variant_record_data) {
      return;
    }

    std::string const dataType = scalarChildValue(node, fom_schema::element::data_type);
    auto const selected = enumerators.find(dataType);
    // Preserve incomplete DIF that does not declare any members for the
    // selected enumeration.  A populated enumerated table is sufficient to
    // expand supplied ranges against its own declaration order.
    if (dataType.empty() || selected == enumerators.end() ||
        selected->second.declarationOrder.empty()) {
      return;
    }

    std::string const recordName = identityValue(node, fom_schema::identity::name_prefix);
    std::map<std::string, std::string> owners;
    std::string hlaOtherAlternative;
    auto assign = [&](std::string const& enumeratorName,
                      std::string const& alternativeName,
                      std::string const& enumeratorPath) {
      auto const [existing, inserted] = owners.emplace(enumeratorName, alternativeName);
      if (inserted || existing->second == alternativeName) {
        return true;
      }
      diagnostics = "Variant record " + quoteDiagnosticString(recordName) +
                    " assigns discriminant enumerator " + quoteDiagnosticString(enumeratorName) +
                    " to both named alternatives " + quoteDiagnosticString(existing->second) +
                    " and " + quoteDiagnosticString(alternativeName) +
                    " after expanding its discriminant-enumerator range at " +
                    enumeratorPath + ".";
      return false;
    };

    for (SemanticNode const* alternative : childrenInDeclarationOrder(node)) {
      if (alternative->localName != fom_schema::element::alternative) {
        continue;
      }
      std::string const alternativeName = identityValue(*alternative, fom_schema::identity::name_prefix);
      for (SemanticNode const* enumerator : variantDiscriminantEnumeratorNodes(*alternative)) {
        std::string const expression = trimAsciiWhitespace(enumerator->text);
        if (expression.empty()) {
          continue;
        }
        if (expression == fom_schema::value::hla_other_enumerator) {
          hlaOtherAlternative = alternativeName;
          continue;
        }

        std::string const enumeratorPath =
            childPath(childPath(path, *alternative), *enumerator);
        for (VariantDiscriminantEnumeratorComponent const& component :
             variantDiscriminantEnumeratorComponents(expression)) {
          if (!component.last.has_value()) {
            if (selected->second.declarationIndex.contains(component.first) &&
                !assign(component.first, alternativeName, enumeratorPath)) {
              valid = false;
              return;
            }
            continue;
          }

          auto const first = selected->second.declarationIndex.find(component.first);
          auto const last = selected->second.declarationIndex.find(*component.last);
          if (first == selected->second.declarationIndex.end() ||
              last == selected->second.declarationIndex.end()) {
            // The earlier membership predicate reports an actionable failure
            // for this case; do not mask it with a range-expansion error.
            continue;
          }
          // IEEE 1516.2 defines a range by the enumerators occurring between
          // its endpoints in the table.  It does not establish a separate
          // lower/upper endpoint convention, so use the inclusive table span.
          std::size_t const rangeFirst = std::min(first->second, last->second);
          std::size_t const rangeLast = std::max(first->second, last->second);
          for (std::size_t index = rangeFirst; index <= rangeLast; ++index) {
            if (!assign(
                    selected->second.declarationOrder[index],
                    alternativeName,
                    enumeratorPath)) {
              valid = false;
              return;
            }
          }
        }
      }
    }

    // HLAother is precisely the complement of every explicitly assigned
    // member.  Retaining that assignment internally avoids treating it as a
    // literal enumerator or as an overlap with an explicit range.
    if (!hlaOtherAlternative.empty()) {
      for (std::string const& enumeratorName : selected->second.declarationOrder) {
        owners.try_emplace(enumeratorName, hlaOtherAlternative);
      }
    }
  });
  return valid;
}

bool parseNonNegativeCardinality(std::string value) {
  value = trimAsciiWhitespace(std::move(value));
  if (value.empty()) {
    return false;
  }
  unsigned long long parsed = 0;
  auto const result = std::from_chars(value.data(), value.data() + value.size(), parsed, 10);
  return result.ec == std::errc{} && result.ptr == value.data() + value.size();
}

bool validArrayCardinalityComponent(std::string value) {
  value = trimAsciiWhitespace(std::move(value));
  if (value == fom_schema::value::dynamic_cardinality) {
    return true;
  }
  if (value.size() >= 2 && value.front() == '[' && value.back() == ']') {
    std::string const range = value.substr(1, value.size() - 2);
    std::size_t const separator = range.find("..");
    if (separator == std::string::npos ||
        range.find("..", separator + 2) != std::string::npos) {
      return false;
    }
    std::string const lower = trimAsciiWhitespace(range.substr(0, separator));
    std::string const upper = trimAsciiWhitespace(range.substr(separator + 2));
    if (!parseNonNegativeCardinality(lower) || !parseNonNegativeCardinality(upper)) {
      return false;
    }
    unsigned long long lowerValue = 0;
    unsigned long long upperValue = 0;
    auto const lowerResult = std::from_chars(
        lower.data(), lower.data() + lower.size(), lowerValue, 10);
    auto const upperResult = std::from_chars(
        upper.data(), upper.data() + upper.size(), upperValue, 10);
    return lowerResult.ec == std::errc{} && upperResult.ec == std::errc{} &&
           lowerResult.ptr == lower.data() + lower.size() &&
           upperResult.ptr == upper.data() + upper.size() && lowerValue <= upperValue;
  }

  return parseNonNegativeCardinality(value);
}

bool validArrayCardinality(std::string value) {
  value = trimAsciiWhitespace(std::move(value));
  std::size_t cursor = 0;
  bool sawValue = false;
  while (cursor <= value.size()) {
    std::size_t const separator = value.find(',', cursor);
    std::string const component = value.substr(
        cursor,
        separator == std::string::npos ? std::string::npos : separator - cursor);
    if (!validArrayCardinalityComponent(component)) {
      return false;
    }
    sawValue = true;
    if (separator == std::string::npos) {
      break;
    }
    cursor = separator + 1;
  }
  return sawValue;
}

bool validateArrayCardinalities(SemanticNode const& root, std::string& diagnostics) {
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != fom_schema::element::array_data) {
      return;
    }
    std::string const cardinality = scalarChildValue(node, fom_schema::element::cardinality);
    if (cardinality.empty() || validArrayCardinality(cardinality)) {
      return;
    }
    diagnostics = "Array data type " + quotedIdentityValue(node, fom_schema::identity::name_prefix) + " at " + path +
                  " has invalid cardinality " + quoteDiagnosticString(cardinality) +
                  "; expected a nonnegative integer, comma-separated integers, "
                  "a nonnegative [lower..upper] range, or Dynamic.";
    valid = false;
  });
  return valid;
}

// The predefined one-dimensional array encodings carry a semantic
// compatibility rule in IEEE 1516.2: HLAfixedArray is for a fixed
// cardinality, while HLAvariableArray is for a varying (including Dynamic)
// cardinality.  Multidimensional cardinalities and provider-defined encodings
// need their own interpretation, so this bounded preflight deliberately leaves
// those cases to the later table-completeness work.
std::optional<bool> oneDimensionalArrayCardinalityIsVariable(std::string value) {
  value = trimAsciiWhitespace(std::move(value));
  if (value.empty() || value.find(',') != std::string::npos) {
    return std::nullopt;
  }
  if (value == fom_schema::value::dynamic_cardinality) {
    return true;
  }
  if (value.size() >= 2 && value.front() == '[' && value.back() == ']') {
    std::string const range = value.substr(1, value.size() - 2);
    std::size_t const separator = range.find("..");
    if (separator == std::string::npos ||
        range.find("..", separator + 2) != std::string::npos) {
      return std::nullopt;
    }
    std::string const lower = trimAsciiWhitespace(range.substr(0, separator));
    std::string const upper = trimAsciiWhitespace(range.substr(separator + 2));
    if (!parseNonNegativeCardinality(lower) || !parseNonNegativeCardinality(upper)) {
      return std::nullopt;
    }
    unsigned long long lowerValue = 0;
    unsigned long long upperValue = 0;
    auto const lowerResult = std::from_chars(
        lower.data(), lower.data() + lower.size(), lowerValue, 10);
    auto const upperResult = std::from_chars(
        upper.data(), upper.data() + upper.size(), upperValue, 10);
    if (lowerResult.ec != std::errc{} || upperResult.ec != std::errc{} ||
        lowerResult.ptr != lower.data() + lower.size() ||
        upperResult.ptr != upper.data() + upper.size() || lowerValue > upperValue) {
      return std::nullopt;
    }
    return lowerValue != upperValue;
  }
  if (!parseNonNegativeCardinality(value)) {
    return std::nullopt;
  }
  return false;
}

bool validateArrayEncodingCardinalityCompatibility(
    SemanticNode const& root,
    std::string& diagnostics) {
  bool valid = true;
  walkNodes(root, fom_schema::element::object_model, [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != fom_schema::element::array_data) {
      return;
    }
    std::string const encoding =
        trimAsciiWhitespace(scalarChildValue(node, fom_schema::element::encoding));
    if (encoding != fom_schema::value::fixed_array_encoding &&
        encoding != fom_schema::value::variable_array_encoding) {
      return;
    }
    auto const cardinality = oneDimensionalArrayCardinalityIsVariable(
        scalarChildValue(node, fom_schema::element::cardinality));
    if (!cardinality.has_value()) {
      return;
    }
    bool const encodingIsVariable = encoding == fom_schema::value::variable_array_encoding;
    if (encodingIsVariable == *cardinality) {
      return;
    }
    diagnostics = "Array data type " + quotedIdentityValue(node, fom_schema::identity::name_prefix) + " at " + path +
                  " pairs " + quoteDiagnosticString(encoding) + " with a " +
                  (*cardinality ? "variable" : "fixed") +
                  " one-dimensional cardinality; the predefined encoding must be " +
                  std::string(*cardinality ? fom_schema::value::variable_array_encoding
                                           : fom_schema::value::fixed_array_encoding) +
                  ".";
    valid = false;
  });
  return valid;
}

// IEEE 1516.2-2025 Table 14 requires dimension names to be unique within a
// Dimension table.  The DIF schema intentionally leaves that semantic rule
// open, and the identity-based SemanticNode map would otherwise coalesce
// repeated rows before the rule could be checked.  Validate each source
// module before composition so equivalent definitions supplied by separate
// modules remain available to the Annex C.4 merge path.
bool validateUniqueDimensionNames(xmlDoc const* document, std::string& diagnostics) {
  xmlNode const* root = xmlDocGetRootElement(const_cast<xmlDoc*>(document));
  xmlNode const* dimensions = directChildElement(root, fom_schema::element::dimensions);
  if (dimensions == nullptr) {
    return true;
  }

  std::set<std::string> names;
  for (xmlNode const* dimension = dimensions->children; dimension != nullptr;
       dimension = dimension->next) {
    if (dimension->type != XML_ELEMENT_NODE ||
        localName(dimension) != fom_schema::element::dimension) {
      continue;
    }
    std::string const name = directChildText(dimension, fom_schema::element::name);
    if (name.empty() || names.insert(name).second) {
      continue;
    }
    diagnostics = "Dimension name " + quoteDiagnosticString(name) +
                  " is repeated in one module; dimension names must be unique.";
    return false;
  }
  return true;
}

}  // namespace

bool validateFomModificationDate(xmlNode const* modelIdentification, std::string& diagnostics) {
  return validateModificationDate(modelIdentification, diagnostics);
}

void collectFomReservedHlaNames(SemanticNode const& root, std::set<std::string>& reservedNames) {
  collectReservedHlaNames(root, reservedNames);
}

bool validateUniqueComposedFomDimensions(xmlDoc const* document, std::string& diagnostics) {
  return validateUniqueDimensionNames(document, diagnostics);
}

bool validateMergedFomModuleState(SemanticNode const& merged, std::string& diagnostics) {
  return validateDataTypeKinds(merged, diagnostics) &&
         validateEnumeratedValues(merged, diagnostics) &&
         validateVariantRecordDiscriminantEnumeratorSyntax(merged, diagnostics) &&
         validateVariantRecordAlternatives(merged, diagnostics);
}

bool validateComposedFomModel(
    SemanticNode const& merged,
    SemanticNode const* mergedNotes,
    FomStandardEdition standardEdition,
    std::set<std::string> const& reservedHlaNames,
    std::string& diagnostics) {
  bool const strictDataTypeTableRules = standardEdition == FomStandardEdition::ieee1516_2025;
  // Keep this table-specific failure ahead of generic reserved-name checks;
  // the established NA-companion case intentionally uses a transport named NA.
  return validateAttributeNaCompanionFields(merged, diagnostics) &&
         (!strictDataTypeTableRules ||
          validateHlaNames(merged, FomModuleKind::fom, &reservedHlaNames, diagnostics)) &&
         (!strictDataTypeTableRules || mergedNotes == nullptr ||
          validateHlaNames(*mergedNotes, FomModuleKind::fom, &reservedHlaNames, diagnostics)) &&
         validateDataTypeReferences(merged, diagnostics) &&
         (!strictDataTypeTableRules ||
          validateObjectAttributeAndInteractionParameterDataTypeKinds(merged, diagnostics)) &&
         validateDynamicAttributeUpdateConditions(merged, diagnostics) &&
         validateUnsharedAttributeValueRequirement(merged, diagnostics) &&
         (!strictDataTypeTableRules || validateArrayElementDataTypeKinds(merged, diagnostics)) &&
         (!strictDataTypeTableRules ||
          validateFixedRecordFieldAndVariantRecordAlternativeDataTypeKinds(merged, diagnostics)) &&
         (!strictDataTypeTableRules ||
          validateVariantRecordDiscriminantDataTypeKinds(merged, diagnostics)) &&
         validateVariantRecordDiscriminantEnumeratorMembership(merged, diagnostics) &&
         validateVariantRecordDiscriminantEnumeratorAssignments(merged, diagnostics) &&
         (!strictDataTypeTableRules || validateDimensionInputDataTypeKinds(merged, diagnostics)) &&
         validateDimensionInputDataTypeNaExclusivity(merged, diagnostics) &&
         validateDimensionInputDataTypeDescriptions(merged, diagnostics) &&
         validateDataTypeRepresentationReferences(merged, diagnostics) &&
         (!strictDataTypeTableRules || validateTimeRepresentationDataTypeKinds(merged, diagnostics)) &&
         (!strictDataTypeTableRules || validateTagDataTypeKinds(merged, diagnostics)) &&
         validateReferenceDataTypeClassReferences(merged, diagnostics) &&
         validateReferenceDataTypeAttributeReferences(merged, diagnostics) &&
         validateDirectedInteractionReferences(merged, diagnostics) &&
         validateAvailableDimensionReferences(merged, diagnostics) &&
         validateDimensionDefaultValues(merged, diagnostics) &&
         validateTransportationReferences(merged, diagnostics) &&
         validateStandardRootClassHierarchies(merged, diagnostics) &&
         validateUpdateRateValues(merged, diagnostics) &&
         validateArrayCardinalities(merged, diagnostics) &&
         validateArrayEncodingCardinalityCompatibility(merged, diagnostics) &&
         validateInheritedObjectClassAttributeNames(merged, diagnostics) &&
         validateInheritedInteractionClassParameterNames(merged, diagnostics);
}

}  // namespace umbra::detail
