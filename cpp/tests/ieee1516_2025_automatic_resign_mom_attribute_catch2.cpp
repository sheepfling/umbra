#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The automatic-resign MOM reflection test requires the Umbra source directory."
#endif

namespace {

using namespace rti1516_2025;

struct AttributeReflection final {
  ObjectInstanceHandle object;
  AttributeHandleValueMap values;
  VariableLengthData tag;
  TransportationTypeHandle transportation;
  FederateHandle producingFederate;
  bool hasSentRegions = false;
};

class MomObserver final : public NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const& object,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    discoveredObjects.push_back(object);
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& object,
      AttributeHandleValueMap const& values,
      VariableLengthData const& tag,
      TransportationTypeHandle const& transportation,
      FederateHandle const& producingFederate,
      RegionHandleSet const* sentRegions) override {
    reflections.push_back({
        object,
        values,
        tag,
        transportation,
        producingFederate,
        sentRegions != nullptr,
    });
  }

  std::vector<ObjectInstanceHandle> discoveredObjects;
  std::vector<AttributeReflection> reflections;
};

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0U};
  return L"umbra-automatic-resign-mom-reflection-" +
      std::to_wstring(++counter);
}

std::filesystem::path standardResource(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relative;
}

std::filesystem::path fixturePath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relative;
}

void drain(RTIambassador& rti) {
  while (rti.evokeCallback(0.0)) {
  }
}

std::optional<ObjectInstanceHandle> findFederateMomObject(
    MomObserver const& observer,
    AttributeHandle const& nameAttribute,
    std::wstring const& federateName) {
  for (auto const& reflection : observer.reflections) {
    auto const nameValue = reflection.values.find(nameAttribute);
    if (nameValue == reflection.values.end()) {
      continue;
    }
    HLAunicodeString decodedName;
    decodedName.decode(nameValue->second);
    if (decodedName.get() == federateName) {
      return reflection.object;
    }
  }
  return std::nullopt;
}

void requireActionReflection(
    MomObserver const& observer,
    std::size_t firstReflection,
    ObjectInstanceHandle const& object,
    AttributeHandle const& actionAttribute,
    TransportationTypeHandle const& reliable,
    ResignAction const expected) {
  std::vector<AttributeReflection const*> matching;
  for (std::size_t index = firstReflection;
       index < observer.reflections.size();
       ++index) {
    auto const& reflection = observer.reflections[index];
    if (reflection.object == object &&
        reflection.values.contains(actionAttribute)) {
      matching.push_back(&reflection);
    }
  }
  REQUIRE(matching.size() == 1U);
  auto const& reflection = *matching.front();
  REQUIRE(reflection.values.size() == 1U);
  REQUIRE(reflection.tag.size() == 0U);
  REQUIRE(reflection.transportation == reliable);
  REQUIRE_FALSE(reflection.producingFederate.isValid());
  REQUIRE_FALSE(reflection.hasSentRegions);
  HLAinteger32BE decodedAction;
  REQUIRE_NOTHROW(decodedAction.decode(reflection.values.at(actionAttribute)));
  REQUIRE(decodedAction.get() == static_cast<std::int32_t>(expected));
}

