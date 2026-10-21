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
          ("umbra-annex-c-general-data-type-merge-" +
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
      std::string const& moduleName) const {
    auto const path = path_ / filename;
    std::ofstream output(path, std::ios::binary);
    REQUIRE(output.good());
    output << R"xml(<?xml version="1.0" encoding="utf-8"?>
<objectModel xmlns="http://standards.ieee.org/IEEE1516-2025"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:schemaLocation="http://standards.ieee.org/IEEE1516-2025 IEEE1516-DIF-2025.xsd">
  <modelIdentification><name>)xml" << moduleName << R"xml(</name><type>FOM</type></modelIdentification>
  <dataTypes>
    <basicDataRepresentations>
      <basicData><name>UmbraAnnexCBasic</name><size>32</size>
        <interpretation>Signed integer for Annex C merge evidence</interpretation>
        <endian>Big</endian><encoding>Two's complement</encoding>
      </basicData>
    </basicDataRepresentations>
    <simpleDataTypes>
      <simpleData><name>UmbraAnnexCSimple</name><representation>HLAinteger32BE</representation>
        <units>NA</units><resolution>1</resolution><accuracy>Perfect</accuracy>
        <semantics>Simple data type merge evidence</semantics>
      </simpleData>
    </simpleDataTypes>
    <referenceDataTypes>
      <referenceDataType><name>UmbraAnnexCReference</name>
        <representation>HLAtoken</representation>
        <referenceClass>HLAobjectRoot</referenceClass>
        <referencedAttribute>HLAprivilegeToDeleteObject</referencedAttribute>
        <semantics>Reference data type merge evidence</semantics>
      </referenceDataType>
    </referenceDataTypes>
    <enumeratedDataTypes>
      <enumeratedData><name>UmbraAnnexCEnumeration</name>
        <representation>HLAinteger32BE</representation>
        <semantics>Enumerated data type merge evidence</semantics>
        <enumerator><name>UmbraAnnexCEnumerationChoice</name><value>1</value></enumerator>
      </enumeratedData>
    </enumeratedDataTypes>
    <arrayDataTypes>
      <arrayData><name>UmbraAnnexCArray</name><dataType>UmbraAnnexCSimple</dataType>
        <cardinality>2</cardinality><encoding>HLAfixedArray</encoding>
        <semantics>Array data type merge evidence</semantics>
      </arrayData>
    </arrayDataTypes>
    <fixedRecordDataTypes>
      <fixedRecordData><name>UmbraAnnexCFixedRecord</name>
        <encoding>HLAfixedRecord</encoding><semantics>Fixed record merge evidence</semantics>
        <field><name>UmbraAnnexCFixedField</name><dataType>UmbraAnnexCSimple</dataType>
          <semantics>Fixed record field merge evidence</semantics></field>
      </fixedRecordData>
    </fixedRecordDataTypes>
    <variantRecordDataTypes>
      <variantRecordData><name>UmbraAnnexCVariantRecord</name>
        <discriminant>UmbraAnnexCEnumeration</discriminant>
        <dataType>UmbraAnnexCEnumeration</dataType>
        <alternative><enumerator>UmbraAnnexCEnumerationChoice</enumerator>
          <name>UmbraAnnexCVariantAlternative</name><dataType>HLAboolean</dataType>
          <semantics>Non-extendable variant alternative merge evidence</semantics></alternative>
        <encoding>HLAvariantRecord</encoding><semantics>Variant record merge evidence</semantics>
      </variantRecordData>
      <variantRecordData><name>UmbraAnnexCExtendableVariant</name>
        <discriminant>UmbraAnnexCEnumeration</discriminant>
        <dataType>UmbraAnnexCEnumeration</dataType>
        <alternative><enumerator>UmbraAnnexCEnumerationChoice</enumerator>
          <name>UmbraAnnexCExtendableAlternative</name><dataType>HLAboolean</dataType>
          <semantics>Extendable variant alternative merge evidence</semantics></alternative>
        <encoding>HLAextendableVariantRecord</encoding>
        <semantics>Extendable variant record merge evidence</semantics>
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
    "Annex C merges matching data-type element classes across FOM modules",
    "[unit][fom][composition][annex-c][annex-c-general-data-type-merge]") {
  TemporaryFomDirectory temporaryDirectory;
  auto const firstModule = temporaryDirectory.write_module("first.xml", "First Module");
  auto const secondModule = temporaryDirectory.write_module("second.xml", "Second Module");
  umbra::detail::LibXml2FomValidator validator;
  auto validateModule = [&](std::filesystem::path const& path, std::wstring designator) {
    auto validation = validator.validate({
        path,
        resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
        umbra::detail::FomModuleKind::fom,
        std::move(designator),
        L"IEEE1516-DIF-2025.xsd",
    });
    CAPTURE(validation.diagnostics);
    REQUIRE(validation.status == umbra::detail::FomValidationStatus::valid);
    REQUIRE(validation.module.has_value());
    return *validation.module;
  };
  std::vector<umbra::detail::PrevalidatedFomModule> modules{
      validated(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          umbra::detail::FomModuleKind::mim,
          L"urn:umbra:test:annex-c-general-mim"),
      validateModule(firstModule, L"urn:umbra:test:annex-c-general-first"),
      validateModule(secondModule, L"urn:umbra:test:annex-c-general-second"),
  };

  auto result = composer().compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == umbra::detail::FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  REQUIRE(result.fdd);

  auto const* basic = result.catalog->dataType("UmbraAnnexCBasic");
  REQUIRE(basic);
  REQUIRE(basic->kind == umbra::detail::FomDataTypeKind::basic);
  REQUIRE(basic->wireEncoding.sizeBits == 32U);

  auto const* simple = result.catalog->dataType("UmbraAnnexCSimple");
  REQUIRE(simple);
  REQUIRE(simple->kind == umbra::detail::FomDataTypeKind::simple);
  REQUIRE(simple->representation == "HLAinteger32BE");

  auto const* reference = result.catalog->dataType("UmbraAnnexCReference");
  REQUIRE(reference);
  REQUIRE(reference->kind == umbra::detail::FomDataTypeKind::reference);
  REQUIRE(reference->representation == "HLAtoken");

  auto const* enumerated = result.catalog->dataType("UmbraAnnexCEnumeration");
  REQUIRE(enumerated);
  REQUIRE(enumerated->kind == umbra::detail::FomDataTypeKind::enumerated);
  REQUIRE(enumerated->representation == "HLAinteger32BE");

  auto const* array = result.catalog->dataType("UmbraAnnexCArray");
  REQUIRE(array);
  REQUIRE(array->kind == umbra::detail::FomDataTypeKind::array);
  REQUIRE(array->elementDataType == "UmbraAnnexCSimple");
  REQUIRE(array->cardinality == "2");

  auto const* fixedRecord = result.catalog->dataType("UmbraAnnexCFixedRecord");
  REQUIRE(fixedRecord);
  REQUIRE(fixedRecord->kind == umbra::detail::FomDataTypeKind::fixed_record);
  REQUIRE(fixedRecord->fields.size() == 1U);
  REQUIRE(fixedRecord->fields.front().name == "UmbraAnnexCFixedField");

  auto const* variant = result.catalog->dataType("UmbraAnnexCVariantRecord");
  REQUIRE(variant);
  REQUIRE(variant->kind == umbra::detail::FomDataTypeKind::variant_record);
  REQUIRE(variant->alternatives.size() == 1U);
  REQUIRE(variant->alternatives.front().name == "UmbraAnnexCVariantAlternative");

  auto const* extendable = result.catalog->dataType("UmbraAnnexCExtendableVariant");
  REQUIRE(extendable);
  REQUIRE(extendable->kind == umbra::detail::FomDataTypeKind::variant_record);
  REQUIRE(extendable->alternatives.size() == 1U);
  REQUIRE(extendable->alternatives.front().name == "UmbraAnnexCExtendableAlternative");
}
