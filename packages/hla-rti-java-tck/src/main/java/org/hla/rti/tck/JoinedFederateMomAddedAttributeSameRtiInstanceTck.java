package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Checks ownership and updates for an added attribute on the RTI's MOM instance. */
final class JoinedFederateMomAddedAttributeSameRtiInstanceTck {
   private static final String MOM_CLASS = "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String MOM_HANDLE = "HLAfederateHandle";
   private static final String ADDED_ATTRIBUTE = "TckFederateAddedValue";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;
   private static final int EXPECTED_VALUE = 15162025;

   private JoinedFederateMomAddedAttributeSameRtiInstanceTck() { }

   static void runFromAdapter() throws Exception {
      run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.momFederateAddedAttributeFom"),
         RtiTckMain.requireMim(), RtiTckMain.timeImplementation(),
         RtiTckMain.callbackModel());
   }

   static void run(RtiFactory factory, String addedAttributeFom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador target = factory.getRtiAmbassador();
      RTIambassador updater = factory.getRtiAmbassador();
      RTIambassador watcher = factory.getRtiAmbassador();
      Recorder[] recorders = {new Recorder(), new Recorder(), new Recorder()};
      RTIambassador[] ambassadors = {target, updater, watcher};
      boolean[] connected = new boolean[3];
      boolean[] joined = new boolean[3];
      boolean created = false;
      boolean updaterSubscribed = false;
      boolean watcherSubscribed = false;
      String federation = "java-tck-mom-added-attribute-" + UUID.randomUUID();
      String targetName = "java-tck-mom-added-target-" + UUID.randomUUID();
      String updaterName = "java-tck-mom-added-updater-" + UUID.randomUUID();
      String watcherName = "java-tck-mom-added-watcher-" + UUID.randomUUID();
      ObjectClassHandle updaterMomClass = null;
      ObjectClassHandle watcherMomClass = null;
      AttributeHandle updaterAddedAttribute = null;
      AttributeHandle watcherAddedAttribute = null;
      AttributeHandle updaterMomHandle = null;
      AttributeHandle watcherMomHandle = null;
      AttributeHandleSet updaterSubscribedAttributes = null;
      AttributeHandleSet watcherSubscribedAttributes = null;
      try {
         JavaTckSupport.check(addedAttributeFom != null && !addedAttributeFom.isEmpty()
               && mim != null && !mim.isEmpty(),
            "adapter-supplied HLAfederate extension FOM and standard MIM are required");
         for (int index = 0; index < ambassadors.length; ++index) {
            ambassadors[index].connect(recorders[index].proxy(), callbackModel);
            connected[index] = true;
         }
         target.createFederationExecutionWithMIM(federation,
            new String[] {addedAttributeFom}, mim, timeImplementation);
         created = true;
         updater.joinFederationExecution(updaterName, "java-tck-type", federation);
         joined[1] = true;
         watcher.joinFederationExecution(watcherName, "java-tck-type", federation);
         joined[2] = true;

         updaterMomClass = updater.getObjectClassHandle(MOM_CLASS);
         watcherMomClass = watcher.getObjectClassHandle(MOM_CLASS);
         updaterAddedAttribute = updater.getAttributeHandle(updaterMomClass, ADDED_ATTRIBUTE);
         watcherAddedAttribute = watcher.getAttributeHandle(watcherMomClass, ADDED_ATTRIBUTE);
         updaterMomHandle = updater.getAttributeHandle(updaterMomClass, MOM_HANDLE);
         watcherMomHandle = watcher.getAttributeHandle(watcherMomClass, MOM_HANDLE);
         final AttributeHandle updaterAddedAttributeResolved = updaterAddedAttribute;
         final AttributeHandle watcherAddedAttributeResolved = watcherAddedAttribute;
         final AttributeHandle updaterMomHandleResolved = updaterMomHandle;
         final AttributeHandle watcherMomHandleResolved = watcherMomHandle;
         JavaTckSupport.check(MOM_CLASS.equals(updater.getObjectClassName(updaterMomClass))
               && MOM_CLASS.equals(watcher.getObjectClassName(watcherMomClass))
               && updaterMomClass.equals(watcherMomClass)
               && updaterAddedAttribute.equals(watcherAddedAttribute)
               && updaterMomHandle.equals(watcherMomHandle)
               && ADDED_ATTRIBUTE.equals(updater.getAttributeName(
                  updaterMomClass, updaterAddedAttribute))
               && ADDED_ATTRIBUTE.equals(watcher.getAttributeName(
                  watcherMomClass, watcherAddedAttribute)),
            "standard MIM or FOM-added HLAfederate attribute handles did not resolve consistently");

         updaterSubscribedAttributes = JavaTckSupport.attributeSet(
            updater, updaterAddedAttribute, updaterMomHandle);
         watcherSubscribedAttributes = JavaTckSupport.attributeSet(
            watcher, watcherAddedAttribute, watcherMomHandle);
         updater.publishObjectClassAttributes(updaterMomClass,
            JavaTckSupport.attributeSet(updater, updaterAddedAttribute));
         updater.subscribeObjectClassAttributes(updaterMomClass, updaterSubscribedAttributes);
         updaterSubscribed = true;
         watcher.subscribeObjectClassAttributes(watcherMomClass, watcherSubscribedAttributes);
         watcherSubscribed = true;

         FederateHandle targetFederate = target.joinFederationExecution(
            targetName, "java-tck-type", federation);
         joined[0] = true;
         byte[] targetFederateBytes = JavaTckSupport.encode(
            targetFederate.encodedLength(), targetFederate::encode);
         await(updater, callbackModel,
            () -> recorders[1].momObjectForHandle(updaterMomHandleResolved, targetFederateBytes)
               .filter(reflection -> recorders[1].discoveries.contains(reflection.object)).isPresent(),
            "updater discovery of the target federate MOM object");
         await(watcher, callbackModel,
            () -> recorders[2].momObjectForHandle(watcherMomHandleResolved, targetFederateBytes)
               .filter(reflection -> recorders[2].discoveries.contains(reflection.object)).isPresent(),
            "watcher discovery of the target federate MOM object");

         Reflection updaterTarget = recorders[1]
            .momObjectForHandle(updaterMomHandle, targetFederateBytes).orElseThrow(
               () -> new AssertionError("updater did not identify the target MOM object"));
         Reflection watcherTarget = recorders[2]
            .momObjectForHandle(watcherMomHandle, targetFederateBytes).orElseThrow(
               () -> new AssertionError("watcher did not identify the target MOM object"));
         JavaTckSupport.check(updaterTarget.object.equals(watcherTarget.object)
               && updaterMomClass.equals(updater.getKnownObjectClassHandle(updaterTarget.object))
               && watcherMomClass.equals(watcher.getKnownObjectClassHandle(watcherTarget.object)),
            "updater and watcher did not identify the same RTI-registered HLAfederate instance");
         String updaterTargetName = updater.getObjectInstanceName(updaterTarget.object);
         String watcherTargetName = watcher.getObjectInstanceName(watcherTarget.object);
         JavaTckSupport.check(!updaterTargetName.isEmpty()
               && updaterTargetName.equals(watcherTargetName)
               && updater.getObjectInstanceHandle(updaterTargetName).equals(updaterTarget.object)
               && watcher.getObjectInstanceHandle(watcherTargetName).equals(watcherTarget.object),
            "target MOM object name and handle did not round-trip across both members");

         int notOwnedBefore = recorders[1].notOwned.size();
         int ownedByRtiBefore = recorders[1].ownedByRti.size();
         updater.queryAttributeOwnership(updaterTarget.object,
            JavaTckSupport.attributeSet(updater, updaterAddedAttribute));
         await(updater, callbackModel,
            () -> recorders[1].notOwned.size() > notOwnedBefore
               || recorders[1].ownedByRti.size() > ownedByRtiBefore,
            "ownership query for the FOM-added MOM attribute");
         String notOwnedReply = recorders[1].notOwned.size() > notOwnedBefore
            ? recorders[1].notOwned.get(notOwnedBefore).describe() : "none";
         JavaTckSupport.check(recorders[1].notOwned.size() == notOwnedBefore + 1
               && recorders[1].notOwned.get(notOwnedBefore).matches(
                  updaterTarget.object, updaterAddedAttribute)
               && recorders[1].ownedByRti.size() == ownedByRtiBefore,
            "FOM-added attribute ownership query returned an unexpected result (notOwnedDelta="
               + (recorders[1].notOwned.size() - notOwnedBefore) + ", ownedByRtiDelta="
               + (recorders[1].ownedByRti.size() - ownedByRtiBefore) + ", notOwnedReply="
               + notOwnedReply + ", expectedObject=" + updaterTarget.object
               + ", expectedAttribute=" + updaterAddedAttribute + ")");

         byte[] acquisitionTag = {0x11, 0x22, 0x33};
         int acquisitionBefore = recorders[1].acquisitions.size();
         updater.attributeOwnershipAcquisition(updaterTarget.object,
            JavaTckSupport.attributeSet(updater, updaterAddedAttribute), acquisitionTag);
         await(updater, callbackModel,
            () -> recorders[1].acquisitions.size() > acquisitionBefore,
            "acquisition of the FOM-added HLAfederate attribute");
         AcquisitionEvent acquisition = recorders[1].acquisitions.get(acquisitionBefore);
         JavaTckSupport.check(acquisition.matches(updaterTarget.object, updaterAddedAttribute,
               acquisitionTag)
               && updater.isAttributeOwnedByFederate(
                  updaterTarget.object, updaterAddedAttribute),
            "FOM-added MOM attribute acquisition was not granted on the discovered RTI object");

         byte[] updateTag = {0x44, 0x55, 0x66};
         AttributeHandleValueMap update = updater.getAttributeHandleValueMapFactory().create(1);
         update.put(updaterAddedAttribute,
            factory.getEncoderFactory().createHLAinteger32BE(EXPECTED_VALUE).toByteArray());
         int reflectionBefore = recorders[2].reflections.size();
         updater.updateAttributeValues(updaterTarget.object, update, updateTag);
         await(watcher, callbackModel,
            () -> recorders[2].reflections.stream().skip(reflectionBefore)
               .anyMatch(reflection -> reflection.object.equals(watcherTarget.object)
                  && reflection.values.containsKey(watcherAddedAttributeResolved)),
            "FOM-added attribute reflection on the target MOM object");
         Reflection addedValue = recorders[2].reflections.stream().skip(reflectionBefore)
            .filter(reflection -> reflection.object.equals(watcherTarget.object)
               && reflection.values.containsKey(watcherAddedAttributeResolved))
            .findFirst().orElseThrow(
               () -> new AssertionError("watcher did not receive the target attribute update"));
         HLAinteger32BE decoded = factory.getEncoderFactory().createHLAinteger32BE();
         decoded.decode(addedValue.values.get(watcherAddedAttributeResolved));
         JavaTckSupport.check(watcherMomClass.equals(
               watcher.getKnownObjectClassHandle(addedValue.object))
               && watcher.getObjectInstanceName(addedValue.object).equals(watcherTargetName)
               && Arrays.equals(addedValue.tag, updateTag)
               && decoded.getValue() == EXPECTED_VALUE,
            "FOM-added value, tag, class, or target MOM object identity was not preserved");

         updater.unsubscribeObjectClassAttributes(updaterMomClass, updaterSubscribedAttributes);
         updaterSubscribed = false;
         watcher.unsubscribeObjectClassAttributes(watcherMomClass, watcherSubscribedAttributes);
         watcherSubscribed = false;
         updater.resignFederationExecution(ResignAction.NO_ACTION);
         joined[1] = false;
         watcher.resignFederationExecution(ResignAction.NO_ACTION);
         joined[2] = false;
         target.resignFederationExecution(ResignAction.NO_ACTION);
         joined[0] = false;
         target.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (updaterSubscribed) try {
            updater.unsubscribeObjectClassAttributes(updaterMomClass, updaterSubscribedAttributes);
         } catch (Exception ignored) { }
         if (watcherSubscribed) try {
            watcher.unsubscribeObjectClassAttributes(watcherMomClass, watcherSubscribedAttributes);
         } catch (Exception ignored) { }
         for (int index : new int[] {1, 2, 0}) {
            if (joined[index]) try {
               ambassadors[index].resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) { }
         }
         if (created) try { target.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         for (int index : new int[] {2, 1, 0}) {
            if (connected[index]) try { ambassadors[index].disconnect(); }
               catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            ambassador.evokeMultipleCallbacks(0.0, 1.0);
         } else {
            ambassador.getObjectClassHandle(MOM_CLASS);
         }
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private interface Condition {
      boolean isTrue() throws Exception;
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values;
      private final byte[] tag;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap source,
            byte[] tag) {
         this.object = object;
         this.values = new java.util.HashMap<>();
         for (AttributeHandle handle : source.keySet()) this.values.put(handle, source.get(handle).clone());
         this.tag = tag == null ? new byte[0] : tag.clone();
      }
   }

   private static class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final HashSet<AttributeHandle> attributes;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
      }

      boolean matches(ObjectInstanceHandle expectedObject, AttributeHandle expectedAttribute) {
         return object.equals(expectedObject) && attributes.equals(new HashSet<>(List.of(expectedAttribute)));
      }

      private String describe() {
         return "object=" + object + ", attributes=" + attributes;
      }
   }

   private static final class AcquisitionEvent extends OwnershipEvent {
      private final byte[] tag;

      private AcquisitionEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         super(object, attributes);
         this.tag = tag == null ? new byte[0] : tag.clone();
      }

      private boolean matches(ObjectInstanceHandle expectedObject, AttributeHandle expectedAttribute,
            byte[] expectedTag) {
         return matches(expectedObject, expectedAttribute) && Arrays.equals(tag, expectedTag);
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> notOwned = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> ownedByRti = new CopyOnWriteArrayList<>();
      private final List<AcquisitionEvent> acquisitions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reflectAttributeValues".equals(name)) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2]));
               } else if ("attributeIsNotOwned".equals(name)) {
                  notOwned.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               } else if ("attributeIsOwnedByRTI".equals(name)) {
                  ownedByRti.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)) {
                  acquisitions.add(new AcquisitionEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK MOM extension callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private Optional<Reflection> momObjectForHandle(AttributeHandle attribute, byte[] expected) {
         return reflections.stream()
            .filter(reflection -> reflection.values.containsKey(attribute)
               && Arrays.equals(expected, reflection.values.get(attribute)))
            .findFirst();
      }
   }
}
