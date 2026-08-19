#include "internal/libxml2_fom_composer.hpp"

#include "internal/fdd_document.hpp"
#include "internal/fom_catalog.hpp"
#include "internal/libxml2_fom_document.hpp"
#include "internal/utf8_string.hpp"

#include <libxml/tree.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlsave.h>
#include <libxml/xmlschemas.h>

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <limits>
#include <utility>
#include <vector>

namespace umbra::detail {
namespace {

struct SemanticNode;
using NodeKey = std::pair<std::string, std::string>;
using NoteLabelMap = std::map<std::string, std::string>;

constexpr char kHla2025Namespace[] = "http://standards.ieee.org/IEEE1516-2025";
constexpr char kXmlSchemaInstanceNamespace[] = "http://www.w3.org/2001/XMLSchema-instance";
constexpr char kFddSchemaLocation[] =
    "http://standards.ieee.org/IEEE1516-2025 IEEE1516-FDD-2025.xsd";

struct SemanticNode {
  std::string localName;
  std::string namespaceName;
  std::string identity;
  std::string text;
  std::map<std::string, std::string> attributes;
  std::map<NodeKey, SemanticNode> children;
  // The map supplies stable identity lookup during module composition.  Some
  // IEEE 1516.2 table semantics, however, depend on the order in which rows
  // occur in the source table, so retain insertion order separately.
  std::vector<NodeKey> childOrder;
};

void appendChild(SemanticNode& parent, NodeKey key, SemanticNode child) {
  auto const [inserted, wasInserted] =
      parent.children.emplace(std::move(key), std::move(child));
  if (wasInserted) {
    parent.childOrder.push_back(inserted->first);
  }
}

std::vector<SemanticNode const*> childrenInDeclarationOrder(SemanticNode const& parent) {
  std::vector<SemanticNode const*> ordered;
  std::set<NodeKey> emitted;
  for (NodeKey const& key : parent.childOrder) {
    auto const child = parent.children.find(key);
    if (child != parent.children.end() && emitted.insert(key).second) {
      ordered.push_back(&child->second);
    }
  }
  // Semantic nodes created before order retention, or maintained by a future
  // merge path that omits it, retain the former deterministic map traversal
  // instead of silently dropping a child from semantic validation.
  for (auto const& [key, child] : parent.children) {
    if (emitted.insert(key).second) {
      ordered.push_back(&child);
    }
  }
  return ordered;
}

std::string normalizedText(std::string value) {
  std::string result;
  bool pendingSpace = false;
  for (unsigned char const character : value) {
    if (std::isspace(character) != 0) {
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

std::string textFromNode(xmlNode const* node) {
  std::string text;
  for (xmlNode const* child = node->children; child != nullptr; child = child->next) {
    if (child->type == XML_TEXT_NODE || child->type == XML_CDATA_SECTION_NODE) {
      text += reinterpret_cast<char const*>(child->content);
    }
  }
  return normalizedText(std::move(text));
}

std::string localName(xmlNode const* node) {
  return node->name == nullptr ? std::string{} : reinterpret_cast<char const*>(node->name);
}

std::string namespaceName(xmlNode const* node) {
  return node->ns == nullptr || node->ns->href == nullptr
             ? std::string{}
             : reinterpret_cast<char const*>(node->ns->href);
}

std::string localName(xmlAttr const* attribute) {
  return attribute->name == nullptr ? std::string{}
                                  : reinterpret_cast<char const*>(attribute->name);
}

std::string namespaceName(xmlAttr const* attribute) {
  return attribute->ns == nullptr || attribute->ns->href == nullptr
             ? std::string{}
             : reinterpret_cast<char const*>(attribute->ns->href);
}

std::string expandedName(std::string const& local, std::string const& nameSpace) {
  return nameSpace.empty() ? local : "{" + nameSpace + "}" + local;
}

std::string attributeValue(xmlAttr const* attribute) {
  xmlChar* value = xmlNodeListGetString(attribute->doc, attribute->children, 1);
  if (value == nullptr) {
    return {};
  }
  std::string result(reinterpret_cast<char const*>(value));
  xmlFree(value);
  return normalizedText(std::move(result));
}

std::string directChildText(xmlNode const* node, std::string_view desiredName) {
  for (xmlNode const* child = node->children; child != nullptr; child = child->next) {
    if (child->type == XML_ELEMENT_NODE && localName(child) == desiredName) {
      return textFromNode(child);
    }
  }
  return {};
}

std::string directAttributeValue(xmlNode const* node, std::string_view desiredName) {
  for (xmlAttr const* attribute = node->properties; attribute != nullptr; attribute = attribute->next) {
    if (localName(attribute) == desiredName) {
      return attributeValue(attribute);
    }
  }
  return {};
}

std::string remappedNoteLabel(
    std::string const& label,
    NoteLabelMap const* noteLabels) {
  if (noteLabels == nullptr) {
    return label;
  }
  auto const found = noteLabels->find(label);
  return found == noteLabels->end() ? label : found->second;
}

std::vector<std::string> splitWhitespaceSeparated(std::string const& value) {
  std::vector<std::string> values;
  std::size_t cursor = 0;
  while (cursor < value.size()) {
    while (cursor < value.size() && std::isspace(static_cast<unsigned char>(value[cursor])) != 0) {
      ++cursor;
    }
    std::size_t const start = cursor;
    while (cursor < value.size() && std::isspace(static_cast<unsigned char>(value[cursor])) == 0) {
      ++cursor;
    }
    if (start != cursor) {
      values.emplace_back(value.substr(start, cursor - start));
    }
  }
  return values;
}

std::string joinWhitespaceSeparated(std::vector<std::string> const& values) {
  std::string result;
  for (std::string const& value : values) {
    if (value.empty()) {
      continue;
    }
    if (!result.empty()) {
      result.push_back(' ');
    }
    result += value;
  }
  return result;
}

std::string remappedNoteReferences(
    std::string const& references,
    NoteLabelMap const* noteLabels) {
  std::vector<std::string> remapped;
  for (std::string const& reference : splitWhitespaceSeparated(references)) {
    remapped.push_back(remappedNoteLabel(reference, noteLabels));
  }
  return joinWhitespaceSeparated(remapped);
}

std::string declarationIdentity(xmlNode const* node, NoteLabelMap const* noteLabels) {
  if (auto const name = directChildText(node, "name"); !name.empty()) {
    return "name=" + name;
  }
  if (auto const label = directChildText(node, "label"); !label.empty()) {
    return "label=" +
           (localName(node) == "note" ? remappedNoteLabel(label, noteLabels) : label);
  }
  if (auto const name = directAttributeValue(node, "name"); !name.empty()) {
    return "name=" + name;
  }
  if (auto const label = directAttributeValue(node, "label"); !label.empty()) {
    return "label=" + label;
  }
  return {};
}

bool isRepeatedScalar(std::string_view parent, std::string_view child) {
  return (parent == "dimensions" && child == "dimension") ||
         (parent == "inputDataTypes" && child == "dataType") ||
         (parent == "enumerator" && child == "value") ||
         (parent == "alternative" && child == "enumerator");
}

NodeKey childKey(SemanticNode const& parent, SemanticNode const& child) {
  std::string identity = child.identity;
  if (identity.empty() && isRepeatedScalar(parent.localName, child.localName)) {
    identity = "value=" + child.text;
  }
  return {expandedName(child.localName, child.namespaceName), std::move(identity)};
}

SemanticNode buildSemanticNode(xmlNode const* xml, NoteLabelMap const* noteLabels = nullptr) {
  std::string text = textFromNode(xml);
  if (localName(xml) == "label" && xml->parent != nullptr &&
      localName(xml->parent) == "note") {
    text = remappedNoteLabel(text, noteLabels);
  }
  SemanticNode node{
      localName(xml),
      namespaceName(xml),
      declarationIdentity(xml, noteLabels),
      std::move(text),
      {},
      {},
  };
  for (xmlAttr const* attribute = xml->properties; attribute != nullptr; attribute = attribute->next) {
    std::string value = attributeValue(attribute);
    if (localName(attribute) == "noteReferences") {
      value = remappedNoteReferences(value, noteLabels);
    }
    node.attributes.emplace(
        expandedName(localName(attribute), namespaceName(attribute)), std::move(value));
  }
  for (xmlNode const* child = xml->children; child != nullptr; child = child->next) {
    if (child->type != XML_ELEMENT_NODE) {
      continue;
    }
    SemanticNode semanticChild = buildSemanticNode(child, noteLabels);
    NodeKey key = childKey(node, semanticChild);
    appendChild(node, std::move(key), std::move(semanticChild));
  }
  return node;
}

bool isCompositionSection(std::string_view local) {
  return local == "objects" || local == "interactions" || local == "dimensions" ||
         local == "time" || local == "tags" || local == "synchronizations" ||
         local == "transportations" || local == "switches" || local == "updateRates" ||
         local == "dataTypes";
}

SemanticNode buildCompositionRoot(xmlDoc const* document, NoteLabelMap const* noteLabels) {
  xmlNode const* xmlRoot = xmlDocGetRootElement(const_cast<xmlDoc*>(document));
  SemanticNode root{"objectModel", {}, {}, {}, {}, {}};
  for (xmlNode const* child = xmlRoot->children; child != nullptr; child = child->next) {
    if (child->type != XML_ELEMENT_NODE || !isCompositionSection(localName(child))) {
      continue;
    }
    SemanticNode section = buildSemanticNode(child, noteLabels);
    NodeKey key = childKey(root, section);
    appendChild(root, std::move(key), std::move(section));
  }
  return root;
}

std::string displayNode(SemanticNode const& node) {
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

std::string childPath(std::string const& parent, SemanticNode const& child) {
  return parent + "/" + displayNode(child);
}

SemanticNode const* firstChildNamed(SemanticNode const& node, std::string_view desiredName) {
  for (auto const& [key, child] : node.children) {
    (void)key;
    if (child.localName == desiredName) {
      return &child;
    }
  }
  return nullptr;
}

std::string scalarChildValue(SemanticNode const& node, std::string_view desiredName) {
  if (auto const* child = firstChildNamed(node, desiredName); child != nullptr) {
    return child->text;
  }
  return {};
}

unsigned long parseUnsignedLong(std::string const& value) {
  if (value.empty()) {
    return 0;
  }
  unsigned long parsed = 0;
  auto const result = std::from_chars(value.data(), value.data() + value.size(), parsed, 10);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) {
    return 0;
  }
  return parsed;
}

bool isEnabledSwitch(SemanticNode const& node) {
  auto const attribute = node.attributes.find("isEnabled");
  // The IEEE 1516.2 schema constrains this to xs:boolean.  Retain both XML
  // lexical spellings so the catalog reflects a validated source exactly.
  return attribute != node.attributes.end() &&
         (attribute->second == "true" || attribute->second == "1");
}

std::string resignActionValue(SemanticNode const& node) {
  auto const attribute = node.attributes.find("resignAction");
  if (attribute == node.attributes.end()) {
    return "CancelThenDeleteThenDivest";
  }
  if (attribute->second == "UnconditionallyDivestAttributes") {
    return attribute->second;
  }
  if (attribute->second == "DeleteObjects") {
    return attribute->second;
  }
  if (attribute->second == "CancelPendingOwnershipAcquisitions") {
    return attribute->second;
  }
  if (attribute->second == "DeleteObjectsThenDivest") {
    return attribute->second;
  }
  if (attribute->second == "CancelThenDeleteThenDivest") {
    return attribute->second;
  }
  if (attribute->second == "NoAction") {
    // NoAction is a valid explicit Automatic Resign Action setting.  It is
    // intentionally distinct from an omitted setting, whose source-derived
    // 1516.2 table default is handled above despite the XSD default tension
    // recorded in RL-024.
    return attribute->second;
  }
  // The DIF/FDD schema rejects an unknown lexical value.  Keep the private
  // catalog defensive for callers that bypass validation and use the
  // IEEE 1516.2 table default rather than manufacturing a non-standard enum.
  return "CancelThenDeleteThenDivest";
}

bool isNoteReferencesAttribute(std::string const& name) {
  return name == "noteReferences";
}

std::string unionNoteReferences(std::string const& current, std::string const& candidate) {
  std::vector<std::string> values = splitWhitespaceSeparated(current);
  std::set<std::string> seen(values.begin(), values.end());
  for (std::string const& value : splitWhitespaceSeparated(candidate)) {
    if (seen.insert(value).second) {
      values.push_back(value);
    }
  }
  return joinWhitespaceSeparated(values);
}

bool hasNewAlternative(SemanticNode const& current, SemanticNode const& candidate) {
  for (auto const& [key, child] : candidate.children) {
    if (child.localName == "alternative" && !current.children.contains(key)) {
      return true;
    }
  }
  return false;
}

bool checkVariantRecordExtension(
    SemanticNode const& current,
    SemanticNode const& candidate,
    std::string const& path,
    std::string& diagnostics) {
  if (current.localName != "variantRecordData" || !hasNewAlternative(current, candidate)) {
    return true;
  }
  if (scalarChildValue(current, "encoding") == "HLAextendableVariantRecord") {
    return true;
  }
  diagnostics = "A new alternative was supplied for non-extendable variant record " + path + ".";
  return false;
}

bool mergeNodeFields(
    SemanticNode& current,
    SemanticNode const& candidate,
    std::string const& path,
    std::string& diagnostics) {
  for (auto const& [name, candidateValue] : candidate.attributes) {
    auto const existing = current.attributes.find(name);
    if (isNoteReferencesAttribute(name)) {
      if (existing == current.attributes.end()) {
        current.attributes.emplace(name, candidateValue);
      } else {
        existing->second = unionNoteReferences(existing->second, candidateValue);
      }
      continue;
    }
    if (existing != current.attributes.end() && !existing->second.empty() &&
        !candidateValue.empty() && existing->second != candidateValue) {
      diagnostics = "Conflicting attribute " + quoteDiagnosticString(name) + " at " + path + ".";
      return false;
    }
    if (existing == current.attributes.end() || existing->second.empty()) {
      current.attributes[name] = candidateValue;
    }
  }
  if (!current.text.empty() && !candidate.text.empty() && current.text != candidate.text) {
    diagnostics = "Conflicting value at " + path + ".";
    return false;
  }
  if (current.text.empty()) {
    current.text = candidate.text;
  }
  return true;
}

bool semanticNodesEquivalent(SemanticNode const& current, SemanticNode const& candidate) {
  if (current.localName != candidate.localName ||
      current.namespaceName != candidate.namespaceName ||
      current.identity != candidate.identity ||
      current.text != candidate.text ||
      current.attributes != candidate.attributes ||
      current.children.size() != candidate.children.size()) {
    return false;
  }
  for (auto const& [key, currentChild] : current.children) {
    auto const candidateChild = candidate.children.find(key);
    if (candidateChild == candidate.children.end() ||
        !semanticNodesEquivalent(currentChild, candidateChild->second)) {
      return false;
    }
  }
  return true;
}

bool mergeSwitches(
    SemanticNode& current,
    SemanticNode const& candidate,
    std::string const& path,
    std::string& diagnostics,
    std::vector<std::string>& warnings) {
  if (!mergeNodeFields(current, candidate, path, diagnostics)) {
    return false;
  }

  // IEEE 1516.2-2025 Annex C.8 treats a same-named switch differently from
  // the ordinary structural merge rules: the existing setting wins and a
  // non-equivalent duplicate is ignored with a warning rather than making the
  // entire FOM composition inconsistent. Unique switch names are inserted.
  for (SemanticNode const* candidateSwitch : childrenInDeclarationOrder(candidate)) {
    NodeKey const key = childKey(current, *candidateSwitch);
    auto const existing = current.children.find(key);
    if (existing == current.children.end()) {
      appendChild(current, key, *candidateSwitch);
      continue;
    }
    if (!semanticNodesEquivalent(existing->second, *candidateSwitch)) {
      warnings.push_back(
          "Annex C.8 warning: non-equivalent duplicate switch " +
          displayNode(*candidateSwitch) + " at " + path +
          " was ignored; the first module's setting is retained.");
    }
  }
  return true;
}

bool mergeNode(
    SemanticNode& current,
    SemanticNode const& candidate,
    std::string const& path,
    std::string& diagnostics,
    std::vector<std::string>& warnings) {
  if (current.localName == "switches") {
    return mergeSwitches(current, candidate, path, diagnostics, warnings);
  }
  if (!checkVariantRecordExtension(current, candidate, path, diagnostics) ||
      !mergeNodeFields(current, candidate, path, diagnostics)) {
    return false;
  }

  for (SemanticNode const* candidateChild : childrenInDeclarationOrder(candidate)) {
    NodeKey const key = childKey(current, *candidateChild);
    auto existing = current.children.find(key);
    if (existing == current.children.end()) {
      appendChild(current, key, *candidateChild);
      continue;
    }
    if (!mergeNode(
            existing->second,
            *candidateChild,
            childPath(path, *candidateChild),
            diagnostics,
            warnings)) {
      return false;
    }
  }
  return true;
}

std::string identityValue(SemanticNode const& node, std::string_view prefix) {
  return node.identity.starts_with(prefix) ? node.identity.substr(prefix.size()) : std::string{};
}

std::string quotedIdentityValue(SemanticNode const& node, std::string_view prefix) {
  return quoteDiagnosticString(identityValue(node, prefix));
}

bool isDataTypeDeclaration(std::string_view local) {
  return local == "basicData" || local == "simpleData" || local == "referenceDataType" ||
         local == "enumeratedData" || local == "arrayData" || local == "fixedRecordData" ||
         local == "variantRecordData";
}

using DataTypeDeclarationKinds = std::map<std::string, std::string>;

template <typename Function>
void walkNodes(SemanticNode const& node, std::string const& path, Function&& function) {
  function(node, path);
  for (auto const& [key, child] : node.children) {
    (void)key;
    walkNodes(child, childPath(path, child), function);
  }
}

DataTypeDeclarationKinds dataTypeDeclarationKinds(SemanticNode const& root) {
  DataTypeDeclarationKinds kinds;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const&) {
    if (!isDataTypeDeclaration(node.localName)) {
      return;
    }
    std::string const name = identityValue(node, "name=");
    if (!name.empty()) {
      kinds.insert_or_assign(name, node.localName);
    }
  });
  return kinds;
}

bool validateDataTypeKinds(SemanticNode const& root, std::string& diagnostics) {
  std::map<std::string, std::pair<std::string, std::string>> seen;
  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || !isDataTypeDeclaration(node.localName)) {
      return;
    }
    std::string const name = identityValue(node, "name=");
    if (name.empty()) {
      return;
    }
    auto const [existing, inserted] = seen.emplace(name, std::make_pair(node.localName, path));
    if (!inserted && existing->second.first != node.localName) {
      diagnostics = "Data type " + quoteDiagnosticString(name) + " has incompatible definitions at " +
                    existing->second.second + " and " + path + ".";
      valid = false;
    }
  });
  return valid;
}

