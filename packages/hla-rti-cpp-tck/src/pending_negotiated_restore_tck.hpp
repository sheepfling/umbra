#pragma once

#include <RTI/RTI1516.h>

#include <chrono>
#include <filesystem>
#include <string>

namespace hla_rti_cpp_tck {

struct PendingNegotiatedRestoreConfiguration final {
  std::filesystem::path applicationFom;
  std::filesystem::path standardMim;
  std::filesystem::path serviceReportDirectory;
  std::wstring logicalTimeImplementation;
  std::string configurationName;
  std::string rtiAddress;
  std::chrono::milliseconds timeout{10000};
  rti1516_2025::CallbackModel callbackModel = rti1516_2025::HLA_EVOKED;
  std::wstring objectClassName = L"HLAobjectRoot.TckObject";
  std::wstring attributeName = L"Value";
};

void runPendingNegotiatedRestore(
    PendingNegotiatedRestoreConfiguration const& configuration);

}  // namespace hla_rti_cpp_tck
