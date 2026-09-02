#pragma once

#include "internal/fom/fom_validation.hpp"
#include "internal/fom/fom_wire_encoding.hpp"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace umbra::detail {

// Immutable query projection of the supported portions of a composed FOM/MIM
// set. It deliberately is not called an FDD: Notes-table remapping, service
// utilization, complete reference validation, and FDD serialization are still
// separate implementation work.
struct FomAttributeDefinition {
  std::string name;
  std::string dataType;
  // The composed catalog does not execute update policy itself. Retaining the
  // standard policy and required-value bit here lets a later RTI-owned
  // MOM-object runtime consume the MIM declaration instead of introducing a
  // parallel hand-written attribute table.
  std::string updateType;
  std::string updateCondition;
  bool valueRequired = false;
  std::string ownership;
  std::string sharing;
  std::string transportation;
  std::string order;
};

// A directed-interaction declaration is a named association between an object
// class and an interaction class.  Its sharing field is OMT model metadata;
// current per-federate declaration state belongs to the federation registry.
// Retain it rather than reducing the association to a bare interaction name so
// Annex C-compatible composition and later FOM-facing behavior can use the
// actual supplied declaration.
struct FomDirectedInteractionDefinition {
  std::string interactionClassName;
  std::string sharing;
};

struct FomObjectClassDefinition {
  std::string name;
  std::string parentName;
  std::string sharing;
  std::string semantics;
  std::map<std::string, FomAttributeDefinition> declaredAttributes;
  std::vector<FomDirectedInteractionDefinition> directedInteractions;
  // Dimensions declared on this class. Available-dimension lookup walks the
  // class hierarchy so inherited dimensions remain visible to subclasses.
  std::vector<std::string> dimensions;

  [[nodiscard]] FomDirectedInteractionDefinition const* directedInteraction(
      std::string const& interactionClassName) const noexcept {
    for (auto const& declaration : directedInteractions) {
      if (declaration.interactionClassName == interactionClassName) {
        return &declaration;
      }
    }
    return nullptr;
  }
};

struct FomInteractionParameterDefinition {
  std::string name;
  std::string dataType;
};

struct FomInteractionClassDefinition {
  std::string name;
  std::string parentName;
  std::string sharing;
  std::string semantics;
  std::map<std::string, FomInteractionParameterDefinition> declaredParameters;
  std::string transportation;
  std::string order;
  // Dimensions declared on this interaction class. Lookup includes inherited
  // dimensions from parent interaction classes.
  std::vector<std::string> dimensions;
};

struct FomDimensionDefinition {
  std::string name;
  std::vector<std::string> inputDataTypes;
  unsigned long upperBound = 0;
};

enum class FomDataTypeKind {
  basic,
  simple,
  reference,
  enumerated,
  array,
  fixed_record,
  variant_record,
};

struct FomDataTypeDefinition {
  std::string name;
  FomDataTypeKind kind = FomDataTypeKind::simple;
  std::string representation;
  // The source encoding and its neutral structural interpretation are kept
  // together. A recognized shape is not evidence that a runtime byte codec
  // is available; consult wireEncoding.codecStatus for that distinction.
  FomWireEncodingDescriptor wireEncoding;
  std::string elementDataType;
  std::string cardinality;
  std::string discriminantDataType;
  struct Field {
    std::string name;
    std::string dataType;
    std::string semantics;
  };
  struct Alternative {
    std::string name;
    std::vector<std::string> discriminantEnumerators;
    std::string dataType;
    std::string semantics;
  };
  // Declaration order is significant for record and variant wire layouts.
  // Keep these projections separate from the generic encoding descriptor so
  // future codecs can consume structure without parsing XML again.
  std::vector<Field> fields;
  std::vector<Alternative> alternatives;
};

struct FomTimeDefinition {
  std::string logicalTimeDataType;
  std::string logicalTimeIntervalDataType;
};

// A transportation row is part of the composed FDD even when no current
// delivery policy is attached to it.  Retaining the declared name in the
// catalog gives the official TransportationTypeHandle lookup services an
// execution-scoped source of truth instead of a hand-written runtime list.
struct FomTransportationDefinition {
  std::string name;
  bool reliable = false;
  std::string semantics;
};

// A synchronization-point row retained from the composed 1516.2 table.  The
// registry currently owns live registration/achievement state; this catalog
// projection keeps the standards-defined declaration available without
// introducing a second hand-written label/type table.
struct FomSynchronizationPointDefinition {
  std::string label;
  std::string dataType;
  std::string capability;
  std::string semantics;
  std::string noteReferences;
};

struct FomUpdateRateDefinition {
  std::string name;
  double rate = 0.0;
};

