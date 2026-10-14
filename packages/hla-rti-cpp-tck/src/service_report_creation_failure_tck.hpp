#pragma once

#include <RTI/RTI1516.h>

#include <filesystem>
#include <string>

namespace hla_rti_cpp_tck {

struct ServiceReportCreationFailureConfiguration final {
  std::filesystem::path applicationFom;
  std::filesystem::path standardMim;
  std::filesystem::path serviceReportDirectory;
  std::wstring logicalTimeImplementation;
  std::string configurationName;
  rti1516_2025::CallbackModel callbackModel = rti1516_2025::HLA_EVOKED;
};

void runServiceReportCreationFailure(
    ServiceReportCreationFailureConfiguration const& configuration);

void runServiceReportConnectFailure(
    ServiceReportCreationFailureConfiguration const& configuration);

}  // namespace hla_rti_cpp_tck
