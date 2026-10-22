package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks object registration service boundaries and local-delete identity cleanup. */
final class ObjectRegistrationServiceBoundariesTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private ObjectRegistrationServiceBoundariesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador lifecycle = factory.getRtiAmbassador();
      Recorder lifecycleCallbacks = new Recorder();
      String federation = "java-tck-object-registration-boundaries-" + UUID.randomUUID();
      boolean lifecycleConnected = false;
      RTIambassador owner = null;
      RTIambassador member = null;
      RTIambassador freshMember = null;
      Recorder ownerCallbacks = new Recorder();
      Recorder memberCallbacks = new Recorder();
      Recorder freshMemberCallbacks = new Recorder();
      boolean ownerConnected = false, memberConnected = false, created = false;
      boolean ownerJoined = false, memberJoined = false, published = false, subscribed = false;
      boolean freshMemberConnected = false, freshMemberJoined = false;
      boolean freshMemberSubscribed = false;
      ObjectClassHandle ownerClass = null, memberClass = null;
      AttributeHandle ownerAttribute = null, memberAttribute = null;
      try {
         String reservedName = "java-tck-lifecycle-reserved-name";
         java.util.Set<String> names = java.util.Set.of(reservedName);
         expectRegistrationLifecycle(lifecycle, names, "NotConnected",
            "before connect");
         lifecycle.connect(lifecycleCallbacks.proxy(), callbackModel);
         lifecycleConnected = true;
         expectRegistrationLifecycle(lifecycle, names,
            "FederateNotExecutionMember", "after connect before join");
         lifecycle.disconnect();
         lifecycleConnected = false;

         owner = factory.getRtiAmbassador();
         member = factory.getRtiAmbassador();
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberCallbacks.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         String ownerName = "java-tck-registration-boundary-owner";
         owner.joinFederationExecution(ownerName, "registration-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-registration-boundary-member",
            "registration-member", federation);
         memberJoined = true;
         RTIambassador joinedMember = member;

         RTIambassador joinedOwner = owner;
         ownerClass = owner.getObjectClassHandle(objectClassName);
         memberClass = member.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         JavaTckSupport.check(ownerClass != null && memberClass != null
               && ownerAttribute != null && memberAttribute != null
               && objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(member.getObjectClassName(memberClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute))
               && attributeName.equals(member.getAttributeName(memberClass, memberAttribute)),
            "adapter FOM did not resolve the registration-boundary object class and attribute");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet memberAttributes = JavaTckSupport.attributeSet(member, memberAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         member.subscribeObjectClassAttributes(memberClass, memberAttributes);
         subscribed = true;

         String unreservedName = federation + "-unreserved-name";
         ObjectClassHandle registeredClass = ownerClass;
         JavaTckSupport.expectFailure(() -> joinedOwner.registerObjectInstance(
            registeredClass, unreservedName),
            "ObjectInstanceNameNotReserved", "registering with an unreserved object name");
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "ordinary object registration returned an invalid handle");
         RTIambassador owningFederate = owner;
         JavaTckSupport.expectFailure(
            () -> owningFederate.localDeleteObjectInstance(object),
            "FederateOwnsAttributes", "locally deleting an object while owning its attributes");
         String objectName = owner.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty(),
            "ordinary object registration returned an empty name");

         await(member, callbackModel, () -> !memberCallbacks.discoveries.isEmpty(),
            "object discovery");
         Discovery discovered = memberCallbacks.discoveries.get(0);
         ObjectInstanceHandle memberObject = member.getObjectInstanceHandle(objectName);
         JavaTckSupport.check(memberObject.equals(discovered.object),
            "discovery returned the wrong local object handle");
         JavaTckSupport.check(memberClass.equals(discovered.objectClass),
            "discovery returned the wrong object class");
         JavaTckSupport.check(objectName.equals(discovered.name),
            "discovery returned the wrong object name");
         String discoveredProducerName = member.getFederateName(discovered.producer);
         JavaTckSupport.check(ownerName.equals(discoveredProducerName),
            "discovery returned producing federate " + discoveredProducerName
               + " rather than " + ownerName);
         JavaTckSupport.check(objectName.equals(member.getObjectInstanceName(memberObject)),
            "member object-name lookup did not round-trip");
         JavaTckSupport.check(memberClass.equals(member.getKnownObjectClassHandle(memberObject)),
            "member known-object-class lookup did not round-trip");

         member.publishObjectClassAttributes(memberClass, memberAttributes);
         member.attributeOwnershipAcquisition(memberObject, memberAttributes, new byte[] {0x51, 0x18});
         JavaTckSupport.expectFailure(() -> joinedMember.localDeleteObjectInstance(memberObject),
            "OwnershipAcquisitionPending", "locally deleting while ownership acquisition is pending");
         member.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         memberJoined = false;
         subscribed = false;

         freshMember = factory.getRtiAmbassador();
         freshMember.connect(freshMemberCallbacks.proxy(), callbackModel);
         freshMemberConnected = true;
         freshMember.joinFederationExecution("java-tck-registration-boundary-fresh-member",
            "registration-fresh-member", federation);
         freshMemberJoined = true;
         RTIambassador joinedFreshMember = freshMember;
         ObjectClassHandle freshClass = freshMember.getObjectClassHandle(objectClassName);
         AttributeHandle freshAttribute = freshMember.getAttributeHandle(freshClass, attributeName);
         JavaTckSupport.check(freshClass != null && freshAttribute != null,
            "fresh local-delete member could not resolve the adapter FOM handles");
         AttributeHandleSet freshAttributes = JavaTckSupport.attributeSet(
            freshMember, freshAttribute);
         freshMember.subscribeObjectClassAttributes(freshClass, freshAttributes);
         freshMemberSubscribed = true;
         await(freshMember, callbackModel,
            () -> !freshMemberCallbacks.discoveries.isEmpty(), "fresh-member discovery");
         ObjectInstanceHandle freshObject = freshMember.getObjectInstanceHandle(objectName);
         JavaTckSupport.check(freshObject.equals(freshMemberCallbacks.discoveries.get(0).object)
               && freshClass.equals(freshMemberCallbacks.discoveries.get(0).objectClass),
            "fresh-member discovery returned the wrong object identity");

         freshMember.localDeleteObjectInstance(freshObject);
         JavaTckSupport.check(freshMemberCallbacks.removals.isEmpty(),
            "local object deletion unexpectedly delivered a removal callback");
         JavaTckSupport.expectFailure(() -> joinedFreshMember.getKnownObjectClassHandle(freshObject),
            "ObjectInstanceNotKnown", "known-class lookup after local deletion");
         JavaTckSupport.expectFailure(() -> joinedFreshMember.getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown", "object-name lookup after local deletion");
         freshMemberCallbacks.discoveries.clear();
         freshMember.subscribeObjectClassAttributes(freshClass, freshAttributes);
         await(freshMember, callbackModel,
            () -> !freshMemberCallbacks.discoveries.isEmpty(), "rediscovery after local deletion");
         ObjectInstanceHandle rediscoveredObject = freshMember.getObjectInstanceHandle(objectName);
         JavaTckSupport.check(freshClass.equals(freshMember.getKnownObjectClassHandle(rediscoveredObject)),
            "rediscovery after local deletion did not restore known-object identity");

         freshMemberCallbacks.reflections.clear();
         byte[] value = new byte[] {0x04, 0x20};
         byte[] tag = new byte[] {0x7A};
         AttributeHandleValueMap update = owner.getAttributeHandleValueMapFactory().create(1);
         update.put(ownerAttribute, value.clone());
         owner.updateAttributeValues(object, update, tag.clone());
         await(freshMember, callbackModel,
            () -> !freshMemberCallbacks.reflections.isEmpty(), "post-local-delete reflection");
         JavaTckSupport.check(freshMemberCallbacks.reflections.size() == 1,
            "post-local-delete update did not produce exactly one reflection");
         Reflection reflection = freshMemberCallbacks.reflections.get(0);
         JavaTckSupport.check(rediscoveredObject.equals(reflection.object)
               && ownerName.equals(freshMember.getFederateName(reflection.producer))
               && Arrays.equals(tag, reflection.tag)
               && reflection.values.size() == 1
               && reflection.values.containsKey(freshAttribute)
               && Arrays.equals(value, reflection.values.get(freshAttribute)),
            "post-local-delete reflection returned the wrong object, value, tag, or producer");
      } finally {
         if (lifecycleConnected) {
            try { lifecycle.disconnect(); } catch (Exception ignored) { }
         }
         if (freshMemberJoined && freshMemberSubscribed) {
            try { freshMember.unsubscribeObjectClassAttributes(
               freshMember.getObjectClassHandle(objectClassName),
               JavaTckSupport.attributeSet(freshMember,
                  freshMember.getAttributeHandle(
                     freshMember.getObjectClassHandle(objectClassName), attributeName))); }
            catch (Exception ignored) { }
         }
         if (memberJoined && subscribed) {
            try { member.unsubscribeObjectClassAttributes(memberClass,
               JavaTckSupport.attributeSet(member, memberAttribute)); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && published) {
            try { owner.unpublishObjectClassAttributes(ownerClass,
               JavaTckSupport.attributeSet(owner, ownerAttribute)); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (freshMemberJoined) {
            try { freshMember.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created && owner != null) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (memberConnected && member != null) {
            try { member.disconnect(); } catch (Exception ignored) { }
         }
         if (freshMemberConnected && freshMember != null) {
            try { freshMember.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected && owner != null) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void expectRegistrationLifecycle(RTIambassador ambassador,
         java.util.Set<String> names, String expected,
         String phase) throws Exception {
      JavaTckSupport.expectFailure(() -> ambassador.reserveObjectInstanceName(
         names.iterator().next()), expected, "reserving an object name " + phase);
      JavaTckSupport.expectFailure(() -> ambassador.releaseObjectInstanceName(
         names.iterator().next()), expected, "releasing an object name " + phase);
      JavaTckSupport.expectFailure(() -> ambassador.reserveMultipleObjectInstanceNames(names),
         expected, "reserving multiple object names " + phase);
      JavaTckSupport.expectFailure(() -> ambassador.releaseMultipleObjectInstanceNames(names),
         expected, "releasing multiple object names " + phase);
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final String name;
      private final FederateHandle producer;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass,
            String name, FederateHandle producer) {
         this.object = object;
         this.objectClass = objectClass;
         this.name = name;
         this.producer = producer;
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final FederateHandle producer;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, FederateHandle producer) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.producer = producer;
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> removals = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length == 4) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (String) arguments[2],
                     (FederateHandle) arguments[3]));
               }
               if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  removals.add((ObjectInstanceHandle) arguments[0]);
               }
               if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (FederateHandle) arguments[4]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK registration-boundary callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
