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
         String objectClassName, String attributeName) throws Exception {
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
         owner.connect(ownerRecorder.proxy(), CallbackModel.HLA_EVOKED);
         ownerConnected = true;
         acquirer.connect(acquirerRecorder.proxy(), CallbackModel.HLA_EVOKED);
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
         JavaTckSupport.drain(acquirer);
         JavaTckSupport.check(acquirerRecorder.discoveries.contains(object),
            "acquirer did not discover the ownership test object");
         ObjectInstanceHandle acquirerObject = object;

         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "owner did not initially own the published attribute");
         JavaTckSupport.check(!acquirer.isAttributeOwnedByFederate(acquirerObject,
               acquirerAttribute), "acquirer initially reported ownership of the attribute");
         acquirer.queryAttributeOwnership(acquirerObject, acquirerAttributes);
         JavaTckSupport.drain(acquirer);
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
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.divestitureRequests.size() == 1,
            "negotiated acquisition did not request divestiture confirmation");
         JavaTckSupport.check(Arrays.equals(new byte[] {0x20},
               ownerRecorder.divestitureRequests.get(0).tag),
            "divestiture request did not carry the acquisition tag");
         owner.confirmDivestiture(object, ownerAttributes, new byte[] {0x30});
         JavaTckSupport.drain(acquirer);
         JavaTckSupport.check(acquirerRecorder.acquisitions.size() == 1,
            "confirmed ownership acquisition did not notify the acquirer");
         JavaTckSupport.check(Arrays.equals(new byte[] {0x30},
               acquirerRecorder.acquisitions.get(0).tag),
            "ownership acquisition notification did not carry the confirmation tag");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "owner retained ownership after confirmed divestiture");
         JavaTckSupport.check(acquirer.isAttributeOwnedByFederate(acquirerObject,
               acquirerAttribute), "acquirer did not gain ownership after confirmation");

         AttributeHandleSet wanted = acquirer.attributeOwnershipDivestitureIfWanted(
            acquirerObject, acquirerAttributes, new byte[] {0x31});
         JavaTckSupport.check(wanted != null,
            "attributeOwnershipDivestitureIfWanted returned null");

         // An unconditional release makes an immediately-available
         // acquisition complete without a negotiated callback.
         acquirer.unconditionalAttributeOwnershipDivestiture(acquirerObject,
            acquirerAttributes, new byte[] {0x40});
         owner.attributeOwnershipAcquisitionIfAvailable(object, ownerAttributes,
            new byte[] {0x50});
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.acquisitions.size() == 1,
            "if-available acquisition did not notify the returning owner");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "returning owner did not regain ownership");
         JavaTckSupport.check(!acquirer.isAttributeOwnedByFederate(acquirerObject,
               acquirerAttribute), "acquirer retained ownership after unconditional release");

         // The owner may deny a normal acquisition request after the release
         // callback.  The denial is delivered to the requester as an
         // unavailable callback with the owner's denial tag.
         acquirer.attributeOwnershipAcquisition(acquirerObject, acquirerAttributes,
            new byte[] {0x60});
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(ownerRecorder.releaseRequests.size() == 1,
            "normal acquisition did not request an ownership release");
         JavaTckSupport.check(Arrays.equals(new byte[] {0x60},
               ownerRecorder.releaseRequests.get(0).tag),
            "ownership release request did not preserve the acquisition tag");
         owner.attributeOwnershipReleaseDenied(object, ownerAttributes, new byte[] {0x70});
         JavaTckSupport.drain(acquirer);
         JavaTckSupport.check(acquirerRecorder.unavailable.size() == 1,
            "ownership release denial did not notify the requester");
         JavaTckSupport.check(Arrays.equals(new byte[] {0x70},
               acquirerRecorder.unavailable.get(0).tag),
            "ownership unavailable callback did not preserve the denial tag");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute),
            "owner lost ownership after denying the acquisition");

         // A second pending request can be canceled through the public Java
         // service and acknowledged by its standard callback.
         acquirer.attributeOwnershipAcquisition(acquirerObject, acquirerAttributes,
            new byte[] {(byte) 0x80});
         JavaTckSupport.drain(owner);
         acquirer.cancelAttributeOwnershipAcquisition(acquirerObject, acquirerAttributes);
         JavaTckSupport.drain(acquirer);
         JavaTckSupport.check(acquirerRecorder.cancellations.size() == 1,
            "ownership acquisition cancellation was not confirmed");

         owner.negotiatedAttributeOwnershipDivestiture(object, ownerAttributes,
            new byte[] {(byte) 0x90});
         owner.cancelNegotiatedAttributeOwnershipDivestiture(object, ownerAttributes);
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
