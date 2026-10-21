#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& matchingFom(Options const& options,
                                         std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Matching data-type merge requires adapter-supplied FOM modules");
  return found->second;
}

}  // namespace

void scenarioFomMatchingDataTypes(Options const& options,
                                   rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Matching data-type merge requires adapter-supplied logical time");
  auto const& first = matchingFom(options, "matching-data-types-first");
  auto const& second = matchingFom(options, "matching-data-types-second");
  Session creator(options, model, "matching-data-types-creator");
  auto const federation = federationName(options, "fom-matching-data-types");
  creator.connect();

  // Both complete DIF modules must be accepted on their own before their
  // matching basic, simple, reference, enumerated, array, fixed-record,
  // variant, and extendable-variant declarations are composed together.
  for (auto const& modules : {
           std::vector<std::wstring>{first.wstring()},
           std::vector<std::wstring>{second.wstring()},
           std::vector<std::wstring>{first.wstring(), second.wstring()}}) {
    creator.rtiAmbassador().createFederationExecution(
        federation, modules, options.logicalTimeImplementationName);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  creator.disconnect();
}

void scenarioFomMatchingDataTypesContract(Options const& options,
                                           rti::CallbackModel model) {
  scenarioFomMatchingDataTypes(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
