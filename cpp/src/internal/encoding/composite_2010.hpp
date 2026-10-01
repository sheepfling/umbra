#pragma once

#include "internal/encoding/composite_primitives.hpp"

namespace umbra {
namespace detail {
namespace composite2010 {

// The adapter remains version-specific even though its type-free primitives
// are shared.  Callers therefore keep the 2010 namespace and include path.
using composite_primitives::appendZeroPadding;
using composite_primitives::hasRange;
using composite_primitives::isZeroPadding;
using composite_primitives::tryCheckedAdd;
using composite_primitives::tryPaddingToBoundary;

}  // namespace composite2010
}  // namespace detail
}  // namespace umbra