bool validateDataTypeReferences(SemanticNode const& root, std::string& diagnostics) {
  std::set<std::string> declaredNames;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const&) {
    // The 2025 OMT schema's dataTypeKey includes every data-type family,
    // including basicData. Keep complete-model name resolution aligned with
    // that key; narrower table-specific predicates below decide whether a
    // resolved basic-data name is valid for an individual table column.
    if (!isDataTypeDeclaration(node.localName)) {
      return;
    }
    std::string const name = identityValue(node, "name=");
    if (!name.empty()) {
      declaredNames.insert(name);
    }
  });

  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "dataType" || node.text.empty() || node.text == "NA") {
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

bool isObjectAttributeOrInteractionParameterDataTypeKind(std::string_view kind) {
  return kind == "simpleData" || kind == "enumeratedData" || kind == "referenceDataType" ||
         kind == "arrayData" || kind == "fixedRecordData" || kind == "variantRecordData";
}

bool validateObjectAttributeAndInteractionParameterDataTypeKinds(
    SemanticNode const& root,
    std::string& diagnostics) {
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);
  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || (node.localName != "attribute" && node.localName != "parameter")) {
      return;
    }
    std::string const dataType = scalarChildValue(node, "dataType");
    if (dataType.empty() || dataType == "NA") {
      return;
    }
    auto const kind = kinds.find(dataType);
    // The general data-type resolver runs first and reports a missing name.
    // This table-specific rule classifies an already declared name. HLAtoken
    // requires no name special case: the standard MIM declares it as arrayData.
    if (kind == kinds.end() || isObjectAttributeOrInteractionParameterDataTypeKind(kind->second)) {
      return;
    }
    std::string const owner = node.localName == "attribute" ? "Object attribute" : "Interaction parameter";
    diagnostics = owner + " data type " + quoteDiagnosticString(dataType) + " at " + path +
                  " is not permitted; it must name a simple, enumerated, reference, array, "
                  "fixed-record, or variant-record data type (or NA).";
    valid = false;
  });
  return valid;
}

bool validateAttributeNaCompanionFields(SemanticNode const& root, std::string& diagnostics) {
  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "attribute" ||
        scalarChildValue(node, "dataType") != "NA") {
      return;
    }

    std::string const attributeName = identityValue(node, "name=");
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
    if (auto const* transportation = firstChildNamed(node, "transportation");
        transportation != nullptr && transportation->text == "NA") {
      reject("transportation", "must name a non-NA transportation type");
      return;
    }
    if (auto const* order = firstChildNamed(node, "order");
        order != nullptr && order->text == "NA") {
      reject("order", "must name a non-NA order type");
      return;
    }
    if (auto const* updateType = firstChildNamed(node, "updateType");
        updateType != nullptr && updateType->text != "NA") {
      reject("update type", "must be NA");
      return;
    }
    if (auto const* updateCondition = firstChildNamed(node, "updateCondition");
        updateCondition != nullptr && updateCondition->text != "NA") {
      reject("update condition", "must be NA");
    }
  });
  return valid;
}

bool validateDynamicAttributeUpdateConditions(SemanticNode const& root, std::string& diagnostics) {
  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "attribute") {
      return;
    }
    auto const* updateType = firstChildNamed(node, "updateType");
    if (updateType == nullptr ||
        (updateType->text != "Conditional" && updateType->text != "Periodic")) {
      return;
    }
    auto const* updateCondition = firstChildNamed(node, "updateCondition");
    // DIF permits incomplete attribute rows at the module boundary. Once a
    // Conditional or Periodic row supplies its condition, the direct table
    // predicate requires actual text. This deliberately does not interpret
    // the conflicting Static/NA direction or parse a periodic-rate grammar.
    if (updateCondition == nullptr ||
        (!updateCondition->text.empty() && updateCondition->text != "NA")) {
      return;
    }
    diagnostics = "Attribute " + quotedIdentityValue(node, "name=") + " at " + path +
                  " has " + updateType->text +
                  " update type but its supplied update condition must contain non-NA text.";
    valid = false;
  });
  return valid;
}

