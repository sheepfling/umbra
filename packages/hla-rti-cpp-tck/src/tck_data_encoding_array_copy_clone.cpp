#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

rti::Integer16 arrayElementValue(rti::DataElement const& element) {
  return dynamic_cast<rti::HLAinteger16BE const&>(element).get();
}

}  // namespace

void verifyArrayCopyCloneExternalMemoryContract() {
  rti::HLAinteger16BE fixedExternalElement{101};
  rti::HLAfixedArrayT<rti::HLAinteger16BE> fixedArray{1U};
  fixedArray.setElementPointer(0U, &fixedExternalElement);
  rti::HLAfixedArrayT<rti::HLAinteger16BE> fixedCopy{fixedArray};
  auto fixedCloneBase = fixedArray.clone();
  auto* fixedClone =
      dynamic_cast<rti::HLAfixedArrayT<rti::HLAinteger16BE>*>(
          fixedCloneBase.get());
  require(fixedClone != nullptr, "HLAfixedArray clone lost its typed API");

  fixedExternalElement.set(102);
  require(
      arrayElementValue(fixedArray.get(0U)) == 102 &&
          arrayElementValue(fixedCopy.get(0U)) == 101 &&
          arrayElementValue(fixedClone->get(0U)) == 101 &&
          &fixedCopy.get(0U) != &fixedExternalElement &&
          &fixedClone->get(0U) != &fixedExternalElement,
      "HLAfixedArray copy or clone retained external element storage");
  fixedCopy.set(0U, rti::HLAinteger16BE{201});
  fixedClone->set(0U, rti::HLAinteger16BE{301});
  require(
      fixedExternalElement.get() == 102 &&
          arrayElementValue(fixedCopy.get(0U)) == 201 &&
          arrayElementValue(fixedClone->get(0U)) == 301,
      "HLAfixedArray copy or clone mutation reached another value");

  rti::HLAinteger16BE variableExternalElement{111};
  rti::HLAvariableArrayT<rti::HLAinteger16BE> variableArray;
  variableArray.addElementPointer(&variableExternalElement);
  rti::HLAvariableArrayT<rti::HLAinteger16BE> variableCopy{variableArray};
  auto variableCloneBase = variableArray.clone();
  auto* variableClone =
      dynamic_cast<rti::HLAvariableArrayT<rti::HLAinteger16BE>*>(
          variableCloneBase.get());
  require(
      variableClone != nullptr,
      "HLAvariableArray clone lost its typed API");

  variableExternalElement.set(112);
  require(
      variableArray.size() == 1U && variableCopy.size() == 1U &&
          variableClone->size() == 1U &&
          arrayElementValue(variableArray.get(0U)) == 112 &&
          arrayElementValue(variableCopy.get(0U)) == 111 &&
          arrayElementValue(variableClone->get(0U)) == 111 &&
          &variableCopy.get(0U) != &variableExternalElement &&
          &variableClone->get(0U) != &variableExternalElement,
      "HLAvariableArray copy or clone retained external element storage");
  variableCopy.set(0U, rti::HLAinteger16BE{211});
  variableClone->set(0U, rti::HLAinteger16BE{311});
  require(
      variableExternalElement.get() == 112 &&
          arrayElementValue(variableCopy.get(0U)) == 211 &&
          arrayElementValue(variableClone->get(0U)) == 311,
      "HLAvariableArray copy or clone mutation reached another value");
}

}  // namespace hla_rti_cpp_tck::detail
