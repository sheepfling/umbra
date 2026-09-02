#include "internal/runtime/reference_2010.hpp"

#include "internal/handles/2010_handle_factories.hpp"

#include <algorithm>
#include <utility>

namespace rti1516e {
namespace umbra_binding_detail {
namespace {

struct AttributeDefinition final {
  std::uint64_t handle = 0;
  std::wstring name;
};

struct ObjectClassDefinition final {
  std::uint64_t handle = 0;
  std::wstring name;
  std::map<std::wstring, AttributeDefinition> attributesByName;
  std::map<std::uint64_t, std::wstring> attributeNamesByHandle;
};

struct ParameterDefinition final {
  std::uint64_t handle = 0;
  std::wstring name;
};

struct InteractionClassDefinition final {
  std::uint64_t handle = 0;
  std::wstring name;
  std::map<std::wstring, ParameterDefinition> parametersByName;
  std::map<std::uint64_t, std::wstring> parameterNamesByHandle;
};

struct SynchronizationPoint final {
  std::wstring label;
  VariableLengthData tag;
  std::set<std::uint64_t> members;
  std::set<std::uint64_t> achieved;
};

}  // namespace

struct ReferenceRuntime2010::Member final {
  void* session = nullptr;
  FederateAmbassador* callback = nullptr;
  FederateHandle handle;
  std::wstring name;
  std::wstring type;
  std::set<std::uint64_t> publishedClasses;
  std::map<std::uint64_t, std::set<std::uint64_t>> publishedAttributes;
  std::map<std::uint64_t, std::set<std::uint64_t>> subscribedAttributes;
  std::set<std::uint64_t> publishedInteractions;
  std::set<std::uint64_t> subscribedInteractions;
};

struct ReferenceRuntime2010::Object final {
  std::uint64_t handle = 0;
  std::uint64_t classHandle = 0;
  std::wstring name;
  std::uint64_t owner = 0;
  std::map<std::uint64_t, VariableLengthData> values;
};

struct ReferenceRuntime2010::Federation final {
  std::wstring name;
  std::map<std::uint64_t, ObjectClassDefinition> classesByHandle;
  std::map<std::wstring, std::uint64_t> classesByName;
  std::map<std::uint64_t, InteractionClassDefinition> interactionsByHandle;
  std::map<std::wstring, std::uint64_t> interactionsByName;
  std::map<std::wstring, SynchronizationPoint> synchronizationPoints;
  std::map<std::uint64_t, Member> members;
  std::map<void*, std::uint64_t> memberBySession;
  std::map<std::uint64_t, Object> objects;
};

ReferenceRuntime2010& ReferenceRuntime2010::instance() {
  static ReferenceRuntime2010 runtime;
  return runtime;
}

void ReferenceRuntime2010::createFederation(std::wstring const& name) {
  if (name.empty()) {
    throw IllegalName(L"The federation execution name must not be empty.");
  }
  if (federations_.find(name) != federations_.end()) {
    throw FederationExecutionAlreadyExists(name);
  }
  Federation federation;
  federation.name = name;
  federations_.emplace(name, std::move(federation));
}

void ReferenceRuntime2010::destroyFederation(std::wstring const& name) {
  auto found = federations_.find(name);
  if (found == federations_.end()) {
    throw FederationExecutionDoesNotExist(name);
  }
  if (!found->second.members.empty()) {
    throw FederatesCurrentlyJoined(name);
  }
  federations_.erase(found);
}

void ReferenceRuntime2010::listFederations(FederateAmbassador& recipient) {
  FederationExecutionInformationVector reports;
  for (auto const& entry : federations_) {
    reports.push_back(FederationExecutionInformation(entry.first, L""));
  }
  recipient.reportFederationExecutions(reports);
}

void ReferenceRuntime2010::registerFederationSynchronizationPoint(
    void* session, std::wstring const& label, VariableLengthData const& tag) {
  FederateHandleSet synchronizationSet;
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  for (auto const& entry : federation.members) synchronizationSet.insert(entry.second.handle);
  registerFederationSynchronizationPoint(session, label, tag, synchronizationSet);
}

void ReferenceRuntime2010::registerFederationSynchronizationPoint(
    void* session,
    std::wstring const& label,
    VariableLengthData const& tag,
    FederateHandleSet const& synchronizationSet) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  if (label.empty() || federation.synchronizationPoints.find(label) !=
      federation.synchronizationPoints.end()) {
    // IEEE 1516.1-2010 does not expose a dedicated duplicate-label
    // exception in the C++ API.  Keep the failure inside the operation's
    // declared exception set while still making the contract violation
    // explicit to callers.
    throw RTIinternalError(L"Synchronization point label is empty or not unique.");
  }
  SynchronizationPoint point;
  point.label = label;
  point.tag = tag;
  if (synchronizationSet.empty()) {
    for (auto const& entry : federation.members) point.members.insert(entry.first);
  } else {
    for (auto const& handle : synchronizationSet) {
      std::uint64_t const identity = value(handle);
      if (federation.members.find(identity) == federation.members.end()) {
        throw InvalidFederateHandle(L"Synchronization set contains an unknown federate.");
      }
      point.members.insert(identity);
    }
  }
  federation.synchronizationPoints.emplace(label, point);
  try {
    member.callback->synchronizationPointRegistrationSucceeded(label);
    for (auto const& entry : federation.members) {
      if (point.members.find(entry.first) == point.members.end() || entry.second.callback == nullptr) continue;
      entry.second.callback->announceSynchronizationPoint(label, tag);
    }
  } catch (FederateInternalError const&) {
    throw RTIinternalError(L"The 2010 reference callback rejected synchronization-point registration.");
  }
}

void ReferenceRuntime2010::synchronizationPointAchieved(
    void* session, std::wstring const& label, bool successfully) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  auto found = federation.synchronizationPoints.find(label);
  if (found == federation.synchronizationPoints.end()) {
    throw SynchronizationPointLabelNotAnnounced(label);
  }
  SynchronizationPoint& point = found->second;
  std::uint64_t const identity = value(member.handle);
  if (point.members.find(identity) == point.members.end()) {
    throw FederateNotExecutionMember(L"The federate is not in the synchronization set.");
  }
  if (successfully) point.achieved.insert(identity);
  if (!successfully || point.achieved.size() != point.members.size()) return;
  FederateHandleSet failed;
  try {
    for (auto const& entry : federation.members) {
      if (point.members.find(entry.first) != point.members.end() && entry.second.callback != nullptr) {
        entry.second.callback->federationSynchronized(label, failed);
      }
    }
  } catch (FederateInternalError const&) {
    throw RTIinternalError(L"The 2010 reference callback rejected federationSynchronized.");
  }
  federation.synchronizationPoints.erase(found);
}

