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

/** Exercises unavailable and free-after-divestiture If Available ownership requests. */
final class OwnershipAcquisitionIfAvailableTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OwnershipAcquisitionIfAvailableTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String secondaryAttributeName,
         String foreignAttributeClassName, String foreignAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-ownership-if-available-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      boolean ownerPublished = false;
      boolean requesterPublished = false;
      boolean requesterSecondaryPublished = false;
      boolean requesterSubscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle requesterClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet requesterAttributes = null;
      AttributeHandleSet requesterSecondaryAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-ifavailable-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-ifavailable-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle ownerSecondaryAttribute = owner.getAttributeHandle(
            ownerClass, secondaryAttributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
            requesterClass, attributeName);
         AttributeHandle requesterSecondaryAttribute = requester.getAttributeHandle(
            requesterClass, secondaryAttributeName);
         JavaTckSupport.check(ownerClass != null && requesterClass != null
               && ownerAttribute != null && ownerSecondaryAttribute != null
               && requesterAttribute != null
               && requesterSecondaryAttribute != null
               && !requesterSecondaryAttribute.equals(requesterAttribute),
            "adapter FOM did not resolve the If Available ownership handles");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);
         final AttributeHandleSet requestedAttributes = requesterAttributes;
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requesterPublished = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         requesterSubscribed = true;

         ObjectInstanceHandle unavailableObject = owner.registerObjectInstance(ownerClass);
         ObjectInstanceHandle availableObject = owner.registerObjectInstance(ownerClass);
         await(requester, callbackModel,
            () -> requesterRecorder.discoveries.contains(unavailableObject)
               && requesterRecorder.discoveries.contains(availableObject),
            "If Available ownership object discoveries");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(unavailableObject, ownerAttribute)
               && owner.isAttributeOwnedByFederate(availableObject, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(unavailableObject, requesterAttribute)
               && !requester.isAttributeOwnedByFederate(availableObject, requesterAttribute),
            "ownership setup did not leave both attributes with their registering federate");

         ObjectInstanceHandle unknownObject = owner.registerObjectInstance(ownerClass);
         await(requester, callbackModel,
            () -> requesterRecorder.discoveries.contains(unknownObject),
            "If Available local-delete fixture discovery");
         requester.localDeleteObjectInstance(unknownObject);
         AttributeHandle undefinedAttribute = requester.getAttributeHandle(
            requester.getObjectClassHandle(foreignAttributeClassName),
            foreignAttributeName);
         JavaTckSupport.check(!undefinedAttribute.equals(requesterAttribute)
               && !undefinedAttribute.equals(requesterSecondaryAttribute),
            "adapter foreign attribute must not belong to the tested object class");
         AttributeHandleSet undefinedAttributes =
            JavaTckSupport.attributeSet(requester, undefinedAttribute);
         byte[] preconditionTag = new byte[0];
         AttributeHandleSet unpublishedAttributes =
            JavaTckSupport.attributeSet(requester, requesterSecondaryAttribute);
         JavaTckSupport.expectFailure(
            () -> requester.attributeOwnershipAcquisitionIfAvailable(
               unavailableObject, unpublishedAttributes, preconditionTag),
            "AttributeNotPublished", "If Available request for an unpublished attribute");
         JavaTckSupport.expectFailure(
            () -> requester.attributeOwnershipAcquisitionIfAvailable(
               unknownObject, requestedAttributes, preconditionTag),
            "ObjectInstanceNotKnown", "If Available request for an unknown object instance");
         JavaTckSupport.expectFailure(
            () -> requester.attributeOwnershipAcquisitionIfAvailable(
               unavailableObject, undefinedAttributes, preconditionTag),
            "AttributeNotDefined", "If Available request for an undefined attribute");
         requester.unpublishObjectClassAttributes(requesterClass, requesterAttributes);
         requesterPublished = false;
         JavaTckSupport.expectFailure(
            () -> requester.attributeOwnershipAcquisitionIfAvailable(
               unavailableObject, requestedAttributes, preconditionTag),
            "ObjectClassNotPublished", "If Available request without class publication");
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requesterPublished = true;

         byte[] unavailableTag = new byte[] {0x51, (byte) 0xa7, 0x0c};
         ownerRecorder.clearOwnershipEvents();
         requesterRecorder.clearOwnershipEvents();
         requester.attributeOwnershipAcquisitionIfAvailable(
            unavailableObject, requesterAttributes, unavailableTag);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            requester.attributeOwnershipAcquisitionIfAvailable(
               unavailableObject, requesterAttributes, unavailableTag);
         }
         await(requester, callbackModel, () -> !requesterRecorder.unavailable.isEmpty(),
            "If Available unavailable callback");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(requester);
         }
         JavaTckSupport.check(requesterRecorder.unavailable.size() == 1,
            "If Available request produced duplicate unavailable callbacks");
         checkEvent(requesterRecorder.unavailable.get(0), unavailableObject,
            requesterAttributes, unavailableTag, "If Available unavailable callback");
         JavaTckSupport.check(requesterRecorder.acquisitions.isEmpty()
               && owner.isAttributeOwnedByFederate(unavailableObject, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(unavailableObject, requesterAttribute),
            "unavailable If Available request changed ownership");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.releaseRequests.isEmpty(),
            "unavailable If Available request asked the owner to release its attribute");

         ownerRecorder.clearOwnershipEvents();
         requesterRecorder.clearOwnershipEvents();
         owner.unconditionalAttributeOwnershipDivestiture(
            availableObject, ownerAttributes, new byte[0]);
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(availableObject, ownerAttribute)
               && !requester.isAttributeOwnedByFederate(availableObject, requesterAttribute),
            "unconditional divestiture transferred ownership without a requester");

         byte[] acquisitionTag = new byte[] {0x3c, (byte) 0xa1, 0x7e};
         requester.attributeOwnershipAcquisitionIfAvailable(
            availableObject, requesterAttributes, acquisitionTag);
         await(requester, callbackModel, () -> !requesterRecorder.acquisitions.isEmpty(),
            "If Available acquisition notification");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(requester);
         JavaTckSupport.check(requesterRecorder.acquisitions.size() == 1,
            "If Available request produced duplicate acquisition notifications");
         checkEvent(requesterRecorder.acquisitions.get(0), availableObject,
            requesterAttributes, acquisitionTag, "If Available acquisition notification");
         JavaTckSupport.check(requesterRecorder.unavailable.isEmpty()
               && !owner.isAttributeOwnedByFederate(availableObject, ownerAttribute)
               && requester.isAttributeOwnedByFederate(availableObject, requesterAttribute),
            "If Available acquisition did not transfer the unowned attribute");
         JavaTckSupport.expectFailure(
            () -> requester.attributeOwnershipAcquisitionIfAvailable(
               availableObject, requestedAttributes, acquisitionTag),
            "FederateOwnsAttributes",
            "repeating If Available after acquiring ownership");

         requester.unconditionalAttributeOwnershipDivestiture(
            availableObject, requesterAttributes, new byte[0]);
         JavaTckSupport.check(!requester.isAttributeOwnedByFederate(
               availableObject, requesterAttribute),
            "If Available ownership cleanup did not divest the acquired attribute");

         if (callbackModel == CallbackModel.HLA_EVOKED) {
            requesterSecondaryAttributes = JavaTckSupport.attributeSet(
               requester, requesterSecondaryAttribute);
            AttributeHandleSet requesterBoth = JavaTckSupport.attributeSet(
               requester, requesterAttribute, requesterSecondaryAttribute);
            requester.publishObjectClassAttributes(requesterClass, requesterBoth);
            requesterSecondaryPublished = true;
            ObjectInstanceHandle mixedObject = owner.registerObjectInstance(ownerClass);
            await(requester, callbackModel,
               () -> requesterRecorder.discoveries.contains(mixedObject),
               "If Available mixed-repeat object discovery");
            owner.unconditionalAttributeOwnershipDivestiture(
               mixedObject, ownerAttributes, new byte[0]);
            JavaTckSupport.check(!owner.isAttributeOwnedByFederate(
                  mixedObject, ownerAttribute)
                  && !owner.isAttributeOwnedByFederate(
                     mixedObject, ownerSecondaryAttribute),
               "If Available mixed-repeat setup did not leave both attributes unowned");

            requesterRecorder.clearOwnershipEvents();
            requester.attributeOwnershipAcquisitionIfAvailable(
               mixedObject, requesterAttributes, acquisitionTag);
            requester.attributeOwnershipAcquisitionIfAvailable(
               mixedObject, requesterBoth, acquisitionTag);
            await(requester, callbackModel,
               () -> requesterRecorder.acquisitions.size() >= 2
                  || (requesterRecorder.acquisitions.size() == 1
                     && requesterBoth.equals(requesterRecorder.acquisitions.get(0).attributes)),
               "If Available mixed-repeat acquisition notifications");
            JavaTckSupport.drain(requester);
            JavaTckSupport.check(requesterRecorder.unavailable.isEmpty()
                  && requesterRecorder.acquisitions.size() <= 2,
               "If Available mixed repeat produced unavailable or duplicate notifications");
            if (requesterRecorder.acquisitions.size() == 1) {
               checkEvent(requesterRecorder.acquisitions.get(0), mixedObject,
                  requesterBoth, acquisitionTag, "If Available combined mixed-repeat callback");
            } else {
               OwnershipEvent first = requesterRecorder.acquisitions.get(0);
               OwnershipEvent second = requesterRecorder.acquisitions.get(1);
               JavaTckSupport.check(first.attributes.equals(requesterAttributes)
                     || first.attributes.equals(requesterSecondaryAttributes),
                  "If Available mixed-repeat first callback did not identify one attribute");
               checkEvent(first, mixedObject, first.attributes.equals(requesterAttributes)
                     ? requesterAttributes : requesterSecondaryAttributes,
                  acquisitionTag, "If Available mixed-repeat first callback");
               checkEvent(second, mixedObject, first.attributes.equals(requesterAttributes)
                     ? requesterSecondaryAttributes : requesterAttributes,
                  acquisitionTag, "If Available mixed-repeat second callback");
            }
            JavaTckSupport.check(requester.isAttributeOwnedByFederate(
                  mixedObject, requesterAttribute)
                  && requester.isAttributeOwnedByFederate(
                     mixedObject, requesterSecondaryAttribute),
               "If Available mixed repeat did not acquire both eligible attributes");
            requester.unconditionalAttributeOwnershipDivestiture(
               mixedObject, requesterBoth, new byte[0]);
         }
      } finally {
         if (requesterJoined && requesterSubscribed && requesterClass != null
               && requesterAttributes != null) {
            try { requester.unsubscribeObjectClassAttributes(requesterClass, requesterAttributes); }
            catch (Exception ignored) { }
         }
         if (requesterJoined && requesterPublished && requesterClass != null
               && requesterAttributes != null) {
            try { requester.unpublishObjectClassAttributes(requesterClass, requesterAttributes); }
            catch (Exception ignored) { }
         }
         if (requesterJoined && requesterSecondaryPublished && requesterClass != null
               && requesterSecondaryAttributes != null) {
            try { requester.unpublishObjectClassAttributes(
               requesterClass, requesterSecondaryAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && ownerPublished && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
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
         if (requesterConnected) { try { requester.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void checkEvent(OwnershipEvent event, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object) && attributes.equals(event.attributes)
            && Arrays.equals(tag, event.tag),
         description + " returned the wrong object, attributes, or tag");
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
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> unavailable = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(event(arguments));
               } else if ("attributeOwnershipUnavailable".equals(name)
                     && arguments != null && arguments.length == 3) {
                  unavailable.add(event(arguments));
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length == 3) {
                  releaseRequests.add(event(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK If Available ownership callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private void clearOwnershipEvents() {
         acquisitions.clear();
         unavailable.clear();
         releaseRequests.clear();
      }

      private static OwnershipEvent event(Object[] arguments) {
         return new OwnershipEvent((ObjectInstanceHandle) arguments[0],
            (AttributeHandleSet) arguments[1], (byte[]) arguments[2]);
      }
   }
}
