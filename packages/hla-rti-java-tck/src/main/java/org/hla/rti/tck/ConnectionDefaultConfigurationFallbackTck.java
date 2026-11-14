package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionInformationSet;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Checks that an unknown configuration name falls back to the same default as no name. */
final class ConnectionDefaultConfigurationFallbackTck {
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private ConnectionDefaultConfigurationFallbackTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied FOM is required");
      RTIambassador defaultConnection = factory.getRtiAmbassador();
      RTIambassador unknownNameConnection = factory.getRtiAmbassador();
      FederationRecorder recorder = new FederationRecorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-default-configuration-fallback-" + suffix;
      boolean defaultConnected = false;
      boolean federationCreated = false;
      boolean unknownConnected = false;
      try {
         defaultConnection.connect(new NullFederateAmbassador(), callbackModel);
         defaultConnected = true;
         defaultConnection.createFederationExecution(
            federation, new String[] {fom}, timeImplementation);
         federationCreated = true;

         RtiConfiguration unknownConfiguration = RtiConfiguration.createConfiguration()
            .withConfigurationName("java-tck-unknown-configuration-" + suffix);
         unknownNameConnection.connect(recorder.proxy(), callbackModel, unknownConfiguration);
         unknownConnected = true;
         int firstReport = recorder.reports.size();
         unknownNameConnection.listFederationExecutions();
         await(callbackModel, unknownNameConnection,
            () -> recorder.listsFederation(federation, firstReport),
            "the default-created federation after Connect with an unknown configuration name");

         unknownNameConnection.disconnect();
         unknownConnected = false;
         defaultConnection.destroyFederationExecution(federation);
         federationCreated = false;
         defaultConnection.disconnect();
         defaultConnected = false;
      } finally {
         if (unknownConnected) disconnect(unknownNameConnection);
         if (federationCreated) {
            try { defaultConnection.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (defaultConnected) disconnect(defaultConnection);
      }
   }

   private static void await(CallbackModel callbackModel, RTIambassador ambassador,
         Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.isMet()) {
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

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   @FunctionalInterface
   private interface Condition { boolean isMet(); }

   private static final class FederationRecorder {
      private final List<FederationExecutionInformationSet> reports =
         new CopyOnWriteArrayList<>();

      private boolean listsFederation(String federation, int firstReport) {
         for (int index = firstReport; index < reports.size(); ++index) {
            for (FederationExecutionInformation info : reports.get(index)) {
               if (federation.equals(info.federationExecutionName)) return true;
            }
         }
         return false;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("reportFederationExecutions".equals(name)
                     && arguments != null && arguments.length == 1) {
                  reports.add(
                     ((FederationExecutionInformationSet) arguments[0]).clone());
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK configuration fallback recorder";
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