ReferenceRuntime2010::Member& ReferenceRuntime2010::requireMember(void* session) {
  for (auto& federationEntry : federations_) {
    auto found = federationEntry.second.memberBySession.find(session);
    if (found != federationEntry.second.memberBySession.end()) {
      return federationEntry.second.members.at(found->second);
    }
  }
  throw FederateNotExecutionMember(L"The session is not a member of a federation execution.");
}

ReferenceRuntime2010::Federation& ReferenceRuntime2010::requireFederation(Member& member) {
  for (auto& federationEntry : federations_) {
    auto found = federationEntry.second.memberBySession.find(member.session);
    if (found != federationEntry.second.memberBySession.end() &&
        found->second == value(member.handle)) {
      return federationEntry.second;
    }
  }
  throw FederationExecutionDoesNotExist(L"The member's federation execution no longer exists.");
}

FederateHandle ReferenceRuntime2010::join(
    void* session,
    FederateAmbassador& callback,
    std::wstring const& federateName,
    std::wstring const& federateType,
    std::wstring const& federationName) {
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    throw FederationExecutionDoesNotExist(federationName);
  }
  for (auto const& member : federation->second.members) {
    if (member.second.name == federateName && !federateName.empty()) {
      throw FederateNameAlreadyInUse(federateName);
    }
  }
  if (federation->second.memberBySession.find(session) != federation->second.memberBySession.end()) {
    throw FederateAlreadyExecutionMember(federationName);
  }
  Member member;
  member.session = session;
  member.callback = &callback;
  member.handle = makeFederateHandle(nextFederateHandle_++);
  member.name = federateName;
  member.type = federateType;
  std::uint64_t const identity = value(member.handle);
  federation->second.memberBySession.emplace(session, identity);
  federation->second.members.emplace(identity, std::move(member));
  return makeFederateHandle(identity);
}

