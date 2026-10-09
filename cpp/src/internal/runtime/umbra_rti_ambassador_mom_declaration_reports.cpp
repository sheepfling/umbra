#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {

template <typename RecipientResolver>
void submitMomDeclarationReport(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId,
    std::vector<umbra::detail::ReceiveOrderInteractionRecipient> const& plannedRecipients,
    std::vector<AmbassadorInteractionParameterValue> sentParameters,
    TransportationTypeHandle reliableTransportation,
    RecipientResolver resolveRecipient) {
  for (auto const& plannedRecipient : plannedRecipients) {
    if (!plannedRecipient.callbackRoute || plannedRecipient.federateId == 0U) {
      continue;
    }
    auto callbackRoute = plannedRecipient.callbackRoute;
    auto const receivingFederateId = plannedRecipient.federateId;
    submitAmbassadorReceiveOrderCallback(
        std::move(callbackRoute),
        federationName,
        receivingFederateId,
        [federationName,
         reportedFederateId,
         receivingFederateId,
         sentParameters,
         reliableTransportation,
         resolveRecipient](FederateAmbassador& recipient) mutable {
          std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
          {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            projection = resolveRecipient(
                embeddedFederationRegistry(),
                federationName,
                reportedFederateId,
                receivingFederateId);
          }
          if (!projection) {
            return;
          }
          auto const parameterValues = ambassadorProjectInteractionParameterValues(
              sentParameters,
              projection->receivedParameterHandles);
          recipient.receiveInteraction(
              makeInteractionClassHandle(projection->receivedInteractionClassHandle),
              parameterValues,
              VariableLengthData{},
              reliableTransportation,
              FederateHandle{},
              nullptr);
        });
  }
}

[[nodiscard]] VariableLengthData encodeMomInteractionClassHandleList(
    std::set<std::uint64_t> const& interactionClassHandles) {
  HLAvariableArray interactionClassHandlePrototype{HLAoctet{}};
  HLAvariableArray handles{
      static_cast<DataElement const&>(interactionClassHandlePrototype)};
  for (auto const interactionClassHandle : interactionClassHandles) {
    if (interactionClassHandle == 0U) {
      continue;
    }
    HLAvariableArray encodedInteractionClassHandle{HLAoctet{}};
    encodedInteractionClassHandle.decode(
        makeInteractionClassHandle(interactionClassHandle).encode());
    handles.addElement(encodedInteractionClassHandle);
  }
  return handles.encode();
}

[[nodiscard]] VariableLengthData encodeMomInteractionSubscriptionList(
    std::map<std::uint64_t, std::set<bool>> const& subscriptions) {
  HLAvariableArrayT<HLAoctet> interactionClassPrototype;
  HLAfixedRecord recordPrototype;
  recordPrototype.appendElement(interactionClassPrototype)
      .appendElement(HLAboolean{});
  HLAvariableArray subscriptionsValue{recordPrototype};
  for (auto const& [interactionClassHandle, activeValues] : subscriptions) {
    if (interactionClassHandle == 0U) {
      continue;
    }
    for (auto const active : activeValues) {
      HLAfixedRecord record;
      HLAvariableArrayT<HLAoctet> encodedInteractionClassHandle;
      encodedInteractionClassHandle.decode(
          makeInteractionClassHandle(interactionClassHandle).encode());
      record.appendElement(encodedInteractionClassHandle)
          .appendElement(HLAboolean{active});
      subscriptionsValue.addElement(record);
    }
  }
  return subscriptionsValue.encode();
}

}  // namespace

VariableLengthData ambassadorEncodeMomAttributeHandleList(
    std::set<std::uint64_t> const& attributeHandles) {
  // HLAattributeHandleList is an HLAvariableArray of encoded handle arrays.
  // Keep handle width and nested-array encoding owned by the official binding.
  HLAvariableArray attributeHandlePrototype{HLAoctet{}};
  HLAvariableArray handles{
      static_cast<DataElement const&>(attributeHandlePrototype)};
  for (auto const attributeHandle : attributeHandles) {
    if (attributeHandle == 0U) {
      continue;
    }
    HLAvariableArray encodedAttributeHandle{HLAoctet{}};
    encodedAttributeHandle.decode(makeAttributeHandle(attributeHandle).encode());
    handles.addElement(encodedAttributeHandle);
  }
  return handles.encode();
}

