#include <catch2/catch_test_macros.hpp>

#include "internal/fom/libxml2_fom_composer.hpp"
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
            "umbra-annex-c-object-attribute-semantics-conflict")) {}

  TemporaryFomDirectory(TemporaryFomDirectory const&) = delete;
  TemporaryFomDirectory& operator=(TemporaryFomDirectory const&) = delete;

  [[nodiscard]] std::filesystem::path write_module(
      std::string const& filename,
      std::string const& moduleName,
      std::string const& attributeSemantics) const {
    auto const path = directory_.path() / filename;
    std::ofstream output(path, std::ios::binary);
    REQUIRE(output.good());
    output << R"xml(<?xml version="1.0" encoding="utf-8"?>
<objectModel xmlns="http://standards.ieee.org/IEEE1516-2025"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:schemaLocation="http://standards.ieee.org/IEEE1516-2025 IEEE1516-DIF-2025.xsd">
  <modelIdentification><name>)xml" << moduleName << R"xml(</name><type>FOM</type></modelIdentification>
  <objects><objectClass><name>HLAobjectRoot</name>
    <objectClass><name>UmbraAnnexCAttributeSemanticsConflict</name><sharing>PublishSubscribe</sharing>
      <semantics>Shared class declaration</semantics>
      <attribute><name>SharedValue</name><dataType>HLAunicodeString</dataType>
        <updateType>Static</updateType><updateCondition>NA</updateCondition>
        <valueRequired>false</valueRequired><ownership>DivestAcquire</ownership>
        <sharing>PublishSubscribe</sharing><transportation>HLAreliable</transportation>
        <order>Receive</order><semantics>)xml" << attributeSemantics << R"xml(</semantics>
      </attribute>
    </objectClass>
  </objectClass></objects>
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
    "Annex C rejects conflicting semantics for a shared object class attribute",
    "[unit][fom][composition][annex-c][object-attribute-semantics-conflict]") {
  TemporaryFomDirectory temporaryDirectory;
  auto const firstModule = temporaryDirectory.write_module(
      "first.xml", "First Module", "Semantics from the first module.");
  auto const secondModule = temporaryDirectory.write_module(
      "second.xml", "Second Module", "Semantics from the second module.");
  std::vector<umbra::detail::PrevalidatedFomModule> modules{
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"),
                umbra::detail::FomModuleKind::mim,
                L"urn:umbra:test:annex-c-attribute-semantics-mim"),
      validated(firstModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-attribute-semantics-first"),
      validated(secondModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-attribute-semantics-second"),
  };

  auto result = composer().compose(modules);
  REQUIRE(result.status == umbra::detail::FomCompositionStatus::inconsistent_modules);
  REQUIRE_FALSE(result.catalog);
  REQUIRE(result.diagnostics.find(
              "Conflicting value at objectModel/objects/objectClass[name=\"HLAobjectRoot\"]/objectClass[name=\"UmbraAnnexCAttributeSemanticsConflict\"]/attribute[name=\"SharedValue\"]/semantics") !=
          std::string::npos);
}
