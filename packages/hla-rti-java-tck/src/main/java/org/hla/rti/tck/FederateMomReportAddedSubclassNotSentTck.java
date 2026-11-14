package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies that the RTI reports service invocations at the predefined MOM class. */
final class FederateMomReportAddedSubclassNotSentTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 15_000_000_000L;
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String ADDED_REPORT_SUBCLASS = REPORT_CLASS
      + ".TckReportServiceInvocationExtension";

   private FederateMomReportAddedSubclassNotSentTck() { }

   static void run(RtiFactory factory, String fom, String extensionFom, String mim,
         String timeImplementation, String interactionName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      String federateType = System.getProperty("hla.rti.tck.federateType", "tck-type");
      JavaTckSupport.check(!federateType.isEmpty(),
         "adapter-supplied federate type must not be empty");

      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador parentObserver = factory.getRtiAmbassador();
      RTIambassador extensionObserver = factory.getRtiAmbassador();
      Recorder parentRecorder = new Recorder();
      Recorder extensionRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-report-added-subclass-" + suffix;
      boolean publisherConnected = false, parentConnected = false, extensionConnected = false;
      boolean created = false;
      boolean publisherJoined = false, parentJoined = false, extensionJoined = false;
      boolean parentSubscribed = false, extensionSubscribed = false, published = false;
      InteractionClassHandle parentReportClass = null, extensionReportClass = null;
      InteractionClassHandle applicationClass = null;
      try {
         publisher.connect(callbacks(), callbackModel);
         publisherConnected = true;
         parentObserver.connect(parentRecorder.proxy(), callbackModel);
         parentConnected = true;
         extensionObserver.connect(extensionRecorder.proxy(), callbackModel);
         extensionConnected = true;

         publisher.createFederationExecutionWithMIM(federation,
            new String[] {fom, extensionFom}, mim, timeImplementation);
         created = true;
         publisher.joinFederationExecution("java-tck-mom-subclass-publisher-" + suffix,
            federateType, federation);
         publisherJoined = true;
         parentObserver.joinFederationExecution(
            "java-tck-mom-subclass-parent-observer-" + suffix, federateType, federation);
         parentJoined = true;
         extensionObserver.joinFederationExecution(
            "java-tck-mom-subclass-extension-observer-" + suffix, federateType, federation);
         extensionJoined = true;

         parentReportClass = parentObserver.getInteractionClassHandle(REPORT_CLASS);
         extensionReportClass = extensionObserver.getInteractionClassHandle(ADDED_REPORT_SUBCLASS);
         JavaTckSupport.check(parentReportClass != null && extensionReportClass != null
               && REPORT_CLASS.equals(parentObserver.getInteractionClassName(parentReportClass))
               && ADDED_REPORT_SUBCLASS.equals(
                  extensionObserver.getInteractionClassName(extensionReportClass)),
            "standard HLAreportServiceInvocation or its FOM-added subclass was unavailable");
         parentObserver.subscribeInteractionClass(parentReportClass);
         parentSubscribed = true;
         extensionObserver.subscribeInteractionClass(extensionReportClass);
         extensionSubscribed = true;

         applicationClass = publisher.getInteractionClassHandle(interactionName);
         ParameterHandle applicationParameter = publisher.getParameterHandle(
            applicationClass, parameterName);
         JavaTckSupport.check(applicationClass != null && applicationParameter != null,
            "adapter application interaction or parameter was unavailable");
         publisher.publishInteractionClass(applicationClass);
         published = true;
         publisher.setServiceReportingSwitch(false);
         parentRecorder.interactions.clear();
         extensionRecorder.interactions.clear();
         publisher.setServiceReportingSwitch(true);

         ParameterHandleValueMap values =
            publisher.getParameterHandleValueMapFactory().create(1);
         values.put(applicationParameter, new byte[] {0x53});
         publisher.sendInteraction(applicationClass, values, new byte[0]);
         await(parentObserver, callbackModel,
            () -> !parentRecorder.interactions.isEmpty(),
            "the standard HLAreportServiceInvocation callback");
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(extensionObserver);
         }

         JavaTckSupport.check(parentRecorder.interactions.size() == 1
               && parentReportClass.equals(parentRecorder.interactions.get(0)),
            "the RTI did not send exactly one report at the predefined HLAreportServiceInvocation class");
         JavaTckSupport.check(extensionRecorder.interactions.stream()
               .noneMatch(extensionReportClass::equals),
            "the RTI sent an HLAreportServiceInvocation interaction using the FOM-added subclass");
      } finally {
         if (parentJoined && parentSubscribed && parentReportClass != null) {
            try { parentObserver.unsubscribeInteractionClass(parentReportClass); }
            catch (Exception ignored) { }
         }
         if (extensionJoined && extensionSubscribed && extensionReportClass != null) {
            try { extensionObserver.unsubscribeInteractionClass(extensionReportClass); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && published && applicationClass != null) {
            try { publisher.unpublishInteractionClass(applicationClass); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         }
         if (extensionJoined) {
            try { extensionObserver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (parentJoined) {
            try { parentObserver.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (extensionConnected) {
            try { extensionObserver.disconnect(); } catch (Exception ignored) { }
         }
         if (parentConnected) {
            try { parentObserver.disconnect(); } catch (Exception ignored) { }
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
      if ("toString".equals(name)) return "Java TCK MOM report-subclass callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
      return null;
   }

   private static final class Recorder {
      private final List<InteractionClassHandle> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null
                     && arguments.length == 6) {
                  interactions.add((InteractionClassHandle) arguments[0]);
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
