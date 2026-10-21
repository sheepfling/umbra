#include <catch2/catch_test_macros.hpp>

#include "internal/fom/fom_catalog.hpp"
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
          ("umbra-annex-c-variant-alternative-merge-" +
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
      std::string const& enumerator,
      std::string const& value) const {
    auto const path = path_ / filename;
    std::ofstream output(path, std::ios::binary);
    REQUIRE(output.good());
    output << R"xml(<?xml version="1.0" encoding="utf-8"?>
<objectModel xmlns="http://standards.ieee.org/IEEE1516-2025"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:schemaLocation="http://standards.ieee.org/IEEE1516-2025 IEEE1516-DIF-2025.xsd">
  <modelIdentification>
    <name>Umbra Annex C Variant Alternative Merge</name>
    <type>FOM</type>
  </modelIdentification>
  <dataTypes>
    <enumeratedDataTypes>
      <enumeratedData>
        <name>UmbraAnnexCVariantChoices</name>
        <representation>HLAinteger32BE</representation>
        <enumerator><name>)xml" << enumerator << R"xml(</name><value>)xml" << value << R"xml(</value></enumerator>
      </enumeratedData>
    </enumeratedDataTypes>
    <variantRecordDataTypes>
      <variantRecordData>
        <name>UmbraAnnexCExtendableVariant</name>
        <discriminant>UmbraSelection</discriminant>
        <dataType>UmbraAnnexCVariantChoices</dataType>
        <alternative>
          <enumerator>)xml" << enumerator << R"xml(</enumerator>
          <name>UmbraAnnexCMergedAlternative</name>
          <dataType>HLAboolean</dataType>
          <semantics>Shared semantics for the merged alternative.</semantics>
        </alternative>
        <encoding>HLAextendableVariantRecord</encoding>
      </variantRecordData>
    </variantRecordDataTypes>
  </dataTypes>
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
    "Annex C merges named extendable-variant alternatives across FOM modules",
    "[unit][fom][composition][annex-c][annex-c-variant-alternative-merge]") {
  TemporaryFomDirectory temporaryDirectory;
  auto const firstModule = temporaryDirectory.write_module(
      "first.xml", "UmbraFirstChoice", "1");
  auto const secondModule = temporaryDirectory.write_module(
      "second.xml", "UmbraSecondChoice", "2");
  std::vector<umbra::detail::PrevalidatedFomModule> modules{
      validated(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          umbra::detail::FomModuleKind::mim,
          L"urn:umbra:test:annex-c-variant-mim"),
      validated(firstModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-variant-first"),
      validated(secondModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-variant-second"),
  };

  auto result = composer().compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == umbra::detail::FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  auto const* merged = result.catalog->dataType("UmbraAnnexCExtendableVariant");
  REQUIRE(merged);
  REQUIRE(merged->kind == umbra::detail::FomDataTypeKind::variant_record);
  REQUIRE(merged->alternatives.size() == 1U);
  auto const& alternative = merged->alternatives.front();
  REQUIRE(alternative.name == "UmbraAnnexCMergedAlternative");
  REQUIRE(alternative.dataType == "HLAboolean");
  REQUIRE(alternative.semantics == "Shared semantics for the merged alternative.");
  REQUIRE(alternative.discriminantEnumerators ==
          std::vector<std::string>{"UmbraFirstChoice, UmbraSecondChoice"});
}
