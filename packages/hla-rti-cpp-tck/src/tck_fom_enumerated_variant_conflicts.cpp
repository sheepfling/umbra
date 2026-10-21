#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Enumerated/variant conflict requires two adapter-supplied FOM modules");
  return found->second;
}

void create(Session& creator, std::wstring const& federation,
            Options const& options, std::vector<std::wstring> const& modules) {
  creator.rtiAmbassador().createFederationExecution(
      federation, modules, options.logicalTimeImplementationName);
}

void runConflict(Options const& options, rti::CallbackModel model,
                 bool nonextendableVariant) {
  require(!options.logicalTimeImplementationName.empty(),
          "Enumerated/variant conflict requires adapter-supplied logical time");
  auto const& first = requiredFom(options, nonextendableVariant
      ? "nonextendable-variant-first" : "enumerated-value-collision-first");
  auto const& second = requiredFom(options, nonextendableVariant
      ? "nonextendable-variant-second" : "enumerated-value-collision-second");
  Session creator(options, model, "enumerated-variant-conflict-creator");
  auto const federation = federationName(options, nonextendableVariant
      ? "fom-nonextendable-variant-conflict" : "fom-enumerated-value-collision");
  creator.connect();

  // Both schema-valid modules must be accepted alone; only their combined
  // same-named declaration is supposed to fail composition.
  for (auto const& module : {first.wstring(), second.wstring()}) {
    create(creator, federation, options, {module});
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  requireException(
      [&] { create(creator, federation, options,
                   {first.wstring(), second.wstring()}); },
      L"InconsistentFOM",
      nonextendableVariant
          ? "different alternatives on one named HLAvariantRecord"
          : "one enumerated value assigned to different names");
  create(creator, federation, options, {first.wstring()});
  creator.rtiAmbassador().destroyFederationExecution(federation);
  creator.disconnect();
}

}  // namespace

void scenarioFomEnumeratedValueCollision(Options const& options,
                                          rti::CallbackModel model) {
  runConflict(options, model, false);
}

void scenarioFomEnumeratedValueCollisionContract(Options const& options,
                                                  rti::CallbackModel model) {
  scenarioFomEnumeratedValueCollision(options, model);
}

void scenarioFomNonextendableVariantAlternativeConflict(
    Options const& options, rti::CallbackModel model) {
  runConflict(options, model, true);
}

void scenarioFomNonextendableVariantAlternativeConflictContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomNonextendableVariantAlternativeConflict(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