bool validateUnsharedAttributeValueRequirement(SemanticNode const& root, std::string& diagnostics) {
  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "attribute") {
      return;
    }
    auto const* sharing = firstChildNamed(node, "sharing");
    auto const* valueRequired = firstChildNamed(node, "valueRequired");
    // DIF permits partial attribute rows. Only apply this Table 10 predicate
    // once the supplied merged row explicitly says the attribute is neither
    // published nor subscribed and also supplies its Value Required field.
    if (sharing == nullptr || sharing->text != "Neither" || valueRequired == nullptr ||
        valueRequired->text == "false") {
      return;
    }
    diagnostics = "Attribute " + quotedIdentityValue(node, "name=") + " at " + path +
                  " is neither published nor subscribed but its value-required field must be false.";
    valid = false;
  });
  return valid;
}

bool validateArrayElementDataTypeKinds(SemanticNode const& root, std::string& diagnostics) {
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);
  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "arrayData") {
      return;
    }
    std::string const dataType = scalarChildValue(node, "dataType");
    // DIF deliberately permits incomplete rows. Keep an omitted element type
    // and the established NA no-type marker representable; this completed-model
    // rule classifies only a supplied named declaration.
    if (dataType.empty() || dataType == "NA") {
      return;
    }
    auto const kind = kinds.find(dataType);
    // The general data-type resolver runs first and reports a missing name.
    // Table 35 restricts an array element to a name from another data-type
    // table, which excludes a raw basic-data representation.
    if (kind == kinds.end() || isObjectAttributeOrInteractionParameterDataTypeKind(kind->second)) {
      return;
    }
    diagnostics = "Array data type " + quotedIdentityValue(node, "name=") + " element type " +
                  quoteDiagnosticString(dataType) +
                  " at " + path +
                  " is not permitted; it must name a simple, enumerated, reference, array, "
                  "fixed-record, or variant-record data type (or NA).";
    valid = false;
  });
  return valid;
}

bool validateFixedRecordFieldAndVariantRecordAlternativeDataTypeKinds(
    SemanticNode const& root,
    std::string& diagnostics) {
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);
  bool valid = true;
  auto validateMember = [&](SemanticNode const& record,
                            SemanticNode const& member,
                            std::string const& memberPath,
                            std::string_view recordLabel,
                            std::string_view memberLabel) {
    std::string const dataType = scalarChildValue(member, "dataType");
    // DIF deliberately permits incomplete rows. This completed-model rule
    // classifies only a supplied named declaration.
    if (dataType.empty() || dataType == "NA") {
      return true;
    }
    auto const kind = kinds.find(dataType);
    // The general resolver runs first and reports an unresolved name. The
    // fixed-record Field Type and variant-record Alternative Type columns
    // accept names from data-type tables, not a raw basic-data representation.
    if (kind == kinds.end() || isObjectAttributeOrInteractionParameterDataTypeKind(kind->second)) {
      return true;
    }
    diagnostics = std::string(recordLabel) + " " + quotedIdentityValue(record, "name=") + " " +
                  std::string(memberLabel) + " " + quotedIdentityValue(member, "name=") +
                  " data type " + quoteDiagnosticString(dataType) + " at " + memberPath +
                  " is not permitted; it must name a simple, enumerated, reference, array, "
                  "fixed-record, or variant-record data type (or NA).";
    return false;
  };

  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || (node.localName != "fixedRecordData" && node.localName != "variantRecordData")) {
      return;
    }
    std::string_view const memberName =
        node.localName == "fixedRecordData" ? "field" : "alternative";
    std::string_view const recordLabel =
        node.localName == "fixedRecordData" ? "Fixed-record data type" : "Variant-record data type";
    std::string_view const memberLabel =
        node.localName == "fixedRecordData" ? "field" : "alternative";
    for (auto const& [key, member] : node.children) {
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
    SemanticNode const& root,
    std::string& diagnostics) {
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);
  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "variantRecordData") {
      return;
    }
    auto const* discriminantType = firstChildNamed(node, "dataType");
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
    if (discriminantType->text != "NA" && kind == kinds.end()) {
      return;
    }
    if (kind != kinds.end() && kind->second == "enumeratedData") {
      return;
    }
    diagnostics = "Variant-record data type " + quotedIdentityValue(node, "name=") +
                  " discriminant type " + quoteDiagnosticString(discriminantType->text) + " at " +
                  childPath(path, *discriminantType) +
                  " is not permitted; it must name an enumerated data type.";
    valid = false;
  });
  return valid;
}

bool validateDimensionInputDataTypeKinds(SemanticNode const& root, std::string& diagnostics) {
  auto const* dimensions = firstChildNamed(root, "dimensions");
  if (dimensions == nullptr) {
    return true;
  }

  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);
  std::string const dimensionsPath = childPath("objectModel", *dimensions);
  for (auto const& [key, dimension] : dimensions->children) {
    (void)key;
    if (dimension.localName != "dimension") {
      continue;
    }
    std::string const dimensionPath = childPath(dimensionsPath, dimension);
    for (auto const& [inputTypesKey, inputDataTypes] : dimension.children) {
      (void)inputTypesKey;
      if (inputDataTypes.localName != "inputDataTypes") {
        continue;
      }
      std::string const inputTypesPath = childPath(dimensionPath, inputDataTypes);
      for (auto const& [dataTypeKey, dataType] : inputDataTypes.children) {
        (void)dataTypeKey;
        if (dataType.localName != "dataType" || dataType.text.empty() || dataType.text == "NA") {
          continue;
        }
        auto const kind = kinds.find(dataType.text);
        // The general resolver runs first and reports a missing name. The
        // Dimension Input data type column refers to the 4.14 data-type
        // tables, so a raw basic-data representation is not a permitted name.
        if (kind == kinds.end() || isObjectAttributeOrInteractionParameterDataTypeKind(kind->second)) {
          continue;
        }
        diagnostics = "Dimension " + quotedIdentityValue(dimension, "name=") +
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
    SemanticNode const& root,
    std::string& diagnostics) {
  auto const* dimensions = firstChildNamed(root, "dimensions");
  if (dimensions == nullptr) {
    return true;
  }

  std::string const dimensionsPath = childPath("objectModel", *dimensions);
  for (auto const& [key, dimension] : dimensions->children) {
    (void)key;
    if (dimension.localName != "dimension") {
      continue;
    }
    auto const* inputDataTypes = firstChildNamed(dimension, "inputDataTypes");
    if (inputDataTypes == nullptr) {
      continue;
    }

    bool hasNaMarker = false;
    bool hasNamedInputType = false;
    for (auto const& [inputTypeKey, inputDataType] : inputDataTypes->children) {
      (void)inputTypeKey;
      if (inputDataType.localName != "dataType" || inputDataType.text.empty()) {
        continue;
      }
      if (inputDataType.text == "NA") {
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
      diagnostics = "Dimension " + quotedIdentityValue(dimension, "name=") + " at " +
                    childPath(dimensionsPath, dimension) +
                    " mixes the NA input data type marker with named input data types.";
      return false;
    }
  }
  return true;
}

bool validateDimensionInputDataTypeDescriptions(SemanticNode const& root, std::string& diagnostics) {
  auto const* dimensions = firstChildNamed(root, "dimensions");
  if (dimensions == nullptr) {
    return true;
  }

  std::string const dimensionsPath = childPath("objectModel", *dimensions);
  for (auto const& [key, dimension] : dimensions->children) {
    (void)key;
    if (dimension.localName != "dimension") {
      continue;
    }
    auto const* inputDataTypes = firstChildNamed(dimension, "inputDataTypes");
    auto const* inputDataDescription = firstChildNamed(dimension, "inputDataDescription");
    // Both fields are required by the DIF schema for a completed Dimension
    // entry. Preserve defensive behavior if an unchecked SemanticNode reaches
    // this private preflight.
    if (inputDataTypes == nullptr || inputDataDescription == nullptr) {
      continue;
    }

    bool hasNamedInputType = false;
    for (auto const& [inputTypeKey, inputDataType] : inputDataTypes->children) {
      (void)inputTypeKey;
      if (inputDataType.localName == "dataType" && !inputDataType.text.empty() &&
          inputDataType.text != "NA") {
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
        (inputDataDescription->text.empty() || inputDataDescription->text == "NA")) {
      diagnostics = "Dimension " + quotedIdentityValue(dimension, "name=") + " at " +
                    childPath(dimensionsPath, dimension) +
                    " has no named input data type but its input data description must be non-NA text.";
      return false;
    }
  }
  return true;
}

bool isStandardInstanceIdentifierAttribute(std::string_view name);

bool validateDataTypeRepresentationReferences(
    SemanticNode const& root,
    std::string& diagnostics) {
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);

  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid ||
        (node.localName != "simpleData" && node.localName != "enumeratedData" &&
         node.localName != "referenceDataType")) {
      return;
    }
    // The two standard instance identifiers are handled by the dedicated
    // special-reference rule below; their implicit attribute rows do not use
    // the ordinary reference-data representation predicate.
    if (node.localName == "referenceDataType" &&
        isStandardInstanceIdentifierAttribute(scalarChildValue(node, "referencedAttribute"))) {
      return;
    }
    std::string const representation = scalarChildValue(node, "representation");
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

    if (node.localName == "referenceDataType" &&
        (kind->second == "basicData" || kind->second == "referenceDataType")) {
      diagnostics = "Reference data type " + quotedIdentityValue(node, "name=") +
                    " representation " + quoteDiagnosticString(representation) + " at " + path +
                    " must name a simple, enumerated, array, fixed-record, or variant-record "
                    "data type.";
      valid = false;
      return;
    }

    if (node.localName == "enumeratedData" && kind->second != "basicData") {
      diagnostics = "Enumerated data type " + quotedIdentityValue(node, "name=") +
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

bool isTimeRepresentationDataTypeKind(std::string_view kind) {
  return kind == "simpleData" || kind == "enumeratedData" || kind == "arrayData" ||
         kind == "fixedRecordData" || kind == "variantRecordData";
}

bool validateTimeRepresentationDataTypeKinds(SemanticNode const& root, std::string& diagnostics) {
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);
  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || (node.localName != "logicalTime" && node.localName != "logicalTimeInterval")) {
      return;
    }
    std::string const dataType = scalarChildValue(node, "dataType");
    if (dataType.empty() || dataType == "NA") {
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
  return localName == "updateReflectTag" || localName == "sendReceiveTag" ||
         localName == "deleteRemoveTag" || localName == "divestitureRequestTag" ||
         localName == "divestitureCompletionTag" || localName == "acquisitionRequestTag" ||
         localName == "requestUpdateTag" || localName == "synchronizationPoint";
}

bool isTagDataTypeKind(std::string_view kind) {
  return isObjectAttributeOrInteractionParameterDataTypeKind(kind);
}

bool validateTagDataTypeKinds(SemanticNode const& root, std::string& diagnostics) {
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);
  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || !isTagDataTypeOwner(node.localName)) {
      return;
    }
    std::string const dataType = scalarChildValue(node, "dataType");
    if (dataType.empty() || dataType == "NA") {
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
    SemanticNode const& parent,
    std::string const& parentName,
    ObjectClassReferenceDefinitions& definitions) {
  for (auto const& [key, child] : parent.children) {
    (void)key;
    if (child.localName != "objectClass") {
      continue;
    }
    std::string const shortName = identityValue(child, "name=");
    if (shortName.empty()) {
      continue;
    }
    std::string const qualifiedName =
        parentName.empty() ? shortName : parentName + "." + shortName;
    ObjectClassReferenceDefinition definition{parentName, {}};
    for (auto const& [childKey, attribute] : child.children) {
      (void)childKey;
      if (attribute.localName != "attribute") {
        continue;
      }
      std::string const attributeName = identityValue(attribute, "name=");
      if (!attributeName.empty()) {
        definition.declaredAttributeTypes.emplace(
            attributeName,
            scalarChildValue(attribute, "dataType"));
      }
    }
    definitions.insert_or_assign(qualifiedName, std::move(definition));
    collectObjectClassReferenceDefinitions(child, qualifiedName, definitions);
  }
}

ObjectClassReferenceDefinitions objectClassReferenceDefinitions(SemanticNode const& root) {
  ObjectClassReferenceDefinitions definitions;
  if (auto const* objects = firstChildNamed(root, "objects"); objects != nullptr) {
    collectObjectClassReferenceDefinitions(*objects, {}, definitions);
  }
  return definitions;
}

bool validateReferenceDataTypeClassReferences(SemanticNode const& root, std::string& diagnostics) {
  ObjectClassReferenceDefinitions const classes = objectClassReferenceDefinitions(root);

  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "referenceDataType") {
      return;
    }
    std::string const referencedClass = scalarChildValue(node, "referenceClass");
    if (!referencedClass.empty() && !classes.contains(referencedClass)) {
      diagnostics = "Reference data type " + quotedIdentityValue(node, "name=") +
                    " refers to object class " + quoteDiagnosticString(referencedClass) + " at " + path +
                    ", but that class is not declared in the composed model.";
      valid = false;
    }
  });
  return valid;
}

