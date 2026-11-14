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
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Completes a pending negotiated divestiture through Attribute Ownership Divestiture If Wanted. */
final class NegotiatedDivestitureIfWantedCompletionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private NegotiatedDivestitureIfWantedCompletionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador acquirer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder acquirerRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-negotiated-if-wanted-completion-" + suffix;
      boolean ownerConnected = false;
      boolean acquirerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean acquirerJoined = false;
      boolean ownerPublished = false;
      boolean acquirerPublished = false;
      boolean acquirerSubscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle acquirerClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet acquirerAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         acquirer.connect(acquirerRecorder.proxy(), callbackModel);
         acquirerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-negotiated-if-wanted-owner-" + suffix,
            federation);
         ownerJoined = true;
         acquirer.joinFederationExecution("java-tck-negotiated-if-wanted-acquirer-" + suffix,
            federation);
         acquirerJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         acquirerClass = acquirer.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(ownerClass != null && acquirerClass != null
               && ownerClass.equals(acquirerClass),
            "adapter FOM did not resolve the same negotiated-divestiture object class");
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle acquirerAttribute = acquirer.getAttributeHandle(
            acquirerClass, attributeName);
         JavaTckSupport.check(ownerAttribute != null && acquirerAttribute != null
               && ownerAttribute.equals(acquirerAttribute),
            "adapter FOM did not resolve the same negotiated-divestiture attribute");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         acquirerAttributes = JavaTckSupport.attributeSet(acquirer, acquirerAttribute);

         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         acquirer.publishObjectClassAttributes(acquirerClass, acquirerAttributes);
         acquirerPublished = true;
         acquirer.subscribeObjectClassAttributes(acquirerClass, acquirerAttributes);
         acquirerSubscribed = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "negotiated If Wanted registration returned no object");
         await(acquirer, callbackModel,
            () -> acquirerRecorder.discoveries.contains(object),
            "negotiated If Wanted object discovery");

         byte[] offerTag = {(byte) 0xd7, 0x41};
         byte[] acquisitionTag = {(byte) 0xa7, 0x41};
         byte[] divestitureTag = {(byte) 0xe7, 0x41};
         owner.negotiatedAttributeOwnershipDivestiture(object, ownerAttributes, offerTag);
         await(acquirer, callbackModel,
            () -> !acquirerRecorder.assumptions.isEmpty(),
            "negotiated divestiture ownership-assumption request");
         OwnershipEvent assumption = acquirerRecorder.assumptions.get(0);
         JavaTckSupport.check(object.equals(assumption.object)
               && assumption.attributes.equals(new HashSet<>(acquirerAttributes)),
            "negotiated divestiture did not identify the eligible acquirer");

         acquirer.attributeOwnershipAcquisition(object, acquirerAttributes, acquisitionTag);
         await(owner, callbackModel,
            () -> !ownerRecorder.confirmations.isEmpty(),
            "negotiated divestiture confirmation request before If Wanted");
         OwnershipEvent confirmation = ownerRecorder.confirmations.get(0);
         JavaTckSupport.check(object.equals(confirmation.object)
               && confirmation.attributes.equals(new HashSet<>(ownerAttributes))
               && Arrays.equals(acquisitionTag, confirmation.tag)
               && owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "negotiated divestiture was not pending before the alternate completion service");

         AttributeHandleSet divestedAttributes = owner.attributeOwnershipDivestitureIfWanted(
            object, ownerAttributes, divestitureTag);
         JavaTckSupport.check(divestedAttributes != null
               && new HashSet<>(divestedAttributes).equals(new HashSet<>(ownerAttributes)),
            "If Wanted did not return the exact set completing negotiated divestiture");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "If Wanted did not complete the pending negotiated ownership transfer");
         await(acquirer, callbackModel,
            () -> !acquirerRecorder.acquisitions.isEmpty(),
            "alternate negotiated-divestiture acquisition notification");
         OwnershipEvent acquisition = acquirerRecorder.acquisitions.get(0);
         JavaTckSupport.check(object.equals(acquisition.object)
               && acquisition.attributes.equals(new HashSet<>(acquirerAttributes))
               && Arrays.equals(divestitureTag, acquisition.tag),
            "If Wanted acquisition notification returned the wrong object, attributes, or tag");
      } finally {
         if (acquirerJoined && acquirerSubscribed && acquirerClass != null
               && acquirerAttributes != null) {
            try { acquirer.unsubscribeObjectClassAttributes(acquirerClass, acquirerAttributes); }
            catch (Exception ignored) { }
         }
         if (acquirerJoined && acquirerPublished && acquirerClass != null
               && acquirerAttributes != null) {
            try { acquirer.unpublishObjectClassAttributes(acquirerClass, acquirerAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && ownerPublished && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (acquirerJoined) {
            try { acquirer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(
               ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (acquirerConnected) { try { acquirer.disconnect(); } catch (Exception ignored) { } }
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

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final HashSet<AttributeHandle> attributes;
      private final byte[] tag;

      private OwnershipEvent(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         attributes = new HashSet<>((AttributeHandleSet) arguments[1]);
         tag = arguments[2] == null ? null : ((byte[]) arguments[2]).clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> assumptions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> confirmations = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();

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
                  assumptions.add(new OwnershipEvent(arguments));
               } else if ("requestDivestitureConfirmation".equals(name)
                     && arguments != null && arguments.length == 3) {
                  confirmations.add(new OwnershipEvent(arguments));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(new OwnershipEvent(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK negotiated If Wanted completion callbacks";
                  }
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
