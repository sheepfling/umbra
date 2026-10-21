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
          ("umbra-annex-c-object-attribute-transport-conflict-" +
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
      std::string const& transportation) const {
    auto const path = path_ / filename;
    std::ofstream output(path, std::ios::binary);
    REQUIRE(output.good());
    output << R"xml(<?xml version="1.0" encoding="utf-8"?>
<objectModel xmlns="http://standards.ieee.org/IEEE1516-2025"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:schemaLocation="http://standards.ieee.org/IEEE1516-2025 IEEE1516-DIF-2025.xsd">
  <modelIdentification><name>)xml" << moduleName << R"xml(</name><type>FOM</type></modelIdentification>
  <objects><objectClass><name>HLAobjectRoot</name>
    <objectClass><name>UmbraAnnexCAttributeTransportConflict</name><sharing>PublishSubscribe</sharing>
      <semantics>Shared class declaration</semantics>
      <attribute><name>SharedValue</name><dataType>HLAunicodeString</dataType>
        <updateType>Static</updateType><updateCondition>NA</updateCondition>
        <valueRequired>false</valueRequired><ownership>DivestAcquire</ownership>
        <sharing>PublishSubscribe</sharing><transportation>)xml" << transportation << R"xml(</transportation>
        <order>Receive</order><semantics>Shared attribute declaration</semantics>
      </attribute>
    </objectClass>
  </objectClass></objects>
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
    "Internal FOM composition rejects conflicting transportation on a shared object attribute",
    "[unit][internal-only][no-normative-mapping][fom][composition]"
    "[annex-c-object-attribute-transportation-conflict]") {
  TemporaryFomDirectory temporaryDirectory;
  auto const firstModule = temporaryDirectory.write_module(
      "first.xml", "First Module", "HLAreliable");
  auto const secondModule = temporaryDirectory.write_module(
      "second.xml", "Second Module", "HLAbestEffort");
  std::vector<umbra::detail::PrevalidatedFomModule> modules{
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"),
                umbra::detail::FomModuleKind::mim,
                L"urn:umbra:test:annex-c-attribute-transport-conflict-mim"),
      validated(firstModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-attribute-transport-conflict-first"),
      validated(secondModule, umbra::detail::FomModuleKind::fom,
                L"urn:umbra:test:annex-c-attribute-transport-conflict-second"),
  };

  auto const result = composer().compose(modules);
  REQUIRE(result.status == umbra::detail::FomCompositionStatus::inconsistent_modules);
  REQUIRE_FALSE(result.catalog);
  REQUIRE(result.diagnostics.find(
              "Conflicting value at objectModel/objects/objectClass[name=\"HLAobjectRoot\"]/objectClass[name=\"UmbraAnnexCAttributeTransportConflict\"]/attribute[name=\"SharedValue\"]/transportation") !=
          std::string::npos);
}
