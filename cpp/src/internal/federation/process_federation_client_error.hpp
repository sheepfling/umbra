#pragma once

#include "internal/federation/transport_service_protocol.hpp"

#include <string>

namespace umbra::detail {

[[nodiscard]] std::string requestFailure(
    TransportServiceOperation operation,
    TransportServiceStatus status);

}  // namespace umbra::detail