FederateHandle ReferenceRuntime2010::getFederateHandle(
    void* session, std::wstring const& name) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  for (auto const& entry : federation.members) {
    if (entry.second.name == name) return entry.second.handle;
  }
  throw NameNotFound(L"Unknown federate name.");
}

std::wstring ReferenceRuntime2010::getFederateName(
    void* session, FederateHandle const& handle) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  auto found = federation.members.find(value(handle));
  if (found == federation.members.end()) throw InvalidFederateHandle(L"Unknown federate handle.");
  return found->second.name;
}

void ReferenceRuntime2010::resign(void* session, ResignAction /*action*/) {
  for (auto& federationEntry : federations_) {
    auto sessionFound = federationEntry.second.memberBySession.find(session);
    if (sessionFound == federationEntry.second.memberBySession.end()) continue;
    auto memberFound = federationEntry.second.members.find(sessionFound->second);
    if (memberFound != federationEntry.second.members.end()) {
      for (auto object = federationEntry.second.objects.begin();
           object != federationEntry.second.objects.end();) {
        if (object->second.owner == sessionFound->second) {
          object = federationEntry.second.objects.erase(object);
        } else {
          ++object;
        }
      }
      federationEntry.second.members.erase(memberFound);
    }
    federationEntry.second.memberBySession.erase(sessionFound);
    return;
  }
  throw FederateNotExecutionMember(L"The session is not a member of a federation execution.");
}

void ReferenceRuntime2010::disconnect(void* session) {
  for (auto const& federationEntry : federations_) {
    if (federationEntry.second.memberBySession.find(session) != federationEntry.second.memberBySession.end()) {
      throw FederateIsExecutionMember(L"A federate must resign before disconnecting.");
    }
  }
}

ObjectClassHandle ReferenceRuntime2010::getObjectClassHandle(void* session, std::wstring const& name) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  auto found = federation.classesByName.find(name);
  if (found != federation.classesByName.end()) return makeObjectClassHandle(found->second);
  ObjectClassDefinition definition;
  definition.handle = nextObjectClassHandle_++;
  definition.name = name;
  federation.classesByName.emplace(name, definition.handle);
  federation.classesByHandle.emplace(definition.handle, std::move(definition));
  return makeObjectClassHandle(federation.classesByName.at(name));
}

std::wstring ReferenceRuntime2010::getObjectClassName(void* session, ObjectClassHandle const& handle) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  auto found = federation.classesByHandle.find(value(handle));
  if (found == federation.classesByHandle.end()) throw InvalidObjectClassHandle(L"Unknown object class handle.");
  return found->second.name;
}

AttributeHandle ReferenceRuntime2010::getAttributeHandle(
    void* session, ObjectClassHandle const& objectClass, std::wstring const& name) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  auto classFound = federation.classesByHandle.find(value(objectClass));
  if (classFound == federation.classesByHandle.end()) throw InvalidObjectClassHandle(L"Unknown object class handle.");
  auto found = classFound->second.attributesByName.find(name);
  if (found != classFound->second.attributesByName.end()) return makeAttributeHandle(found->second.handle);
  AttributeDefinition definition;
  definition.handle = nextAttributeHandle_++;
  definition.name = name;
  classFound->second.attributeNamesByHandle.emplace(definition.handle, name);
  classFound->second.attributesByName.emplace(name, definition);
  return makeAttributeHandle(definition.handle);
}

