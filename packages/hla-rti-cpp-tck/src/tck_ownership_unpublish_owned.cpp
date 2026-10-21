#include "tck_ownership_unpublish_owned.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioOwnershipUnpublishOwned(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.fom.empty(),
          "owned-attribute unpublication requires an adapter-supplied FOM");
  require(options.attributeName != options.secondaryAttributeName,
          "owned-attribute unpublication requires distinct adapter attributes");
  Session owner(options, model, "owner");
  Session observer(options, model, "observer");
  auto const federation = federationName(options, "ownership-unpublish-owned");
  connectAndJoin(owner, observer, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const observerClass = observer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const removed = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.attributeName);
  auto const retained = owner.rtiAmbassador().getAttributeHandle(
      ownerClass, options.secondaryAttributeName);
  auto const observerRemoved = observer.rtiAmbassador().getAttributeHandle(
      observerClass, options.attributeName);
  auto const observerRetained = observer.rtiAmbassador().getAttributeHandle(
      observerClass, options.secondaryAttributeName);
  require(ownerClass.isValid() && observerClass == ownerClass &&
              removed.isValid() && retained.isValid() && removed != retained &&
              observerRemoved == removed && observerRetained == retained,
          "owned-attribute unpublication did not resolve matching standard handles");
  rti::AttributeHandleSet const both{removed, retained};
  rti::AttributeHandleSet const removedOnly{removed};
  rti::AttributeHandleSet const retainedOnly{retained};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, both);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass, both, true, L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(),
          "owned-attribute unpublication registered an invalid object");
  auto drive = [&] {
    if (model == rti::HLA_IMMEDIATE) {
      require(observer.rtiAmbassador().getObjectClassHandle(
                  options.objectClassName) == observerClass,
              "callback servicing changed the configured object class handle");
    }
  };
  waitFor(observer,
          [&] { drive(); return observer.recorder().hasDiscovery(object); },
          options, "owned-attribute unpublication object discovery");
  require(observer.rtiAmbassador().getKnownObjectClassHandle(object) ==
              observerClass &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, removed) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, retained),
          "registered object did not start with both attributes owned");

  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, removedOnly);
  require(!owner.rtiAmbassador().isAttributeOwnedByFederate(object, removed) &&
              owner.rtiAmbassador().isAttributeOwnedByFederate(object, retained),
          "selective unpublication did not release only the removed attribute");
  observer.recorder().clearOwnershipRecords();
  observer.rtiAmbassador().queryAttributeOwnership(object, both);
  if (model == rti::HLA_EVOKED) {
    require(observer.recorder().ownershipInformationEvents().empty() &&
                observer.recorder().ownershipNotOwnedEvents().empty(),
            "evoked ownership query delivered before callback servicing");
  }
  waitFor(observer,
          [&] {
            drive();
            return !observer.recorder().ownershipInformationEvents().empty() &&
                !observer.recorder().ownershipNotOwnedEvents().empty();
          },
          options, "selective unpublication ownership reports");
  auto const unowned = observer.recorder().ownershipNotOwnedEvents();
  auto const owned = observer.recorder().ownershipInformationEvents();
  require(unowned.size() == 1U && owned.size() == 1U &&
              unowned.front().object == object &&
              unowned.front().attributes == removedOnly &&
              owned.front().object == object &&
              owned.front().attributes == retainedOnly &&
              owned.front().owner == owner.federateHandle(),
          "ownership query did not partition removed and retained attributes");

  observer.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  owner.disconnect();
}

void scenarioOwnershipUnpublishOwnedContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipUnpublishOwned(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
