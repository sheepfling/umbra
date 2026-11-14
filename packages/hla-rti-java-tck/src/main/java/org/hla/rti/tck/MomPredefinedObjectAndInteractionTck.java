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
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies predefined MOM interaction families, the federate object, and a service report. */
final class MomPredefinedObjectAndInteractionTck {
   private static final String MOM_FEDERATE_CLASS =
      "HLAobjectRoot.HLAmanager.HLAfederate";
   private static final String MOM_REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;

   private MomPredefinedObjectAndInteractionTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String applicationInteractionName, String applicationParameterName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder subjectRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-predefined-" + suffix;
      String subjectName = "java-tck-mom-predefined-subject-" + suffix;
      String observerName = "java-tck-mom-predefined-observer-" + suffix;
      boolean subjectConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false;
      boolean observerJoined = false;
      boolean momSubscribed = false;
      boolean reportSubscribed = false;
      boolean applicationPublished = false;
      ObjectClassHandle momClass = null;
      AttributeHandleSet momAttributes = null;
      InteractionClassHandle reportClass = null;
      InteractionClassHandle applicationInteraction = null;
      try {
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            subjectName, "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(observerName, "java-tck", federation);
         observerJoined = true;

         for (String interactionClassName : List.of(
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust",
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches",
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest",
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestMIMdata",
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport",
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata",
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust",
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming",
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest",
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications",
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport",
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation",
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice",
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAenableTimeRegulation")) {
            InteractionClassHandle interactionClass =
               observer.getInteractionClassHandle(interactionClassName);
            JavaTckSupport.check(interactionClass != null
                  && interactionClassName.equals(
                     observer.getInteractionClassName(interactionClass)),
               "a Figure 20 MOM family or selected subclass did not resolve through the public API");
         }

         momClass = observer.getObjectClassHandle(MOM_FEDERATE_CLASS);
         AttributeHandle federateHandleAttribute = observer.getAttributeHandle(
            momClass, "HLAfederateHandle");
         AttributeHandle federateNameAttribute = observer.getAttributeHandle(
            momClass, "HLAfederateName");
         JavaTckSupport.check(MOM_FEDERATE_CLASS.equals(observer.getObjectClassName(momClass))
               && "HLAfederateHandle".equals(observer.getAttributeName(
                  momClass, federateHandleAttribute))
               && "HLAfederateName".equals(observer.getAttributeName(
                  momClass, federateNameAttribute)),
            "standard HLAfederate MOM object lookup did not round-trip");
         momAttributes = JavaTckSupport.attributeSet(
            observer, federateHandleAttribute, federateNameAttribute);
         observer.subscribeObjectClassAttributes(momClass, momAttributes, "");
         momSubscribed = true;

         reportClass = observer.getInteractionClassHandle(MOM_REPORT_CLASS);
         ParameterHandle reportService = observer.getParameterHandle(reportClass, "HLAservice");
         ParameterHandle reportFederate = observer.getParameterHandle(reportClass, "HLAfederate");
         JavaTckSupport.check(MOM_REPORT_CLASS.equals(
                  observer.getInteractionClassName(reportClass))
               && "HLAservice".equals(observer.getParameterName(reportClass, reportService))
               && "HLAfederate".equals(observer.getParameterName(reportClass, reportFederate)),
            "standard HLAreportServiceInvocation lookup did not round-trip");
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         applicationInteraction =
            subject.getInteractionClassHandle(applicationInteractionName);
         ParameterHandle applicationParameter = subject.getParameterHandle(
            applicationInteraction, applicationParameterName);
         JavaTckSupport.check(applicationInteraction != null && applicationParameter != null,
            "adapter-supplied application interaction or parameter is unavailable");
         subject.publishInteractionClass(applicationInteraction);
         applicationPublished = true;

         for (RTIambassador ambassador : new RTIambassador[] {subject, observer}) {
            ambassador.setServiceReportingSwitch(false);
            ambassador.setSendServiceReportsToFileSwitch(false);
         }

         byte[] encodedSubject = encode(subjectFederate);
         final AttributeHandle handleAttribute = federateHandleAttribute;
         final ObjectClassHandle expectedMomClass = momClass;
         await(observer, callbackModel,
            () -> observerRecorder.subjectMomReflection(
               handleAttribute, encodedSubject, expectedMomClass) != null,
            "the joined federate's predefined HLAfederate MOM object");
         Reflection subjectReflection = observerRecorder.subjectMomReflection(
            federateHandleAttribute, encodedSubject, momClass);
         JavaTckSupport.check(subjectReflection != null
               && momClass.equals(observer.getKnownObjectClassHandle(subjectReflection.object))
               && subjectReflection.values.containsKey(federateNameAttribute),
            "the joined-federate MOM reflection omitted its standard class or name");
         HLAunicodeString reflectedName = encoder.createHLAunicodeString();
         reflectedName.decode(subjectReflection.values.get(federateNameAttribute));
         JavaTckSupport.check(subjectName.equals(reflectedName.getValue()),
            "the predefined HLAfederate MOM object identified the wrong federate");

         observerRecorder.interactions.clear();
         subject.setServiceReportingSwitch(true);
         ParameterHandleValueMap values =
            subject.getParameterHandleValueMapFactory().create(1);
         values.put(applicationParameter, new byte[] {0x4D, 0x4F, 0x4D});
         subject.sendInteraction(applicationInteraction, values, new byte[0]);
         final InteractionClassHandle expectedReportClass = reportClass;
         await(observer, callbackModel,
            () -> observerRecorder.interaction(expectedReportClass) != null,
            "the predefined HLAreportServiceInvocation callback");
         Interaction report = observerRecorder.interaction(reportClass);
         JavaTckSupport.check(report != null && report.tag != null && report.tag.length == 0
               && report.parameters.containsKey(reportService)
               && report.parameters.containsKey(reportFederate),
            "the predefined MOM interaction omitted its standard identity or parameters");
         JavaTckSupport.requireNoProducingFederate(
            observer, report.producer, "RTI-originated SendInteraction MOM report");
         HLAunicodeString reportedService = encoder.createHLAunicodeString();
         reportedService.decode(report.parameters.get(reportService));
         FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
            report.parameters.get(reportFederate), 0);
         JavaTckSupport.check("SendInteraction".equals(reportedService.getValue())
               && subjectFederate.equals(reportedFederate),
            "the predefined MOM interaction did not identify the public SendInteraction call");
      } finally {
         if (applicationPublished && subjectJoined && applicationInteraction != null) {
            try { subject.unpublishInteractionClass(applicationInteraction); }
            catch (Exception ignored) { }
         }
         if (momSubscribed && observerJoined && momClass != null && momAttributes != null) {
            try { observer.unsubscribeObjectClassAttributes(momClass, momAttributes); }
            catch (Exception ignored) { }
         }
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.setServiceReportingSwitch(false); }
            catch (Exception ignored) { }
            try { subject.setSendServiceReportsToFileSwitch(false); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.setServiceReportingSwitch(false); }
            catch (Exception ignored) { }
            try { observer.setSendServiceReportsToFileSwitch(false); }
            catch (Exception ignored) { }
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); }
            catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static byte[] encode(FederateHandle federate) {
      byte[] result = new byte[federate.encodedLength()];
      federate.encode(result, 0);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier ready, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!ready.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(ready.getAsBoolean(), "timed out waiting for " + description);
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK predefined MOM callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Recorder {
      private final Map<ObjectInstanceHandle, ObjectClassHandle> discovered =
         new ConcurrentHashMap<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 2) {
                  discovered.put(
                     (ObjectInstanceHandle) arguments[0], (ObjectClassHandle) arguments[1]);
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length >= 2
                     && arguments[1] instanceof AttributeHandleValueMap) {
                  reflections.add(new Reflection(arguments));
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 6
                     && arguments[1] instanceof ParameterHandleValueMap) {
                  interactions.add(new Interaction(arguments));
               }
               return objectMethod(proxy, name, arguments);
            });
      }

      private Reflection subjectMomReflection(AttributeHandle handle, byte[] encodedSubject,
            ObjectClassHandle expectedClass) {
         for (Reflection reflection : reflections) {
            byte[] actual = reflection.values.get(handle);
            if (actual != null && Arrays.equals(encodedSubject, actual)
                  && expectedClass.equals(discovered.get(reflection.object))) {
               return reflection;
            }
         }
         return null;
      }

      private Interaction interaction(InteractionClassHandle expectedClass) {
         for (Interaction interaction : interactions) {
            if (expectedClass.equals(interaction.interaction)) return interaction;
         }
         return null;
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values = new LinkedHashMap<>();

      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         AttributeHandleValueMap source = (AttributeHandleValueMap) arguments[1];
         for (AttributeHandle handle : source.keySet()) {
            values.put(handle, source.get(handle).clone());
         }
      }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new LinkedHashMap<>();
      private final byte[] tag;
      private final FederateHandle producer;

      private Interaction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
         for (ParameterHandle handle : source.keySet()) {
            parameters.put(handle, source.get(handle).clone());
         }
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
      }
   }
}
