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
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Tests standard object-name reservation followed by named object registration. */
final class NamedRegistrationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private NamedRegistrationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder memberCallbacks = new Recorder();
      String federation = "java-tck-named-registration-" + UUID.randomUUID();
      boolean ownerConnected = false, memberConnected = false, created = false;
      boolean ownerJoined = false, memberJoined = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberCallbacks.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-named-registration-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-named-registration-member", federation);
         memberJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle memberClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         JavaTckSupport.check(ownerClass != null && memberClass != null
               && ownerAttribute != null && memberAttribute != null,
            "adapter FOM did not resolve the named-registration object class and attribute");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet memberAttributes = JavaTckSupport.attributeSet(member, memberAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         member.subscribeObjectClassAttributes(memberClass, memberAttributes);

         JavaTckSupport.expectFailure(() -> owner.reserveObjectInstanceName(""), "IllegalName",
            "reserving an empty object instance name");

         String releasedName = federation + "-released-name";
         owner.reserveObjectInstanceName(releasedName);
         await(owner, callbackModel,
            () -> ownerCallbacks.singleSucceeded.contains(releasedName),
            "single object-name reservation");
         owner.releaseObjectInstanceName(releasedName);
         ownerCallbacks.clear();
         owner.reserveObjectInstanceName(releasedName);
         await(owner, callbackModel,
            () -> ownerCallbacks.singleSucceeded.contains(releasedName),
            "reuse of a released object name");
         owner.releaseObjectInstanceName(releasedName);
         JavaTckSupport.expectFailure(() -> owner.releaseObjectInstanceName(releasedName),
            "ObjectInstanceNameNotReserved", "second release of an object name");

         Set<String> batchNames = new LinkedHashSet<>();
         batchNames.add(federation + "-batch-a");
         batchNames.add(federation + "-batch-b");
         ownerCallbacks.clear();
         owner.reserveMultipleObjectInstanceNames(batchNames);
         await(owner, callbackModel,
            () -> ownerCallbacks.multipleSucceeded.contains(batchNames),
            "multiple object-name reservation");
         owner.releaseMultipleObjectInstanceNames(batchNames);
         ownerCallbacks.clear();
         String reusableBatchName = batchNames.iterator().next();
         owner.reserveObjectInstanceName(reusableBatchName);
         await(owner, callbackModel,
            () -> ownerCallbacks.singleSucceeded.contains(reusableBatchName),
            "reuse of a released multiple reservation");
         owner.releaseObjectInstanceName(reusableBatchName);

         String objectName = federation + "-named-object";
         ownerCallbacks.clear();
         owner.reserveObjectInstanceName(objectName);
         await(owner, callbackModel, () -> ownerCallbacks.singleSucceeded.contains(objectName),
            "named object reservation");
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass, objectName);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "named registration returned an invalid object handle");
         JavaTckSupport.check(object.equals(owner.getObjectInstanceHandle(objectName)),
            "named registration lookup did not round-trip at the owner");
         JavaTckSupport.check(objectName.equals(owner.getObjectInstanceName(object)),
            "named registration object handle did not round-trip to its reserved name");

         await(member, callbackModel, () -> !memberCallbacks.discoveries.isEmpty(),
            "named object discovery");
         Discovery discovered = memberCallbacks.discoveries.get(0);
         JavaTckSupport.check(object.equals(discovered.object),
            "named discovery returned the wrong object handle");
         JavaTckSupport.check(objectName.equals(discovered.name),
            "named discovery returned the wrong instance name");
         JavaTckSupport.check(memberClass.equals(discovered.objectClass),
            "named discovery returned the wrong object class");
         JavaTckSupport.check(object.equals(member.getObjectInstanceHandle(objectName)),
            "named registration lookup did not round-trip at the member");

         String unreservedName = federation + "-unreserved-name";
         JavaTckSupport.expectFailure(() -> owner.registerObjectInstance(ownerClass,
            unreservedName), "ObjectInstanceNameNotReserved",
            "named registration with an unreserved object name");

         memberCallbacks.clear();
         member.reserveObjectInstanceName(objectName);
         await(member, callbackModel,
            () -> memberCallbacks.singleFailed.contains(objectName),
            "reservation failure for an in-use object name");
      } finally {
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (memberConnected) {
            try { member.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
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

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;
      private final String name;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass, String name) {
         this.object = object;
         this.objectClass = objectClass;
         this.name = name;
      }
   }

   private static final class Recorder {
      private final List<String> singleSucceeded = new CopyOnWriteArrayList<>();
      private final List<String> singleFailed = new CopyOnWriteArrayList<>();
      private final List<Set<String>> multipleSucceeded = new CopyOnWriteArrayList<>();
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();

      private void clear() {
         singleSucceeded.clear();
         singleFailed.clear();
         multipleSucceeded.clear();
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if (arguments != null && arguments.length == 1) {
                  if ("objectInstanceNameReservationSucceeded".equals(name)) {
                     singleSucceeded.add((String) arguments[0]);
                  } else if ("objectInstanceNameReservationFailed".equals(name)) {
                     singleFailed.add((String) arguments[0]);
                  } else if ("multipleObjectInstanceNameReservationSucceeded".equals(name)) {
                     multipleSucceeded.add(copyNames(arguments[0]));
                  }
               }
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1], (String) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK named-registration callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private static Set<String> copyNames(Object value) {
         Set<String> copy = new LinkedHashSet<>();
         for (Object name : (Set<?>) value) {
            if (name instanceof String) copy.add((String) name);
         }
         return Set.copyOf(copy);
      }
   }
}
