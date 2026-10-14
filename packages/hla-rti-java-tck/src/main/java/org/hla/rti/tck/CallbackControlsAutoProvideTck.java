package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.HashSet;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Portable callback-enable gating for the standard Auto Provide facility. */
final class CallbackControlsAutoProvideTck {
   private CallbackControlsAutoProvideTck() { }

   static void runDisabledExplicitRequest(RtiFactory factory, String fom,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-auto-provide-disabled-request-" + UUID.randomUUID();
      boolean ownerConnected = false, requesterConnected = false, created = false;
      boolean ownerJoined = false, requesterJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-auto-provide-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-auto-provide-requester", federation);
         requesterJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(requesterClass, attributeName);
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerAttribute.equals(requesterAttribute),
            "standard FOM lookup returned inconsistent Auto Provide handles");
         JavaTckSupport.check(!owner.getAutoProvideSwitch() && !requester.getAutoProvideSwitch(),
            "adapter-supplied ordinary FOM unexpectedly enabled Auto Provide");

         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet requesterAttributes =
            JavaTckSupport.attributeSet(requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "disabled Auto Provide registration returned a null object handle");
         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.discoveries.contains(object),
            "discovery with Auto Provide disabled");
         JavaTckSupport.check(ownerRecorder.provided().isEmpty(),
            "disabled Auto Provide emitted a callback before an explicit request");

         byte[] objectTag = new byte[] {0x51, 0x52, 0x54};
         requester.requestAttributeValueUpdate(object, requesterAttributes, objectTag);
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.provided().size() == 1,
            "object-scoped explicit attribute-value request");
         checkProvided(ownerRecorder.provided(), object, ownerAttributes, objectTag,
            "object-scoped explicit request");

         ownerRecorder.clearProvided();
         byte[] classTag = new byte[] {0x43, 0x4c, 0x53};
         requester.requestAttributeValueUpdate(requesterClass, requesterAttributes, classTag);
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.provided().size() == 1,
            "class-scoped explicit attribute-value request");
         checkProvided(ownerRecorder.provided(), object, ownerAttributes, classTag,
            "class-scoped explicit request");

         requester.unsubscribeObjectClassAttributes(requesterClass, requesterAttributes);
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (requesterConnected) {
            try { requester.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runDisabledDiscoveryOnly(RtiFactory factory, String fom,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-auto-provide-disabled-discovery-" + UUID.randomUUID();
      boolean ownerConnected = false, requesterConnected = false, created = false;
      boolean ownerJoined = false, requesterJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-auto-provide-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-auto-provide-requester", federation);
         requesterJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(requesterClass, attributeName);
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerAttribute.equals(requesterAttribute),
            "standard FOM lookup returned inconsistent disabled Auto Provide handles");
         JavaTckSupport.check(!owner.getAutoProvideSwitch() && !requester.getAutoProvideSwitch(),
            "adapter-supplied ordinary FOM unexpectedly enabled Auto Provide");

         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet requesterAttributes =
            JavaTckSupport.attributeSet(requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "disabled Auto Provide registration returned a null object handle");

         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.discoveries.contains(object),
            "object discovery while Auto Provide is disabled");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            owner.evokeMultipleCallbacks(0.0, 1.0);
         } else {
            owner.getObjectClassHandle(objectClassName);
         }
         JavaTckSupport.check(ownerRecorder.provided().isEmpty(),
            "disabled Auto Provide generated a provider callback");
         List<DiscoveryEvent> discoveries = requesterRecorder.discoveryEvents();
         JavaTckSupport.check(discoveries.size() == 1
               && object.equals(discoveries.get(0).object)
               && requesterClass.equals(discoveries.get(0).objectClass),
            "disabled Auto Provide discovery returned the wrong object or class");

         requester.unsubscribeObjectClassAttributes(requesterClass, requesterAttributes);
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (requesterConnected) {
            try { requester.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void checkProvided(List<ProvidedUpdate> updates, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String phase) {
      JavaTckSupport.check(updates.size() == 1
            && object.equals(updates.get(0).object)
            && new HashSet<>(attributes).equals(updates.get(0).attributes)
            && java.util.Arrays.equals(tag, updates.get(0).tag),
         phase + " returned the wrong callback count, object, attributes, or tag");
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, timeImplementation, objectClassName, firstAttributeName,
         secondAttributeName, callbackModel, true);
   }

   static void runDelivery(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      run(factory, fom, timeImplementation, objectClassName, firstAttributeName,
         secondAttributeName, callbackModel, false);
   }

   private static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel, boolean gateCallbacks) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-callback-auto-provide-" + UUID.randomUUID();
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
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-auto-provide-owner", "owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution(
            "java-tck-auto-provide-requester", "requester", federation);
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
               && ownerFirst.equals(requesterFirst) && ownerSecond.equals(requesterSecond),
            "standard FOM lookups returned mismatched Auto Provide handles");
         JavaTckSupport.check(owner.getAutoProvideSwitch() && requester.getAutoProvideSwitch(),
            "adapter-supplied Auto Provide FOM did not enable the standard switch");

         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(
            owner, ownerFirst, ownerSecond);
         AttributeHandleSet requesterAttributes = JavaTckSupport.attributeSet(
            requester, requesterFirst, requesterSecond);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);

          ownerRecorder.clearProvided();
          if (gateCallbacks) owner.disableCallbacks();
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "Auto Provide registration returned a null object handle");
         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.discoveries.contains(object),
            "requester did not discover the Auto Provide object");

          if (gateCallbacks) {
             if (callbackModel == CallbackModel.HLA_EVOKED) {
                JavaTckSupport.check(owner.evokeMultipleCallbacks(0.0, 1.0),
                   "evoked callback control did not admit the pending Auto Provide callback");
             } else {
                service(owner, callbackModel, objectClassName);
             }
             JavaTckSupport.check(ownerRecorder.provided().isEmpty(),
                "disabled callbacks exposed a provideAttributeValueUpdate callback");
             owner.enableCallbacks();
          }
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.provided().size() == 1,
             "Auto Provide callback did not arrive");
         List<ProvidedUpdate> updates = ownerRecorder.provided();
         ProvidedUpdate update = updates.get(0);
         JavaTckSupport.check(updates.size() == 1
               && object.equals(update.object)
               && new HashSet<>(ownerAttributes).equals(update.attributes)
               && update.tag.length == 0,
            "re-enabled Auto Provide callback lost standard object, attribute, or tag data");

         requester.unsubscribeObjectClassAttributes(requesterClass, requesterAttributes);
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
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

   private static final class ProvidedUpdate {
      private final ObjectInstanceHandle object;
      private final HashSet<AttributeHandle> attributes;
      private final byte[] tag;

      private ProvidedUpdate(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
         this.tag = tag.clone();
      }
   }

   private static final class DiscoveryEvent {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;

      private DiscoveryEvent(ObjectInstanceHandle object, ObjectClassHandle objectClass) {
         this.object = object;
         this.objectClass = objectClass;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<DiscoveryEvent> discoveryEvents = new CopyOnWriteArrayList<>();
      private final List<ProvidedUpdate> provided = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
                   discoveryEvents.add(new DiscoveryEvent(
                      (ObjectInstanceHandle) arguments[0], (ObjectClassHandle) arguments[1]));
               } else if ("provideAttributeValueUpdate".equals(name)) {
                  provided.add(new ProvidedUpdate((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK Auto Provide callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private List<ProvidedUpdate> provided() {
         return List.copyOf(provided);
      }

      private List<DiscoveryEvent> discoveryEvents() {
         return List.copyOf(discoveryEvents);
      }

      private void clearProvided() {
         provided.clear();
      }
   }
}
