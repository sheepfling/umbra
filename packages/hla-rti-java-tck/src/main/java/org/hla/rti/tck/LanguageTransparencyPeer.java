package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.RtiFactoryFactory;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.util.concurrent.LinkedBlockingQueue;

/** Java API peer fixture for the native RTI TCK language-transparency case. */
public final class LanguageTransparencyPeer {
   private static final String FEDERATE_NAME = "java-tck-language-peer";
   private static final String CPP_FEDERATE_NAME = "cpp-tck-language-peer";
   private static final int EXPECTED_MESSAGES = 2;

   private LanguageTransparencyPeer() {
   }

   public static void main(String[] arguments) throws Exception {
      if (arguments.length != 7) {
         throw new IllegalArgumentException(
            "expected rti-address, federation, FOM, time implementation, ready marker, stop marker, and result file");
      }
      String rtiAddress = arguments[0];
      String federation = arguments[1];
      String fom = arguments[2];
      String timeImplementation = arguments[3];
      Path readyMarker = Path.of(arguments[4]);
      Path stopMarker = Path.of(arguments[5]);
      Path resultFile = Path.of(arguments[6]);
      LinkedBlockingQueue<ReceivedInteraction> received = new LinkedBlockingQueue<>();
      RTIambassador rti = null;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      int receivedCount = 0;

      try {
         RtiFactory factory = RtiFactoryFactory.getRtiFactory();
         rti = factory.getRtiAmbassador();
         FederateAmbassador federateAmbassador = callbackProxy(received);
         RtiConfiguration configuration = RtiConfiguration.createConfiguration()
            .withRtiAddress(rtiAddress);
         rti.connect(federateAmbassador, CallbackModel.HLA_EVOKED, configuration);
         connected = true;
         rti.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         rti.joinFederationExecution(FEDERATE_NAME, "language-transparency-java", federation);
         joined = true;

         InteractionClassHandle interaction = rti.getInteractionClassHandle(
            "HLAinteractionRoot.TckInteraction");
         ParameterHandle parameter = rti.getParameterHandle(interaction, "Payload");
         rti.publishInteractionClass(interaction);
         rti.subscribeInteractionClass(interaction);
         Files.createDirectories(readyMarker.toAbsolutePath().getParent());
         Files.writeString(readyMarker, "ready\n", StandardCharsets.UTF_8);
         System.out.println("Java API peer ready in " + federation);

         long deadline = System.nanoTime() + Duration.ofSeconds(45).toNanos();
         while (!Files.exists(stopMarker)) {
            if (System.nanoTime() >= deadline) {
               throw new IllegalStateException("timed out waiting for native TCK shutdown");
            }
            rti.evokeCallback(0.05);
            ReceivedInteraction message = received.poll();
            if (message == null) {
               continue;
            }
            String model = verifyRequest(rti, parameter, message);
            String response = "JAVA-TO-CPP:" + model;
            ParameterHandleValueMap responseParameters =
               rti.getParameterHandleValueMapFactory().create(1);
            responseParameters.put(parameter, response.getBytes(StandardCharsets.UTF_8));
            rti.sendInteraction(
               interaction,
               responseParameters,
               ("java-tag:" + model).getBytes(StandardCharsets.UTF_8));
            receivedCount++;
            if (receivedCount > EXPECTED_MESSAGES) {
               throw new IllegalStateException("received more native TCK requests than expected");
            }
         }
         if (receivedCount != EXPECTED_MESSAGES) {
            throw new IllegalStateException(
               "received " + receivedCount + " native requests; expected " + EXPECTED_MESSAGES);
         }
         Files.writeString(resultFile, "received=" + receivedCount + "\n", StandardCharsets.UTF_8);
      } finally {
         if (joined) {
            rti.resignFederationExecution(ResignAction.NO_ACTION);
         }
         if (created) {
            rti.destroyFederationExecution(federation);
         }
         if (connected) {
            rti.disconnect();
         }
      }
   }

   private static FederateAmbassador callbackProxy(
         LinkedBlockingQueue<ReceivedInteraction> received) {
      InvocationHandler handler = (proxy, method, arguments) -> {
         if (method.getDeclaringClass() == Object.class) {
            return handleObjectMethod(proxy, method, arguments);
         }
         if ("receiveInteraction".equals(method.getName())
               && arguments != null && arguments.length >= 6) {
            ParameterHandleValueMap parameters = (ParameterHandleValueMap) arguments[1];
            byte[] tag = ((byte[]) arguments[2]).clone();
            FederateHandle producer = (FederateHandle) arguments[4];
            byte[] payload = parameters.values().stream()
               .findFirst()
               .orElseThrow(() -> new IllegalStateException("native interaction had no parameter"))
               .clone();
            received.add(new ReceivedInteraction(payload, tag, producer));
         }
         return null;
      };
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         handler);
   }

   private static Object handleObjectMethod(Object proxy, Method method, Object[] arguments) {
      switch (method.getName()) {
         case "toString":
            return "Java language-transparency TCK peer";
         case "hashCode":
            return System.identityHashCode(proxy);
         case "equals":
            return proxy == arguments[0];
         default:
            throw new IllegalStateException("unexpected Object method: " + method.getName());
      }
   }

   private static String verifyRequest(
         RTIambassador rti,
         ParameterHandle parameter,
         ReceivedInteraction message) throws Exception {
      String payload = new String(message.payload, StandardCharsets.UTF_8);
      if (!payload.startsWith("CPP-TO-JAVA:")) {
         throw new IllegalStateException("unexpected native interaction payload: " + payload);
      }
      String model = payload.substring("CPP-TO-JAVA:".length());
      String expectedTag = "cpp-tag:" + model;
      if (!expectedTag.equals(new String(message.tag, StandardCharsets.UTF_8))) {
         throw new IllegalStateException("native interaction tag did not cross the API boundary");
      }
      if (!CPP_FEDERATE_NAME.equals(rti.getFederateName(message.producer))) {
         throw new IllegalStateException("native producer identity did not cross the API boundary");
      }
      if (rti.getFederateHandle(CPP_FEDERATE_NAME) == null
            || parameter == null) {
         throw new IllegalStateException("native federate or parameter handle was unavailable");
      }
      return model;
   }

   private static final class ReceivedInteraction {
      private final byte[] payload;
      private final byte[] tag;
      private final FederateHandle producer;

      private ReceivedInteraction(byte[] payload, byte[] tag, FederateHandle producer) {
         this.payload = payload;
         this.tag = tag;
         this.producer = producer;
      }
   }
}
