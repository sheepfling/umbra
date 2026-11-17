#include "tck_harness.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioUnsupportedFomTransportationFallback(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.additionalSettings.empty(),
      "transportation fallback requires adapter-supplied additional settings");
  require(
      !options.modelFom.empty() && !options.additionalFomModules.empty(),
      "transportation fallback requires the rich and extension FOM modules");

  Session owner(options, model, "unsupported-transportation-fallback-owner");
  auto const connection = owner.connect();
  require(
      connection.additionalSettingsResult == rti::SETTINGS_APPLIED,
      "adapter settings did not establish the unavailable-transport precondition");

  auto const federation = federationName(options, "unsupported-transportation-fallback");
  std::vector<std::wstring> fomModules{options.modelFom.wstring()};
  for (auto const& module : options.additionalFomModules) {
    fomModules.push_back(module.wstring());
  }
  owner.rtiAmbassador().createFederationExecution(
      federation,
      fomModules,
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto& rti = owner.rtiAmbassador();
  auto const objectClass = rti.getObjectClassHandle(options.customRegionalObjectClassName);
  auto const attribute = rti.getAttributeHandle(
      objectClass,
      options.customRegionalAttributeName);
  auto const interaction =
      rti.getInteractionClassHandle(options.customRegionalInteractionClassName);
  auto const bestEffortCustom =
      rti.getTransportationTypeHandle(options.fomCustomTransportationName);
  auto const reliableCustom =
      rti.getTransportationTypeHandle(options.fomAdditionalTransportationName);
  auto const bestEffort = rti.getTransportationTypeHandle(L"HLAbestEffort");
  auto const reliable = rti.getTransportationTypeHandle(L"HLAreliable");
  require(
      objectClass.isValid() && attribute.isValid() && interaction.isValid(),
      "fallback FOM lookup returned an invalid object, attribute, or interaction handle");
  require(
      bestEffortCustom.isValid() && reliableCustom.isValid() &&
          bestEffortCustom != reliableCustom && bestEffort.isValid() &&
          reliable.isValid() && bestEffort != reliable &&
          rti.getTransportationTypeName(bestEffortCustom) ==
              options.fomCustomTransportationName &&
          rti.getTransportationTypeName(reliableCustom) ==
              options.fomAdditionalTransportationName &&
          rti.getTransportationTypeName(bestEffort) == L"HLAbestEffort" &&
          rti.getTransportationTypeName(reliable) == L"HLAreliable",
      "transportation type lookup did not preserve custom and standard identities");

  rti::AttributeHandleSet attributes{attribute};
  rti.publishObjectClassAttributes(objectClass, attributes);
  rti.publishInteractionClass(interaction);
  auto const object = rti.registerObjectInstance(objectClass);
  require(object.isValid(), "fallback attribute test could not register its object");

  auto requireAttributeReport = [&](std::wstring const& expectedName,
                                    std::string const& description) {
    owner.recorder().clearTransportationRecords();
    rti.queryAttributeTransportationType(object, attribute);
    waitFor(
        owner,
        [&] { return !owner.recorder().attributeTransportationReports().empty(); },
        options,
        description);
    auto const reports = owner.recorder().attributeTransportationReports();
    auto const report = std::find_if(
        reports.begin(),
        reports.end(),
        [&](auto const& row) {
          return row.object == object && row.attribute == attribute;
        });
    require(
        report != reports.end() && report->transportation.isValid() &&
            rti.getTransportationTypeName(report->transportation) == expectedName,
        description + " reported the wrong effective transportation type");
  };

  auto requireInteractionReport = [&](std::wstring const& expectedName,
                                      std::string const& description) {
    owner.recorder().clearTransportationRecords();
    rti.queryInteractionTransportationType(owner.federateHandle(), interaction);
    waitFor(
        owner,
        [&] { return !owner.recorder().interactionTransportationReports().empty(); },
        options,
        description);
    auto const reports = owner.recorder().interactionTransportationReports();
    auto const report = std::find_if(
        reports.begin(),
        reports.end(),
        [&](auto const& row) { return row.interaction == interaction; });
    require(
        report != reports.end() && report->transportation.isValid() &&
            rti.getTransportationTypeName(report->transportation) == expectedName,
        description + " reported the wrong effective transportation type");
  };

  requireAttributeReport(
      L"HLAbestEffort",
      "FOM-declared unsupported best-effort attribute transportation query");
  requireInteractionReport(
      L"HLAbestEffort",
      "FOM-declared unsupported best-effort interaction transportation query");

  owner.recorder().clearTransportationRecords();
  rti.requestAttributeTransportationTypeChange(object, attributes, reliableCustom);
  waitFor(
      owner,
      [&] { return !owner.recorder().attributeTransportationConfirmations().empty(); },
      options,
      "unsupported reliable attribute transportation change confirmation");
  auto const attributeConfirmations =
      owner.recorder().attributeTransportationConfirmations();
  require(
      attributeConfirmations.back().attributes.count(attribute) == 1U &&
          attributeConfirmations.back().transportation.isValid() &&
          rti.getTransportationTypeName(
              attributeConfirmations.back().transportation) == L"HLAreliable",
      "unsupported reliable attribute transport did not fall back to HLAreliable");
  requireAttributeReport(
      L"HLAreliable",
      "post-fallback attribute transportation query");

  owner.recorder().clearTransportationRecords();
  rti.requestInteractionTransportationTypeChange(interaction, reliableCustom);
  waitFor(
      owner,
      [&] { return !owner.recorder().interactionTransportationConfirmations().empty(); },
      options,
      "unsupported reliable interaction transportation change confirmation");
  auto const interactionConfirmations =
      owner.recorder().interactionTransportationConfirmations();
  require(
      interactionConfirmations.back().interaction == interaction &&
          interactionConfirmations.back().transportation.isValid() &&
          rti.getTransportationTypeName(
              interactionConfirmations.back().transportation) == L"HLAreliable",
      "unsupported reliable interaction transport did not fall back to HLAreliable");
  requireInteractionReport(
      L"HLAreliable",
      "post-fallback interaction transportation query");

  owner.resign(rti::DELETE_OBJECTS);
  rti.destroyFederationExecution(federation);
  owner.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
