#pragma once

#include "ieee1516_2025_federation_management_test_support.hpp"

namespace {
std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"connection-loss-cancel-delete-divest-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}
}  // namespace
