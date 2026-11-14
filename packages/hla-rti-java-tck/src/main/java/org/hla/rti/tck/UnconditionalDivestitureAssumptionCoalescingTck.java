package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks that unconditional divestiture offers one complete set to an eligible publisher. */
final class UnconditionalDivestitureAssumptionCoalescingTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private UnconditionalDivestitureAssumptionCoalescingTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador candidate = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder candidateRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-unconditional-assumption-coalescing-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean candidateConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean candidateJoined = false;
      boolean observerJoined = false;
      boolean ownerPublished = false;
      boolean candidatePublished = false;
      boolean candidateSubscribed = false;
      boolean observerSubscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle candidateClass = null;
      ObjectClassHandle observerClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet candidateAttributes = null;
      AttributeHandleSet observerAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         candidate.connect(candidateRecorder.proxy(), callbackModel);
         candidateConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;

         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-assumption-coalescing-owner", federation);
         ownerJoined = true;
         candidate.joinFederationExecution("java-tck-assumption-coalescing-candidate", federation);
         candidateJoined = true;
         observer.joinFederationExecution("java-tck-assumption-coalescing-observer", federation);
         observerJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         candidateClass = candidate.getObjectClassHandle(objectClassName);
         observerClass = observer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerFirst = owner.getAttributeHandle(ownerClass, firstAttributeName);
         AttributeHandle ownerSecond = owner.getAttributeHandle(ownerClass, secondAttributeName);
         AttributeHandle candidateFirst = candidate.getAttributeHandle(
            candidateClass, firstAttributeName);
         AttributeHandle candidateSecond = candidate.getAttributeHandle(
            candidateClass, secondAttributeName);
         AttributeHandle observerFirst = observer.getAttributeHandle(
            observerClass, firstAttributeName);
         AttributeHandle observerSecond = observer.getAttributeHandle(
            observerClass, secondAttributeName);
         JavaTckSupport.check(ownerClass != null && candidateClass != null
               && observerClass != null && ownerFirst != null && ownerSecond != null
               && candidateFirst != null && candidateSecond != null
               && observerFirst != null && observerSecond != null
               && !ownerFirst.equals(ownerSecond)
               && !candidateFirst.equals(candidateSecond)
               && !observerFirst.equals(observerSecond),
            "adapter FOM did not resolve two distinct ownership attributes");

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerFirst, ownerSecond);
         candidateAttributes = JavaTckSupport.attributeSet(candidate, candidateFirst,
            candidateSecond);
         observerAttributes = JavaTckSupport.attributeSet(observer, observerFirst, observerSecond);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         candidate.publishObjectClassAttributes(candidateClass, candidateAttributes);
         candidatePublished = true;
         candidate.subscribeObjectClassAttributes(candidateClass, candidateAttributes);
         candidateSubscribed = true;
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes);
         observerSubscribed = true;

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         awaitSessions(new RTIambassador[] {candidate, observer}, callbackModel,
            () -> candidateRecorder.discoveries.contains(object)
               && observerRecorder.discoveries.contains(object),
            "assumption-coalescing object discovery");

         byte[] tag = new byte[] {0x74, 0x43, 0x4f};
         owner.unconditionalAttributeOwnershipDivestiture(object, ownerAttributes, tag);
         await(candidate, callbackModel, () -> !candidateRecorder.assumptions.isEmpty(),
            "unconditional-divestiture assumption callback");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(owner);
            JavaTckSupport.drain(candidate);
            JavaTckSupport.drain(observer);
         }

         JavaTckSupport.check(candidateRecorder.assumptions.size() == 1,
            "eligible publisher did not receive exactly one assumption callback");
         checkEvent(candidateRecorder.assumptions.get(0), object, candidateAttributes, tag,
            "unconditional-divestiture assumption");
         JavaTckSupport.check(ownerRecorder.assumptions.isEmpty(),
            "divesting federate was asked to assume its own attributes");
         JavaTckSupport.check(observerRecorder.assumptions.isEmpty(),
            "subscribed non-publisher was offered ownership");
      } finally {
         unsubscribeIfJoined(candidate, candidateJoined, candidateSubscribed,
            candidateClass, candidateAttributes);
         unsubscribeIfJoined(observer, observerJoined, observerSubscribed,
            observerClass, observerAttributes);
         unpublishIfJoined(owner, ownerJoined, ownerPublished, ownerClass, ownerAttributes);
         unpublishIfJoined(candidate, candidateJoined, candidatePublished,
            candidateClass, candidateAttributes);
         resignIfJoined(candidate, candidateJoined);
         resignIfJoined(observer, observerJoined);
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) { try { observer.disconnect(); } catch (Exception ignored) { } }
         if (candidateConnected) { try { candidate.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void checkEvent(OwnershipEvent event, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object) && event.attributes.equals(attributes)
            && Arrays.equals(tag, event.tag),
         description + " returned the wrong object, attributes, or tag");
   }

   private static void unsubscribeIfJoined(RTIambassador ambassador, boolean joined,
         boolean subscribed, ObjectClassHandle objectClass, AttributeHandleSet attributes) {
      if (joined && subscribed) {
         try { ambassador.unsubscribeObjectClassAttributes(objectClass, attributes); }
         catch (Exception ignored) { }
      }
   }

   private static void unpublishIfJoined(RTIambassador ambassador, boolean joined,
         boolean published, ObjectClassHandle objectClass, AttributeHandleSet attributes) {
      if (joined && published) {
         try { ambassador.unpublishObjectClassAttributes(objectClass, attributes); }
         catch (Exception ignored) { }
      }
   }

   private static void resignIfJoined(RTIambassador ambassador, boolean joined) {
      if (joined) {
         try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
         catch (Exception ignored) { }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      awaitSessions(new RTIambassador[] {ambassador}, callbackModel, condition, description);
   }

   private static void awaitSessions(RTIambassador[] ambassadors, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            for (RTIambassador ambassador : ambassadors) JavaTckSupport.drain(ambassador);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag == null ? null : tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> assumptions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestAttributeOwnershipAssumption".equals(name)
                     && arguments != null && arguments.length == 3) {
                  assumptions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK assumption-coalescing callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
