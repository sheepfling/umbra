#include "internal/time/federation_time_factory_plugin.hpp"
#include "internal/time/federation_time_factory_provider.hpp"

#include <RTI/libfedtime/LogicalTimeFactoryFactory.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <cstdlib>
#include <memory>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace umbra::detail {
namespace {

class DynamicFedTimeLibrary final {
 public:
  DynamicFedTimeLibrary() = default;
  DynamicFedTimeLibrary(DynamicFedTimeLibrary const&) = delete;
  DynamicFedTimeLibrary& operator=(DynamicFedTimeLibrary const&) = delete;

  ~DynamicFedTimeLibrary() {
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

  [[nodiscard]] FederationFedTimeFactoryEntryPoint factoryEntryPoint() const {
    if (handle_ == nullptr) {
      return nullptr;
    }
    auto const symbol = GetProcAddress(
        handle_, federationFedTimeFactoryEntryPoint);
    return reinterpret_cast<FederationFedTimeFactoryEntryPoint>(symbol);
  }
#else
  [[nodiscard]] bool load(std::string const& path) {
    handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    return handle_ != nullptr;
  }

  [[nodiscard]] FederationFedTimeFactoryEntryPoint factoryEntryPoint() const {
    if (handle_ == nullptr) {
      return nullptr;
    }
    auto const symbol = dlsym(handle_, federationFedTimeFactoryEntryPoint);
    return reinterpret_cast<FederationFedTimeFactoryEntryPoint>(symbol);
  }
#endif

 private:
#if defined(_WIN32)
  HMODULE handle_{};
#else
  void* handle_{};
#endif
};

struct FedTimeProvider final {
  bool configured{};
  std::unique_ptr<DynamicFedTimeLibrary> library;
  FederationFedTimeFactoryEntryPoint createFactory{};
};

[[nodiscard]] FedTimeProvider loadFedTimeProvider() {
  FedTimeProvider provider;
#if defined(_WIN32)
  constexpr wchar_t environmentName[] = L"UMBRA_FEDTIME_PROVIDER_LIBRARY";
  auto const required = GetEnvironmentVariableW(environmentName, nullptr, 0U);
  if (required == 0U) {
    return provider;
  }
  provider.configured = true;
  std::wstring path(required, L'\0');
  auto const written = GetEnvironmentVariableW(
      environmentName, path.data(), required);
  if (written == 0U || written >= required) {
    return provider;
  }
  path.resize(written);
#else
  constexpr char environmentName[] = "UMBRA_FEDTIME_PROVIDER_LIBRARY";
  auto const* environmentValue = std::getenv(environmentName);
  if (environmentValue == nullptr || *environmentValue == '\0') {
    return provider;
  }
  provider.configured = true;
  std::string path(environmentValue);
#endif

  provider.library = std::make_unique<DynamicFedTimeLibrary>();
  if (!provider.library->load(path)) {
    return provider;
  }
  provider.createFactory = provider.library->factoryEntryPoint();
  return provider;
}

[[nodiscard]] FedTimeProvider const& fedTimeProvider() {
  static FedTimeProvider const provider = loadFedTimeProvider();
  return provider;
}

}  // namespace

std::unique_ptr<rti1516_2025::LogicalTimeFactory>
makeFederationLogicalTimeFactory(std::wstring const& implementationName) {
#if defined(UMBRA_BUILD_SHARED_LIBRARIES)
  auto const& provider = fedTimeProvider();
  if (!provider.configured) {
    return rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
        implementationName);
  }
  if (provider.createFactory == nullptr) {
    return nullptr;
  }
  if (auto* factory = provider.createFactory(implementationName.c_str())) {
    return std::unique_ptr<rti1516_2025::LogicalTimeFactory>(factory);
  }
  return rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      implementationName);
#else
  return rti1516_2025::LogicalTimeFactoryFactory::makeLogicalTimeFactory(
      implementationName);
#endif
}

}  // namespace umbra::detail