std::wstring ReferenceRuntime2010::getAttributeName(
    void* session, ObjectClassHandle const& objectClass, AttributeHandle const& handle) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  auto classFound = federation.classesByHandle.find(value(objectClass));
  if (classFound == federation.classesByHandle.end()) throw InvalidObjectClassHandle(L"Unknown object class handle.");
  auto found = classFound->second.attributeNamesByHandle.find(value(handle));
  if (found == classFound->second.attributeNamesByHandle.end()) throw InvalidAttributeHandle(L"Unknown attribute handle.");
  return found->second;
}

InteractionClassHandle ReferenceRuntime2010::getInteractionClassHandle(
    void* session, std::wstring const& name) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  if (name.empty()) throw NameNotFound(L"The interaction class name must not be empty.");
  auto found = federation.interactionsByName.find(name);
  if (found != federation.interactionsByName.end()) return makeInteractionClassHandle(found->second);
  InteractionClassDefinition definition;
  definition.handle = nextInteractionClassHandle_++;
  definition.name = name;
  federation.interactionsByName.emplace(name, definition.handle);
  federation.interactionsByHandle.emplace(definition.handle, std::move(definition));
  return makeInteractionClassHandle(federation.interactionsByName.at(name));
}

std::wstring ReferenceRuntime2010::getInteractionClassName(
    void* session, InteractionClassHandle const& handle) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  auto found = federation.interactionsByHandle.find(value(handle));
  if (found == federation.interactionsByHandle.end()) {
    throw InvalidInteractionClassHandle(L"Unknown interaction class handle.");
  }
  return found->second.name;
}

ParameterHandle ReferenceRuntime2010::getParameterHandle(
    void* session,
    InteractionClassHandle const& interactionClass,
    std::wstring const& name) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  auto classFound = federation.interactionsByHandle.find(value(interactionClass));
  if (classFound == federation.interactionsByHandle.end()) {
    throw InvalidInteractionClassHandle(L"Unknown interaction class handle.");
  }
  if (name.empty()) throw NameNotFound(L"The parameter name must not be empty.");
  auto found = classFound->second.parametersByName.find(name);
  if (found != classFound->second.parametersByName.end()) return makeParameterHandle(found->second.handle);
  ParameterDefinition definition;
  definition.handle = nextParameterHandle_++;
  definition.name = name;
  classFound->second.parameterNamesByHandle.emplace(definition.handle, name);
  classFound->second.parametersByName.emplace(name, definition);
  return makeParameterHandle(definition.handle);
}

std::wstring ReferenceRuntime2010::getParameterName(
    void* session,
    InteractionClassHandle const& interactionClass,
    ParameterHandle const& handle) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  auto classFound = federation.interactionsByHandle.find(value(interactionClass));
  if (classFound == federation.interactionsByHandle.end()) {
    throw InvalidInteractionClassHandle(L"Unknown interaction class handle.");
  }
  auto found = classFound->second.parameterNamesByHandle.find(value(handle));
  if (found == classFound->second.parameterNamesByHandle.end()) {
    throw InvalidParameterHandle(L"Unknown parameter handle.");
  }
  return found->second;
}

void ReferenceRuntime2010::publishInteractionClass(
    void* session, InteractionClassHandle const& interactionClass) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  if (federation.interactionsByHandle.find(value(interactionClass)) ==
      federation.interactionsByHandle.end()) {
    throw InteractionClassNotDefined(L"Unknown interaction class handle.");
  }
  member.publishedInteractions.insert(value(interactionClass));
}

