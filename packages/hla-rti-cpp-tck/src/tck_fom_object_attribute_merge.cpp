#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void create(Session& session, std::wstring const& federation,
            Options const& options, std::filesystem::path const& first,
            std::filesystem::path const& second) {
  session.rtiAmbassador().createFederationExecution(
      federation, std::vector<std::wstring>{first.wstring(), second.wstring()},
      options.logicalTimeImplementationName);
}

}  // namespace

void scenarioFomObjectAttributeMergeConflict(Options const& options,
                                              rti::CallbackModel model) {
  require(!options.objectAttributeBaseFom.empty() &&
              !options.objectAttributeDisjointFom.empty() &&
              !options.objectAttributeConflictFom.empty(),
          "Annex C.3 object-attribute test requires three adapter-supplied FOM modules");
  require(!options.logicalTimeImplementationName.empty(),
          "Annex C.3 object-attribute test requires adapter-supplied logical time");
  Session creator(options, model, "fom-attribute-creator");
  Session member(options, model, "fom-attribute-member");
  auto const federation = federationName(options, "fom-object-attribute-merge");
  creator.connect();
  member.connect();

  create(creator, federation, options, options.objectAttributeBaseFom,
         options.objectAttributeDisjointFom);
  member.join(options.memberFederateName, options.federateType, federation);
  auto const objectClass = member.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.TckAttributeMergeObject");
  require(objectClass.isValid(),
          "Merged object class was not visible to a joined member");
  auto const first = member.rtiAmbassador().getAttributeHandle(
      objectClass, L"FirstValue");
  auto const second = member.rtiAmbassador().getAttributeHandle(
      objectClass, L"SecondValue");
  require(first.isValid() && second.isValid() && first != second,
          "Merged object class did not retain distinct attributes");
  member.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);

  requireException(
      [&] { create(creator, federation, options, options.objectAttributeBaseFom,
                   options.objectAttributeConflictFom); },
      L"InconsistentFOM", "merging conflicting semantics for one named attribute");
  create(creator, federation, options, options.objectAttributeBaseFom,
         options.objectAttributeDisjointFom);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  creator.disconnect();
}

void scenarioFomObjectAttributeMergeConflictContract(Options const& options,
                                                      rti::CallbackModel model) {
  scenarioFomObjectAttributeMergeConflict(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
