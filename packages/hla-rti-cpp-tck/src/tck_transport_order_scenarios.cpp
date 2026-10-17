#include "tck_transport_order_scenarios.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioTransportOrderPortable(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session observer(options, model, "member");
  auto const federation = federationName(options, "transport-order");
  connectAndJoin(owner, observer, options, federation, options.fom);

  rti::ObjectClassHandle ownerClass;
  rti::AttributeHandle ownerAttribute;
  rti::InteractionClassHandle ownerInteraction;
  rti::ParameterHandle ownerParameter;
  handles(owner, options, ownerClass, ownerAttribute, ownerInteraction, ownerParameter);
  auto const observerClass = observer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const observerAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerClass, options.attributeName);
  auto const observerInteraction = observer.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(observerClass.isValid() && observerAttribute.isValid() &&
              observerInteraction.isValid(),
          "Transport/order observer lookup returned an invalid handle");
  auto const reliable = owner.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  auto const bestEffort = owner.rtiAmbassador().getTransportationTypeHandle(L"HLAbestEffort");
  require(reliable.isValid() && bestEffort.isValid(),
          "Transport/order lookup did not return the standard transportation handles");

  rti::AttributeHandleSet ownerAttributes;
  ownerAttributes.insert(ownerAttribute);
  rti::AttributeHandleSet observerAttributes;
  observerAttributes.insert(observerAttribute);
  rti::ObjectClassHandle const invalidObjectClass;
  rti::ObjectInstanceHandle const invalidObject;
  rti::InteractionClassHandle const invalidInteraction;
  rti::AttributeHandleSet const invalidAttributes{rti::AttributeHandle{}};
  rti::TransportationTypeHandle const invalidTransportation;
  requireException(
      [&] {
        owner.rtiAmbassador().changeDefaultAttributeOrderType(
            invalidObjectClass,
            ownerAttributes,
            rti::RECEIVE);
      },
      L"ObjectClassNotDefined",
      "default attribute order change with an invalid object class");
  requireException(
      [&] {
        owner.rtiAmbassador().changeDefaultAttributeOrderType(
            ownerClass,
            invalidAttributes,
            rti::RECEIVE);
      },
      L"AttributeNotDefined",
      "default attribute order change with an invalid attribute");
  requireException(
      [&] {
        owner.rtiAmbassador().changeDefaultAttributeTransportationType(
            invalidObjectClass,
            ownerAttributes,
            bestEffort);
      },
      L"ObjectClassNotDefined",
      "default attribute transportation change with an invalid object class");
  requireException(
      [&] {
        owner.rtiAmbassador().changeDefaultAttributeTransportationType(
            ownerClass,
            invalidAttributes,
            bestEffort);
      },
      L"AttributeNotDefined",
      "default attribute transportation change with an invalid attribute");
  requireException(
      [&] {
        owner.rtiAmbassador().changeDefaultAttributeTransportationType(
            ownerClass,
            ownerAttributes,
            invalidTransportation);
      },
      L"InvalidTransportationTypeHandle",
      "default attribute transportation change with an invalid transportation handle");
  requireException(
      [&] {
        owner.rtiAmbassador().requestAttributeTransportationTypeChange(
            invalidObject,
            ownerAttributes,
            bestEffort);
      },
      L"ObjectInstanceNotKnown",
      "attribute transportation change for an unknown object");
  requireException(
      [&] {
        owner.rtiAmbassador().queryAttributeTransportationType(
            invalidObject,
            ownerAttribute);
      },
      L"ObjectInstanceNotKnown",
      "attribute transportation query for an unknown object");
  requireException(
      [&] { owner.rtiAmbassador().changeInteractionOrderType(invalidInteraction, rti::RECEIVE); },
      L"InteractionClassNotDefined",
      "interaction order change with an invalid interaction class");
  requireException(
      [&] {
        owner.rtiAmbassador().requestInteractionTransportationTypeChange(
            invalidInteraction,
            bestEffort);
      },
      L"InteractionClassNotDefined",
      "interaction transportation change with an invalid interaction class");
  requireException(
      [&] {
        owner.rtiAmbassador().queryInteractionTransportationType(
            owner.federateHandle(),
            invalidInteraction);
      },
      L"InteractionClassNotDefined",
      "interaction transportation query with an invalid interaction class");
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass, observerAttributes, true, L"");
  requireException(
      [&] {
        owner.rtiAmbassador().changeInteractionOrderType(
            ownerInteraction, rti::RECEIVE);
      },
      L"InteractionClassNotPublished",
      "unpublished interaction order change");
  owner.rtiAmbassador().publishInteractionClass(ownerInteraction);
  observer.rtiAmbassador().subscribeInteractionClass(observerInteraction, true);
  requireException(
      [&] {
        owner.rtiAmbassador().requestInteractionTransportationTypeChange(
            ownerInteraction,
            invalidTransportation);
      },
      L"InvalidTransportationTypeHandle",
      "published interaction transportation change with an invalid transportation handle");

  owner.rtiAmbassador().changeDefaultAttributeOrderType(
      ownerClass, ownerAttributes, rti::RECEIVE);
  auto const first = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(first.isValid(), "Transport/order first registration returned an invalid handle");
  requireException(
      [&] {
        owner.rtiAmbassador().changeAttributeOrderType(
            invalidObject,
            ownerAttributes,
            rti::RECEIVE);
      },
      L"ObjectInstanceNotKnown",
      "per-instance attribute order change with an unknown object");
  requireException(
      [&] {
        owner.rtiAmbassador().changeAttributeOrderType(
            first,
            invalidAttributes,
            rti::TIMESTAMP);
      },
      L"AttributeNotDefined",
      "per-instance attribute order change with an invalid attribute");
  requireException(
      [&] {
        owner.rtiAmbassador().requestAttributeTransportationTypeChange(
            first,
            invalidAttributes,
            bestEffort);
      },
      L"AttributeNotDefined",
      "attribute transportation change with an invalid attribute");
  requireException(
      [&] {
        owner.rtiAmbassador().requestAttributeTransportationTypeChange(
            first,
            ownerAttributes,
            invalidTransportation);
      },
      L"InvalidTransportationTypeHandle",
      "attribute transportation change with an invalid transportation handle");
  requireException(
      [&] {
        owner.rtiAmbassador().queryAttributeTransportationType(
            first,
            rti::AttributeHandle{});
      },
      L"AttributeNotDefined",
      "attribute transportation query with an invalid attribute");
  owner.rtiAmbassador().changeAttributeOrderType(
      first, ownerAttributes, rti::TIMESTAMP);
  owner.rtiAmbassador().changeAttributeOrderType(
      first, ownerAttributes, rti::RECEIVE);
  owner.rtiAmbassador().changeInteractionOrderType(ownerInteraction, rti::TIMESTAMP);
  owner.rtiAmbassador().changeInteractionOrderType(ownerInteraction, rti::RECEIVE);

  owner.rtiAmbassador().changeDefaultAttributeTransportationType(
      ownerClass, ownerAttributes, bestEffort);
  auto const second = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(second.isValid(), "Transport/order second registration returned an invalid handle");
  waitFor(observer,
          [&] {
            return observer.recorder().hasDiscovery(first) &&
                observer.recorder().hasDiscovery(second);
          },
          options,
          "transport/order object discovery");

  auto sameTransport = [](rti::RTIambassador& inspector,
                          rti::TransportationTypeHandle const& actual,
                          std::wstring const& expectedName) {
    return actual.isValid() &&
        inspector.getTransportationTypeName(actual) == expectedName;
  };

  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().queryAttributeTransportationType(first, ownerAttribute);
  owner.rtiAmbassador().queryAttributeTransportationType(second, ownerAttribute);
  waitFor(owner,
          [&] { return owner.recorder().attributeTransportationReports().size() >= 2U; },
          options,
          "attribute transportation query reports");
  auto const attributeReports = owner.recorder().attributeTransportationReports();
  auto const firstReport = std::find_if(
      attributeReports.begin(), attributeReports.end(), [&](auto const& report) {
        return report.object == first && report.attribute == ownerAttribute;
      });
  auto const secondReport = std::find_if(
      attributeReports.begin(), attributeReports.end(), [&](auto const& report) {
        return report.object == second && report.attribute == ownerAttribute;
      });
  require(firstReport != attributeReports.end() && secondReport != attributeReports.end(),
          "Attribute transportation query reports omitted a registered object");
  require(sameTransport(owner.rtiAmbassador(), firstReport->transportation, L"HLAreliable"),
          "first object did not retain the reliable transportation default");
  require(sameTransport(owner.rtiAmbassador(), secondReport->transportation, L"HLAbestEffort"),
          "second object did not capture the best-effort class default");

  owner.recorder().clearTransportationRecords();
  observer.recorder().clearReflection();
  owner.rtiAmbassador().requestAttributeTransportationTypeChange(
      first, ownerAttributes, bestEffort);
  if (model == rti::HLA_EVOKED) {
    requireException(
        [&] {
          owner.rtiAmbassador().requestAttributeTransportationTypeChange(
              first, ownerAttributes, reliable);
        },
        L"AttributeAlreadyBeingChanged",
        "duplicate attribute transportation request");
  } else {
    waitFor(owner,
            [&] {
              return owner.recorder().attributeTransportationConfirmations().size() >= 1U;
            },
            options,
            "immediate attribute transportation confirmation");
  }
  std::vector<std::uint8_t> beforeValueBytes{0x01U};
  std::vector<std::uint8_t> beforeTagBytes{0x01U};
  rti::AttributeHandleValueMap beforeValues;
  beforeValues.emplace(
      ownerAttribute,
      rti::VariableLengthData(beforeValueBytes.data(), beforeValueBytes.size()));
  rti::VariableLengthData beforeTag(beforeTagBytes.data(), beforeTagBytes.size());
  owner.rtiAmbassador().updateAttributeValues(first, beforeValues, beforeTag);
  waitFor(observer,
          [&] { return observer.recorder().reflection().present; },
          options,
          "pre-confirmation attribute update");
  auto const beforeReflection = observer.recorder().reflection();
  require(beforeReflection.object == first &&
              sameTransport(observer.rtiAmbassador(), beforeReflection.transportation,
                            model == rti::HLA_EVOKED ? L"HLAreliable" : L"HLAbestEffort"),
          model == rti::HLA_EVOKED
              ? "pre-confirmation attribute update did not use reliable transport"
              : "immediate attribute transport change did not use best-effort transport");
  waitFor(owner,
          [&] {
            return owner.recorder().attributeTransportationConfirmations().size() >= 1U;
          },
          options,
          "attribute transportation confirmation");
  auto const attributeConfirmations = owner.recorder().attributeTransportationConfirmations();
  auto const& attributeConfirmation = attributeConfirmations.back();
  require(attributeConfirmation.object == first &&
              attributeConfirmation.attributes.count(ownerAttribute) == 1U &&
              sameTransport(owner.rtiAmbassador(), attributeConfirmation.transportation,
                            L"HLAbestEffort"),
          "attribute transportation confirmation did not commit best effort");

  observer.recorder().clearReflection();
  std::vector<std::uint8_t> afterValueBytes{0x02U};
  std::vector<std::uint8_t> afterTagBytes{0x02U};
  rti::AttributeHandleValueMap afterValues;
  afterValues.emplace(
      ownerAttribute,
      rti::VariableLengthData(afterValueBytes.data(), afterValueBytes.size()));
  rti::VariableLengthData afterTag(afterTagBytes.data(), afterTagBytes.size());
  owner.rtiAmbassador().updateAttributeValues(first, afterValues, afterTag);
  waitFor(observer,
          [&] { return observer.recorder().reflection().present; },
          options,
          "post-confirmation attribute update");
  auto const afterReflection = observer.recorder().reflection();
  require(afterReflection.object == first &&
              sameTransport(observer.rtiAmbassador(), afterReflection.transportation,
                            L"HLAbestEffort"),
          "post-confirmation attribute update did not use best-effort transport");
  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().queryAttributeTransportationType(first, ownerAttribute);
  waitFor(owner,
          [&] { return owner.recorder().attributeTransportationReports().size() >= 1U; },
          options,
          "final attribute transportation query");
  require(sameTransport(
              owner.rtiAmbassador(),
              owner.recorder().attributeTransportationReports().back().transportation,
              L"HLAbestEffort"),
          "attribute transportation query did not observe the committed change");

  owner.recorder().clearTransportationRecords();
  observer.recorder().clearInteraction();
  owner.rtiAmbassador().queryInteractionTransportationType(
      owner.federateHandle(), ownerInteraction);
  waitFor(owner,
          [&] { return owner.recorder().interactionTransportationReports().size() >= 1U; },
          options,
          "initial interaction transportation query");
  auto const initialInteractionReports =
      owner.recorder().interactionTransportationReports();
  require(initialInteractionReports.size() == 1U &&
              initialInteractionReports.back().federate == owner.federateHandle() &&
              initialInteractionReports.back().interaction == ownerInteraction &&
              sameTransport(owner.rtiAmbassador(),
                            initialInteractionReports.back().transportation,
                            L"HLAreliable"),
          "interaction did not begin with the reliable transportation type");
  owner.recorder().clearTransportationRecords();

  // The achieved type must be confirmed even when the request does not change it.
  owner.rtiAmbassador().requestInteractionTransportationTypeChange(
      ownerInteraction, reliable);
  waitFor(owner,
          [&] {
            return owner.recorder().interactionTransportationConfirmations().size() >= 1U;
          },
          options,
          "unchanged interaction transportation confirmation");
  auto const unchangedInteractionConfirmations =
      owner.recorder().interactionTransportationConfirmations();
  require(unchangedInteractionConfirmations.size() == 1U &&
              unchangedInteractionConfirmations.back().interaction == ownerInteraction &&
              sameTransport(owner.rtiAmbassador(),
                            unchangedInteractionConfirmations.back().transportation,
                            L"HLAreliable"),
          "unchanged interaction transportation request did not confirm reliable transport");

  auto const interactionConfirmationsBeforeChange =
      owner.recorder().interactionTransportationConfirmations().size();
  owner.rtiAmbassador().requestInteractionTransportationTypeChange(
      ownerInteraction, bestEffort);
  if (model == rti::HLA_EVOKED) {
    requireException(
        [&] {
          owner.rtiAmbassador().requestInteractionTransportationTypeChange(
              ownerInteraction, reliable);
        },
        L"InteractionClassAlreadyBeingChanged",
        "duplicate interaction transportation request");
  } else {
    waitFor(owner,
            [&] {
              return owner.recorder().interactionTransportationConfirmations().size() >=
                  interactionConfirmationsBeforeChange + 1U;
            },
            options,
            "immediate interaction transportation confirmation");
  }
  std::vector<std::uint8_t> interactionBeforeTagBytes{0x03U};
  rti::VariableLengthData interactionBeforeTag(
      interactionBeforeTagBytes.data(), interactionBeforeTagBytes.size());
  owner.rtiAmbassador().sendInteraction(
      ownerInteraction, rti::ParameterHandleValueMap{}, interactionBeforeTag);
  waitFor(observer,
          [&] { return observer.recorder().interaction().present; },
          options,
          "pre-confirmation interaction delivery");
  auto const interactionBefore = observer.recorder().interaction();
  require(sameTransport(observer.rtiAmbassador(), interactionBefore.transportation,
                        model == rti::HLA_EVOKED ? L"HLAreliable" : L"HLAbestEffort"),
          model == rti::HLA_EVOKED
              ? "pre-confirmation interaction did not use reliable transport"
              : "immediate interaction transport change did not use best-effort transport");
  waitFor(owner,
          [&] {
            return owner.recorder().interactionTransportationConfirmations().size() >=
                interactionConfirmationsBeforeChange + 1U;
          },
          options,
          "interaction transportation confirmation");
  auto const interactionConfirmations =
      owner.recorder().interactionTransportationConfirmations();
  auto const& interactionConfirmation = interactionConfirmations.back();
  require(interactionConfirmations.size() == interactionConfirmationsBeforeChange + 1U &&
              interactionConfirmation.interaction == ownerInteraction &&
              sameTransport(owner.rtiAmbassador(), interactionConfirmation.transportation,
                            L"HLAbestEffort"),
          "interaction transportation confirmation did not commit best effort");

  observer.recorder().clearInteraction();
  std::vector<std::uint8_t> interactionAfterTagBytes{0x04U};
  rti::VariableLengthData interactionAfterTag(
      interactionAfterTagBytes.data(), interactionAfterTagBytes.size());
  owner.rtiAmbassador().sendInteraction(
      ownerInteraction, rti::ParameterHandleValueMap{}, interactionAfterTag);
  waitFor(observer,
          [&] { return observer.recorder().interaction().present; },
          options,
          "post-confirmation interaction delivery");
  auto const interactionAfter = observer.recorder().interaction();
  require(sameTransport(observer.rtiAmbassador(), interactionAfter.transportation,
                        L"HLAbestEffort"),
          "post-confirmation interaction did not use best-effort transport");
  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().queryInteractionTransportationType(
      owner.federateHandle(), ownerInteraction);
  waitFor(owner,
          [&] { return owner.recorder().interactionTransportationReports().size() >= 1U; },
          options,
          "interaction transportation query");
  auto const interactionReports = owner.recorder().interactionTransportationReports();
  auto const& interactionReport = interactionReports.back();
  require(interactionReport.federate == owner.federateHandle() &&
              interactionReport.interaction == ownerInteraction &&
              sameTransport(owner.rtiAmbassador(), interactionReport.transportation,
                            L"HLAbestEffort"),
          "interaction transportation query did not observe the committed change");

  observer.rtiAmbassador().unsubscribeInteractionClass(observerInteraction);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerClass, observerAttributes);
  owner.rtiAmbassador().unpublishInteractionClass(ownerInteraction);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  owner.rtiAmbassador().unpublishObjectClass(ownerClass);
  owner.resign(rti::DELETE_OBJECTS);
  observer.resign(rti::NO_ACTION);
}

void scenarioTransportOrderPortableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTransportOrderPortable(options, model);
}

void scenarioStandardOrderAndTransportationLookupsPortable(
    Options const& options,
    rti::CallbackModel model) {
  Session lifecycle(options, model, "standard-support-type-lifecycle");
  auto requireLookupLifecycle = [&](std::wstring const& expected,
                                   std::string const& phase) {
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().getOrderType(L"Receive"));
        },
        expected,
        "looking up Receive order type " + phase);
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().getOrderName(rti::RECEIVE));
        },
        expected,
        "looking up Receive order name " + phase);
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().getOrderType(L"TimeStamp"));
        },
        expected,
        "looking up TimeStamp order type " + phase);
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().getOrderName(rti::TIMESTAMP));
        },
        expected,
        "looking up TimeStamp order name " + phase);
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().getTransportationTypeHandle(
              L"HLAreliable"));
        },
        expected,
        "looking up reliable transportation type " + phase);
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().getTransportationTypeName(
              rti::TransportationTypeHandle{}));
        },
        expected,
        "looking up an invalid transportation type " + phase);
  };

  requireLookupLifecycle(L"NotConnected", "before connect");
  lifecycle.connect();
  requireLookupLifecycle(
      L"FederateNotExecutionMember",
      "after connect before join");
  lifecycle.disconnect();

  Session owner(options, model, "standard-support-type-owner");
  Session member(options, model, "standard-support-type-member");
  auto const federation = federationName(options, "standard-support-type-lookups");
  connectAndJoin(owner, member, options, federation, options.fom);

  require(
      owner.rtiAmbassador().getOrderType(L"Receive") == rti::RECEIVE &&
          owner.rtiAmbassador().getOrderType(L"TimeStamp") == rti::TIMESTAMP,
      "standard order names did not resolve to the mandatory order types");
  require(
      owner.rtiAmbassador().getOrderName(rti::RECEIVE) == L"Receive" &&
          owner.rtiAmbassador().getOrderName(rti::TIMESTAMP) == L"TimeStamp",
      "mandatory order types did not round-trip to their standard names");
  require(
      member.rtiAmbassador().getOrderType(L"Receive") == rti::RECEIVE &&
          member.rtiAmbassador().getOrderType(L"TimeStamp") == rti::TIMESTAMP,
      "member order lookups did not resolve to the mandatory order types");

  auto const ownerReliable = owner.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  auto const ownerBestEffort = owner.rtiAmbassador().getTransportationTypeHandle(
      L"HLAbestEffort");
  auto const memberReliable = member.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  auto const memberBestEffort = member.rtiAmbassador().getTransportationTypeHandle(
      L"HLAbestEffort");
  require(
      ownerReliable.isValid() && ownerBestEffort.isValid() &&
          memberReliable.isValid() && memberBestEffort.isValid(),
      "mandatory transportation lookup returned an invalid handle");
  require(
      ownerReliable != ownerBestEffort && memberReliable != memberBestEffort,
      "mandatory transportation types did not remain distinct");
  require(
      ownerReliable == memberReliable && ownerBestEffort == memberBestEffort,
      "mandatory transportation handles were not stable across federates");
  require(
      owner.rtiAmbassador().getTransportationTypeName(ownerReliable) ==
              L"HLAreliable" &&
          owner.rtiAmbassador().getTransportationTypeName(ownerBestEffort) ==
              L"HLAbestEffort" &&
          member.rtiAmbassador().getTransportationTypeName(memberReliable) ==
              L"HLAreliable" &&
          member.rtiAmbassador().getTransportationTypeName(memberBestEffort) ==
              L"HLAbestEffort",
      "mandatory transportation handles did not round-trip to their standard names");

  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getOrderType(L"MissingOrder"));
      },
      L"InvalidOrderName",
      "missing order name lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getOrderName(
            static_cast<rti::OrderType>(0x7fU)));
      },
      L"InvalidOrderType",
      "invalid order type lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getTransportationTypeHandle(
            L"MissingTransportation"));
      },
      L"InvalidTransportationName",
      "missing transportation name lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getTransportationTypeName(
            rti::TransportationTypeHandle{}));
      },
      L"InvalidTransportationTypeHandle",
      "invalid transportation handle lookup");

  owner.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioStandardOrderAndTransportationLookupsPortableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioStandardOrderAndTransportationLookupsPortable(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
