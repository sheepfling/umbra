#include <RTI/Enums.h>
#include <RTI/Exception.h>
#include <RTI/FederateAmbassador.h>
#include <RTI/Handle.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/RTIambassador.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/RangeBounds.h>
#include <RTI/RtiConfiguration.h>
#include <RTI/Typedefs.h>
#include <RTI/VariableLengthData.h>
#include <RTI/auth/AuthorizationResult.h>
#include <RTI/auth/Authorizer.h>
#include <RTI/auth/AuthorizerFactory.h>
#include <RTI/auth/Credentials.h>
#include <RTI/auth/HLAauthorizerFactoryFactory.h>
#include <RTI/auth/HLAnoCredentials.h>
#include <RTI/auth/HLAplainTextPassword.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAextendableVariantRecord.h>
#include <RTI/encoding/HLAfixedArray.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAlogicalTime.h>
#include <RTI/encoding/HLAlogicalTimeInterval.h>
#include <RTI/encoding/HLAopaqueData.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/encoding/HLAvariantRecord.h>
#include <RTI/libauth/AuthorizerFactoryFactory.h>
#include <RTI/libfedtime/LogicalTimeFactoryFactory.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64TimeFactory.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>
#include <RTI/time/LogicalTime.h>
#include <RTI/time/LogicalTimeFactory.h>
#include <RTI/time/LogicalTimeInterval.h>

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <type_traits>

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

#if defined(HLA_TCK_PACKAGE_RTI_LIBRARY_PATH)
namespace {

struct SharedLibrary final {
  std::string_view base_name;
  std::filesystem::path path;
};

bool hasHlaSharedLibraryName(std::string_view filename,
                             std::string_view expected_prefix) {
#if defined(_WIN32)
  return filename == std::string(expected_prefix) + ".dll";
#elif defined(__APPLE__)
  std::string const exact = std::string(expected_prefix) + ".dylib";
  std::string const versioned_prefix = std::string(expected_prefix) + ".";
  return filename == exact ||
         (filename.starts_with(versioned_prefix) && filename.ends_with(".dylib"));
#else
  std::string const exact = std::string(expected_prefix) + ".so";
  std::string const versioned_prefix = exact + ".";
  return filename == exact || filename.starts_with(versioned_prefix);
#endif
}

void requireSharedLibraryNames() {
  std::string expected_suffix{"1516_2023"};
  expected_suffix += HLA_TCK_PACKAGE_COMPILER_MARKER;
  expected_suffix += HLA_TCK_PACKAGE_32BIT_MARKER;
#if HLA_TCK_PACKAGE_DEBUG_BUILD
  expected_suffix += 'd';
#endif

  std::array<SharedLibrary, 3> const libraries{{
      {"rti", HLA_TCK_PACKAGE_RTI_LIBRARY_PATH},
      {"fedtime", HLA_TCK_PACKAGE_FEDTIME_LIBRARY_PATH},
      {"authorizer", HLA_TCK_PACKAGE_AUTHORIZER_LIBRARY_PATH},
  }};

  for (SharedLibrary const& library : libraries) {
    require(
        std::filesystem::exists(library.path),
        "the installed package is missing its " + std::string(library.base_name) +
            " shared library");

    std::string expected_name{"lib"};
    expected_name += library.base_name;
    expected_name += expected_suffix;
    require(
        hasHlaSharedLibraryName(library.path.filename().string(), expected_name),
        "the installed " + std::string(library.base_name) +
            " shared-library filename does not match SISO-REF-080-2023");
  }
}

}  // namespace
#endif

void scenarioLogicalTimeHeaderLayoutContract(
    Options const&,
    rti::CallbackModel) {
  static_assert(std::is_class_v<rti1516_2025::RTIambassador>);
  static_assert(std::is_class_v<rti1516_2025::RTIambassadorFactory>);
  static_assert(std::is_class_v<rti1516_2025::LogicalTime>);
  static_assert(std::is_class_v<rti1516_2025::LogicalTimeInterval>);
  static_assert(std::is_class_v<rti1516_2025::LogicalTimeFactory>);
  static_assert(std::is_class_v<rti1516_2025::HLAinteger64Time>);
  static_assert(std::is_class_v<rti1516_2025::HLAinteger64Interval>);
  static_assert(std::is_class_v<rti1516_2025::HLAinteger64TimeFactory>);
  static_assert(std::is_class_v<rti1516_2025::HLAfloat64Time>);
  static_assert(std::is_class_v<rti1516_2025::HLAfloat64Interval>);
  static_assert(std::is_class_v<rti1516_2025::HLAfloat64TimeFactory>);

  rti1516_2025::RTIambassadorFactory factory;
  auto ambassador = factory.createRTIambassador();
  require(
      static_cast<bool>(ambassador),
      "the dynamically linked standard factory returned no RTIambassador");

#if defined(HLA_TCK_PACKAGE_RTI_LIBRARY_PATH)
  requireSharedLibraryNames();
#endif
}

}  // namespace hla_rti_cpp_tck::detail
