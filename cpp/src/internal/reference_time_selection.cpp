#include "internal/reference_time_selection.hpp"

#include "internal/fom_catalog.hpp"

#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <array>
#include <string_view>

namespace umbra::detail {
namespace {

constexpr std::wstring_view kInteger64TimeName = L"HLAinteger64Time";
constexpr std::wstring_view kFloat64TimeName = L"HLAfloat64Time";

std::string_view documentedTimeDataType(std::wstring_view implementationName) {
  if (implementationName == kInteger64TimeName) {
    return "HLAinteger64Time";
  }
  if (implementationName == kFloat64TimeName) {
    return "HLAfloat64Time";
  }
  return {};
}

bool isUnspecified(std::string const& dataType) {
  // IEEE 1516.2 permits NA when timestamp or lookahead information is not
  // appropriate. An omitted FDD entry carries the same no-constraint meaning
  // for the relaxed composition artifact.
  return dataType.empty() || dataType == "NA";
}

std::string describeMismatch(
    std::string_view column,
    std::string const& actual,
    std::string_view expected) {
  return "The composed FDD documents " + std::string(column) + " data type '" + actual +
         "', which is incompatible with Umbra's selected reference implementation '" +
         std::string(expected) + "'.";
}

}  // namespace

ReferenceLogicalTimeSelection ReferenceLogicalTimeSelector::select(
    FomCatalog const& catalog,
    std::wstring const& requestedImplementationName) const {
  ReferenceLogicalTimeSelection selection;
  selection.requestedImplementationName = requestedImplementationName;

  // This is the official reference factory selector. An empty input resolves
  // to HLAfloat64Time, exactly as the Create Federation Execution signature
  // prescribes; no Umbra-specific default is introduced here.
  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      requestedImplementationName);
  if (!factory) {
    selection.diagnostics =
        "Umbra currently provides only the IEEE reference logical-time implementations "
        "HLAinteger64Time and HLAfloat64Time.";
    return selection;
  }

  selection.selectedImplementationName = factory->getName();
  std::string_view const expectedDataType = documentedTimeDataType(selection.selectedImplementationName);
  if (expectedDataType.empty()) {
    // Do not silently accept a third-party implementation merely because a
    // factory happened to be linked. Its encoding-to-FDD relationship must be
    // defined in a future provider contract.
    selection.diagnostics =
        "Umbra has no reviewed FDD data-type contract for the selected logical-time implementation.";
    return selection;
  }

  auto const& time = catalog.time();
  std::array<std::pair<std::string_view, std::string const*>, 2> const documentedTypes{{
      {"logical time", &time.logicalTimeDataType},
      {"logical time interval", &time.logicalTimeIntervalDataType},
  }};
  for (auto const& [column, documentedDataType] : documentedTypes) {
    if (!isUnspecified(*documentedDataType) && *documentedDataType != expectedDataType) {
      selection.status = ReferenceLogicalTimeSelectionStatus::inconsistent_fdd_time_representation;
      selection.diagnostics = describeMismatch(column, *documentedDataType, expectedDataType);
      return selection;
    }
  }

  selection.status = ReferenceLogicalTimeSelectionStatus::selected;
  return selection;
}

}  // namespace umbra::detail