bool isStandardInstanceIdentifierAttribute(std::string_view name) {
  return name == "HLAobjectInstanceName" || name == "HLAobjectInstanceHandle";
}

std::optional<std::string> referencedAttributeDataType(
    ObjectClassReferenceDefinitions const& classes,
    std::string className,
    std::string const& attributeName) {
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

bool validateReferenceDataTypeAttributeReferences(SemanticNode const& root, std::string& diagnostics) {
  ObjectClassReferenceDefinitions const classes = objectClassReferenceDefinitions(root);
  DataTypeDeclarationKinds const kinds = dataTypeDeclarationKinds(root);

  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "referenceDataType") {
      return;
    }
    std::string const referencedClass = scalarChildValue(node, "referenceClass");
    std::string const referencedAttribute = scalarChildValue(node, "referencedAttribute");
    if (referencedClass.empty() || referencedAttribute.empty()) {
      return;
    }
    if (isStandardInstanceIdentifierAttribute(referencedAttribute)) {
      // IEEE 1516.2 gives the two standard instance identifiers special
      // semantics rather than requiring an ordinary attribute row. Preserve
      // that exception, but still enforce their standardized representations.
      std::string const expectedRepresentation =
          referencedAttribute == "HLAobjectInstanceName" ? "HLAunicodeString"
                                                            : "HLAobjectInstanceHandle";
      if (scalarChildValue(node, "representation") != expectedRepresentation) {
        diagnostics = "Reference data type " + quotedIdentityValue(node, "name=") +
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
      diagnostics = "Reference data type " + quotedIdentityValue(node, "name=") +
                    " refers to attribute " + quoteDiagnosticString(referencedAttribute) +
                    " of object class " + quoteDiagnosticString(referencedClass) + " at " + path +
                    ", but that attribute is not declared by the class or an ancestor in the composed model.";
      valid = false;
      return;
    }
    std::string const representation = scalarChildValue(node, "representation");
    if (!representation.empty() && representation != *attributeType) {
      diagnostics = "Reference data type " + quotedIdentityValue(node, "name=") +
                    " representation " + quoteDiagnosticString(representation) +
                    " does not match referenced attribute " + quoteDiagnosticString(referencedAttribute) +
                    " data type " + quoteDiagnosticString(*attributeType) + " at " + path + ".";
      valid = false;
    }
  });
  return valid;
}

void collectInteractionClassNames(
    SemanticNode const& parent,
    std::string const& parentName,
    std::set<std::string>& names) {
  for (auto const& [key, child] : parent.children) {
    (void)key;
    if (child.localName != "interactionClass") {
      continue;
    }
    std::string const shortName = identityValue(child, "name=");
    if (shortName.empty()) {
      continue;
    }
    std::string const qualifiedName =
        parentName.empty() ? shortName : parentName + "." + shortName;
    names.insert(qualifiedName);
    collectInteractionClassNames(child, qualifiedName, names);
  }
}

bool validateDirectedInteractionReferences(SemanticNode const& root, std::string& diagnostics) {
  std::set<std::string> interactionNames;
  if (auto const* interactions = firstChildNamed(root, "interactions"); interactions != nullptr) {
    collectInteractionClassNames(*interactions, {}, interactionNames);
  }

  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "directedInteraction") {
      return;
    }
    std::string const interactionName = scalarChildValue(node, "name");
    if (!interactionName.empty() && !interactionNames.contains(interactionName)) {
      diagnostics = "Directed interaction " + quoteDiagnosticString(interactionName) + " at " + path +
                    " is not declared in the composed interaction hierarchy.";
      valid = false;
    }
  });
  return valid;
}

