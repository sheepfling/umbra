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
import java.util.UUID;

/** Checks that handle decoding preserves syntax without consulting federation-local state. */
final class HandleDecoderSemanticValidityTck {
   private HandleDecoderSemanticValidityTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador source = factory.getRtiAmbassador();
      RTIambassador decoder = factory.getRtiAmbassador();
      String sourceFederation = "java-tck-handle-decode-source-" + UUID.randomUUID();
      String targetFederation = "java-tck-handle-decode-target-" + UUID.randomUUID();
      boolean sourceConnected = false;
      boolean decoderConnected = false;
      boolean sourceCreated = false;
      boolean targetCreated = false;
      boolean sourceJoined = false;
      boolean decoderJoined = false;
      try {
         source.connect(callbacks(), callbackModel);
         sourceConnected = true;
         decoder.connect(callbacks(), callbackModel);
         decoderConnected = true;

         source.createFederationExecution(sourceFederation, fom, timeImplementation);
         sourceCreated = true;
         source.joinFederationExecution("java-tck-handle-decode-source", sourceFederation);
         sourceJoined = true;
         decoder.createFederationExecution(targetFederation, fom, timeImplementation);
         targetCreated = true;
         decoder.joinFederationExecution("java-tck-handle-decode-target", targetFederation);
         decoderJoined = true;

         ObjectClassHandle objectClass = source.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = source.getAttributeHandle(objectClass, attributeName);
         AttributeHandleSet publishedAttributes =
            JavaTckSupport.attributeSet(source, attribute);
         source.publishObjectClassAttributes(objectClass, publishedAttributes);
         ObjectInstanceHandle sourceObject = source.registerObjectInstance(objectClass);
         JavaTckSupport.check(sourceObject != null && sourceObject.encodedLength() > 0,
            "source federation returned an invalid object-instance handle");

         byte[] encoded = JavaTckSupport.encode(sourceObject.encodedLength(), sourceObject::encode);
         ObjectInstanceHandle decoded =
            decoder.getObjectInstanceHandleFactory().decode(encoded, 0);
         JavaTckSupport.check(sourceObject.equals(decoded),
            "object-instance handle decoding changed the encoded handle value");
         JavaTckSupport.expectFailure(() -> decoder.getObjectInstanceName(decoded),
            "ObjectInstanceNotKnown",
            "lookup of the decoded handle in a federation with no such instance");
      } finally {
         resign(source);
         resign(decoder);
         if (sourceCreated) destroy(source, sourceFederation);
         if (targetCreated) destroy(decoder, targetFederation);
         if (sourceConnected) disconnect(source);
         if (decoderConnected) disconnect(decoder);
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK handle-decoder callbacks";
               }
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static void resign(RTIambassador ambassador) {
      try {
         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
      } catch (Exception ignored) { }
   }

   private static void destroy(RTIambassador ambassador, String federation) {
      try {
         ambassador.destroyFederationExecution(federation);
      } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try {
         ambassador.disconnect();
      } catch (Exception ignored) { }
   }
}
