#include "tck_fom_module_scenarios.hpp"

#include <cstdint>

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Dimension FOM testing requires adapter-supplied modules");
  return found->second;
}

void create(Session& creator, std::wstring const& federation,
            Options const& options, std::vector<std::wstring> const& modules) {
  creator.rtiAmbassador().createFederationExecution(
      federation, modules, options.logicalTimeImplementationName);
}

void runDimensionDuplicate(Options const& options, rti::CallbackModel model,
                           bool conflictingUpperBound) {
  require(!options.logicalTimeImplementationName.empty(),
          "Dimension duplicate requires adapter-supplied logical time");
  auto const& base = requiredFom(options, "dimension-duplicate-base");
  auto const& second = requiredFom(options, conflictingUpperBound
      ? "dimension-duplicate-conflicting-upper-bound"
      : "dimension-duplicate-equal-unique");
  Session creator(options, model, "dimension-duplicate-creator");
  Session member(options, model, "dimension-duplicate-member");
  auto const federation = federationName(options, conflictingUpperBound
      ? "fom-dimension-duplicate-conflict" : "fom-dimension-duplicate-union");
  creator.connect();
  member.connect();

  // Independently accepted modules isolate composition from schema failures.
  for (auto const& module : {base.wstring(), second.wstring()}) {
    create(creator, federation, options, {module});
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }

  if (conflictingUpperBound) {
    requireException(
        [&] { create(creator, federation, options,
                     {base.wstring(), second.wstring()}); },
        L"InconsistentFOM", "conflicting upperBound on one named dimension");
    create(creator, federation, options, {base.wstring()});
    creator.rtiAmbassador().destroyFederationExecution(federation);
  } else {
    create(creator, federation, options,
           {base.wstring(), second.wstring()});
    member.join(options.memberFederateName, options.federateType, federation);
    auto& rti = member.rtiAmbassador();
    auto const duplicate = rti.getDimensionHandle(L"TckDuplicateDimension");
    auto const unique = rti.getDimensionHandle(L"TckUniqueDimension");
    require(duplicate.isValid() && unique.isValid() && duplicate != unique &&
                rti.getDimensionName(duplicate) == L"TckDuplicateDimension" &&
                rti.getDimensionName(unique) == L"TckUniqueDimension" &&
                rti.getDimensionUpperBound(duplicate) == 100U &&
                rti.getDimensionUpperBound(unique) == 200U,
            "Equivalent duplicate or unique dimension did not survive composition");
    member.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  member.disconnect();
  creator.disconnect();
}

}  // namespace

void scenarioFomDimensionDuplicateUnion(Options const& options,
                                         rti::CallbackModel model) {
  runDimensionDuplicate(options, model, false);
}

void scenarioFomDimensionDuplicateUnionContract(Options const& options,
                                                 rti::CallbackModel model) {
  scenarioFomDimensionDuplicateUnion(options, model);
}

void scenarioFomDimensionDuplicateConflict(Options const& options,
                                            rti::CallbackModel model) {
  runDimensionDuplicate(options, model, true);
}

void scenarioFomDimensionDuplicateConflictContract(Options const& options,
                                                    rti::CallbackModel model) {
  scenarioFomDimensionDuplicateConflict(options, model);
}

