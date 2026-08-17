#include "internal/object_class_handle_directory.hpp"

#include "internal/fom_catalog.hpp"

#include <limits>
#include <utility>

namespace umbra::detail {

bool ObjectClassHandleDirectory::reconcile(FomCatalog const* catalog) {
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
    if (nextHandle_ == 0) {
      return false;
    }

    std::uint64_t const handle = nextHandle_;
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

    nextHandle_ = handle == std::numeric_limits<std::uint64_t>::max()
        ? 0
        : handle + 1;
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
