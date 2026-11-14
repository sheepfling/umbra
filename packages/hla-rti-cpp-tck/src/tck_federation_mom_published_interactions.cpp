#include "tck_main_scenarios.hpp"

#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void verifyFederationMomTable7PublishedInteractions(Session& observer) {
  // These are the 2025 HLAstandardMIM leaf interaction classes under the
  // HLAmanager report branches whose sharing is Publish (Table 7, P).
  std::vector<std::wstring> const publishedNames{
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionPublication",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionsReceived",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionsSent",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionSubscription",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportException",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFederateLost",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFOMmoduleData",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionPublication",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsReceived",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsSent",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionSubscription",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassPublication",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassSubscription",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstanceInformation",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesReflected",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesThatCanBeDeleted",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesUpdated",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportReflectionsReceived",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation",
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportUpdatesSent",
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportFOMmoduleData",
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata",
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPoints",
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPointStatus",
  };

  require(
      publishedNames.size() == 25U,
      "The Table 7 P-leaf inventory must contain all 25 2025 MIM entries");
  for (auto const& name : publishedNames) {
    auto const handle = observer.rtiAmbassador().getInteractionClassHandle(name);
    require(
        handle.isValid() &&
            observer.rtiAmbassador().getInteractionClassName(handle) == name,
        "The standard MIM did not publish Table 7 P interaction " +
            std::string(name.begin(), name.end()));
  }
}

}  // namespace hla_rti_cpp_tck::detail