void ReferenceRuntime2010::unpublishInteractionClass(
    void* session, InteractionClassHandle const& interactionClass) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  if (federation.interactionsByHandle.find(value(interactionClass)) ==
      federation.interactionsByHandle.end()) {
    throw InteractionClassNotDefined(L"Unknown interaction class handle.");
  }
  member.publishedInteractions.erase(value(interactionClass));
}

void ReferenceRuntime2010::subscribeInteractionClass(
    void* session, InteractionClassHandle const& interactionClass) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  if (federation.interactionsByHandle.find(value(interactionClass)) ==
      federation.interactionsByHandle.end()) {
    throw InteractionClassNotDefined(L"Unknown interaction class handle.");
  }
  member.subscribedInteractions.insert(value(interactionClass));
}

void ReferenceRuntime2010::unsubscribeInteractionClass(
    void* session, InteractionClassHandle const& interactionClass) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  if (federation.interactionsByHandle.find(value(interactionClass)) ==
      federation.interactionsByHandle.end()) {
    throw InteractionClassNotDefined(L"Unknown interaction class handle.");
  }
  member.subscribedInteractions.erase(value(interactionClass));
}

void ReferenceRuntime2010::publishObjectClassAttributes(
    void* session, ObjectClassHandle const& objectClass, AttributeHandleSet const& attributes) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  if (federation.classesByHandle.find(value(objectClass)) == federation.classesByHandle.end()) {
    throw ObjectClassNotDefined(L"Unknown object class handle.");
  }
  member.publishedClasses.insert(value(objectClass));
  auto& published = member.publishedAttributes[value(objectClass)];
  for (auto const& attribute : attributes) published.insert(value(attribute));
}

void ReferenceRuntime2010::unpublishObjectClass(void* session, ObjectClassHandle const& objectClass) {
  Member& member = requireMember(session);
  requireFederation(member);
  member.publishedClasses.erase(value(objectClass));
  member.publishedAttributes.erase(value(objectClass));
}

void ReferenceRuntime2010::unpublishObjectClassAttributes(
    void* session, ObjectClassHandle const& objectClass, AttributeHandleSet const& attributes) {
  Member& member = requireMember(session);
  requireFederation(member);
  auto& published = member.publishedAttributes[value(objectClass)];
  for (auto const& attribute : attributes) published.erase(value(attribute));
  if (published.empty()) member.publishedClasses.erase(value(objectClass));
}

void ReferenceRuntime2010::subscribeObjectClassAttributes(
    void* session, ObjectClassHandle const& objectClass, AttributeHandleSet const& attributes) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  if (federation.classesByHandle.find(value(objectClass)) == federation.classesByHandle.end()) {
    throw ObjectClassNotDefined(L"Unknown object class handle.");
  }
  auto& subscribed = member.subscribedAttributes[value(objectClass)];
  for (auto const& attribute : attributes) subscribed.insert(value(attribute));
}

void ReferenceRuntime2010::unsubscribeObjectClass(void* session, ObjectClassHandle const& objectClass) {
  Member& member = requireMember(session);
  requireFederation(member);
  member.subscribedAttributes.erase(value(objectClass));
}

void ReferenceRuntime2010::unsubscribeObjectClassAttributes(
    void* session, ObjectClassHandle const& objectClass, AttributeHandleSet const& attributes) {
  Member& member = requireMember(session);
  requireFederation(member);
  auto& subscribed = member.subscribedAttributes[value(objectClass)];
  for (auto const& attribute : attributes) subscribed.erase(value(attribute));
  if (subscribed.empty()) member.subscribedAttributes.erase(value(objectClass));
}

