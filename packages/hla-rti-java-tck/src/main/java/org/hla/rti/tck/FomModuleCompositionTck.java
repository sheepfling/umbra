package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.io.File;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** Verifies extension declarations compose at Create and at a later Join. */
final class FomModuleCompositionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String[] BASE_OBJECT_CLASS_NAMES = {
      "HLAobjectRoot",
      "HLAobjectRoot.TckObject",
      "HLAobjectRoot.TckObject.TckSecondaryObject"
   };
   private static final String[] COMPOSED_OBJECT_CLASS_NAMES = {
      "HLAobjectRoot",
      "HLAobjectRoot.TckObject",
      "HLAobjectRoot.TckObject.TckSecondaryObject",
      "HLAobjectRoot.TckExtensionObject",
      "HLAobjectRoot.TckExtensionDimensionalObject"
   };
   private static final String[] BASE_INTERACTION_CLASS_NAMES = {
      "HLAinteractionRoot",
      "HLAinteractionRoot.TckInteraction",
      "HLAinteractionRoot.TckPreferredTimestampInteraction",
      "HLAinteractionRoot.TckPreferredReceiveInteraction",
      "HLAinteractionRoot.TckSecondaryDirectedInteraction"
   };
   private static final String[] COMPOSED_INTERACTION_CLASS_NAMES = {
      "HLAinteractionRoot",
      "HLAinteractionRoot.TckInteraction",
      "HLAinteractionRoot.TckPreferredTimestampInteraction",
      "HLAinteractionRoot.TckPreferredReceiveInteraction",
      "HLAinteractionRoot.TckSecondaryDirectedInteraction",
      "HLAinteractionRoot.TckExtensionInteraction"
   };
   private static final String[][] BASE_ATTRIBUTE_TABLE = {
      {"HLAobjectRoot.TckObject", "Name", "PayRate", "Value"}
   };
   private static final String[][] COMPOSED_ATTRIBUTE_TABLE = {
      {"HLAobjectRoot.TckObject", "Name", "PayRate", "Value"},
      {"HLAobjectRoot.TckExtensionObject", "ExtensionValue"},
      {"HLAobjectRoot.TckExtensionDimensionalObject", "AdditionalValue"}
   };
   private static final String[][] BASE_PARAMETER_TABLE = {
      {"HLAinteractionRoot.TckInteraction", "Payload"},
      {"HLAinteractionRoot.TckPreferredTimestampInteraction", "Payload"},
      {"HLAinteractionRoot.TckPreferredReceiveInteraction", "Payload"},
      {"HLAinteractionRoot.TckSecondaryDirectedInteraction", "SecondaryPayload"}
   };
   private static final String[][] COMPOSED_PARAMETER_TABLE = {
      {"HLAinteractionRoot.TckInteraction", "Payload"},
      {"HLAinteractionRoot.TckPreferredTimestampInteraction", "Payload"},
      {"HLAinteractionRoot.TckPreferredReceiveInteraction", "Payload"},
      {"HLAinteractionRoot.TckSecondaryDirectedInteraction", "SecondaryPayload"},
      {"HLAinteractionRoot.TckExtensionInteraction", "ExtensionParameter"}
   };

   private FomModuleCompositionTck() { }

   static void run(RtiFactory factory, String baseFom, String extensionFom,
         String timeImplementation, String objectClassName, String attributeName,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      createComposition(factory, baseFom, extensionFom, timeImplementation,
         objectClassName, attributeName, interactionClassName, parameterName, callbackModel);
      joinComposition(factory, baseFom, extensionFom, timeImplementation, objectClassName,
         attributeName, interactionClassName, parameterName, callbackModel);
   }

   private static void createComposition(RtiFactory factory, String baseFom,
         String extensionFom, String timeImplementation, String objectClassName,
         String attributeName, String interactionClassName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder memberRecorder = new Recorder();
      String federation = "java-tck-fom-composed-create-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberRecorder.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation,
            new String[] {fileDesignator(baseFom), fileDesignator(extensionFom)},
            timeImplementation);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            "java-tck-fom-composed-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-fom-composed-member", federation);
         memberJoined = true;

         List<ObjectClassHandle> ownerObjectClasses = verifyObjectClassTable(
            owner, COMPOSED_OBJECT_CLASS_NAMES, "create-time owner object-class table");
         List<ObjectClassHandle> memberObjectClasses = verifyObjectClassTable(
            member, COMPOSED_OBJECT_CLASS_NAMES, "create-time member object-class table");
         List<InteractionClassHandle> ownerInteractionClasses = verifyInteractionClassTable(
            owner, COMPOSED_INTERACTION_CLASS_NAMES,
            "create-time owner interaction-class table");
         List<InteractionClassHandle> memberInteractionClasses = verifyInteractionClassTable(
            member, COMPOSED_INTERACTION_CLASS_NAMES,
            "create-time member interaction-class table");
         List<AttributeHandle> ownerAttributeHandles = verifyAttributeTable(
            owner, COMPOSED_ATTRIBUTE_TABLE, "create-time owner attribute table");
         List<AttributeHandle> memberAttributeHandles = verifyAttributeTable(
            member, COMPOSED_ATTRIBUTE_TABLE, "create-time member attribute table");
         List<ParameterHandle> ownerParameters = verifyParameterTable(
            owner, COMPOSED_PARAMETER_TABLE, "create-time owner parameter table");
         List<ParameterHandle> memberParameters = verifyParameterTable(
            member, COMPOSED_PARAMETER_TABLE, "create-time member parameter table");
         JavaTckSupport.check(ownerObjectClasses.equals(memberObjectClasses)
               && ownerInteractionClasses.equals(memberInteractionClasses)
               && ownerAttributeHandles.equals(memberAttributeHandles)
               && ownerParameters.equals(memberParameters),
            "create-time FOM composition did not expose shared complete fixture tables");
         JavaTckSupport.check(!owner.getAttributeScopeAdvisorySwitch()
               && !member.getAttributeScopeAdvisorySwitch(),
            "attribute scope advisory switch did not default to disabled after Create");

         ObjectClassHandle ownerMomClass = owner.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederate");
         AttributeHandle ownerMomFederateHandle = owner.getAttributeHandle(
            ownerMomClass, "HLAfederateHandle");
         ObjectClassHandle memberMomClass = member.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederate");
         AttributeHandle memberMomFederateHandle = member.getAttributeHandle(
            memberMomClass, "HLAfederateHandle");
         JavaTckSupport.check(ownerMomClass.equals(memberMomClass)
               && ownerMomFederateHandle.equals(memberMomFederateHandle),
            "create-time FOM composition did not preserve shared standard MIM handles");

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
         await(member, callbackModel, () -> object.equals(memberRecorder.discoveredObject),
            "extension object discovery");
         JavaTckSupport.check(object.equals(memberRecorder.discoveredObject)
               && memberClass.equals(memberRecorder.discoveredClass),
            "Create-time FOM composition discovery returned the wrong object or class");

         byte[] payload = new byte[] {0, 0, 0, 123};
         byte[] tag = new byte[] {(byte) 0xe1};
         AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
         values.put(ownerAttribute, payload);
         owner.updateAttributeValues(object, values, tag);
         await(member, callbackModel, () -> memberRecorder.reflectedValues != null,
            "extension attribute reflection");
         JavaTckSupport.check(object.equals(memberRecorder.reflectedObject)
               && memberRecorder.reflectedValues.size() == 1
               && Arrays.equals(payload, memberRecorder.reflectedValues.get(memberAttribute))
               && Arrays.equals(tag, memberRecorder.reflectionTag)
               && memberRecorder.reflectedTransportation != null
               && "HLAreliable".equals(member.getTransportationTypeName(
                  memberRecorder.reflectedTransportation)),
            "Create-time FOM composition changed extension attribute data, tag, or transportation");

         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(ownerParameter, payload);
         owner.sendInteraction(ownerInteraction, parameters, tag);
         await(member, callbackModel, () -> memberRecorder.receivedParameters != null,
            "extension interaction delivery");
         JavaTckSupport.check(memberInteraction.equals(memberRecorder.receivedInteraction)
               && memberRecorder.receivedParameters.size() == 1
               && Arrays.equals(payload, memberRecorder.receivedParameters.get(memberParameter))
               && Arrays.equals(tag, memberRecorder.interactionTag),
            "Create-time FOM composition changed extension interaction data or tag");

         verifyDirectedExtension(owner, member, ownerClass, memberClass, ownerAttribute,
            memberAttribute, ownerInteraction, ownerParameter, ownerFederate, ownerRecorder,
            memberRecorder, callbackModel, "create-time composition");
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
         String attributeName, String interactionClassName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador extension = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder extensionRecorder = new Recorder();
      String federation = "java-tck-fom-composed-join-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean extensionConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean extensionJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         extension.connect(extensionRecorder.proxy(), callbackModel);
         extensionConnected = true;
         owner.createFederationExecution(federation, fileDesignator(baseFom),
            timeImplementation);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            "java-tck-fom-base-owner", federation);
         ownerJoined = true;
         JavaTckSupport.expectFailure(
            () -> owner.getObjectClassHandle(extensionObjectClassName), "NameNotFound",
            "extension object-class lookup before the additional-module join");
         verifyObjectClassTable(owner, BASE_OBJECT_CLASS_NAMES,
            "pre-join base object-class table");
         verifyInteractionClassTable(owner, BASE_INTERACTION_CLASS_NAMES,
            "pre-join base interaction-class table");
         verifyAttributeTable(owner, BASE_ATTRIBUTE_TABLE, "pre-join base attribute table");
         verifyParameterTable(owner, BASE_PARAMETER_TABLE, "pre-join base parameter table");
         ObjectClassHandle baseMomClass = owner.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederate");
         AttributeHandle baseMomFederateHandle = owner.getAttributeHandle(
            baseMomClass, "HLAfederateHandle");

         extension.joinFederationExecution("java-tck-fom-extension-joiner",
            "java-tck-fom-extension-joiner", federation,
            new String[] {fileDesignator(extensionFom)});
         extensionJoined = true;
         List<ObjectClassHandle> joinedOwnerObjectClasses = verifyObjectClassTable(
            owner, COMPOSED_OBJECT_CLASS_NAMES, "post-join owner object-class table");
         List<ObjectClassHandle> joinedExtensionObjectClasses = verifyObjectClassTable(
            extension, COMPOSED_OBJECT_CLASS_NAMES, "post-join member object-class table");
         List<InteractionClassHandle> joinedOwnerInteractionClasses =
            verifyInteractionClassTable(owner, COMPOSED_INTERACTION_CLASS_NAMES,
               "post-join owner interaction-class table");
         List<InteractionClassHandle> joinedExtensionInteractionClasses =
            verifyInteractionClassTable(extension, COMPOSED_INTERACTION_CLASS_NAMES,
               "post-join member interaction-class table");
         List<AttributeHandle> joinedOwnerAttributeHandles = verifyAttributeTable(
            owner, COMPOSED_ATTRIBUTE_TABLE, "post-join owner attribute table");
         List<AttributeHandle> joinedExtensionAttributeHandles = verifyAttributeTable(
            extension, COMPOSED_ATTRIBUTE_TABLE, "post-join member attribute table");
         List<ParameterHandle> joinedOwnerParameters = verifyParameterTable(
            owner, COMPOSED_PARAMETER_TABLE, "post-join owner parameter table");
         List<ParameterHandle> joinedExtensionParameters = verifyParameterTable(
            extension, COMPOSED_PARAMETER_TABLE, "post-join member parameter table");
         JavaTckSupport.check(joinedOwnerObjectClasses.equals(joinedExtensionObjectClasses)
               && joinedOwnerInteractionClasses.equals(joinedExtensionInteractionClasses)
               && joinedOwnerAttributeHandles.equals(joinedExtensionAttributeHandles)
               && joinedOwnerParameters.equals(joinedExtensionParameters),
            "additional-module Join did not expose shared complete fixture tables");
         ObjectClassHandle joinedOwnerMomClass = owner.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederate");
         AttributeHandle joinedOwnerMomFederateHandle = owner.getAttributeHandle(
            joinedOwnerMomClass, "HLAfederateHandle");
         ObjectClassHandle joinedExtensionMomClass = extension.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederate");
         AttributeHandle joinedExtensionMomFederateHandle = extension.getAttributeHandle(
            joinedExtensionMomClass, "HLAfederateHandle");
         JavaTckSupport.check(baseMomClass.equals(joinedOwnerMomClass)
               && baseMomFederateHandle.equals(joinedOwnerMomFederateHandle)
               && joinedOwnerMomClass.equals(joinedExtensionMomClass)
               && joinedOwnerMomFederateHandle.equals(joinedExtensionMomFederateHandle),
            "additional-module Join changed or disagreed on standard MIM handles");
         ObjectClassHandle ownerExtension = owner.getObjectClassHandle(extensionObjectClassName);
         ObjectClassHandle memberExtension =
            extension.getObjectClassHandle(extensionObjectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerExtension, attributeName);
         AttributeHandle memberAttribute = extension.getAttributeHandle(memberExtension,
            attributeName);
         InteractionClassHandle ownerInteraction =
            owner.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle memberInteraction =
            extension.getInteractionClassHandle(interactionClassName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle memberParameter = extension.getParameterHandle(memberInteraction,
            parameterName);
         JavaTckSupport.check(ownerExtension.equals(memberExtension)
               && ownerAttribute.equals(memberAttribute)
               && ownerInteraction.equals(memberInteraction)
               && ownerParameter.equals(memberParameter)
               && extensionObjectClassName.equals(owner.getObjectClassName(ownerExtension))
               && extensionObjectClassName.equals(extension.getObjectClassName(memberExtension))
               && owner.getAttributeName(ownerExtension, ownerAttribute).equals(
                  attributeName)
               && extension.getAttributeName(memberExtension, memberAttribute).equals(
                  attributeName)
               && owner.getInteractionClassName(ownerInteraction).equals(
                  interactionClassName)
               && extension.getInteractionClassName(memberInteraction).equals(
                  interactionClassName)
               && owner.getParameterName(ownerInteraction, ownerParameter).equals(
                  parameterName)
               && extension.getParameterName(memberInteraction, memberParameter).equals(
                  parameterName),
            "additional-module Join did not expose shared extension object, attribute, "
               + "interaction, and parameter declarations to all members");

         verifyDirectedExtension(owner, extension, ownerExtension, memberExtension,
            ownerAttribute, memberAttribute, ownerInteraction, ownerParameter, ownerFederate,
            ownerRecorder, extensionRecorder, callbackModel, "additional-module Join");
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

   private static List<ObjectClassHandle> verifyObjectClassTable(RTIambassador ambassador,
         String[] expectedNames, String context) throws Exception {
      List<ObjectClassHandle> handles = new ArrayList<>();
      for (String expectedName : expectedNames) {
         ObjectClassHandle handle = ambassador.getObjectClassHandle(expectedName);
         JavaTckSupport.check(expectedName.equals(ambassador.getObjectClassName(handle)),
            context + " did not round-trip an object-class path");
         JavaTckSupport.check(!handles.contains(handle),
            context + " assigned one handle to distinct object-class paths");
         handles.add(handle);
      }
      return handles;
   }

   private static List<InteractionClassHandle> verifyInteractionClassTable(
         RTIambassador ambassador, String[] expectedNames, String context) throws Exception {
      List<InteractionClassHandle> handles = new ArrayList<>();
      for (String expectedName : expectedNames) {
         InteractionClassHandle handle = ambassador.getInteractionClassHandle(expectedName);
         JavaTckSupport.check(expectedName.equals(
               ambassador.getInteractionClassName(handle)),
            context + " did not round-trip an interaction-class path");
         JavaTckSupport.check(!handles.contains(handle),
            context + " assigned one handle to distinct interaction-class paths");
         handles.add(handle);
      }
      return handles;
   }

   private static List<AttributeHandle> verifyAttributeTable(RTIambassador ambassador,
         String[][] expectedEntries, String context) throws Exception {
      List<AttributeHandle> handles = new ArrayList<>();
      for (String[] entry : expectedEntries) {
         ObjectClassHandle objectClass = ambassador.getObjectClassHandle(entry[0]);
         for (int index = 1; index < entry.length; index++) {
            String expectedName = entry[index];
            AttributeHandle handle = ambassador.getAttributeHandle(objectClass, expectedName);
            JavaTckSupport.check(expectedName.equals(
                  ambassador.getAttributeName(objectClass, handle)),
               context + " did not round-trip an object/attribute entry");
            handles.add(handle);
         }
      }
      return handles;
   }

   private static List<ParameterHandle> verifyParameterTable(RTIambassador ambassador,
         String[][] expectedEntries, String context) throws Exception {
      List<ParameterHandle> handles = new ArrayList<>();
      for (String[] entry : expectedEntries) {
         InteractionClassHandle interactionClass =
            ambassador.getInteractionClassHandle(entry[0]);
         for (int index = 1; index < entry.length; index++) {
            String expectedName = entry[index];
            ParameterHandle handle = ambassador.getParameterHandle(
               interactionClass, expectedName);
            JavaTckSupport.check(expectedName.equals(
                  ambassador.getParameterName(interactionClass, handle)),
               context + " did not round-trip an interaction/parameter entry");
            handles.add(handle);
         }
      }
      return handles;
   }

   private static void verifyDirectedExtension(RTIambassador publisher,
         RTIambassador receiver, ObjectClassHandle publisherClass, ObjectClassHandle receiverClass,
         AttributeHandle publisherAttribute, AttributeHandle receiverAttribute,
         InteractionClassHandle interaction, ParameterHandle parameter,
         FederateHandle publisherFederate, Recorder publisherRecorder, Recorder receiverRecorder,
         CallbackModel callbackModel, String context) throws Exception {
      // The directed declarations are a row in the adapter's extension FOM.
      JavaTckSupport.check(receiverClass != null && publisherClass.equals(receiverClass),
         context + " did not resolve matching extension object classes");
      publisher.publishObjectClassDirectedInteractions(publisherClass,
         JavaTckSupport.interactionSet(publisher, interaction));
      receiver.subscribeObjectClassDirectedInteractions(receiverClass,
         JavaTckSupport.interactionSet(receiver, interaction));
      receiver.publishObjectClassAttributes(receiverClass,
         JavaTckSupport.attributeSet(receiver, receiverAttribute));
      publisher.subscribeObjectClassAttributes(publisherClass,
         JavaTckSupport.attributeSet(publisher, publisherAttribute));

      ObjectInstanceHandle target = receiver.registerObjectInstance(receiverClass);
      await(publisher, callbackModel,
         () -> target.equals(publisherRecorder.discoveredObject),
         context + " directed target discovery");
      byte[] payload = new byte[] {0x4d, 0x46};
      byte[] tag = new byte[] {(byte) 0xd1, 0x21};
      ParameterHandleValueMap parameters =
         publisher.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, payload);
      publisher.sendDirectedInteraction(interaction, target, parameters, tag);
      await(receiver, callbackModel,
         () -> receiverRecorder.directedInteraction != null,
         context + " directed extension interaction");
      JavaTckSupport.check(interaction.equals(receiverRecorder.directedInteraction)
            && target.equals(receiverRecorder.directedTarget)
            && receiverRecorder.directedParameters.size() == 1
            && Arrays.equals(payload, receiverRecorder.directedParameters.get(parameter))
            && Arrays.equals(tag, receiverRecorder.directedTag)
            && publisherFederate.equals(receiverRecorder.directedProducer),
         context + " directed delivery changed its extension target, producer, or payload");
   }

   private static String fileDesignator(String path) {
      return path.startsWith("file:") ? path
         : new File(path).getAbsoluteFile().toURI().toASCIIString();
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
      private volatile TransportationTypeHandle reflectedTransportation;
      private volatile InteractionClassHandle receivedInteraction;
      private volatile ParameterHandleValueMap receivedParameters;
      private volatile byte[] interactionTag;
      private volatile InteractionClassHandle directedInteraction;
      private volatile ObjectInstanceHandle directedTarget;
      private volatile ParameterHandleValueMap directedParameters;
      private volatile byte[] directedTag;
      private volatile FederateHandle directedProducer;

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
                  if (arguments.length >= 4) {
                     reflectedTransportation = (TransportationTypeHandle) arguments[3];
                  }
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  receivedInteraction = (InteractionClassHandle) arguments[0];
                  receivedParameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  if (arguments.length >= 3 && arguments[2] instanceof byte[]) {
                     interactionTag = ((byte[]) arguments[2]).clone();
                  }
               } else if ("receiveDirectedInteraction".equals(name) && arguments != null
                     && arguments.length >= 6) {
                  ObjectInstanceHandle target = (ObjectInstanceHandle) arguments[1];
                  ParameterHandleValueMap parameters =
                     ((ParameterHandleValueMap) arguments[2]).clone();
                  byte[] tag = ((byte[]) arguments[3]).clone();
                  FederateHandle producer = (FederateHandle) arguments[5];
                  directedTarget = target;
                  directedParameters = parameters;
                  directedTag = tag;
                  directedProducer = producer;
                  directedInteraction = (InteractionClassHandle) arguments[0];
               }
               return objectMethod(proxy, name, arguments);
            });
      }
   }
}
