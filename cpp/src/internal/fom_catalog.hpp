#pragma once

#include "internal/fom_validation.hpp"

#include <map>
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
  std::string sharing;
  std::string transportation;
  std::string order;
};

struct FomObjectClassDefinition {
  std::string name;
  std::string parentName;
  std::map<std::string, FomAttributeDefinition> declaredAttributes;
  std::vector<std::string> directedInteractions;
  // Dimensions declared on this class. Available-dimension lookup walks the
  // class hierarchy so inherited dimensions remain visible to subclasses.
  std::vector<std::string> dimensions;
};

struct FomInteractionParameterDefinition {
  std::string name;
  std::string dataType;
};

struct FomInteractionClassDefinition {
  std::string name;
  std::string parentName;
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
};

struct FomTimeDefinition {
  std::string logicalTimeDataType;
  std::string logicalTimeIntervalDataType;
};

// Federation-wide FDD switch settings that influence time-management
// coordination.  The OMT default is represented here rather than inferred by
// a later scheduler: omitted switch entries are Disabled.
struct FomTimeManagementSwitches {
  bool nonRegulatedGrant = false;
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

  [[nodiscard]] FomDataTypeDefinition const* dataType(std::string const& name) const noexcept {
    auto const found = dataTypes_.find(name);
    return found == dataTypes_.end() ? nullptr : &found->second;
  }

  [[nodiscard]] FomTimeDefinition const& time() const noexcept {
    return time_;
  }

  [[nodiscard]] FomTimeManagementSwitches const& timeManagementSwitches() const noexcept {
    return timeManagementSwitches_;
  }

 private:
  std::vector<PrevalidatedFomModule> modules_;
  std::map<std::string, FomObjectClassDefinition> objectClasses_;
  std::map<std::string, FomInteractionClassDefinition> interactionClasses_;
  std::map<std::string, FomDimensionDefinition> dimensions_;
  std::map<std::string, FomDataTypeDefinition> dataTypes_;
  FomTimeDefinition time_;
  FomTimeManagementSwitches timeManagementSwitches_;

  friend class FomCatalogBuilder;
};

}  // namespace umbra::detail