ObjectInstanceHandle ReferenceRuntime2010::registerObjectInstance(
    void* session, ObjectClassHandle const& objectClass, std::wstring const* name) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  std::uint64_t const classValue = value(objectClass);
  if (federation.classesByHandle.find(classValue) == federation.classesByHandle.end()) {
    throw ObjectClassNotDefined(L"Unknown object class handle.");
  }
  if (member.publishedClasses.find(classValue) == member.publishedClasses.end()) {
    throw ObjectClassNotPublished(L"The object class is not published by this federate.");
  }
  std::wstring objectName = name == nullptr ? generatedObjectName(nextObjectInstanceHandle_) : *name;
  for (auto const& object : federation.objects) {
    if (object.second.name == objectName) throw ObjectInstanceNameInUse(objectName);
  }
  Object object;
  object.handle = nextObjectInstanceHandle_++;
  object.classHandle = classValue;
  object.name = objectName;
  object.owner = value(member.handle);
  federation.objects.emplace(object.handle, object);
  ObjectInstanceHandle result = makeObjectInstanceHandle(object.handle);
  for (auto const& recipient : federation.members) {
    if (recipient.first == object.owner || recipient.second.callback == nullptr) continue;
    auto subscription = recipient.second.subscribedAttributes.find(classValue);
    if (subscription == recipient.second.subscribedAttributes.end()) continue;
    try {
      recipient.second.callback->discoverObjectInstance(
          result, objectClass, objectName, member.handle);
    } catch (FederateInternalError const&) {
      throw RTIinternalError(L"The 2010 reference callback rejected discoverObjectInstance.");
    }
  }
  return result;
}

std::wstring ReferenceRuntime2010::getObjectInstanceName(
    void* session, ObjectInstanceHandle const& objectHandle) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  auto found = federation.objects.find(value(objectHandle));
  if (found == federation.objects.end()) {
    throw ObjectInstanceNotKnown(L"Unknown object instance handle.");
  }
  return found->second.name;
}

void ReferenceRuntime2010::updateAttributeValues(
    void* session,
    ObjectInstanceHandle const& objectHandle,
    AttributeHandleValueMap const& values,
    VariableLengthData const& tag) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  auto objectFound = federation.objects.find(value(objectHandle));
  if (objectFound == federation.objects.end()) throw ObjectInstanceNotKnown(L"Unknown object instance handle.");
  Object& object = objectFound->second;
  if (object.owner != value(member.handle)) throw AttributeNotOwned(L"The federate does not own this object.");
  for (auto const& entry : values) object.values[value(entry.first)] = entry.second;
  AttributeHandleValueMap delivered;
  for (auto const& entry : values) delivered.emplace(entry.first, entry.second);
  for (auto const& recipient : federation.members) {
    if (recipient.first == object.owner || recipient.second.callback == nullptr) continue;
    auto subscription = recipient.second.subscribedAttributes.find(object.classHandle);
    if (subscription == recipient.second.subscribedAttributes.end()) continue;
    AttributeHandleValueMap filtered;
    for (auto const& entry : delivered) {
      if (subscription->second.find(value(entry.first)) != subscription->second.end()) {
        filtered.emplace(entry.first, entry.second);
      }
    }
    if (filtered.empty()) continue;
    try {
      recipient.second.callback->reflectAttributeValues(
          objectHandle, filtered, tag, RECEIVE, RELIABLE,
          SupplementalReflectInfo(member.handle));
    } catch (FederateInternalError const&) {
      throw RTIinternalError(L"The 2010 reference callback rejected reflectAttributeValues.");
    }
  }
}

void ReferenceRuntime2010::sendInteraction(
    void* session,
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& values,
    VariableLengthData const& tag) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  std::uint64_t const interactionValue = value(interactionClass);
  auto interactionFound = federation.interactionsByHandle.find(interactionValue);
  if (interactionFound == federation.interactionsByHandle.end()) {
    throw InteractionClassNotDefined(L"Unknown interaction class handle.");
  }
  if (member.publishedInteractions.find(interactionValue) == member.publishedInteractions.end()) {
    throw InteractionClassNotPublished(L"The interaction class is not published by this federate.");
  }
  for (auto const& entry : values) {
    if (interactionFound->second.parameterNamesByHandle.find(value(entry.first)) ==
        interactionFound->second.parameterNamesByHandle.end()) {
      throw InteractionParameterNotDefined(L"Unknown parameter handle.");
    }
  }
  for (auto const& recipient : federation.members) {
    if (recipient.first == value(member.handle) || recipient.second.callback == nullptr) continue;
    if (recipient.second.subscribedInteractions.find(interactionValue) ==
        recipient.second.subscribedInteractions.end()) continue;
    try {
      recipient.second.callback->receiveInteraction(
          interactionClass, values, tag, RECEIVE, RELIABLE,
          SupplementalReceiveInfo(member.handle));
    } catch (FederateInternalError const&) {
      throw RTIinternalError(L"The 2010 reference callback rejected receiveInteraction.");
    }
  }
}

