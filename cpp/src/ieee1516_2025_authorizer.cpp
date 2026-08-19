#include <RTI/auth/AuthorizerFactory.h>
#include <RTI/auth/HLAauthorizerFactoryFactory.h>
#include <RTI/libauth/AuthorizerFactoryFactory.h>

#include <memory>
#include <string>

namespace rti1516_2025 {

std::unique_ptr<AuthorizerFactory> AuthorizerFactoryFactory::getAuthorizerFactory(
    std::wstring const& authorizerName) {
  // The standard library-level entry point is intentionally only a forwarding
  // boundary. Custom authorizer-library loading remains a later RID/DLC slice;
  // unknown names therefore receive the reference factory's null result.
  return HLAauthorizerFactoryFactory::getAuthorizerFactory(authorizerName);
}

}  // namespace rti1516_2025
