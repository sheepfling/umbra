#pragma once

#include "internal/encoding/composite_primitives.hpp"

namespace umbra {
namespace detail {
namespace composite2025 {

// The adapter remains version-specific even though its type-free primitives
// are shared.  Callers therefore keep the 2025 namespace and include path.
using composite_primitives::appendZeroPadding;
using composite_primitives::hasRange;
using composite_primitives::isZeroPadding;
using composite_primitives::tryCheckedAdd;
using composite_primitives::tryPaddingToBoundary;

}  // namespace composite2025
}  // namespace detail
}  // namespace umbra
