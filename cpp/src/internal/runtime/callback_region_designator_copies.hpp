#pragma once

#include "internal/handles/region_handle.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

// Owns temporary receiver-visible copies for the duration of one callback.
// These copies are deliberately separate from ordinary region accounting and
// are only queryable through the callback's Get Dimension Handle Set and
// Get Range Bounds calls.
class CallbackRegionDesignatorCopies final {
 public:
  CallbackRegionDesignatorCopies(
      std::wstring federationName,
      std::uint64_t receivingFederateId)
      : federationName_(std::move(federationName)),
        receivingFederateId_(receivingFederateId) {}

  CallbackRegionDesignatorCopies(CallbackRegionDesignatorCopies const&) = delete;
  CallbackRegionDesignatorCopies& operator=(
      CallbackRegionDesignatorCopies const&) = delete;

  ~CallbackRegionDesignatorCopies() {
    if (copiedRegionHandles_.empty()) {
      return;
    }
    try {
      embeddedFederationRegistry().releaseCallbackScopedRegionDesignatorCopies(
          federationName_, receivingFederateId_, copiedRegionHandles_);
    } catch (...) {
      // Cleanup must not replace an exception thrown by the user callback.
    }
  }

  [[nodiscard]] umbra::detail::RegionServiceStatus create(
      std::set<std::uint64_t> const& sentRegionHandles,
      std::map<std::uint64_t, umbra::detail::RegionSpecificationSnapshot> const& sentRegionSnapshots,
      RegionHandleSet& callbackRegionHandles) {
    auto const status = embeddedFederationRegistry()
        .createCallbackScopedRegionDesignatorCopies(
            federationName_,
            receivingFederateId_,
            sentRegionHandles,
            sentRegionSnapshots,
            copiedRegionHandles_);
    if (status != umbra::detail::RegionServiceStatus::applied) {
      return status;
    }

    for (auto const& [sourceRegionHandle, copiedRegionHandle] : copiedRegionHandles_) {
      static_cast<void>(sourceRegionHandle);
      callbackRegionHandles.insert(makeRegionHandle(copiedRegionHandle));
    }
    return status;
  }

 private:
  std::wstring federationName_;
  std::uint64_t receivingFederateId_;
  std::map<std::uint64_t, std::uint64_t> copiedRegionHandles_;
};

#endif

}  // namespace rti1516_2025::umbra_binding_detail
