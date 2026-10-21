#include <catch2/catch_test_macros.hpp>

#include "internal/fom/libxml2_fom_composer.hpp"
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
          ("umbra-annex-c-interaction-class-semantics-conflict-" +
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
      std::string const& interactionSemantics) const {
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
        <name>UmbraAnnexCInteractionSemanticsConflict</name>
        <sharing>PublishSubscribe</sharing>
        <transportation>HLAreliable</transportation>
        <order>Receive</order>
        <semantics>)xml" << interactionSemantics << R"xml(</semantics>
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
    "Internal Annex C composition rejects conflicting semantics on a shared interaction class",
    "[unit][internal-only][no-normative-mapping][fom][composition]"
    "[annex-c-interaction-class-semantics-conflict]") {
  TemporaryFomDirectory temporaryDirectory;
  auto const firstModule = temporaryDirectory.write_module(
      "first.xml", "First Module", "Semantics from the first module.");
  auto const secondModule = temporaryDirectory.write_module(
      "second.xml", "Second Module", "Different semantics from the second module.");
  std::vector<umbra::detail::PrevalidatedFomModule> modules{
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"),
                umbra::detail::FomModuleKind::mim,
                L"urn:umbra:test:annex-c-interaction-semantics-mim"),
      validated(firstModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-interaction-semantics-first"),
      validated(secondModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-interaction-semantics-second"),
  };

  auto const result = composer().compose(modules);
  REQUIRE(result.status == umbra::detail::FomCompositionStatus::inconsistent_modules);
  REQUIRE_FALSE(result.catalog);
  CHECK(result.diagnostics.find(
            "Conflicting value at objectModel/interactions/interactionClass[name=\"HLAinteractionRoot\"]/interactionClass[name=\"UmbraAnnexCInteractionSemanticsConflict\"]/semantics") !=
        std::string::npos);
}
