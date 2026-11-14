#include "tck_main_scenarios.hpp"

#include <RTI/time/HLAinteger64TimeFactory.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace hla_rti_cpp_tck::detail {
namespace {

[[maybe_unused]] constexpr char tckScenarioId[] =
    "cpp-tck.federation-supplied-custom-tradt";
constexpr wchar_t customImplementationName[] = L"TckTaggedTime";
constexpr std::uint64_t timeKindBit = std::uint64_t{1} << 62U;

class TaggedTimeProbe final : public rti::HLAinteger64Time {
 public:
  using HLAinteger64Time::HLAinteger64Time;

  [[nodiscard]] std::wstring implementationName() const override {
    return customImplementationName;
  }
};

}  // namespace

void scenarioFederationSuppliedCustomTradt(
    Options const& options,
    rti::CallbackModel callbackModel) {
  auto const fom = options.scenarioFoms.find("federation-supplied-custom-tradt");
  require(
      fom != options.scenarioFoms.end() && std::filesystem::exists(fom->second),
      "adapter must supply the custom TRADT FOM");

  Session session(options, callbackModel, "custom-tradt");
  session.connect();
  auto const federation =
      federationName(options, "federation-supplied-custom-tradt");
  session.rtiAmbassador().createFederationExecution(
      federation, fom->second.wstring(), customImplementationName);
  session.join(options.ownerFederateName, options.federateType, federation);

  auto factory = session.rtiAmbassador().getTimeFactory();
  require(factory != nullptr, "joined federate received no logical-time factory");
  require(
      factory->getName() == customImplementationName,
      "RTI did not select the federation-supplied custom logical-time factory");

  auto const integerFactory =
      dynamic_cast<rti::HLAinteger64TimeFactory*>(factory.get());
  require(
      integerFactory != nullptr,
      "custom TRADT factory does not expose its declared integer64 value type");

  auto initial = factory->makeInitial();
  require(
      initial != nullptr && initial->isInitial() &&
          initial->implementationName() == customImplementationName,
      "custom factory did not create its selected initial-time representation");

  auto logicalTime = integerFactory->makeLogicalTime(7);
  require(
      logicalTime != nullptr && logicalTime->getTime() == 7 &&
          logicalTime->implementationName() == customImplementationName &&
          (static_cast<std::uint64_t>(logicalTime->getTime()) & timeKindBit) == 0U,
      "custom factory did not produce its logical-time discriminator");

  auto const wallClockWireTime = rti::HLAinteger64Time(
      static_cast<rti::Integer64>(timeKindBit | 7U));
  auto wallClockTime = factory->decodeLogicalTime(wallClockWireTime.encode());
  auto const decodedWallClock =
      dynamic_cast<rti::HLAinteger64Time*>(wallClockTime.get());
  require(
      decodedWallClock != nullptr &&
          decodedWallClock->implementationName() == customImplementationName &&
          (static_cast<std::uint64_t>(decodedWallClock->getTime()) &
           timeKindBit) != 0U,
      "custom factory decode lost the wall-clock discriminator");
  require(
      !(*logicalTime == *wallClockTime),
      "equal payloads with distinct TRADT time kinds compared equal");

  TaggedTimeProbe currentTime;
  session.rtiAmbassador().queryLogicalTime(currentTime);
  require(
      currentTime.isInitial() &&
          currentTime.implementationName() == customImplementationName,
      "Query Logical Time did not preserve the federation-selected TRADT type");

  session.resign(rti::NO_ACTION);
  session.rtiAmbassador().destroyFederationExecution(federation);
  session.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
