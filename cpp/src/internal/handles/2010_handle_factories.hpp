#pragma once

#include <RTI/Handle.h>

#include <cstdint>
#include <utility>

namespace rti1516e {
namespace umbra_binding_detail {

#define UMBRA_2010_HANDLE_FACTORY_DECLARATIONS(HandleKind) \
  HandleKind make##HandleKind(std::uint64_t value); \
  HandleKind decode##HandleKind(VariableLengthData const& encodedValue); \
  std::pair<bool, std::uint64_t> HandleKind##Value(HandleKind const& handle) noexcept;

UMBRA_2010_HANDLE_FACTORY_DECLARATIONS(FederateHandle)
UMBRA_2010_HANDLE_FACTORY_DECLARATIONS(ObjectClassHandle)
UMBRA_2010_HANDLE_FACTORY_DECLARATIONS(InteractionClassHandle)
UMBRA_2010_HANDLE_FACTORY_DECLARATIONS(ObjectInstanceHandle)
UMBRA_2010_HANDLE_FACTORY_DECLARATIONS(AttributeHandle)
UMBRA_2010_HANDLE_FACTORY_DECLARATIONS(ParameterHandle)
UMBRA_2010_HANDLE_FACTORY_DECLARATIONS(DimensionHandle)
UMBRA_2010_HANDLE_FACTORY_DECLARATIONS(MessageRetractionHandle)
UMBRA_2010_HANDLE_FACTORY_DECLARATIONS(RegionHandle)

#undef UMBRA_2010_HANDLE_FACTORY_DECLARATIONS

}  // namespace umbra_binding_detail
}  // namespace rti1516e
