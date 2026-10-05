#pragma once

#include <cstdint>

namespace umbra::detail {

inline constexpr char kReportServiceInvocationInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
inline constexpr char kReportFederateLostInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFederateLost";
inline constexpr char kHlaFederateDimensionName[] = "HLAfederate";
inline constexpr char kFederateLostFederateParameterName[] = "HLAfederate";
inline constexpr char kExceptionReportServiceParameterName[] = "HLAservice";
inline constexpr char kExceptionReportExceptionParameterName[] = "HLAexception";
inline constexpr char kReportObjectInstanceCountsParameterName[] =
    "HLAobjectInstanceCounts";
inline constexpr std::uint64_t kFederateNormalizationKind = 0xEB41A82B7D1E63F5ULL;
inline constexpr std::uint64_t kObjectClassNormalizationKind = 0x49B17E0D9346AC27ULL;
inline constexpr std::uint64_t kInteractionClassNormalizationKind = 0xC3D05B987A2E41F9ULL;
inline constexpr std::uint64_t kObjectInstanceNormalizationKind = 0x76F29C3E0B5DA418ULL;

}  // namespace umbra::detail
