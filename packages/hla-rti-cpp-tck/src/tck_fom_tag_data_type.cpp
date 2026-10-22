#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Tag data-type category requires an adapter-supplied FOM");
  return found->second;
}

}  // namespace

void scenarioFomTagDataTypeCategories(Options const& options,
                                      rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Tag data-type category requires adapter-supplied logical time");
  auto const& valid = requiredFom(options, "tag-data-type-category-valid");
  Session creator(options, model, "tag-data-type-creator");
  auto const federation = federationName(options, "fom-tag-data-type-categories");
  creator.connect();
  auto const create = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecution(
        federation, std::vector<std::wstring>{module.wstring()},
        options.logicalTimeImplementationName);
  };
  create(valid);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  for (auto const& key : {"tag-data-type-basic-user",
                          "tag-data-type-basic-synchronization"}) {
    auto const& invalid = requiredFom(options, key);
    requireException([&] { create(invalid); }, L"InconsistentFOM", key);
    create(valid);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  creator.disconnect();
}

void scenarioFomTagDataTypeCategoriesContract(Options const& options,
                                              rti::CallbackModel model) {
  scenarioFomTagDataTypeCategories(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
