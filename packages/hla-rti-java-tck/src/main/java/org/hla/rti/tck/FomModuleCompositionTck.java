package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** Verifies extension declarations compose at Create and at a later Join. */
final class FomModuleCompositionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private FomModuleCompositionTck() { }

   static void run(RtiFactory factory, String baseFom, String extensionFom,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      createComposition(factory, baseFom, extensionFom, timeImplementation,
         objectClassName, attributeName, interactionClassName, parameterName, callbackModel);
      joinComposition(factory, baseFom, extensionFom, timeImplementation, objectClassName,
         callbackModel);
   }

   private static void createComposition(RtiFactory factory, String baseFom,
         String extensionFom, String timeImplementation, String objectClassName,
         String attributeName, String interactionClassName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-fom-composed-create-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         member.connect(recorder.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation,
            new String[] {baseFom, extensionFom}, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-fom-composed-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-fom-composed-member", federation);
         memberJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         InteractionClassHandle ownerInteraction =
            owner.getInteractionClassHandle(interactionClassName);
         ParameterHandle ownerParameter =
            owner.getParameterHandle(ownerInteraction, parameterName);
         ObjectClassHandle memberClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         InteractionClassHandle memberInteraction =
            member.getInteractionClassHandle(interactionClassName);
         ParameterHandle memberParameter =
            member.getParameterHandle(memberInteraction, parameterName);
         JavaTckSupport.check(ownerClass.equals(memberClass)
               && ownerAttribute.equals(memberAttribute)
               && ownerInteraction.equals(memberInteraction)
               && ownerParameter.equals(memberParameter),
            "Create-time FOM composition did not expose shared declaration handles");

         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         member.subscribeObjectClassAttributes(memberClass,
            JavaTckSupport.attributeSet(member, memberAttribute));
         owner.publishInteractionClass(ownerInteraction);
         member.subscribeInteractionClass(memberInteraction);

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         await(member, callbackModel, () -> object.equals(recorder.discoveredObject),
            "extension object discovery");
         JavaTckSupport.check(object.equals(recorder.discoveredObject)
               && memberClass.equals(recorder.discoveredClass),
            "Create-time FOM composition discovery returned the wrong object or class");

         byte[] payload = new byte[] {0x31, 0x52, (byte) 0xe1};
         byte[] tag = new byte[] {0x6a, 0x09};
         AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
         values.put(ownerAttribute, payload);
         owner.updateAttributeValues(object, values, tag);
         await(member, callbackModel, () -> recorder.reflectedValues != null,
            "extension attribute reflection");
         JavaTckSupport.check(object.equals(recorder.reflectedObject)
               && recorder.reflectedValues.size() == 1
               && Arrays.equals(payload, recorder.reflectedValues.get(memberAttribute))
               && Arrays.equals(tag, recorder.reflectionTag),
            "Create-time FOM composition changed extension attribute data or tag");

         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(ownerParameter, payload);
         owner.sendInteraction(ownerInteraction, parameters, tag);
         await(member, callbackModel, () -> recorder.receivedParameters != null,
            "extension interaction delivery");
         JavaTckSupport.check(memberInteraction.equals(recorder.receivedInteraction)
               && recorder.receivedParameters.size() == 1
               && Arrays.equals(payload, recorder.receivedParameters.get(memberParameter))
               && Arrays.equals(tag, recorder.interactionTag),
            "Create-time FOM composition changed extension interaction data or tag");
      } finally {
         if (memberJoined) resign(member, ResignAction.NO_ACTION);
         if (ownerJoined) resign(owner, ResignAction.DELETE_OBJECTS);
         if (created) destroy(owner, federation);
         if (memberConnected) disconnect(member);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void joinComposition(RtiFactory factory, String baseFom,
         String extensionFom, String timeImplementation, String extensionObjectClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador extension = factory.getRtiAmbassador();
      String federation = "java-tck-fom-composed-join-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean extensionConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean extensionJoined = false;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         extension.connect(callbacks(), callbackModel);
         extensionConnected = true;
         owner.createFederationExecution(federation, baseFom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-fom-base-owner", federation);
         ownerJoined = true;
         JavaTckSupport.expectFailure(
            () -> owner.getObjectClassHandle(extensionObjectClassName), "NameNotFound",
            "extension object-class lookup before the additional-module join");

         extension.joinFederationExecution("java-tck-fom-extension-joiner",
            "java-tck-fom-extension-joiner", federation, new String[] {extensionFom});
         extensionJoined = true;
         ObjectClassHandle ownerExtension = owner.getObjectClassHandle(extensionObjectClassName);
         ObjectClassHandle memberExtension =
            extension.getObjectClassHandle(extensionObjectClassName);
         JavaTckSupport.check(ownerExtension.equals(memberExtension)
               && extensionObjectClassName.equals(owner.getObjectClassName(ownerExtension))
               && extensionObjectClassName.equals(extension.getObjectClassName(memberExtension)),
            "additional-module Join did not expose one shared extension class to all members");
      } finally {
         if (extensionJoined) resign(extension, ResignAction.NO_ACTION);
         if (ownerJoined) resign(owner, ResignAction.NO_ACTION);
         if (created) destroy(owner, federation);
         if (extensionConnected) disconnect(extension);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void await(RTIambassador member, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(member);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK FOM composition callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static void destroy(RTIambassador ambassador, String federation) {
      try { ambassador.destroyFederationExecution(federation); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static final class Recorder {
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile ObjectClassHandle discoveredClass;
      private volatile ObjectInstanceHandle reflectedObject;
      private volatile AttributeHandleValueMap reflectedValues;
      private volatile byte[] reflectionTag;
      private volatile InteractionClassHandle receivedInteraction;
      private volatile ParameterHandleValueMap receivedParameters;
      private volatile byte[] interactionTag;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
                  discoveredClass = (ObjectClassHandle) arguments[1];
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  reflectedObject = (ObjectInstanceHandle) arguments[0];
                  reflectedValues = ((AttributeHandleValueMap) arguments[1]).clone();
                  if (arguments.length >= 3 && arguments[2] instanceof byte[]) {
                     reflectionTag = ((byte[]) arguments[2]).clone();
                  }
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  receivedInteraction = (InteractionClassHandle) arguments[0];
                  receivedParameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  if (arguments.length >= 3 && arguments[2] instanceof byte[]) {
                     interactionTag = ((byte[]) arguments[2]).clone();
                  }
               }
               return objectMethod(proxy, name, arguments);
            });
      }
   }
}
