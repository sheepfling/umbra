#include "internal/federation/federation_registry_state_image_restore_helpers.hpp"

#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <limits>
#include <stdexcept>
#include <utility>

namespace umbra::detail::federation_registry_state_image_restore_helpers {

std::shared_ptr<rti1516_2025::LogicalTime const> decodeLogicalTimeEncoding(
    std::wstring const& implementationName,
    std::optional<std::string> const& encoding) {
  if (!encoding) {
    return nullptr;
  }
  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      implementationName);
  if (!factory || factory->getName() != implementationName) {
    throw std::logic_error(
        "The saved TSO queue does not have a selected logical-time factory.");
  }
  rti1516_2025::VariableLengthData bytes(encoding->data(), encoding->size());
  auto decoded = factory->decodeLogicalTime(bytes);
  if (!decoded || decoded->implementationName() != implementationName) {
    throw std::logic_error(
        "The saved TSO queue contains an invalid logical-time encoding.");
  }
  return std::shared_ptr<rti1516_2025::LogicalTime const>{std::move(decoded)};
}

std::shared_ptr<rti1516_2025::LogicalTimeInterval const>
decodeLogicalTimeIntervalEncoding(
    std::wstring const& implementationName,
    std::optional<std::string> const& encoding) {
  if (!encoding) {
    return nullptr;
  }
  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      implementationName);
  if (!factory || factory->getName() != implementationName) {
    throw std::logic_error(
        "The saved temporal state does not have a selected logical-time factory.");
  }
  rti1516_2025::VariableLengthData bytes(encoding->data(), encoding->size());
  auto decoded = factory->decodeLogicalTimeInterval(bytes);
  if (!decoded || decoded->implementationName() != implementationName) {
    throw std::logic_error(
        "The saved temporal state contains an invalid logical-time interval encoding.");
  }
  return std::shared_ptr<rti1516_2025::LogicalTimeInterval const>{std::move(decoded)};
}

rti1516_2025::VariableLengthData variableLengthDataFromBytes(
    std::string const& value) {
  if (value.empty()) {
    return {};
  }
  return rti1516_2025::VariableLengthData(value.data(), value.size());
}

std::optional<rti1516_2025::OrderType> decodeSavedOrderType(
    std::uint32_t encoded) {
  if (encoded == static_cast<std::uint32_t>(rti1516_2025::RECEIVE)) {
    return rti1516_2025::RECEIVE;
  }
  if (encoded == static_cast<std::uint32_t>(rti1516_2025::TIMESTAMP)) {
    return rti1516_2025::TIMESTAMP;
  }
  return std::nullopt;
}

RegionSpecificationSnapshot decodeSavedRegionSnapshot(
    FederationStateImageInteractionRegionSnapshot const& saved) {
  if (saved.regionHandle == 0U || !saved.specificationCommitted ||
      saved.dimensionHandles.size() != saved.committedRangeBounds.size()) {
    throw std::logic_error(
        "The saved TSO interaction contains an incomplete region snapshot.");
  }

  RegionSpecificationSnapshot result;
  result.specificationCommitted = true;
  for (auto const dimensionHandle : saved.dimensionHandles) {
    if (dimensionHandle == 0U ||
        !result.dimensionHandles.insert(dimensionHandle).second) {
      throw std::logic_error(
          "The saved TSO interaction contains duplicate region dimensions.");
    }
  }
  for (auto const& savedRange : saved.committedRangeBounds) {
    if (savedRange.dimensionHandle == 0U ||
        savedRange.lowerBound > savedRange.upperBound ||
        !result.dimensionHandles.contains(savedRange.dimensionHandle) ||
        savedRange.lowerBound >
            static_cast<std::uint64_t>(
                std::numeric_limits<unsigned long>::max()) ||
        savedRange.upperBound >
            static_cast<std::uint64_t>(
                std::numeric_limits<unsigned long>::max())) {
      throw std::logic_error(
          "The saved TSO interaction contains an invalid region range.");
    }
    auto const [position, inserted] = result.committedRangeBounds.emplace(
        savedRange.dimensionHandle,
        RegionRangeBounds{
            static_cast<unsigned long>(savedRange.lowerBound),
            static_cast<unsigned long>(savedRange.upperBound)});
    static_cast<void>(position);
    if (!inserted) {
      throw std::logic_error(
          "The saved TSO interaction contains duplicate region ranges.");
    }
  }
  return result;
}

} // namespace umbra::detail::federation_registry_state_image_restore_helpers