void verifyAutomaticResignMomReflection(
    bool const withExplicitFddValue,
    ResignAction const expectedInitialAction) {
  MomObserver monitorCallbacks;
  NullFederateAmbassador subjectCallbacks;
  RTIambassadorFactory factory;
  auto monitor = factory.createRTIambassador();
  auto subject = factory.createRTIambassador();
  auto const federationName = nextFederationName();
  auto const monitorName = L"automatic-resign-monitor";
  auto const subjectName = L"automatic-resign-subject";
  auto const mim = standardResource("mim/HLAstandardMIM-2025.xml").wstring();
  std::vector<std::wstring> fomModules{
      standardResource("examples/RestaurantFOMmodule-2025.xml").wstring()};
  if (withExplicitFddValue) {
    fomModules.push_back(fixturePath("switch-support-enabled-fom.xml").wstring());
  }

  bool monitorConnected = false;
  bool subjectConnected = false;
  bool federationCreated = false;
  bool monitorJoined = false;
  bool subjectJoined = false;
  try {
    REQUIRE_NOTHROW(monitor->connect(monitorCallbacks, HLA_EVOKED));
    monitorConnected = true;
    REQUIRE_NOTHROW(subject->connect(subjectCallbacks, HLA_EVOKED));
    subjectConnected = true;
    REQUIRE_NOTHROW(monitor->createFederationExecutionWithMIM(
        federationName, fomModules, mim, L"HLAinteger64Time"));
    federationCreated = true;
    REQUIRE_NOTHROW(monitor->joinFederationExecution(
        monitorName, L"monitor", federationName));
    monitorJoined = true;

    auto const momClass = monitor->getObjectClassHandle(
        L"HLAobjectRoot.HLAmanager.HLAfederate");
    auto const nameAttribute =
        monitor->getAttributeHandle(momClass, L"HLAfederateName");
    auto const actionAttribute =
        monitor->getAttributeHandle(momClass, L"HLAautomaticResignAction");
    auto const reliable = monitor->getTransportationTypeHandle(L"HLAreliable");
    REQUIRE(momClass.isValid());
    REQUIRE(nameAttribute.isValid());
    REQUIRE(actionAttribute.isValid());
    REQUIRE(reliable.isValid());
    REQUIRE_NOTHROW(monitor->subscribeObjectClassAttributes(
        momClass, AttributeHandleSet{nameAttribute, actionAttribute}));
    drain(*monitor);

    REQUIRE_NOTHROW(subject->joinFederationExecution(
        subjectName, L"subject", federationName));
    subjectJoined = true;
    drain(*monitor);
    auto const subjectMomObject =
        findFederateMomObject(monitorCallbacks, nameAttribute, subjectName);
    REQUIRE(subjectMomObject.has_value());
    REQUIRE(subjectMomObject->isValid());

    auto const initialAction = subject->getAutomaticResignDirective();
    REQUIRE(initialAction == expectedInitialAction);
    auto requestAction = [&](ResignAction const expected) {
      auto const firstReflection = monitorCallbacks.reflections.size();
      REQUIRE_NOTHROW(monitor->requestAttributeValueUpdate(
          *subjectMomObject,
          AttributeHandleSet{actionAttribute},
          VariableLengthData{}));
      drain(*monitor);
      requireActionReflection(
          monitorCallbacks,
          firstReflection,
          *subjectMomObject,
          actionAttribute,
          reliable,
          expected);
    };
    requestAction(initialAction);

    auto const changedAction = initialAction == NO_ACTION
        ? DELETE_OBJECTS
        : NO_ACTION;
    auto const firstChangedReflection = monitorCallbacks.reflections.size();
    REQUIRE_NOTHROW(subject->setAutomaticResignDirective(changedAction));
    REQUIRE(subject->getAutomaticResignDirective() == changedAction);
    drain(*monitor);
    requireActionReflection(
        monitorCallbacks,
        firstChangedReflection,
        *subjectMomObject,
        actionAttribute,
        reliable,
        changedAction);
    requestAction(changedAction);

    auto const firstRestoredReflection = monitorCallbacks.reflections.size();
    REQUIRE_NOTHROW(subject->setAutomaticResignDirective(initialAction));
    REQUIRE(subject->getAutomaticResignDirective() == initialAction);
    drain(*monitor);
    requireActionReflection(
        monitorCallbacks,
        firstRestoredReflection,
        *subjectMomObject,
        actionAttribute,
        reliable,
        initialAction);
    requestAction(initialAction);

    REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
    subjectJoined = false;
    REQUIRE_NOTHROW(monitor->resignFederationExecution(NO_ACTION));
    monitorJoined = false;
    REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
    federationCreated = false;
    REQUIRE_NOTHROW(subject->disconnect());
    subjectConnected = false;
    REQUIRE_NOTHROW(monitor->disconnect());
    monitorConnected = false;
  } catch (...) {
    if (subjectJoined) {
      try {
        subject->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (monitorJoined) {
      try {
        monitor->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (federationCreated) {
      try {
        subject->destroyFederationExecution(federationName);
      } catch (...) {
      }
    }
    if (subjectConnected) {
      try {
        subject->disconnect();
      } catch (...) {
      }
    }
    if (monitorConnected) {
      try {
        monitor->disconnect();
      } catch (...) {
      }
    }
    throw;
  }
}

}  // namespace

TEST_CASE(
    "Embedded MOM reflects the default automatic-resign action and its restoration",
    "[integration][development-profile][federation-management][mom]"
    "[automatic-resign-mom-reflection][2025]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.subscribe-object-class-attributes]"
    "[federate.callback.reflect-attribute-values]") {
  verifyAutomaticResignMomReflection(
      false, CANCEL_THEN_DELETE_THEN_DIVEST);
}

TEST_CASE(
    "Embedded MOM reflects the explicit FDD automatic-resign action and its restoration",
    "[integration][development-profile][federation-management][mom]"
    "[automatic-resign-mom-reflection][2025]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.subscribe-object-class-attributes]"
    "[federate.callback.reflect-attribute-values]") {
  verifyAutomaticResignMomReflection(
      true, DELETE_OBJECTS_THEN_DIVEST);
}
