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
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks class-scoped value requests over concrete subclass instances. */
final class ObjectClassAttributeValueUpdateRequestBaselineTck {
   private ObjectClassAttributeValueUpdateRequestBaselineTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseClassName, String derivedClassName, String identityAttributeName,
         String integerAttributeName, String derivedAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-class-attribute-value-request-" + UUID.randomUUID();
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
         owner.joinFederationExecution("java-tck-class-request-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-class-requester", federation);
         requesterJoined = true;

         ObjectClassHandle ownerDerivedClass = owner.getObjectClassHandle(derivedClassName);
         ObjectClassHandle requesterBaseClass = requester.getObjectClassHandle(baseClassName);
         AttributeHandle ownerIdentity = owner.getAttributeHandle(
            ownerDerivedClass, identityAttributeName);
         AttributeHandle ownerDerived = owner.getAttributeHandle(
            ownerDerivedClass, derivedAttributeName);
         AttributeHandle requesterIdentity = requester.getAttributeHandle(
            requesterBaseClass, identityAttributeName);
         AttributeHandle requesterInteger = requester.getAttributeHandle(
            requesterBaseClass, integerAttributeName);
         JavaTckSupport.check(derivedClassName.equals(
               owner.getObjectClassName(ownerDerivedClass))
               && baseClassName.equals(requester.getObjectClassName(requesterBaseClass))
               && identityAttributeName.equals(owner.getAttributeName(
                  ownerDerivedClass, ownerIdentity))
               && derivedAttributeName.equals(owner.getAttributeName(
                  ownerDerivedClass, ownerDerived))
               && identityAttributeName.equals(requester.getAttributeName(
                  requesterBaseClass, requesterIdentity))
               && integerAttributeName.equals(requester.getAttributeName(
                  requesterBaseClass, requesterInteger)),
            "adapter FOM did not resolve the typed base/derived object attributes");

         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(
            owner, ownerIdentity, ownerDerived);
         AttributeHandleSet requesterAttributes = JavaTckSupport.attributeSet(
            requester, requesterInteger);
         AttributeHandleSet requestedAttributes = JavaTckSupport.attributeSet(
            requester, requesterIdentity, requesterInteger);
         owner.publishObjectClassAttributes(ownerDerivedClass, ownerAttributes);
         requester.publishObjectClassAttributes(requesterBaseClass, requesterAttributes);
         requester.subscribeObjectClassAttributes(requesterBaseClass, requestedAttributes);

         ObjectInstanceHandle firstOwnerObject =
            owner.registerObjectInstance(ownerDerivedClass);
         ObjectInstanceHandle secondOwnerObject =
            owner.registerObjectInstance(ownerDerivedClass);
         await(requester, () -> requesterRecorder.discovered(firstOwnerObject)
               && requesterRecorder.discovered(secondOwnerObject),
            "requester did not discover both derived instances before the deadline");
         requester.registerObjectInstance(requesterBaseClass);

         byte[] requestTag = new byte[] {0x43, 0x4c, 0x53, 0x2d, 0x31};
         ownerRecorder.clearProvided();
         requesterRecorder.clearProvided();
         requester.requestAttributeValueUpdate(
            requesterBaseClass, requestedAttributes, requestTag);
         await(owner, () -> ownerRecorder.provided().size() == 2,
            "class request did not produce one callback per matching derived instance");

         List<ProvidedUpdate> ownerRequests = ownerRecorder.provided();
         Set<ObjectInstanceHandle> expectedObjects =
            Set.of(firstOwnerObject, secondOwnerObject);
         Set<ObjectInstanceHandle> receivedObjects = new HashSet<>();
         Set<AttributeHandle> expectedAttributes = Set.of(ownerIdentity);
         for (ProvidedUpdate update : ownerRequests) {
            JavaTckSupport.check(expectedObjects.contains(update.object)
                  && receivedObjects.add(update.object)
                  && expectedAttributes.equals(update.attributes)
                  && Arrays.equals(requestTag, update.tag),
               "class request callback did not preserve object, inherited attribute subset, or tag");
         }
         JavaTckSupport.check(expectedObjects.equals(receivedObjects),
            "class request did not cover each matching subclass instance exactly once");
         JavaTckSupport.check(requesterRecorder.provided().isEmpty(),
            "class request solicited the requester for its own object attribute");

         requester.unsubscribeObjectClassAttributes(requesterBaseClass, requestedAttributes);
         requester.unpublishObjectClassAttributes(requesterBaseClass, requesterAttributes);
         owner.unpublishObjectClassAttributes(ownerDerivedClass, ownerAttributes);
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
      private final Set<AttributeHandle> attributes;
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
                  if ("toString".equals(name)) return "Java TCK class-request callbacks";
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
