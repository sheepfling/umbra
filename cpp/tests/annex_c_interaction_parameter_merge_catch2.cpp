#include <catch2/catch_test_macros.hpp>

#include "internal/fom/fom_catalog.hpp"
#include "fom_composer_test_support.hpp"
#include "scoped_temporary_directory.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

class TemporaryFomDirectory final {
 public:
  TemporaryFomDirectory()
      : directory_(umbra::test_support::ScopedTemporaryDirectory::reserve(
            "umbra-annex-c-interaction-parameter-merge")) {}

  TemporaryFomDirectory(TemporaryFomDirectory const&) = delete;
  TemporaryFomDirectory& operator=(TemporaryFomDirectory const&) = delete;

  [[nodiscard]] std::filesystem::path write_module(
      std::string const& filename,
      std::string const& moduleName,
      std::string const& parameterName,
      std::string const& dataType) const {
    auto const path = directory_.path() / filename;
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
        <name>UmbraAnnexCParameterMergeInteraction</name>
        <sharing>PublishSubscribe</sharing>
        <transportation>HLAreliable</transportation>
        <order>Receive</order>
        <parameter><name>)xml" << parameterName << R"xml(</name><dataType>)xml" << dataType << R"xml(</dataType></parameter>
      </interactionClass>
    </interactionClass>
  </interactions>
</objectModel>
)xml";
    REQUIRE(output.good());
    return path;
  }

 private:
  umbra::test_support::ScopedTemporaryDirectory directory_;
};

}  // namespace

TEST_CASE(
    "Annex C merges disjoint same-named interaction parameters across FOM modules",
    "[unit][fom][composition][annex-c][annex-c-interaction-parameter-merge]") {
  TemporaryFomDirectory temporaryDirectory;
  auto const firstModule = temporaryDirectory.write_module(
      "first.xml", "First Module", "UmbraFirstParameter", "HLAunicodeString");
  auto const secondModule = temporaryDirectory.write_module(
      "second.xml", "Second Module", "UmbraSecondParameter", "HLAboolean");
  std::vector<umbra::detail::PrevalidatedFomModule> modules{
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"),
                umbra::detail::FomModuleKind::mim,
                L"urn:umbra:test:annex-c-interaction-parameter-mim"),
      validated(firstModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-interaction-parameter-first"),
      validated(secondModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-interaction-parameter-second"),
  };

  auto result = composer().compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == umbra::detail::FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  auto const* merged = result.catalog->interactionClass(
      "HLAinteractionRoot.UmbraAnnexCParameterMergeInteraction");
  REQUIRE(merged);
  REQUIRE(merged->declaredParameters.size() == 2U);
  REQUIRE(merged->declaredParameters.at("UmbraFirstParameter").dataType ==
          "HLAunicodeString");
  REQUIRE(merged->declaredParameters.at("UmbraSecondParameter").dataType ==
          "HLAboolean");
}
