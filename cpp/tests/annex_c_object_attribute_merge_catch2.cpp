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
            "umbra-annex-c-object-attribute-merge")) {}

  TemporaryFomDirectory(TemporaryFomDirectory const&) = delete;
  TemporaryFomDirectory& operator=(TemporaryFomDirectory const&) = delete;

  [[nodiscard]] std::filesystem::path write_module(
      std::string const& filename,
      std::string const& attributeName,
      std::string const& dataType) const {
    auto const path = directory_.path() / filename;
    std::ofstream output(path, std::ios::binary);
    REQUIRE(output.good());
    output << R"xml(<?xml version="1.0" encoding="utf-8"?>
<objectModel xmlns="http://standards.ieee.org/IEEE1516-2025"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:schemaLocation="http://standards.ieee.org/IEEE1516-2025 IEEE1516-DIF-2025.xsd">
  <modelIdentification><name>Umbra Annex C Object Attribute Merge</name><type>FOM</type></modelIdentification>
  <objects><objectClass><name>HLAobjectRoot</name>
    <objectClass><name>UmbraAnnexCAttributeMerge</name><sharing>PublishSubscribe</sharing>
      <semantics>Shared class declaration</semantics>
      <attribute><name>)xml" << attributeName << R"xml(</name><dataType>)xml" << dataType << R"xml(</dataType>
        <updateType>Static</updateType><updateCondition>NA</updateCondition>
        <valueRequired>false</valueRequired><ownership>DivestAcquire</ownership>
        <sharing>PublishSubscribe</sharing><transportation>HLAreliable</transportation>
        <order>Receive</order><semantics>Metadata for )xml" << attributeName << R"xml(</semantics>
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
    "Annex C merges disjoint attributes declared on the same object class across FOM modules",
    "[unit][fom][composition][annex-c][annex-c-object-attribute-merge]") {
  TemporaryFomDirectory temporaryDirectory;
  auto const firstModule = temporaryDirectory.write_module("first.xml", "FirstValue", "HLAunicodeString");
  auto const secondModule = temporaryDirectory.write_module("second.xml", "SecondValue", "HLAunicodeString");
  std::vector<umbra::detail::PrevalidatedFomModule> modules{
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"),
                umbra::detail::FomModuleKind::mim, L"urn:umbra:test:annex-c-object-attribute-mim"),
      validated(firstModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-object-attribute-first"),
      validated(secondModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-object-attribute-second"),
  };

  auto result = composer().compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == umbra::detail::FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  auto const* objectClass = result.catalog->objectClass("HLAobjectRoot.UmbraAnnexCAttributeMerge");
  REQUIRE(objectClass);
  REQUIRE(objectClass->sharing == "PublishSubscribe");
  REQUIRE(objectClass->semantics == "Shared class declaration");
  REQUIRE(objectClass->declaredAttributes.size() == 2U);

  auto const first = objectClass->declaredAttributes.find("FirstValue");
  REQUIRE(first != objectClass->declaredAttributes.end());
  REQUIRE(first->second.dataType == "HLAunicodeString");
  REQUIRE(first->second.ownership == "DivestAcquire");
  auto const second = objectClass->declaredAttributes.find("SecondValue");
  REQUIRE(second != objectClass->declaredAttributes.end());
  REQUIRE(second->second.dataType == "HLAunicodeString");
  REQUIRE(second->second.transportation == "HLAreliable");
}
