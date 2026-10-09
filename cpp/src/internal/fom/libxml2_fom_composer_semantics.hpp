#pragma once

#include "internal/runtime/utf8_string.hpp"

#include <libxml/tree.h>

#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace umbra::detail {

enum class FomStandardEdition;

using NodeKey = std::pair<std::string, std::string>;

struct SemanticNode {
  std::string localName;
  std::string namespaceName;
  std::string identity;
  std::string text;
  std::map<std::string, std::string> attributes;
  std::map<NodeKey, SemanticNode> children;
  // The map supplies stable identity lookup during module composition. Some
  // IEEE 1516.2 table semantics depend on source row order, so retain that
  // order separately as well.
  std::vector<NodeKey> childOrder;
};

inline std::string normalizedText(std::string value) {
  std::string result;
  bool pendingSpace = false;
  for (unsigned char const character : value) {
    bool const asciiWhitespace =
        character == static_cast<unsigned char>(' ') ||
        character == static_cast<unsigned char>('\t') ||
        character == static_cast<unsigned char>('\n') ||
        character == static_cast<unsigned char>('\r') ||
        character == static_cast<unsigned char>('\f') ||
        character == static_cast<unsigned char>('\v');
    if (asciiWhitespace) {
      pendingSpace = !result.empty();
      continue;
    }
    if (pendingSpace) {
      result.push_back(' ');
      pendingSpace = false;
    }
    result.push_back(static_cast<char>(character));
  }
  return result;
}

inline std::string textFromNode(xmlNode const* node) {
  std::string text;
  for (xmlNode const* child = node->children; child != nullptr; child = child->next) {
    if (child->type == XML_TEXT_NODE || child->type == XML_CDATA_SECTION_NODE) {
      text += reinterpret_cast<char const*>(child->content);
    }
  }
  return normalizedText(std::move(text));
}

inline std::string localName(xmlNode const* node) {
  return node->name == nullptr ? std::string{} : reinterpret_cast<char const*>(node->name);
}

inline std::string localName(xmlAttr const* attribute) {
  return attribute->name == nullptr ? std::string{}
                                    : reinterpret_cast<char const*>(attribute->name);
}

inline xmlNode const* directChildElement(xmlNode const* parent, std::string_view desiredName) {
  if (parent == nullptr) {
    return nullptr;
  }
  for (xmlNode const* child = parent->children; child != nullptr; child = child->next) {
    if (child->type == XML_ELEMENT_NODE && localName(child) == desiredName) {
      return child;
    }
  }
  return nullptr;
}

inline std::string directChildText(xmlNode const* parent, std::string_view desiredName) {
  if (xmlNode const* child = directChildElement(parent, desiredName); child != nullptr) {
    return textFromNode(child);
  }
  return {};
}

inline std::vector<SemanticNode const*> childrenInDeclarationOrder(SemanticNode const& parent) {
  std::vector<SemanticNode const*> ordered;
  std::set<NodeKey> emitted;
  for (NodeKey const& key : parent.childOrder) {
    auto const child = parent.children.find(key);
    if (child != parent.children.end() && emitted.insert(key).second) {
      ordered.push_back(&child->second);
    }
  }
  // Preserve deterministic map traversal for nodes created before order
  // retention or by a merge path that did not populate childOrder.
  for (auto const& [key, child] : parent.children) {
    if (emitted.insert(key).second) {
      ordered.push_back(&child);
    }
  }
  return ordered;
}

inline std::string displayNode(SemanticNode const& node) {
  if (node.identity.empty()) {
    return node.localName;
  }
  auto const separator = node.identity.find('=');
  if (separator == std::string::npos) {
    return node.localName + "[" + quoteDiagnosticString(node.identity) + "]";
  }
  return node.localName + "[" + node.identity.substr(0, separator + 1U) +
         quoteDiagnosticString(node.identity.substr(separator + 1U)) + "]";
}

inline std::string childPath(std::string const& parent, SemanticNode const& child) {
  return parent + "/" + displayNode(child);
}

inline SemanticNode const* firstChildNamed(SemanticNode const& node, std::string_view desiredName) {
  for (auto const& [key, child] : node.children) {
    (void)key;
    if (child.localName == desiredName) {
      return &child;
    }
  }
  return nullptr;
}

inline std::string scalarChildValue(SemanticNode const& node, std::string_view desiredName) {
  if (auto const* child = firstChildNamed(node, desiredName); child != nullptr) {
    return child->text;
  }
  return {};
}

inline std::string identityValue(SemanticNode const& node, std::string_view prefix) {
  return node.identity.starts_with(prefix) ? node.identity.substr(prefix.size()) : std::string{};
}

inline std::string quotedIdentityValue(SemanticNode const& node, std::string_view prefix) {
  return quoteDiagnosticString(identityValue(node, prefix));
}

