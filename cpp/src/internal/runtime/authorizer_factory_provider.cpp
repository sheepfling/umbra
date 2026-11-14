#include "authorizer_factory_provider.hpp"

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace umbra::detail {
namespace {

class DynamicAuthorizerLibrary final {
 public:
  DynamicAuthorizerLibrary() = default;
  DynamicAuthorizerLibrary(DynamicAuthorizerLibrary const&) = delete;
  DynamicAuthorizerLibrary& operator=(DynamicAuthorizerLibrary const&) =
      delete;

  ~DynamicAuthorizerLibrary() {
#if defined(_WIN32)
    if (handle_ != nullptr) {
      FreeLibrary(handle_);
    }
#else
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
#endif
  }

#if defined(_WIN32)
  [[nodiscard]] bool load(std::wstring const& path) {
    handle_ = LoadLibraryW(path.c_str());
    return handle_ != nullptr;
  }

  [[nodiscard]] AuthorizerFactoryProviderEntryPoint factoryEntryPoint()
      const {
    if (handle_ == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<AuthorizerFactoryProviderEntryPoint>(
        GetProcAddress(handle_, authorizerFactoryProviderEntryPoint));
  }
#else
  [[nodiscard]] bool load(std::string const& path) {
    handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    return handle_ != nullptr;
  }

  [[nodiscard]] AuthorizerFactoryProviderEntryPoint factoryEntryPoint()
      const {
    if (handle_ == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<AuthorizerFactoryProviderEntryPoint>(
        dlsym(handle_, authorizerFactoryProviderEntryPoint));
  }
#endif

 private:
#if defined(_WIN32)
  HMODULE handle_{};
#else
  void* handle_{};
#endif
};

struct AuthorizerProvider final {
  bool configured{};
  bool available{};
  std::unique_ptr<DynamicAuthorizerLibrary> library;
  AuthorizerFactoryProviderEntryPoint createFactory{};
};

[[nodiscard]] AuthorizerProvider loadAuthorizerProvider() {
  AuthorizerProvider provider;
#if defined(_WIN32)
  constexpr wchar_t environmentName[] =
      L"UMBRA_AUTHORIZER_PROVIDER_LIBRARY";
  auto const required = GetEnvironmentVariableW(environmentName, nullptr, 0U);
  if (required == 0U) {
    return provider;
  }
  provider.configured = true;
  std::wstring path(required, L'\0');
  auto const written =
      GetEnvironmentVariableW(environmentName, path.data(), required);
  if (written == 0U || written >= required) {
    return provider;
  }
  path.resize(written);
  if (!std::filesystem::path(path).is_absolute()) {
    return provider;
  }
#else
  constexpr char environmentName[] = "UMBRA_AUTHORIZER_PROVIDER_LIBRARY";
  auto const* environmentValue = std::getenv(environmentName);
  if (environmentValue == nullptr || *environmentValue == '\0') {
    return provider;
  }
  provider.configured = true;
  std::string path(environmentValue);
  if (!std::filesystem::path(path).is_absolute()) {
    return provider;
  }
#endif

  provider.library = std::make_unique<DynamicAuthorizerLibrary>();
  if (!provider.library->load(path)) {
    provider.library.reset();
    return provider;
  }
  provider.createFactory = provider.library->factoryEntryPoint();
  provider.available = provider.createFactory != nullptr;
  if (!provider.available) {
    provider.library.reset();
  }
  return provider;
}

[[nodiscard]] AuthorizerProvider const& authorizerProvider() {
  static AuthorizerProvider const provider = loadAuthorizerProvider();
  return provider;
}

}  // namespace

AuthorizerFactoryProviderResult makeAuthorizerFactoryFromProviderLibrary(
    std::wstring const& serviceName) {
  auto const& provider = authorizerProvider();
  AuthorizerFactoryProviderResult result;
  result.configured = provider.configured;
  result.available = provider.available;
  if (provider.createFactory != nullptr) {
    if (auto* factory = provider.createFactory(serviceName.c_str())) {
      result.factory.reset(factory);
      if (result.factory->getName() != serviceName) {
        result.factory.reset();
      }
    }
  }
  return result;
}

}  // namespace umbra::detail
