package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicReference;

/** Observe adapter-induced connection loss using only the standard HLA Java API. */
final class ConnectionLossCleanupTck {
   private ConnectionLossCleanupTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, CallbackModel callbackModel,
         String readyMarkerPath, String completionMarkerPath, long timeoutMillis)
         throws Exception {
      JavaTckSupport.check(readyMarkerPath != null && !readyMarkerPath.isEmpty(),
         "adapter connection-loss readiness marker is required");
      JavaTckSupport.check(completionMarkerPath != null && !completionMarkerPath.isEmpty(),
         "adapter connection-loss completion marker is required");
      JavaTckSupport.check(timeoutMillis > 0, "connection-loss timeout must be positive");

      RTIambassador survivor = factory.getRtiAmbassador();
      RTIambassador lost = factory.getRtiAmbassador();
      AtomicReference<String> survivorFaultDescription = new AtomicReference<>();
      AtomicReference<String> lostFaultDescription = new AtomicReference<>();
      String federation = "java-tck-connection-loss-" + UUID.randomUUID();
      String survivorName = "java-tck-connection-loss-survivor-" + UUID.randomUUID();
      String lostName = "java-tck-connection-loss-member-" + UUID.randomUUID();
      boolean survivorConnected = false;
      boolean lostConnected = false;
      boolean created = false;
      boolean survivorJoined = false;
      try {
         survivor.connect(callbackProxy(survivorFaultDescription), callbackModel);
         survivorConnected = true;
         lost.connect(callbackProxy(lostFaultDescription), callbackModel);
         lostConnected = true;
         survivor.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         survivor.joinFederationExecution(survivorName, federation);
         survivorJoined = true;
         lost.joinFederationExecution(lostName, federation);
         InteractionClassHandle interactionClass =
            survivor.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(interactionClass != null,
            "surviving member could not resolve the adapter-configured interaction class");
         survivor.publishInteractionClass(interactionClass);

         Path readyMarker = Path.of(readyMarkerPath);
         Path parent = readyMarker.toAbsolutePath().getParent();
         if (parent != null) Files.createDirectories(parent);
         Files.write(readyMarker, "ready\n".getBytes(StandardCharsets.UTF_8));

         long deadline = System.nanoTime() + timeoutMillis * 1_000_000L;
         while (lostFaultDescription.get() == null && System.nanoTime() < deadline) {
            // A transport fault can require a standard callback-service call to
            // observe EOF even when the selected model is HLA_IMMEDIATE.
            try {
               lost.evokeMultipleCallbacks(0.0, 0.05);
            } catch (Exception ignored) {
               // The provider may surface the transport fault before queuing
               // the normative connectionLost callback.
            }
            if (lostFaultDescription.get() == null) Thread.sleep(1L);
         }
         JavaTckSupport.check(lostFaultDescription.get() != null,
            "timed out waiting for the standard connectionLost callback");
         JavaTckSupport.check(!lostFaultDescription.get().isEmpty(),
            "connectionLost returned an empty fault description");
         JavaTckSupport.check(survivorFaultDescription.get() == null,
            "the surviving member also received a connectionLost callback");

         survivor.sendInteraction(interactionClass,
            survivor.getParameterHandleValueMapFactory().create(0), new byte[0]);

         try {
            lost.disconnect();
            lostConnected = false;
         } catch (Exception ignored) {
            // Connection-loss cleanup may precede delivery of connectionLost.
         }
         writeCompletionMarker(Path.of(completionMarkerPath));
      } finally {
         if (lostConnected) {
            try { lost.disconnect(); } catch (Exception ignored) { }
         }
         if (survivorJoined) {
            try {
               survivor.resignFederationExecution(ResignAction.NO_ACTION);
               survivorJoined = false;
            } catch (Exception ignored) { }
         }
         if (created) {
            try { survivor.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (survivorConnected) {
            try { survivor.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador callbackProxy(AtomicReference<String> faultDescription) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("connectionLost".equals(method.getName())
                  && arguments != null && arguments.length == 1) {
               faultDescription.set((String) arguments[0]);
            }
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK connection-loss callback";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static void writeCompletionMarker(Path marker) throws Exception {
      Path parent = marker.toAbsolutePath().getParent();
      if (parent != null) Files.createDirectories(parent);
      Files.write(marker, "callback-ok\n".getBytes(StandardCharsets.UTF_8));
   }
}
