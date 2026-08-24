#include "internal/handles/attribute_handle_directory.hpp"

#include "internal/fom/fom_catalog.hpp"

#include <limits>
#include <set>
#include <utility>

namespace umbra::detail {

std::optional<AttributeHandleDirectory::AttributeKey>
AttributeHandleDirectory::resolveDefinition(
    FomCatalog const* catalog,
    std::string const& objectClassName,
    std::string const& attributeName) {
  if (catalog == nullptr || objectClassName.empty() || attributeName.empty()) {
    return std::nullopt;
  }

  std::set<std::string> visited;
  std::string currentClassName = objectClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* objectClass = catalog->objectClass(currentClassName);
    if (objectClass == nullptr) {
      return std::nullopt;
    }
    if (objectClass->declaredAttributes.contains(attributeName)) {
      return AttributeKey{objectClass->name, attributeName};
    }
    currentClassName = objectClass->parentName;
  }
  return std::nullopt;
}

bool AttributeHandleDirectory::reconcile(FomCatalog const* catalog) {
  if (catalog == nullptr) {
    return true;
  }

  for (std::string const& objectClassName : catalog->objectClassNames()) {
    auto const* objectClass = catalog->objectClass(objectClassName);
    if (objectClass == nullptr || objectClass->name != objectClassName) {
      return false;
    }
    for (auto const& [attributeName, definition] : objectClass->declaredAttributes) {
      static_cast<void>(definition);
      if (attributeName.empty()) {
        return false;
      }

      AttributeKey const key{objectClassName, attributeName};
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

std::optional<std::uint64_t> AttributeHandleDirectory::handleFor(
    FomCatalog const* catalog,
    std::string const& objectClassName,
    std::string const& attributeName) const {
  auto const definition = resolveDefinition(catalog, objectClassName, attributeName);
  if (!definition) {
    return std::nullopt;
  }
  auto const found = handlesByDefinition_.find(*definition);
  return found == handlesByDefinition_.end() ? std::nullopt : std::optional{found->second};
}

std::optional<std::string> AttributeHandleDirectory::nameFor(
    FomCatalog const* catalog,
    std::string const& objectClassName,
    std::uint64_t handle) const {
  if (handle == 0) {
    return std::nullopt;
  }
  auto const found = definitionsByHandle_.find(handle);
  if (found == definitionsByHandle_.end()) {
    return std::nullopt;
  }

  AttributeKey const& expectedDefinition = found->second;
  auto const resolvedDefinition = resolveDefinition(
      catalog,
      objectClassName,
      expectedDefinition.second);
  if (!resolvedDefinition || *resolvedDefinition != expectedDefinition) {
    return std::nullopt;
  }
  return expectedDefinition.second;
}

}  // namespace umbra::detail
