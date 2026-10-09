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

/** Checks ordinary instance-scoped attribute-value requests and provider callbacks. */
final class AttributeValueUpdateRequestBaselineTck {
   private AttributeValueUpdateRequestBaselineTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-attribute-value-request-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-request-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-requester", federation);
         requesterJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle ownerFirst = owner.getAttributeHandle(ownerClass, firstAttributeName);
         AttributeHandle ownerSecond = owner.getAttributeHandle(ownerClass, secondAttributeName);
         AttributeHandle requesterFirst = requester.getAttributeHandle(
            requesterClass, firstAttributeName);
         AttributeHandle requesterSecond = requester.getAttributeHandle(
            requesterClass, secondAttributeName);
         JavaTckSupport.check(objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(requester.getObjectClassName(requesterClass))
               && firstAttributeName.equals(owner.getAttributeName(ownerClass, ownerFirst))
               && secondAttributeName.equals(owner.getAttributeName(ownerClass, ownerSecond))
               && firstAttributeName.equals(requester.getAttributeName(requesterClass,
                  requesterFirst))
               && secondAttributeName.equals(requester.getAttributeName(requesterClass,
                  requesterSecond)),
            "adapter FOM did not resolve the requested object-class attributes");

         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(
            owner, ownerFirst, ownerSecond);
         AttributeHandleSet requesterAttributes = JavaTckSupport.attributeSet(
            requester, requesterSecond);
         AttributeHandleSet requestedAttributes = JavaTckSupport.attributeSet(
            requester, requesterFirst, requesterSecond);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.publishObjectClassAttributes(requesterClass, requesterAttributes);
         requester.subscribeObjectClassAttributes(requesterClass, requestedAttributes);

         ObjectInstanceHandle ownerObject = owner.registerObjectInstance(ownerClass);
         await(requester,
            () -> requesterRecorder.discovered(ownerObject),
            "requester did not discover the published object before the deadline");

         byte[] requestTag = new byte[] {0x52, 0x51, 0x2d, 0x31};
         ownerRecorder.clearProvided();
         requester.requestAttributeValueUpdate(ownerObject, requestedAttributes, requestTag);
         await(owner,
            () -> !ownerRecorder.provided().isEmpty(),
            "owner did not receive Provide Attribute Value Update before the deadline");
         List<ProvidedUpdate> provided = ownerRecorder.provided();
         JavaTckSupport.check(provided.size() == 1
               && ownerObject.equals(provided.get(0).object)
               && new HashSet<>(ownerAttributes).equals(provided.get(0).attributes)
               && Arrays.equals(requestTag, provided.get(0).tag),
            "provider callback did not preserve the requested object, owned attributes, and tag");

         ObjectInstanceHandle requesterObject = requester.registerObjectInstance(requesterClass);
         ownerRecorder.clearProvided();
         requesterRecorder.clearProvided();
         requester.requestAttributeValueUpdate(
            requesterObject, requestedAttributes, requestTag);
         for (int pass = 0; pass < 8; ++pass) {
            JavaTckSupport.drain(requester);
            JavaTckSupport.drain(owner);
         }
         JavaTckSupport.check(requesterRecorder.provided().isEmpty()
               && ownerRecorder.provided().isEmpty(),
            "request solicited the requester for its own or unowned instance attributes");

         requester.unsubscribeObjectClassAttributes(requesterClass, requestedAttributes);
         requester.unpublishObjectClassAttributes(requesterClass, requesterAttributes);
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         requester.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
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
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, BooleanSupplier condition,
         String failure) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         JavaTckSupport.drain(ambassador);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), failure);
   }

   private static final class ProvidedUpdate {
      private final ObjectInstanceHandle object;
      private final HashSet<AttributeHandle> attributes;
      private final byte[] tag;

      private ProvidedUpdate(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
         this.tag = tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<ProvidedUpdate> provided = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("provideAttributeValueUpdate".equals(name)) {
                  provided.add(new ProvidedUpdate((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK attribute-value request callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private boolean discovered(ObjectInstanceHandle object) {
         return discoveries.contains(object);
      }

      private List<ProvidedUpdate> provided() {
         return List.copyOf(provided);
      }

      private void clearProvided() {
         provided.clear();
      }
   }
}
