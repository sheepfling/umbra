#include "internal/handles/object_class_handle_directory.hpp"

#include "internal/fom/fom_catalog.hpp"
#include "internal/handles/fom_handle_allocator.hpp"

#include <utility>

namespace umbra::detail {

bool ObjectClassHandleDirectory::reconcile(
    FomCatalog const* catalog,
    FomHandleAllocator& handleAllocator) {
  if (catalog == nullptr) {
    return true;
  }

  for (std::string const& name : catalog->objectClassNames()) {
    if (name.empty()) {
      return false;
    }
    if (handlesByName_.contains(name)) {
      continue;
    }
    auto const allocatedHandle =
        handleAllocator.allocate("object-class", name);
    if (!allocatedHandle) {
      return false;
    }
    std::uint64_t const handle = *allocatedHandle;
    auto [namePosition, insertedName] = handlesByName_.emplace(name, handle);
    if (!insertedName) {
      continue;
    }
    try {
      auto [handlePosition, insertedHandle] = namesByHandle_.emplace(handle, name);
      static_cast<void>(handlePosition);
      if (!insertedHandle) {
        handlesByName_.erase(namePosition);
        return false;
      }
    } catch (...) {
      handlesByName_.erase(namePosition);
      throw;
    }
  }
  return true;
}

std::optional<std::uint64_t> ObjectClassHandleDirectory::handleFor(
    std::string const& objectClassName) const {
  auto const found = handlesByName_.find(objectClassName);
  return found == handlesByName_.end() ? std::nullopt : std::optional{found->second};
}

std::optional<std::string> ObjectClassHandleDirectory::nameFor(std::uint64_t handle) const {
  if (handle == 0) {
    return std::nullopt;
  }
  auto const found = namesByHandle_.find(handle);
  return found == namesByHandle_.end() ? std::nullopt : std::optional{found->second};
}

}  // namespace umbra::detail
