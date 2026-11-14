#pragma once

#include <RTI/auth/AuthorizerFactory.h>

#include <memory>
#include <string>

namespace umbra::detail {

struct AuthorizerFactoryProviderResult final {
  bool configured = false;
  bool available = false;
  std::unique_ptr<rti1516_2025::AuthorizerFactory> factory;
};

inline constexpr char authorizerFactoryProviderEntryPoint[] =
    "umbra_authorizer_factory_v1";
using AuthorizerFactoryProviderEntryPoint =
    rti1516_2025::AuthorizerFactory* (*)(wchar_t const* serviceName);

// Resolve an optional shared authorizer provider configured by the absolute
// UMBRA_AUTHORIZER_PROVIDER_LIBRARY path. The library exports the internal
// umbra_authorizer_factory_v1 bridge and is pinned for process lifetime so
// returned factories/authorizers can safely execute virtual methods.
[[nodiscard]] AuthorizerFactoryProviderResult
makeAuthorizerFactoryFromProviderLibrary(std::wstring const& serviceName);

}  // namespace umbra::detail

#if defined(_WIN32)
#define UMBRA_AUTHORIZER_PROVIDER_EXPORT __declspec(dllexport)
#else
#define UMBRA_AUTHORIZER_PROVIDER_EXPORT \
  __attribute__((visibility("default")))
#endif