void ReferenceRuntime2010::queryAttributeOwnership(
    void* session,
    ObjectInstanceHandle const& objectHandle,
    AttributeHandle const& attribute) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  auto objectFound = federation.objects.find(value(objectHandle));
  if (objectFound == federation.objects.end()) {
    throw ObjectInstanceNotKnown(L"Unknown object instance handle.");
  }
  auto classFound = federation.classesByHandle.find(objectFound->second.classHandle);
  if (classFound == federation.classesByHandle.end() ||
      classFound->second.attributeNamesByHandle.find(value(attribute)) ==
          classFound->second.attributeNamesByHandle.end()) {
    throw AttributeNotDefined(L"Unknown attribute handle.");
  }
  auto ownerFound = federation.members.find(objectFound->second.owner);
  if (ownerFound == federation.members.end() || ownerFound->second.callback == nullptr) {
    throw RTIinternalError(L"The owning federate is no longer available.");
  }
  try {
    member.callback->informAttributeOwnership(objectHandle, attribute, ownerFound->second.handle);
  } catch (FederateInternalError const&) {
    throw RTIinternalError(L"The 2010 reference callback rejected informAttributeOwnership.");
  }
}

bool ReferenceRuntime2010::isAttributeOwnedByFederate(
    void* session,
    ObjectInstanceHandle const& objectHandle,
    AttributeHandle const& attribute) {
  Member& member = requireMember(session);
  Federation& federation = requireFederation(member);
  auto objectFound = federation.objects.find(value(objectHandle));
  if (objectFound == federation.objects.end()) {
    throw ObjectInstanceNotKnown(L"Unknown object instance handle.");
  }
  auto classFound = federation.classesByHandle.find(objectFound->second.classHandle);
  if (classFound == federation.classesByHandle.end() ||
      classFound->second.attributeNamesByHandle.find(value(attribute)) ==
          classFound->second.attributeNamesByHandle.end()) {
    throw AttributeNotDefined(L"Unknown attribute handle.");
  }
  return objectFound->second.owner == value(member.handle);
}

std::uint64_t ReferenceRuntime2010::value(FederateHandle const& handle) {
  return umbra_binding_detail::FederateHandleValue(handle).second;
}
std::uint64_t ReferenceRuntime2010::value(ObjectClassHandle const& handle) {
  return umbra_binding_detail::ObjectClassHandleValue(handle).second;
}

std::uint64_t ReferenceRuntime2010::value(InteractionClassHandle const& handle) {
  return umbra_binding_detail::InteractionClassHandleValue(handle).second;
}
std::uint64_t ReferenceRuntime2010::value(ObjectInstanceHandle const& handle) {
  return umbra_binding_detail::ObjectInstanceHandleValue(handle).second;
}
std::uint64_t ReferenceRuntime2010::value(AttributeHandle const& handle) {
  return umbra_binding_detail::AttributeHandleValue(handle).second;
}

std::uint64_t ReferenceRuntime2010::value(ParameterHandle const& handle) {
  return umbra_binding_detail::ParameterHandleValue(handle).second;
}

std::wstring ReferenceRuntime2010::generatedObjectName(std::uint64_t value) {
  return L"Umbra2010Object-" + std::to_wstring(value);
}

}  // namespace umbra_binding_detail
}  // namespace rti1516e