void scenarioFomDimensionInputType(Options const& options,
                                   rti::CallbackModel model) {
  require(!options.mimFom.empty() &&
              !options.logicalTimeImplementationName.empty(),
          "Dimension input-type testing requires adapter-supplied MIM and logical time");
  auto const& rawBasic = requiredFom(options, "dimension-input-raw-basic");
  auto const& na = requiredFom(options, "dimension-input-na");
  auto const& tableType = requiredFom(options, "dimension-input-table-type");
  Session creator(options, model, "dimension-input-type-creator");
  Session member(options, model, "dimension-input-type-member");
  auto const federation = federationName(options, "fom-dimension-input-type");
  creator.connect();
  member.connect();
  auto createWithMim = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecutionWithMIM(
        federation, std::vector<std::wstring>{module.wstring()},
        options.mimFom.wstring(), options.logicalTimeImplementationName);
  };
  requireException([&] { createWithMim(rawBasic); }, L"InconsistentFOM",
                   "a raw basic representation used as a dimension input type");

  auto const checkAccepted = [&](std::filesystem::path const& module,
                                 std::wstring const& dimensionName) {
    // Reusing the rejected federation name also checks failed-create cleanup.
    createWithMim(module);
    member.join(options.memberFederateName, options.federateType, federation);
    auto& rti = member.rtiAmbassador();
    auto const dimension = rti.getDimensionHandle(dimensionName);
    require(dimension.isValid() &&
                rti.getDimensionName(dimension) == dimensionName &&
                rti.getDimensionUpperBound(dimension) == 100U,
            "an accepted dimension input type was not publicly queryable");
    member.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  };
  checkAccepted(na, L"TckNoTypeDimensionInput");
  checkAccepted(tableType, L"TckTableDimensionInput");
  member.disconnect();
  creator.disconnect();
}

void scenarioFomDimensionInputTypeContract(Options const& options,
                                           rti::CallbackModel model) {
  scenarioFomDimensionInputType(options, model);
}

void scenarioFomDimensionInputDescription(Options const& options,
                                          rti::CallbackModel model) {
  require(!options.mimFom.empty() &&
              !options.logicalTimeImplementationName.empty(),
          "Dimension input-description testing requires adapter-supplied MIM and logical time");
  auto const& missing = requiredFom(options, "dimension-input-description-na-invalid");
  auto const& described = requiredFom(options, "dimension-input-description-text");
  auto const& namedNa = requiredFom(options, "dimension-input-description-named-na");
  auto const& namedText = requiredFom(options, "dimension-input-description-named-text");
  Session creator(options, model, "dimension-input-description-creator");
  Session member(options, model, "dimension-input-description-member");
  auto const federation = federationName(options, "fom-dimension-input-description");
  creator.connect();
  member.connect();
  auto const createWithMim = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecutionWithMIM(
        federation, std::vector<std::wstring>{module.wstring()},
        options.mimFom.wstring(), options.logicalTimeImplementationName);
  };
  requireException([&] { createWithMim(missing); }, L"InconsistentFOM",
                   "a dimension with no named input type and NA description");

  auto const checkAccepted = [&](std::filesystem::path const& module,
                                 std::wstring const& dimensionName) {
    createWithMim(module);
    member.join(options.memberFederateName, options.federateType, federation);
    auto& rti = member.rtiAmbassador();
    auto const dimension = rti.getDimensionHandle(dimensionName);
    require(dimension.isValid() &&
                rti.getDimensionName(dimension) == dimensionName &&
                rti.getDimensionUpperBound(dimension) == 100U,
            "an accepted dimension input-description path was not publicly queryable");
    member.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  };
  checkAccepted(described, L"TckDescribedInputWithoutNamedType");
  checkAccepted(namedNa, L"TckNamedInputNaDescription");
  checkAccepted(namedText, L"TckTableDimensionInput");
  member.disconnect();
  creator.disconnect();
}

void scenarioFomDimensionInputDescriptionContract(Options const& options,
                                                  rti::CallbackModel model) {
  scenarioFomDimensionInputDescription(options, model);
}

