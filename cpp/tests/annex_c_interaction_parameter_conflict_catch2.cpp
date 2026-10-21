#include <catch2/catch_test_macros.hpp>

#include "fom_composer_test_support.hpp"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

class TemporaryFomDirectory final {
 public:
  TemporaryFomDirectory() {
    static std::atomic_uint64_t next{0U};
    do {
      path_ = std::filesystem::temp_directory_path() /
          ("umbra-annex-c-interaction-parameter-conflict-" +
           std::to_string(++next));
    } while (!std::filesystem::create_directory(path_));
  }

  TemporaryFomDirectory(TemporaryFomDirectory const&) = delete;
  TemporaryFomDirectory& operator=(TemporaryFomDirectory const&) = delete;

  ~TemporaryFomDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }

  [[nodiscard]] std::filesystem::path write_module(
      std::string const& filename,
      std::string const& moduleName,
      std::string const& dataType) const {
    auto const path = path_ / filename;
    std::ofstream output(path, std::ios::binary);
    REQUIRE(output.good());
    output << R"xml(<?xml version="1.0" encoding="utf-8"?>
<objectModel xmlns="http://standards.ieee.org/IEEE1516-2025"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:schemaLocation="http://standards.ieee.org/IEEE1516-2025 IEEE1516-DIF-2025.xsd">
  <modelIdentification><name>)xml" << moduleName << R"xml(</name><type>FOM</type></modelIdentification>
  <interactions>
    <interactionClass>
      <name>HLAinteractionRoot</name>
      <interactionClass>
        <name>UmbraAnnexCParameterConflictInteraction</name>
        <sharing>PublishSubscribe</sharing>
        <transportation>HLAreliable</transportation>
        <order>Receive</order>
        <parameter><name>UmbraConflictingParameter</name><dataType>)xml"
           << dataType << R"xml(</dataType></parameter>
      </interactionClass>
    </interactionClass>
  </interactions>
</objectModel>
)xml";
    REQUIRE(output.good());
    return path;
  }

 private:
  std::filesystem::path path_;
};

}  // namespace

TEST_CASE(
    "Annex C rejects conflicting values for a same-named interaction parameter",
    "[unit][fom][composition][annex-c][annex-c-interaction-parameter-conflict]") {
  TemporaryFomDirectory temporaryDirectory;
  auto const firstModule = temporaryDirectory.write_module(
      "first.xml", "First Module", "HLAunicodeString");
  auto const secondModule = temporaryDirectory.write_module(
      "second.xml", "Second Module", "HLAboolean");
  std::vector<umbra::detail::PrevalidatedFomModule> modules{
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"),
                umbra::detail::FomModuleKind::mim,
                L"urn:umbra:test:annex-c-interaction-parameter-conflict-mim"),
      validated(firstModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-interaction-parameter-conflict-first"),
      validated(secondModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-interaction-parameter-conflict-second"),
  };

  auto result = composer().compose(modules);
  REQUIRE(result.status ==
          umbra::detail::FomCompositionStatus::inconsistent_modules);
  REQUIRE_FALSE(result.catalog);
  REQUIRE(result.diagnostics.find(
              "Conflicting value at objectModel/interactions/interactionClass[name=\"HLAinteractionRoot\"]/interactionClass[name=\"UmbraAnnexCParameterConflictInteraction\"]/parameter[name=\"UmbraConflictingParameter\"]/dataType") !=
          std::string::npos);
}