void queueMomObjectClassPublicationReports(
    std::wstring federationName,
    umbra::detail::MomPublicationsReportPlan const& report) {
  if (report.status != umbra::detail::MomPublicationsReportStatus::applied ||
      report.objectClassRouting.interactionClassHandle == 0U ||
      report.objectClassRouting.numberOfClassesParameterHandle == 0U ||
      report.objectClassRouting.objectClassParameterHandle == 0U ||
      report.objectClassRouting.attributeListParameterHandle == 0U) {
    return;
  }

  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportObjectClassPublication transportation.");
  auto const boundedClassCount = [&report] {
    auto const maximum = static_cast<std::size_t>(
        std::numeric_limits<Integer32>::max());
    auto const count = std::min(report.objectClassAttributesByClass.size(), maximum);
    return HLAinteger32BE{static_cast<Integer32>(count)};
  }();
  auto const emit = [&](
                         std::optional<std::pair<std::uint64_t, std::set<std::uint64_t>>> publication) {
    std::vector<AmbassadorInteractionParameterValue> sentParameters;
    sentParameters.emplace_back(
        report.objectClassRouting.numberOfClassesParameterHandle,
        boundedClassCount.encode());
    if (publication) {
      sentParameters.emplace_back(
          report.objectClassRouting.objectClassParameterHandle,
          makeObjectClassHandle(publication->first).encode());
      sentParameters.emplace_back(
          report.objectClassRouting.attributeListParameterHandle,
          ambassadorEncodeMomAttributeHandleList(publication->second));
    }
    submitMomDeclarationReport(
        federationName,
        report.reportedFederateId,
        report.objectClassRecipients,
        std::move(sentParameters),
        reliableTransportation,
        [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
          return registry.momObjectClassPublicationReportRecipientFor(
              federation, reportedId, receivingId);
        });
  };

  if (report.objectClassAttributesByClass.empty()) {
    emit(std::nullopt);
    return;
  }
  for (auto const& publication : report.objectClassAttributesByClass) {
    emit(std::make_pair(publication.first, publication.second));
  }
}

void queueMomInteractionPublicationReport(
    std::wstring federationName,
    umbra::detail::MomPublicationsReportPlan const& report) {
  if (report.status != umbra::detail::MomPublicationsReportStatus::applied ||
      report.interactionRouting.interactionClassHandle == 0U ||
      report.interactionRouting.interactionClassListParameterHandle == 0U) {
    return;
  }

  std::vector<AmbassadorInteractionParameterValue> sentParameters{
      {report.interactionRouting.interactionClassListParameterHandle,
       encodeMomInteractionClassHandleList(report.interactionClassHandles)}};
  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportInteractionPublication transportation.");
  submitMomDeclarationReport(
      federationName,
      report.reportedFederateId,
      report.interactionRecipients,
      std::move(sentParameters),
      reliableTransportation,
      [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
        return registry.momInteractionPublicationReportRecipientFor(
            federation, reportedId, receivingId);
      });
}

void queueMomDirectedInteractionPublicationReports(
    std::wstring federationName,
    umbra::detail::MomPublicationsReportPlan const& report) {
  if (report.status != umbra::detail::MomPublicationsReportStatus::applied ||
      report.directedRouting.interactionClassHandle == 0U ||
      report.directedRouting.numberOfClassesParameterHandle == 0U ||
      report.directedRouting.objectClassParameterHandle == 0U ||
      report.directedRouting.interactionClassListParameterHandle == 0U) {
    return;
  }

  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportDirectedInteractionPublication transportation.");
  auto const boundedClassCount = [&report] {
    auto const maximum = static_cast<std::size_t>(
        std::numeric_limits<Integer32>::max());
    auto const count = std::min(report.directedInteractionClassesByObjectClass.size(), maximum);
    return HLAinteger32BE{static_cast<Integer32>(count)};
  }();
  auto const emit = [&](
                         std::optional<std::pair<std::uint64_t, std::set<std::uint64_t>>> publication) {
    std::vector<AmbassadorInteractionParameterValue> sentParameters;
    sentParameters.emplace_back(
        report.directedRouting.numberOfClassesParameterHandle,
        boundedClassCount.encode());
    if (publication) {
      sentParameters.emplace_back(
          report.directedRouting.objectClassParameterHandle,
          makeObjectClassHandle(publication->first).encode());
      sentParameters.emplace_back(
          report.directedRouting.interactionClassListParameterHandle,
          encodeMomInteractionClassHandleList(publication->second));
    } else {
      // The MIM requires an empty list in a directed-publication NULL response.
      sentParameters.emplace_back(
          report.directedRouting.interactionClassListParameterHandle,
          encodeMomInteractionClassHandleList(std::set<std::uint64_t>{}));
    }
    submitMomDeclarationReport(
        federationName,
        report.reportedFederateId,
        report.directedRecipients,
        std::move(sentParameters),
        reliableTransportation,
        [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
          return registry.momDirectedInteractionPublicationReportRecipientFor(
              federation, reportedId, receivingId);
        });
  };

  if (report.directedInteractionClassesByObjectClass.empty()) {
    emit(std::nullopt);
    return;
  }
  bool emitted = false;
  for (auto const& publication : report.directedInteractionClassesByObjectClass) {
    if (!publication.second.empty()) {
      emit(std::make_pair(publication.first, publication.second));
      emitted = true;
    }
  }
  if (!emitted) {
    emit(std::nullopt);
  }
}