void scenarioFomDimensionInputNaExclusivity(Options const& options,
                                            rti::CallbackModel model) {
  require(!options.mimFom.empty() &&
              !options.logicalTimeImplementationName.empty(),
          "Dimension NA exclusivity requires adapter-supplied MIM and logical time");
  auto const& mixed = requiredFom(options, "dimension-input-mixed-na");
  auto const& explicitNa = requiredFom(options, "dimension-input-na");
  auto const& emptyList = requiredFom(options, "dimension-input-description-text");
  auto const& namedType = requiredFom(options, "dimension-input-table-type");
  Session creator(options, model, "dimension-input-na-exclusivity-creator");
  Session member(options, model, "dimension-input-na-exclusivity-member");
  auto const federation = federationName(options, "fom-dimension-input-na-exclusivity");
  creator.connect();
  member.connect();
  auto const createWithMim = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecutionWithMIM(
        federation, std::vector<std::wstring>{module.wstring()},
        options.mimFom.wstring(), options.logicalTimeImplementationName);
  };
  requireException([&] { createWithMim(mixed); }, L"InconsistentFOM",
                   "a dimension with both NA and a named input type");

  auto const checkAccepted = [&](std::filesystem::path const& module,
                                 std::wstring const& dimensionName) {
    // Reuse the rejected name to check that failed Create left no execution.
    createWithMim(module);
    member.join(options.memberFederateName, options.federateType, federation);
    auto& rti = member.rtiAmbassador();
    auto const dimension = rti.getDimensionHandle(dimensionName);
    require(dimension.isValid() &&
                rti.getDimensionName(dimension) == dimensionName &&
                rti.getDimensionUpperBound(dimension) == 100U,
            "an accepted dimension input path was not publicly queryable");
    member.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  };
  checkAccepted(explicitNa, L"TckNoTypeDimensionInput");
  checkAccepted(emptyList, L"TckDescribedInputWithoutNamedType");
  checkAccepted(namedType, L"TckTableDimensionInput");
  member.disconnect();
  creator.disconnect();
}

void scenarioFomDimensionInputNaExclusivityContract(Options const& options,
                                                    rti::CallbackModel model) {
  scenarioFomDimensionInputNaExclusivity(options, model);
}

