package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionInformationSet;
import hla.rti1516_2025.FederationExecutionMemberInformation;
import hla.rti1516_2025.FederationExecutionMemberInformationSet;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;

/**
 * Portable federation-management and ordinary object-management vectors.
 *
 * <p>This helper intentionally binds only to the standard Java RTI API. It
 * contains no provider implementation or regional/DDM types, so the compiled
 * class can be used with another 1516.1-2025 Java provider.
 */
final class FederationBasicsTck {
   private FederationBasicsTck() {
   }

   static void federationMembership(RtiFactory factory, String fom, String timeImplementation)
         throws Exception {
      RTIambassador disconnected = factory.getRtiAmbassador();
      RTIambassador unjoined = factory.getRtiAmbassador();
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador modulesJoiner = factory.getRtiAmbassador();
      RTIambassador typedJoiner = factory.getRtiAmbassador();
      RTIambassador typedModulesJoiner = factory.getRtiAmbassador();
      RTIambassador duplicateName = factory.getRtiAmbassador();
      RTIambassador foreignCreator = factory.getRtiAmbassador();
      RTIambassador foreignMember = factory.getRtiAmbassador();
      RTIambassador immediateLister = factory.getRtiAmbassador();
      CallbackRecorder creatorRecorder = new CallbackRecorder();
      CallbackRecorder unjoinedRecorder = new CallbackRecorder();
      CallbackRecorder modulesRecorder = new CallbackRecorder();
      CallbackRecorder typedRecorder = new CallbackRecorder();
      CallbackRecorder typedModulesRecorder = new CallbackRecorder();
      CallbackRecorder duplicateNameRecorder = new CallbackRecorder();
      CallbackRecorder foreignCreatorRecorder = new CallbackRecorder();
      CallbackRecorder foreignMemberRecorder = new CallbackRecorder();
      CallbackRecorder immediateRecorder = new CallbackRecorder();
      String federation = "java-tck-membership-" + UUID.randomUUID();
      String foreignFederation = "java-tck-foreign-" + UUID.randomUUID();
      String missingFederation = "java-tck-missing-" + UUID.randomUUID();
      String[] additionalFomModules = new String[] {fom};
      boolean unjoinedConnected = false;
      boolean creatorConnected = false;
      boolean modulesConnected = false;
      boolean typedConnected = false;
      boolean typedModulesConnected = false;
      boolean duplicateNameConnected = false;
      boolean foreignCreatorConnected = false;
      boolean foreignMemberConnected = false;
      boolean immediateListerConnected = false;
      boolean creatorJoined = false;
      boolean modulesJoined = false;
      boolean typedJoined = false;
      boolean typedModulesJoined = false;
      boolean foreignMemberJoined = false;
      boolean created = false;
      boolean foreignCreated = false;
      try {
         expectFailure(
            () -> disconnected.createFederationExecution(federation, fom, timeImplementation),
            "NotConnected", "createFederationExecution before connect");
         expectFailure(
            () -> disconnected.destroyFederationExecution(federation),
            "NotConnected", "destroyFederationExecution before connect");
         expectFailure(
            () -> disconnected.listFederationExecutions(),
            "NotConnected", "listFederationExecutions before connect");
         expectFailure(
            () -> disconnected.listFederationExecutionMembers(federation),
            "NotConnected", "listFederationExecutionMembers before connect");
         expectFailure(
            () -> disconnected.joinFederationExecution("java-tck-disconnected", federation),
            "NotConnected", "joinFederationExecution before connect");
         expectFailure(
            () -> disconnected.resignFederationExecution(ResignAction.NO_ACTION),
            "NotConnected", "resignFederationExecution before connect");

         ConfigurationResult unjoinedConfiguration = unjoined.connect(
            unjoinedRecorder.proxy(), CallbackModel.HLA_EVOKED);
         unjoinedConnected = true;
         check(unjoinedConfiguration != null, "unjoined connect returned null ConfigurationResult");
         expectFailure(
            () -> unjoined.getFederateHandle("java-tck-not-a-member"),
            "FederateNotExecutionMember", "federate lookup before joining");
         expectFailure(
            () -> unjoined.resignFederationExecution(ResignAction.NO_ACTION),
            "FederateNotExecutionMember", "resignFederationExecution before joining");

         ConfigurationResult creatorConfiguration = creator.connect(
            creatorRecorder.proxy(), CallbackModel.HLA_EVOKED);
         creatorConnected = true;
         ConfigurationResult modulesConfiguration = modulesJoiner.connect(
            modulesRecorder.proxy(), CallbackModel.HLA_EVOKED);
         modulesConnected = true;
         ConfigurationResult typedConfiguration = typedJoiner.connect(
            typedRecorder.proxy(), CallbackModel.HLA_EVOKED);
         typedConnected = true;
         ConfigurationResult typedModulesConfiguration = typedModulesJoiner.connect(
            typedModulesRecorder.proxy(), CallbackModel.HLA_EVOKED);
         typedModulesConnected = true;
         ConfigurationResult duplicateNameConfiguration = duplicateName.connect(
            duplicateNameRecorder.proxy(), CallbackModel.HLA_EVOKED);
         duplicateNameConnected = true;
         ConfigurationResult foreignCreatorConfiguration = foreignCreator.connect(
            foreignCreatorRecorder.proxy(), CallbackModel.HLA_EVOKED);
         foreignCreatorConnected = true;
         ConfigurationResult foreignMemberConfiguration = foreignMember.connect(
            foreignMemberRecorder.proxy(), CallbackModel.HLA_EVOKED);
         foreignMemberConnected = true;
         ConfigurationResult immediateConfiguration = immediateLister.connect(
            immediateRecorder.proxy(), CallbackModel.HLA_IMMEDIATE);
         immediateListerConnected = true;
         check(creatorConfiguration != null, "creator connect returned null ConfigurationResult");
         check(modulesConfiguration != null, "modules joiner connect returned null ConfigurationResult");
         check(typedConfiguration != null, "typed joiner connect returned null ConfigurationResult");
         check(typedModulesConfiguration != null,
            "typed modules joiner connect returned null ConfigurationResult");
         check(duplicateNameConfiguration != null,
            "duplicate-name connect returned null ConfigurationResult");
         check(foreignCreatorConfiguration != null,
            "foreign creator connect returned null ConfigurationResult");
         check(foreignMemberConfiguration != null,
            "foreign member connect returned null ConfigurationResult");
         check(immediateConfiguration != null,
            "immediate lister connect returned null ConfigurationResult");

         creator.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         foreignCreator.createFederationExecution(foreignFederation, fom, timeImplementation);
         foreignCreated = true;
         expectFailure(
            () -> creator.createFederationExecution(federation, fom, timeImplementation),
            "FederationExecutionAlreadyExists", "duplicate federation creation");

         FederateHandle creatorHandle = creator.joinFederationExecution(
            "java-tck-creator", federation);
         creatorJoined = true;
         String creatorName = creator.getFederateName(creatorHandle);
         check(creatorName != null && !creatorName.isEmpty(),
            "two-argument join did not return an assigned federate name");
         expectFailure(
            () -> creator.joinFederationExecution("java-tck-creator", federation),
            "FederateAlreadyExecutionMember", "duplicate join by one ambassador");
         expectFailure(
            () -> creator.disconnect(),
            "FederateIsExecutionMember", "disconnect while joined");

         FederateHandle modulesHandle = modulesJoiner.joinFederationExecution(
            "java-tck-modules", federation, additionalFomModules);
         modulesJoined = true;
         String modulesName = modulesJoiner.getFederateName(modulesHandle);
         check(modulesName != null && !modulesName.isEmpty(),
            "additional-FOM join did not return an assigned federate name");
         FederateHandle typedHandle = typedJoiner.joinFederationExecution(
            "java-tck-typed", "typed", federation);
         typedJoined = true;
         FederateHandle typedModulesHandle = typedModulesJoiner.joinFederationExecution(
            "java-tck-typed-modules", "typed-modules", federation, additionalFomModules);
         typedModulesJoined = true;
         expectFailure(
            () -> duplicateName.joinFederationExecution(
               creatorName, "duplicate", federation),
            "FederateNameAlreadyInUse", "duplicate federate name");

         FederateHandle foreignHandle = foreignMember.joinFederationExecution(
            "java-tck-foreign", "foreign", foreignFederation);
         foreignMemberJoined = true;

         expectFailure(
            () -> disconnected.getFederateHandle(creatorName),
            "NotConnected", "federate handle lookup before connect");
         expectFailure(
            () -> disconnected.getFederateName(foreignHandle),
            "NotConnected", "federate name lookup before connect");

         check(creatorHandle != null && creatorHandle.encodedLength() > 0,
            "creator join returned an invalid FederateHandle");
         check(modulesHandle != null && modulesHandle.encodedLength() > 0,
            "additional-FOM join returned an invalid FederateHandle");
         check(typedHandle != null && typedHandle.encodedLength() > 0,
            "typed join returned an invalid FederateHandle");
         check(typedModulesHandle != null && typedModulesHandle.encodedLength() > 0,
            "typed additional-FOM join returned an invalid FederateHandle");
         check(foreignHandle != null && foreignHandle.encodedLength() > 0,
            "foreign join returned an invalid FederateHandle");
         check(!creatorHandle.equals(modulesHandle)
               && !creatorHandle.equals(typedHandle)
               && !creatorHandle.equals(typedModulesHandle),
            "distinct joined federates received the same FederateHandle");
         check(creatorName.equals(creator.getFederateName(creatorHandle)),
            "creator federate name did not round-trip");
         check(modulesName.equals(creator.getFederateName(modulesHandle)),
            "additional-FOM federate name did not round-trip");
         check("java-tck-typed".equals(creator.getFederateName(typedHandle)),
            "typed federate name did not round-trip");
         check("java-tck-typed-modules".equals(creator.getFederateName(typedModulesHandle)),
            "typed additional-FOM federate name did not round-trip");
         check(creatorName.equals(typedJoiner.getFederateName(creatorHandle)),
            "typed joiner could not query another federate name");
         check(creator.getFederateHandle(creatorName).equals(creatorHandle),
            "creator federate handle did not round-trip");
         check(creator.getFederateHandle(modulesName).equals(modulesHandle),
            "additional-FOM federate handle did not round-trip");
         check(creator.getFederateHandle("java-tck-typed").equals(typedHandle),
            "typed federate handle did not round-trip");
         check(creator.getFederateHandle("java-tck-typed-modules").equals(typedModulesHandle),
            "typed additional-FOM federate handle did not round-trip");
         expectFailure(
            () -> creator.getFederateHandle("java-tck-missing"),
            "NameNotFound", "lookup of a missing federate name");
         expectFailure(
            () -> creator.getFederateName(foreignHandle),
            "FederateHandleNotKnown", "lookup of a foreign federate handle");

         creator.listFederationExecutions();
         drain(creator);
         check(creatorRecorder.federationExecutionReports > 0,
            "federation execution report did not cross the standard callback interface");
         check(containsFederation(creatorRecorder.lastFederationExecutions,
               federation, timeImplementation),
            "created federation was missing from the federation execution report");

         immediateLister.listFederationExecutions();
         check(immediateRecorder.federationExecutionReports > 0,
            "immediate federation execution report was not dispatched");
         check(containsFederation(immediateRecorder.lastFederationExecutions,
               federation, timeImplementation),
            "immediate federation execution report omitted the created federation");

         creator.listFederationExecutionMembers(federation);
         drain(creator);
         check(creatorRecorder.memberReports > 0,
            "federation member report did not cross the standard callback interface");
         check(federation.equals(creatorRecorder.lastMemberFederation),
            "member report identified the wrong federation execution");
         check(creatorRecorder.lastMembers != null && creatorRecorder.lastMembers.size() == 4,
            "member report did not contain all four ordinary joined federates");
         check(containsMember(creatorRecorder.lastMembers, creatorName, "java-tck-creator"),
            "member report omitted the two-argument joiner");
         check(containsMember(creatorRecorder.lastMembers, modulesName, "java-tck-modules"),
            "member report omitted the additional-FOM joiner");
         check(containsMember(creatorRecorder.lastMembers, "java-tck-typed", "typed"),
            "member report did not preserve the typed joiner type");
         check(containsMember(creatorRecorder.lastMembers, "java-tck-typed-modules",
               "typed-modules"),
            "member report omitted the typed additional-FOM joiner");

         immediateLister.listFederationExecutionMembers(federation);
         check(immediateRecorder.memberReports > 0,
            "immediate federation member report was not dispatched");
         check(federation.equals(immediateRecorder.lastMemberFederation),
            "immediate member report identified the wrong federation execution");

         creator.listFederationExecutionMembers(missingFederation);
         drain(creator);
         check(creatorRecorder.missingFederationReports > 0,
            "missing federation report did not cross the standard callback interface");
         check(missingFederation.equals(creatorRecorder.lastMissingFederation),
            "missing federation report identified the wrong execution");

         modulesJoiner.resignFederationExecution(ResignAction.NO_ACTION);
         modulesJoined = false;
         expectFailure(
            () -> creator.getFederateHandle(modulesName),
            "NameNotFound", "lookup of a resigned federate name");
         check(modulesName.equals(creator.getFederateName(modulesHandle)),
            "resigned federate handle lost its standard designator identity");
         creator.listFederationExecutionMembers(federation);
         drain(creator);
         check(creatorRecorder.lastMembers != null && creatorRecorder.lastMembers.size() == 3,
            "member report retained a resigned federate");

         expectFailure(
            () -> creator.destroyFederationExecution(federation),
            "FederatesCurrentlyJoined", "destroy while ordinary members remain joined");

         typedModulesJoiner.resignFederationExecution(ResignAction.NO_ACTION);
         typedModulesJoined = false;
         typedJoiner.resignFederationExecution(ResignAction.NO_ACTION);
         typedJoined = false;
         creator.resignFederationExecution(ResignAction.NO_ACTION);
         creatorJoined = false;
         creator.destroyFederationExecution(federation);
         created = false;
         expectFailure(
            () -> creator.destroyFederationExecution(federation),
            "FederationExecutionDoesNotExist", "destroy of a missing federation");

         foreignMember.resignFederationExecution(ResignAction.NO_ACTION);
         foreignMemberJoined = false;
         foreignCreator.destroyFederationExecution(foreignFederation);
         foreignCreated = false;
      } finally {
         if (foreignMemberJoined) {
            try {
               foreignMember.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (typedModulesJoined) {
            try {
               typedModulesJoiner.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (typedJoined) {
            try {
               typedJoiner.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (modulesJoined) {
            try {
               modulesJoiner.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (creatorJoined) {
            try {
               creator.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (created) {
            try {
               creator.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (foreignCreated) {
            try {
               foreignCreator.destroyFederationExecution(foreignFederation);
            } catch (Exception ignored) {
            }
         }
         if (immediateListerConnected) {
            try {
               immediateLister.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (foreignMemberConnected) {
            try {
               foreignMember.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (foreignCreatorConnected) {
            try {
               foreignCreator.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (duplicateNameConnected) {
            try {
               duplicateName.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (typedModulesConnected) {
            try {
               typedModulesJoiner.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (typedConnected) {
            try {
               typedJoiner.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (modulesConnected) {
            try {
               modulesJoiner.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (creatorConnected) {
            try {
               creator.disconnect();
            } catch (Exception ignored) {
            }
         }
         }
         if (unjoinedConnected) {
            try {
               unjoined.disconnect();
            } catch (Exception ignored) {
            }
         }
      }

   private static boolean containsFederation(FederationExecutionInformationSet reports,
         String federation, String timeImplementation) {
      if (reports == null) {
         return false;
      }
      for (FederationExecutionInformation report : reports) {
         if (federation.equals(report.federationExecutionName)
               && timeImplementation.equals(report.logicalTimeImplementationName)) {
            return true;
         }
      }
      return false;
   }

   private static boolean containsMember(FederationExecutionMemberInformationSet reports,
         String federateName) {
      return containsMember(reports, federateName, null);
   }

   private static boolean containsMember(FederationExecutionMemberInformationSet reports,
         String federateName, String federateType) {
      if (reports == null) {
         return false;
      }
      for (FederationExecutionMemberInformation report : reports) {
         if (federateName.equals(report.federateName)
               && (federateType == null || federateType.equals(report.federateType))) {
            return true;
         }
      }
      return false;
   }

   private static void expectFailure(CheckedOperation operation, String exceptionName,
         String description) throws Exception {
      try {
         operation.run();
      } catch (Throwable error) {
         for (Throwable candidate = error; candidate != null; candidate = candidate.getCause()) {
            if (exceptionName.equals(candidate.getClass().getSimpleName())) {
               return;
            }
         }
         throw new AssertionError(description + " threw " + error.getClass().getName(), error);
      }
      throw new AssertionError(description + " completed without " + exceptionName);
   }

   @FunctionalInterface
   private interface CheckedOperation {
      void run() throws Exception;
   }

   static void declarationManagement(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      CallbackRecorder publisherRecorder = new CallbackRecorder();
      CallbackRecorder subscriberRecorder = new CallbackRecorder();
      String federation = "java-tck-declaration-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean created = false;
      try {
         ConfigurationResult publisherConfiguration = publisher.connect(
            publisherRecorder.proxy(), CallbackModel.HLA_EVOKED);
         publisherConnected = true;
         check(publisherConfiguration != null,
            "publisher connect returned null ConfigurationResult");
         ConfigurationResult subscriberConfiguration = subscriber.connect(
            subscriberRecorder.proxy(), CallbackModel.HLA_EVOKED);
         subscriberConnected = true;
         check(subscriberConfiguration != null,
            "subscriber connect returned null ConfigurationResult");
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-publisher", "publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-subscriber", "subscriber", federation);
         subscriberJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         ObjectClassHandle subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle subscriberAttribute = subscriber.getAttributeHandle(
            subscriberClass, attributeName);
         check(objectClassName.equals(publisher.getObjectClassName(publisherClass)),
            "publisher object-class name did not round-trip");
         check(attributeName.equals(publisher.getAttributeName(publisherClass, publisherAttribute)),
            "publisher attribute name did not round-trip");
         check(objectClassName.equals(subscriber.getObjectClassName(subscriberClass)),
            "subscriber object-class name did not round-trip");
         check(attributeName.equals(subscriber.getAttributeName(subscriberClass, subscriberAttribute)),
            "subscriber attribute name did not round-trip");

         AttributeHandleSet publisherAttributes =
            publisher.getAttributeHandleSetFactory().create();
         publisherAttributes.add(publisherAttribute);
         AttributeHandleSet subscriberAttributes =
            subscriber.getAttributeHandleSetFactory().create();
         subscriberAttributes.add(subscriberAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         subscriber.unsubscribeObjectClass(subscriberClass);
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         publisher.unpublishObjectClass(publisherClass);
      } finally {
         if (subscriberJoined) {
            try {
               subscriber.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (publisherJoined) {
            try {
               publisher.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (created) {
            try {
               publisher.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (subscriberConnected) {
            try {
               subscriber.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (publisherConnected) {
            try {
               publisher.disconnect();
            } catch (Exception ignored) {
            }
         }
      }
   }

   static void objectRegistrationAndReflection(RtiFactory factory, String fom,
         String timeImplementation, String objectClassName, String attributeName) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      CallbackRecorder publisherRecorder = new CallbackRecorder();
      CallbackRecorder subscriberRecorder = new CallbackRecorder();
      String federation = "java-tck-object-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean created = false;
      try {
         ConfigurationResult publisherConfiguration = publisher.connect(
            publisherRecorder.proxy(), CallbackModel.HLA_EVOKED);
         publisherConnected = true;
         check(publisherConfiguration != null,
            "publisher connect returned null ConfigurationResult");
         ConfigurationResult subscriberConfiguration = subscriber.connect(
            subscriberRecorder.proxy(), CallbackModel.HLA_EVOKED);
         subscriberConnected = true;
         check(subscriberConfiguration != null,
            "subscriber connect returned null ConfigurationResult");
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-publisher", "publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-subscriber", "subscriber", federation);
         subscriberJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         ObjectClassHandle subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle subscriberAttribute = subscriber.getAttributeHandle(
            subscriberClass, attributeName);
         AttributeHandleSet publisherAttributes =
            publisher.getAttributeHandleSetFactory().create();
         publisherAttributes.add(publisherAttribute);
         AttributeHandleSet subscriberAttributes =
            subscriber.getAttributeHandleSetFactory().create();
         subscriberAttributes.add(subscriberAttribute);

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         ObjectInstanceHandle registered = publisher.registerObjectInstance(publisherClass);
         check(registered != null && registered.encodedLength() > 0,
            "registerObjectInstance returned an invalid handle");
         drain(subscriber);
         check(registered.equals(subscriberRecorder.discoveredObject),
            "discovered object handle differs from the registered handle");
         check(publisher.getObjectInstanceName(registered).equals(
               subscriberRecorder.discoveredName),
            "discovered object name did not match the registered instance");
         check(subscriberRecorder.discoveredClass.equals(subscriberClass),
            "discovered object class did not match the subscribed class");

         byte[] payload = new byte[] {1, 2, 3, (byte) 0xff};
         AttributeHandleValueMap values =
            publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, payload);
         byte[] tag = new byte[] {9, 8, 7};
         publisher.updateAttributeValues(registered, values, tag);
         drain(subscriber);
         check(subscriberRecorder.reflectedObject.equals(registered),
            "reflected object handle differs from the registered handle");
         check(subscriberRecorder.reflectedValues != null
               && subscriberRecorder.reflectedValues.size() == 1,
            "reflected attribute map has the wrong size");
         byte[] reflectedPayload = subscriberRecorder.reflectedValues.get(subscriberAttribute);
         check(Arrays.equals(payload, reflectedPayload),
            "reflected attribute payload did not round-trip");
         check(Arrays.equals(tag, subscriberRecorder.reflectedTag),
            "update tag did not round-trip");
      } finally {
         if (subscriberJoined) {
            try {
               subscriber.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (publisherJoined) {
            try {
               publisher.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (created) {
            try {
               publisher.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (subscriberConnected) {
            try {
               subscriber.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (publisherConnected) {
            try {
               publisher.disconnect();
            } catch (Exception ignored) {
            }
         }
      }
   }

   private static void drain(RTIambassador ambassador) throws Exception {
      for (int i = 0; i < 64; ++i) {
         ambassador.evokeCallback(0.0);
      }
   }

   private static void check(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }

   private static final class CallbackRecorder {
      private int federationExecutionReports;
      private FederationExecutionInformationSet lastFederationExecutions;
      private int memberReports;
      private String lastMemberFederation;
      private FederationExecutionMemberInformationSet lastMembers;
      private int missingFederationReports;
      private String lastMissingFederation;
      private ObjectInstanceHandle discoveredObject;
      private ObjectClassHandle discoveredClass;
      private String discoveredName;
      private ObjectInstanceHandle reflectedObject;
      private AttributeHandleValueMap reflectedValues;
      private byte[] reflectedTag;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("reportFederationExecutions".equals(method.getName())
                     && arguments != null && arguments.length >= 1) {
                  federationExecutionReports++;
                  lastFederationExecutions = (FederationExecutionInformationSet) arguments[0];
               }
               if ("reportFederationExecutionMembers".equals(method.getName())) {
                  memberReports++;
                  if (arguments != null && arguments.length >= 2) {
                     lastMemberFederation = (String) arguments[0];
                     lastMembers = (FederationExecutionMemberInformationSet) arguments[1];
                  }
               }
               if ("reportFederationExecutionDoesNotExist".equals(method.getName())
                     && arguments != null && arguments.length >= 1) {
                  missingFederationReports++;
                  lastMissingFederation = (String) arguments[0];
               }
               if ("discoverObjectInstance".equals(method.getName())
                     && arguments != null && arguments.length >= 3) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
                  discoveredClass = (ObjectClassHandle) arguments[1];
                  discoveredName = (String) arguments[2];
               }
               if ("reflectAttributeValues".equals(method.getName())
                     && arguments != null && arguments.length >= 2) {
                  reflectedObject = (ObjectInstanceHandle) arguments[0];
                  reflectedValues = (AttributeHandleValueMap) arguments[1];
                  if (arguments.length >= 3 && arguments[2] instanceof byte[]) {
                     reflectedTag = (byte[]) arguments[2];
                  }
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java RTI TCK callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
