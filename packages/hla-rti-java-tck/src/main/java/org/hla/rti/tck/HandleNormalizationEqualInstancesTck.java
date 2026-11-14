package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Checks equal standard handle values normalize to equal values after factory decoding. */
final class HandleNormalizationEqualInstancesTck {
   private HandleNormalizationEqualInstancesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-handle-normalization-equal-" + UUID.randomUUID();
      String ownerName = "java-tck-normalize-equal-owner-" + UUID.randomUUID();
      String memberName = "java-tck-normalize-equal-member-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         owner.connect(new NullFederateAmbassador(), callbackModel);
         ownerConnected = true;
         member.connect(new NullFederateAmbassador(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, new String[] {fom}, timeImplementation);
         created = true;
         FederateHandle federate = owner.joinFederationExecution(ownerName, "java-tck", federation);
         ownerJoined = true;
         member.joinFederationExecution(memberName, "java-tck", federation);
         memberJoined = true;

         ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
         InteractionClassHandle interactionClass =
            owner.getInteractionClassHandle(interactionClassName);
         ParameterHandle parameter = owner.getParameterHandle(interactionClass, parameterName);
         owner.publishObjectClassAttributes(objectClass, JavaTckSupport.attributeSet(owner, attribute));
         ObjectInstanceHandle object = owner.registerObjectInstance(objectClass);
         JavaTckSupport.check(federate != null && parameter != null && object != null,
            "handle-normalization fixture returned a null standard handle");

         FederateHandle equalFederate = decodedCopy(federate, federate.encodedLength(),
            (handle, bytes, offset) -> handle.encode(bytes, offset),
            (bytes, offset) -> owner.getFederateHandleFactory().decode(bytes, offset));
         ObjectClassHandle equalObjectClass = decodedCopy(objectClass, objectClass.encodedLength(),
            (handle, bytes, offset) -> handle.encode(bytes, offset),
            (bytes, offset) -> owner.getObjectClassHandleFactory().decode(bytes, offset));
         InteractionClassHandle equalInteractionClass = decodedCopy(
            interactionClass, interactionClass.encodedLength(),
            (handle, bytes, offset) -> handle.encode(bytes, offset),
            (bytes, offset) -> owner.getInteractionClassHandleFactory().decode(bytes, offset));
         ObjectInstanceHandle equalObject = decodedCopy(object, object.encodedLength(),
            (handle, bytes, offset) -> handle.encode(bytes, offset),
            (bytes, offset) -> owner.getObjectInstanceHandleFactory().decode(bytes, offset));

         assertEqualNormalization(federate, equalFederate,
            owner::normalizeFederateHandle, "federate handle");
         assertEqualNormalization(objectClass, equalObjectClass,
            owner::normalizeObjectClassHandle, "object-class handle");
         assertEqualNormalization(interactionClass, equalInteractionClass,
            owner::normalizeInteractionClassHandle, "interaction-class handle");
         assertEqualNormalization(object, equalObject,
            owner::normalizeObjectInstanceHandle, "object-instance handle");
      } finally {
         if (memberJoined) try {
            member.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         } catch (Exception ignored) { }
         if (created) try {
            owner.destroyFederationExecution(federation);
         } catch (Exception ignored) { }
         if (memberConnected) try { member.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static <H> H decodedCopy(H handle, int encodedLength,
         HandleEncoder<H> encoder, HandleDecoder<H> decoder) throws Exception {
      JavaTckSupport.check(handle != null, "normalization fixture returned a null handle");
      JavaTckSupport.check(encodedLength > 0, "normalization fixture returned an empty handle encoding");
      byte[] bytes = new byte[encodedLength];
      encoder.encode(handle, bytes, 0);
      H copy = decoder.decode(bytes, 0);
      JavaTckSupport.check(copy != null, "standard handle factory decoded a null handle");
      return copy;
   }

   private static <H> void assertEqualNormalization(H handle, H equalHandle,
         HandleNormalizer<H> normalizer, String description) throws Exception {
      JavaTckSupport.check(handle.equals(equalHandle) && equalHandle.equals(handle),
         description + " factory-decoded value was not equal to the original handle");
      JavaTckSupport.check(normalizer.normalize(handle) == normalizer.normalize(equalHandle),
         description + " equal handle values produced different normalized values");
   }

   @FunctionalInterface
   private interface HandleEncoder<H> {
      void encode(H handle, byte[] bytes, int offset);
   }

   @FunctionalInterface
   private interface HandleDecoder<H> {
      H decode(byte[] bytes, int offset) throws Exception;
   }

   @FunctionalInterface
   private interface HandleNormalizer<H> {
      long normalize(H handle) throws Exception;
   }
}
