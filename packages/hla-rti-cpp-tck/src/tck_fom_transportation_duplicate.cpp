#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Transportation duplicate requires adapter-supplied FOM modules");
  return found->second;
}

void create(Session& creator, std::wstring const& federation,
            Options const& options, std::vector<std::wstring> const& modules) {
  creator.rtiAmbassador().createFederationExecution(
      federation, modules, options.logicalTimeImplementationName);
}

void runTransportationDuplicate(Options const& options,
                                rti::CallbackModel model, bool noteConflict) {
  require(!options.logicalTimeImplementationName.empty(),
          "Transportation duplicate requires adapter-supplied logical time");
  auto const& base = requiredFom(options, "transportation-duplicate-base");
  auto const& second = requiredFom(options, noteConflict
      ? "transportation-duplicate-note-conflict"
      : "transportation-duplicate-equal");
  Session creator(options, model, "transportation-duplicate-creator");
  Session member(options, model, "transportation-duplicate-member");
  auto const federation = federationName(options, noteConflict
      ? "fom-transportation-note-conflict" : "fom-transportation-duplicate-equal");
  creator.connect();
  member.connect();

  for (auto const& module : {base.wstring(), second.wstring()}) {
    create(creator, federation, options, {module});
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  if (noteConflict) {
    requireException(
        [&] { create(creator, federation, options,
                     {base.wstring(), second.wstring()}); },
        L"InconsistentFOM", "different note references on one transportation type");
    create(creator, federation, options, {base.wstring()});
    creator.rtiAmbassador().destroyFederationExecution(federation);
    auto const& reliabilityConflict = requiredFom(
        options, "transportation-duplicate-reliability-conflict");
    create(creator, federation, options, {reliabilityConflict.wstring()});
    creator.rtiAmbassador().destroyFederationExecution(federation);
    requireException(
        [&] { create(creator, federation, options,
                     {base.wstring(), reliabilityConflict.wstring()}); },
        L"InconsistentFOM",
        "different reliability on one same-named transportation type");
    create(creator, federation, options, {base.wstring()});
    creator.rtiAmbassador().destroyFederationExecution(federation);
    auto const& invalidName = requiredFom(
        options, "transportation-table-invalid-name");
    requireException(
        [&] { create(creator, federation, options, {invalidName.wstring()}); },
        L"InconsistentFOM", "transportation name violates HLA naming conventions");
    auto const& invalidReliability = requiredFom(
        options, "transportation-table-invalid-reliability");
    requireException(
        [&] { create(creator, federation, options,
                     {invalidReliability.wstring()}); },
        L"InvalidFOM", "transportation reliability is outside the DIF enumeration");
  } else {
    create(creator, federation, options,
           {base.wstring(), second.wstring()});
    member.join(options.memberFederateName, options.federateType, federation);
    auto& rti = member.rtiAmbassador();
    auto const transport = rti.getTransportationTypeHandle(L"TckMergeTransport");
    require(transport.isValid() &&
                rti.getTransportationTypeName(transport) == L"TckMergeTransport",
            "Equivalent transportation declaration was not exposed by public lookup");
    member.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  member.disconnect();
  creator.disconnect();
}

void runTransportationDescriptionIgnored(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Transportation description coverage requires adapter-supplied logical time");
  auto const& base = requiredFom(options, "transportation-duplicate-base");
  auto const& descriptionDifferent = requiredFom(
      options, "transportation-description-different");
  Session creator(options, model, "transportation-description-creator");
  Session member(options, model, "transportation-description-member");
  auto const federation = federationName(
      options, "fom-transportation-description-not-fdd");
  creator.connect();
  member.connect();

  auto verifyPublicTransportationLookup = [&] {
    auto& rti = member.rtiAmbassador();
    auto const transport = rti.getTransportationTypeHandle(L"TckMergeTransport");
    require(transport.isValid() &&
                rti.getTransportationTypeName(transport) == L"TckMergeTransport",
            "FDD-equivalent transportation declaration was not exposed by public lookup");
  };

  create(creator, federation, options,
         {base.wstring(), descriptionDifferent.wstring()});
  member.join(options.memberFederateName, options.federateType, federation);
  verifyPublicTransportationLookup();
  member.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);

  create(creator, federation, options, {base.wstring()});
  member.join(
      options.memberFederateName,
      options.federateType,
      federation,
      {descriptionDifferent.wstring()});
  verifyPublicTransportationLookup();
  member.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);

  member.disconnect();
  creator.disconnect();
}

}  // namespace

void scenarioFomTransportationDuplicateEqual(Options const& options,
                                              rti::CallbackModel model) {
  runTransportationDuplicate(options, model, false);
}

void scenarioFomTransportationDuplicateEqualContract(Options const& options,
                                                      rti::CallbackModel model) {
  scenarioFomTransportationDuplicateEqual(options, model);
}

void scenarioFomTransportationDuplicateNoteConflict(Options const& options,
                                                     rti::CallbackModel model) {
  runTransportationDuplicate(options, model, true);
}

void scenarioFomTransportationDuplicateNoteConflictContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomTransportationDuplicateNoteConflict(options, model);
}

void scenarioFomTransportationDescriptionIgnored(
    Options const& options,
    rti::CallbackModel model) {
  runTransportationDescriptionIgnored(options, model);
}

void scenarioFomTransportationDescriptionIgnoredContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomTransportationDescriptionIgnored(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
