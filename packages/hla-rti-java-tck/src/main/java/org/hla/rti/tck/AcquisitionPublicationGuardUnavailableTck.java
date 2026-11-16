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
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Keeps a requester's publication active until acquisition reaches its terminal callback. */
final class AcquisitionPublicationGuardUnavailableTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private AcquisitionPublicationGuardUnavailableTck() { }

   static void run(RtiFactory factory, String fom, String federateType,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String federation = "java-tck-acquisition-publication-guard-unavailable-"
         + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      boolean ownerPublished = false;
      boolean requesterPublished = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle requesterClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet requesterAttributes = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-publication-guard-owner",
            federateType, federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-publication-guard-requester",
            federateType, federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerAttribute != null && requesterAttribute != null
               && ownerClass.equals(requesterClass)
               && ownerAttribute.equals(requesterAttribute),
            "Acquisition publication-guard FOM did not resolve stable object handles");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requesterPublished = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "Acquisition publication-guard registration returned no object handle");
         await(requester, callbackModel,
            () -> requesterCallbacks.discoveries.contains(object),
            "acquisition publication-guard object discovery");

         ObjectInstanceHandle ifAvailableObject = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(ifAvailableObject != null && !ifAvailableObject.equals(object),
            "If-Available publication-guard registration returned an invalid object handle");
         await(requester, callbackModel,
            () -> requesterCallbacks.discoveries.contains(ifAvailableObject),
            "If-Available publication-guard object discovery");

         byte[] acquisitionTag = { (byte) 0xa5, 0x01 };
         byte[] denialTag = { (byte) 0xd3, 0x02 };
         byte[] divestitureTag = { (byte) 0xe7, 0x03 };
         ownerCallbacks.clearOwnershipEvents();
         requesterCallbacks.clearOwnershipEvents();
         requester.attributeOwnershipAcquisition(object, requesterAttributes, acquisitionTag);
         await(owner, callbackModel,
            () -> !ownerCallbacks.releaseRequests.isEmpty(),
            "acquisition publication-guard release request");
         checkEvent(ownerCallbacks.releaseRequests.get(0), object, ownerAttributes,
            acquisitionTag, "acquisition release request");
         expectPublicationPending(requester, requesterClass, requesterAttributes,
            "before ownership unavailable");

         owner.attributeOwnershipReleaseDenied(object, ownerAttributes, denialTag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            expectPublicationPending(requester, requesterClass, requesterAttributes,
               "before evoking ownership unavailable");
         }
         await(requester, callbackModel,
            () -> !requesterCallbacks.unavailable.isEmpty(),
            "acquisition publication-guard ownership unavailable callback");
         checkEvent(requesterCallbacks.unavailable.get(0), object, requesterAttributes,
            denialTag, "ownership unavailable callback");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "ownership unavailable changed attribute ownership");

         requester.unpublishObjectClassAttributes(requesterClass, requesterAttributes);
         requesterPublished = false;
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requesterPublished = true;
         requesterCallbacks.clearOwnershipEvents();
         ownerCallbacks.clearOwnershipEvents();
         requester.attributeOwnershipAcquisition(object, requesterAttributes, acquisitionTag);
         await(owner, callbackModel,
            () -> !ownerCallbacks.releaseRequests.isEmpty(),
            "second acquisition publication-guard release request");
         checkEvent(ownerCallbacks.releaseRequests.get(0), object, ownerAttributes,
            acquisitionTag, "second acquisition release request");
         expectPublicationPending(requester, requesterClass, requesterAttributes,
            "before acquisition notification");

         AttributeHandleSet divested = owner.attributeOwnershipDivestitureIfWanted(
            object, ownerAttributes, divestitureTag);
         JavaTckSupport.check(ownerAttributes.equals(divested)
               && !owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && requester.isAttributeOwnedByFederate(object, requesterAttribute),
            "If-Wanted divestiture did not transfer the requested attribute");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            expectPublicationPending(requester, requesterClass, requesterAttributes,
               "before evoking acquisition notification");
         }
         await(requester, callbackModel,
            () -> !requesterCallbacks.acquisitions.isEmpty(),
            "acquisition publication-guard acquisition notification");
         checkEvent(requesterCallbacks.acquisitions.get(0), object, requesterAttributes,
            divestitureTag, "acquisition notification");

         requesterCallbacks.clearOwnershipEvents();
         ownerCallbacks.clearOwnershipEvents();
         requester.attributeOwnershipAcquisitionIfAvailable(ifAvailableObject,
            requesterAttributes, acquisitionTag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(requesterCallbacks.unavailable.isEmpty(),
               "If-Available result was delivered before the evoked callback was serviced");
            expectPublicationPending(requester, requesterClass, requesterAttributes,
               "before If-Available ownership-unavailable callback");
         }
         await(requester, callbackModel,
            () -> !requesterCallbacks.unavailable.isEmpty(),
            "If-Available publication-guard unavailable callback");
         checkEvent(requesterCallbacks.unavailable.get(0), ifAvailableObject,
            requesterAttributes, acquisitionTag, "If-Available unavailable callback");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(ifAvailableObject, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(ifAvailableObject, requesterAttribute),
            "If-Available callback changed attribute ownership");

         requester.unpublishObjectClassAttributes(requesterClass, requesterAttributes);
         requesterPublished = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = false;

         requester.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (requesterJoined && requesterPublished && requesterClass != null
               && requesterAttributes != null) {
            try { requester.unpublishObjectClassAttributes(requesterClass, requesterAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && ownerPublished && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) resign(requester);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) disconnect(requester);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void expectPublicationPending(RTIambassador requester,
         ObjectClassHandle objectClass, AttributeHandleSet attributes, String phase)
         throws Exception {
      JavaTckSupport.expectFailure(
         () -> requester.unpublishObjectClassAttributes(objectClass, attributes),
         "OwnershipAcquisitionPending", "unpublishing " + phase);
   }

   private static void checkEvent(OwnershipEvent event, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object)
            && attributes.equals(event.attributes)
            && Arrays.equals(tag, event.tag),
         description + " returned the wrong object, attribute set, or tag");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final Set<AttributeHandle> attributes;
      private final byte[] tag;

      private OwnershipEvent(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         attributes = new HashSet<>((AttributeHandleSet) arguments[1]);
         tag = arguments[2] == null ? null : ((byte[]) arguments[2]).clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> unavailable = new CopyOnWriteArrayList<>();

      private void clearOwnershipEvents() {
         releaseRequests.clear();
         acquisitions.clear();
         unavailable.clear();
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length == 3) {
                  releaseRequests.add(new OwnershipEvent(arguments));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(new OwnershipEvent(arguments));
               } else if ("attributeOwnershipUnavailable".equals(name)
                     && arguments != null && arguments.length == 3) {
                  unavailable.add(new OwnershipEvent(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK ownership publication guard callbacks";
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
