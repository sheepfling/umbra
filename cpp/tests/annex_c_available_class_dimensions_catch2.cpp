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
            "umbra-annex-c-available-class-dimensions")) {}

  TemporaryFomDirectory(TemporaryFomDirectory const&) = delete;
  TemporaryFomDirectory& operator=(TemporaryFomDirectory const&) = delete;

  [[nodiscard]] std::filesystem::path write_module(
      std::string const& filename,
      std::string const& moduleName,
      std::string const& dimensionName) const {
    auto const path = directory_.path() / filename;
    std::ofstream output(path, std::ios::binary);
    REQUIRE(output.good());
    output << R"xml(<?xml version="1.0" encoding="utf-8"?>
<objectModel xmlns="http://standards.ieee.org/IEEE1516-2025"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:schemaLocation="http://standards.ieee.org/IEEE1516-2025 IEEE1516-DIF-2025.xsd">
  <modelIdentification><name>)xml" << moduleName << R"xml(</name><type>FOM</type></modelIdentification>
  <objects>
    <objectClass>
      <name>HLAobjectRoot</name>
      <objectClass>
        <name>UmbraAvailableDimensionsClass</name>
        <sharing>PublishSubscribe</sharing>
        <dimensions><dimension>)xml" << dimensionName << R"xml(</dimension></dimensions>
      </objectClass>
    </objectClass>
  </objects>
  <dimensions>
    <dimension>
      <name>)xml" << dimensionName << R"xml(</name>
      <inputDataTypes><dataType>HLAcount</dataType></inputDataTypes>
      <inputDataDescription>NA</inputDataDescription>
      <upperBound>100</upperBound>
      <normalization>Fixture normalization.</normalization>
      <outputDataSemantics>Fixture normalized coordinate.</outputDataSemantics>
      <value>[0..100)</value>
    </dimension>
  </dimensions>
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
    "Annex C composition merges available dimensions for one named object class",
    "[unit][fom][composition][annex-c][annex-c-available-class-dimensions][available-class-dimensions]") {
  TemporaryFomDirectory temporaryDirectory;
  auto const firstModule = temporaryDirectory.write_module(
      "first.xml", "First Module", "UmbraFirstAvailableDimension");
  auto const secondModule = temporaryDirectory.write_module(
      "second.xml", "Second Module", "UmbraSecondAvailableDimension");
  std::vector<umbra::detail::PrevalidatedFomModule> modules{
      validated(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          umbra::detail::FomModuleKind::mim,
          L"urn:umbra:test:annex-c-class-dimensions-mim"),
      validated(firstModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-class-dimensions-first"),
      validated(secondModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-class-dimensions-second"),
  };

  auto result = composer().compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == umbra::detail::FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  auto const* objectClass =
      result.catalog->objectClass("HLAobjectRoot.UmbraAvailableDimensionsClass");
  REQUIRE(objectClass != nullptr);
  CHECK(objectClass->dimensions == std::vector<std::string>{
                                      "UmbraFirstAvailableDimension",
                                      "UmbraSecondAvailableDimension"});
}
