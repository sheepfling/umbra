#pragma once

#include <RTI/time/LogicalTimeFactory.h>

namespace umbra::detail {

inline constexpr char federationFedTimeFactoryEntryPoint[] =
    "umbra_fedtime_factory_v1";

using FederationFedTimeFactoryEntryPoint =
    rti1516_2025::LogicalTimeFactory* (*)(wchar_t const* implementationName);

}  // namespace umbra::detail

#if defined(_WIN32)
#define UMBRA_FEDTIME_PROVIDER_EXPORT __declspec(dllexport)
#else
#define UMBRA_FEDTIME_PROVIDER_EXPORT __attribute__((visibility("default")))
#endif
