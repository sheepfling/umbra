#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  if (found == options.scenarioFoms.end()) {
    auto message = "Time representation requires adapter-supplied FOM module '" +
                   key + "'";
    if (!options.scenarioFoms.empty()) {
      message += "; available bindings:";
      for (auto const& [name, path] : options.scenarioFoms) {
        static_cast<void>(path);
        message += " " + name;
      }
    } else {
      message += "; no scenario FOM bindings were supplied";
    }
    require(false, message);
  }
  return found->second;
}

void requireLogicalTimeEncodingVector(
    rti::LogicalTimeFactory& factory,
    rti::LogicalTime const& value,
    std::vector<std::uint8_t> const& expected,
    std::string const& description) {
  requireDirectTimeEncoding(value, description);
  require(encodeTime(value) == expected,
          description + " did not use its standardized byte representation");
  auto const decoded = decodeTime(factory, expected);
  require(decoded != nullptr,
          description + " factory could not decode the standardized bytes");
  requireTimeEquals(*decoded, value,
                    description + " factory decoded the wrong logical time");
}

void requireLogicalTimeIntervalEncodingVector(
    rti::LogicalTimeFactory& factory,
    rti::LogicalTimeInterval const& value,
    std::vector<std::uint8_t> const& expected,
    std::string const& description) {
  require(copyBytes(value.encode()) == expected,
          description + " did not use its standardized byte representation");
  auto const decoded = factory.decodeLogicalTimeInterval(
      expected.data(), expected.size());
  require(decoded != nullptr,
          description + " factory could not decode the standardized bytes");
  require(*decoded == value,
          description + " factory decoded the wrong logical-time interval");
}

void verifyStandardTimeEncodingVectors(rti::LogicalTimeFactory& factory,
                                       std::string const& federateRole) {
  if (factory.getName() == L"HLAinteger64Time") {
    constexpr rti::Integer64 value = 0x0102030405060708LL;
    auto const expected = std::vector<std::uint8_t>{
        0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U};
    rti::HLAinteger64Time logicalTime(value);
    rti::HLAinteger64Interval lookahead(value);
    requireLogicalTimeEncodingVector(
        factory, logicalTime, expected,
        federateRole + " HLAinteger64Time");
    requireLogicalTimeIntervalEncodingVector(
        factory, lookahead, expected,
        federateRole + " HLAinteger64Time lookahead");
    return;
  }

  if (factory.getName() == L"HLAfloat64Time") {
    auto const expected = std::vector<std::uint8_t>{
        0x3fU, 0xf0U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
    rti::HLAfloat64Time logicalTime(1.0);
    rti::HLAfloat64Interval lookahead(1.0);
    requireLogicalTimeEncodingVector(
        factory, logicalTime, expected,
        federateRole + " HLAfloat64Time");
    requireLogicalTimeIntervalEncodingVector(
        factory, lookahead, expected,
        federateRole + " HLAfloat64Time lookahead");
    return;
  }

  require(false, federateRole + " received an unsupported time representation");
}

void runTimeRepresentation(Options const& options, rti::CallbackModel model,
                           bool rejectInvalid) {
  require(!options.logicalTimeImplementationName.empty(),
          "Time representation requires adapter-supplied logical time");
  auto const& valid = requiredFom(options, "time-representation-valid");
  Session creator(options, model, "time-representation-creator");
  auto const federation = federationName(options, rejectInvalid
      ? "fom-time-representation-rejections"
      : "fom-time-representation-valid");
  creator.connect();
  auto const create = [&](std::filesystem::path const& module,
                          std::wstring const& implementationName) {
    creator.rtiAmbassador().createFederationExecution(
        federation, std::vector<std::wstring>{module.wstring()},
        implementationName);
  };
  if (!rejectInvalid) {
    auto const verify = [&](std::filesystem::path const& module,
                            std::wstring const& implementationName) {
      create(module, implementationName);
      creator.join(options.ownerFederateName, options.federateType, federation);
      auto const time = makeTimeContext(creator);
      require(time.factory->getName() == implementationName,
              "Joined factory did not preserve the representation selected at Create");
      verifyStandardTimeEncodingVectors(*time.factory, "Creator");

      Session member(options, model, "member");
      member.connect();
      member.join(options.memberFederateName, options.federateType, federation);
      auto const memberTime = makeTimeContext(member);
      require(memberTime.factory->getName() == implementationName,
              "Joining federate did not receive the federation logical-time representation");
      verifyStandardTimeEncodingVectors(*memberTime.factory, "Joining federate");

      auto const oneStep = timeAfter(
          *time.factory, *time.initial, *time.epsilon, 1U);
      auto const memberOneStep = timeAfter(
          *memberTime.factory,
          *memberTime.initial,
          *memberTime.epsilon,
          1U);
      auto const encodedOneStep = encodeTime(*oneStep);
      auto const encodedMemberOneStep = encodeTime(*memberOneStep);
      require(encodedMemberOneStep == encodedOneStep,
              "Joined federates did not produce the same logical-time encoding");
      auto const decodedByMember = decodeTime(*memberTime.factory, encodedOneStep);
      require(decodedByMember != nullptr,
              "Joining federate could not decode the creator's logical time");
      requireTimeEquals(
          *decodedByMember,
          *memberOneStep,
          "Joining federate decoded the creator's logical time differently");
      auto const decodedByCreator =
          decodeTime(*time.factory, encodedMemberOneStep);
      require(decodedByCreator != nullptr,
              "Creator could not decode the joining federate's logical time");
      requireTimeEquals(
          *decodedByCreator,
          *oneStep,
          "Creator decoded the joining federate's logical time differently");
      require(!oneStep->isInitial(),
              "Standard logical-time representation did not advance by epsilon");
      auto const decodedTime = decodeTime(*time.factory, encodedOneStep);
      require(decodedTime != nullptr,
              "Standard logical-time representation could not decode its encoded time");
      requireTimeEquals(
          *decodedTime, *oneStep,
          "Standard logical-time representation time round trip");

      auto measuredStep = time.factory->makeZero();
      require(measuredStep != nullptr,
              "Standard logical-time representation returned no working interval");
      measuredStep->setToDifference(*oneStep, *time.initial);
      require(*measuredStep == *time.epsilon,
              "Standard logical-time representation computed the wrong interval difference");
      auto const encodedEpsilon = copyBytes(time.epsilon->encode());
      auto const decodedEpsilon = time.factory->decodeLogicalTimeInterval(
          encodedEpsilon.data(), encodedEpsilon.size());
      require(decodedEpsilon != nullptr,
              "Standard logical-time representation could not decode its interval");
      require(*decodedEpsilon == *time.epsilon,
              "Standard logical-time representation interval round trip changed epsilon");

      member.resign(rti::NO_ACTION);
      member.disconnect();
      creator.resign(rti::NO_ACTION);
      creator.rtiAmbassador().destroyFederationExecution(federation);
    };

    verify(valid, L"HLAinteger64Time");
    verify(
        requiredFom(options, "time-representation-valid-float64"),
        L"HLAfloat64Time");
  } else {
    create(valid, options.logicalTimeImplementationName);
    creator.rtiAmbassador().destroyFederationExecution(federation);
    for (auto const& key : {"time-representation-basic-data",
                            "time-representation-reference-data"}) {
      auto const& invalid = requiredFom(options, key);
      requireException(
          [&] { create(invalid, options.logicalTimeImplementationName); },
          L"InconsistentFOM",
          key);
      // A rejected Create must not reserve the execution name.
      create(valid, options.logicalTimeImplementationName);
      creator.rtiAmbassador().destroyFederationExecution(federation);
    }
  }
  creator.disconnect();
}

}  // namespace