void queueMomObjectClassSubscriptionReports(
    std::wstring federationName,
    umbra::detail::MomSubscriptionsReportPlan const& report) {
  if (report.status != umbra::detail::MomSubscriptionsReportStatus::applied ||
      report.objectClassRouting.interactionClassHandle == 0U ||
      report.objectClassRouting.numberOfClassesParameterHandle == 0U ||
      report.objectClassRouting.objectClassParameterHandle == 0U ||
      report.objectClassRouting.activeParameterHandle == 0U ||
      report.objectClassRouting.maxUpdateRateParameterHandle == 0U ||
      report.objectClassRouting.attributeListParameterHandle == 0U) {
    return;
  }

  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportObjectClassSubscription transportation.");
  auto const groupCount = [&report] {
    std::size_t count = 0U;
    for (auto const& [objectClassHandle, groups] : report.objectClassSubscriptions) {
      static_cast<void>(objectClassHandle);
      for (auto const& [active, snapshot] : groups) {
        static_cast<void>(active);
        if (!snapshot.attributeHandles.empty()) {
          ++count;
        }
      }
    }
    return static_cast<Integer32>(std::min(
        count,
        static_cast<std::size_t>(std::numeric_limits<Integer32>::max())));
  }();
  auto const emit = [&](
                         std::optional<umbra::detail::MomObjectClassSubscriptionSnapshot> snapshot) {
    std::vector<AmbassadorInteractionParameterValue> sentParameters;
    sentParameters.emplace_back(
        report.objectClassRouting.numberOfClassesParameterHandle,
        HLAinteger32BE{groupCount}.encode());
    if (snapshot) {
      sentParameters.emplace_back(
          report.objectClassRouting.objectClassParameterHandle,
          makeObjectClassHandle(snapshot->objectClassHandle).encode());
      sentParameters.emplace_back(
          report.objectClassRouting.activeParameterHandle,
          HLAboolean{snapshot->active}.encode());
      auto const wideRate = umbra::detail::wideFromUtf8(
          snapshot->maxUpdateRate.empty()
              ? std::string_view{umbra::detail::hla::utf8::mom::default_update_rate}
              : std::string_view{snapshot->maxUpdateRate});
      if (!wideRate) {
        return;
      }
      sentParameters.emplace_back(
          report.objectClassRouting.maxUpdateRateParameterHandle,
          HLAunicodeString{*wideRate}.encode());
      sentParameters.emplace_back(
          report.objectClassRouting.attributeListParameterHandle,
          ambassadorEncodeMomAttributeHandleList(snapshot->attributeHandles));
    }
    submitMomDeclarationReport(
        federationName,
        report.reportedFederateId,
        report.objectClassRecipients,
        std::move(sentParameters),
        reliableTransportation,
        [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
          return registry.momObjectClassSubscriptionReportRecipientFor(
              federation, reportedId, receivingId);
        });
  };

  if (groupCount == 0) {
    emit(std::nullopt);
    return;
  }
  for (auto const& [objectClassHandle, groups] : report.objectClassSubscriptions) {
    static_cast<void>(objectClassHandle);
    for (auto const& [active, snapshot] : groups) {
      static_cast<void>(active);
      if (!snapshot.attributeHandles.empty()) {
        emit(snapshot);
      }
    }
  }
}

void queueMomInteractionSubscriptionReport(
    std::wstring federationName,
    umbra::detail::MomSubscriptionsReportPlan const& report) {
  if (report.status != umbra::detail::MomSubscriptionsReportStatus::applied ||
      report.interactionRouting.interactionClassHandle == 0U ||
      report.interactionRouting.interactionClassListParameterHandle == 0U) {
    return;
  }
  std::vector<AmbassadorInteractionParameterValue> sentParameters{
      {report.interactionRouting.interactionClassListParameterHandle,
       encodeMomInteractionSubscriptionList(report.interactionSubscriptionActives)}};
  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportInteractionSubscription transportation.");
  submitMomDeclarationReport(
      federationName,
      report.reportedFederateId,
      report.interactionRecipients,
      std::move(sentParameters),
      reliableTransportation,
      [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
        return registry.momInteractionSubscriptionReportRecipientFor(
            federation, reportedId, receivingId);
      });
}