void scenarioFomDimensionNameUniqueness(Options const& options,
                                        rti::CallbackModel model) {
  require(!options.mimFom.empty() &&
              !options.logicalTimeImplementationName.empty(),
          "Dimension name uniqueness requires adapter-supplied MIM and logical time");
  auto const& duplicate = requiredFom(options, "dimension-duplicate-within-module");
  auto const& base = requiredFom(options, "dimension-duplicate-base");
  auto const& equivalent = requiredFom(options, "dimension-duplicate-equal-unique");
  Session creator(options, model, "dimension-name-uniqueness-creator");
  Session member(options, model, "dimension-name-uniqueness-member");
  auto const federation = federationName(options, "fom-dimension-name-uniqueness");
  creator.connect();
  member.connect();
  auto const createWithMim = [&](std::vector<std::wstring> const& modules) {
    creator.rtiAmbassador().createFederationExecutionWithMIM(
        federation, modules, options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  };
  requireException([&] { createWithMim({duplicate.wstring()}); },
                   L"InconsistentFOM", "duplicate dimension names within one module");

  auto const checkAccepted = [&](std::vector<std::wstring> const& modules,
                                 bool expectUnique) {
    // Same federation name checks failed-create cleanup and legal reuse.
    createWithMim(modules);
    member.join(options.memberFederateName, options.federateType, federation);
    auto& rti = member.rtiAmbassador();
    auto const shared = rti.getDimensionHandle(L"TckDuplicateDimension");
    require(shared.isValid() &&
                rti.getDimensionName(shared) == L"TckDuplicateDimension" &&
                rti.getDimensionUpperBound(shared) == 100U,
            "an accepted dimension was not publicly queryable");
    if (expectUnique) {
      auto const unique = rti.getDimensionHandle(L"TckUniqueDimension");
      require(unique.isValid() && unique != shared &&
                  rti.getDimensionName(unique) == L"TckUniqueDimension" &&
                  rti.getDimensionUpperBound(unique) == 200U,
              "legal cross-module dimension union was not publicly queryable");
    }
    member.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  };
  checkAccepted({base.wstring()}, false);
  checkAccepted({base.wstring(), equivalent.wstring()}, true);
  member.disconnect();
  creator.disconnect();
}

void scenarioFomDimensionNameUniquenessContract(Options const& options,
                                                rti::CallbackModel model) {
  scenarioFomDimensionNameUniqueness(options, model);
}

void scenarioFomDimensionDefaultRange(Options const& options,
                                      rti::CallbackModel model) {
  require(!options.mimFom.empty() &&
              !options.logicalTimeImplementationName.empty(),
          "Dimension default range requires adapter-supplied MIM and logical time");
  auto const& outside = requiredFom(options, "dimension-default-range-outside");
  auto const& bounded = requiredFom(options, "dimension-default-range-bounded");
  Session creator(options, model, "dimension-default-range-creator");
  Session member(options, model, "dimension-default-range-member");
  Session endpoint(options, model, "dimension-default-range-endpoint");
  auto const federation = federationName(options, "fom-dimension-default-range");
  creator.connect();
  member.connect();
  endpoint.connect();
  auto const createWithMim = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecutionWithMIM(
        federation, std::vector<std::wstring>{module.wstring()},
        options.mimFom.wstring(), options.logicalTimeImplementationName);
  };
  requireException([&] { createWithMim(outside); }, L"InconsistentFOM",
                   "a dimension default range extending beyond upperBound");

  // A valid half-open boundary must work under the same failed-create name.
  createWithMim(bounded);
  creator.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);
  endpoint.join(options.memberFederateName + L"-upper-endpoint",
                options.federateType, federation);
  auto& rti = member.rtiAmbassador();
  auto const name = std::wstring(L"TckBoundedDefaultDimension");
  auto const dimension = rti.getDimensionHandle(name);
  require(dimension.isValid() && rti.getDimensionName(dimension) == name &&
              rti.getDimensionUpperBound(dimension) == 100U,
          "a bounded dimension default range did not compose publicly");
  auto const intervalRegion =
      rti.createRegion(rti::DimensionHandleSet{dimension});
  require(intervalRegion.isValid(),
          "the FDD dimension could not be used in a region interval");
  // The smallest valid interval has a one-unit span; equality of the two
  // endpoints is invalid under the standard RangeBounds contract.
  rti.setRangeBounds(
      intervalRegion, dimension, rti::RangeBounds(0U, 1U));
  auto const minimumSpan = rti.getRangeBounds(intervalRegion, dimension);
  require(minimumSpan.getLowerBound() == 0U &&
              minimumSpan.getUpperBound() == 1U,
          "the minimum one-unit region interval did not round-trip in bound order");
  requireException(
      [&] {
        rti.setRangeBounds(
            intervalRegion, dimension, rti::RangeBounds(1U, 1U));
      },
      L"InvalidRangeBound",
      "a zero-width region interval");
  rti.setRangeBounds(
      intervalRegion, dimension, rti::RangeBounds(0U, 100U));
  auto const intervalBounds = rti.getRangeBounds(intervalRegion, dimension);
  require(intervalBounds.getLowerBound() == 0U &&
              intervalBounds.getUpperBound() == rti.getDimensionUpperBound(dimension),
          "the FDD dimension interval did not preserve its zero origin and upper bound");
  rti.commitRegionModifications(rti::RegionHandleSet{intervalRegion});
  rti.deleteRegion(intervalRegion);

  auto& producerRti = creator.rtiAmbassador();
  auto& endpointRti = endpoint.rtiAmbassador();
  auto const objectClass = producerRti.getObjectClassHandle(
      L"HLAobjectRoot.TckBoundedDefaultObject");
  auto const attribute = producerRti.getAttributeHandle(objectClass, L"Value");
  auto const producerDimension = producerRti.getDimensionHandle(name);
  auto const endpointDimension = endpointRti.getDimensionHandle(name);
  require(objectClass.isValid() && attribute.isValid() &&
              producerDimension.isValid() && endpointDimension.isValid(),
          "the bounded-default FOM did not expose its routing class, attribute, and dimension");
  rti::AttributeHandleSet const attributes{attribute};
  producerRti.publishObjectClassAttributes(objectClass, attributes);

  auto const producerRegion = producerRti.createRegion(
      rti::DimensionHandleSet{producerDimension});
  producerRti.setRangeBounds(producerRegion, producerDimension,
                             rti::RangeBounds(0U, 1U));
  producerRti.commitRegionModifications(
      rti::RegionHandleSet{producerRegion});
  auto const memberRegion = rti.createRegion(
      rti::DimensionHandleSet{dimension});
  rti.setRangeBounds(memberRegion, dimension, rti::RangeBounds(0U, 1U));
  rti.commitRegionModifications(rti::RegionHandleSet{memberRegion});
  auto const endpointRegion = endpointRti.createRegion(
      rti::DimensionHandleSet{endpointDimension});
  endpointRti.setRangeBounds(endpointRegion, endpointDimension,
                             rti::RangeBounds(1U, 2U));
  endpointRti.commitRegionModifications(
      rti::RegionHandleSet{endpointRegion});

  rti::AttributeHandleSetRegionHandleSetPairVector const producerPair{{
      attributes, rti::RegionHandleSet{producerRegion}}};
  rti::AttributeHandleSetRegionHandleSetPairVector const memberPair{{
      attributes, rti::RegionHandleSet{memberRegion}}};
  rti::AttributeHandleSetRegionHandleSetPairVector const endpointPair{{
      attributes, rti::RegionHandleSet{endpointRegion}}};
  rti.subscribeObjectClassAttributesWithRegions(objectClass, memberPair);
  endpointRti.subscribeObjectClassAttributesWithRegions(objectClass,
                                                        endpointPair);
  auto const object = producerRti.registerObjectInstanceWithRegions(
      objectClass, producerPair);
  require(object.isValid(),
          "the bounded default range did not permit a regional object registration");
  waitForSessions({&creator, &member, &endpoint}, [&] {
    return member.recorder().hasDiscovery(object);
  }, options, "discovery for the overlapping [0,1) subscription");
  require(endpoint.recorder().discoveryCount() == 0U,
          "a subscriber at [1,2) discovered an object whose region ends at 1");

  std::uint8_t const valueByte = 0x5AU;
  rti::AttributeHandleValueMap const values{{
      attribute, rti::VariableLengthData(&valueByte, sizeof(valueByte))}};
  producerRti.updateAttributeValues(object, values,
                                    rti::VariableLengthData{});
  waitForSessions({&creator, &endpoint, &member}, [&] {
    return member.recorder().reflectionCount() == 1U;
  }, options, "reflection for the overlapping [0,1) subscription");
  auto const reflections = member.recorder().reflections();
  require(reflections.size() == 1U && reflections.front().object == object &&
              reflections.front().values.count(attribute) == 1U,
          "the overlapping [0,1) subscription did not receive the source update");
  require(endpoint.recorder().discoveryCount() == 0U &&
              endpoint.recorder().reflectionCount() == 0U,
          "the half-open upper endpoint at 1 matched the adjacent [1,2) subscription");

  rti.unsubscribeObjectClassAttributesWithRegions(objectClass, memberPair);
  endpointRti.unsubscribeObjectClassAttributesWithRegions(objectClass,
                                                          endpointPair);
  producerRti.unassociateRegionsForUpdates(object, producerPair);
  rti.deleteRegion(memberRegion);
  endpointRti.deleteRegion(endpointRegion);
  producerRti.deleteRegion(producerRegion);
  producerRti.unpublishObjectClassAttributes(objectClass, attributes);
  member.resign(rti::NO_ACTION);
  endpoint.resign(rti::NO_ACTION);
  creator.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  endpoint.disconnect();
  member.disconnect();
  creator.disconnect();
}

void scenarioFomDimensionDefaultRangeContract(Options const& options,
                                              rti::CallbackModel model) {
  scenarioFomDimensionDefaultRange(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
