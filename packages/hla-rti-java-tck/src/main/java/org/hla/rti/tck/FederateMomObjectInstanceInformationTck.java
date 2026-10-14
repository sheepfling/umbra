package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.AttributeHandleFactory;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectClassHandleFactory;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies owned, known, and NULL object-information reports through the standard federate MOM. */
final class FederateMomObjectInstanceInformationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final String REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstanceInformation";
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstanceInformation";

   private FederateMomObjectInstanceInformationTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel,
         String federateType)
         throws Exception {
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-federate-mom-object-info-" + UUID.randomUUID();
      String requesterName = "java-tck-object-info-requester-" + UUID.randomUUID();
      String ownerName = "java-tck-object-info-owner-" + UUID.randomUUID();
      boolean requesterConnected = false, ownerConnected = false, created = false;
      boolean requesterJoined = false, ownerJoined = false;
      boolean classSubscribed = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      ObjectClassHandle objectClass = null;
      AttributeHandleSet objectSubscription = null;
      try {
         requester.connect(recorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         owner.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation);
         created = true;
         FederateHandle requesterFederate = requester.joinFederationExecution(
            requesterName, federateType, federation);
         requesterJoined = true;
         owner.joinFederationExecution(ownerName, federateType, federation);
         ownerJoined = true;

         objectClass = requester.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = requester.getAttributeHandle(objectClass, attributeName);
         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         JavaTckSupport.check(objectClass != null && attribute != null && ownerClass != null
               && ownerAttribute != null && objectClassName.equals(
                  requester.getObjectClassName(objectClass))
               && attributeName.equals(requester.getAttributeName(objectClass, attribute)),
            "adapter-configured object class or attribute did not resolve through the standard API");
         requester.publishObjectClassAttributes(objectClass,
            JavaTckSupport.attributeSet(requester, attribute));
         owner.publishObjectClassAttributes(ownerClass,
            JavaTckSupport.attributeSet(owner, ownerAttribute));
         objectSubscription = JavaTckSupport.attributeSet(requester, attribute);
         requester.subscribeObjectClassAttributes(objectClass, objectSubscription, "");
         classSubscribed = true;

         reportClass = requester.getInteractionClassHandle(REPORT);
         ParameterHandle reportObject = requester.getParameterHandle(reportClass, "HLAobjectInstance");
         ParameterHandle reportAttributes = requester.getParameterHandle(
            reportClass, "HLAownedInstanceAttributeList");
         ParameterHandle reportRegisteredClass = requester.getParameterHandle(
            reportClass, "HLAregisteredClass");
         ParameterHandle reportKnownClass = requester.getParameterHandle(reportClass, "HLAknownClass");
         ParameterHandle reportFederate = requester.getParameterHandle(reportClass, "HLAfederate");
         InteractionClassHandle requestClass = requester.getInteractionClassHandle(REQUEST);
         ParameterHandle requestObject = requester.getParameterHandle(requestClass, "HLAobjectInstance");
         TransportationTypeHandle reliable = requester.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reportClass != null && reportObject != null && reportAttributes != null
               && reportRegisteredClass != null && reportKnownClass != null && reportFederate != null
               && requestClass != null && requestObject != null && reliable != null,
            "standard object-information request/report lookup returned a null handle");
         JavaTckSupport.check(REPORT.equals(requester.getInteractionClassName(reportClass))
               && REQUEST.equals(requester.getInteractionClassName(requestClass))
               && "HLAobjectInstance".equals(requester.getParameterName(reportClass, reportObject))
               && "HLAownedInstanceAttributeList".equals(
                  requester.getParameterName(reportClass, reportAttributes))
               && "HLAregisteredClass".equals(
                  requester.getParameterName(reportClass, reportRegisteredClass))
               && "HLAknownClass".equals(requester.getParameterName(reportClass, reportKnownClass))
               && "HLAfederate".equals(requester.getParameterName(reportClass, reportFederate))
               && "HLAobjectInstance".equals(requester.getParameterName(requestClass, requestObject))
               && "HLAreliable".equals(requester.getTransportationTypeName(reliable)),
            "standard object-information handle names did not round-trip");
         requester.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         ObjectInstanceHandle ownedObject = requester.registerObjectInstance(objectClass);
         JavaTckSupport.check(ownedObject != null, "owned object registration returned null");
         Interaction owned = requestInformation(requester, recorder, requestClass,
            requestObject, ownedObject, callbackModel, "owned object-information report");
         verifyBase(requester, owned, reportClass, reportObject, reportAttributes,
            reportRegisteredClass, reportKnownClass, reportFederate, requesterFederate,
            reliable, ownedObject);
         List<AttributeHandle> ownedAttributes = decodeOwnedAttributes(encoder,
            requester.getAttributeHandleFactory(), owned.parameters.get(reportAttributes));
         JavaTckSupport.check(ownedAttributes.contains(attribute),
            "owned object-information report omitted the requesting federate's attribute");
         JavaTckSupport.check(owned.parameters.containsKey(reportRegisteredClass)
               && owned.parameters.containsKey(reportKnownClass)
               && decodeObjectClass(requester.getObjectClassHandleFactory(),
                  owned.parameters.get(reportRegisteredClass)).equals(objectClass)
               && decodeObjectClass(requester.getObjectClassHandleFactory(),
                  owned.parameters.get(reportKnownClass)).equals(objectClass),
            "owned object-information report returned the wrong registered or known class");

         ObjectInstanceHandle remoteObject = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(remoteObject != null, "remote object registration returned null");
         await(requester, callbackModel, () -> recorder.discoveredObjects.contains(remoteObject),
            "discovery of the object-information query target");
         JavaTckSupport.check(objectClass.equals(requester.getKnownObjectClassHandle(remoteObject)),
            "object-information query target was discovered with a different class");
         Interaction known = requestInformation(requester, recorder, requestClass,
            requestObject, remoteObject, callbackModel, "known object-information report");
         verifyBase(requester, known, reportClass, reportObject, reportAttributes,
            reportRegisteredClass, reportKnownClass, reportFederate, requesterFederate,
            reliable, remoteObject);
         JavaTckSupport.check(decodeOwnedAttributes(encoder,
               requester.getAttributeHandleFactory(), known.parameters.get(reportAttributes)).isEmpty()
               && known.parameters.containsKey(reportRegisteredClass)
               && known.parameters.containsKey(reportKnownClass)
               && objectClass.equals(decodeObjectClass(requester.getObjectClassHandleFactory(),
                  known.parameters.get(reportRegisteredClass)))
               && objectClass.equals(decodeObjectClass(requester.getObjectClassHandleFactory(),
                  known.parameters.get(reportKnownClass))),
            "known-object report did not return an empty owned list and the registered/known class");

         requester.localDeleteObjectInstance(remoteObject);
         Interaction deleted = requestInformation(requester, recorder, requestClass,
            requestObject, remoteObject, callbackModel,
            "NULL object-information report after local deletion");
         verifyBase(requester, deleted, reportClass, reportObject, reportAttributes,
            reportRegisteredClass, reportKnownClass, reportFederate, requesterFederate,
            reliable, remoteObject);
         JavaTckSupport.check(decodeOwnedAttributes(encoder,
               requester.getAttributeHandleFactory(), deleted.parameters.get(reportAttributes)).isEmpty()
               && !deleted.parameters.containsKey(reportRegisteredClass)
               && !deleted.parameters.containsKey(reportKnownClass),
            "NULL object-information report did not use the empty-list/class-omission shape");
      } finally {
         if (reportSubscribed && requesterJoined && reportClass != null) try {
            requester.unsubscribeInteractionClass(reportClass);
         } catch (Exception ignored) { }
         if (classSubscribed && requesterJoined && objectClass != null && objectSubscription != null)
            try { requester.unsubscribeObjectClassAttributes(objectClass, objectSubscription); }
            catch (Exception ignored) { }
         if (requesterJoined) try {
            requester.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         } catch (Exception ignored) { }
         if (ownerJoined) try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) try { requester.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static Interaction requestInformation(RTIambassador requester, Recorder recorder,
         InteractionClassHandle requestClass, ParameterHandle requestObject,
         ObjectInstanceHandle object, CallbackModel callbackModel, String description)
         throws Exception {
      recorder.interactions.clear();
      ParameterHandleValueMap values = requester.getParameterHandleValueMapFactory().create(1);
      values.put(requestObject, encode(object));
      requester.sendInteraction(requestClass, values, new byte[0]);
      await(requester, callbackModel, () -> !recorder.interactions.isEmpty(), description);
      JavaTckSupport.check(recorder.interactions.size() == 1,
         description + " did not deliver exactly one report");
      return recorder.interactions.get(0);
   }

   private static void verifyBase(RTIambassador requester, Interaction report,
         InteractionClassHandle reportClass, ParameterHandle reportObject,
         ParameterHandle reportAttributes, ParameterHandle reportRegisteredClass,
         ParameterHandle reportKnownClass, ParameterHandle reportFederate,
         FederateHandle requesterFederate, TransportationTypeHandle reliable,
         ObjectInstanceHandle expectedObject) throws Exception {
      JavaTckSupport.check(report.present && reportClass.equals(report.interaction)
            && reliable.equals(report.transportation) && report.tag.length == 0
            && report.regionsEmpty && report.parameters.containsKey(reportObject)
            && report.parameters.containsKey(reportAttributes),
         "object-information report omitted standard fields or reliable delivery metadata");
      int expectedSize = 2 + (report.parameters.containsKey(reportRegisteredClass) ? 1 : 0)
         + (report.parameters.containsKey(reportKnownClass) ? 1 : 0)
         + (report.parameters.containsKey(reportFederate) ? 1 : 0);
      JavaTckSupport.check(report.parameters.size() == expectedSize,
         "object-information report contained parameters outside its standard response shape");
      ObjectInstanceHandle decodedObject = requester.getObjectInstanceHandleFactory().decode(
         report.parameters.get(reportObject), 0);
      JavaTckSupport.check(expectedObject.equals(decodedObject),
         "object-information report identified a different object instance");
      if (report.parameters.containsKey(reportFederate)) {
         JavaTckSupport.check(java.util.Arrays.equals(encode(requesterFederate),
               report.parameters.get(reportFederate)),
            "object-information report identified a different joined federate");
      }
   }

   private static List<AttributeHandle> decodeOwnedAttributes(EncoderFactory encoder,
         AttributeHandleFactory handleFactory, byte[] encoded) throws Exception {
      HLAvariableArray<HLAvariableArray<HLAbyte>> references = encoder.createHLAvariableArray(
         outer -> encoder.createHLAvariableArray(inner -> encoder.createHLAbyte()));
      references.decode(encoded);
      List<AttributeHandle> result = new ArrayList<>();
      for (HLAvariableArray<HLAbyte> reference : references) {
         result.add(handleFactory.decode(reference.toByteArray(), 0));
      }
      return result;
   }

   private static ObjectClassHandle decodeObjectClass(ObjectClassHandleFactory factory,
         byte[] encoded) throws Exception {
      return factory.decode(encoded, 0);
   }

   private static byte[] encode(ObjectInstanceHandle handle) throws Exception {
      byte[] encoded = new byte[handle.encodedLength()];
      handle.encode(encoded, 0);
      return encoded;
   }

   private static byte[] encode(FederateHandle handle) throws Exception {
      byte[] encoded = new byte[handle.encodedLength()];
      handle.encode(encoded, 0);
      return encoded;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
         Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, args) -> objectMethod(proxy, method.getName(), args));
   }

   private static Object objectMethod(Object proxy, String name, Object[] args) {
      if ("toString".equals(name)) return "Java TCK object-information callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
      return null;
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private boolean regionsEmpty;
      private boolean present;
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveredObjects = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               if ("discoverObjectInstance".equals(method.getName()) && args != null
                     && args.length >= 2) {
                  discoveredObjects.add((ObjectInstanceHandle) args[0]);
               } else if ("receiveInteraction".equals(method.getName()) && args != null
                     && args.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) args[0];
                  event.parameters = ((ParameterHandleValueMap) args[1]).clone();
                  event.tag = ((byte[]) args[2]).clone();
                  event.transportation = (TransportationTypeHandle) args[3];
                  RegionHandleSet regions = (RegionHandleSet) args[5];
                  event.regionsEmpty = regions == null || regions.isEmpty();
                  event.present = true;
                  interactions.add(event);
               }
               return objectMethod(proxy, method.getName(), args);
            });
      }
   }
}
