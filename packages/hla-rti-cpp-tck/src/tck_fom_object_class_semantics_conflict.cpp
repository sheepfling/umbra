#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& conflictFom(Options const& options) {
  auto const found = options.scenarioFoms.find("object-class-semantics-conflict");
  require(found != options.scenarioFoms.end(),
          "Object-class semantics conflict requires an adapter-supplied FOM");
  return found->second;
}

void create(Session& session, std::wstring const& federation,
            Options const& options, std::filesystem::path const& first,
            std::filesystem::path const& second) {
  session.rtiAmbassador().createFederationExecution(
      federation, std::vector<std::wstring>{first.wstring(), second.wstring()},
      options.logicalTimeImplementationName);
}

}  // namespace

void scenarioFomObjectClassSemanticsConflict(Options const& options,
                                              rti::CallbackModel model) {
  require(!options.objectAttributeBaseFom.empty() &&
              !options.objectAttributeDisjointFom.empty(),
          "Object-class semantics conflict requires two adapter-supplied FOMs");
  require(!options.logicalTimeImplementationName.empty(),
          "Object-class semantics conflict requires adapter-supplied logical time");
  auto const& conflict = conflictFom(options);
  Session creator(options, model, "class-semantics-creator");
  Session member(options, model, "class-semantics-member");
  auto const federation = federationName(options, "fom-class-semantics-conflict");
  creator.connect();
  member.connect();

  requireException(
      [&] { create(creator, federation, options, options.objectAttributeBaseFom,
                   conflict); },
      L"InconsistentFOM", "conflicting semantics for one named object class");
  create(creator, federation, options, options.objectAttributeBaseFom,
         options.objectAttributeDisjointFom);
  member.join(options.memberFederateName, options.federateType, federation);
  auto const objectClass = member.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckAttributeMergeObject");
  require(objectClass.isValid(),
          "Valid retry did not expose the named object class to a member");
  member.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  creator.disconnect();
}

void scenarioFomObjectClassSemanticsConflictContract(Options const& options,
                                                      rti::CallbackModel model) {
  scenarioFomObjectClassSemanticsConflict(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
