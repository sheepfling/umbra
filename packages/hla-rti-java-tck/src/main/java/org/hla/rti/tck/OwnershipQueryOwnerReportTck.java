package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Portable checks for the standard Inform Attribute Ownership query callback. */
final class OwnershipQueryOwnerReportTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OwnershipQueryOwnerReportTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-ownership-query-owner-report-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean observerJoined = false;
      boolean ownerPublished = false;
      boolean observerSubscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle observerClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet observerAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            "java-tck-ownership-query-owner", federation);
         ownerJoined = true;
         observer.joinFederationExecution("java-tck-ownership-query-observer", federation);
         observerJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         observerClass = observer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle observerAttribute = observer.getAttributeHandle(
            observerClass, attributeName);
         JavaTckSupport.check(ownerClass != null && observerClass != null
               && ownerAttribute != null && observerAttribute != null,
            "adapter FOM did not resolve the ownership-query class and attribute");
         AttributeHandleSet ownerAttributeSet = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet observerAttributeSet = JavaTckSupport.attributeSet(
            observer, observerAttribute);
         ownerAttributes = ownerAttributeSet;
         observerAttributes = observerAttributeSet;

         owner.publishObjectClassAttributes(ownerClass, ownerAttributeSet);
         ownerPublished = true;
         observer.subscribeObjectClassAttributes(observerClass, observerAttributeSet);
         observerSubscribed = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "ownership query registration returned a null object handle");
         await(observer, callbackModel, () -> observerRecorder.discoveries.contains(object),
            "ownership query object discovery");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !observer.isAttributeOwnedByFederate(object, observerAttribute),
            "ownership query setup did not establish the registering federate as owner");

         observerRecorder.ownershipInformation.clear();
         observer.queryAttributeOwnership(object, observerAttributeSet);
         await(observer, callbackModel,
            () -> !observerRecorder.ownershipInformation.isEmpty(),
            "Inform Attribute Ownership callback");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(observerRecorder.ownershipInformation.size() == 1,
            "ownership query produced duplicate Inform Attribute Ownership callbacks");
         OwnershipInformation result = observerRecorder.ownershipInformation.get(0);
         JavaTckSupport.check(object.equals(result.object)
               && observerAttributeSet.equals(result.attributes)
               && ownerFederate.equals(result.owner),
            "Inform Attribute Ownership returned the wrong object, attributes, or owner");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !observer.isAttributeOwnedByFederate(object, observerAttribute),
            "ownership query changed the ownership state it reported");
      } finally {
         if (observerJoined && observerSubscribed && observerClass != null
               && observerAttributes != null) {
            try { observer.unsubscribeObjectClassAttributes(observerClass, observerAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && ownerPublished && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (observerConnected) { try { observer.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
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

   private static final class OwnershipInformation {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final FederateHandle owner;

      private OwnershipInformation(ObjectInstanceHandle object, AttributeHandleSet attributes,
            FederateHandle owner) {
         this.object = object;
         this.attributes = attributes;
         this.owner = owner;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipInformation> ownershipInformation =
         new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("informAttributeOwnership".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  ownershipInformation.add(new OwnershipInformation(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandleSet) arguments[1],
                     (FederateHandle) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK ownership query callbacks";
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
