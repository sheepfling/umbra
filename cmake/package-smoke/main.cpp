#include <RTI/RTI1516.h>
#include <RTI/encoding/EncodingConfig.h>

#include <array>
#include <cstring>
#include <memory>

int main() {
  using rti1516_2025::Octet;
  using rti1516_2025::VariableLengthData;

  // This executable is configured against the installed package, rather than
  // Umbra's source tree.  Exercise the official support types a federate
  // needs before it can call any service with tags or encoded values.
  std::array<Octet, 4> callerBytes{'H', 'L', 'A', '!'};
  std::array<Octet, 4> const expectedBytes{'H', 'L', 'A', '!'};
  VariableLengthData copiedFromCaller(callerBytes.data(), callerBytes.size());
  callerBytes.front() = 'X';
  if (copiedFromCaller.size() != expectedBytes.size() ||
      std::memcmp(copiedFromCaller.data(), expectedBytes.data(), expectedBytes.size()) != 0) {
    return 1;
  }

  std::array<Octet, 3> borrowedBytes{'o', 'l', 'd'};
  VariableLengthData borrowed;
  borrowed.setDataPointer(borrowedBytes.data(), borrowedBytes.size());
  VariableLengthData materialized(borrowed);
  borrowedBytes[0] = 'n';
  if (static_cast<Octet const*>(borrowed.data())[0] != 'n' ||
      static_cast<Octet const*>(materialized.data())[0] != 'o') {
    return 2;
  }

  rti1516_2025::FederateHandle invalidHandle;
  if (invalidHandle.isValid()) {
    return 3;
  }

  auto configuration = rti1516_2025::RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"package-smoke")
                           .withRtiAddress(L"in-process")
                           .withAdditionalSettings(L"none");
  if (configuration.configurationName() != L"package-smoke" ||
      configuration.rtiAddress() != L"in-process" ||
      configuration.additionalSettings() != L"none") {
    return 4;
  }

  rti1516_2025::RTIambassadorFactory ambassadorFactory;
  std::unique_ptr<rti1516_2025::RTIambassador> ambassador = ambassadorFactory.createRTIambassador();
  if (!ambassador || rti1516_2025::rtiName().empty() || rti1516_2025::rtiVersion().empty()) {
    return 5;
  }

  auto defaultFactory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(L"");
  if (!defaultFactory || defaultFactory->getName() != L"HLAfloat64Time") {
    return 6;
  }

  auto fedtimeFactory = rti1516_2025::LogicalTimeFactoryFactory::makeLogicalTimeFactory(
      L"HLAinteger64Time");
  if (!fedtimeFactory || fedtimeFactory->getName() != L"HLAinteger64Time") {
    return 7;
  }

  auto initialTime = defaultFactory->makeInitial();
  return initialTime && initialTime->implementationName() == L"HLAfloat64Time" ? 0 : 8;
}
