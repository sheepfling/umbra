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
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

final class FederationMomStaticIdentityTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String FEDERATION_CLASS = "HLAobjectRoot.HLAmanager.HLAfederation";
   private static final List<String> TABLE_8_PUBLISHED_ATTRIBUTES = Arrays.asList(
      "HLAfederationName",
      "HLAfederatesInFederation",
      "HLARTIversion",
      "HLAMIMdesignator",
      "HLAFOMmoduleDesignatorList",
      "HLAcurrentFDD",
      "HLAtimeImplementationName",
      "HLAlastSaveName",
      "HLAlastSaveTime",
      "HLAnextSaveName",
      "HLAnextSaveTime",
      "HLAautoProvide",
      "HLAadvisoriesUseKnownClass",
      "HLAdelaySubscriptionEvaluation",
      "HLAnonRegulatedGrant",
      "HLAallowRelaxedDDM");
   private static final List<String> TABLE_7_PUBLISHED_INTERACTIONS = Arrays.asList(
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionPublication",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionsReceived",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionsSent",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionSubscription",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportException",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFederateLost",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFOMmoduleData",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionPublication",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsReceived",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsSent",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionSubscription",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassPublication",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassSubscription",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstanceInformation",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesReflected",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesThatCanBeDeleted",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesUpdated",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportReflectionsReceived",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportUpdatesSent",
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportFOMmoduleData",
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata",
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPoints",
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPointStatus");

   private FederationMomStaticIdentityTck() {
   }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel, String federateType) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-mom-static-identity-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean observerJoined = false;
      boolean subscribed = false;
      boolean publishedAttributesSubscribed = false;
      boolean ownershipProbePublished = false;
      ObjectClassHandle federationClass = null;
      AttributeHandleSet identityAttributes = null;
      AttributeHandleSet publishedAttributes = null;
      AttributeHandleSet ownershipProbeAttributes = null;
      try {
         check(owner.connect(callbacks(), callbackModel) != null,
            "static-identity owner connect returned null ConfigurationResult");
         ownerConnected = true;
         check(observer.connect(recorder.proxy(), callbackModel) != null,
            "static-identity observer connect returned null ConfigurationResult");
         observerConnected = true;

         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.joinFederationExecution(
            "java-tck-static-identity-owner", federateType, federation);
         ownerJoined = true;
         observer.joinFederationExecution(
            "java-tck-static-identity-observer", federateType, federation);
         observerJoined = true;

         federationClass = observer.getObjectClassHandle(FEDERATION_CLASS);
         AttributeHandle federationName = observer.getAttributeHandle(
            federationClass, "HLAfederationName");
         AttributeHandle rtiVersion = observer.getAttributeHandle(
            federationClass, "HLARTIversion");
         AttributeHandle mimDesignator = observer.getAttributeHandle(
            federationClass, "HLAMIMdesignator");
         AttributeHandle timeImplementationName = observer.getAttributeHandle(
            federationClass, "HLAtimeImplementationName");
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         check(federationClass != null && federationName != null && rtiVersion != null
               && mimDesignator != null && timeImplementationName != null && reliable != null,
            "static-identity lookup returned a null standard handle");
         check(FEDERATION_CLASS.equals(observer.getObjectClassName(federationClass))
               && "HLAfederationName".equals(
                  observer.getAttributeName(federationClass, federationName))
               && "HLARTIversion".equals(observer.getAttributeName(federationClass, rtiVersion))
               && "HLAMIMdesignator".equals(
                  observer.getAttributeName(federationClass, mimDesignator))
               && "HLAtimeImplementationName".equals(
                  observer.getAttributeName(federationClass, timeImplementationName))
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "static-identity handle names did not round-trip through the standard API");

         identityAttributes = JavaTckSupport.attributeSet(
            observer, federationName, rtiVersion, mimDesignator, timeImplementationName);
         observer.subscribeObjectClassAttributes(federationClass, identityAttributes);
         subscribed = true;
         await(observer, callbackModel, () -> recorder.discoveredObject != null,
            "standard federation MOM object discovery");
         ObjectInstanceHandle federationObject = recorder.discoveredObject;
         check(federationClass.equals(recorder.discoveredClass)
               && "HLAfederation".equals(recorder.discoveredName)
               && federationClass.equals(observer.getKnownObjectClassHandle(federationObject))
               && "HLAfederation".equals(observer.getObjectInstanceName(federationObject))
               && federationObject.equals(observer.getObjectInstanceHandle("HLAfederation")),
            "standard federation MOM discovery did not preserve object identity");

         requestIdentity(observer, recorder, encoder, callbackModel, federationObject,
            identityAttributes, federationName, rtiVersion, mimDesignator,
            timeImplementationName, reliable, federation, timeImplementation);

         verifyTable7PublishedInteractionLookups(observer);
         publishedAttributes = lookupTable8PublishedAttributes(observer, federationClass);
         check(publishedAttributes.size() == TABLE_8_PUBLISHED_ATTRIBUTES.size(),
            "Table 8 did not resolve 16 distinct federation P attributes");
         observer.subscribeObjectClassAttributes(federationClass, publishedAttributes);
         publishedAttributesSubscribed = true;
         requestTable8PublishedAttributes(observer, recorder, callbackModel, federationObject,
            publishedAttributes, reliable);

         AttributeHandle ownershipProbeAttribute = observer.getAttributeHandle(
            federationClass, "HLAfederationName");
         ownershipProbeAttributes = JavaTckSupport.attributeSet(
            observer, ownershipProbeAttribute);
         observer.publishObjectClassAttributes(federationClass, ownershipProbeAttributes);
         ownershipProbePublished = true;
         verifyFederationNameNoTransfer(observer, recorder, callbackModel, federationObject,
            ownershipProbeAttributes, ownershipProbeAttribute);
         observer.unpublishObjectClassAttributes(federationClass, ownershipProbeAttributes);
         ownershipProbePublished = false;
      } finally {
         if (ownershipProbePublished && observerJoined && federationClass != null
               && ownershipProbeAttributes != null) {
            try {
               observer.unpublishObjectClassAttributes(federationClass, ownershipProbeAttributes);
            } catch (Exception ignored) {
            }
         }
         if (publishedAttributesSubscribed && observerJoined && federationClass != null
               && publishedAttributes != null) {
            try {
               observer.unsubscribeObjectClassAttributes(federationClass, publishedAttributes);
            } catch (Exception ignored) {
            }
         }
         if (subscribed && observerJoined && federationClass != null
               && identityAttributes != null) {
            try {
               observer.unsubscribeObjectClassAttributes(federationClass, identityAttributes);
            } catch (Exception ignored) {
            }
         }
         if (observerJoined) resign(observer);
         if (ownerJoined) resign(owner);
         if (created) {
            try {
               owner.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (observerConnected) disconnect(observer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void requestIdentity(RTIambassador observer, Recorder recorder,
         EncoderFactory encoder, CallbackModel callbackModel, ObjectInstanceHandle federationObject,
         AttributeHandleSet attributes, AttributeHandle federationName, AttributeHandle rtiVersion,
         AttributeHandle mimDesignator, AttributeHandle timeImplementationName,
         TransportationTypeHandle reliable, String expectedFederation,
         String expectedTimeImplementation) throws Exception {
      recorder.clearReflection();
      observer.requestAttributeValueUpdate(federationObject, attributes, new byte[0]);
      await(observer, callbackModel, () -> recorder.hasIdentity(federationObject,
         federationName, rtiVersion, mimDesignator, timeImplementationName),
         "requested standard federation MOM identity values");
      check(recorder.reflectedValues.size() == 4
            && recorder.reflectionTag != null && recorder.reflectionTag.length == 0
            && reliable.equals(recorder.reflectionTransportation)
            && recorder.reflectionRegionsEmpty,
         "federation MOM identity reflection had incomplete or non-standard metadata");
      JavaTckSupport.requireNoProducingFederate(observer, recorder.reflectionProducer,
         "federation MOM identity reflection");

      String actualFederation = decode(encoder, recorder.reflectedValues.get(federationName));
      String actualVersion = decode(encoder, recorder.reflectedValues.get(rtiVersion));
      String actualMim = decode(encoder, recorder.reflectedValues.get(mimDesignator));
      String actualTimeImplementation =
         decode(encoder, recorder.reflectedValues.get(timeImplementationName));
      check(expectedFederation.equals(actualFederation),
         "HLAfederationName did not match the created federation execution");
      check(actualVersion != null && !actualVersion.isEmpty(),
         "required static HLARTIversion value was empty");
      check("HLAstandardMIM".equals(actualMim),
         "HLAMIMdesignator did not identify the supplied standard MIM");
      check(expectedTimeImplementation.equals(actualTimeImplementation),
         "HLAtimeImplementationName did not match the create-federation argument");
   }

   private static void verifyTable7PublishedInteractionLookups(RTIambassador observer)
         throws Exception {
      check(TABLE_7_PUBLISHED_INTERACTIONS.size() == 25,
         "Table 7 P-leaf lookup inventory must contain all 25 standard MIM entries");
      for (String name : TABLE_7_PUBLISHED_INTERACTIONS) {
         InteractionClassHandle handle = observer.getInteractionClassHandle(name);
         check(handle != null && name.equals(observer.getInteractionClassName(handle)),
            "standard MIM did not resolve and round-trip Table 7 P interaction " + name);
      }
   }

   private static AttributeHandleSet lookupTable8PublishedAttributes(
         RTIambassador observer, ObjectClassHandle federationClass) throws Exception {
      check(TABLE_8_PUBLISHED_ATTRIBUTES.size() == 16,
         "Table 8 P-attribute inventory must contain all 16 standard MIM entries");
      List<AttributeHandle> handles = new ArrayList<>();
      for (String name : TABLE_8_PUBLISHED_ATTRIBUTES) {
         AttributeHandle handle = observer.getAttributeHandle(federationClass, name);
         check(handle != null && name.equals(observer.getAttributeName(federationClass, handle)),
            "Table 8 attribute lookup did not round-trip " + name);
         handles.add(handle);
      }
      return JavaTckSupport.attributeSet(observer, handles.toArray(new AttributeHandle[0]));
   }

   private static void requestTable8PublishedAttributes(RTIambassador observer,
         Recorder recorder, CallbackModel callbackModel, ObjectInstanceHandle federationObject,
         AttributeHandleSet attributes, TransportationTypeHandle reliable) throws Exception {
      recorder.clearReflection();
      observer.requestAttributeValueUpdate(federationObject, attributes, new byte[0]);
      await(observer, callbackModel, () -> recorder.reflectedObject != null
            && federationObject.equals(recorder.reflectedObject)
            && recorder.reflectedValues != null
            && recorder.reflectedValues.size() == attributes.size()
            && recorder.reflectedValues.keySet().containsAll(attributes),
         "complete HLAfederation Table 8 P-attribute request reflection");
      check(attributes.size() == 16 && recorder.reflectedValues.size() == 16
            && recorder.reflectionTag != null && recorder.reflectionTag.length == 0
            && reliable.equals(recorder.reflectionTransportation)
            && recorder.reflectionRegionsEmpty,
         "Table 8 request did not return all 16 values with standard callback metadata");
      JavaTckSupport.requireNoProducingFederate(observer, recorder.reflectionProducer,
         "HLAfederation Table 8 P-attribute reflection");
   }

   private static void verifyFederationNameNoTransfer(RTIambassador observer,
         Recorder recorder, CallbackModel callbackModel, ObjectInstanceHandle federationObject,
         AttributeHandleSet attributes, AttributeHandle attribute) throws Exception {
      recorder.clearOwnershipRecords();
      OwnershipObservation initial = queryFederationNameOwnership(observer, recorder,
         callbackModel, federationObject, attributes, attribute);
      int acquisitionNotificationsBefore = recorder.ownershipAcquisitions.size();
      try {
         observer.attributeOwnershipAcquisitionIfAvailable(
            federationObject, attributes, new byte[0]);
      } catch (Exception ignored) {
         // The public service does not prescribe one exception for this prohibited transfer.
      }
      OwnershipObservation afterAttempt = queryFederationNameOwnership(observer, recorder,
         callbackModel, federationObject, attributes, attribute);
      check(initial.sameAs(afterAttempt),
         "an acquisition attempt changed HLAfederationName MOM ownership");
      check(recorder.ownershipAcquisitions.size() == acquisitionNotificationsBefore,
         "the RTI transferred HLAfederationName to the requesting federate");
   }

   private static OwnershipObservation queryFederationNameOwnership(RTIambassador observer,
         Recorder recorder, CallbackModel callbackModel, ObjectInstanceHandle federationObject,
         AttributeHandleSet attributes, AttributeHandle attribute) throws Exception {
      check(attributes.size() == 1 && attributes.contains(attribute),
         "HLAfederationName ownership probe must contain only its selected attribute");
      int ownedByRtiBefore = recorder.ownershipOwnedByRti.size();
      int notOwnedBefore = recorder.ownershipNotOwned.size();
      int ownershipInformationBefore = recorder.ownershipInformation.size();
      int responsesBefore = ownedByRtiBefore + notOwnedBefore + ownershipInformationBefore;
      observer.queryAttributeOwnership(federationObject, attributes);
      await(observer, callbackModel, () -> recorder.ownershipOwnedByRti.size()
            + recorder.ownershipNotOwned.size() + recorder.ownershipInformation.size()
            > responsesBefore,
         "HLAfederationName MOM ownership query");

      int ownedByRtiDelta = recorder.ownershipOwnedByRti.size() - ownedByRtiBefore;
      int notOwnedDelta = recorder.ownershipNotOwned.size() - notOwnedBefore;
      int ownershipInformationDelta =
         recorder.ownershipInformation.size() - ownershipInformationBefore;
      check(ownedByRtiDelta + notOwnedDelta + ownershipInformationDelta == 1,
         "HLAfederationName ownership query did not return exactly one status");
      OwnershipEvent event;
      String status;
      if (ownedByRtiDelta == 1) {
         event = recorder.ownershipOwnedByRti.get(recorder.ownershipOwnedByRti.size() - 1);
         status = "RTI";
      } else if (notOwnedDelta == 1) {
         event = recorder.ownershipNotOwned.get(recorder.ownershipNotOwned.size() - 1);
         status = "not-owned";
      } else {
         event = recorder.ownershipInformation.get(recorder.ownershipInformation.size() - 1);
         status = "federate-owner";
      }
      check(federationObject.equals(event.object)
            && new HashSet<>(attributes).equals(event.attributes)
            && ("federate-owner".equals(status) ? event.owner != null : event.owner == null),
         "HLAfederationName ownership callback identified the wrong object, attribute, or owner");
      return new OwnershipObservation(status, event.owner);
   }

   private static final class OwnershipObservation {
      private final String status;
      private final FederateHandle owner;

      private OwnershipObservation(String status, FederateHandle owner) {
         this.status = status;
         this.owner = owner;
      }

      private boolean sameAs(OwnershipObservation other) {
         return status.equals(other.status)
            && (owner == null ? other.owner == null : owner.equals(other.owner));
      }
   }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final Set<AttributeHandle> attributes;
      private final FederateHandle owner;

      private OwnershipEvent(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         attributes = new HashSet<>((AttributeHandleSet) arguments[1]);
         owner = arguments.length >= 3 && arguments[2] instanceof FederateHandle
            ? (FederateHandle) arguments[2] : null;
      }
   }

   private static String decode(EncoderFactory encoder, byte[] value) throws Exception {
      HLAunicodeString decoded = encoder.createHLAunicodeString();
      decoded.decode(value);
      return decoded.getValue();
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            ambassador.evokeCallback(0.01);
         } else {
            Thread.sleep(1L);
         }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java RTI TCK callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static void resign(RTIambassador ambassador) {
      try {
         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
      } catch (Exception ignored) {
      }
   }

   private static void disconnect(RTIambassador ambassador) {
      try {
         ambassador.disconnect();
      } catch (Exception ignored) {
      }
   }

   private static void check(boolean condition, String message) {
      if (!condition) throw new AssertionError(message);
   }

   private static final class Recorder {
      private final List<OwnershipEvent> ownershipOwnedByRti = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> ownershipNotOwned = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> ownershipInformation = new CopyOnWriteArrayList<>();
      private final List<OwnershipEvent> ownershipAcquisitions = new CopyOnWriteArrayList<>();
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile ObjectClassHandle discoveredClass;
      private volatile String discoveredName;
      private volatile ObjectInstanceHandle reflectedObject;
      private volatile Map<AttributeHandle, byte[]> reflectedValues;
      private volatile byte[] reflectionTag;
      private volatile TransportationTypeHandle reflectionTransportation;
      private volatile FederateHandle reflectionProducer;
      private volatile boolean reflectionRegionsEmpty;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("discoverObjectInstance".equals(method.getName())
                     && arguments != null && arguments.length >= 3) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
                  discoveredClass = (ObjectClassHandle) arguments[1];
                  discoveredName = (String) arguments[2];
               } else if ("reflectAttributeValues".equals(method.getName())
                     && arguments != null && arguments.length == 6
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  reflectedObject = (ObjectInstanceHandle) arguments[0];
                  AttributeHandleValueMap source = (AttributeHandleValueMap) arguments[1];
                  Map<AttributeHandle, byte[]> copy = new LinkedHashMap<>();
                  for (AttributeHandle handle : source.keySet()) {
                     copy.put(handle, source.get(handle).clone());
                  }
                  reflectedValues = copy;
                  reflectionTag = arguments[2] instanceof byte[]
                     ? ((byte[]) arguments[2]).clone() : null;
                  reflectionTransportation = (TransportationTypeHandle) arguments[3];
                  reflectionProducer = (FederateHandle) arguments[4];
                  reflectionRegionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
               } else if ("attributeIsOwnedByRTI".equals(method.getName())
                     && arguments != null && arguments.length == 2) {
                  ownershipOwnedByRti.add(new OwnershipEvent(arguments));
               } else if ("attributeIsNotOwned".equals(method.getName())
                     && arguments != null && arguments.length == 2) {
                  ownershipNotOwned.add(new OwnershipEvent(arguments));
               } else if ("informAttributeOwnership".equals(method.getName())
                     && arguments != null && arguments.length == 3) {
                  ownershipInformation.add(new OwnershipEvent(arguments));
               } else if ("attributeOwnershipAcquisitionNotification".equals(method.getName())
                     && arguments != null && arguments.length == 3) {
                  ownershipAcquisitions.add(new OwnershipEvent(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java RTI TCK MOM callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == arguments[0];
               }
               return null;
            });
      }

      private boolean hasIdentity(ObjectInstanceHandle expectedObject,
            AttributeHandle federationName, AttributeHandle rtiVersion,
            AttributeHandle mimDesignator, AttributeHandle timeImplementationName) {
         Map<AttributeHandle, byte[]> values = reflectedValues;
         return expectedObject.equals(reflectedObject) && values != null && values.size() == 4
            && values.containsKey(federationName) && values.containsKey(rtiVersion)
            && values.containsKey(mimDesignator) && values.containsKey(timeImplementationName);
      }

      private void clearReflection() {
         reflectedObject = null;
         reflectedValues = null;
         reflectionTag = null;
         reflectionTransportation = null;
         reflectionProducer = null;
         reflectionRegionsEmpty = false;
      }

      private void clearOwnershipRecords() {
         ownershipOwnedByRti.clear();
         ownershipNotOwned.clear();
         ownershipInformation.clear();
         ownershipAcquisitions.clear();
      }
   }
}
