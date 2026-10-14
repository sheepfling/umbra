package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies MOM reports and provider callbacks for both standard AVU request overloads. */
final class ServiceReportRequestAttributeValueUpdateTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REPORT_CLASS_NAME =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETER_NAMES = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };

   private ServiceReportRequestAttributeValueUpdateTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-request-avu-" + UUID.randomUUID();
      boolean ownerConnected = false, requesterConnected = false, observerConnected = false;
      boolean created = false, ownerJoined = false, requesterJoined = false, observerJoined = false;
      boolean published = false, subscribed = false, reportSubscribed = false;
      ObjectClassHandle ownerClass = null, requesterClass = null;
      AttributeHandle ownerAttribute = null, requesterAttribute = null;
      AttributeHandleSet ownerAttributes = null, requesterAttributes = null;
      InteractionClassHandle reportClass = null;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-avu-report-owner", "java-tck", federation);
         ownerJoined = true;
         FederateHandle requesterHandle = requester.joinFederationExecution(
            "java-tck-avu-report-requester", "java-tck", federation);
         requesterJoined = true;
         observer.joinFederationExecution("java-tck-avu-report-observer", "java-tck", federation);
         observerJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         requesterAttribute = requester.getAttributeHandle(requesterClass, attributeName);
         JavaTckSupport.check(ownerClass != null && ownerAttribute != null
               && requesterClass != null && requesterAttribute != null
               && objectClassName.equals(owner.getObjectClassName(ownerClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute)),
            "adapter-supplied object class or attribute lookup failed");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS_NAME);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS_NAME.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETER_NAMES) {
            ParameterHandle handle = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(handle != null
                  && name.equals(observer.getParameterName(reportClass, handle)),
               "standard service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, handle);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup/name round-trip failed");

         for (RTIambassador ambassador : new RTIambassador[] {owner, requester, observer}) {
            ambassador.setServiceReportingSwitch(false);
            ambassador.setSendServiceReportsToFileSwitch(false);
         }
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         published = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes);
         subscribed = true;
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null, "object registration returned an invalid handle");
         await(requester, callbackModel, () -> requesterRecorder.discoveries.size() == 1,
            "ordinary object discovery before RequestAttributeValueUpdate");
         Discovery discovery = requesterRecorder.discoveries.get(0);
         JavaTckSupport.check(object.equals(discovery.object)
               && requesterClass.equals(discovery.objectClass),
            "requester discovered the wrong object instance or class");

         requesterRecorder.providedUpdates.clear();
         observerRecorder.interactions.clear();
         requester.setServiceReportingSwitch(true);
         byte[] instanceTag = new byte[] {0x52, 0x51, 0x54};
         requester.requestAttributeValueUpdate(discovery.object, requesterAttributes, instanceTag);
         await(observer, callbackModel, () -> !observerRecorder.interactions.isEmpty(),
            "known-object RequestAttributeValueUpdate service report");
         await(owner, callbackModel, () -> !ownerRecorder.providedUpdates.isEmpty(),
            "known-object provideAttributeValueUpdate callback");
         verifyReport(observer, observerRecorder, reportClass, reportParameters, reliable,
            requesterHandle, encoder, 0);
         verifyProvider(ownerRecorder, object, ownerAttributes, instanceTag,
            "known-object RequestAttributeValueUpdate");

         ownerRecorder.providedUpdates.clear();
         observerRecorder.interactions.clear();
         byte[] classTag = new byte[] {0x43, 0x4c, 0x53};
         requester.requestAttributeValueUpdate(requesterClass, requesterAttributes, classTag);
         await(observer, callbackModel, () -> !observerRecorder.interactions.isEmpty(),
            "object-class RequestAttributeValueUpdate service report");
         await(owner, callbackModel, () -> !ownerRecorder.providedUpdates.isEmpty(),
            "object-class provideAttributeValueUpdate callback");
         verifyReport(observer, observerRecorder, reportClass, reportParameters, reliable,
            requesterHandle, encoder, 1);
         verifyProvider(ownerRecorder, object, ownerAttributes, classTag,
            "object-class RequestAttributeValueUpdate");
      } finally {
         if (requesterJoined) {
            try { requester.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         }
         if (observerJoined && reportSubscribed) {
            try { observer.unsubscribeInteractionClass(reportClass); } catch (Exception ignored) { }
         }
         if (requesterJoined && subscribed) {
            try { requester.unsubscribeObjectClassAttributes(requesterClass, requesterAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerJoined && published) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (requesterConnected) {
            try { requester.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void verifyReport(RTIambassador observer, Recorder recorder,
         InteractionClassHandle reportClass, Map<String, ParameterHandle> reportParameters,
         TransportationTypeHandle reliable, FederateHandle requesterHandle,
         EncoderFactory encoder, int serialNumber) throws Exception {
      JavaTckSupport.check(recorder.interactions.size() == 1,
         "RequestAttributeValueUpdate did not produce exactly one service report");
      Interaction event = recorder.interactions.get(0);
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETER_NAMES.length
            && event.tag.length == 0 && event.producer == null && event.regionsEmpty
            && reliable.equals(event.transportation),
         "RequestAttributeValueUpdate report had invalid callback identity or metadata");
      for (String name : REPORT_PARAMETER_NAMES) {
         JavaTckSupport.check(event.parameters.containsKey(reportParameters.get(name)),
            "service report omitted standard parameter " + name);
      }
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         event.parameters.get(reportParameters.get("HLAfederate")), 0);
      JavaTckSupport.check(requesterHandle.equals(reportedFederate),
         "service report identified the wrong invoking federate");
      HLAunicodeString service = encoder.createHLAunicodeString();
      service.decode(event.parameters.get(reportParameters.get("HLAservice")));
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(event.parameters.get(reportParameters.get("HLAserviceType")));
      HLAboolean success = encoder.createHLAboolean();
      success.decode(event.parameters.get(reportParameters.get("HLAsuccessIndicator")));
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(event.parameters.get(reportParameters.get("HLAexception")));
      HLAinteger32BE serial = encoder.createHLAinteger32BE();
      serial.decode(event.parameters.get(reportParameters.get("HLAserialNumber")));
      JavaTckSupport.check("RequestAttributeValueUpdate".equals(service.getValue())
            && serviceType.getValue() == 2 && success.getValue()
            && exception.getValue().isEmpty() && serial.getValue() == serialNumber,
         "RequestAttributeValueUpdate report contained an incorrect result or serial number");
      recorder.interactions.clear();
   }

   private static void verifyProvider(Recorder recorder, ObjectInstanceHandle object,
         AttributeHandleSet attributes, byte[] tag, String description) {
      JavaTckSupport.check(recorder.providedUpdates.size() == 1,
         description + " produced an unexpected provider callback count");
      ProvidedUpdate update = recorder.providedUpdates.get(0);
      JavaTckSupport.check(object.equals(update.object) && attributes.equals(update.attributes)
            && java.util.Arrays.equals(tag, update.tag),
         description + " provider callback returned the wrong object, attributes, or tag");
      recorder.providedUpdates.clear();
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.isMet() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.isMet(), "timed out waiting for " + description);
   }

   @FunctionalInterface
   private interface Condition { boolean isMet(); }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
   }

   private static final class ProvidedUpdate {
      private ObjectInstanceHandle object;
      private AttributeHandleSet attributes;
      private byte[] tag;
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private Map<ParameterHandle, byte[]> parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<ProvidedUpdate> providedUpdates = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length == 4) {
                  Discovery discovery = new Discovery();
                  discovery.object = (ObjectInstanceHandle) arguments[0];
                  discovery.objectClass = (ObjectClassHandle) arguments[1];
                  discoveries.add(discovery);
               }
               if ("provideAttributeValueUpdate".equals(name) && arguments != null
                     && arguments.length == 3) {
                  ProvidedUpdate update = new ProvidedUpdate();
                  update.object = (ObjectInstanceHandle) arguments[0];
                  update.attributes = (AttributeHandleSet) arguments[1];
                  update.tag = ((byte[]) arguments[2]).clone();
                  providedUpdates.add(update);
               }
               if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  Interaction event = new Interaction();
                  event.interaction = (InteractionClassHandle) arguments[0];
                  ParameterHandleValueMap values = (ParameterHandleValueMap) arguments[1];
                  event.parameters = new LinkedHashMap<>();
                  for (ParameterHandle handle : values.keySet()) {
                     event.parameters.put(handle, values.get(handle).clone());
                  }
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  interactions.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK AVU service-report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
