#include "internal/handles/transportation_type_handle_directory.hpp"

#include "internal/fom/fom_catalog.hpp"
#include "internal/handles/fom_handle_allocator.hpp"
#include "internal/handles/transportation_type_handle.hpp"

namespace umbra::detail {

bool TransportationTypeHandleDirectory::reconcile(
    FomCatalog const* catalog,
    FomHandleAllocator& handleAllocator) {
  if (catalog == nullptr) {
    return true;
  }

  for (std::string const& name : catalog->transportationTypeNames()) {
    if (name.empty()) {
      return false;
    }
    if (name == "HLAreliable" || name == "HLAbestEffort" ||
        handlesByName_.contains(name)) {
      continue;
    }
    auto const allocatedHandle =
        handleAllocator.allocate("transportation-type", name);
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

std::optional<std::uint64_t> TransportationTypeHandleDirectory::handleFor(
    std::string const& transportationTypeName) const {
  if (transportationTypeName == "HLAreliable") {
    return rti1516_2025::umbra_binding_detail::
        kHlaReliableTransportationTypeValue;
  }
  if (transportationTypeName == "HLAbestEffort") {
    return rti1516_2025::umbra_binding_detail::
        kHlaBestEffortTransportationTypeValue;
  }
  auto const found = handlesByName_.find(transportationTypeName);
  return found == handlesByName_.end() ? std::nullopt : std::optional{found->second};
}

std::optional<std::string> TransportationTypeHandleDirectory::nameFor(
    std::uint64_t handle) const {
  if (handle == rti1516_2025::umbra_binding_detail::
                    kHlaReliableTransportationTypeValue) {
    return std::string{"HLAreliable"};
  }
  if (handle == rti1516_2025::umbra_binding_detail::
                    kHlaBestEffortTransportationTypeValue) {
    return std::string{"HLAbestEffort"};
  }
  if (handle == 0) {
    return std::nullopt;
  }
  auto const found = namesByHandle_.find(handle);
  return found == namesByHandle_.end() ? std::nullopt : std::optional{found->second};
}

}  // namespace umbra::detail
