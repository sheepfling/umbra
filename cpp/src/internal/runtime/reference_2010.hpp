#pragma once

#include <RTI/RTI1516.h>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace rti1516e {
namespace umbra_binding_detail {

// A deliberately small provider-owned 1516.1-2010 semantic core.  It is an
// in-process reference transport for conformance tests, not a replacement for
// the production federation runtime.  FOM paths are accepted as standard
// inputs, while class/attribute lookup is represented by a deterministic
// provider directory until the 2010 FOM loader is added.
class ReferenceRuntime2010 final {
 public:
  static ReferenceRuntime2010& instance();

  void createFederation(std::wstring const& name);
  void destroyFederation(std::wstring const& name);
  void listFederations(FederateAmbassador& recipient);

  void registerFederationSynchronizationPoint(
      void* session, std::wstring const& label, VariableLengthData const& tag);
  void registerFederationSynchronizationPoint(
      void* session,
      std::wstring const& label,
      VariableLengthData const& tag,
      FederateHandleSet const& synchronizationSet);
  void synchronizationPointAchieved(void* session, std::wstring const& label, bool successfully);

  FederateHandle join(
      void* session,
      FederateAmbassador& callback,
      std::wstring const& federateName,
      std::wstring const& federateType,
      std::wstring const& federationName);
  FederateHandle getFederateHandle(void* session, std::wstring const& name);
  std::wstring getFederateName(void* session, FederateHandle const& handle);
  void resign(void* session, ResignAction action);
  void disconnect(void* session);

  ObjectClassHandle getObjectClassHandle(void* session, std::wstring const& name);
  std::wstring getObjectClassName(void* session, ObjectClassHandle const& handle);
  AttributeHandle getAttributeHandle(
      void* session, ObjectClassHandle const& objectClass, std::wstring const& name);
  std::wstring getAttributeName(
      void* session, ObjectClassHandle const& objectClass, AttributeHandle const& handle);

  InteractionClassHandle getInteractionClassHandle(void* session, std::wstring const& name);
  std::wstring getInteractionClassName(void* session, InteractionClassHandle const& handle);
  ParameterHandle getParameterHandle(
      void* session, InteractionClassHandle const& interactionClass, std::wstring const& name);
  std::wstring getParameterName(
      void* session, InteractionClassHandle const& interactionClass, ParameterHandle const& handle);

  void publishInteractionClass(void* session, InteractionClassHandle const& interactionClass);
  void unpublishInteractionClass(void* session, InteractionClassHandle const& interactionClass);
  void subscribeInteractionClass(void* session, InteractionClassHandle const& interactionClass);
  void unsubscribeInteractionClass(void* session, InteractionClassHandle const& interactionClass);

  void publishObjectClassAttributes(
      void* session, ObjectClassHandle const& objectClass, AttributeHandleSet const& attributes);
  void unpublishObjectClass(void* session, ObjectClassHandle const& objectClass);
  void unpublishObjectClassAttributes(
      void* session, ObjectClassHandle const& objectClass, AttributeHandleSet const& attributes);
  void subscribeObjectClassAttributes(
      void* session, ObjectClassHandle const& objectClass, AttributeHandleSet const& attributes);
  void unsubscribeObjectClass(void* session, ObjectClassHandle const& objectClass);
  void unsubscribeObjectClassAttributes(
      void* session, ObjectClassHandle const& objectClass, AttributeHandleSet const& attributes);

  ObjectInstanceHandle registerObjectInstance(
      void* session, ObjectClassHandle const& objectClass, std::wstring const* name);
  std::wstring getObjectInstanceName(void* session, ObjectInstanceHandle const& object);
  void updateAttributeValues(
      void* session,
      ObjectInstanceHandle const& object,
      AttributeHandleValueMap const& values,
      VariableLengthData const& tag);
  void sendInteraction(
      void* session,
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& values,
      VariableLengthData const& tag);
  void queryAttributeOwnership(
      void* session,
      ObjectInstanceHandle const& object,
      AttributeHandle const& attribute);
  bool isAttributeOwnedByFederate(
      void* session,
      ObjectInstanceHandle const& object,
      AttributeHandle const& attribute);

 private:
  struct Member;
  struct Object;
  struct Federation;

  Member& requireMember(void* session);
  Federation& requireFederation(Member& member);
  static std::uint64_t value(FederateHandle const& handle);
  static std::uint64_t value(ObjectClassHandle const& handle);
  static std::uint64_t value(InteractionClassHandle const& handle);
  static std::uint64_t value(ObjectInstanceHandle const& handle);
  static std::uint64_t value(AttributeHandle const& handle);
  static std::uint64_t value(ParameterHandle const& handle);
  static std::wstring generatedObjectName(std::uint64_t value);

  std::map<std::wstring, Federation> federations_;
  std::uint64_t nextFederateHandle_ = 1;
  std::uint64_t nextObjectClassHandle_ = 1;
  std::uint64_t nextAttributeHandle_ = 1;
  std::uint64_t nextInteractionClassHandle_ = 1;
  std::uint64_t nextParameterHandle_ = 1;
  std::uint64_t nextObjectInstanceHandle_ = 1;
};

}  // namespace umbra_binding_detail
}  // namespace rti1516e
