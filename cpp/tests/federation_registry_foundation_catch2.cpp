#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE("The embedded federation registry preserves a prevalidated definition", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;

  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(registry.contains(L"exercise"));

  auto definition = registry.definitionFor(L"exercise");
  REQUIRE(definition.has_value());
  REQUIRE(definition->logicalTimeImplementationName == L"HLAinteger64Time");
  REQUIRE(definition->fomModules.size() == 2);
  REQUIRE(definition->fomModules.front().designator == L"file:///fom/base.xml");
  REQUIRE(definition->fomModules.front().schemaDesignator == L"IEEE1516-DIF-2025.xsd");
}

TEST_CASE(
    "The embedded federation registry exposes internal operation timing",
    "[unit][kernel][federation-registry][instrumentation][foundation][federation-management]") {
  auto instrumentation = std::make_shared<umbra::detail::RuntimeInstrumentation>();
  EmbeddedFederationRegistry registry(instrumentation);

  REQUIRE(registry.create(L"instrumented", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto const snapshot = registry.runtimeInstrumentationSnapshotForTesting();
  auto const found = std::find_if(
      snapshot.operations.begin(),
      snapshot.operations.end(),
      [](umbra::detail::InstrumentationOperationSnapshot const& operation) {
        return operation.layer == InstrumentationLayer::federation_registry &&
            operation.name == "create";
      });
  REQUIRE(found != snapshot.operations.end());
  REQUIRE(found->calls == 1);
  REQUIRE(found->totalDurationNanoseconds > 0);
}

TEST_CASE("The embedded federation registry rejects missing definitions without imposing name policy", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  FederationDefinition noModules;
  FederationDefinition repeatedDesignators{
      {
          module(L"file:///fom/base.xml", L"C:/fom/base.xml"),
          module(L"file:///fom/base.xml", L"C:/fom/base.xml"),
      },
      L"",
  };

  REQUIRE(registry.create(L"", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(registry.create(L"no-modules", noModules).status == FederationRegistryStatus::invalid_request);
  REQUIRE(
      registry.create(L"repeated-module-designators", repeatedDesignators).status ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(
      registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::federation_already_exists);

  auto explicitEmptyName = registry.join(L"exercise", L"", L"");
  REQUIRE(explicitEmptyName.status == FederationRegistryStatus::applied);
  REQUIRE(explicitEmptyName.membership.has_value());
  REQUIRE(explicitEmptyName.membership->name.empty());
  REQUIRE(explicitEmptyName.membership->type.empty());
}

TEST_CASE("The embedded federation registry maintains active membership and destroy invariants", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto named = registry.join(L"exercise", L"trainer", L"alice");
  REQUIRE(named.status == FederationRegistryStatus::applied);
  REQUIRE(named.membership.has_value());
  REQUIRE(named.membership->name == L"alice");
  REQUIRE(named.membership->id != 0);

  auto generated = registry.join(L"exercise", L"observer");
  REQUIRE(generated.status == FederationRegistryStatus::applied);
  REQUIRE(generated.membership.has_value());
  REQUIRE(generated.membership->name == L"federate-2");
  REQUIRE(generated.membership->id != named.membership->id);
  REQUIRE(registry.memberCount(L"exercise") == 2);

  auto namedByName = registry.memberByName(L"exercise", L"alice");
  REQUIRE(namedByName.has_value());
  REQUIRE(namedByName->id == named.membership->id);
  auto generatedById = registry.memberById(L"exercise", generated.membership->id);
  REQUIRE(generatedById.has_value());
  REQUIRE(generatedById->name == generated.membership->name);
  REQUIRE_FALSE(registry.memberByName(L"exercise", L"missing").has_value());
  REQUIRE_FALSE(registry.memberById(L"missing", named.membership->id).has_value());

  REQUIRE(
      registry.join(L"exercise", L"trainer", L"alice").status ==
      FederationRegistryStatus::federate_name_already_in_use);
  REQUIRE(
      registry.destroy(L"exercise").status == FederationRegistryStatus::federates_currently_joined);

  REQUIRE(
      registry.resign(L"exercise", named.membership->id).status == FederationRegistryStatus::applied);
  REQUIRE_FALSE(registry.memberByName(L"exercise", L"alice").has_value());
  REQUIRE_FALSE(registry.memberById(L"exercise", named.membership->id).has_value());
  REQUIRE(
      registry.resign(L"exercise", generated.membership->id).status == FederationRegistryStatus::applied);
  REQUIRE(registry.memberCount(L"exercise") == 0);
  REQUIRE(registry.destroy(L"exercise").status == FederationRegistryStatus::applied);
  REQUIRE_FALSE(registry.contains(L"exercise"));
}

TEST_CASE("The embedded federation registry makes generated names unique despite user lookalikes", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto lookalike = registry.join(L"exercise", L"trainer", L"federate-1");
  REQUIRE(lookalike.status == FederationRegistryStatus::applied);
  auto generated = registry.join(L"exercise", L"observer");
  REQUIRE(generated.status == FederationRegistryStatus::applied);
  REQUIRE(generated.membership.has_value());
  REQUIRE(generated.membership->name == L"federate-2");
  REQUIRE(generated.membership->id == 2);
}

TEST_CASE("The embedded federation registry commits an additional-module definition with membership", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  auto original = validDefinition();
  auto replacement = validDefinition();
  replacement.logicalTimeImplementationName = L"HLAfloat64Time";

  REQUIRE(registry.create(L"exercise", original).status == FederationRegistryStatus::applied);
  auto joined = registry.joinWithDefinition(L"exercise", replacement, L"observer", L"bob");
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership.has_value());
  REQUIRE(registry.memberCount(L"exercise") == 1);
  auto afterJoin = registry.definitionFor(L"exercise");
  REQUIRE(afterJoin.has_value());
  REQUIRE(afterJoin->logicalTimeImplementationName == L"HLAfloat64Time");

  auto incompatibleReplacement = validDefinition();
  incompatibleReplacement.logicalTimeImplementationName = L"HLAinteger64Time";
  auto rejected = registry.joinWithDefinition(
      L"exercise",
      incompatibleReplacement,
      L"observer",
      L"carol");
  REQUIRE(rejected.status == FederationRegistryStatus::invalid_request);
  REQUIRE(registry.memberCount(L"exercise") == 1);
  auto afterRejectedJoin = registry.definitionFor(L"exercise");
  REQUIRE(afterRejectedJoin.has_value());
  REQUIRE(afterRejectedJoin->logicalTimeImplementationName == L"HLAfloat64Time");

  auto duplicate = registry.joinWithDefinition(
      L"exercise",
      replacement,
      L"observer",
      L"bob");
  REQUIRE(duplicate.status == FederationRegistryStatus::federate_name_already_in_use);
}

TEST_CASE("The embedded federation registry reports missing federation and membership distinctly", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;

  REQUIRE(
      registry.join(L"missing", L"trainer").status ==
      FederationRegistryStatus::federation_does_not_exist);
  REQUIRE(
      registry.destroy(L"missing").status == FederationRegistryStatus::federation_does_not_exist);

  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(
      registry.resign(L"exercise", 42).status == FederationRegistryStatus::federate_not_member);
}
