package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** P2.6 basic attribute ownership query, transfer, and denial flows. */
final class OwnershipTck {
   private OwnershipTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      checkOwnershipLifecycle(factory, callbackModel);
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador acquirer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder acquirerRecorder = new Recorder();
      String federation = "java-tck-ownership-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean acquirerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean acquirerJoined = false;
      ObjectInstanceHandle object = null;
      AttributeHandle ownerAttribute = null;
      AttributeHandle acquirerAttribute = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         acquirer.connect(acquirerRecorder.proxy(), callbackModel);
         acquirerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            "java-tck-ownership-owner", federation);
         ownerJoined = true;
         acquirer.joinFederationExecution("java-tck-ownership-acquirer", federation);
         acquirerJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle acquirerClass = acquirer.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         acquirerAttribute = acquirer.getAttributeHandle(acquirerClass, attributeName);
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet acquirerAttributes = JavaTckSupport.attributeSet(acquirer,
            acquirerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         acquirer.publishObjectClassAttributes(acquirerClass, acquirerAttributes);
         acquirer.subscribeObjectClassAttributes(acquirerClass, acquirerAttributes);
         object = owner.registerObjectInstance(ownerClass);
         drain(acquirer, callbackModel);
         JavaTckSupport.check(acquirerRecorder.discoveries.contains(object),
            "acquirer did not discover the ownership test object");
         ObjectInstanceHandle acquirerObject = object;

         ObjectInstanceHandle unknownObject = unknownObjectHandle(acquirer, object);
         AttributeHandle unknownAttribute = unknownAttributeHandle(acquirer, acquirerAttribute);
         AttributeHandle knownAcquirerAttribute = acquirerAttribute;
         AttributeHandleSet unknownAttributes =
            JavaTckSupport.attributeSet(acquirer, unknownAttribute);
         JavaTckSupport.expectFailure(
            () -> acquirer.queryAttributeOwnership(unknownObject, acquirerAttributes),
            "ObjectInstanceNotKnown", "ownership query for an unknown object");
         JavaTckSupport.expectFailure(
            () -> acquirer.queryAttributeOwnership(acquirerObject, unknownAttributes),
            "AttributeNotDefined", "ownership query for an undefined attribute");
         JavaTckSupport.expectFailure(
            () -> acquirer.isAttributeOwnedByFederate(unknownObject, knownAcquirerAttribute),
            "ObjectInstanceNotKnown", "ownership status query for an unknown object");
         JavaTckSupport.expectFailure(
            () -> acquirer.isAttributeOwnedByFederate(acquirerObject, unknownAttribute),
            "AttributeNotDefined", "ownership status query for an undefined attribute");
         byte[] emptyTag = new byte[0];
         JavaTckSupport.expectFailure(
            () -> owner.unconditionalAttributeOwnershipDivestiture(
               unknownObject, ownerAttributes, emptyTag),
            "ObjectInstanceNotKnown", "unconditional divestiture for an unknown object");
         JavaTckSupport.expectFailure(
            () -> owner.negotiatedAttributeOwnershipDivestiture(
               unknownObject, ownerAttributes, emptyTag),
            "ObjectInstanceNotKnown", "negotiated divestiture for an unknown object");
         JavaTckSupport.expectFailure(
            () -> owner.confirmDivestiture(unknownObject, ownerAttributes, emptyTag),
            "ObjectInstanceNotKnown", "divestiture confirmation for an unknown object");
         JavaTckSupport.expectFailure(
            () -> owner.cancelNegotiatedAttributeOwnershipDivestiture(
               unknownObject, ownerAttributes),
            "ObjectInstanceNotKnown", "negotiated divestiture cancellation for an unknown object");
         JavaTckSupport.expectFailure(
            () -> owner.attributeOwnershipDivestitureIfWanted(
               unknownObject, ownerAttributes, emptyTag),
            "ObjectInstanceNotKnown", "If Wanted divestiture for an unknown object");
         JavaTckSupport.expectFailure(
            () -> owner.attributeOwnershipReleaseDenied(
               unknownObject, ownerAttributes, emptyTag),
            "ObjectInstanceNotKnown", "ownership release denial for an unknown object");
         JavaTckSupport.expectFailure(
            () -> acquirer.attributeOwnershipAcquisition(
               unknownObject, acquirerAttributes, emptyTag),
            "ObjectInstanceNotKnown", "ownership acquisition for an unknown object");
         JavaTckSupport.expectFailure(
            () -> acquirer.attributeOwnershipAcquisitionIfAvailable(
               unknownObject, acquirerAttributes, emptyTag),
            "ObjectInstanceNotKnown", "If Available acquisition for an unknown object");
         JavaTckSupport.expectFailure(
            () -> acquirer.cancelAttributeOwnershipAcquisition(
               unknownObject, acquirerAttributes),
            "ObjectInstanceNotKnown", "acquisition cancellation for an unknown object");

         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "owner did not initially own the published attribute");
         JavaTckSupport.check(!acquirer.isAttributeOwnedByFederate(acquirerObject,
               acquirerAttribute), "acquirer initially reported ownership of the attribute");
         acquirer.queryAttributeOwnership(acquirerObject, acquirerAttributes);
         drain(acquirer, callbackModel);
         JavaTckSupport.check(acquirerRecorder.information.size() == 1,
            "ownership query did not produce one inform-ownership callback");
         OwnershipInformation information = acquirerRecorder.information.get(0);
         JavaTckSupport.check(acquirerObject.equals(information.object)
               && information.attributes.contains(acquirerAttribute)
               && ownerFederate.equals(information.owner),
            "ownership query did not identify the owning federate");

         // Negotiated acquisition exercises the request/reply callback pair.
         owner.negotiatedAttributeOwnershipDivestiture(object, ownerAttributes,
            new byte[] {0x10});
         acquirer.attributeOwnershipAcquisition(acquirerObject, acquirerAttributes,
            new byte[] {0x20});
         drain(owner, callbackModel);
         JavaTckSupport.check(ownerRecorder.divestitureRequests.size() == 1,
            "negotiated acquisition did not request divestiture confirmation");
         OwnershipEvent divestitureRequest = ownerRecorder.divestitureRequests.get(0);
         JavaTckSupport.check(object.equals(divestitureRequest.object)
               && divestitureRequest.attributes.size() == 1
               && divestitureRequest.attributes.contains(ownerAttribute)
               && Arrays.equals(new byte[] {0x20}, divestitureRequest.tag),
            "divestiture request did not preserve object, attribute set, and acquisition tag");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !acquirer.isAttributeOwnedByFederate(acquirerObject, acquirerAttribute)
               && acquirerRecorder.acquisitions.isEmpty(),
            "negotiated divestiture transferred ownership before confirmation");
         owner.confirmDivestiture(object, ownerAttributes, new byte[] {0x30});
         drain(acquirer, callbackModel);
         JavaTckSupport.check(acquirerRecorder.acquisitions.size() == 1,
            "confirmed ownership acquisition did not notify the acquirer");
         OwnershipEvent acquisition = acquirerRecorder.acquisitions.get(0);
         JavaTckSupport.check(object.equals(acquisition.object)
               && acquisition.attributes.size() == 1
               && acquisition.attributes.contains(acquirerAttribute)
               && Arrays.equals(new byte[] {0x30}, acquisition.tag),
            "ownership notification did not preserve object, attribute set, and confirmation tag");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "owner retained ownership after confirmed divestiture");
         JavaTckSupport.check(acquirer.isAttributeOwnedByFederate(acquirerObject,
               acquirerAttribute), "acquirer did not gain ownership after confirmation");

         AttributeHandleSet wanted = acquirer.attributeOwnershipDivestitureIfWanted(
            acquirerObject, acquirerAttributes, new byte[] {0x31});
         JavaTckSupport.check(wanted != null,
            "attributeOwnershipDivestitureIfWanted returned null");
         JavaTckSupport.check(wanted.size() == 0,
            "If Wanted divested an attribute without a pending acquisition");

         // If-available acquisition must report an attribute held by another
         // joined federate as unavailable without changing either owner's state.
         byte[] unavailableTag = {(byte) 0x32};
         owner.attributeOwnershipAcquisitionIfAvailable(object, ownerAttributes,
            unavailableTag);
         drain(owner, callbackModel);
         JavaTckSupport.check(ownerRecorder.unavailable.size() == 1
               && ownerRecorder.unavailable.get(0).object.equals(object)
               && ownerRecorder.unavailable.get(0).attributes.contains(ownerAttribute)
               && Arrays.equals(unavailableTag, ownerRecorder.unavailable.get(0).tag),
            "if-available acquisition of an owned attribute lacked the unavailable callback");
         JavaTckSupport.check(ownerRecorder.releaseRequests.isEmpty(),
            "If Available acquisition asked the owner to release an attribute");
         JavaTckSupport.check(acquirerRecorder.acquisitions.size() == 1,
            "If Available acquisition transferred an attribute held by another federate");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && acquirer.isAttributeOwnedByFederate(acquirerObject, acquirerAttribute),
            "unavailable if-available acquisition changed attribute ownership");

         // An unconditional release makes an immediately-available
         // acquisition complete without a negotiated callback.
         acquirer.unconditionalAttributeOwnershipDivestiture(acquirerObject,
            acquirerAttributes, new byte[] {0x40});
         drain(owner, callbackModel);
         JavaTckSupport.check(ownerRecorder.assumptions.size() == 1,
            "unconditional divestiture did not offer ownership to the owner");
         OwnershipEvent assumption = ownerRecorder.assumptions.get(0);
         JavaTckSupport.check(object.equals(assumption.object)
               && assumption.attributes.size() == 1
               && assumption.attributes.contains(ownerAttribute)
               && Arrays.equals(new byte[] {0x40}, assumption.tag),
            "ownership assumption did not preserve object, attributes, and divestiture tag");
         owner.queryAttributeOwnership(object, ownerAttributes);
         drain(owner, callbackModel);
         JavaTckSupport.check(ownerRecorder.notOwned.size() == 1,
            "ownership query for the unowned attribute did not report attributeIsNotOwned");
         OwnershipEvent notOwned = ownerRecorder.notOwned.get(0);
         JavaTckSupport.check(object.equals(notOwned.object)
               && notOwned.attributes.size() == 1
               && notOwned.attributes.contains(ownerAttribute),
            "attributeIsNotOwned returned the wrong object or attribute set");
         owner.attributeOwnershipAcquisitionIfAvailable(object, ownerAttributes,
            new byte[] {0x50});
         drain(owner, callbackModel);
         JavaTckSupport.check(ownerRecorder.acquisitions.size() == 1,
            "if-available acquisition did not notify the returning owner");
         OwnershipEvent returnAcquisition = ownerRecorder.acquisitions.get(0);
         JavaTckSupport.check(object.equals(returnAcquisition.object)
               && returnAcquisition.attributes.size() == 1
               && returnAcquisition.attributes.contains(ownerAttribute)
               && Arrays.equals(new byte[] {0x50}, returnAcquisition.tag),
            "If Available acquisition did not preserve object, attributes, and user tag");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "returning owner did not regain ownership");
         JavaTckSupport.check(!acquirer.isAttributeOwnedByFederate(acquirerObject,
               acquirerAttribute), "acquirer retained ownership after unconditional release");

         // The owner may deny a normal acquisition request after the release
         // callback.  The denial is delivered to the requester as an
         // unavailable callback with the owner's denial tag.
         acquirer.attributeOwnershipAcquisition(acquirerObject, acquirerAttributes,
            new byte[] {0x60});
         drain(owner, callbackModel);
         JavaTckSupport.check(ownerRecorder.releaseRequests.size() == 1,
            "normal acquisition did not request an ownership release");
         OwnershipEvent denialRequest = ownerRecorder.releaseRequests.get(0);
         JavaTckSupport.check(object.equals(denialRequest.object)
               && denialRequest.attributes.size() == 1
               && denialRequest.attributes.contains(ownerAttribute)
               && Arrays.equals(new byte[] {0x60}, denialRequest.tag),
            "ownership release request did not preserve object, attributes, and acquisition tag");
         owner.attributeOwnershipReleaseDenied(object, ownerAttributes, new byte[] {0x70});
         drain(acquirer, callbackModel);
         JavaTckSupport.check(acquirerRecorder.unavailable.size() == 1,
            "ownership release denial did not notify the requester");
         OwnershipEvent denial = acquirerRecorder.unavailable.get(0);
         JavaTckSupport.check(object.equals(denial.object)
               && denial.attributes.size() == 1
               && denial.attributes.contains(acquirerAttribute)
               && Arrays.equals(new byte[] {0x70}, denial.tag),
            "ownership unavailable callback did not preserve object, attributes, and denial tag");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "owner lost ownership after denying the acquisition");

         // A second pending request can be canceled through the public Java
         // service and acknowledged by its standard callback.
         acquirer.attributeOwnershipAcquisition(acquirerObject, acquirerAttributes,
            new byte[] {(byte) 0x80});
         drain(owner, callbackModel);
         JavaTckSupport.check(ownerRecorder.releaseRequests.size() == 2,
            "cancelable acquisition did not request an ownership release");
         OwnershipEvent cancellationRequest = ownerRecorder.releaseRequests.get(1);
         JavaTckSupport.check(object.equals(cancellationRequest.object)
               && cancellationRequest.attributes.size() == 1
               && cancellationRequest.attributes.contains(ownerAttribute)
               && Arrays.equals(new byte[] {(byte) 0x80}, cancellationRequest.tag),
            "cancelable acquisition release request returned the wrong object, attributes, or tag");
         acquirer.cancelAttributeOwnershipAcquisition(acquirerObject, acquirerAttributes);
         JavaTckSupport.drain(acquirer);
         JavaTckSupport.check(acquirerRecorder.cancellations.size() == 1,
            "ownership acquisition cancellation was not confirmed");
         OwnershipEvent cancellation = acquirerRecorder.cancellations.get(0);
         JavaTckSupport.check(object.equals(cancellation.object)
               && cancellation.attributes.size() == 1
               && cancellation.attributes.contains(acquirerAttribute),
            "ownership acquisition cancellation returned the wrong object or attributes");

         owner.negotiatedAttributeOwnershipDivestiture(object, ownerAttributes,
            new byte[] {(byte) 0x90});
         owner.cancelNegotiatedAttributeOwnershipDivestiture(object, ownerAttributes);
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "owner lost ownership after negotiated divestiture cancellation");

         // A pending acquisition is completed directly through the ordinary
         // If Wanted service, preserving the divestiture tag on notification.
         ownerRecorder.releaseRequests.clear();
         int previousAcquisitions = acquirerRecorder.acquisitions.size();
         byte[] ifWantedRequestTag = {(byte) 0xA0};
         byte[] ifWantedDivestitureTag = {(byte) 0xA1};
         acquirer.attributeOwnershipAcquisition(acquirerObject, acquirerAttributes,
            ifWantedRequestTag);
         drain(owner, callbackModel);
         JavaTckSupport.check(ownerRecorder.releaseRequests.size() == 1,
            "pending acquisition did not request an ownership release");
         OwnershipEvent ifWantedRelease = ownerRecorder.releaseRequests.get(0);
         JavaTckSupport.check(object.equals(ifWantedRelease.object)
               && ifWantedRelease.attributes.size() == 1
               && ifWantedRelease.attributes.contains(ownerAttribute)
               && Arrays.equals(ifWantedRequestTag, ifWantedRelease.tag),
            "pending acquisition release request returned the wrong object, attributes, or tag");
         AttributeHandleSet ifWantedDivested = owner.attributeOwnershipDivestitureIfWanted(
            object, ownerAttributes, ifWantedDivestitureTag);
         JavaTckSupport.check(ifWantedDivested != null && ifWantedDivested.size() == 1
               && ifWantedDivested.contains(ownerAttribute),
            "If Wanted did not return the transferred attribute");
         drain(acquirer, callbackModel);
         JavaTckSupport.check(acquirerRecorder.acquisitions.size() == previousAcquisitions + 1,
            "If Wanted did not notify the pending acquirer exactly once");
         OwnershipEvent ifWantedAcquisition =
            acquirerRecorder.acquisitions.get(acquirerRecorder.acquisitions.size() - 1);
         JavaTckSupport.check(object.equals(ifWantedAcquisition.object)
               && ifWantedAcquisition.attributes.size() == 1
               && ifWantedAcquisition.attributes.contains(acquirerAttribute)
               && Arrays.equals(ifWantedDivestitureTag, ifWantedAcquisition.tag),
            "If Wanted acquisition did not preserve object, attributes, and divestiture tag");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && acquirer.isAttributeOwnedByFederate(acquirerObject, acquirerAttribute),
            "If Wanted did not transfer ownership to the pending acquirer");
      } finally {
         if (object != null && acquirerJoined && acquirerAttribute != null) {
            try {
               if (acquirer.isAttributeOwnedByFederate(object, acquirerAttribute)) {
                  acquirer.unconditionalAttributeOwnershipDivestiture(object,
                     JavaTckSupport.attributeSet(acquirer, acquirerAttribute), new byte[] {0x7F});
               }
            } catch (Exception ignored) {
            }
         }
         if (acquirerJoined) resign(acquirer);
         if (ownerJoined) resign(owner);
         if (created) {
            try {
               owner.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (acquirerConnected) disconnect(acquirer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void checkOwnershipLifecycle(RtiFactory factory, CallbackModel callbackModel)
         throws Exception {
      RTIambassador lifecycle = factory.getRtiAmbassador();
      Recorder lifecycleRecorder = new Recorder();
      ObjectInstanceHandle invalidObject =
         JavaTckSupport.invalidHandle(ObjectInstanceHandle.class);
      AttributeHandle invalidAttribute = JavaTckSupport.invalidHandle(AttributeHandle.class);
      AttributeHandleSet emptyAttributes = JavaTckSupport.attributeSet(lifecycle);
      byte[] emptyTag = new byte[0];
      checkOwnershipLifecycle(lifecycle, invalidObject, invalidAttribute, emptyAttributes,
         emptyTag, "NotConnected", "before connect");
      try {
         lifecycle.connect(lifecycleRecorder.proxy(), callbackModel);
         checkOwnershipLifecycle(lifecycle, invalidObject, invalidAttribute, emptyAttributes,
            emptyTag, "FederateNotExecutionMember", "after connect before join");
      } finally {
         try { lifecycle.disconnect(); }
         catch (Exception ignored) { }
      }
   }

   private static void checkOwnershipLifecycle(RTIambassador ambassador,
         ObjectInstanceHandle invalidObject, AttributeHandle invalidAttribute,
         AttributeHandleSet emptyAttributes, byte[] tag, String expected, String phase)
         throws Exception {
      JavaTckSupport.expectFailure(
         () -> ambassador.unconditionalAttributeOwnershipDivestiture(
            invalidObject, emptyAttributes, tag), expected, phase + " unconditional divestiture");
      JavaTckSupport.expectFailure(
         () -> ambassador.negotiatedAttributeOwnershipDivestiture(
            invalidObject, emptyAttributes, tag), expected, phase + " negotiated divestiture");
      JavaTckSupport.expectFailure(
         () -> ambassador.confirmDivestiture(invalidObject, emptyAttributes, tag),
         expected, phase + " divestiture confirmation");
      JavaTckSupport.expectFailure(
         () -> ambassador.attributeOwnershipAcquisition(invalidObject, emptyAttributes, tag),
         expected, phase + " ownership acquisition");
      JavaTckSupport.expectFailure(
         () -> ambassador.attributeOwnershipAcquisitionIfAvailable(
            invalidObject, emptyAttributes, tag),
         expected, phase + " If Available ownership acquisition");
      JavaTckSupport.expectFailure(
         () -> ambassador.attributeOwnershipReleaseDenied(invalidObject, emptyAttributes, tag),
         expected, phase + " ownership release denial");
      JavaTckSupport.expectFailure(
         () -> ambassador.attributeOwnershipDivestitureIfWanted(
            invalidObject, emptyAttributes, tag),
         expected, phase + " If Wanted divestiture");
      JavaTckSupport.expectFailure(
         () -> ambassador.cancelNegotiatedAttributeOwnershipDivestiture(
            invalidObject, emptyAttributes),
         expected, phase + " negotiated divestiture cancellation");
      JavaTckSupport.expectFailure(
         () -> ambassador.cancelAttributeOwnershipAcquisition(invalidObject, emptyAttributes),
         expected, phase + " acquisition cancellation");
      JavaTckSupport.expectFailure(
         () -> ambassador.queryAttributeOwnership(invalidObject, emptyAttributes),
         expected, phase + " ownership query");
      JavaTckSupport.expectFailure(
         () -> ambassador.isAttributeOwnedByFederate(invalidObject, invalidAttribute),
         expected, phase + " ownership status query");
   }

   private static ObjectInstanceHandle unknownObjectHandle(RTIambassador ambassador,
         ObjectInstanceHandle knownObject) throws Exception {
      byte[] encoding = JavaTckSupport.encode(knownObject.encodedLength(), knownObject::encode);
      encoding[encoding.length - 1] ^= 0x01;
      ObjectInstanceHandle unknown =
         ambassador.getObjectInstanceHandleFactory().decode(encoding, 0);
      JavaTckSupport.check(!knownObject.equals(unknown),
         "derived unknown object handle equals the registered object");
      return unknown;
   }

   private static AttributeHandle unknownAttributeHandle(RTIambassador ambassador,
         AttributeHandle knownAttribute) throws Exception {
      byte[] encoding = JavaTckSupport.encode(
         knownAttribute.encodedLength(), knownAttribute::encode);
      encoding[encoding.length - 1] ^= 0x01;
      AttributeHandle unknown = ambassador.getAttributeHandleFactory().decode(encoding, 0);
      JavaTckSupport.check(!knownAttribute.equals(unknown),
         "derived undefined attribute handle equals the defined attribute");
      return unknown;
   }

   private static void drain(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.drain(ambassador);
      }
   }

   private static void resign(RTIambassador ambassador) {
      try {
         ambassador.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
      } catch (Exception ignored) {
      }
   }

   private static void disconnect(RTIambassador ambassador) {
      try {
         ambassador.disconnect();
      } catch (Exception ignored) {
      }
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

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      private final List<OwnershipInformation> information = new ArrayList<>();
      private final List<OwnershipEvent> divestitureRequests = new ArrayList<>();
      private final List<OwnershipEvent> releaseRequests = new ArrayList<>();
      private final List<OwnershipEvent> assumptions = new ArrayList<>();
      private final List<OwnershipEvent> notOwned = new ArrayList<>();
      private final List<OwnershipEvent> acquisitions = new ArrayList<>();
      private final List<OwnershipEvent> unavailable = new ArrayList<>();
      private final List<OwnershipEvent> cancellations = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("informAttributeOwnership".equals(name)) {
                  information.add(new OwnershipInformation((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (FederateHandle) arguments[2]));
               } else if ("requestDivestitureConfirmation".equals(name)) {
                  divestitureRequests.add(event(arguments));
               } else if ("requestAttributeOwnershipRelease".equals(name)) {
                  releaseRequests.add(event(arguments));
               } else if ("requestAttributeOwnershipAssumption".equals(name)) {
                  assumptions.add(event(arguments));
               } else if ("attributeIsNotOwned".equals(name)) {
                  notOwned.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)) {
                  acquisitions.add(event(arguments));
               } else if ("attributeOwnershipUnavailable".equals(name)) {
                  unavailable.add(event(arguments));
               } else if ("confirmAttributeOwnershipAcquisitionCancellation".equals(name)) {
                  cancellations.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], null));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK ownership callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private static OwnershipEvent event(Object[] arguments) {
         return new OwnershipEvent((ObjectInstanceHandle) arguments[0],
            (AttributeHandleSet) arguments[1], ((byte[]) arguments[2]).clone());
      }
   }
}
