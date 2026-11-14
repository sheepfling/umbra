#include <RTI/auth/AuthorizerFactory.h>
#include <RTI/auth/HLAauthorizerFactoryFactory.h>
#include <RTI/libauth/AuthorizerFactoryFactory.h>

#include <memory>
#include <string>
#include <utility>

#include "internal/runtime/authorizer_factory_provider.hpp"

namespace rti1516_2025 {

std::unique_ptr<AuthorizerFactory> AuthorizerFactoryFactory::getAuthorizerFactory(
    std::wstring const& authorizerName) {
  auto provider =
      umbra::detail::makeAuthorizerFactoryFromProviderLibrary(authorizerName);
  if (provider.factory) {
    return std::move(provider.factory);
  }
  if (provider.configured && !provider.available) {
    return {};
  }
  // The standard library boundary delegates unsupported names to the
  // reference implementation, as required by AuthorizerFactoryFactory.
  return HLAauthorizerFactoryFactory::getAuthorizerFactory(authorizerName);
}

}  // namespace rti1516_2025
