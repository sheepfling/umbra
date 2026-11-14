package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
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
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies RTI service reports omit parameters added to the predefined report class. */
final class ServiceReportInvocationExtensionParameterTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final List<String> STANDARD_PARAMETER_NAMES = List.of(
      "HLAservice",
      "HLAserviceType",
      "HLAsuccessIndicator",
      "HLAsuppliedArguments",
      "HLAreturnedArgument",
      "HLAexception",
      "HLAserialNumber",
      "HLAfederate");
   private static final String EXTENSION_PARAMETER_NAME =
      "TckServiceInvocationExtensionPayload";

   private ServiceReportInvocationExtensionParameterTck() { }

   static void run(RtiFactory factory, String fom, String extensionFom, String mim,
         String timeImplementation, String interactionClassName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      String federateType = System.getProperty("hla.rti.tck.federateType", "tck-type");
      JavaTckSupport.check(!federateType.isEmpty(),
         "adapter-supplied federate type must not be empty");

      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-service-report-extension-parameter-" + suffix;
      String publisherName = "java-tck-service-report-extension-publisher-" + suffix;
      boolean publisherConnected = false, observerConnected = false, created = false;
      boolean publisherJoined = false, observerJoined = false, subscribed = false;
      boolean reportingEnabled = false;
      InteractionClassHandle reportClass = null, applicationClass = null;
      try {
         publisher.connect(callbacks(), callbackModel);
         publisherConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         publisher.createFederationExecutionWithMIM(federation,
            new String[] {fom, extensionFom}, mim, timeImplementation);
         created = true;
         publisher.joinFederationExecution(publisherName, federateType, federation);
         publisherJoined = true;
         observer.joinFederationExecution(
            "java-tck-service-report-extension-observer-" + suffix,
            federateType, federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null,
            "the standard HLAreportServiceInvocation class was unavailable");
         List<ParameterHandle> standardParameters = new ArrayList<>();
         for (String name : STANDARD_PARAMETER_NAMES) {
            ParameterHandle handle = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(handle != null,
               "a predefined HLAreportServiceInvocation parameter was unavailable: " + name);
            standardParameters.add(handle);
         }
         ParameterHandle extensionParameter =
            observer.getParameterHandle(reportClass, EXTENSION_PARAMETER_NAME);
         JavaTckSupport.check(extensionParameter != null,
            "the FOM-added service-invocation report parameter was unavailable");
         observer.subscribeInteractionClass(reportClass);
         subscribed = true;

         applicationClass = publisher.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(applicationClass != null,
            "the adapter application interaction was unavailable");
         ParameterHandle applicationParameter =
            publisher.getParameterHandle(applicationClass, parameterName);
         JavaTckSupport.check(applicationParameter != null,
            "the adapter application parameter was unavailable");
         publisher.publishInteractionClass(applicationClass);
         recorder.interactions.clear();
         publisher.setServiceReportingSwitch(true);
         reportingEnabled = true;

         ParameterHandleValueMap payload =
            publisher.getParameterHandleValueMapFactory().create(1);
         payload.put(applicationParameter, new byte[] {0x52, 0x45, 0x50, 0x54});
         publisher.sendInteraction(applicationClass, payload, new byte[0]);
         await(observer, callbackModel, () -> !recorder.interactions.isEmpty(),
            "the RTI-originated HLAreportServiceInvocation callback");

         JavaTckSupport.check(recorder.interactions.size() == 1
               && reportClass.equals(recorder.interactions.get(0).interaction),
            "the application send did not produce exactly one standard service-invocation report");
         Interaction report = recorder.interactions.get(0);
         FederateHandle publisherHandle = publisher.getFederateHandle(publisherName);
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(publisherHandle != null && reliable != null
               && reliable.equals(report.transportation)
               && report.parameters.size() == standardParameters.size()
               && !report.parameters.containsKey(extensionParameter),
            "the RTI-originated service report used the wrong transport, added a FOM-only parameter, or omitted standard parameter cardinality");
         for (int index = 0; index < standardParameters.size(); ++index) {
            JavaTckSupport.check(report.parameters.containsKey(standardParameters.get(index)),
               "the RTI-originated service report omitted predefined parameter: "
                  + STANDARD_PARAMETER_NAMES.get(index));
         }

         FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
            report.parameters.get(standardParameters.get(7)), 0);
         JavaTckSupport.check(publisherHandle.equals(reportedFederate),
            "the service report identified the wrong joined federate");
         HLAunicodeString service = encoder.createHLAunicodeString();
         service.decode(report.parameters.get(standardParameters.get(0)));
         HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
         serviceType.decode(report.parameters.get(standardParameters.get(1)));
         HLAboolean success = encoder.createHLAboolean();
         success.decode(report.parameters.get(standardParameters.get(2)));
         HLAunicodeString exception = encoder.createHLAunicodeString();
         exception.decode(report.parameters.get(standardParameters.get(5)));
         HLAinteger32BE serialNumber = encoder.createHLAinteger32BE();
         serialNumber.decode(report.parameters.get(standardParameters.get(6)));
         JavaTckSupport.check("SendInteraction".equals(service.getValue())
               && serviceType.getValue() == 2 && success.getValue()
               && exception.getValue().isEmpty() && serialNumber.getValue() == 0,
            "the RTI-originated standard service report did not preserve its required base fields");
      } finally {
         if (observerJoined && subscribed && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && reportingEnabled) {
            try { publisher.setServiceReportingSwitch(false); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
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
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK service-report publisher callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters = new LinkedHashMap<>();
      private TransportationTypeHandle transportation;
   }

   private static final class Recorder {
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null
                     && arguments.length == 6) {
                  Interaction interaction = new Interaction();
                  interaction.interaction = (InteractionClassHandle) arguments[0];
                  ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
                  for (ParameterHandle handle : source.keySet()) {
                     interaction.parameters.put(handle, source.get(handle).clone());
                  }
                  interaction.transportation = (TransportationTypeHandle) arguments[3];
                  interactions.add(interaction);
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
