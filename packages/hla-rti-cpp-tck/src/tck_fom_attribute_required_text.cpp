#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Attribute-table required-text check needs an adapter-supplied FOM");
  return found->second;
}

}  // namespace

void scenarioFomAttributeTableNonblankSemantics(Options const& options,
                                                rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Attribute-table required-text check needs adapter-supplied logical time");
  auto const& invalid = requiredFom(options, "attribute-table-empty-semantics");
  Session creator(options, model, "attribute-required-text-creator");
  auto const federation = federationName(options, "fom-attribute-required-text");
  creator.connect();
  bool rejectedAsInconsistentFom = false;
  bool federationWasCreated = false;
  try {
    creator.rtiAmbassador().createFederationExecution(
        federation, std::vector<std::wstring>{invalid.wstring()},
        options.logicalTimeImplementationName);
    federationWasCreated = true;
  } catch (rti::InconsistentFOM const&) {
    rejectedAsInconsistentFom = true;
  }
  if (federationWasCreated) {
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  creator.disconnect();
  require(rejectedAsInconsistentFom,
          "an attribute table row with a blank Semantics field was not rejected as InconsistentFOM");
}

void scenarioFomAttributeTableNonblankSemanticsContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomAttributeTableNonblankSemantics(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
