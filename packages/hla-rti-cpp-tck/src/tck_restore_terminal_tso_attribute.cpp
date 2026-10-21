#include "tck_restore_terminal_tso_attribute.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestoreTerminalTsoAttribute(Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Terminal timestamped attribute restore has native evidence only for HLA_EVOKED");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Terminal attribute restore requires adapter-supplied FOM and time implementation");
  Session owner(options, model, "owner");
  auto const federation = federationName(options, "restore-terminal-tso-attribute");
  std::wstring const saveLabel = L"tck-terminal-attribute-retraction-baseline";
  std::array<std::uint8_t, 4> const payload{'T', 'A', '-', 'T'};
  rti::VariableLengthData const tag(payload.data(), payload.size());

  owner.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  auto const objectClass = owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const attribute = owner.rtiAmbassador().getAttributeHandle(
      objectClass, options.attributeName);
  require(objectClass.isValid() && attribute.isValid(),
          "Adapter FOM lacks terminal TSO object class or attribute");
  rti::AttributeHandleSet const attributes{attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  owner.rtiAmbassador().changeDefaultAttributeOrderType(
      objectClass, attributes, rti::TIMESTAMP);
  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass);
  auto const time = makeTimeContext(owner);
  auto const sendTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  owner.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitForSessions({&owner}, [&] {
    return owner.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "terminal TSO attribute time-regulation enablement");

  rti::AttributeHandleValueMap values;
  values.emplace(attribute, rti::VariableLengthData(payload.data(), payload.size()));
  auto const terminal = owner.rtiAmbassador().updateAttributeValues(
      object, values, tag, *sendTime);
  require(terminal.isValid(), "Terminal TSO update lacked a retraction handle");
  owner.rtiAmbassador().retract(terminal);
  requireException([&] { owner.rtiAmbassador().retract(terminal); },
                   L"MessageCanNoLongerBeRetracted",
                   "terminal TSO attribute retraction before save");

  owner.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions({&owner}, [&] {
    return owner.federateSaveInitiations() == std::vector<std::wstring>{saveLabel};
  }, options, "terminal TSO attribute save initiation");
  owner.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  waitForSessions({&owner}, [&] {
    return owner.federationSavedCount() == 1U;
  }, options, "terminal TSO attribute federation saved");

  auto const discarded = owner.rtiAmbassador().updateAttributeValues(
      object, values, tag, *sendTime);
  require(discarded.isValid() &&
              copyBytes(terminal.encode()) != copyBytes(discarded.encode()),
          "Post-save TSO update reused the saved terminal designator");
  owner.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions({&owner}, [&] {
    return owner.recorder().federationRestoreRequestsSucceeded().size() == 1U &&
           owner.federateRestoreInitiations().size() == 1U &&
           owner.federateRestoreInitiations().front().label == saveLabel;
  }, options, "terminal TSO attribute restore initiation");
  owner.rtiAmbassador().federateRestoreComplete();
  waitForSessions({&owner}, [&] {
    return owner.federationRestoredCount() == 1U;
  }, options, "terminal TSO attribute federation restored");
  requireException([&] { owner.rtiAmbassador().retract(terminal); },
                   L"MessageCanNoLongerBeRetracted",
                   "saved terminal TSO attribute classification");
  requireException([&] { owner.rtiAmbassador().retract(discarded); },
                   L"InvalidMessageRetractionHandle",
                   "discarded post-save TSO attribute designator");

  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
}

}  // namespace

void scenarioRestoreTerminalTsoAttribute(Options const& options, rti::CallbackModel model) {
  runRestoreTerminalTsoAttribute(options, model);
}

void scenarioRestoreTerminalTsoAttributeContract(
    Options const& options, rti::CallbackModel model) {
  runRestoreTerminalTsoAttribute(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
