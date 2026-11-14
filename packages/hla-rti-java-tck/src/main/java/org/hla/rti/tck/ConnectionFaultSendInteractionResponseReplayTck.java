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
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Replays an accepted SendInteraction response after an adapter-managed fault. */
final class ConnectionFaultSendInteractionResponseReplayTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long CALLBACK_SETTLE_NANOS = 20_000_000L;

   private ConnectionFaultSendInteractionResponseReplayTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, String interactionName, String parameterName,
         CallbackModel callbackModel, RtiConfiguration ownerConfiguration,
         RtiConfiguration memberConfiguration) throws Exception {
      Federate owner = new Federate(factory.getRtiAmbassador());
      Federate member = new Federate(factory.getRtiAmbassador());
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-send-interaction-response-replay-" + suffix;
      String ownerName = "java-tck-send-interaction-owner-" + suffix;
      String memberName = "java-tck-send-interaction-member-" + suffix;
      boolean created = false;
      try {
         owner.connect(callbackModel, ownerConfiguration);
         owner.rti.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.join(ownerName, federation);

         member.connect(callbackModel, memberConfiguration);
         member.join(memberName, federation);

         InteractionClassHandle ownerInteraction =
            owner.rti.getInteractionClassHandle(interactionName);
         ParameterHandle ownerParameter =
            owner.rti.getParameterHandle(ownerInteraction, parameterName);
         InteractionClassHandle memberInteraction =
            member.rti.getInteractionClassHandle(interactionName);
         ParameterHandle memberParameter =
            member.rti.getParameterHandle(memberInteraction, parameterName);
         JavaTckSupport.check(ownerInteraction != null && ownerInteraction.encodedLength() > 0
               && ownerParameter != null && ownerParameter.encodedLength() > 0
               && memberInteraction != null && memberInteraction.encodedLength() > 0
               && memberParameter != null && memberParameter.encodedLength() > 0,
            "SendInteraction response replay could not resolve the interaction and parameter");
         owner.rti.publishInteractionClass(ownerInteraction);
         member.rti.subscribeInteractionClass(memberInteraction);

         byte[] firstValue = new byte[] {0x61, 0x62, 0x63};
         byte[] firstTag = new byte[] {(byte) 0xC1, (byte) 0xC2};
         owner.send(ownerInteraction, ownerParameter, firstValue, firstTag);

         byte[] fenceValue = new byte[] {0x71, 0x72};
         byte[] fenceTag = new byte[] {(byte) 0xD1};
         owner.send(ownerInteraction, ownerParameter, fenceValue, fenceTag);

         member.awaitInteractions(interactionName, callbackModel);
         member.settleCallbacks(interactionName, callbackModel);
         List<Interaction> interactions = member.interactions();
         JavaTckSupport.check(interactions.size() == 2,
            "SendInteraction response replay lost or duplicated a received interaction");
         JavaTckSupport.check(matches(interactions.get(0), memberInteraction, memberParameter,
               firstValue, firstTag, ownerHandle)
               && matches(interactions.get(1), memberInteraction, memberParameter,
                  fenceValue, fenceTag, ownerHandle),
            "SendInteraction response replay lost, reordered, or changed a received interaction");

         member.resign(ResignAction.NO_ACTION);
         owner.resign(ResignAction.NO_ACTION);
         owner.rti.destroyFederationExecution(federation);
         created = false;
      } finally {
         member.resignQuietly(ResignAction.NO_ACTION);
         owner.resignQuietly(ResignAction.NO_ACTION);
         if (created) {
            try { owner.rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         member.disconnectQuietly();
         owner.disconnectQuietly();
      }
   }

   private static boolean matches(Interaction interaction,
         InteractionClassHandle expectedInteraction, ParameterHandle expectedParameter,
         byte[] expectedValue, byte[] expectedTag, FederateHandle expectedProducer) {
      return !interaction.timestampOrdered
         && expectedInteraction.equals(interaction.interaction)
         && interaction.parameters.size() == 1
         && interaction.parameters.containsKey(expectedParameter)
         && Arrays.equals(expectedValue, interaction.parameters.get(expectedParameter))
         && Arrays.equals(expectedTag, interaction.tag)
         && expectedProducer.equals(interaction.producer);
   }

   private static FederateAmbassador callbacks(Federate federate) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if ("receiveInteraction".equals(name)
                  && arguments != null && arguments.length >= 6) {
               federate.interactions.add(new Interaction(
                  (InteractionClassHandle) arguments[0],
                  (ParameterHandleValueMap) arguments[1],
                  (byte[]) arguments[2],
                  (FederateHandle) arguments[4],
                  arguments.length != 6));
            }
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) return "Java TCK send-interaction replay callbacks";
               if ("hashCode".equals(name)) return System.identityHashCode(proxy);
               if ("equals".equals(name)) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;
      private final boolean timestampOrdered;

      private Interaction(InteractionClassHandle interaction,
            ParameterHandleValueMap parameters, byte[] tag, FederateHandle producer,
            boolean timestampOrdered) {
         this.interaction = interaction;
         this.parameters = parameters.clone();
         this.tag = tag == null ? null : tag.clone();
         this.producer = producer;
         this.timestampOrdered = timestampOrdered;
      }
   }

   private static final class Federate {
      private final RTIambassador rti;
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private boolean connected;
      private boolean joined;

      private Federate(RTIambassador rti) { this.rti = rti; }

      private void connect(CallbackModel callbackModel,
            RtiConfiguration configuration) throws Exception {
         rti.connect(callbacks(this), callbackModel, configuration);
         connected = true;
      }

      private FederateHandle join(String name, String federation) throws Exception {
         FederateHandle handle = rti.joinFederationExecution(
            name, "java-tck-federate", federation);
         joined = true;
         return handle;
      }

      private void send(InteractionClassHandle interaction, ParameterHandle parameter,
            byte[] value, byte[] tag) throws Exception {
         ParameterHandleValueMap parameters = rti.getParameterHandleValueMapFactory().create(1);
         parameters.put(parameter, value.clone());
         rti.sendInteraction(interaction, parameters, tag.clone());
      }

      private void awaitInteractions(String interactionName, CallbackModel callbackModel)
            throws Exception {
         long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
         while (System.nanoTime() < deadline && interactions.size() < 2) {
            serviceCallbacks(interactionName, callbackModel);
         }
         JavaTckSupport.check(interactions.size() >= 2,
            "member timed out waiting for replay and callback-fence interactions");
      }

      private void settleCallbacks(String interactionName, CallbackModel callbackModel)
            throws Exception {
         long deadline = System.nanoTime() + CALLBACK_SETTLE_NANOS;
         while (System.nanoTime() < deadline) {
            serviceCallbacks(interactionName, callbackModel);
         }
      }

      private void serviceCallbacks(String interactionName, CallbackModel callbackModel)
            throws Exception {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            rti.evokeMultipleCallbacks(0.0, 0.001);
         } else {
            rti.getInteractionClassHandle(interactionName);
            Thread.sleep(1L);
         }
      }

      private List<Interaction> interactions() { return List.copyOf(interactions); }

      private void resign(ResignAction action) throws Exception {
         if (joined) {
            rti.resignFederationExecution(action);
            joined = false;
         }
      }

      private void resignQuietly(ResignAction action) {
         try { resign(action); }
         catch (Exception ignored) { joined = false; }
      }

      private void disconnectQuietly() {
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
            connected = false;
         }
      }
   }
}
