#pragma once

#include <filesystem>
#include <string>

namespace umbra::test {

int runNoTsoCoordinatorClientProbe(
    std::filesystem::path const& directory,
    std::string const& role);

int runDeleteTsoCoordinatorClientProbe(
    std::filesystem::path const& directory,
    std::string const& role);

}  // namespace umbra::test
