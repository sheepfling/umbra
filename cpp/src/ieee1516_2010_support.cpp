#include <RTI/RTI1516.h>

namespace rti1516e {

FederationExecutionInformation::FederationExecutionInformation(
    std::wstring const& federationName,
    std::wstring const& logicalTimeImplementationName)
    : federationExecutionName(federationName),
      logicalTimeImplementationName(logicalTimeImplementationName) {}

SupplementalReflectInfo::SupplementalReflectInfo()
    : hasProducingFederate(false), hasSentRegions(false), producingFederate(), sentRegions() {}

SupplementalReflectInfo::SupplementalReflectInfo(FederateHandle const& federateHandle)
    : hasProducingFederate(true), hasSentRegions(false), producingFederate(federateHandle), sentRegions() {}

SupplementalReflectInfo::SupplementalReflectInfo(RegionHandleSet const& regions)
    : hasProducingFederate(false), hasSentRegions(true), producingFederate(), sentRegions(regions) {}

SupplementalReflectInfo::SupplementalReflectInfo(
    FederateHandle const& federateHandle, RegionHandleSet const& regions)
    : hasProducingFederate(true), hasSentRegions(true), producingFederate(federateHandle), sentRegions(regions) {}

SupplementalReceiveInfo::SupplementalReceiveInfo()
    : hasProducingFederate(false), hasSentRegions(false), producingFederate(), sentRegions() {}

SupplementalReceiveInfo::SupplementalReceiveInfo(FederateHandle const& federateHandle)
    : hasProducingFederate(true), hasSentRegions(false), producingFederate(federateHandle), sentRegions() {}

SupplementalReceiveInfo::SupplementalReceiveInfo(RegionHandleSet const& regions)
    : hasProducingFederate(false), hasSentRegions(true), producingFederate(), sentRegions(regions) {}

SupplementalReceiveInfo::SupplementalReceiveInfo(
    FederateHandle const& federateHandle, RegionHandleSet const& regions)
    : hasProducingFederate(true), hasSentRegions(true), producingFederate(federateHandle), sentRegions(regions) {}

SupplementalRemoveInfo::SupplementalRemoveInfo()
    : hasProducingFederate(false), producingFederate() {}

SupplementalRemoveInfo::SupplementalRemoveInfo(FederateHandle const& federateHandle)
    : hasProducingFederate(true), producingFederate(federateHandle) {}

}  // namespace rti1516e