namespace fom_schema {
namespace element {
constexpr char object_model[] = "objectModel";
constexpr std::string_view objects = "objects";
constexpr std::string_view object_class = "objectClass";
constexpr std::string_view interactions = "interactions";
constexpr std::string_view interaction_class = "interactionClass";
constexpr std::string_view transportations = "transportations";
constexpr std::string_view dimensions = "dimensions";
constexpr std::string_view dimension = "dimension";
constexpr std::string_view input_data_types = "inputDataTypes";
constexpr std::string_view input_data_description = "inputDataDescription";
constexpr std::string_view upper_bound = "upperBound";
constexpr std::string_view data_type = "dataType";
constexpr std::string_view value = "value";
constexpr std::string_view rate = "rate";
constexpr std::string_view attribute = "attribute";
constexpr std::string_view parameter = "parameter";
constexpr std::string_view transportation = "transportation";
constexpr std::string_view order = "order";
constexpr std::string_view update_type = "updateType";
constexpr std::string_view update_condition = "updateCondition";
constexpr std::string_view sharing = "sharing";
constexpr std::string_view value_required = "valueRequired";
constexpr std::string_view field = "field";
constexpr std::string_view alternative = "alternative";
constexpr std::string_view basic_data = "basicData";
constexpr std::string_view simple_data = "simpleData";
constexpr std::string_view enumerated_data = "enumeratedData";
constexpr std::string_view reference_data_type = "referenceDataType";
constexpr std::string_view array_data = "arrayData";
constexpr std::string_view fixed_record_data = "fixedRecordData";
constexpr std::string_view variant_record_data = "variantRecordData";
constexpr std::string_view logical_time = "logicalTime";
constexpr std::string_view logical_time_interval = "logicalTimeInterval";
constexpr std::string_view update_reflect_tag = "updateReflectTag";
constexpr std::string_view send_receive_tag = "sendReceiveTag";
constexpr std::string_view delete_remove_tag = "deleteRemoveTag";
constexpr std::string_view divestiture_request_tag = "divestitureRequestTag";
constexpr std::string_view divestiture_completion_tag = "divestitureCompletionTag";
constexpr std::string_view acquisition_request_tag = "acquisitionRequestTag";
constexpr std::string_view request_update_tag = "requestUpdateTag";
constexpr std::string_view synchronization_point = "synchronizationPoint";
constexpr std::string_view update_rate = "updateRate";
constexpr std::string_view enumerator = "enumerator";
constexpr std::string_view representation = "representation";
constexpr std::string_view referenced_attribute = "referencedAttribute";
constexpr std::string_view reference_class = "referenceClass";
constexpr std::string_view directed_interaction = "directedInteraction";
constexpr std::string_view name = "name";
constexpr std::string_view note = "note";
constexpr std::string_view label = "label";
constexpr std::string_view modification_date = "modificationDate";
constexpr std::string_view encoding = "encoding";
constexpr std::string_view cardinality = "cardinality";
}  // namespace element

namespace value {
constexpr std::string_view not_applicable = "NA";
constexpr std::string_view conditional_update = "Conditional";
constexpr std::string_view periodic_update = "Periodic";
constexpr std::string_view neither_sharing = "Neither";
constexpr std::string_view false_literal = "false";
constexpr std::string_view true_literal = "true";
constexpr std::string_view true_numeric = "1";
constexpr std::string_view not_applicable_name = "na";
constexpr std::string_view reserved_hla_prefix = "hla";
constexpr std::string_view excluded_dimension_value = "Excluded";
constexpr std::string_view dynamic_cardinality = "Dynamic";
constexpr std::string_view hla_other_enumerator = "HLAother";
constexpr std::string_view extendable_variant_record_encoding = "HLAextendableVariantRecord";
constexpr std::string_view fixed_array_encoding = "HLAfixedArray";
constexpr std::string_view variable_array_encoding = "HLAvariableArray";
}  // namespace value

namespace identity {
constexpr std::string_view name_prefix = "name=";
}  // namespace identity
}  // namespace fom_schema

enum class DataTypeKind {
  basic,
  simple,
  enumerated,
  reference,
  array,
  fixed_record,
  variant_record,
};

using DataTypeDeclarationKinds = std::map<std::string, DataTypeKind>;

inline std::optional<DataTypeKind> dataTypeKindForElement(std::string_view elementName) {
  using namespace fom_schema::element;
  if (elementName == basic_data) {
    return DataTypeKind::basic;
  }
  if (elementName == simple_data) {
    return DataTypeKind::simple;
  }
  if (elementName == enumerated_data) {
    return DataTypeKind::enumerated;
  }
  if (elementName == reference_data_type) {
    return DataTypeKind::reference;
  }
  if (elementName == array_data) {
    return DataTypeKind::array;
  }
  if (elementName == fixed_record_data) {
    return DataTypeKind::fixed_record;
  }
  if (elementName == variant_record_data) {
    return DataTypeKind::variant_record;
  }
  return std::nullopt;
}

inline bool isDataTypeDeclaration(std::string_view localName) {
  return dataTypeKindForElement(localName).has_value();
}

template <typename Function>
inline void walkNodes(SemanticNode const& node, std::string const& path, Function&& function) {
  function(node, path);
  for (auto const& [key, child] : node.children) {
    (void)key;
    walkNodes(child, childPath(path, child), function);
  }
}

bool validateFomModificationDate(xmlNode const* modelIdentification, std::string& diagnostics);
void collectFomReservedHlaNames(SemanticNode const& root, std::set<std::string>& reservedNames);
bool validateUniqueComposedFomDimensions(xmlDoc const* document, std::string& diagnostics);
bool validateMergedFomModuleState(SemanticNode const& merged, std::string& diagnostics);
bool validateComposedFomModel(
    SemanticNode const& merged,
    SemanticNode const* mergedNotes,
    FomStandardEdition standardEdition,
    std::set<std::string> const& reservedHlaNames,
    std::string& diagnostics);

}  // namespace umbra::detail
