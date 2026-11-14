#pragma once

#include <filesystem>

namespace umbra::test {

int runTlsEncryptionOnlyServiceFixture(
    std::filesystem::path const& directory);
int runTlsServerValidationServiceFixture(
    std::filesystem::path const& directory);
int runTlsDefaultPortServiceFixture(
    std::filesystem::path const& directory);
int runTlsAlternatePortServiceFixture(
    std::filesystem::path const& directory);
int runTlsSecureWebSocketServiceFixture(
    std::filesystem::path const& directory);

}  // namespace umbra::test
