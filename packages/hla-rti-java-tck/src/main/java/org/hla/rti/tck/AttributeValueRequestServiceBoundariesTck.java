package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies ordinary value requests and supplemental handle-context failures. */
final class AttributeValueRequestServiceBoundariesTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private AttributeValueRequestServiceBoundariesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder memberRecorder = new Recorder();
      String federation = "java-tck-attribute-value-request-boundaries-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle memberClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet memberAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberRecorder.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-value-request-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-value-request-member", federation);
         memberJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         memberClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         JavaTckSupport.check(ownerClass != null && memberClass != null
               && ownerAttribute != null && memberAttribute != null
               && objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(member.getObjectClassName(memberClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute))
               && attributeName.equals(member.getAttributeName(memberClass, memberAttribute)),
            "adapter FOM did not resolve value-request object class and attribute");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         memberAttributes = JavaTckSupport.attributeSet(member, memberAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         member.subscribeObjectClassAttributes(memberClass, memberAttributes);
         subscribed = true;

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         String objectName = owner.getObjectInstanceName(object);
         await(member, callbackModel, () -> memberRecorder.discoveries.size() == 1,
            "value-request object discovery");
         ObjectInstanceHandle memberObject = member.getObjectInstanceHandle(objectName);
         JavaTckSupport.check(memberObject.equals(memberRecorder.discoveries.get(0)),
            "value-request discovery returned the wrong object");

         byte[] instanceRequestTag = new byte[] {0x31, 0x32};
         byte[] instanceValue = new byte[] {0x41, 0x42, 0x43};
         byte[] instanceUpdateTag = new byte[] {0x51, 0x52};
         ownerRecorder.provided.clear();
         member.requestAttributeValueUpdate(
            memberObject, memberAttributes, instanceRequestTag);
         await(owner, callbackModel, () -> ownerRecorder.provided.size() == 1,
            "instance value-request provider callback");
         checkProvided(ownerRecorder.provided.get(0), object, ownerAttributes,
            instanceRequestTag, "instance value-request provider callback");
         respond(owner, member, memberRecorder, object, memberObject,
            ownerAttribute, memberAttribute,
            instanceValue, instanceUpdateTag, callbackModel, "instance value-request response");

         byte[] classRequestTag = new byte[] {0x61, 0x62};
         byte[] classValue = new byte[] {0x71, 0x72, 0x73};
         byte[] classUpdateTag = new byte[] {(byte) 0x81, (byte) 0x82};
         ownerRecorder.provided.clear();
         member.requestAttributeValueUpdate(memberClass, memberAttributes, classRequestTag);
         await(owner, callbackModel, () -> ownerRecorder.provided.size() == 1,
            "class value-request provider callback");
         checkProvided(ownerRecorder.provided.get(0), object, ownerAttributes,
            classRequestTag, "class value-request provider callback");
         respond(owner, member, memberRecorder, object, memberObject,
            ownerAttribute, memberAttribute,
            classValue, classUpdateTag, callbackModel, "class value-request response");
      } finally {
         if (memberJoined && subscribed) {
            try { member.unsubscribeObjectClassAttributes(memberClass, memberAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && published) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (memberConnected) { try { member.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   static void runInvalidHandleContexts(RtiFactory factory, String fom, String additionalFom,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      RTIambassador foreign = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder memberRecorder = new Recorder();
      Recorder foreignRecorder = new Recorder();
      String federation = "java-tck-value-request-context-errors-" + UUID.randomUUID();
      String foreignFederation = federation + "-foreign";
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean foreignConnected = false;
      boolean created = false;
      boolean foreignCreated = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      boolean foreignJoined = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle memberClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet memberAttributes = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberRecorder.proxy(), callbackModel);
         memberConnected = true;
         foreign.connect(foreignRecorder.proxy(), callbackModel);
         foreignConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         foreign.createFederationExecution(foreignFederation, additionalFom, timeImplementation);
         foreignCreated = true;
         owner.joinFederationExecution("java-tck-value-request-context-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-value-request-context-member", federation);
         memberJoined = true;
         foreign.joinFederationExecution("java-tck-value-request-context-foreign",
            foreignFederation);
         foreignJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         memberClass = member.getObjectClassHandle(objectClassName);
         ObjectClassHandle foreignClass =
            foreign.getObjectClassHandle("HLAobjectRoot.TckExtensionObject");
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         AttributeHandle foreignAttribute =
            foreign.getAttributeHandle(foreignClass, "ExtensionValue");
         JavaTckSupport.check(ownerClass != null && memberClass != null
               && ownerAttribute != null && memberAttribute != null
               && foreignClass != null && foreignAttribute != null
               && objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(member.getObjectClassName(memberClass))
               && "HLAobjectRoot.TckExtensionObject".equals(
                  foreign.getObjectClassName(foreignClass))
               && "ExtensionValue".equals(
                  foreign.getAttributeName(foreignClass, foreignAttribute))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute))
               && attributeName.equals(member.getAttributeName(memberClass, memberAttribute)),
            "adapter FOM did not resolve the primary/foreign value-request handles");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         memberAttributes = JavaTckSupport.attributeSet(member, memberAttribute);
         AttributeHandleSet contextInvalidAttributes =
            member.getAttributeHandleSetFactory().create();
         contextInvalidAttributes.add(foreignAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         member.subscribeObjectClassAttributes(memberClass, memberAttributes);
         subscribed = true;

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         String objectName = owner.getObjectInstanceName(object);
         await(member, callbackModel, () -> memberRecorder.discoveries.size() == 1,
            "value-request context-error object discovery");
         ObjectInstanceHandle memberObject = member.getObjectInstanceHandle(objectName);
         JavaTckSupport.check(memberObject.equals(memberRecorder.discoveries.get(0)),
            "value-request context-error discovery returned the wrong object");

         ObjectClassHandle invalidObjectClass = foreignClass;
         ObjectClassHandle validMemberClass = memberClass;
         AttributeHandleSet validMemberAttributes = memberAttributes;
         JavaTckSupport.expectFailure(
            () -> member.requestAttributeValueUpdate(
               invalidObjectClass, validMemberAttributes, new byte[0]),
            "ObjectClassNotDefined",
            "class value request with an object class from a different FOM execution");
         JavaTckSupport.expectFailure(
            () -> member.requestAttributeValueUpdate(
               validMemberClass, contextInvalidAttributes, new byte[0]),
            "AttributeNotDefined",
            "class value request with an attribute defined on another object class");
         JavaTckSupport.expectFailure(
            () -> member.requestAttributeValueUpdate(
               memberObject, contextInvalidAttributes, new byte[0]),
            "AttributeNotDefined",
            "instance value request with an attribute defined on another object class");
      } finally {
         if (memberJoined && subscribed) {
            try { member.unsubscribeObjectClassAttributes(memberClass, memberAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && published) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (foreignJoined) {
            try { foreign.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (foreignCreated) {
            try { foreign.destroyFederationExecution(foreignFederation); }
            catch (Exception ignored) { }
         }
         if (memberConnected) { try { member.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
         if (foreignConnected) { try { foreign.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void checkProvided(ProvidedUpdate update, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String description) {
      JavaTckSupport.check(object.equals(update.object)
            && new HashSet<>(attributes).equals(update.attributes)
            && Arrays.equals(tag, update.tag),
         description + " returned the wrong object, attributes, or tag");
   }

   private static void respond(RTIambassador owner, RTIambassador member, Recorder recorder,
         ObjectInstanceHandle ownerObject, ObjectInstanceHandle memberObject,
         AttributeHandle ownerAttribute,
         AttributeHandle memberAttribute, byte[] value, byte[] tag,
         CallbackModel callbackModel, String description) throws Exception {
      recorder.reflections.clear();
      AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
      values.put(ownerAttribute, value);
      owner.updateAttributeValues(ownerObject, values, tag);
      await(member, callbackModel, () -> recorder.reflections.size() == 1,
         description + " reflection");
      Reflection reflection = recorder.reflections.get(0);
      JavaTckSupport.check(memberObject.equals(reflection.object)
            && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(memberAttribute))
            && Arrays.equals(tag, reflection.tag),
         description + " returned the wrong object, attribute value, or update tag");
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

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<ProvidedUpdate> provided = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("provideAttributeValueUpdate".equals(name)
                     && arguments != null && arguments.length == 3) {
                  provided.add(new ProvidedUpdate((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (byte[]) arguments[2]));
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length >= 5) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) arguments[0];
                  reflection.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  reflection.tag = ((byte[]) arguments[2]).clone();
                  reflections.add(reflection);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK attribute value-request callbacks";
                  }
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