void queueMomDirectedInteractionSubscriptionReports(
    std::wstring federationName,
    umbra::detail::MomSubscriptionsReportPlan const& report) {
  if (report.status != umbra::detail::MomSubscriptionsReportStatus::applied ||
      report.directedRouting.interactionClassHandle == 0U ||
      report.directedRouting.numberOfClassesParameterHandle == 0U ||
      report.directedRouting.objectClassParameterHandle == 0U ||
      report.directedRouting.interactionClassListParameterHandle == 0U) {
    return;
  }
  auto const hasUniversal = report.directedRouting.universalParameterHandle != 0U;
  std::size_t groupCount = 0U;
  if (hasUniversal) {
    for (auto const& [objectClassHandle, groups] :
         report.directedInteractionClassesByObjectClassAndUniversal) {
      static_cast<void>(objectClassHandle);
      for (auto const& [universal, interactions] : groups) {
        static_cast<void>(universal);
        if (!interactions.empty()) {
          ++groupCount;
        }
      }
    }
  } else {
    for (auto const& [objectClassHandle, groups] :
         report.directedInteractionClassesByObjectClassAndUniversal) {
      static_cast<void>(objectClassHandle);
      std::set<std::uint64_t> interactions;
      for (auto const& [universal, classHandles] : groups) {
        static_cast<void>(universal);
        interactions.insert(classHandles.begin(), classHandles.end());
      }
      if (!interactions.empty()) {
        ++groupCount;
      }
    }
  }
  auto const boundedClassCount = static_cast<Integer32>(std::min(
      groupCount,
      static_cast<std::size_t>(std::numeric_limits<Integer32>::max())));
  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportDirectedInteractionSubscription transportation.");
  auto const emit = [&](
                         std::optional<std::pair<std::uint64_t, std::set<std::uint64_t>>> snapshot,
                         std::optional<bool> universal) {
    std::vector<AmbassadorInteractionParameterValue> sentParameters;
    sentParameters.emplace_back(
        report.directedRouting.numberOfClassesParameterHandle,
        HLAinteger32BE{boundedClassCount}.encode());
    if (snapshot) {
      sentParameters.emplace_back(
          report.directedRouting.objectClassParameterHandle,
          makeObjectClassHandle(snapshot->first).encode());
      if (hasUniversal) {
        sentParameters.emplace_back(
            report.directedRouting.universalParameterHandle,
            HLAboolean{universal.value_or(false)}.encode());
      }
      sentParameters.emplace_back(
          report.directedRouting.interactionClassListParameterHandle,
          encodeMomInteractionClassHandleList(snapshot->second));
    } else {
      sentParameters.emplace_back(
          report.directedRouting.interactionClassListParameterHandle,
          encodeMomInteractionClassHandleList(std::set<std::uint64_t>{}));
    }
    submitMomDeclarationReport(
        federationName,
        report.reportedFederateId,
        report.directedRecipients,
        std::move(sentParameters),
        reliableTransportation,
        [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
          return registry.momDirectedInteractionSubscriptionReportRecipientFor(
              federation, reportedId, receivingId);
        });
  };

  if (groupCount == 0U) {
    emit(std::nullopt, std::nullopt);
    return;
  }
  if (hasUniversal) {
    for (auto const& [objectClassHandle, groups] :
         report.directedInteractionClassesByObjectClassAndUniversal) {
      for (auto const& [universal, interactions] : groups) {
        if (!interactions.empty()) {
          emit(std::make_pair(objectClassHandle, interactions), universal);
        }
      }
    }
  } else {
    for (auto const& [objectClassHandle, groups] :
         report.directedInteractionClassesByObjectClassAndUniversal) {
      std::set<std::uint64_t> interactions;
      for (auto const& [universal, classHandles] : groups) {
        static_cast<void>(universal);
        interactions.insert(classHandles.begin(), classHandles.end());
      }
      if (!interactions.empty()) {
        emit(std::make_pair(objectClassHandle, interactions), std::nullopt);
      }
    }
  }
}
#endif

}  // namespace rti1516_2025::umbra_binding_detail
