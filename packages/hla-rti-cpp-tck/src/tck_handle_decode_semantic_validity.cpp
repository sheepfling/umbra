#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioHandleDecoderDoesNotCheckSemanticValidity(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.ddmFom.empty(),
          "Handle-decoder semantic-validity TCK requires its configured DDM FOM");
  Session source(options, model, "handle-decoder-semantic-source");
  Session decoder(options, model, "handle-decoder-semantic-target");
  auto const sourceFederation =
      federationName(options, "handle-decoder-semantic-source");
  auto const targetFederation =
      federationName(options, "handle-decoder-semantic-target");

  source.connect();
  decoder.connect();
  source.rtiAmbassador().createFederationExecution(
      sourceFederation,
      options.ddmFom.wstring(),
      options.logicalTimeImplementationName);
  source.join(options.ownerFederateName, options.federateType, sourceFederation);
  decoder.rtiAmbassador().createFederationExecution(
      targetFederation,
      options.ddmFom.wstring(),
      options.logicalTimeImplementationName);
  decoder.join(options.memberFederateName, options.federateType, targetFederation);

  auto const objectClass = source.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = source.rtiAmbassador().getAttributeHandle(
      objectClass, options.attributeName);
  rti::AttributeHandleSet attributes;
  attributes.insert(attribute);
  source.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  auto const sourceObject =
      source.rtiAmbassador().registerObjectInstance(objectClass);
  require(sourceObject.isValid(),
          "Source federation returned an invalid object-instance handle");

  auto const decoded = decoder.rtiAmbassador().decodeObjectInstanceHandle(
      sourceObject.encode());
  require(decoded.isValid() && decoded == sourceObject,
          "Object-instance handle decoder checked federation-local semantic validity");
  requireException(
      [&] {
        static_cast<void>(
            decoder.rtiAmbassador().getObjectInstanceName(decoded));
      },
      L"ObjectInstanceNotKnown",
      "semantically unknown decoded object-instance handle lookup");

  source.resign(rti::NO_ACTION);
  decoder.resign(rti::NO_ACTION);
  source.rtiAmbassador().destroyFederationExecution(sourceFederation);
  decoder.rtiAmbassador().destroyFederationExecution(targetFederation);
  source.disconnect();
  decoder.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
