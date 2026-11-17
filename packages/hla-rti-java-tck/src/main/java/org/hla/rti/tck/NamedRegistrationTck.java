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

         JavaTckSupport.expectFailure(() -> owner.reserveMultipleObjectInstanceNames(Set.of()),
            "NameSetWasEmpty", "reserving an empty object instance name set");
         JavaTckSupport.expectFailure(() -> owner.reserveMultipleObjectInstanceNames(
            Set.of("HLA.ReservedByTheRTI")), "IllegalName",
            "reserving a name with the reserved HLA prefix");
         String validNameInRejectedSet = federation + "-valid-in-rejected-set";
         JavaTckSupport.expectFailure(() -> owner.reserveMultipleObjectInstanceNames(
            Set.of(validNameInRejectedSet, "HLA.Invalid")), "IllegalName",
            "rejecting a multiple-name reservation containing an illegal name");
         memberCallbacks.clear();
         member.reserveObjectInstanceName(validNameInRejectedSet);
         await(member, callbackModel,
            () -> memberCallbacks.singleSucceeded.contains(validNameInRejectedSet),
            "valid name remains available after an illegal multiple-name reservation");
         checkNoMultipleReservationOutcome(memberCallbacks,
            "single-name reservation after an illegal multiple-name request");
         member.releaseObjectInstanceName(validNameInRejectedSet);

         String releasedName = federation + "-released-name";
         owner.reserveObjectInstanceName(releasedName);
         await(owner, callbackModel,
            () -> ownerCallbacks.singleSucceeded.contains(releasedName),
            "single object-name reservation");
         checkNoMultipleReservationOutcome(ownerCallbacks, "single-name reservation");
         owner.releaseObjectInstanceName(releasedName);
         memberCallbacks.clear();
         member.reserveObjectInstanceName(releasedName);
         await(member, callbackModel,
            () -> memberCallbacks.singleSucceeded.contains(releasedName),
            "a released object name is available to another joined federate");
         checkNoMultipleReservationOutcome(memberCallbacks,
            "single-name reservation of a name released by another federate");
         member.releaseObjectInstanceName(releasedName);
         ownerCallbacks.clear();
         owner.reserveObjectInstanceName(releasedName);
         await(owner, callbackModel,
            () -> ownerCallbacks.singleSucceeded.contains(releasedName),
            "reuse of a released object name");
         checkNoMultipleReservationOutcome(ownerCallbacks,
            "single-name reuse of a released object name");
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
         JavaTckSupport.check(ownerCallbacks.multipleSucceeded.size() == 1
               && ownerCallbacks.multipleFailed.isEmpty()
               && ownerCallbacks.singleSucceeded.isEmpty()
               && ownerCallbacks.singleFailed.isEmpty(),
            "multiple-name reservation used an incorrect reservation callback");
         String unreservedReleaseName = federation + "-not-reserved-for-release";
         Set<String> invalidReleaseNames = new LinkedHashSet<>();
         invalidReleaseNames.add(batchNames.iterator().next());
         invalidReleaseNames.add(unreservedReleaseName);
         JavaTckSupport.expectFailure(
            () -> owner.releaseMultipleObjectInstanceNames(invalidReleaseNames),
            "ObjectInstanceNameNotReserved",
            "releasing a multiple-name set containing an unreserved name");
         for (String batchName : batchNames) {
            memberCallbacks.clear();
            member.reserveObjectInstanceName(batchName);
            await(member, callbackModel, () -> memberCallbacks.singleFailed.contains(batchName),
               "failed multiple-name release retains every reserved name");
            checkNoMultipleReservationOutcome(memberCallbacks,
               "single-name contention against a name retained by failed multiple release");
         }
         owner.releaseMultipleObjectInstanceNames(batchNames);
         memberCallbacks.clear();
         String reusableBatchName = batchNames.iterator().next();
         member.reserveObjectInstanceName(reusableBatchName);
         await(member, callbackModel,
            () -> memberCallbacks.singleSucceeded.contains(reusableBatchName),
            "a name released from a multiple-name set is available to another joined federate");
         checkNoMultipleReservationOutcome(memberCallbacks,
            "single-name reservation after multiple-name release");
         member.releaseObjectInstanceName(reusableBatchName);
         ownerCallbacks.clear();
         owner.reserveObjectInstanceName(reusableBatchName);
         await(owner, callbackModel,
            () -> ownerCallbacks.singleSucceeded.contains(reusableBatchName),
            "reuse of a released multiple reservation");
         checkNoMultipleReservationOutcome(ownerCallbacks,
            "single-name reuse after multiple-name release");
         owner.releaseObjectInstanceName(reusableBatchName);

         String objectName = federation + "-named-object";
         JavaTckSupport.expectFailure(() -> owner.registerObjectInstance(ownerClass, objectName),
            "ObjectInstanceNameNotReserved",
            "named registration with an unreserved object name");
         ownerCallbacks.clear();
         memberCallbacks.discoveries.clear();
         owner.reserveObjectInstanceName(objectName);
         await(owner, callbackModel, () -> ownerCallbacks.singleSucceeded.contains(objectName),
            "named object reservation");
         checkNoMultipleReservationOutcome(ownerCallbacks, "named object reservation");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(member);
         JavaTckSupport.check(memberCallbacks.discoveries.isEmpty(),
            "Reserve Object Instance Name caused an object-discovery callback");
         memberCallbacks.clear();
         member.reserveObjectInstanceName(objectName);
         await(member, callbackModel, () -> memberCallbacks.singleFailed.contains(objectName),
            "another federate cannot successfully reserve a name held by the owner");
         checkNoMultipleReservationOutcome(memberCallbacks,
            "single-name contention against the owner's reservation");
         member.publishObjectClassAttributes(memberClass, memberAttributes);
         JavaTckSupport.expectFailure(
            () -> member.registerObjectInstance(memberClass, objectName),
            "ObjectInstanceNameNotReserved",
            "another publisher consuming the owner's object-name reservation");
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         JavaTckSupport.expectFailure(
            () -> owner.registerObjectInstance(ownerClass, objectName),
            "ObjectClassNotPublished",
            "named registration before object-class publication");
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass, objectName);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "named registration returned an invalid object handle");
         JavaTckSupport.check(object.equals(owner.getObjectInstanceHandle(objectName)),
            "named registration lookup did not round-trip at the owner");
         JavaTckSupport.check(objectName.equals(owner.getObjectInstanceName(object)),
            "named registration object handle did not round-trip to its reserved name");
         JavaTckSupport.expectFailure(() -> owner.releaseObjectInstanceName(objectName),
            "ObjectInstanceNameNotReserved",
            "releasing an object-name reservation consumed by registration");
         JavaTckSupport.expectFailure(
            () -> member.registerObjectInstance(memberClass, objectName),
            "ObjectInstanceNameInUse",
            "another publisher registering an already-used object name");
         member.unpublishObjectClassAttributes(memberClass, memberAttributes);

         await(member, callbackModel, () -> !memberCallbacks.discoveries.isEmpty(),
            "named object discovery");
         JavaTckSupport.check(memberCallbacks.discoveries.size() == 1,
            "named registration did not produce exactly one discovery callback");
         Discovery discovered = memberCallbacks.discoveries.get(0);
         JavaTckSupport.check(object.equals(discovered.object),
            "named discovery returned the wrong object handle");
         JavaTckSupport.check(objectName.equals(discovered.name),
            "named discovery returned the wrong instance name");
         JavaTckSupport.check(memberClass.equals(discovered.objectClass),
            "named discovery returned the wrong object class");
         JavaTckSupport.check(object.equals(member.getObjectInstanceHandle(objectName)),
            "named registration lookup did not round-trip at the member");

         String reservedAgainstGeneration = federation + "-reserved-against-generation";
         ownerCallbacks.clear();
         owner.reserveObjectInstanceName(reservedAgainstGeneration);
         await(owner, callbackModel,
            () -> ownerCallbacks.singleSucceeded.contains(reservedAgainstGeneration),
            "reservation before unnamed registration");
         memberCallbacks.discoveries.clear();
         ObjectInstanceHandle generatedObject = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(generatedObject != null && generatedObject.encodedLength() > 0,
            "unnamed registration returned an invalid object handle");
         String generatedName = owner.getObjectInstanceName(generatedObject);
         JavaTckSupport.check(!reservedAgainstGeneration.equals(generatedName),
            "unnamed registration consumed another reserved object name");
         ObjectInstanceHandle secondGeneratedObject = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(secondGeneratedObject != null
               && secondGeneratedObject.encodedLength() > 0
               && !generatedObject.equals(secondGeneratedObject),
            "second unnamed registration returned an invalid or duplicate object handle");
         String secondGeneratedName = owner.getObjectInstanceName(secondGeneratedObject);
         JavaTckSupport.check(generatedName != null && !generatedName.isEmpty()
               && secondGeneratedName != null && !secondGeneratedName.isEmpty()
               && !generatedName.equals(secondGeneratedName)
               && generatedObject.equals(owner.getObjectInstanceHandle(generatedName))
               && secondGeneratedObject.equals(owner.getObjectInstanceHandle(secondGeneratedName)),
            "unnamed registration names were not unique and associated with their handles");
         await(member, callbackModel, () -> memberCallbacks.discoveries.size() >= 2,
            "unnamed object discoveries after name reservation");
         JavaTckSupport.check(memberCallbacks.discoveries.size() == 2,
            "unnamed registration did not produce exactly two discovery callbacks");
         Discovery firstGeneratedDiscovery = findDiscovery(memberCallbacks, generatedObject);
         Discovery secondGeneratedDiscovery = findDiscovery(memberCallbacks, secondGeneratedObject);
         JavaTckSupport.check(firstGeneratedDiscovery != null && secondGeneratedDiscovery != null
               && generatedName.equals(firstGeneratedDiscovery.name)
               && secondGeneratedName.equals(secondGeneratedDiscovery.name)
               && memberClass.equals(firstGeneratedDiscovery.objectClass)
               && memberClass.equals(secondGeneratedDiscovery.objectClass)
               && generatedObject.equals(member.getObjectInstanceHandle(generatedName))
               && secondGeneratedObject.equals(member.getObjectInstanceHandle(secondGeneratedName)),
            "unnamed discoveries did not preserve each generated name and handle");
         owner.releaseObjectInstanceName(reservedAgainstGeneration);

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

   private static void checkNoMultipleReservationOutcome(Recorder recorder, String description) {
      JavaTckSupport.check(!recorder.hasMultipleReservationOutcome(),
         description + " used a multiple-name reservation callback");
   }

   private static Discovery findDiscovery(Recorder recorder, ObjectInstanceHandle object) {
      for (Discovery discovery : recorder.discoveries) {
         if (object.equals(discovery.object)) return discovery;
      }
      return null;
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
      private final List<Set<String>> multipleFailed = new CopyOnWriteArrayList<>();
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();

      private void clear() {
         singleSucceeded.clear();
         singleFailed.clear();
         multipleSucceeded.clear();
         multipleFailed.clear();
      }

      private boolean hasMultipleReservationOutcome() {
         return !multipleSucceeded.isEmpty() || !multipleFailed.isEmpty();
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
                  } else if ("multipleObjectInstanceNameReservationFailed".equals(name)) {
                     multipleFailed.add(copyNames(arguments[0]));
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
