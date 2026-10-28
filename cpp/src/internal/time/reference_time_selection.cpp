#include "internal/time/reference_time_selection.hpp"

#include "internal/fom/fom_catalog.hpp"
#include "internal/runtime/utf8_string.hpp"

#include "internal/time/federation_time_factory_provider.hpp"

#include <array>
#include <string_view>

namespace umbra::detail {
namespace {

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
  return "The composed FDD documents " + std::string(column) + " data type " +
         quoteDiagnosticString(actual) +
         ", which is incompatible with Umbra's selected logical-time factory " +
         quoteDiagnosticString(expected) + ".";
}

}  // namespace

ReferenceLogicalTimeSelection ReferenceLogicalTimeSelector::select(
    FomCatalog const& catalog,
    std::wstring const& requestedImplementationName) const {
  ReferenceLogicalTimeSelection selection;
  selection.requestedImplementationName = requestedImplementationName;

  // The federation's official fedtime factory-factory chooses its concrete
  // time classes from the Create Federation Execution implementation name.
  auto factory = makeFederationLogicalTimeFactory(
      requestedImplementationName);
  if (!factory) {
    selection.diagnostics = "The federation's fedtime library does not provide the requested "
                            "logical-time implementation.";
    return selection;
  }

  selection.selectedImplementationName = factory->getName();
  if (selection.selectedImplementationName.empty() ||
      (!requestedImplementationName.empty() &&
       selection.selectedImplementationName != requestedImplementationName)) {
    selection.diagnostics = "The federation's fedtime library returned a factory with an "
                            "unexpected implementation name.";
    return selection;
  }
  auto expectedDataType = utf8FromWide(selection.selectedImplementationName);
  if (!expectedDataType || expectedDataType->empty()) {
    selection.diagnostics = "The selected logical-time implementation name cannot be represented "
                            "in the FDD's UTF-8 data-type field.";
    return selection;
  }

  auto const& time = catalog.time();
  std::array<std::pair<std::string_view, std::string const*>, 2> const documentedTypes{{
      {"logical time", &time.logicalTimeDataType},
      {"logical time interval", &time.logicalTimeIntervalDataType},
  }};
  for (auto const& [column, documentedDataType] : documentedTypes) {
    if (!isUnspecified(*documentedDataType) && *documentedDataType != *expectedDataType) {
      selection.status = ReferenceLogicalTimeSelectionStatus::inconsistent_fdd_time_representation;
      selection.diagnostics = describeMismatch(column, *documentedDataType, *expectedDataType);
      return selection;
    }
  }

  selection.status = ReferenceLogicalTimeSelectionStatus::selected;
  return selection;
}

}  // namespace umbra::detail
