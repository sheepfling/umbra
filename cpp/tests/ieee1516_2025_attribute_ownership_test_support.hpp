#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#endif

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The IEEE 1516.1-2025 ownership tests require the Umbra source directory."
#endif

namespace umbra::test::attribute_ownership_2025 {

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
using umbra::detail::FederationDefinition;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::FomValidationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;
#endif

inline std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"attribute-ownership-acquisition-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

inline std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
inline std::filesystem::path processResourcePath(
    std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

inline PrevalidatedFomModule validatedProcessModule(
    std::filesystem::path const& source,
    FomModuleKind kind,
    std::wstring designator) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      source,
      processResourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  if (result.status != FomValidationStatus::valid || !result.module) {
    throw std::runtime_error("The process ownership FOM did not validate.");
  }
  return *result.module;
}

inline FederationDefinition composedProcessOwnershipDefinition() {
  std::vector<PrevalidatedFomModule> modules{
      validatedProcessModule(
          processResourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:process-ownership-acquisition-mim"),
      validatedProcessModule(
          resourcePath("attribute-update-passel-fom.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:process-ownership-acquisition-fom"),
  };
  LibXml2FomModuleComposer composer(
      processResourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid ||
      !result.catalog || !result.fdd) {
    throw std::runtime_error("The process ownership FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}
#endif

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
  };

  struct ObjectRemovalReport final {
    ObjectInstanceHandle objectInstance;
  };

  struct AcquisitionReport final {
    enum class Kind { notification, unavailable };

    Kind kind = Kind::unavailable;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct ReleaseRequestReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AcquisitionCancellationReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back({objectInstance, objectClass});
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const&,
      FederateHandle const&) override {
    objectRemovalReports.push_back({objectInstance});
  }

  void attributeOwnershipAcquisitionNotification(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& securedAttributes,
      VariableLengthData const& userSuppliedTag) override {
    acquisitionReports.push_back({
        AcquisitionReport::Kind::notification,
        objectInstance,
        securedAttributes,
        userSuppliedTag,
    });
  }

  void attributeOwnershipUnavailable(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    acquisitionReports.push_back({
        AcquisitionReport::Kind::unavailable,
        objectInstance,
        attributes,
        userSuppliedTag,
    });
  }

  void requestAttributeOwnershipRelease(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& candidateAttributes,
      VariableLengthData const& userSuppliedTag) override {
    releaseRequests.push_back({
        objectInstance,
        candidateAttributes,
        userSuppliedTag,
    });
  }

  void confirmAttributeOwnershipAcquisitionCancellation(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    acquisitionCancellationReports.push_back({objectInstance, attributes});
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  std::vector<AcquisitionReport> acquisitionReports;
  std::vector<ReleaseRequestReport> releaseRequests;
  std::vector<AcquisitionCancellationReport> acquisitionCancellationReports;
};

inline std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

inline std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr || value.size() == 0U) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

}  // namespace umbra::test::attribute_ownership_2025
