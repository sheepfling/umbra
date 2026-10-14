package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
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
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Portable callback control for ownership-query result families. */
final class CallbackControlsOwnershipQueryTck {
   private static final String MOM_FEDERATE_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String MOM_FEDERATE_HANDLE = "HLAfederateHandle";

   private CallbackControlsOwnershipQueryTck() { }

   static void run(RtiFactory factory, String multiAttributeFom, String mim,
         String timeImplementation, String objectClassName, String firstAttributeName,
         String secondAttributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-callback-ownership-query-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecutionWithMIM(
            federation, new String[] {multiAttributeFom}, mim, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "java-tck-query-owner", "java-tck", federation);
         ownerJoined = true;
         FederateHandle requesterHandle = requester.joinFederationExecution(
            "java-tck-query-requester", "java-tck", federation);
         requesterJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerFirst = owner.getAttributeHandle(ownerClass, firstAttributeName);
         AttributeHandle ownerSecond = owner.getAttributeHandle(ownerClass, secondAttributeName);
         AttributeHandle requesterFirst =
            requester.getAttributeHandle(requesterClass, firstAttributeName);
         AttributeHandle requesterSecond =
            requester.getAttributeHandle(requesterClass, secondAttributeName);
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerFirst.equals(requesterFirst)
               && ownerSecond.equals(requesterSecond)
               && !ownerFirst.equals(ownerSecond),
            "standard ownership-query FOM lookups returned inconsistent handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerFirst);
         AttributeHandleSet requesterAttributes =
            JavaTckSupport.attributeSet(requester, requesterFirst, requesterSecond);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "ownership-query registration returned a null object handle");
         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.discoveries.contains(object),
            "requester did not discover the ownership-query object");

         requesterRecorder.clearOwnershipEvents();
         requester.disableCallbacks();
         requester.queryAttributeOwnership(object, requesterAttributes);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(requester.evokeMultipleCallbacks(0.0, 1.0),
               "evoked callback control did not admit pending ownership-query callbacks");
         } else {
            service(requester, callbackModel, objectClassName);
         }
         JavaTckSupport.check(requesterRecorder.ownershipEventCount() == 0,
            "disabled callbacks exposed ownership-query callbacks");

         requester.enableCallbacks();
         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.information().size() == 1
               && requesterRecorder.notOwned().size() == 1,
            "re-enabled ownership query did not return both owned and unowned subsets");
         OwnershipEvent information = requesterRecorder.information().get(0);
         OwnershipEvent notOwned = requesterRecorder.notOwned().get(0);
         JavaTckSupport.check(object.equals(information.object)
               && new HashSet<>(List.of(requesterFirst)).equals(information.attributes)
               && ownerHandle.equals(information.owner),
            "ownership query returned the wrong owned subset or owner");
         JavaTckSupport.check(object.equals(notOwned.object)
               && new HashSet<>(List.of(requesterSecond)).equals(notOwned.attributes),
            "ownership query returned the wrong unowned subset");
          JavaTckSupport.check(requesterRecorder.ownedByRti().isEmpty()
                && requesterRecorder.ownershipEventCount() == 2,
             "ordinary ownership query returned an unexpected RTI-owned callback");

         requester.setServiceReportingSwitch(false);
         ObjectClassHandle momClass = requester.getObjectClassHandle(MOM_FEDERATE_CLASS);
         AttributeHandle momHandleAttribute =
            requester.getAttributeHandle(momClass, MOM_FEDERATE_HANDLE);
         JavaTckSupport.check(momClass != null && momHandleAttribute != null,
            "standard joined-federate MOM ownership-query lookup returned a null handle");
          JavaTckSupport.check(MOM_FEDERATE_CLASS.equals(requester.getObjectClassName(momClass))
                && MOM_FEDERATE_HANDLE.equals(
                   requester.getAttributeName(momClass, momHandleAttribute)),
             "standard MIM ownership-query handles did not round-trip to their names");
         requester.subscribeObjectClassAttributes(
            momClass, JavaTckSupport.attributeSet(requester, momHandleAttribute));
         byte[] encodedRequester = new byte[requesterHandle.encodedLength()];
         requesterHandle.encode(encodedRequester, 0);
         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.momObjectForHandle(momHandleAttribute, encodedRequester)
               .isPresent(),
            "MOM did not reflect the joined federate's standard handle");
         ObjectInstanceHandle momObject = requesterRecorder
            .momObjectForHandle(momHandleAttribute, encodedRequester).get();
         JavaTckSupport.check(momClass.equals(requester.getKnownObjectClassHandle(momObject)),
            "joined-federate MOM reflection identified the wrong object class");

         requesterRecorder.clearOwnershipEvents();
         requester.disableCallbacks();
         requester.queryAttributeOwnership(
            momObject, JavaTckSupport.attributeSet(requester, momHandleAttribute));
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(requester.evokeMultipleCallbacks(0.0, 1.0),
               "evoked callback control did not admit the pending MOM ownership callback");
         } else {
            service(requester, callbackModel, objectClassName);
         }
         JavaTckSupport.check(requesterRecorder.ownershipEventCount() == 0,
            "disabled callbacks exposed the MOM RTI-owned query callback");

         requester.enableCallbacks();
         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.ownedByRti().size() == 1,
            "re-enabled MOM ownership query did not report RTI ownership");
         OwnershipEvent ownedByRti = requesterRecorder.ownedByRti().get(0);
         JavaTckSupport.check(momObject.equals(ownedByRti.object)
               && new HashSet<>(List.of(momHandleAttribute)).equals(ownedByRti.attributes)
               && requesterRecorder.information().isEmpty()
               && requesterRecorder.notOwned().isEmpty()
               && !requester.isAttributeOwnedByFederate(momObject, momHandleAttribute),
            "MOM ownership query returned the wrong RTI-owned subset");

         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         service(ambassador, model, objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 1.0);
      } else {
         ambassador.getObjectClassHandle(objectClassName);
      }
   }

   private interface Condition {
      boolean isTrue();
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final HashSet<AttributeHandle> attributes;
      private final FederateHandle owner;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            FederateHandle owner) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
         this.owner = owner;
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values) {
         this.object = object;
         this.values = new java.util.HashMap<>();
         for (AttributeHandle handle : values.keySet()) {
            this.values.put(handle, values.get(handle).clone());
         }
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> information = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> notOwned = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> ownedByRti = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reflectAttributeValues".equals(name)) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1]));
               } else if ("informAttributeOwnership".equals(name)) {
                  information.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (FederateHandle) arguments[2]));
               } else if ("attributeIsNotOwned".equals(name)) {
                  notOwned.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               } else if ("attributeIsOwnedByRTI".equals(name)) {
                  ownedByRti.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK ownership-query callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private List<OwnershipEvent> information() { return List.copyOf(information); }

      private List<OwnershipEvent> notOwned() { return List.copyOf(notOwned); }

      private List<OwnershipEvent> ownedByRti() { return List.copyOf(ownedByRti); }

      private int ownershipEventCount() {
         return information.size() + notOwned.size() + ownedByRti.size();
      }

      private Optional<ObjectInstanceHandle> momObjectForHandle(AttributeHandle attribute,
            byte[] expected) {
         return reflections.stream()
            .filter(reflection -> reflection.values.containsKey(attribute)
               && Arrays.equals(expected, reflection.values.get(attribute)))
            .map(reflection -> reflection.object)
            .findFirst();
      }

      private void clearOwnershipEvents() {
         information.clear();
         notOwned.clear();
         ownedByRti.clear();
      }
   }
}
