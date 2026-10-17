#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

struct Options {
  std::filesystem::path fom;
  std::filesystem::path modelFom;
  std::filesystem::path rateFom;
  std::filesystem::path ddmFom;
  std::filesystem::path multiAttributeFom;
  std::filesystem::path threeDimensionalFom;
  std::filesystem::path mimFom;
  std::filesystem::path switchesFom;
  std::filesystem::path autoProvideFom;
  std::filesystem::path knownClassFom;
  std::filesystem::path knownClassDisabledFom;
  std::vector<std::filesystem::path> additionalFomModules;
  std::vector<std::filesystem::path> invalidFomModules;
  std::vector<std::filesystem::path> standardInvalidFomModules;
  std::vector<std::string> scenarios;
  std::string callbackModel = "both";
  std::string providerId = "unspecified";
  std::string results;
  std::string junit;
  std::string connectionLossMarker;
  std::string connectionLossReadyMarker;
  bool connectionLossServerManaged = false;
  std::string federationPrefix = "cpp-tck";
  std::string configurationName;
  std::string ownerConfigurationName;
  std::string memberConfigurationName;
  std::string rtiAddress;
  std::string additionalSettings;
  std::filesystem::path serviceReportDirectory;
  std::wstring federationOverride;
  std::wstring ownerFederateName = L"tck-owner";
  std::wstring memberFederateName = L"tck-member";
  std::wstring federateType = L"tck-type";
  std::wstring objectClassName = L"HLAobjectRoot.TckObject";
  std::wstring attributeName = L"Value";
  std::wstring secondaryAttributeName = L"Name";
  std::wstring objectClassThirdAttributeName = L"PayRate";
  std::wstring autoProvideObjectClassName = L"HLAobjectRoot.TckAutoProvideObject";
  std::wstring autoProvideFirstAttributeName = L"ProviderAValue";
  std::wstring autoProvideSecondAttributeName = L"ProviderBValue";
  std::wstring multiAttributeObjectClassName =
      L"HLAobjectRoot.TckMultiAttributeObject";
  std::wstring multiAttributeFirstName = L"FirstValue";
  std::wstring multiAttributeSecondName = L"SecondValue";
  std::wstring threeDimensionalObjectClassName =
      L"HLAobjectRoot.TckThreeDimensionalObject";
  std::wstring threeDimensionalAttributeName = L"Value";
  std::wstring customRegionalObjectClassName =
      L"HLAobjectRoot.TckTransportRegionalObject";
  std::wstring customRegionalAttributeName = L"RegionalValue";
  std::wstring customRegionalInteractionClassName =
      L"HLAinteractionRoot.TckTransportRegionalInteraction";
  std::wstring customRegionalParameterName = L"RegionalPayload";
  std::wstring interactionClassName = L"HLAinteractionRoot.TckInteraction";
  std::wstring preferredTimestampInteractionClassName =
      L"HLAinteractionRoot.TckPreferredTimestampInteraction";
  std::wstring preferredReceiveInteractionClassName =
      L"HLAinteractionRoot.TckPreferredReceiveInteraction";
  std::wstring secondaryDirectedObjectClassName =
      L"HLAobjectRoot.TckObject.TckSecondaryObject";
  std::wstring secondaryDirectedInteractionClassName =
      L"HLAinteractionRoot.TckSecondaryDirectedInteraction";
  std::wstring secondaryDirectedInteractionParameterName =
      L"SecondaryPayload";
  std::wstring parameterName = L"Payload";
  std::wstring typedObjectClassName = L"HLAobjectRoot.TckTypedObject";
  std::wstring typedDerivedObjectClassName =
      L"HLAobjectRoot.TckTypedObject.TckTypedDerivedObject";
  std::wstring typedIdentityAttributeName = L"Identity";
  std::wstring typedIntegerAttributeName = L"IntegerValue";
  std::wstring typedFloatAttributeName = L"FloatValue";
  std::wstring typedAsciiAttributeName = L"AsciiValue";
  std::wstring typedUnicodeAttributeName = L"UnicodeValue";
  std::wstring typedEnumAttributeName = L"EnumValue";
  std::wstring typedFixedArrayAttributeName = L"FixedArrayValue";
  std::wstring typedVariableArrayAttributeName = L"VariableArrayValue";
  std::wstring typedRecordAttributeName = L"RecordValue";
  std::wstring typedChoiceAttributeName = L"ChoiceValue";
  std::wstring typedReferenceAttributeName = L"ReferenceValue";
  std::wstring typedOpaqueAttributeName = L"OpaqueValue";
  std::wstring typedDerivedAttributeName = L"DerivedValue";
  std::wstring knownClassObjectClassName =
      L"HLAobjectRoot.TckKnownClassObject";
  std::wstring knownClassDerivedObjectClassName =
      L"HLAobjectRoot.TckKnownClassObject.TckKnownClassDerivedObject";
  std::wstring knownClassKnownAttributeName = L"Name";
  std::wstring knownClassDerivedAttributeName = L"Efficiency";
  std::wstring typedInteractionClassName = L"HLAinteractionRoot.TckTypedInteraction";
  std::wstring typedDerivedInteractionClassName =
      L"HLAinteractionRoot.TckTypedInteraction.TckTypedDerivedInteraction";
  std::wstring typedIntegerParameterName = L"IntegerParameter";
  std::wstring typedAsciiParameterName = L"AsciiParameter";
  std::wstring typedFixedArrayParameterName = L"FixedArrayParameter";
  std::wstring typedRecordParameterName = L"RecordParameter";
  std::wstring typedChoiceParameterName = L"ChoiceParameter";
  std::wstring typedDerivedParameterName = L"DerivedParameter";
  std::wstring rateObjectClassName = L"HLAobjectRoot.TckRateObject";
  std::wstring rateReliableAttributeName = L"ReliableValue";
  std::wstring rateBestEffortAttributeName = L"BestEffortValue";
  std::wstring rateDesignatorName = L"TckSlow";
  std::wstring fomDimensionName = L"TckDimension";
  std::wstring fomAdditionalDimensionName = L"TckAdditionalDimension";
  std::wstring fomAdditionalDimensionObjectClassName =
      L"HLAobjectRoot.TckExtensionDimensionalObject";
  std::wstring fomAdditionalDimensionAttributeName = L"AdditionalValue";
  std::vector<std::wstring> ddmDimensionNames = {L"TckDimensionX", L"TckDimensionY"};
  std::wstring ddmUnrelatedDimensionName = L"TckUnrelatedDimension";
  bool ddmDimensionsConfigured = false;
  std::vector<std::wstring> threeDimensionalDimensionNames = {
      L"TckDimensionX", L"TckDimensionY", L"TckDimensionZ"};
  bool threeDimensionalDimensionsConfigured = false;
  std::wstring fomUpdateRateName = L"TckFast";
  std::wstring fomAdditionalUpdateRateName = L"TckAdditionalRate";
  std::wstring fomCustomTransportationName = L"TckBestEffort";
  std::wstring fomAdditionalTransportationName = L"TckAdditionalTransport";
  bool requireCustomTransportation = false;
  std::wstring logicalTimeImplementationName;
  int timeoutMilliseconds = 5000;
};

}  // namespace hla_rti_cpp_tck::detail
