#include "internal/parameter_handle_directory.hpp"

#include "internal/fom_catalog.hpp"

#include <limits>
#include <set>
#include <utility>

namespace umbra::detail {

std::optional<ParameterHandleDirectory::ParameterKey>
ParameterHandleDirectory::resolveDefinition(
    FomCatalog const* catalog,
    std::string const& interactionClassName,
    std::string const& parameterName) {
  if (catalog == nullptr || interactionClassName.empty() || parameterName.empty()) {
    return std::nullopt;
  }

  std::set<std::string> visited;
  std::string currentClassName = interactionClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* interactionClass = catalog->interactionClass(currentClassName);
    if (interactionClass == nullptr) {
      return std::nullopt;
    }
    if (interactionClass->declaredParameters.contains(parameterName)) {
      return ParameterKey{interactionClass->name, parameterName};
    }
    currentClassName = interactionClass->parentName;
  }
  return std::nullopt;
}

bool ParameterHandleDirectory::reconcile(FomCatalog const* catalog) {
  if (catalog == nullptr) {
    return true;
  }

  for (std::string const& interactionClassName : catalog->interactionClassNames()) {
    auto const* interactionClass = catalog->interactionClass(interactionClassName);
    if (interactionClass == nullptr || interactionClass->name != interactionClassName) {
      return false;
    }
    for (auto const& [parameterName, definition] : interactionClass->declaredParameters) {
      static_cast<void>(definition);
      if (parameterName.empty()) {
        return false;
      }

      ParameterKey const key{interactionClassName, parameterName};
      if (handlesByDefinition_.contains(key)) {
        continue;
      }
      if (nextHandle_ == 0) {
        return false;
      }

      std::uint64_t const handle = nextHandle_;
      auto [definitionPosition, insertedDefinition] = handlesByDefinition_.emplace(key, handle);
      if (!insertedDefinition) {
        continue;
      }
      try {
        auto [handlePosition, insertedHandle] = definitionsByHandle_.emplace(handle, key);
        static_cast<void>(handlePosition);
        if (!insertedHandle) {
          handlesByDefinition_.erase(definitionPosition);
          return false;
        }
      } catch (...) {
        handlesByDefinition_.erase(definitionPosition);
        throw;
      }

      nextHandle_ = handle == std::numeric_limits<std::uint64_t>::max()
          ? 0
          : handle + 1;
    }
  }
  return true;
}

std::optional<std::uint64_t> ParameterHandleDirectory::handleFor(
    FomCatalog const* catalog,
    std::string const& interactionClassName,
    std::string const& parameterName) const {
  auto const definition = resolveDefinition(catalog, interactionClassName, parameterName);
  if (!definition) {
    return std::nullopt;
  }
  auto const found = handlesByDefinition_.find(*definition);
  return found == handlesByDefinition_.end() ? std::nullopt : std::optional{found->second};
}

std::optional<std::string> ParameterHandleDirectory::nameFor(
    FomCatalog const* catalog,
    std::string const& interactionClassName,
    std::uint64_t handle) const {
  if (handle == 0) {
    return std::nullopt;
  }
  auto const found = definitionsByHandle_.find(handle);
  if (found == definitionsByHandle_.end()) {
    return std::nullopt;
  }

  ParameterKey const& expectedDefinition = found->second;
  auto const resolvedDefinition = resolveDefinition(
      catalog,
      interactionClassName,
      expectedDefinition.second);
  if (!resolvedDefinition || *resolvedDefinition != expectedDefinition) {
    return std::nullopt;
  }
  return expectedDefinition.second;
}

}  // namespace umbra::detail
