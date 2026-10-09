#pragma once

#include "internal/federation/federation_registry_service_types.hpp"
#include "internal/federation/federation_state_image.hpp"

#include <RTI/RTIambassador.h>
#include <RTI/encoding/BasicDataElements.h>

#include <memory>
#include <optional>
#include <string>

namespace umbra::detail::federation_registry_state_image_restore_helpers {

std::shared_ptr<rti1516_2025::LogicalTime const> decodeLogicalTimeEncoding(
    std::wstring const& implementationName,
    std::optional<std::string> const& encoding);

std::shared_ptr<rti1516_2025::LogicalTimeInterval const>
decodeLogicalTimeIntervalEncoding(
    std::wstring const& implementationName,
    std::optional<std::string> const& encoding);

rti1516_2025::VariableLengthData variableLengthDataFromBytes(
    std::string const& value);

std::optional<rti1516_2025::OrderType> decodeSavedOrderType(
    std::uint32_t encoded);

RegionSpecificationSnapshot decodeSavedRegionSnapshot(
    FederationStateImageInteractionRegionSnapshot const& saved);

} // namespace umbra::detail::federation_registry_state_image_restore_helpers
