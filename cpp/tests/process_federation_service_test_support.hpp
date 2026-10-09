#pragma once

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/transport_service_protocol.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Shared private fixtures for the process-federation service tests.
namespace umbra::test::process_federation_service_support {

using ::umbra::detail::FederationDefinition;
using ::umbra::detail::FomCompositionStatus;
using ::umbra::detail::FomModuleKind;
using ::umbra::detail::FomValidationStatus;
using ::umbra::detail::LibXml2FomModuleComposer;
using ::umbra::detail::LibXml2FomValidator;
using ::umbra::detail::PrevalidatedFomModule;
using ::umbra::detail::TransportServiceMessage;
using ::umbra::detail::TransportServiceMessageKind;
using ::umbra::detail::TransportServiceOperation;
using ::umbra::detail::TransportServiceStatus;

inline std::filesystem::path resourcePath(
    std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relative;
}

inline PrevalidatedFomModule validatedModule(
    std::filesystem::path const& source,
    FomModuleKind kind,
    std::wstring designator) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      source,
      resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  if (result.status != FomValidationStatus::valid || !result.module) {
    throw std::runtime_error("The process service FOM did not validate.");
  }
  return *result.module;
}

inline FederationDefinition composedRestaurantDefinition() {
  std::vector<PrevalidatedFomModule> modules{
      validatedModule(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:process-service-mim"),
      validatedModule(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:process-service-restaurant"),
  };
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid || !result.catalog ||
      !result.fdd) {
    throw std::runtime_error("The process service FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

inline TransportServiceMessage request(
    TransportServiceOperation operation,
    std::uint64_t requestId,
    std::vector<std::uint8_t> payload) {
  return {
      TransportServiceMessageKind::request,
      operation,
      TransportServiceStatus::ok,
      requestId,
      std::move(payload)};
}

}  // namespace umbra::test::process_federation_service_support
