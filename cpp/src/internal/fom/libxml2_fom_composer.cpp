#include "internal/fom/libxml2_fom_composer.hpp"

#include "internal/fom/hla_names.hpp"

#include "internal/fom/fdd_document.hpp"
#include "internal/fom/fom_catalog.hpp"
#include "internal/fom/fom_rpr_wire_encoding.hpp"
#include "internal/fom/libxml2_fom_document.hpp"
#include "internal/fom/libxml2_fom_composer_semantics.hpp"
#include "internal/runtime/utf8_string.hpp"

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

using NoteLabelMap = std::map<std::string, std::string>;
using NoteDefinitionMap = std::map<std::string, std::string>;

constexpr char kXmlSchemaInstanceNamespace[] = "http://www.w3.org/2001/XMLSchema-instance";

void appendChild(SemanticNode& parent, NodeKey key, SemanticNode child) {
  auto const [inserted, wasInserted] =
      parent.children.emplace(std::move(key), std::move(child));
  if (wasInserted) {
    parent.childOrder.push_back(inserted->first);
  }
}

std::string namespaceName(xmlNode const* node) {
  return node->ns == nullptr || node->ns->href == nullptr
             ? std::string{}
             : reinterpret_cast<char const*>(node->ns->href);
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
    if (node.localName == "modelIdentification" && node.children.contains(key)) {
      // Model-identification contains several legitimately repeated rows
      // (POCs, references, keywords, and history/restriction text).  The
      // composition map uses declaration identity for merge tables, but this
      // metadata is copied from the validated source and must not silently
      // collapse merely because its rows have no identity field.
      std::size_t duplicate = 1;
      NodeKey candidateKey = key;
      do {
        candidateKey.second = key.second + "#" + std::to_string(duplicate++);
      } while (node.children.contains(candidateKey));
      key = std::move(candidateKey);
    }
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

std::map<std::string, std::string> canonicalSwitchAttributes(
    SemanticNode const& node) {
  auto attributes = node.attributes;
  if (node.localName != "automaticResignAction") {
    // IEEE 1516.2's switchType supplies an XML-schema default of false for
    // an omitted isEnabled attribute.  Annex C.8 compares the meaning of a
    // duplicate switch, not the incidental spelling chosen by each DIF
    // module.  Keep Automatic Resign Action out of this normalization: its
    // distinct resignAction default is still under the reviewed RL-024
    // Lab/XSD precedence decision.
    auto const isEnabled = attributes.find("isEnabled");
    if (isEnabled == attributes.end() || isEnabled->second == "false" ||
        isEnabled->second == "0") {
      attributes.insert_or_assign("isEnabled", "false");
    } else if (isEnabled->second == "true" || isEnabled->second == "1") {
      attributes.insert_or_assign("isEnabled", "true");
    }
  }
  return attributes;
}

bool switchNodesEquivalent(
    SemanticNode const& current,
    SemanticNode const& candidate) {
  if (current.localName != candidate.localName ||
      current.namespaceName != candidate.namespaceName ||
      current.identity != candidate.identity ||
      current.text != candidate.text ||
      canonicalSwitchAttributes(current) != canonicalSwitchAttributes(candidate) ||
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

std::string noteDefinitionSignature(SemanticNode const& note) {
  // Note labels are reassigned to unique Umbra-owned identifiers for every
  // module. Compare the note's meaning rather than those generated labels
  // when deciding whether an Annex C table duplicate is equivalent.
  std::string signature = note.localName + "|" + note.namespaceName;
  for (auto const& [name, value] : note.attributes) {
    signature += "|attribute=" + name + ":" + value;
  }
  for (SemanticNode const* child : childrenInDeclarationOrder(note)) {
    if (child->localName == "label") {
      continue;
    }
    signature += "|child=" + child->localName + ":" + child->namespaceName + ":" +
                 child->text;
    for (auto const& [name, value] : child->attributes) {
      signature += "|attribute=" + name + ":" + value;
    }
    for (SemanticNode const* grandchild : childrenInDeclarationOrder(*child)) {
      signature += "|grandchild=" + grandchild->localName + ":" + grandchild->namespaceName +
                   ":" + grandchild->text;
    }
  }
  return signature;
}

NoteDefinitionMap noteDefinitions(SemanticNode const* notes) {
  NoteDefinitionMap definitions;
  if (notes == nullptr) {
    return definitions;
  }
  for (SemanticNode const* note : childrenInDeclarationOrder(*notes)) {
    if (note->localName != "note") {
      continue;
    }
    std::string const label = scalarChildValue(*note, "label");
    if (!label.empty()) {
      definitions.emplace(label, noteDefinitionSignature(*note));
    }
  }
  return definitions;
}

bool noteReferencesEquivalent(
    std::string const& current,
    std::string const& candidate,
    NoteDefinitionMap const& currentDefinitions,
    NoteDefinitionMap const& candidateDefinitions) {
  std::vector<std::string> currentSignatures;
  std::vector<std::string> candidateSignatures;
  for (std::string const& label : splitWhitespaceSeparated(current)) {
    auto const definition = currentDefinitions.find(label);
    currentSignatures.push_back(
        definition == currentDefinitions.end() ? "@unknown:" + label : definition->second);
  }
  for (std::string const& label : splitWhitespaceSeparated(candidate)) {
    auto const definition = candidateDefinitions.find(label);
    candidateSignatures.push_back(
        definition == candidateDefinitions.end() ? "@unknown:" + label : definition->second);
  }
  std::sort(currentSignatures.begin(), currentSignatures.end());
  std::sort(candidateSignatures.begin(), candidateSignatures.end());
  return currentSignatures == candidateSignatures;
}

bool semanticNodesEquivalentWithNotes(
    SemanticNode const& current,
    SemanticNode const& candidate,
    NoteDefinitionMap const& currentDefinitions,
    NoteDefinitionMap const& candidateDefinitions) {
  if (current.localName != candidate.localName ||
      current.namespaceName != candidate.namespaceName ||
      current.identity != candidate.identity ||
      current.text != candidate.text ||
      current.attributes.size() != candidate.attributes.size() ||
      current.children.size() != candidate.children.size()) {
    return false;
  }
  for (auto const& [name, currentValue] : current.attributes) {
    auto const candidateAttribute = candidate.attributes.find(name);
    if (candidateAttribute == candidate.attributes.end()) {
      return false;
    }
    if (isNoteReferencesAttribute(name)) {
      if (!noteReferencesEquivalent(
              currentValue,
              candidateAttribute->second,
              currentDefinitions,
              candidateDefinitions)) {
        return false;
      }
    } else if (currentValue != candidateAttribute->second) {
      return false;
    }
  }
  for (auto const& [key, currentChild] : current.children) {
    auto const candidateChild = candidate.children.find(key);
    if (candidateChild == candidate.children.end() ||
        !semanticNodesEquivalentWithNotes(
            currentChild,
            candidateChild->second,
            currentDefinitions,
            candidateDefinitions)) {
      return false;
    }
  }
  return true;
}

bool transportationNodesEquivalent(
    SemanticNode const& current,
    SemanticNode const& candidate,
    NoteDefinitionMap const& currentDefinitions = {},
    NoteDefinitionMap const& candidateDefinitions = {}) {
  if (current.localName != candidate.localName ||
      current.namespaceName != candidate.namespaceName ||
      current.identity != candidate.identity) {
    return false;
  }

  // A DIF transportation row may be incomplete.  Compare only values the
  // candidate actually supplies, while treating a newly supplied note as a
  // real duplicate difference rather than allowing the generic note union to
  // manufacture a changed definition.
  for (auto const& [name, candidateValue] : candidate.attributes) {
    auto const existing = current.attributes.find(name);
    if (name == "noteReferences") {
      if (existing == current.attributes.end()) {
        return false;
      }
      if (!noteReferencesEquivalent(
              existing->second,
              candidateValue,
              currentDefinitions,
              candidateDefinitions)) {
        return false;
      }
      continue;
    }
    if (existing != current.attributes.end() && existing->second != candidateValue) {
      return false;
    }
  }

  if (!candidate.text.empty() && !current.text.empty() && candidate.text != current.text) {
    return false;
  }
  for (auto const& [key, candidateChild] : candidate.children) {
    auto const existing = current.children.find(key);
    if (existing == current.children.end()) {
      // The first definition may be a partial DIF row; a later supplied
      // sub-element can complete it without being treated as a conflict.
      continue;
    }
    if (!transportationNodesEquivalent(
            existing->second,
            candidateChild,
            currentDefinitions,
            candidateDefinitions)) {
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
    if (!switchNodesEquivalent(existing->second, *candidateSwitch)) {
      warnings.push_back(
          "Annex C.8 warning: non-equivalent duplicate switch " +
          displayNode(*candidateSwitch) + " at " + path +
          " was ignored; the first module's setting is retained.");
    }
  }
  return true;
}

bool mergeSynchronizationPoints(
    SemanticNode& current,
    SemanticNode const& candidate,
    std::string const& path,
    std::string& diagnostics,
    NoteDefinitionMap const& currentDefinitions,
    NoteDefinitionMap const& candidateDefinitions) {
  if (!mergeNodeFields(current, candidate, path, diagnostics)) {
    return false;
  }

  // IEEE 1516.2-2025 Annex C.5 does not use the ordinary structural merge for
  // synchronization points.  A same-label point is compared with the first
  // definition: an identical duplicate is ignored, while a duplicate with
  // any differing sub-element (including notes) makes composition
  // inconsistent.  In particular, do not union noteReferences here; doing
  // so would silently turn a partially different point into a new definition.
  for (SemanticNode const* candidatePoint : childrenInDeclarationOrder(candidate)) {
    NodeKey const key = childKey(current, *candidatePoint);
    auto const existing = current.children.find(key);
    if (existing == current.children.end()) {
      appendChild(current, key, *candidatePoint);
      continue;
    }
    if (semanticNodesEquivalentWithNotes(
            existing->second,
            *candidatePoint,
            currentDefinitions,
            candidateDefinitions)) {
      continue;
    }
    diagnostics = "Conflicting duplicate synchronization point " +
                  displayNode(*candidatePoint) + " at " +
                  childPath(path, *candidatePoint) + ".";
    return false;
  }
  return true;
}

bool mergeTransportationTypes(
    SemanticNode& current,
    SemanticNode const& candidate,
    std::string const& path,
    std::string& diagnostics,
    bool allowLegacyStandardAliases,
    std::vector<std::string>& warnings,
    NoteDefinitionMap const& currentDefinitions,
    NoteDefinitionMap const& candidateDefinitions) {
  if (!mergeNodeFields(current, candidate, path, diagnostics)) {
    return false;
  }

  // IEEE 1516.2-2025 Annex C.6 compares a same-name transportation type with
  // the first definition.  An identical duplicate is ignored; a duplicate
  // with any differing name/semantics/notes sub-element fails.  Keep this out
  // of the generic structural merge so noteReferences cannot be unioned into
  // a silently changed transport definition.
  for (SemanticNode const* candidateTransportation : childrenInDeclarationOrder(candidate)) {
    NodeKey const key = childKey(current, *candidateTransportation);
    auto const existing = current.children.find(key);
    if (existing == current.children.end()) {
      appendChild(current, key, *candidateTransportation);
      continue;
    }
    if (transportationNodesEquivalent(
            existing->second,
            *candidateTransportation,
            currentDefinitions,
            candidateDefinitions)) {
      continue;
    }
    if (allowLegacyStandardAliases) {
      auto const name = candidateTransportation->identity.starts_with("name=")
          ? candidateTransportation->identity.substr(std::string{"name="}.size())
          : std::string{};
      auto const currentReliable = scalarChildValue(existing->second, "reliable");
      auto const candidateReliable = scalarChildValue(*candidateTransportation, "reliable");
      if ((name == "HLAreliable" || name == "HLAbestEffort") &&
          currentReliable == candidateReliable && !candidateReliable.empty()) {
        // A number of otherwise valid 1516-2010 modules repeat the standard
        // transport rows with abbreviated semantics. Preserve the standard
        // MIM definition when the reliability contract agrees; this narrow
        // compatibility rule is not used by the strict 2025 lane.
        warnings.push_back(
            "IEEE 1516-2010 compatibility: retained the first standard " +
            name + " transportation definition despite abbreviated duplicate semantics.");
        continue;
      }
    }
    diagnostics = "Conflicting duplicate transportation type " +
                  displayNode(*candidateTransportation) + " at " +
                  childPath(path, *candidateTransportation) + ".";
    return false;
  }
  return true;
}

bool updateRateNodesEquivalent(
    SemanticNode const& current,
    SemanticNode const& candidate,
    NoteDefinitionMap const& currentDefinitions = {},
    NoteDefinitionMap const& candidateDefinitions = {}) {
  if (current.localName != candidate.localName ||
      current.namespaceName != candidate.namespaceName ||
      current.identity != candidate.identity) {
    return false;
  }

  // DIF permits an update-rate row to omit rate/semantics at the module
  // boundary.  Compare only supplied candidate fields, but keep a supplied
  // note reference as a meaningful duplicate difference rather than applying
  // the generic note union.
  for (auto const& [name, candidateValue] : candidate.attributes) {
    auto const existing = current.attributes.find(name);
    if (name == "noteReferences") {
      if (existing == current.attributes.end() ||
          !noteReferencesEquivalent(
              existing->second,
              candidateValue,
              currentDefinitions,
              candidateDefinitions)) {
        return false;
      }
      continue;
    }
    if (existing != current.attributes.end() && existing->second != candidateValue) {
      return false;
    }
  }

  if (!candidate.text.empty() && !current.text.empty() && candidate.text != current.text) {
    return false;
  }
  for (auto const& [key, candidateChild] : candidate.children) {
    auto const existing = current.children.find(key);
    if (existing == current.children.end()) {
      // A later module may complete an incomplete DIF row.
      continue;
    }
    if (!semanticNodesEquivalentWithNotes(
            existing->second,
            candidateChild,
            currentDefinitions,
            candidateDefinitions)) {
      return false;
    }
  }
  return true;
}

bool mergeUpdateRates(
    SemanticNode& current,
    SemanticNode const& candidate,
    std::string const& path,
    std::string& diagnostics,
    NoteDefinitionMap const& currentDefinitions,
    NoteDefinitionMap const& candidateDefinitions) {
  if (!mergeNodeFields(current, candidate, path, diagnostics)) {
    return false;
  }

  // IEEE 1516.2-2025 Annex C.7 compares a same-name update rate with the
  // first definition. An identical duplicate is ignored; a duplicate with a
  // differing name/rate/semantics/note sub-element fails. Unique rows are
  // inserted under the existing table.
  for (SemanticNode const* candidateUpdateRate : childrenInDeclarationOrder(candidate)) {
    NodeKey const key = childKey(current, *candidateUpdateRate);
    auto const existing = current.children.find(key);
    if (existing == current.children.end()) {
      appendChild(current, key, *candidateUpdateRate);
      continue;
    }
    if (updateRateNodesEquivalent(
            existing->second,
            *candidateUpdateRate,
            currentDefinitions,
            candidateDefinitions)) {
      continue;
    }
    diagnostics = "Conflicting duplicate update rate " +
                  displayNode(*candidateUpdateRate) + " at " +
                  childPath(path, *candidateUpdateRate) + ".";
    return false;
  }
  return true;
}

bool mergeDimensions(
    SemanticNode& current,
    SemanticNode const& candidate,
    std::string const& path,
    std::string& diagnostics,
    NoteDefinitionMap const& currentNoteDefinitions,
    NoteDefinitionMap const& candidateNoteDefinitions) {
  if (!mergeNodeFields(current, candidate, path, diagnostics)) {
    return false;
  }

  // IEEE 1516.2-2025 Annex C.4 compares a same-name dimension with the first
  // definition.  An identical duplicate is ignored; if any dimension
  // sub-element differs, composition fails.  Keep this out of the generic
  // structural merge so optional fields and noteReferences cannot be filled
  // or unioned into a silently changed dimension definition.  Unique names
  // remain candidates for insertion into the composed dimensions table.
  for (SemanticNode const* candidateDimension : childrenInDeclarationOrder(candidate)) {
    NodeKey const key = childKey(current, *candidateDimension);
    auto const existing = current.children.find(key);
    if (existing == current.children.end()) {
      appendChild(current, key, *candidateDimension);
      continue;
    }
    if (semanticNodesEquivalentWithNotes(
            existing->second,
            *candidateDimension,
            currentNoteDefinitions,
            candidateNoteDefinitions)) {
      continue;
    }
    diagnostics = "Conflicting duplicate dimension " +
                  displayNode(*candidateDimension) + " at " +
                  childPath(path, *candidateDimension) + ".";
    return false;
  }
  return true;
}

bool mergeNode(
    SemanticNode& current,
    SemanticNode const& candidate,
    std::string const& path,
    std::string& diagnostics,
    std::vector<std::string>& warnings,
    bool allowLegacyStandardAliases = false,
    NoteDefinitionMap const& currentNoteDefinitions = {},
    NoteDefinitionMap const& candidateNoteDefinitions = {}) {
  if (current.localName == "switches") {
    return mergeSwitches(current, candidate, path, diagnostics, warnings);
  }
  if (current.localName == "synchronizations") {
    return mergeSynchronizationPoints(
        current,
        candidate,
        path,
        diagnostics,
        currentNoteDefinitions,
        candidateNoteDefinitions);
  }
  if (current.localName == "transportations") {
    return mergeTransportationTypes(
        current,
        candidate,
        path,
        diagnostics,
        allowLegacyStandardAliases,
        warnings,
        currentNoteDefinitions,
        candidateNoteDefinitions);
  }
  if (current.localName == "updateRates") {
    return mergeUpdateRates(
        current,
        candidate,
        path,
        diagnostics,
        currentNoteDefinitions,
        candidateNoteDefinitions);
  }
  if (current.localName == "dimensions") {
    return mergeDimensions(
        current,
        candidate,
        path,
        diagnostics,
        currentNoteDefinitions,
        candidateNoteDefinitions);
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
            warnings,
            allowLegacyStandardAliases,
            currentNoteDefinitions,
            candidateNoteDefinitions)) {
      return false;
    }
  }
  return true;
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
    std::string_view expectedNamespace,
    std::string& diagnostics) {
  if (semantic.namespaceName != expectedNamespace) {
    diagnostics = "The FDD materializer does not accept an extension element outside the selected IEEE 1516 namespace: " +
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
    if (!appendSemanticNode(
            document,
            output,
            hlaNamespace,
            *child,
            expectedNamespace,
            diagnostics)) {
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
    std::optional<SemanticNode> const& modelIdentification,
    std::optional<SemanticNode> const& serviceUtilization,
    std::optional<SemanticNode> const& notes,
    std::vector<std::string> composedFromModuleNames,
    FomStandardEdition standardEdition,
    std::wstring_view fddSchemaDesignator,
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
  std::string const hlaNamespaceName = std::string(fomNamespace(standardEdition));
  std::string const fddSchemaLocation =
      hlaNamespaceName + " " + utf8FromWide(std::wstring{fddSchemaDesignator});
  xmlNs* hlaNamespace = xmlNewNs(root, BAD_CAST hlaNamespaceName.c_str(), nullptr);
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
          BAD_CAST fddSchemaLocation.c_str()) == nullptr) {
    return {
        FddMaterializationStatus::validator_failure,
        {},
        "Cannot set the FDD schema-location attribute.",
    };
  }

  xmlNode* identification = nullptr;
  if (modelIdentification.has_value()) {
    std::string identificationDiagnostics;
    if (!appendSemanticNode(
            document.get(),
            root,
            hlaNamespace,
            *modelIdentification,
            hlaNamespaceName,
            identificationDiagnostics)) {
      return {
          FddMaterializationStatus::invalid_model,
          {},
          std::move(identificationDiagnostics),
      };
    }
    identification = const_cast<xmlNode*>(directChildElement(root, "modelIdentification"));
  } else {
    identification = xmlNewChild(root, hlaNamespace, BAD_CAST "modelIdentification", nullptr);
  }
  if (identification == nullptr) {
    return {
        FddMaterializationStatus::validator_failure,
        {},
        "Cannot allocate composed-FDD model identification.",
    };
  }
  auto appendComposedReference = [&](std::string const& moduleName) {
    xmlNode* reference = xmlNewNode(hlaNamespace, BAD_CAST "reference");
    if (reference == nullptr ||
        xmlNewChild(reference, hlaNamespace, BAD_CAST "type", BAD_CAST "Composed_From") == nullptr ||
        xmlNewChild(
            reference,
            hlaNamespace,
            BAD_CAST "identification",
            BAD_CAST moduleName.c_str()) == nullptr) {
      if (reference != nullptr) {
        xmlFreeNode(reference);
      }
      return false;
    }
    // `other` and `glyph` follow all references in both the FDD and OMT
    // model-identification sequences.  The source template may retain either
    // element, so insert generated references before that boundary instead of
    // blindly appending them after a source glyph.
    xmlNode* insertionPoint = nullptr;
    for (xmlNode* child = identification->children; child != nullptr; child = child->next) {
      if (child->type == XML_ELEMENT_NODE &&
          (localName(child) == "other" || localName(child) == "glyph")) {
        insertionPoint = child;
        break;
      }
    }
    if (insertionPoint != nullptr) {
      if (xmlAddPrevSibling(insertionPoint, reference) == nullptr) {
        xmlFreeNode(reference);
        return false;
      }
    } else if (xmlAddChild(identification, reference) == nullptr) {
      xmlFreeNode(reference);
      return false;
    }
    return true;
  };
  for (std::string const& moduleName : composedFromModuleNames) {
    if (!appendComposedReference(moduleName)) {
      return {
          FddMaterializationStatus::validator_failure,
          {},
          "Cannot allocate composed-FDD module references.",
      };
    }
  }

  std::string diagnostics;
  if (serviceUtilization.has_value() &&
      !appendSemanticNode(
          document.get(),
          root,
          hlaNamespace,
          *serviceUtilization,
          hlaNamespaceName,
          diagnostics)) {
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
    if (!appendSemanticNode(
            document.get(),
            root,
            hlaNamespace,
            *section,
            hlaNamespaceName,
            diagnostics)) {
      return {FddMaterializationStatus::invalid_model, {}, std::move(diagnostics)};
    }
  }
  if (notes.has_value() &&
      !appendSemanticNode(
          document.get(),
          root,
          hlaNamespace,
          *notes,
          hlaNamespaceName,
          diagnostics)) {
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
    FomSourceCompatibility const sourceCompatibility = modules.empty()
        ? FomSourceCompatibility::strict
        : modules.front().sourceCompatibility;

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
    if (auto const* transportations = firstChildNamed(root, "transportations");
        transportations != nullptr) {
      for (auto const& [key, transportation] : transportations->children) {
        (void)key;
        if (transportation.localName != "transportation") {
          continue;
        }
        std::string const name = identityValue(transportation, "name=");
        if (name.empty()) {
          continue;
        }
        auto const reliable = scalarChildValue(transportation, "reliable");
        catalog->transportationTypes_.insert_or_assign(
            name,
            FomTransportationDefinition{
                name,
                reliable == "Yes" || reliable == "true" || reliable == "1",
                scalarChildValue(transportation, "semantics"),
            });
      }
    }
    if (auto const* dataTypes = firstChildNamed(root, "dataTypes"); dataTypes != nullptr) {
      appendDataTypes(*catalog, *dataTypes, sourceCompatibility);
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
    if (auto const* synchronizations = firstChildNamed(root, "synchronizations");
        synchronizations != nullptr) {
      for (auto const& [key, synchronizationPoint] : synchronizations->children) {
        (void)key;
        if (synchronizationPoint.localName != "synchronizationPoint") {
          continue;
        }
        std::string const label = identityValue(synchronizationPoint, "label=");
        if (label.empty()) {
          continue;
        }
        auto noteReferences = synchronizationPoint.attributes.find("noteReferences");
        catalog->synchronizationPoints_.insert_or_assign(
            label,
            FomSynchronizationPointDefinition{
                label,
                scalarChildValue(synchronizationPoint, "dataType"),
                scalarChildValue(synchronizationPoint, "capability"),
                scalarChildValue(synchronizationPoint, "semantics"),
                noteReferences == synchronizationPoint.attributes.end()
                    ? std::string{}
                    : noteReferences->second,
            });
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
    FomObjectClassDefinition definition;
    definition.name = name;
    definition.parentName = parentName;
    // Preserve these fields exactly as supplied. DIF permits partial rows and
    // the completed-model merge handles compatible completion; this catalog
    // must not invent a default or reinterpret the FOM's capability metadata
    // as current per-federate declaration state.
    definition.sharing = scalarChildValue(node, "sharing");
    definition.semantics = scalarChildValue(node, "semantics");
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
          definition.directedInteractions.push_back(
              FomDirectedInteractionDefinition{
                  interactionName,
                  scalarChildValue(child, "sharing"),
              });
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
    FomInteractionClassDefinition definition;
    definition.name = name;
    definition.parentName = parentName;
    definition.sharing = scalarChildValue(node, "sharing");
    definition.semantics = scalarChildValue(node, "semantics");
    definition.transportation = scalarChildValue(node, "transportation");
    definition.order = scalarChildValue(node, "order");
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

  static FomWireEncodingDescriptor wireEncoding(
      SemanticNode const& node,
      FomSourceCompatibility compatibility) {
    std::string const sourceEncoding = scalarChildValue(node, "encoding");
    if (node.localName != "basicData") {
      if (compatibility == FomSourceCompatibility::rpr_2010) {
        // RPR uses its custom unsigned primitive names as the representation
        // of simple/enumerated declarations rather than as an <encoding>
        // child. Keep that promotion in the RPR adapter; standard composition
        // continues to see only the neutral descriptor path.
        auto const representation = scalarChildValue(node, "representation");
        if (rprUnsignedIntegerWireCodecAvailable(representation)) {
          return normalizeRprUnsignedIntegerFomWireEncoding(representation);
        }
        return normalizeRprFomWireEncoding(sourceEncoding);
      }
      return normalizeFomWireEncoding(sourceEncoding, compatibility);
    }

    std::uint32_t sizeBits = 0;
    std::string const sizeText = scalarChildValue(node, "size");
    if (!sizeText.empty()) {
      unsigned long const parsedSize = parseUnsignedLong(sizeText);
      if (parsedSize <= std::numeric_limits<std::uint32_t>::max()) {
        sizeBits = static_cast<std::uint32_t>(parsedSize);
      }
    }
    FomByteOrder byteOrder = FomByteOrder::unspecified;
    std::string const endian = scalarChildValue(node, "endian");
    if (endian == "Big") {
      byteOrder = FomByteOrder::big;
    } else if (endian == "Little") {
      byteOrder = FomByteOrder::little;
    }
    if (compatibility == FomSourceCompatibility::rpr_2010) {
      return normalizeRprBasicFomWireEncoding(
          identityValue(node, "name="),
          sourceEncoding,
          sizeBits,
          byteOrder);
    }
    return makeBasicFomWireEncoding(sourceEncoding, sizeBits, byteOrder);
  }

  static void appendDataTypes(
      FomCatalog& catalog,
      SemanticNode const& dataTypes,
      FomSourceCompatibility compatibility) {
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
      FomDataTypeDefinition definition{name, dataTypeKind(node.localName), std::move(representation)};
      definition.wireEncoding = wireEncoding(node, compatibility);
      if (node.localName == "arrayData") {
        definition.elementDataType = scalarChildValue(node, "dataType");
        definition.cardinality = scalarChildValue(node, "cardinality");
      } else if (node.localName == "fixedRecordData") {
        for (SemanticNode const* field : childrenInDeclarationOrder(node)) {
          if (field->localName != "field") {
            continue;
          }
          definition.fields.push_back({
              identityValue(*field, "name="),
              scalarChildValue(*field, "dataType"),
              scalarChildValue(*field, "semantics"),
          });
        }
      } else if (node.localName == "variantRecordData") {
        definition.discriminantDataType = scalarChildValue(node, "dataType");
        for (SemanticNode const* alternative : childrenInDeclarationOrder(node)) {
          if (alternative->localName != "alternative") {
            continue;
          }
          FomDataTypeDefinition::Alternative projection{
              identityValue(*alternative, "name="),
              {},
              scalarChildValue(*alternative, "dataType"),
              scalarChildValue(*alternative, "semantics"),
          };
          for (SemanticNode const* enumerator : childrenInDeclarationOrder(*alternative)) {
            if (enumerator->localName == "enumerator" && !enumerator->text.empty()) {
              projection.discriminantEnumerators.push_back(enumerator->text);
            }
          }
          definition.alternatives.push_back(std::move(projection));
        }
      }
      catalog.dataTypes_.insert_or_assign(name, std::move(definition));
    });
  }
};

LibXml2FomModuleComposer::LibXml2FomModuleComposer(
    std::filesystem::path fddSchemaPath2025,
    std::filesystem::path fddSchemaPath2010)
    : fddSchemaPath2025_(std::move(fddSchemaPath2025)),
      fddSchemaPath2010_(std::move(fddSchemaPath2010)) {}

FomCompositionResult LibXml2FomModuleComposer::compose(
    std::vector<PrevalidatedFomModule> const& modules) const {
  if (modules.empty()) {
    return {FomCompositionStatus::invalid_model, {}, "At least one FOM or MIM module is required."};
  }

  SemanticNode merged{"objectModel", {}, {}, {}, {}, {}};
  FomStandardEdition const standardEdition = modules.front().standardEdition;
  FomSourceCompatibility const sourceCompatibility = modules.front().sourceCompatibility;
  std::string const expectedNamespace = std::string(fomNamespace(standardEdition));
  SemanticNode mergedNotes{"notes", expectedNamespace, {}, {}, {}, {}};
  bool haveNotes = false;
  std::optional<SemanticNode> mergedServiceUtilization;
  std::optional<SemanticNode> modelIdentification;
  std::vector<PrevalidatedFomModule> revalidated;
  revalidated.reserve(modules.size());
  std::vector<std::string> moduleNames;
  moduleNames.reserve(modules.size());
  std::vector<std::string> warnings;
  std::size_t nextNoteLabel = 1;
  std::set<std::string> reservedHlaNames;

  for (std::size_t moduleIndex = 0; moduleIndex < modules.size(); ++moduleIndex) {
    PrevalidatedFomModule const& module = modules[moduleIndex];
    if (module.standardEdition != standardEdition) {
      return {
          FomCompositionStatus::inconsistent_modules,
          {},
          "FOM modules from IEEE 1516-2010 and IEEE 1516-2025 cannot be composed in one execution."};
    }
    if (module.sourceCompatibility != sourceCompatibility) {
      return {
          FomCompositionStatus::inconsistent_modules,
          {},
          "FOM modules with different source-compatibility profiles cannot be composed in one execution."};
    }
    LibXml2ValidatedFomDocument parsed;
    FomValidationResult const validation = loadValidatedLibXml2FomDocument(
        {
            module.sourcePath,
            module.schemaPath,
            module.kind,
            module.designator,
            module.schemaDesignator,
            module.standardEdition,
            module.sourceCompatibility,
        },
        parsed);
    if (validation.status != FomValidationStatus::valid) {
      return {statusFor(validation.status), {}, validation.diagnostics};
    }

    warnings.insert(
        warnings.end(),
        parsed.module.warnings.begin(),
        parsed.module.warnings.end());

    NoteLabelMap const noteLabels = makeNoteLabelMap(parsed.document.get(), nextNoteLabel);
    SemanticNode candidate = buildCompositionRoot(parsed.document.get(), &noteLabels);
    xmlNode const* root = xmlDocGetRootElement(parsed.document.get());
    std::string diagnostics;
    if (!validateFomModificationDate(
            directChildElement(root, "modelIdentification"),
            diagnostics)) {
      return {FomCompositionStatus::inconsistent_modules, {}, std::move(diagnostics)};
    }
    if (!modelIdentification.has_value()) {
      if (xmlNode const* identification = directChildElement(root, "modelIdentification");
          identification != nullptr) {
        modelIdentification = buildSemanticNode(identification, &noteLabels);
      }
    }
    std::optional<SemanticNode> candidateNotes;
    if (xmlNode const* notes = directChildElement(root, "notes"); notes != nullptr) {
      candidateNotes = buildSemanticNode(notes, &noteLabels);
    }
    if (module.kind == FomModuleKind::mim) {
      collectFomReservedHlaNames(candidate, reservedHlaNames);
      if (candidateNotes.has_value()) {
        collectFomReservedHlaNames(*candidateNotes, reservedHlaNames);
      }
    }
    NoteDefinitionMap const currentNoteDefinitions =
        haveNotes ? noteDefinitions(&mergedNotes) : NoteDefinitionMap{};
    NoteDefinitionMap const candidateNoteDefinitions =
        candidateNotes.has_value() ? noteDefinitions(&*candidateNotes) : NoteDefinitionMap{};
    // Name conventions are validated after all modules have merged.  This
    // preserves the completed-model ordering of the MIM-reserved set and
    // lets table-specific NA companion rules report their more precise
    // diagnostic before the generic reserved-name check.
    if (!validateUniqueComposedFomDimensions(parsed.document.get(), diagnostics) ||
        !mergeNode(
            merged,
            candidate,
            "objectModel",
            diagnostics,
            warnings,
            standardEdition == FomStandardEdition::ieee1516_2010,
            currentNoteDefinitions,
            candidateNoteDefinitions) ||
        !validateMergedFomModuleState(merged, diagnostics)) {
      return {FomCompositionStatus::inconsistent_modules, {}, std::move(diagnostics)};
    }

    if (candidateNotes.has_value()) {
      if (!haveNotes) {
        mergedNotes = std::move(*candidateNotes);
        haveNotes = true;
      } else if (!mergeNode(
                     mergedNotes,
                     *candidateNotes,
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
  if (!validateComposedFomModel(
          merged,
          haveNotes ? &mergedNotes : nullptr,
          standardEdition,
          reservedHlaNames,
          referenceDiagnostics)) {
    return {FomCompositionStatus::inconsistent_modules, {}, std::move(referenceDiagnostics)};
  }

  std::set<std::string> referencedNoteLabels;
  collectNoteReferences(merged, referencedNoteLabels);
  if (mergedServiceUtilization.has_value()) {
    collectNoteReferences(*mergedServiceUtilization, referencedNoteLabels);
  }
  std::optional<SemanticNode> const retainedNotes =
      haveNotes ? selectedNotes(mergedNotes, referencedNoteLabels) : std::nullopt;

  // The first 2010 compatibility slice is intentionally catalog-only. The
  // 2010 FDD schema describes a different materialized view from the 2025
  // FDD (for example, class-level dimension rows are legal in legacy DIF but
  // not in the legacy FDD). Do not silently rewrite that model merely to
  // manufacture an FDD artifact; callers in this slice need validated class,
  // attribute, interaction, and datatype metadata for handle/object creation.
  // FDD serialization remains available for the complete 2025 lane and is a
  // later, separately reviewed 2010 compatibility milestone.
  auto catalog = FomCatalogBuilder::build(merged, revalidated);
  if (standardEdition == FomStandardEdition::ieee1516_2010) {
    return {
        FomCompositionStatus::valid,
        std::move(revalidated),
        {},
        std::move(catalog),
        {},
        std::move(warnings),
    };
  }

  FddMaterializationResult materialization = materializeFdd(
      merged,
      modelIdentification,
      mergedServiceUtilization,
      retainedNotes,
      std::move(moduleNames),
      standardEdition,
      fomFddSchemaDesignator(standardEdition),
      standardEdition == FomStandardEdition::ieee1516_2010
          ? fddSchemaPath2010_
          : fddSchemaPath2025_);
  if (materialization.status != FddMaterializationStatus::valid) {
    return {
        materialization.status == FddMaterializationStatus::invalid_model
            ? FomCompositionStatus::inconsistent_modules
            : FomCompositionStatus::validator_failure,
        {},
        std::move(materialization.diagnostics),
    };
  }

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
