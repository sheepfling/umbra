#pragma once

#include "internal/runtime/ambassador_shared_utilities.hpp"

namespace rti1516_2025::umbra_binding_detail::ownership_callback_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
void queueAttributeOwnershipAcquisitionIfAvailableReport(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestId,
    VariableLengthData userSuppliedTag);

void queueAttributeOwnershipUnavailableRecipients(
    std::vector<umbra::detail::AttributeOwnershipUnavailableRecipient> recipients,
    std::wstring const &federationName,
    VariableLengthData const &userSuppliedTag);
#endif

}  // namespace rti1516_2025::umbra_binding_detail::ownership_callback_detail
