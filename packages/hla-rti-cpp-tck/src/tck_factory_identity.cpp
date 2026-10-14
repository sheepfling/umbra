#include "tck_factory_identity.hpp"

namespace hla_rti_cpp_tck::detail {

void verifyStandardRTIambassadorFactoryContract() {
  rti::RTIambassadorFactory firstFactory;
  auto firstAmbassador = firstFactory.createRTIambassador();
  require(
      static_cast<bool>(firstAmbassador),
      "RTIambassadorFactory did not create a standard RTIambassador");

  rti::RTIambassadorFactory secondFactory;
  auto secondAmbassador = secondFactory.createRTIambassador();
  require(
      static_cast<bool>(secondAmbassador),
      "RTIambassadorFactory was not reusable for a second standard RTIambassador");
}


void verifyStandardRuntimeIdentityContract() {
  auto const name = rti::rtiName();
  auto const version = rti::rtiVersion();
  require(
      !name.empty() && !version.empty(),
      "standard RTI runtime identity returned an empty name or version");
  require(
      name == rti::rtiName() && version == rti::rtiVersion(),
      "standard RTI runtime identity was not stable within the process");
}


}  // namespace hla_rti_cpp_tck::detail
