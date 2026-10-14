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
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Resolves regular, If Available, and assumption flows through one unconditional divestiture. */
final class UnconditionalAttributeOwnershipDivestitureTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private UnconditionalAttributeOwnershipDivestitureTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String identityName, String integerName,
         String asciiName, String unicodeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador regular = factory.getRtiAmbassador();
      RTIambassador ifAvailable = factory.getRtiAmbassador();
      RTIambassador invited = factory.getRtiAmbassador();
      RTIambassador unpublished = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder regularRecorder = new Recorder();
      Recorder ifAvailableRecorder = new Recorder();
      Recorder invitedRecorder = new Recorder();
      Recorder unpublishedRecorder = new Recorder();
      String federation = "java-tck-unconditional-ownership-divestiture-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean regularConnected = false;
      boolean ifAvailableConnected = false;
      boolean invitedConnected = false;
      boolean unpublishedConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean regularJoined = false;
      boolean ifAvailableJoined = false;
      boolean invitedJoined = false;
      boolean unpublishedJoined = false;
      boolean ownerPublished = false;
      boolean regularPublished = false;
      boolean ifAvailablePublished = false;
      boolean invitedPublished = false;
      boolean regularSubscribed = false;
      boolean ifAvailableSubscribed = false;
      boolean invitedSubscribed = false;
      boolean unpublishedSubscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle regularClass = null;
      ObjectClassHandle ifAvailableClass = null;
      ObjectClassHandle invitedClass = null;
      ObjectClassHandle unpublishedClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet regularAttributes = null;
      AttributeHandleSet ifAvailableAttributes = null;
      AttributeHandleSet invitedAttributes = null;
      AttributeHandleSet unpublishedAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         regular.connect(regularRecorder.proxy(), callbackModel);
         regularConnected = true;
         ifAvailable.connect(ifAvailableRecorder.proxy(), callbackModel);
         ifAvailableConnected = true;
         invited.connect(invitedRecorder.proxy(), callbackModel);
         invitedConnected = true;
         unpublished.connect(unpublishedRecorder.proxy(), callbackModel);
         unpublishedConnected = true;

         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-unconditional-divest-owner", federation);
         ownerJoined = true;
         regular.joinFederationExecution("java-tck-unconditional-divest-regular", federation);
         regularJoined = true;
         ifAvailable.joinFederationExecution(
            "java-tck-unconditional-divest-if-available", federation);
         ifAvailableJoined = true;
         invited.joinFederationExecution("java-tck-unconditional-divest-invited", federation);
         invitedJoined = true;
         unpublished.joinFederationExecution(
            "java-tck-unconditional-divest-unpublished", federation);
         unpublishedJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         regularClass = regular.getObjectClassHandle(objectClassName);
         ifAvailableClass = ifAvailable.getObjectClassHandle(objectClassName);
         invitedClass = invited.getObjectClassHandle(objectClassName);
         unpublishedClass = unpublished.getObjectClassHandle(objectClassName);
         AttributeHandle ownerIdentity = owner.getAttributeHandle(ownerClass, identityName);
         AttributeHandle ownerInteger = owner.getAttributeHandle(ownerClass, integerName);
         AttributeHandle ownerAscii = owner.getAttributeHandle(ownerClass, asciiName);
         AttributeHandle ownerUnicode = owner.getAttributeHandle(ownerClass, unicodeName);
         AttributeHandle regularIdentity = regular.getAttributeHandle(regularClass, identityName);
         AttributeHandle ifAvailableInteger = ifAvailable.getAttributeHandle(
            ifAvailableClass, integerName);
         AttributeHandle invitedAscii = invited.getAttributeHandle(invitedClass, asciiName);
         AttributeHandle invitedUnicode = invited.getAttributeHandle(invitedClass, unicodeName);
         AttributeHandle unpublishedAscii = unpublished.getAttributeHandle(
            unpublishedClass, asciiName);
         AttributeHandle unpublishedUnicode = unpublished.getAttributeHandle(
            unpublishedClass, unicodeName);
         JavaTckSupport.check(ownerClass != null && regularClass != null
               && ifAvailableClass != null && invitedClass != null && unpublishedClass != null
               && ownerIdentity != null && ownerInteger != null && ownerAscii != null
               && ownerUnicode != null && regularIdentity != null
               && ifAvailableInteger != null && invitedAscii != null && invitedUnicode != null
               && unpublishedAscii != null && unpublishedUnicode != null,
            "adapter model FOM did not resolve all ownership test handles");
         JavaTckSupport.check(!ownerIdentity.equals(ownerInteger)
               && !ownerInteger.equals(ownerAscii) && !ownerAscii.equals(ownerUnicode),
            "adapter model FOM returned duplicate ownership attributes");

         ownerAttributes = JavaTckSupport.attributeSet(
            owner, ownerIdentity, ownerInteger, ownerAscii, ownerUnicode);
         regularAttributes = JavaTckSupport.attributeSet(regular, regularIdentity);
         ifAvailableAttributes = JavaTckSupport.attributeSet(ifAvailable, ifAvailableInteger);
         invitedAttributes = JavaTckSupport.attributeSet(invited, invitedAscii, invitedUnicode);
         unpublishedAttributes = JavaTckSupport.attributeSet(
            unpublished, unpublishedAscii, unpublishedUnicode);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         regular.publishObjectClassAttributes(regularClass, regularAttributes);
         regularPublished = true;
         ifAvailable.publishObjectClassAttributes(ifAvailableClass, ifAvailableAttributes);
         ifAvailablePublished = true;
         invited.publishObjectClassAttributes(invitedClass, invitedAttributes);
         invitedPublished = true;
         regular.subscribeObjectClassAttributes(regularClass, regularAttributes);
         regularSubscribed = true;
         ifAvailable.subscribeObjectClassAttributes(ifAvailableClass, ifAvailableAttributes);
         ifAvailableSubscribed = true;
         invited.subscribeObjectClassAttributes(invitedClass, invitedAttributes);
         invitedSubscribed = true;
         unpublished.subscribeObjectClassAttributes(unpublishedClass, unpublishedAttributes);
         unpublishedSubscribed = true;

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         awaitSessions(new RTIambassador[] {regular, ifAvailable, invited, unpublished},
            callbackModel,
            () -> regularRecorder.discoveries.contains(object)
               && ifAvailableRecorder.discoveries.contains(object)
               && invitedRecorder.discoveries.contains(object)
               && unpublishedRecorder.discoveries.contains(object),
            "ownership-test object discovery");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerIdentity)
               && owner.isAttributeOwnedByFederate(object, ownerInteger)
               && owner.isAttributeOwnedByFederate(object, ownerAscii)
               && owner.isAttributeOwnedByFederate(object, ownerUnicode),
            "owner did not initially own all four attributes");

         byte[] regularTag = new byte[] {(byte) 0xa2, 0x07, 0x20};
         byte[] ifAvailableTag = new byte[] {(byte) 0xb2, 0x07, 0x20};
         byte[] assumptionTag = new byte[] {(byte) 0xd2, 0x07, 0x20};
         byte[] invitedTag = new byte[] {(byte) 0xc2, 0x07, 0x20};
         regularRecorder.clearOwnershipEvents();
         ifAvailableRecorder.clearOwnershipEvents();
         invitedRecorder.clearOwnershipEvents();
         unpublishedRecorder.clearOwnershipEvents();
         regular.attributeOwnershipAcquisition(object, regularAttributes, regularTag);
         ifAvailable.attributeOwnershipAcquisitionIfAvailable(
            object, ifAvailableAttributes, ifAvailableTag);
         awaitSessions(new RTIambassador[] {owner, regular, ifAvailable}, callbackModel,
            () -> ownerRecorder.releaseRequests.size() == 1
               && ifAvailableRecorder.unavailable.size() == 1,
            "regular release request and initial If Available rejection");
         checkEvent(ownerRecorder.releaseRequests.get(0), object, regularAttributes,
            regularTag, "regular acquisition release request");
         checkEvent(ifAvailableRecorder.unavailable.get(0), object, ifAvailableAttributes,
            ifAvailableTag, "initial If Available rejection");
         JavaTckSupport.check(!ifAvailable.isAttributeOwnedByFederate(object,
               ifAvailableInteger),
            "initial If Available rejection changed the current owner");

         owner.unconditionalAttributeOwnershipDivestiture(object, ownerAttributes, assumptionTag);
         awaitSessions(new RTIambassador[] {regular, invited}, callbackModel,
            () -> regularRecorder.acquisitions.size() == 1
               && invitedRecorder.assumptions.size() == 1,
            "regular acquisition and invited assumption callbacks");
         checkEvent(regularRecorder.acquisitions.get(0), object, regularAttributes,
            regularTag, "regular acquisition after unconditional divestiture");
         checkEvent(invitedRecorder.assumptions.get(0), object, invitedAttributes,
            assumptionTag, "ownership assumption after unconditional divestiture");
         JavaTckSupport.check(ifAvailableRecorder.acquisitions.isEmpty()
               && unpublishedRecorder.assumptions.isEmpty(),
            "unconditional divestiture bypassed the rejected request or invited an unpublished subscriber");
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerIdentity)
               && !owner.isAttributeOwnedByFederate(object, ownerInteger)
               && !owner.isAttributeOwnedByFederate(object, ownerAscii)
               && !owner.isAttributeOwnedByFederate(object, ownerUnicode)
               && regular.isAttributeOwnedByFederate(object, regularIdentity)
               && !ifAvailable.isAttributeOwnedByFederate(object,
                  ifAvailableInteger)
               && !invited.isAttributeOwnedByFederate(object,
                  invitedAscii)
               && !invited.isAttributeOwnedByFederate(object,
                  invitedUnicode),
            "unconditional divestiture established the wrong ownership split");

         ifAvailableRecorder.clearOwnershipEvents();
         ifAvailable.attributeOwnershipAcquisitionIfAvailable(
            object, ifAvailableAttributes, ifAvailableTag);
         await(ifAvailable, callbackModel, () -> ifAvailableRecorder.acquisitions.size() == 1,
            "retried If Available acquisition");
         checkEvent(ifAvailableRecorder.acquisitions.get(0), object, ifAvailableAttributes,
            ifAvailableTag, "retried If Available acquisition");
         JavaTckSupport.check(ifAvailable.isAttributeOwnedByFederate(object,
               ifAvailableInteger),
            "retried If Available acquisition did not establish ownership");

         invitedRecorder.clearOwnershipEvents();
         invited.attributeOwnershipAcquisitionIfAvailable(object, invitedAttributes, invitedTag);
         await(invited, callbackModel, () -> invitedRecorder.acquisitions.size() == 1,
            "invited candidate acquisition");
         checkEvent(invitedRecorder.acquisitions.get(0), object, invitedAttributes,
            invitedTag, "invited candidate acquisition");
         JavaTckSupport.check(invited.isAttributeOwnedByFederate(object,
               invitedAscii)
               && invited.isAttributeOwnedByFederate(object,
                  invitedUnicode),
            "invited candidate did not acquire both attributes");
      } finally {
         unsubscribeIfJoined(regular, regularJoined, regularSubscribed,
            regularClass, regularAttributes);
         unsubscribeIfJoined(ifAvailable, ifAvailableJoined, ifAvailableSubscribed,
            ifAvailableClass, ifAvailableAttributes);
         unsubscribeIfJoined(invited, invitedJoined, invitedSubscribed,
            invitedClass, invitedAttributes);
         unsubscribeIfJoined(unpublished, unpublishedJoined, unpublishedSubscribed,
            unpublishedClass, unpublishedAttributes);
         unpublishIfJoined(owner, ownerJoined, ownerPublished, ownerClass, ownerAttributes);
         unpublishIfJoined(regular, regularJoined, regularPublished, regularClass, regularAttributes);
         unpublishIfJoined(ifAvailable, ifAvailableJoined, ifAvailablePublished,
            ifAvailableClass, ifAvailableAttributes);
         unpublishIfJoined(invited, invitedJoined, invitedPublished, invitedClass, invitedAttributes);
         resignIfJoined(invited, invitedJoined);
         resignIfJoined(ifAvailable, ifAvailableJoined);
         resignIfJoined(regular, regularJoined);
         resignIfJoined(unpublished, unpublishedJoined);
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (unpublishedConnected) { try { unpublished.disconnect(); } catch (Exception ignored) { } }
         if (invitedConnected) { try { invited.disconnect(); } catch (Exception ignored) { } }
         if (ifAvailableConnected) { try { ifAvailable.disconnect(); } catch (Exception ignored) { } }
         if (regularConnected) { try { regular.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void checkEvent(OwnershipEvent event, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(event.object) && event.attributes.equals(attributes)
            && Arrays.equals(tag, event.tag),
         description + " returned the wrong object, attributes, or tag");
   }

   private static void unsubscribeIfJoined(RTIambassador ambassador, boolean joined,
         boolean subscribed, ObjectClassHandle objectClass, AttributeHandleSet attributes) {
      if (joined && subscribed) {
         try { ambassador.unsubscribeObjectClassAttributes(objectClass, attributes); }
         catch (Exception ignored) { }
      }
   }

   private static void unpublishIfJoined(RTIambassador ambassador, boolean joined,
         boolean published, ObjectClassHandle objectClass, AttributeHandleSet attributes) {
      if (joined && published) {
         try { ambassador.unpublishObjectClassAttributes(objectClass, attributes); }
         catch (Exception ignored) { }
      }
   }

   private static void resignIfJoined(RTIambassador ambassador, boolean joined) {
      if (joined) {
         try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
         catch (Exception ignored) { }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      awaitSessions(new RTIambassador[] {ambassador}, callbackModel, condition, description);
   }

   private static void awaitSessions(RTIambassador[] ambassadors, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            for (RTIambassador ambassador : ambassadors) JavaTckSupport.drain(ambassador);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;

      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag == null ? null : tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> releaseRequests = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> unavailable = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> acquisitions = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> assumptions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestAttributeOwnershipRelease".equals(name)
                     && arguments != null && arguments.length == 3) {
                  releaseRequests.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("attributeOwnershipUnavailable".equals(name)
                     && arguments != null && arguments.length == 3) {
                  unavailable.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("requestAttributeOwnershipAssumption".equals(name)
                     && arguments != null && arguments.length == 3) {
                  assumptions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK unconditional ownership-divestiture callbacks";
                  }
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }

      private void clearOwnershipEvents() {
         releaseRequests.clear();
         unavailable.clear();
         acquisitions.clear();
         assumptions.clear();
      }
   }
}
