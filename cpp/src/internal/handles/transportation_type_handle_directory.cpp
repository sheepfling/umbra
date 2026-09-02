#include "internal/handles/transportation_type_handle_directory.hpp"

#include "internal/fom/fom_catalog.hpp"

#include <limits>

namespace umbra::detail {
namespace {

constexpr std::uint64_t kReliableHandle = 1;
constexpr std::uint64_t kBestEffortHandle = 2;

bool isStandardName(std::string const& name) {
  return name == "HLAreliable" || name == "HLAbestEffort";
}

}  // namespace

bool TransportationTypeHandleDirectory::reconcile(FomCatalog const* catalog) {
  if (catalog == nullptr) {
    return true;
  }

  for (std::string const& name : catalog->transportationTypeNames()) {
    if (name.empty()) {
      return false;
    }
    if (isStandardName(name) || handlesByName_.contains(name)) {
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

std::optional<std::uint64_t> TransportationTypeHandleDirectory::handleFor(
    std::string const& transportationTypeName) const {
  if (transportationTypeName == "HLAreliable") {
    return kReliableHandle;
  }
  if (transportationTypeName == "HLAbestEffort") {
    return kBestEffortHandle;
  }
  auto const found = handlesByName_.find(transportationTypeName);
  return found == handlesByName_.end() ? std::nullopt : std::optional{found->second};
}

std::optional<std::string> TransportationTypeHandleDirectory::nameFor(
    std::uint64_t handle) const {
  switch (handle) {
    case kReliableHandle:
      return std::string{"HLAreliable"};
    case kBestEffortHandle:
      return std::string{"HLAbestEffort"};
    default:
      break;
  }
  if (handle < 3) {
    return std::nullopt;
  }
  auto const found = namesByHandle_.find(handle);
  return found == namesByHandle_.end() ? std::nullopt : std::optional{found->second};
}

}  // namespace umbra::detail