bool validateAvailableDimensionReferences(SemanticNode const& root, std::string& diagnostics) {
  std::set<std::string> dimensionNames;
  if (auto const* dimensions = firstChildNamed(root, "dimensions"); dimensions != nullptr) {
    for (auto const& [key, dimension] : dimensions->children) {
      (void)key;
      if (dimension.localName != "dimension") {
        continue;
      }
      std::string const name = identityValue(dimension, "name=");
      if (!name.empty()) {
        dimensionNames.insert(name);
      }
    }
  }

  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "dimensions") {
      return;
    }
    for (auto const& [key, dimension] : node.children) {
      (void)key;
      // IEEE 1516.2-2025's OMT dimensionRef keyref selects only the scalar
      // dimension values below object and interaction classes. Top-level
      // dimension declarations carry a name identity and are not references.
      if (dimension.localName != "dimension" || !dimension.identity.empty() ||
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
  if (value == "Excluded") {
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
  auto const* dimensions = firstChildNamed(root, "dimensions");
  if (dimensions == nullptr) {
    return true;
  }

  bool valid = true;
  for (auto const& [key, dimension] : dimensions->children) {
    (void)key;
    if (!valid || dimension.localName != "dimension") {
      continue;
    }
    std::string const value = scalarChildValue(dimension, "value");
    if (value.empty() || value == "Excluded") {
      continue;
    }

    std::string const upperBoundText = scalarChildValue(dimension, "upperBound");
    auto const upperBound = parseDimensionValueInteger(upperBoundText);
    auto const path = childPath("objectModel/dimensions", dimension);
    if (!upperBound.has_value() || *upperBound == 0) {
      diagnostics = "Dimension " + quotedIdentityValue(dimension, "name=") + " at " + path +
                    " supplies a default value but no positive upper bound.";
      valid = false;
      continue;
    }

    auto const range = parseDimensionValueRange(value, *upperBound);
    if (!range.has_value() || range->lower >= range->upper ||
        range->upper > *upperBound) {
      diagnostics = "Dimension " + quotedIdentityValue(dimension, "name=") + " value " +
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
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "updateRate") {
      return;
    }
    std::string const rateText = scalarChildValue(node, "rate");
    if (rateText.empty()) {
      // DIF permits an incomplete update-rate row; do not invent a value.
      return;
    }
    try {
      std::size_t consumed = 0;
      double const rate = std::stod(rateText, &consumed);
      if (consumed != rateText.size() || !std::isfinite(rate) || rate <= 0.0) {
        diagnostics = "Update rate " + quotedIdentityValue(node, "name=") + " at " + path +
                      " must be a decimal value greater than zero.";
        valid = false;
      }
    } catch (std::exception const&) {
      diagnostics = "Update rate " + quotedIdentityValue(node, "name=") + " at " + path +
                    " must be a decimal value greater than zero.";
      valid = false;
    }
  });
  return valid;
}

bool validateTransportationReferences(SemanticNode const& root, std::string& diagnostics) {
  std::set<std::string> transportationNames;
  if (auto const* transportations = firstChildNamed(root, "transportations");
      transportations != nullptr) {
    for (auto const& [key, transportation] : transportations->children) {
      (void)key;
      if (transportation.localName != "transportation") {
        continue;
      }
      std::string const name = identityValue(transportation, "name=");
      if (!name.empty()) {
        transportationNames.insert(name);
      }
    }
  }

  bool valid = true;
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    // IEEE 1516.2-2025 OMT's transportationRef keyref selects attributes and
    // interaction classes, whose transportation fields resolve through the
    // completed top-level transportation table.
    if (!valid || (node.localName != "attribute" && node.localName != "interactionClass")) {
      return;
    }
    std::string const transportation = scalarChildValue(node, "transportation");
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
      std::string const name = identityValue(classDefinition, "name=");
      if (name == requiredRootName) {
        continue;
      }
      diagnostics = std::string(hierarchyKind) + " class " + quoteDiagnosticString(name) + " at " +
                    childPath("objectModel/" + std::string(sectionName), classDefinition) +
                    " is not nested below " + quoteDiagnosticString(requiredRootName) +
                    "; the completed hierarchy must be rooted by " +
                    quoteDiagnosticString(requiredRootName) + ".";
      return false;
    }
    return true;
  };

  return validate("objects", "objectClass", "HLAobjectRoot", "Object") &&
         validate("interactions", "interactionClass", "HLAinteractionRoot", "Interaction");
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
    if (child.localName != "interactionClass") {
      continue;
    }
    std::string const shortName = identityValue(child, "name=");
    if (shortName.empty()) {
      continue;
    }
    std::string const qualifiedName =
        parentName.empty() ? shortName : parentName + "." + shortName;
    InteractionClassReferenceDefinition definition{parentName, {}};
    for (auto const& [childKey, parameter] : child.children) {
      (void)childKey;
      if (parameter.localName != "parameter") {
        continue;
      }
      std::string const parameterName = identityValue(parameter, "name=");
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
  if (auto const* interactions = firstChildNamed(root, "interactions"); interactions != nullptr) {
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
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "enumeratedData") {
      return;
    }
    std::map<std::string, std::string> valueOwners;
    for (auto const& [key, enumerator] : node.children) {
      (void)key;
      if (enumerator.localName != "enumerator") {
        continue;
      }
      std::string const enumeratorName = identityValue(enumerator, "name=");
      for (auto const& [valueKey, value] : enumerator.children) {
        (void)valueKey;
        if (value.localName != "value" || value.text.empty()) {
          continue;
        }
        auto const [existing, inserted] = valueOwners.emplace(value.text, enumeratorName);
        if (!inserted && existing->second != enumeratorName) {
          diagnostics = "Enumerated data type " + quotedIdentityValue(node, "name=") +
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
    if (child->localName == "enumerator") {
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
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "variantRecordData") {
      return;
    }

    std::string const recordName = identityValue(node, "name=");
    std::string hlaOtherPath;
    for (SemanticNode const* alternative : childrenInDeclarationOrder(node)) {
      if (alternative->localName != "alternative") {
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

        if (expression == "HLAother") {
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
          if (component == "HLAother") {
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
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "variantRecordData") {
      return;
    }
    bool const isExtendable =
        scalarChildValue(node, "encoding") == "HLAextendableVariantRecord";
    std::map<std::string, std::string> enumeratorOwners;
    for (SemanticNode const* alternative : childrenInDeclarationOrder(node)) {
      if (alternative->localName != "alternative") {
        continue;
      }
      std::string const alternativeName = identityValue(*alternative, "name=");
      for (SemanticNode const* enumerator : variantDiscriminantEnumeratorNodes(*alternative)) {
        if (enumerator->text.empty()) {
          continue;
        }
        std::string const expression = trimAsciiWhitespace(enumerator->text);
        if (isExtendable && expression == "HLAother") {
          diagnostics = "Extendable variant record " + quotedIdentityValue(node, "name=") +
                        " uses the prohibited HLAother alternative at " + path + ".";
          valid = false;
          return;
        }
        auto const [existing, inserted] = enumeratorOwners.emplace(expression, alternativeName);
        if (!inserted && existing->second != alternativeName) {
          diagnostics = "Variant record " + quotedIdentityValue(node, "name=") +
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
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const&) {
    if (node.localName != "enumeratedData") {
      return;
    }
    std::string const dataTypeName = identityValue(node, "name=");
    if (dataTypeName.empty()) {
      return;
    }
    auto& enumerators = names[dataTypeName];
    for (SemanticNode const* enumerator : childrenInDeclarationOrder(node)) {
      if (enumerator->localName != "enumerator") {
        continue;
      }
      std::string const enumeratorName = identityValue(*enumerator, "name=");
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
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "variantRecordData") {
      return;
    }

    std::string const dataType = scalarChildValue(node, "dataType");
    auto const namedEnumerators = names.find(dataType);
    // This predicate follows the general resolver and discriminant-type
    // category check. If the selected enumeration has no supplied enumerators,
    // retain incomplete DIF without inventing a closed member set.
    if (dataType.empty() || namedEnumerators == names.end() ||
        namedEnumerators->second.declarationOrder.empty()) {
      return;
    }

    std::string const recordName = identityValue(node, "name=");
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
      if (alternative->localName != "alternative") {
        continue;
      }
      for (SemanticNode const* enumerator : variantDiscriminantEnumeratorNodes(*alternative)) {
        if (enumerator->text.empty()) {
          continue;
        }
        std::string const expression = trimAsciiWhitespace(enumerator->text);
        // The lexical predicate has already accepted the grammar. HLAother is
        // not an enumerator name and therefore has no direct membership check.
        if (expression.empty() || expression == "HLAother") {
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
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "variantRecordData") {
      return;
    }

    std::string const dataType = scalarChildValue(node, "dataType");
    auto const selected = enumerators.find(dataType);
    // Preserve incomplete DIF that does not declare any members for the
    // selected enumeration.  A populated enumerated table is sufficient to
    // expand supplied ranges against its own declaration order.
    if (dataType.empty() || selected == enumerators.end() ||
        selected->second.declarationOrder.empty()) {
      return;
    }

    std::string const recordName = identityValue(node, "name=");
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
      if (alternative->localName != "alternative") {
        continue;
      }
      std::string const alternativeName = identityValue(*alternative, "name=");
      for (SemanticNode const* enumerator : variantDiscriminantEnumeratorNodes(*alternative)) {
        std::string const expression = trimAsciiWhitespace(enumerator->text);
        if (expression.empty()) {
          continue;
        }
        if (expression == "HLAother") {
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
  if (value == "Dynamic") {
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
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "arrayData") {
      return;
    }
    std::string const cardinality = scalarChildValue(node, "cardinality");
    if (cardinality.empty() || validArrayCardinality(cardinality)) {
      return;
    }
    diagnostics = "Array data type " + quotedIdentityValue(node, "name=") + " at " + path +
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
  if (value == "Dynamic") {
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
  walkNodes(root, "objectModel", [&](SemanticNode const& node, std::string const& path) {
    if (!valid || node.localName != "arrayData") {
      return;
    }
    std::string const encoding = trimAsciiWhitespace(scalarChildValue(node, "encoding"));
    if (encoding != "HLAfixedArray" && encoding != "HLAvariableArray") {
      return;
    }
    auto const cardinality = oneDimensionalArrayCardinalityIsVariable(
        scalarChildValue(node, "cardinality"));
    if (!cardinality.has_value()) {
      return;
    }
    bool const encodingIsVariable = encoding == "HLAvariableArray";
    if (encodingIsVariable == *cardinality) {
      return;
    }
    diagnostics = "Array data type " + quotedIdentityValue(node, "name=") + " at " + path +
                  " pairs " + quoteDiagnosticString(encoding) + " with a " +
                  (*cardinality ? "variable" : "fixed") +
                  " one-dimensional cardinality; the predefined encoding must be " +
                  (*cardinality ? "HLAvariableArray" : "HLAfixedArray") + ".";
    valid = false;
  });
  return valid;
}

xmlNode const* directChildElement(xmlNode const* parent, std::string_view desiredName) {
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

NoteLabelMap makeNoteLabelMap(xmlDoc const* document, std::size_t& nextLabel) {
  NoteLabelMap labels;
  xmlNode const* root = xmlDocGetRootElement(const_cast<xmlDoc*>(document));
  xmlNode const* notes = directChildElement(root, "notes");
  if (notes == nullptr) {
    return labels;
  }
  for (xmlNode const* note = notes->children; note != nullptr; note = note->next) {
    if (note->type != XML_ELEMENT_NODE || localName(note) != "note") {
      continue;
    }
    std::string const label = directChildText(note, "label");
    if (!label.empty()) {
      labels.emplace(label, "UmbraNote" + std::to_string(nextLabel++));
    }
  }
  return labels;
}

std::string utf8FromWide(std::wstring const& value) {
  auto const encoded = std::filesystem::path(value).u8string();
  std::string result;
  result.reserve(encoded.size());
  for (auto const character : encoded) {
    result.push_back(static_cast<char>(character));
  }
  return result;
}

std::string moduleName(
    xmlDoc const* document,
    PrevalidatedFomModule const& module,
    std::size_t moduleIndex) {
  xmlNode const* root = xmlDocGetRootElement(const_cast<xmlDoc*>(document));
  if (xmlNode const* identification = directChildElement(root, "modelIdentification");
      identification != nullptr) {
    if (std::string const name = directChildText(identification, "name"); !name.empty()) {
      return name;
    }
  }
  if (!module.designator.empty()) {
    return utf8FromWide(module.designator);
  }
  return "UmbraModule" + std::to_string(moduleIndex + 1);
}

bool mergeServiceAttributes(
    SemanticNode& current,
    SemanticNode const& candidate,
    std::string const& path,
    std::string& diagnostics) {
  for (auto const& [name, candidateValue] : candidate.attributes) {
    auto const existing = current.attributes.find(name);
    if (isNoteReferencesAttribute(name)) {
      if (existing == current.attributes.end()) {
        current.attributes.emplace(name, candidateValue);
      } else {
        existing->second = unionNoteReferences(existing->second, candidateValue);
      }
      continue;
    }
    if (name == "isUsed") {
      continue;
    }
    if (existing != current.attributes.end() && !existing->second.empty() &&
        !candidateValue.empty() && existing->second != candidateValue) {
      diagnostics = "Conflicting service-utilization attribute " + quoteDiagnosticString(name) +
                    " at " + path + ".";
      return false;
    }
    if (existing == current.attributes.end() || existing->second.empty()) {
      current.attributes[name] = candidateValue;
    }
  }
  return true;
}

bool isUsed(SemanticNode const& service) {
  auto const value = service.attributes.find("isUsed");
  return value != service.attributes.end() &&
         (value->second == "true" || value->second == "1");
}

bool mergeServiceUtilization(
    SemanticNode& current,
    SemanticNode const& candidate,
    std::string& diagnostics) {
  if (!mergeServiceAttributes(current, candidate, "objectModel/serviceUtilization", diagnostics)) {
    return false;
  }
  for (SemanticNode const* candidateService : childrenInDeclarationOrder(candidate)) {
    NodeKey const key = childKey(current, *candidateService);
    auto existing = current.children.find(key);
    if (existing == current.children.end()) {
      SemanticNode inserted = *candidateService;
      inserted.attributes["isUsed"] = isUsed(*candidateService) ? "true" : "false";
      appendChild(current, key, std::move(inserted));
      continue;
    }
    if (!mergeServiceAttributes(
            existing->second,
            *candidateService,
            childPath("objectModel/serviceUtilization", *candidateService),
            diagnostics)) {
      return false;
    }
    if (!existing->second.text.empty() && !candidateService->text.empty() &&
        existing->second.text != candidateService->text) {
      diagnostics = "Conflicting service-utilization value at " +
                    childPath("objectModel/serviceUtilization", *candidateService) + ".";
      return false;
    }
    existing->second.attributes["isUsed"] =
        (isUsed(existing->second) || isUsed(*candidateService)) ? "true" : "false";
  }
  return true;
}

void collectNoteReferences(SemanticNode const& node, std::set<std::string>& references) {
  auto const attribute = node.attributes.find("noteReferences");
  if (attribute != node.attributes.end()) {
    for (std::string const& reference : splitWhitespaceSeparated(attribute->second)) {
      references.insert(reference);
    }
  }
  for (auto const& [key, child] : node.children) {
    (void)key;
    collectNoteReferences(child, references);
  }
}

std::optional<SemanticNode> selectedNotes(
    SemanticNode const& allNotes,
    std::set<std::string> const& referencedLabels) {
  SemanticNode result{"notes", allNotes.namespaceName, {}, {}, allNotes.attributes, {}};
  for (auto const& [key, note] : allNotes.children) {
    if (note.localName != "note" ||
        !referencedLabels.contains(identityValue(note, "label="))) {
      continue;
    }
    appendChild(result, key, note);
  }
  if (result.children.empty()) {
    return std::nullopt;
  }
  return result;
}

int sequenceRank(
    std::string_view parent,
    std::string_view child) {
  auto rank = [child](std::initializer_list<std::string_view> order) {
    int value = 0;
    for (std::string_view const candidate : order) {
      if (candidate == child) {
        return value;
      }
      ++value;
    }
    return 1000;
  };

  if (parent == "objectModel") {
    return rank({"modelIdentification", "serviceUtilization", "objects", "interactions", "dimensions",
                 "time", "tags", "synchronizations", "transportations", "switches", "updateRates",
                 "dataTypes", "notes"});
  }
  if (parent == "modelIdentification") {
    return rank({"name", "type", "version", "modificationDate", "securityClassification", "copyright",
                 "releaseRestriction", "purpose", "applicationDomain", "description", "useLimitation",
                 "useHistory", "keyword", "poc", "reference", "other", "glyph"});
  }
  if (parent == "keyword") {
    return rank({"taxonomy", "keywordValue"});
  }
  if (parent == "poc") {
    return rank({"pocType", "pocName", "pocOrg", "pocTelephone", "pocEmail"});
  }
  if (parent == "reference") {
    return rank({"type", "identification"});
  }
  if (parent == "objects") {
    return rank({"objectClass"});
  }
  if (parent == "objectClass") {
    return rank({"name", "sharing", "directedInteraction", "dimensions", "semantics", "attribute",
                 "objectClass"});
  }
  if (parent == "attribute") {
    return rank({"name", "dataType", "updateType", "updateCondition", "valueRequired", "ownership",
                 "sharing", "transportation", "order", "semantics"});
  }
  if (parent == "directedInteraction") {
    return rank({"name", "sharing"});
  }
  if (parent == "interactions") {
    return rank({"interactionClass"});
  }
  if (parent == "interactionClass") {
    return rank({"name", "sharing", "dimensions", "transportation", "order", "semantics", "parameter",
                 "interactionClass"});
  }
  if (parent == "parameter") {
    return rank({"name", "dataType", "semantics"});
  }
  if (parent == "dimensions") {
    return rank({"dimension"});
  }
  if (parent == "dimension") {
    return rank({"name", "inputDataTypes", "inputDataDescription", "upperBound", "normalization",
                 "outputDataSemantics", "value"});
  }
  if (parent == "inputDataTypes") {
    return rank({"dataType"});
  }
  if (parent == "time") {
    return rank({"logicalTime", "logicalTimeInterval"});
  }
  if (parent == "logicalTime" || parent == "logicalTimeInterval" ||
      parent == "updateReflectTag" || parent == "sendReceiveTag" ||
      parent == "deleteRemoveTag" || parent == "divestitureRequestTag" ||
      parent == "divestitureCompletionTag" || parent == "acquisitionRequestTag" ||
      parent == "requestUpdateTag") {
    return rank({"dataType", "semantics"});
  }
  if (parent == "tags") {
    return rank({"updateReflectTag", "sendReceiveTag", "deleteRemoveTag", "divestitureRequestTag",
                 "divestitureCompletionTag", "acquisitionRequestTag", "requestUpdateTag"});
  }
  if (parent == "synchronizations") {
    return rank({"synchronizationPoint"});
  }
  if (parent == "synchronizationPoint") {
    return rank({"label", "dataType", "capability", "semantics"});
  }
  if (parent == "transportations") {
    return rank({"transportation"});
  }
  if (parent == "transportation") {
    return rank({"name", "reliable", "semantics"});
  }
  if (parent == "switches") {
    return rank({"autoProvide", "conveyRegionDesignatorSets", "attributeScopeAdvisory",
                 "attributeRelevanceAdvisory", "objectClassRelevanceAdvisory",
                 "interactionRelevanceAdvisory", "serviceReporting", "exceptionReporting",
                 "delaySubscriptionEvaluation", "nonRegulatedGrant", "automaticResignAction",
                 "allowRelaxedDDM", "advisoriesUseKnownClass", "sendServiceReportsToFile"});
  }
  if (parent == "updateRates") {
    return rank({"updateRate"});
  }
  if (parent == "updateRate") {
    return rank({"name", "rate", "semantics"});
  }
  if (parent == "dataTypes") {
    return rank({"basicDataRepresentations", "simpleDataTypes", "referenceDataTypes", "enumeratedDataTypes",
                 "arrayDataTypes", "fixedRecordDataTypes", "variantRecordDataTypes"});
  }
  if (parent == "basicDataRepresentations") {
    return rank({"basicData"});
  }
  if (parent == "basicData") {
    return rank({"name", "size", "interpretation", "endian", "encoding"});
  }
  if (parent == "simpleDataTypes") {
    return rank({"simpleData"});
  }
  if (parent == "simpleData") {
    return rank({"name", "representation", "units", "resolution", "accuracy", "semantics"});
  }
  if (parent == "referenceDataTypes") {
    return rank({"referenceDataType"});
  }
  if (parent == "referenceDataType") {
    return rank({"name", "representation", "referenceClass", "referencedAttribute", "semantics"});
  }
  if (parent == "enumeratedDataTypes") {
    return rank({"enumeratedData"});
  }
  if (parent == "enumeratedData") {
    return rank({"name", "representation", "semantics", "enumerator"});
  }
  if (parent == "enumerator") {
    return rank({"name", "value"});
  }
  if (parent == "arrayDataTypes") {
    return rank({"arrayData"});
  }
  if (parent == "arrayData") {
    return rank({"name", "dataType", "cardinality", "encoding", "semantics"});
  }
  if (parent == "fixedRecordDataTypes") {
    return rank({"fixedRecordData"});
  }
  if (parent == "fixedRecordData") {
    return rank({"name", "encoding", "semantics", "field"});
  }
  if (parent == "field") {
    return rank({"name", "dataType", "semantics"});
  }
  if (parent == "variantRecordDataTypes") {
    return rank({"variantRecordData"});
  }
  if (parent == "variantRecordData") {
    return rank({"name", "discriminant", "dataType", "alternative", "encoding", "semantics"});
  }
  if (parent == "alternative") {
    return rank({"enumerator", "name", "dataType", "semantics"});
  }
  if (parent == "notes") {
    return rank({"note"});
  }
  if (parent == "note") {
    return rank({"label", "semantics"});
  }
  return 1000;
}

struct XmlDocumentDeleter {
  void operator()(xmlDoc* document) const noexcept {
    xmlFreeDoc(document);
  }
};

struct XmlSchemaParserContextDeleter {
  void operator()(xmlSchemaParserCtxt* context) const noexcept {
    xmlSchemaFreeParserCtxt(context);
  }
};

struct XmlSchemaDeleter {
  void operator()(xmlSchema* schema) const noexcept {
    xmlSchemaFree(schema);
  }
};

struct XmlSchemaValidationContextDeleter {
  void operator()(xmlSchemaValidCtxt* context) const noexcept {
    xmlSchemaFreeValidCtxt(context);
  }
};

class XmlErrorCollector final {
 public:
  static void collect(void* userData, xmlError const* error) {
    if (userData == nullptr || error == nullptr || error->message == nullptr) {
      return;
    }
    auto* collector = static_cast<XmlErrorCollector*>(userData);
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

std::string pathAsUtf8(std::filesystem::path const& path) {
  auto const encoded = path.u8string();
  std::string result;
  result.reserve(encoded.size());
  for (auto const byte : encoded) {
    result.push_back(static_cast<char>(byte));
  }
  return result;
}

enum class FddMaterializationStatus {
  valid,
  invalid_model,
  validator_failure,
};

struct FddMaterializationResult {
  FddMaterializationStatus status = FddMaterializationStatus::validator_failure;
  std::shared_ptr<MaterializedFdd const> fdd;
  std::string diagnostics;
};

bool appendSemanticNode(
    xmlDoc* document,
    xmlNode* parent,
    xmlNs* hlaNamespace,
    SemanticNode const& semantic,
    std::string& diagnostics) {
  if (semantic.namespaceName != kHla2025Namespace) {
    diagnostics = "The FDD materializer does not accept an extension element outside the IEEE 1516-2025 namespace: " +
                  quoteDiagnosticString(semantic.localName) + ".";
    return false;
  }
  xmlNode* output = xmlNewChild(
      parent,
      hlaNamespace,
      BAD_CAST semantic.localName.c_str(),
      nullptr);
  if (output == nullptr) {
    diagnostics = "Cannot allocate an FDD XML element for " +
                  quoteDiagnosticString(semantic.localName) + ".";
    return false;
  }
  if (!semantic.text.empty()) {
    xmlNodeSetContent(output, BAD_CAST semantic.text.c_str());
  }
  for (auto const& [name, value] : semantic.attributes) {
    if (!name.empty() && name.front() == '{') {
      diagnostics = "The FDD materializer does not accept a namespaced attribute on " +
                    quoteDiagnosticString(semantic.localName) + ".";
      return false;
    }
    if (xmlNewProp(output, BAD_CAST name.c_str(), BAD_CAST value.c_str()) == nullptr) {
      diagnostics = "Cannot allocate an FDD XML attribute on " +
                    quoteDiagnosticString(semantic.localName) + ".";
      return false;
    }
  }

  std::vector<SemanticNode const*> children;
  children.reserve(semantic.children.size());
  for (auto const& [key, child] : semantic.children) {
    (void)key;
    children.push_back(&child);
  }
  std::stable_sort(
      children.begin(),
      children.end(),
      [&semantic](SemanticNode const* left, SemanticNode const* right) {
        return sequenceRank(semantic.localName, left->localName) <
               sequenceRank(semantic.localName, right->localName);
      });
  for (SemanticNode const* child : children) {
    if (!appendSemanticNode(document, output, hlaNamespace, *child, diagnostics)) {
      return false;
    }
  }
  return true;
}

FddMaterializationStatus validateFddDocument(
    xmlDoc* document,
    std::filesystem::path const& schemaPath,
    std::string& diagnostics) {
  if (schemaPath.empty()) {
    diagnostics = "A materialized FDD requires an explicitly selected FDD schema.";
    return FddMaterializationStatus::validator_failure;
  }
  std::error_code error;
  if (!std::filesystem::is_regular_file(schemaPath, error)) {
    diagnostics = error ? "Cannot inspect the FDD schema: " + error.message()
                        : "Cannot find the FDD schema.";
    return FddMaterializationStatus::validator_failure;
  }
  std::filesystem::path const canonicalSchema = std::filesystem::canonical(schemaPath, error);
  if (error) {
    diagnostics = "Cannot canonicalize the FDD schema: " + error.message();
    return FddMaterializationStatus::validator_failure;
  }

  XmlErrorCollector errors;
  std::unique_ptr<xmlSchemaParserCtxt, XmlSchemaParserContextDeleter> parser(
      xmlSchemaNewParserCtxt(pathAsUtf8(canonicalSchema).c_str()));
  if (!parser) {
    diagnostics = "Cannot allocate an FDD schema parser context.";
    return FddMaterializationStatus::validator_failure;
  }
  xmlSchemaSetParserStructuredErrors(parser.get(), XmlErrorCollector::collect, &errors);
  std::unique_ptr<xmlSchema, XmlSchemaDeleter> schema(xmlSchemaParse(parser.get()));
  if (!schema) {
    diagnostics = errors.message().empty() ? "The selected FDD schema cannot be parsed."
                                           : errors.message();
    return FddMaterializationStatus::validator_failure;
  }
  std::unique_ptr<xmlSchemaValidCtxt, XmlSchemaValidationContextDeleter> validation(
      xmlSchemaNewValidCtxt(schema.get()));
  if (!validation) {
    diagnostics = "Cannot allocate an FDD schema validation context.";
    return FddMaterializationStatus::validator_failure;
  }
  xmlSchemaSetValidStructuredErrors(validation.get(), XmlErrorCollector::collect, &errors);
  int const result = xmlSchemaValidateDoc(validation.get(), document);
  if (result != 0) {
    diagnostics = errors.message().empty() ? "The composed FDD does not validate against the FDD schema."
                                           : errors.message();
    return result > 0 ? FddMaterializationStatus::invalid_model
                      : FddMaterializationStatus::validator_failure;
  }
  return FddMaterializationStatus::valid;
}

FddMaterializationResult materializeFdd(
    SemanticNode const& merged,
    std::optional<SemanticNode> const& serviceUtilization,
    std::optional<SemanticNode> const& notes,
    std::vector<std::string> composedFromModuleNames,
    std::filesystem::path const& fddSchemaPath) {
  std::unique_ptr<xmlDoc, XmlDocumentDeleter> document(xmlNewDoc(BAD_CAST "1.0"));
  if (!document) {
    return {
        FddMaterializationStatus::validator_failure,
        {},
        "Cannot allocate a composed FDD XML document.",
    };
  }
  xmlNode* root = xmlNewNode(nullptr, BAD_CAST "objectModel");
  if (root == nullptr) {
    return {
        FddMaterializationStatus::validator_failure,
        {},
        "Cannot allocate the composed FDD root element.",
    };
  }
  xmlDocSetRootElement(document.get(), root);
  xmlNs* hlaNamespace = xmlNewNs(root, BAD_CAST kHla2025Namespace, nullptr);
  xmlNs* schemaInstanceNamespace =
      xmlNewNs(root, BAD_CAST kXmlSchemaInstanceNamespace, BAD_CAST "xsi");
  if (hlaNamespace == nullptr || schemaInstanceNamespace == nullptr) {
    return {
        FddMaterializationStatus::validator_failure,
        {},
        "Cannot allocate namespaces for the composed FDD.",
    };
  }
  xmlSetNs(root, hlaNamespace);
  if (xmlSetNsProp(
          root,
          schemaInstanceNamespace,
          BAD_CAST "schemaLocation",
          BAD_CAST kFddSchemaLocation) == nullptr) {
    return {
        FddMaterializationStatus::validator_failure,
        {},
        "Cannot set the FDD schema-location attribute.",
    };
  }

  xmlNode* identification = xmlNewChild(root, hlaNamespace, BAD_CAST "modelIdentification", nullptr);
  if (identification == nullptr) {
    return {
        FddMaterializationStatus::validator_failure,
        {},
        "Cannot allocate composed-FDD model identification.",
    };
  }
  for (std::string const& moduleName : composedFromModuleNames) {
    xmlNode* reference = xmlNewChild(identification, hlaNamespace, BAD_CAST "reference", nullptr);
    if (reference == nullptr ||
        xmlNewChild(reference, hlaNamespace, BAD_CAST "type", BAD_CAST "Composed_From") == nullptr ||
        xmlNewChild(
            reference,
            hlaNamespace,
            BAD_CAST "identification",
            BAD_CAST moduleName.c_str()) == nullptr) {
      return {
          FddMaterializationStatus::validator_failure,
          {},
          "Cannot allocate composed-FDD module references.",
      };
    }
  }

  std::string diagnostics;
  if (serviceUtilization.has_value() &&
      !appendSemanticNode(document.get(), root, hlaNamespace, *serviceUtilization, diagnostics)) {
    return {FddMaterializationStatus::invalid_model, {}, std::move(diagnostics)};
  }
  std::vector<SemanticNode const*> sections;
  sections.reserve(merged.children.size());
  for (auto const& [key, section] : merged.children) {
    (void)key;
    sections.push_back(&section);
  }
  std::stable_sort(
      sections.begin(),
      sections.end(),
      [](SemanticNode const* left, SemanticNode const* right) {
        return sequenceRank("objectModel", left->localName) <
               sequenceRank("objectModel", right->localName);
      });
  for (SemanticNode const* section : sections) {
    if (!appendSemanticNode(document.get(), root, hlaNamespace, *section, diagnostics)) {
      return {FddMaterializationStatus::invalid_model, {}, std::move(diagnostics)};
    }
  }
  if (notes.has_value() &&
      !appendSemanticNode(document.get(), root, hlaNamespace, *notes, diagnostics)) {
    return {FddMaterializationStatus::invalid_model, {}, std::move(diagnostics)};
  }

  FddMaterializationStatus const validation =
      validateFddDocument(document.get(), fddSchemaPath, diagnostics);
  if (validation != FddMaterializationStatus::valid) {
    return {validation, {}, std::move(diagnostics)};
  }

  xmlChar* encoded = nullptr;
  int size = 0;
  xmlDocDumpFormatMemoryEnc(document.get(), &encoded, &size, "UTF-8", 1);
  if (encoded == nullptr || size < 0) {
    if (encoded != nullptr) {
      xmlFree(encoded);
    }
    return {
        FddMaterializationStatus::validator_failure,
        {},
        "Cannot serialize the composed FDD.",
    };
  }
  auto fdd = std::make_shared<MaterializedFdd>(
      std::string(reinterpret_cast<char const*>(encoded), static_cast<std::size_t>(size)),
      std::move(composedFromModuleNames));
  xmlFree(encoded);
  return {FddMaterializationStatus::valid, std::move(fdd), {}};
}

FomCompositionStatus statusFor(FomValidationStatus status) {
  switch (status) {
    case FomValidationStatus::valid:
      return FomCompositionStatus::valid;
    case FomValidationStatus::source_not_found:
      return FomCompositionStatus::source_not_found;
    case FomValidationStatus::source_unreadable:
      return FomCompositionStatus::source_unreadable;
    case FomValidationStatus::source_parse_error:
      return FomCompositionStatus::source_parse_error;
    case FomValidationStatus::invalid_model:
      return FomCompositionStatus::invalid_model;
    case FomValidationStatus::validator_failure:
      return FomCompositionStatus::validator_failure;
  }
  return FomCompositionStatus::validator_failure;
}

}  // namespace

class FomCatalogBuilder final {
 public:
  [[nodiscard]] static std::shared_ptr<FomCatalog const> build(
      SemanticNode const& root,
      std::vector<PrevalidatedFomModule> const& modules) {
    auto catalog = std::make_shared<FomCatalog>();
    catalog->modules_ = modules;

    if (auto const* objects = firstChildNamed(root, "objects"); objects != nullptr) {
      for (auto const& [key, objectClass] : objects->children) {
        (void)key;
        if (objectClass.localName == "objectClass") {
          appendObjectClass(*catalog, objectClass, {});
        }
      }
    }
    if (auto const* interactions = firstChildNamed(root, "interactions"); interactions != nullptr) {
      for (auto const& [key, interactionClass] : interactions->children) {
        (void)key;
        if (interactionClass.localName == "interactionClass") {
          appendInteractionClass(*catalog, interactionClass, {});
        }
      }
    }
    if (auto const* dimensions = firstChildNamed(root, "dimensions"); dimensions != nullptr) {
      for (auto const& [key, dimension] : dimensions->children) {
        (void)key;
        if (dimension.localName == "dimension") {
          appendDimension(*catalog, dimension);
        }
      }
    }
    if (auto const* dataTypes = firstChildNamed(root, "dataTypes"); dataTypes != nullptr) {
      appendDataTypes(*catalog, *dataTypes);
    }
    if (auto const* updateRates = firstChildNamed(root, "updateRates");
        updateRates != nullptr) {
      for (auto const& [key, updateRate] : updateRates->children) {
        (void)key;
        if (updateRate.localName != "updateRate") {
          continue;
        }
        std::string const name = identityValue(updateRate, "name=");
        std::string const rateText = scalarChildValue(updateRate, "rate");
        if (name.empty() || rateText.empty()) {
          continue;
        }
        try {
          std::size_t consumed = 0;
          double const rate = std::stod(rateText, &consumed);
          if (consumed == rateText.size() && std::isfinite(rate) && rate >= 0.0) {
            catalog->updateRates_.insert_or_assign(name, rate);
          }
        } catch (std::exception const&) {
          // The DIF/FDD schema already validates the lexical form. Keep the
          // private catalog defensive if an unchecked SemanticNode is used by
          // a future caller.
        }
      }
    }
    if (auto const* time = firstChildNamed(root, "time"); time != nullptr) {
      if (auto const* logicalTime = firstChildNamed(*time, "logicalTime"); logicalTime != nullptr) {
        catalog->time_.logicalTimeDataType = scalarChildValue(*logicalTime, "dataType");
      }
      if (auto const* interval = firstChildNamed(*time, "logicalTimeInterval"); interval != nullptr) {
        catalog->time_.logicalTimeIntervalDataType = scalarChildValue(*interval, "dataType");
      }
    }
    if (auto const* switches = firstChildNamed(root, "switches"); switches != nullptr) {
      if (auto const* autoProvide = firstChildNamed(*switches, "autoProvide");
          autoProvide != nullptr) {
        catalog->federationSwitches_.autoProvide = isEnabledSwitch(*autoProvide);
      }
      if (auto const* conveyRegionDesignatorSets =
              firstChildNamed(*switches, "conveyRegionDesignatorSets");
          conveyRegionDesignatorSets != nullptr) {
        catalog->federateSupportSwitches_.conveyRegionDesignatorSets =
            isEnabledSwitch(*conveyRegionDesignatorSets);
      }
      if (auto const* automaticResignAction =
              firstChildNamed(*switches, "automaticResignAction");
          automaticResignAction != nullptr) {
        catalog->federateSupportSwitches_.automaticResignAction =
            resignActionValue(*automaticResignAction);
      }
      if (auto const* serviceReporting = firstChildNamed(*switches, "serviceReporting");
          serviceReporting != nullptr) {
        catalog->federateSupportSwitches_.serviceReporting =
            isEnabledSwitch(*serviceReporting);
      }
      if (auto const* exceptionReporting =
              firstChildNamed(*switches, "exceptionReporting");
          exceptionReporting != nullptr) {
        catalog->federateSupportSwitches_.exceptionReporting =
            isEnabledSwitch(*exceptionReporting);
      }
      if (auto const* sendServiceReportsToFile =
              firstChildNamed(*switches, "sendServiceReportsToFile");
          sendServiceReportsToFile != nullptr) {
        catalog->federateSupportSwitches_.sendServiceReportsToFile =
            isEnabledSwitch(*sendServiceReportsToFile);
      }
      if (auto const* delaySubscriptionEvaluation =
              firstChildNamed(*switches, "delaySubscriptionEvaluation");
          delaySubscriptionEvaluation != nullptr) {
        catalog->federationSwitches_.delaySubscriptionEvaluation =
            isEnabledSwitch(*delaySubscriptionEvaluation);
      }
      if (auto const* allowRelaxedDDM = firstChildNamed(*switches, "allowRelaxedDDM");
          allowRelaxedDDM != nullptr) {
        catalog->federationSwitches_.allowRelaxedDDM = isEnabledSwitch(*allowRelaxedDDM);
      }
      if (auto const* attributeScopeAdvisory =
              firstChildNamed(*switches, "attributeScopeAdvisory");
          attributeScopeAdvisory != nullptr) {
        catalog->advisorySwitches_.attributeScopeAdvisory =
            isEnabledSwitch(*attributeScopeAdvisory);
      }
      if (auto const* attributeRelevanceAdvisory =
              firstChildNamed(*switches, "attributeRelevanceAdvisory");
          attributeRelevanceAdvisory != nullptr) {
        catalog->advisorySwitches_.attributeRelevanceAdvisory =
            isEnabledSwitch(*attributeRelevanceAdvisory);
      }
      if (auto const* objectClassRelevanceAdvisory =
              firstChildNamed(*switches, "objectClassRelevanceAdvisory");
          objectClassRelevanceAdvisory != nullptr) {
        catalog->advisorySwitches_.objectClassRelevanceAdvisory =
            isEnabledSwitch(*objectClassRelevanceAdvisory);
      }
      if (auto const* interactionRelevanceAdvisory =
              firstChildNamed(*switches, "interactionRelevanceAdvisory");
          interactionRelevanceAdvisory != nullptr) {
        catalog->advisorySwitches_.interactionRelevanceAdvisory =
            isEnabledSwitch(*interactionRelevanceAdvisory);
      }
      if (auto const* advisoriesUseKnownClass =
              firstChildNamed(*switches, "advisoriesUseKnownClass");
          advisoriesUseKnownClass != nullptr) {
        catalog->advisorySwitches_.advisoriesUseKnownClass =
            isEnabledSwitch(*advisoriesUseKnownClass);
      }
      if (auto const* nonRegulatedGrant = firstChildNamed(*switches, "nonRegulatedGrant");
          nonRegulatedGrant != nullptr) {
        catalog->timeManagementSwitches_.nonRegulatedGrant = isEnabledSwitch(*nonRegulatedGrant);
      }
    }
    return catalog;
  }

 private:
  static void appendObjectClass(
      FomCatalog& catalog,
      SemanticNode const& node,
      std::string const& parentName) {
    std::string const shortName = identityValue(node, "name=");
    if (shortName.empty()) {
      return;
    }
    std::string const name = parentName.empty() ? shortName : parentName + "." + shortName;
    FomObjectClassDefinition definition{name, parentName, {}, {}, {}};
    for (auto const& [key, child] : node.children) {
      (void)key;
      if (child.localName == "dimensions") {
        for (auto const& [dimensionKey, dimension] : child.children) {
          (void)dimensionKey;
          if (dimension.localName == "dimension" && !dimension.text.empty()) {
            definition.dimensions.push_back(dimension.text);
          }
        }
      } else if (child.localName == "attribute") {
        std::string const attributeName = identityValue(child, "name=");
        if (!attributeName.empty()) {
          std::string const valueRequired = scalarChildValue(child, "valueRequired");
          definition.declaredAttributes.emplace(
              attributeName,
              FomAttributeDefinition{
                  attributeName,
                  scalarChildValue(child, "dataType"),
                  scalarChildValue(child, "updateType"),
                  scalarChildValue(child, "updateCondition"),
                  valueRequired == "true" || valueRequired == "1",
                  scalarChildValue(child, "ownership"),
                  scalarChildValue(child, "sharing"),
                  scalarChildValue(child, "transportation"),
                  scalarChildValue(child, "order"),
              });
        }
      } else if (child.localName == "directedInteraction") {
        std::string const interactionName = identityValue(child, "name=");
        if (!interactionName.empty()) {
          definition.directedInteractions.push_back(interactionName);
        }
      }
    }
    catalog.objectClasses_.insert_or_assign(name, std::move(definition));

    for (auto const& [key, child] : node.children) {
      (void)key;
      if (child.localName == "objectClass") {
        appendObjectClass(catalog, child, name);
      }
    }
  }

  static void appendInteractionClass(
      FomCatalog& catalog,
      SemanticNode const& node,
      std::string const& parentName) {
    std::string const shortName = identityValue(node, "name=");
    if (shortName.empty()) {
      return;
    }
    std::string const name = parentName.empty() ? shortName : parentName + "." + shortName;
    FomInteractionClassDefinition definition{
        name,
        parentName,
        {},
        scalarChildValue(node, "transportation"),
        scalarChildValue(node, "order"),
        {},
    };
    for (auto const& [key, child] : node.children) {
      (void)key;
      if (child.localName == "dimensions") {
        for (auto const& [dimensionKey, dimension] : child.children) {
          (void)dimensionKey;
          if (dimension.localName == "dimension" && !dimension.text.empty()) {
            definition.dimensions.push_back(dimension.text);
          }
        }
        continue;
      }
      if (child.localName != "parameter") {
        continue;
      }
      std::string const parameterName = identityValue(child, "name=");
      if (!parameterName.empty()) {
        definition.declaredParameters.emplace(
            parameterName,
            FomInteractionParameterDefinition{parameterName, scalarChildValue(child, "dataType")});
      }
    }
    catalog.interactionClasses_.insert_or_assign(name, std::move(definition));

    for (auto const& [key, child] : node.children) {
      (void)key;
      if (child.localName == "interactionClass") {
        appendInteractionClass(catalog, child, name);
      }
    }
  }

  static void appendDimension(FomCatalog& catalog, SemanticNode const& node) {
    std::string const name = identityValue(node, "name=");
    if (name.empty()) {
      return;
    }
    // 1516.2 permits dimensions such as the MIM HLAfederate dimension to omit
    // upperBound.  That denotes the full unsigned-long domain, not a zero
    // sized domain; retaining zero here made every otherwise-valid point
    // range for Normalize Federate Handle fail at commit time.
    auto const upperBoundText = scalarChildValue(node, "upperBound");
    auto const upperBound = upperBoundText.empty()
        ? std::numeric_limits<unsigned long>::max()
        : parseUnsignedLong(upperBoundText);
    FomDimensionDefinition definition{name, {}, upperBound};
    if (auto const* inputDataTypes = firstChildNamed(node, "inputDataTypes"); inputDataTypes != nullptr) {
      for (auto const& [key, dataType] : inputDataTypes->children) {
        (void)key;
        if (dataType.localName == "dataType" && !dataType.text.empty()) {
          definition.inputDataTypes.push_back(dataType.text);
        }
      }
    }
    catalog.dimensions_.insert_or_assign(name, std::move(definition));
  }

  static FomDataTypeKind dataTypeKind(std::string_view localName) {
    if (localName == "basicData") {
      return FomDataTypeKind::basic;
    }
    if (localName == "simpleData") {
      return FomDataTypeKind::simple;
    }
    if (localName == "referenceDataType") {
      return FomDataTypeKind::reference;
    }
    if (localName == "enumeratedData") {
      return FomDataTypeKind::enumerated;
    }
    if (localName == "arrayData") {
      return FomDataTypeKind::array;
    }
    if (localName == "fixedRecordData") {
      return FomDataTypeKind::fixed_record;
    }
    return FomDataTypeKind::variant_record;
  }

  static void appendDataTypes(FomCatalog& catalog, SemanticNode const& dataTypes) {
    walkNodes(dataTypes, "dataTypes", [&](SemanticNode const& node, std::string const&) {
      if (!isDataTypeDeclaration(node.localName)) {
        return;
      }
      std::string const name = identityValue(node, "name=");
      if (name.empty()) {
        return;
      }
      std::string representation = scalarChildValue(node, "representation");
      if (representation.empty()) {
        representation = scalarChildValue(node, "dataType");
      }
      catalog.dataTypes_.insert_or_assign(
          name,
          FomDataTypeDefinition{name, dataTypeKind(node.localName), std::move(representation)});
    });
  }
};

LibXml2FomModuleComposer::LibXml2FomModuleComposer(std::filesystem::path fddSchemaPath)
    : fddSchemaPath_(std::move(fddSchemaPath)) {}

FomCompositionResult LibXml2FomModuleComposer::compose(
    std::vector<PrevalidatedFomModule> const& modules) const {
  if (modules.empty()) {
    return {FomCompositionStatus::invalid_model, {}, "At least one FOM or MIM module is required."};
  }

  SemanticNode merged{"objectModel", {}, {}, {}, {}, {}};
  SemanticNode mergedNotes{"notes", kHla2025Namespace, {}, {}, {}, {}};
  bool haveNotes = false;
  std::optional<SemanticNode> mergedServiceUtilization;
  std::vector<PrevalidatedFomModule> revalidated;
  revalidated.reserve(modules.size());
  std::vector<std::string> moduleNames;
  moduleNames.reserve(modules.size());
  std::vector<std::string> warnings;
  std::size_t nextNoteLabel = 1;

  for (std::size_t moduleIndex = 0; moduleIndex < modules.size(); ++moduleIndex) {
    PrevalidatedFomModule const& module = modules[moduleIndex];
    LibXml2ValidatedFomDocument parsed;
    FomValidationResult const validation = loadValidatedLibXml2FomDocument(
        {
            module.sourcePath,
            module.schemaPath,
            module.kind,
            module.designator,
            module.schemaDesignator,
        },
        parsed);
    if (validation.status != FomValidationStatus::valid) {
      return {statusFor(validation.status), {}, validation.diagnostics};
    }

    NoteLabelMap const noteLabels = makeNoteLabelMap(parsed.document.get(), nextNoteLabel);
    SemanticNode candidate = buildCompositionRoot(parsed.document.get(), &noteLabels);
    std::string diagnostics;
    if (!mergeNode(merged, candidate, "objectModel", diagnostics, warnings) ||
        !validateDataTypeKinds(merged, diagnostics) ||
        !validateEnumeratedValues(merged, diagnostics) ||
        !validateVariantRecordDiscriminantEnumeratorSyntax(merged, diagnostics) ||
        !validateVariantRecordAlternatives(merged, diagnostics)) {
      return {FomCompositionStatus::inconsistent_modules, {}, std::move(diagnostics)};
    }

    xmlNode const* root = xmlDocGetRootElement(parsed.document.get());
    if (xmlNode const* notes = directChildElement(root, "notes"); notes != nullptr) {
      SemanticNode candidateNotes = buildSemanticNode(notes, &noteLabels);
      if (!haveNotes) {
        mergedNotes = std::move(candidateNotes);
        haveNotes = true;
      } else if (!mergeNode(
                     mergedNotes,
                     candidateNotes,
                     "objectModel/notes",
                     diagnostics,
                     warnings)) {
        return {FomCompositionStatus::inconsistent_modules, {}, std::move(diagnostics)};
      }
    }
    if (xmlNode const* service = directChildElement(root, "serviceUtilization"); service != nullptr) {
      SemanticNode candidateService = buildSemanticNode(service, &noteLabels);
      if (!mergedServiceUtilization.has_value()) {
        for (auto& [key, child] : candidateService.children) {
          (void)key;
          child.attributes["isUsed"] = isUsed(child) ? "true" : "false";
        }
        mergedServiceUtilization = std::move(candidateService);
      } else if (!mergeServiceUtilization(*mergedServiceUtilization, candidateService, diagnostics)) {
        return {FomCompositionStatus::inconsistent_modules, {}, std::move(diagnostics)};
      }
    }

    moduleNames.push_back(moduleName(parsed.document.get(), module, moduleIndex));
    revalidated.push_back(std::move(parsed.module));
  }

  // DIF deliberately accepts incomplete modules. Resolve direct data-type
  // references only after the complete MIM-first module set is merged, so a
  // later module can supply a type used by an earlier extension module.
  std::string referenceDiagnostics;
  if (!validateDataTypeReferences(merged, referenceDiagnostics) ||
      !validateObjectAttributeAndInteractionParameterDataTypeKinds(merged, referenceDiagnostics) ||
      !validateAttributeNaCompanionFields(merged, referenceDiagnostics) ||
      !validateDynamicAttributeUpdateConditions(merged, referenceDiagnostics) ||
      !validateUnsharedAttributeValueRequirement(merged, referenceDiagnostics) ||
      !validateArrayElementDataTypeKinds(merged, referenceDiagnostics) ||
      !validateFixedRecordFieldAndVariantRecordAlternativeDataTypeKinds(
          merged,
          referenceDiagnostics) ||
      !validateVariantRecordDiscriminantDataTypeKinds(merged, referenceDiagnostics) ||
      !validateVariantRecordDiscriminantEnumeratorMembership(merged, referenceDiagnostics) ||
      !validateVariantRecordDiscriminantEnumeratorAssignments(merged, referenceDiagnostics) ||
      !validateDimensionInputDataTypeKinds(merged, referenceDiagnostics) ||
      !validateDimensionInputDataTypeNaExclusivity(merged, referenceDiagnostics) ||
      !validateDimensionInputDataTypeDescriptions(merged, referenceDiagnostics) ||
      !validateDataTypeRepresentationReferences(merged, referenceDiagnostics) ||
      !validateTimeRepresentationDataTypeKinds(merged, referenceDiagnostics) ||
      !validateTagDataTypeKinds(merged, referenceDiagnostics) ||
      !validateReferenceDataTypeClassReferences(merged, referenceDiagnostics) ||
      !validateReferenceDataTypeAttributeReferences(merged, referenceDiagnostics) ||
      !validateDirectedInteractionReferences(merged, referenceDiagnostics) ||
      !validateAvailableDimensionReferences(merged, referenceDiagnostics) ||
      !validateDimensionDefaultValues(merged, referenceDiagnostics) ||
      !validateTransportationReferences(merged, referenceDiagnostics) ||
      !validateStandardRootClassHierarchies(merged, referenceDiagnostics) ||
      !validateUpdateRateValues(merged, referenceDiagnostics) ||
      !validateArrayCardinalities(merged, referenceDiagnostics) ||
      !validateArrayEncodingCardinalityCompatibility(merged, referenceDiagnostics) ||
      !validateInheritedObjectClassAttributeNames(merged, referenceDiagnostics) ||
      !validateInheritedInteractionClassParameterNames(merged, referenceDiagnostics)) {
    return {FomCompositionStatus::inconsistent_modules, {}, std::move(referenceDiagnostics)};
  }

  std::set<std::string> referencedNoteLabels;
  collectNoteReferences(merged, referencedNoteLabels);
  if (mergedServiceUtilization.has_value()) {
    collectNoteReferences(*mergedServiceUtilization, referencedNoteLabels);
  }
  std::optional<SemanticNode> const retainedNotes =
      haveNotes ? selectedNotes(mergedNotes, referencedNoteLabels) : std::nullopt;
  FddMaterializationResult materialization = materializeFdd(
      merged,
      mergedServiceUtilization,
      retainedNotes,
      std::move(moduleNames),
      fddSchemaPath_);
  if (materialization.status != FddMaterializationStatus::valid) {
    return {
        materialization.status == FddMaterializationStatus::invalid_model
            ? FomCompositionStatus::inconsistent_modules
            : FomCompositionStatus::validator_failure,
        {},
        std::move(materialization.diagnostics),
    };
  }

  auto catalog = FomCatalogBuilder::build(merged, revalidated);
  return {
      FomCompositionStatus::valid,
      std::move(revalidated),
      {},
      std::move(catalog),
      std::move(materialization.fdd),
      std::move(warnings),
  };
}

}  // namespace umbra::detail