void scenarioFomTimeRepresentationValid(Options const& options,
                                        rti::CallbackModel model) {
  runTimeRepresentation(options, model, false);
}

void scenarioFomTimeRepresentationValidContract(Options const& options,
                                                rti::CallbackModel model) {
  scenarioFomTimeRepresentationValid(options, model);
}

void scenarioFomTimeRepresentationRejections(Options const& options,
                                             rti::CallbackModel model) {
  runTimeRepresentation(options, model, true);
}

void scenarioFomTimeRepresentationRejectionsContract(Options const& options,
                                                     rti::CallbackModel model) {
  scenarioFomTimeRepresentationRejections(options, model);
}

void scenarioFomDefaultLogicalTimeSelection(Options const& options,
                                            rti::CallbackModel model) {
  auto const& unconstrained = requiredFom(options, "default-logical-time");
  auto const& integerTime = requiredFom(options, "time-representation-valid");
  Session member(options, model, "fom-default-logical-time-member");
  auto const federation = federationName(options, "fom-default-logical-time");
  member.connect();

  // Omitting the time name selects the standard default, which conflicts with
  // an FDD that explicitly documents HLAinteger64Time.
  requireException(
      [&] {
        member.rtiAmbassador().createFederationExecution(
            federation, integerTime.wstring());
      },
      L"InconsistentFOM",
      "default logical time against an integer64 FDD");

  // A rejected Create must not reserve the name. The adapter supplies a FOM
  // without a declared time representation for the positive default path.
  member.rtiAmbassador().createFederationExecution(
      federation, unconstrained.wstring());
  member.join(options.ownerFederateName, options.federateType, federation);
  auto factory = member.rtiAmbassador().getTimeFactory();
  require(factory != nullptr && factory->getName() == L"HLAfloat64Time",
          "Create without a time name did not select HLAfloat64Time");
  member.resign(rti::NO_ACTION);
  member.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
}

void scenarioFomDefaultLogicalTimeSelectionContract(Options const& options,
                                                    rti::CallbackModel model) {
  scenarioFomDefaultLogicalTimeSelection(options, model);
}

void scenarioFomUnavailableLogicalTimeFactory(Options const& options,
                                              rti::CallbackModel model) {
  auto const& unconstrained = requiredFom(options, "default-logical-time");
  require(!options.unsupportedLogicalTimeImplementationName.empty(),
          "Unavailable-factory testing requires an adapter-declared unsupported name");
  Session creator(options, model, "fom-unavailable-time-factory-creator");
  auto const federation = federationName(options, "fom-unavailable-time-factory");
  creator.connect();
  requireException(
      [&] {
        creator.rtiAmbassador().createFederationExecution(
            federation, unconstrained.wstring(),
            options.unsupportedLogicalTimeImplementationName);
      },
      L"CouldNotCreateLogicalTimeFactory",
      "Create with an adapter-declared unavailable logical-time factory");
  creator.rtiAmbassador().createFederationExecution(
      federation, unconstrained.wstring());
  creator.rtiAmbassador().destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomUnavailableLogicalTimeFactoryContract(Options const& options,
                                                      rti::CallbackModel model) {
  scenarioFomUnavailableLogicalTimeFactory(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