// Federation-wide FDD switch settings that influence time-management
// coordination.  The OMT default is represented here rather than inferred by
// a later scheduler: omitted switch entries are Disabled.
struct FomTimeManagementSwitches {
  bool nonRegulatedGrant = false;
};

// Federation-wide switch settings supplied by the composed FDD.  The initial
// Auto Provide value is dynamic and may be adjusted by the MOM path; the
// remaining values are static for the lifetime of an execution.
struct FomFederationSwitches {
  bool autoProvide = false;
  // These switches are federation-wide and static for an execution.  Their
  // values are seeded from the creation FDD; additional FOM modules cannot
  // silently change an active execution's policy.
  bool delaySubscriptionEvaluation = false;
  bool allowRelaxedDDM = false;
};

// Initial values for switches that belong to an individual joined federate.
// The FDD switch table supplies defaults for each new member; the member may
// subsequently change the value through its support-service setter.
struct FomFederateSupportSwitches {
  bool conveyRegionDesignatorSets = false;
  // Keep the schema lexical value here so the FOM/catalog layer remains
  // usable by the validation-only CMake target, which intentionally has no
  // public RTI binding include path.  The embedded registry maps this
  // validated lexical value to the official C++ enum at membership time.
  // IEEE 1516.2-2025 clause 4.13.3 gives this switch its distinct default;
  // unlike the other switches it is not Disabled.
  std::string automaticResignAction = "CancelThenDeleteThenDivest";
  bool serviceReporting = false;
  bool exceptionReporting = false;
  bool sendServiceReportsToFile = false;
};

// Per-federate advisory switch settings supplied by the composed FDD.  The
// 1516.2 switch table is optional in the input model, but the composed FDD
// always has a value: omitted entries use the standard Disabled default.
// Keeping the values in the catalog lets the registry seed each new member
// without conflating FDD initial settings with later per-federate changes.
struct FomAdvisorySwitches {
  bool attributeScopeAdvisory = false;
  bool attributeRelevanceAdvisory = false;
  bool objectClassRelevanceAdvisory = false;
  bool interactionRelevanceAdvisory = false;
  // The FDD-level switch controls whether advisory calculations may use a
  // federate's known class rather than only its registered class.  Keep the
  // parsed value separate from the per-federate advisory gates above: this
  // switch has an official getter but no corresponding setter.
  bool advisoriesUseKnownClass = false;
};

class FomCatalogBuilder;

class FomCatalog final {
 public:
  [[nodiscard]] std::vector<PrevalidatedFomModule> const& modules() const noexcept {
    return modules_;
  }

  [[nodiscard]] FomObjectClassDefinition const* objectClass(
      std::string const& name) const noexcept {
    auto const found = objectClasses_.find(name);
    return found == objectClasses_.end() ? nullptr : &found->second;
  }

  // Returns every attribute available at an object class, including inherited
  // attributes.  Composition already rejects an attribute that redeclares an
  // inherited name; retain that invariant here instead of silently letting a
  // derived definition hide a base-class policy.  A missing class, broken
  // parent chain, duplicate, or cycle therefore has no usable projection.
  //
  // This is especially important for MOM object construction: the standard
  // HLAmanager.HLAfederate class inherits HLAprivilegeToDeleteObject from
  // HLAobjectRoot, whose ownership policy differs from the joined-federate
  // attributes declared directly on HLAfederate.
  [[nodiscard]] std::optional<std::map<std::string, FomAttributeDefinition>>
  effectiveObjectClassAttributes(std::string const& name) const {
    if (name.empty()) {
      return std::nullopt;
    }
    std::vector<FomObjectClassDefinition const*> hierarchy;
    std::set<std::string> visited;
    std::string currentName = name;
    while (!currentName.empty()) {
      auto const current = objectClasses_.find(currentName);
      if (current == objectClasses_.end() || !visited.insert(currentName).second) {
        return std::nullopt;
      }
      hierarchy.push_back(&current->second);
      currentName = current->second.parentName;
    }

    std::map<std::string, FomAttributeDefinition> result;
    for (auto current = hierarchy.rbegin(); current != hierarchy.rend(); ++current) {
      for (auto const& [attributeName, attribute] : (*current)->declaredAttributes) {
        if (!result.emplace(attributeName, attribute).second) {
          return std::nullopt;
        }
      }
    }
    return result;
  }

  // Names are emitted in the catalog's deterministic map order. A federation
  // handle directory consumes this projection so it can assign stable
  // per-federation ObjectClassHandle values without exposing the catalog's
  // private storage to the public binding layer.
  [[nodiscard]] std::vector<std::string> objectClassNames() const {
    std::vector<std::string> result;
    result.reserve(objectClasses_.size());
    for (auto const& [name, definition] : objectClasses_) {
      static_cast<void>(definition);
      result.push_back(name);
    }
    return result;
  }

