#include "tck_argument_file.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

namespace hla_rti_cpp_tck::detail {

std::vector<std::string> expandArgumentFiles(int argc, char** argv) {
  std::vector<std::string> expanded;
  if (argc > 0) {
    expanded.emplace_back(argv[0]);
  }

  for (int index = 1; index < argc; ++index) {
    std::string const argument{argv[index]};
    if (argument != "--common-arguments-file") {
      expanded.push_back(argument);
      continue;
    }

    if (index + 1 >= argc) {
      throw std::invalid_argument(
          "--common-arguments-file requires a file path");
    }

    auto const filePath = argv[++index];
    std::ifstream input{filePath, std::ios::binary};
    if (!input) {
      throw std::runtime_error(
          "Unable to open TCK common-arguments file: " +
          std::string{filePath});
    }

    std::string line;
    while (std::getline(input, line)) {
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      expanded.push_back(std::move(line));
    }
    if (input.bad()) {
      throw std::runtime_error(
          "Unable to read TCK common-arguments file: " +
          std::string{filePath});
    }
  }

  return expanded;
}

}  // namespace hla_rti_cpp_tck::detail
