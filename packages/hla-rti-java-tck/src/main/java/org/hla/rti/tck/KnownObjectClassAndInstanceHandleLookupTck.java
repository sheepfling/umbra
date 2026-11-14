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
import java.util.function.BooleanSupplier;

/** Portable IEEE 1516.1-2025 lookup of a discovered named object. */
final class KnownObjectClassAndInstanceHandleLookupTck {
   private KnownObjectClassAndInstanceHandleLookupTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder observerCallbacks = new Recorder();
      String token = UUID.randomUUID().toString();
      String federation = "java-tck-known-object-lookup-" + token;
      String objectName = "java-tck-known-object-" + token;
      boolean ownerConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean observerJoined = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle observerClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet observerAttributes = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         observer.connect(observerCallbacks.proxy(), callbackModel);
         observerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-known-object-owner", "tck-type", federation);
         ownerJoined = true;
         observer.joinFederationExecution("java-tck-known-object-observer", "tck-type", federation);
         observerJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         observerClass = observer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle observerAttribute = observer.getAttributeHandle(observerClass, attributeName);
         JavaTckSupport.check(ownerClass.equals(observerClass)
               && ownerAttribute.equals(observerAttribute),
            "known-object lookup resolved different class or attribute handles");

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         observerAttributes = JavaTckSupport.attributeSet(observer, observerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes);
         subscribed = true;

         ownerCallbacks.clear();
         owner.reserveObjectInstanceName(objectName);
         await(() -> ownerCallbacks.reservationSucceeded(objectName), owner,
            callbackModel, "owner did not receive the named-object reservation callback");
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass, objectName);
         JavaTckSupport.check(object != null,
            "named object registration returned no object-instance handle");
         await(() -> observerCallbacks.discovery(object) != null, observer,
            callbackModel, "observer did not discover the named object");

         Discovery discovery = observerCallbacks.discovery(object);
         JavaTckSupport.check(object.equals(discovery.object)
               && observerClass.equals(discovery.objectClass)
               && objectName.equals(discovery.objectName),
            "observer discovered a different named object or class");
         JavaTckSupport.check(observerClass.equals(observer.getKnownObjectClassHandle(object)),
            "Get Known Object Class Handle did not return the discovered object's class");
         JavaTckSupport.check(object.equals(observer.getObjectInstanceHandle(objectName)),
            "Get Object Instance Handle did not resolve the registered object's name");
         JavaTckSupport.check(objectName.equals(observer.getObjectInstanceName(object)),
            "Get Object Instance Name did not return the discovered object's registered name");
      } finally {
         if (observerJoined && subscribed && observerClass != null && observerAttributes != null) {
            try { observer.unsubscribeObjectClassAttributes(observerClass, observerAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && published && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void await(BooleanSupplier condition, RTIambassador ambassador,
         CallbackModel callbackModel, String failure) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(ambassador);
         }
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), failure);
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final String objectName;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass,
            String objectName) {
         this.object = object;
         this.objectClass = objectClass;
         this.objectName = objectName;
      }
   }

   private static final class Recorder {
      private final List<String> reservedNames = new CopyOnWriteArrayList<>();
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("objectInstanceNameReservationSucceeded".equals(name)
                     && arguments != null && arguments.length == 1) {
                  reservedNames.add((String) arguments[0]);
               } else if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (String) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK known-object lookup callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private boolean reservationSucceeded(String name) {
         return reservedNames.contains(name);
      }

      private Discovery discovery(ObjectInstanceHandle object) {
         for (int i = discoveries.size() - 1; i >= 0; --i) {
            Discovery discovery = discoveries.get(i);
            if (object.equals(discovery.object)) return discovery;
         }
         return null;
      }

      private void clear() {
         reservedNames.clear();
      }
   }
}
