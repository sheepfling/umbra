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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

final class SelectiveUnpublicationPendingAcquisitionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private SelectiveUnpublicationPendingAcquisitionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String pendingAttributeName,
         String unrelatedAttributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      String federation = "java-tck-selective-unpublication-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      boolean ownerPublished = false;
      boolean requesterPublished = false;
      boolean requesterSubscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle requesterClass = null;
      AttributeHandleSet ownerBoth = null;
      AttributeHandleSet requesterPublishedSet = null;
      AttributeHandleSet requesterBoth = null;
      try {
         owner.connect(callbacks(null), callbackModel);
         ownerConnected = true;
         requester.connect(callbacks(discoveries), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-selective-unpublication-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-selective-unpublication-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerPending = owner.getAttributeHandle(ownerClass, pendingAttributeName);
         AttributeHandle ownerUnrelated = owner.getAttributeHandle(
            ownerClass, unrelatedAttributeName);
         AttributeHandle requesterPending = requester.getAttributeHandle(
            requesterClass, pendingAttributeName);
         AttributeHandle requesterUnrelated = requester.getAttributeHandle(
            requesterClass, unrelatedAttributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerPending != null && ownerUnrelated != null
               && requesterPending != null && requesterUnrelated != null
               && !ownerPending.equals(ownerUnrelated)
               && !requesterPending.equals(requesterUnrelated),
            "adapter FOM did not resolve two distinct ownership attributes");

         ownerBoth = JavaTckSupport.attributeSet(owner, ownerPending, ownerUnrelated);
         requesterBoth = JavaTckSupport.attributeSet(
            requester, requesterPending, requesterUnrelated);
         AttributeHandleSet pendingOnly = JavaTckSupport.attributeSet(requester,
            requesterPending);
         AttributeHandleSet unrelatedOnly = JavaTckSupport.attributeSet(requester,
            requesterUnrelated);
         owner.publishObjectClassAttributes(ownerClass, ownerBoth);
         ownerPublished = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterBoth);
         requesterSubscribed = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "selective unpublication registration returned a null object handle");
         awaitDiscovery(requester, callbackModel, discoveries, object);

         requester.publishObjectClassAttributes(requesterClass, requesterBoth);
         requesterPublished = true;
         requesterPublishedSet = requesterBoth;
         requester.attributeOwnershipAcquisition(object, pendingOnly, new byte[0]);
         JavaTckSupport.check(!requester.isAttributeOwnedByFederate(object, requesterPending)
               && owner.isAttributeOwnedByFederate(object, ownerPending)
               && owner.isAttributeOwnedByFederate(object, ownerUnrelated),
            "pending acquisition unexpectedly changed ownership");

         requester.unpublishObjectClassAttributes(requesterClass, unrelatedOnly);
         requesterPublishedSet = pendingOnly;
         ObjectClassHandle requesterObjectClass = requesterClass;
         JavaTckSupport.expectFailure(
            () -> requester.attributeOwnershipAcquisition(object, unrelatedOnly, new byte[0]),
            "AttributeNotPublished", "acquiring the separately unpublished attribute");
         JavaTckSupport.expectFailure(
            () -> requester.unpublishObjectClassAttributes(requesterObjectClass, pendingOnly),
            "OwnershipAcquisitionPending", "unpublishing the pending acquisition attribute");
         JavaTckSupport.check(!requester.isAttributeOwnedByFederate(object, requesterPending)
               && owner.isAttributeOwnedByFederate(object, ownerPending)
               && owner.isAttributeOwnedByFederate(object, ownerUnrelated),
            "selective unpublication changed ownership of the pending attribute");
      } finally {
         if (requesterJoined && requesterPublished && requesterClass != null
               && requesterPublishedSet != null) {
            try { requester.unpublishObjectClassAttributes(requesterClass, requesterPublishedSet); }
            catch (Exception ignored) { }
         }
         if (requesterJoined && requesterSubscribed && requesterClass != null
               && requesterBoth != null) {
            try { requester.unsubscribeObjectClassAttributes(requesterClass, requesterBoth); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && ownerPublished && ownerClass != null && ownerBoth != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerBoth); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) { try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { } }
         if (requesterConnected) { try { requester.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static FederateAmbassador callbacks(List<ObjectInstanceHandle> discoveries) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (discoveries != null && "discoverObjectInstance".equals(method.getName())
                  && arguments != null && arguments.length >= 1) {
               discoveries.add((ObjectInstanceHandle) arguments[0]);
            }
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK selective unpublication";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static void awaitDiscovery(RTIambassador requester, CallbackModel model,
         List<ObjectInstanceHandle> discoveries, ObjectInstanceHandle object) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!discoveries.contains(object) && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(requester);
         else Thread.yield();
      }
      JavaTckSupport.check(discoveries.contains(object),
         "timed out waiting for selective-unpublication discovery");
   }
}
