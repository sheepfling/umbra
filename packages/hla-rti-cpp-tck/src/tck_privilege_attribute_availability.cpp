#include "tck_main_scenarios.hpp"

#include <array>
#include <string>

namespace hla_rti_cpp_tck::detail {

void scenarioPrivilegeAttributeAvailability(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.fom.empty(),
          "HLAprivilegeToDeleteObject availability requires the P0 TCK FOM");

  Session owner(options, model, "privilege-attribute-owner");
  Session observer(options, model, "privilege-attribute-observer");
  auto const federation = federationName(options, "privilege-attribute-availability");
  connectAndJoin(owner, observer, options, federation, options.fom);

  // Keep this list aligned with the complete object-class hierarchy declared
  // by fom/p0-tck.xml. The public API has no object-class enumeration service.
  std::array<std::wstring, 3> const p0ObjectClasses{
      L"HLAobjectRoot",
      L"HLAobjectRoot.TckObject",
      L"HLAobjectRoot.TckObject.TckSecondaryObject"};
  for (auto const& className : p0ObjectClasses) {
    auto const objectClass = owner.rtiAmbassador().getObjectClassHandle(className);
    require(objectClass.isValid(),
            "P0 TCK object-class lookup failed for the declared hierarchy");
    auto const privilege = owner.rtiAmbassador().getAttributeHandle(
        objectClass, L"HLAprivilegeToDeleteObject");
    require(
        privilege.isValid() &&
            owner.rtiAmbassador().getAttributeName(objectClass, privilege) ==
                L"HLAprivilegeToDeleteObject",
        "HLAprivilegeToDeleteObject was unavailable on a P0 TCK object class");
  }

  owner.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  owner.disconnect();
}

void scenarioPrivilegeAttributeAvailabilityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioPrivilegeAttributeAvailability(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
