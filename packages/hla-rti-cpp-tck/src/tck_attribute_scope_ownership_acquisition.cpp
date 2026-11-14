#include "tck_main_scenarios.hpp"

#include <chrono>
#include <thread>

namespace hla_rti_cpp_tck::detail {

void scenarioAttributeScopeOwnershipAcquisitionException(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session acquirer(options, model, "acquirer");
  auto const federation = federationName(options, "attribute-scope-owner-acquisition");
  connectAndJoin(owner, acquirer, options, federation, options.fom);

  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const acquirerClass =
      acquirer.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute =
      owner.rtiAmbassador().getAttributeHandle(ownerClass, options.attributeName);
  auto const acquirerAttribute =
      acquirer.rtiAmbassador().getAttributeHandle(acquirerClass, options.attributeName);
  require(ownerClass.isValid() && acquirerClass.isValid(),
          "attribute-scope ownership acquisition class lookup failed");
  require(ownerAttribute.isValid() && acquirerAttribute.isValid(),
          "attribute-scope ownership acquisition attribute lookup failed");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const acquirerAttributes{acquirerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  acquirer.rtiAmbassador().publishObjectClassAttributes(
      acquirerClass, acquirerAttributes);
  require(!acquirer.rtiAmbassador().getAttributeScopeAdvisorySwitch(),
          "Attribute Scope Advisory Switch did not start disabled");
  acquirer.rtiAmbassador().setAttributeScopeAdvisorySwitch(true);
  require(acquirer.rtiAmbassador().getAttributeScopeAdvisorySwitch(),
          "Attribute Scope Advisory Switch did not enable for the acquiring federate");
  acquirer.rtiAmbassador().subscribeObjectClassAttributes(
      acquirerClass, acquirerAttributes, true, L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "attribute-scope ownership registration returned an invalid object");
  waitFor(acquirer,
          [&] { return acquirer.recorder().hasDiscovery(object); },
          options,
          "the ownership-acquisition scope object discovery");
  require(owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
          "the acquired attribute was not initially owned by another federate");

  auto drainAcquirer = [&] {
    if (model == rti::HLA_EVOKED) {
      while (acquirer.evokeMultipleCallbacks(0.0, 0.0)) {
      }
    }
  };
  drainAcquirer();
  acquirer.recorder().clearScopeAdvisories();

  rti::VariableLengthData const emptyTag;
  // The discovered, subscribed attribute is in scope while the other
  // federate owns it. Successful acquisition makes the acting federate its
  // owner, which is an implicit out-of-scope cause under §6.1.2.
  owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
      object, ownerAttributes, emptyTag);
  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object, acquirerAttributes, emptyTag);
  waitFor(owner,
          [&] { return owner.recorder().divestitureConfirmation().has_value(); },
          options,
          "the acquisition request at the current owner");
  owner.rtiAmbassador().confirmDivestiture(object, ownerAttributes, emptyTag);
  waitFor(acquirer,
          [&] { return acquirer.recorder().ownershipAcquisition().has_value(); },
          options,
          "the acquisition notification at the acquiring federate");

  auto const acquisition = acquirer.recorder().ownershipAcquisition();
  require(acquisition->object == object && acquisition->attributes == acquirerAttributes,
          "ownership acquisition notification identified the wrong object or attributes");
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
              acquirer.rtiAmbassador().isAttributeOwnedByFederate(object, acquirerAttribute),
          "ownership acquisition did not transfer the attribute to the acting federate");

  auto const observationDeadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (std::chrono::steady_clock::now() < observationDeadline) {
    acquirer.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  drainAcquirer();
  require(acquirer.recorder().attributesOutOfScope().empty(),
          "ownership acquisition emitted an out-of-scope advisory to the acting federate");

  acquirer.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object, acquirerAttributes, emptyTag);
  owner.rtiAmbassador().deleteObjectInstance(object, emptyTag);
  owner.resign(rti::NO_ACTION);
  acquirer.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  acquirer.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