  [[nodiscard]] FomInteractionClassDefinition const* interactionClass(
      std::string const& name) const noexcept {
    auto const found = interactionClasses_.find(name);
    return found == interactionClasses_.end() ? nullptr : &found->second;
  }

  // Names are emitted in the catalog's deterministic map order. A federation
  // handle directory consumes this projection so it can assign stable
  // per-federation InteractionClassHandle values without exposing the
  // catalog's private storage to the public binding layer.
  [[nodiscard]] std::vector<std::string> interactionClassNames() const {
    std::vector<std::string> result;
    result.reserve(interactionClasses_.size());
    for (auto const& [name, definition] : interactionClasses_) {
      static_cast<void>(definition);
      result.push_back(name);
    }
    return result;
  }

  [[nodiscard]] FomDimensionDefinition const* dimension(std::string const& name) const noexcept {
    auto const found = dimensions_.find(name);
    return found == dimensions_.end() ? nullptr : &found->second;
  }

  // Names are emitted in deterministic map order. The handle directory uses
  // this projection to allocate stable per-federation DimensionHandle values.
  [[nodiscard]] std::vector<std::string> dimensionNames() const {
    std::vector<std::string> result;
    result.reserve(dimensions_.size());
    for (auto const& [name, definition] : dimensions_) {
      static_cast<void>(definition);
      result.push_back(name);
    }
    return result;
  }

  // Names are emitted in deterministic map order.  Transportation handles
  // are allocated from this projection per federation execution, with the
  // two mandatory HLA names retaining their official values.
  [[nodiscard]] std::vector<std::string> transportationTypeNames() const {
    std::vector<std::string> result;
    result.reserve(transportationTypes_.size());
    for (auto const& [name, definition] : transportationTypes_) {
      static_cast<void>(definition);
      result.push_back(name);
    }
    return result;
  }

  [[nodiscard]] FomTransportationDefinition const* transportationType(
      std::string const& name) const noexcept {
    auto const found = transportationTypes_.find(name);
    return found == transportationTypes_.end() ? nullptr : &found->second;
  }

  [[nodiscard]] FomDataTypeDefinition const* dataType(std::string const& name) const noexcept {
    auto const found = dataTypes_.find(name);
    return found == dataTypes_.end() ? nullptr : &found->second;
  }

  [[nodiscard]] FomTimeDefinition const& time() const noexcept {
    return time_;
  }

  [[nodiscard]] FomSynchronizationPointDefinition const* synchronizationPoint(
      std::string const& label) const noexcept {
    auto const found = synchronizationPoints_.find(label);
    return found == synchronizationPoints_.end() ? nullptr : &found->second;
  }

  [[nodiscard]] std::vector<std::string> synchronizationPointLabels() const {
    std::vector<std::string> result;
    result.reserve(synchronizationPoints_.size());
    for (auto const& [label, definition] : synchronizationPoints_) {
      static_cast<void>(definition);
      result.push_back(label);
    }
    return result;
  }

  [[nodiscard]] std::optional<double> updateRateValue(
      std::string const& name) const noexcept {
    auto const found = updateRates_.find(name);
    return found == updateRates_.end() ? std::nullopt : std::optional<double>{found->second};
  }

  [[nodiscard]] FomTimeManagementSwitches const& timeManagementSwitches() const noexcept {
    return timeManagementSwitches_;
  }

  [[nodiscard]] FomFederationSwitches const& federationSwitches() const noexcept {
    return federationSwitches_;
  }

  [[nodiscard]] FomAdvisorySwitches const& advisorySwitches() const noexcept {
    return advisorySwitches_;
  }

  [[nodiscard]] FomFederateSupportSwitches const& federateSupportSwitches() const noexcept {
    return federateSupportSwitches_;
  }

 private:
  std::vector<PrevalidatedFomModule> modules_;
  std::map<std::string, FomObjectClassDefinition> objectClasses_;
  std::map<std::string, FomInteractionClassDefinition> interactionClasses_;
  std::map<std::string, FomDimensionDefinition> dimensions_;
  std::map<std::string, FomTransportationDefinition> transportationTypes_;
  std::map<std::string, FomDataTypeDefinition> dataTypes_;
  std::map<std::string, FomSynchronizationPointDefinition> synchronizationPoints_;
  std::map<std::string, double> updateRates_;
  FomTimeDefinition time_;
  FomTimeManagementSwitches timeManagementSwitches_;
  FomFederationSwitches federationSwitches_;
  FomFederateSupportSwitches federateSupportSwitches_;
  FomAdvisorySwitches advisorySwitches_;

  friend class FomCatalogBuilder;
};

}  // namespace umbra::detail
