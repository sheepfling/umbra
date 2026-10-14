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
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

/** Preserve unserviced discovery and ownership-assumption work through save/restore. */
final class FederationRestoreOwnershipAssumptionTck {
   private static final byte[] TAG = new byte[] {(byte) 0xd4, 0x31, 0x7b, 0x0a};

   private FederationRestoreOwnershipAssumptionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel model) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerEvents = new Recorder();
      Recorder memberEvents = new Recorder();
      String federation = "java-tck-restore-assumption-" + UUID.randomUUID();
      String ownerName = "java-tck-restore-assumption-owner";
      String memberName = "java-tck-restore-assumption-member";
      String saveLabel = "java-tck-restore-assumption-save";
      boolean ownerConnected = false, memberConnected = false, created = false;
      boolean ownerJoined = false, memberJoined = false;
      try {
         owner.connect(ownerEvents.proxy(), model);
         ownerConnected = true;
         member.connect(memberEvents.proxy(), model);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(ownerName, federation);
         ownerJoined = true;
         FederateHandle memberHandle = member.joinFederationExecution(memberName, federation);
         memberJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle memberClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         JavaTckSupport.check(ownerClass.equals(memberClass)
               && ownerAttribute.equals(memberAttribute),
            "restore ownership-assumption lookup returned inconsistent standard handles");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet memberAttributes = JavaTckSupport.attributeSet(member, memberAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         member.subscribeObjectClassAttributes(memberClass, memberAttributes);
         member.publishObjectClassAttributes(memberClass, memberAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);

         if (model == CallbackModel.HLA_IMMEDIATE) {
            await(member, model, objectClassName,
               () -> memberEvents.discoveries.contains(object), "initial object discovery");
            owner.disableCallbacks();
            member.disableCallbacks();
         }
         owner.unconditionalAttributeOwnershipDivestiture(
            object, ownerAttributes, TAG.clone());
         JavaTckSupport.check(memberEvents.assumptions.isEmpty(),
            "ownership-assumption callback was serviced before the save");
         if (model == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(memberEvents.discoveries.isEmpty(),
               "evoked object discovery was serviced before save/restore");
         }

         owner.requestFederationSave(saveLabel);
         if (model == CallbackModel.HLA_EVOKED) {
            await(owner, model, objectClassName,
               () -> ownerEvents.saveInitiations.get() == 1, "owner save initiation");
         }
         owner.federateSaveBegun();
         member.federateSaveBegun();
         owner.federateSaveComplete();
         member.federateSaveComplete();
         if (model == CallbackModel.HLA_EVOKED) {
            await(owner, model, objectClassName,
               () -> ownerEvents.saved.get() == 1, "owner federationSaved callback");
         } else {
            JavaTckSupport.check(ownerEvents.saved.get() == 0
                  && memberEvents.saved.get() == 0,
               "disabled callbacks exposed federationSaved");
         }
         JavaTckSupport.check(memberEvents.assumptions.isEmpty(),
            "pending ownership-assumption callback was lost or delivered during save");

         owner.requestFederationRestore(saveLabel);
         if (model == CallbackModel.HLA_EVOKED) {
            await(owner, model, objectClassName,
               () -> ownerEvents.restoreRequestSuccesses.size() == 1
                  && ownerEvents.restoreBegun.get() == 1
                  && ownerEvents.restoreInitiations.size() == 1,
               "owner restore initiation");
         }
         owner.federateRestoreComplete();
         member.federateRestoreComplete();
         if (model == CallbackModel.HLA_EVOKED) {
            await(owner, model, objectClassName,
               () -> ownerEvents.restored.get() == 1, "owner federationRestored callback");
         } else {
            JavaTckSupport.check(ownerEvents.restored.get() == 0
                  && memberEvents.restored.get() == 0,
               "disabled callbacks exposed federationRestored");
            owner.enableCallbacks();
            member.enableCallbacks();
         }

         JavaTckSupport.check(memberEvents.discoveries.isEmpty()
               && memberEvents.assumptions.isEmpty(),
            "member callback queue was serviced before restore completion");
         await(member, model, objectClassName,
            () -> memberEvents.discoveries.contains(object)
               && !memberEvents.assumptions.isEmpty()
               && memberEvents.saveInitiations.get() == 1
               && memberEvents.saved.get() == 1
               && memberEvents.restoreBegun.get() == 1
               && memberEvents.restoreInitiations.size() == 1
               && memberEvents.restored.get() == 1,
            "restored discovery, ownership, and save/restore callbacks");

         OwnershipEvent assumption = lastOwnership(memberEvents.assumptions);
         JavaTckSupport.check(memberEvents.assumptions.size() == 1
               && assumption.object.equals(object)
               && new HashSet<>(memberAttributes).equals(assumption.attributes)
               && Arrays.equals(TAG, assumption.tag),
            "restored ownership-assumption callback returned incorrect metadata");
         RestoreInitiation ownerRestore = lastRestore(ownerEvents.restoreInitiations);
         RestoreInitiation memberRestore = lastRestore(memberEvents.restoreInitiations);
         JavaTckSupport.check(saveLabel.equals(ownerRestore.label)
               && ownerName.equals(ownerRestore.federateName)
               && ownerRestore.postRestoreHandle != null
               && saveLabel.equals(memberRestore.label)
               && memberName.equals(memberRestore.federateName)
               && memberRestore.postRestoreHandle != null,
            "restore initiation returned incorrect saved federate metadata");
         JavaTckSupport.check(owner.getFederateHandle(ownerName).equals(ownerRestore.postRestoreHandle)
               && member.getFederateHandle(memberName).equals(memberRestore.postRestoreHandle)
               && owner.getFederateName(ownerRestore.postRestoreHandle).equals(ownerName)
               && member.getFederateName(memberRestore.postRestoreHandle).equals(memberName),
            "post-restore federate handles did not round-trip");
         JavaTckSupport.check(ownerEvents.restoreFailures.isEmpty()
               && memberEvents.restoreFailures.isEmpty()
               && ownerEvents.restoreRequestFailures.isEmpty()
               && memberEvents.restoreRequestFailures.isEmpty(),
            "ownership-assumption restore reported a failure");
         JavaTckSupport.check(ownerEvents.restoreRequestSuccesses.size() == 1
               && saveLabel.equals(ownerEvents.restoreRequestSuccesses.get(0))
               && ownerEvents.restoreBegun.get() == 1,
            "owner restore request did not report the expected success and begun callbacks");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !member.isAttributeOwnedByFederate(object, memberAttribute),
            "restored ownership-assumption work transferred ownership prematurely");
         JavaTckSupport.check(ownerEvents.restored.get() == 1 && memberEvents.restored.get() == 1,
            "ownership callback was delivered before federation restore completed");
      } finally {
         if (memberJoined) resign(member);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (memberConnected) disconnect(member);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.test() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeMultipleCallbacks(0.0, 0.01);
         else ambassador.getObjectClassHandle(objectClassName);
         Thread.yield();
      }
      JavaTckSupport.check(condition.test(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private interface Condition { boolean test(); }

   private static RestoreInitiation lastRestore(List<RestoreInitiation> values) {
      return values.get(values.size() - 1);
   }

   private static OwnershipEvent lastOwnership(List<OwnershipEvent> values) {
      return values.get(values.size() - 1);
   }

   private static final class RestoreInitiation {
      final String label, federateName;
      final FederateHandle postRestoreHandle;
      RestoreInitiation(Object[] args) {
         label = (String) args[0];
         federateName = (String) args[1];
         postRestoreHandle = (FederateHandle) args[2];
      }
   }

   private static final class OwnershipEvent {
      final ObjectInstanceHandle object;
      final HashSet<AttributeHandle> attributes;
      final byte[] tag;
      OwnershipEvent(Object[] args) {
         object = (ObjectInstanceHandle) args[0];
         attributes = new HashSet<>((AttributeHandleSet) args[1]);
         tag = ((byte[]) args[2]).clone();
      }
   }

   private static final class Recorder {
      final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      final List<OwnershipEvent> assumptions = new CopyOnWriteArrayList<>();
      final List<RestoreInitiation> restoreInitiations = new CopyOnWriteArrayList<>();
      final List<String> restoreFailures = new CopyOnWriteArrayList<>();
      final List<String> restoreRequestFailures = new CopyOnWriteArrayList<>();
      final List<String> restoreRequestSuccesses = new CopyOnWriteArrayList<>();
      final AtomicInteger saveInitiations = new AtomicInteger();
      final AtomicInteger saved = new AtomicInteger();
      final AtomicInteger restoreBegun = new AtomicInteger();
      final AtomicInteger restored = new AtomicInteger();
      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               Object[] a = args == null ? new Object[0] : args;
               switch (method.getName()) {
                  case "discoverObjectInstance": discoveries.add((ObjectInstanceHandle) a[0]); break;
                  case "requestAttributeOwnershipAssumption": assumptions.add(new OwnershipEvent(a)); break;
                  case "initiateFederateSave": saveInitiations.incrementAndGet(); break;
                  case "federationSaved": saved.incrementAndGet(); break;
                  case "federationRestoreBegun": restoreBegun.incrementAndGet(); break;
                  case "requestFederationRestoreSucceeded":
                     restoreRequestSuccesses.add((String) a[0]); break;
                  case "initiateFederateRestore": restoreInitiations.add(new RestoreInitiation(a)); break;
                  case "federationRestored": restored.incrementAndGet(); break;
                  case "federationNotRestored": restoreFailures.add(String.valueOf(a[0])); break;
                  case "requestFederationRestoreFailed": restoreRequestFailures.add(String.valueOf(a[0])); break;
                  case "toString": return "FederationRestoreOwnershipAssumptionTck.Recorder";
                  case "hashCode": return System.identityHashCode(proxy);
                  case "equals": return proxy == a[0];
                  default: break;
               }
               return null;
            });
      }
   }
}
