package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.InvalidMessageRetractionHandle;
import hla.rti1516_2025.exceptions.MessageCanNoLongerBeRetracted;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Saved terminal timestamped attribute-retraction classification through the public API. */
final class RestoreTerminalTsoAttributeTck {
   private RestoreTerminalTsoAttributeTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "terminal timestamped attribute restore has native evidence only for evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder();
      String federation = "java-tck-restore-terminal-tso-attribute-" + UUID.randomUUID();
      String saveLabel = "tck-terminal-attribute-retraction-baseline";
      byte[] payload = {'T', 'A', '-', 'T'};
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      try {
         owner.connect(callbacks.proxy(), callbackModel);
         connected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("terminal-attribute-publisher", federation);
         joined = true;
         ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
         AttributeHandleSet attributes = owner.getAttributeHandleSetFactory().create();
         attributes.add(attribute);
         owner.publishObjectClassAttributes(objectClass, attributes);
         owner.changeDefaultAttributeOrderType(objectClass, attributes, OrderType.TIMESTAMP);
         ObjectInstanceHandle object = owner.registerObjectInstance(objectClass);
         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         LogicalTime<?, ?> initial = owner.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> sendTime = offset(initial, epsilon, 6);
         owner.enableTimeRegulation(epsilon);
         await(owner, () -> callbacks.regulationEnabled == 1,
            "terminal TSO attribute time-regulation enablement");

         AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
         values.put(attribute, payload);
         MessageRetractionReturn terminal = owner.updateAttributeValues(
            object, values, payload, sendTime);
         JavaTckSupport.check(valid(terminal),
            "terminal TSO attribute update lacked a retraction handle");
         owner.retract(terminal.handle);
         expectTerminal(owner, terminal.handle);

         owner.requestFederationSave(saveLabel);
         await(owner, () -> callbacks.saveLabels.equals(List.of(saveLabel)),
            "terminal TSO attribute save initiation");
         owner.federateSaveBegun();
         owner.federateSaveComplete();
         await(owner, () -> callbacks.savedCount == 1,
            "terminal TSO attribute federation saved");

         MessageRetractionReturn discarded = owner.updateAttributeValues(
            object, values, payload, sendTime);
         JavaTckSupport.check(valid(discarded)
               && !terminal.handle.equals(discarded.handle),
            "post-save TSO attribute update reused the terminal designator");
         owner.requestFederationRestore(saveLabel);
         await(owner, () -> callbacks.restoreRequestSucceeded == 1
               && callbacks.restoreLabels.equals(List.of(saveLabel)),
            "terminal TSO attribute restore initiation");
         owner.federateRestoreComplete();
         await(owner, () -> callbacks.restoredCount == 1,
            "terminal TSO attribute federation restored");
         expectTerminal(owner, terminal.handle);
         expectInvalid(owner, discarded.handle);

         owner.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static boolean valid(MessageRetractionReturn value) {
      return value != null && value.retractionHandleIsValid && value.handle != null;
   }

   private static void expectTerminal(RTIambassador member, MessageRetractionHandle handle)
         throws Exception {
      try { member.retract(handle); }
      catch (MessageCanNoLongerBeRetracted expected) { return; }
      throw new IllegalStateException("saved retraction handle did not remain terminal");
   }

   private static void expectInvalid(RTIambassador member, MessageRetractionHandle handle)
         throws Exception {
      try { member.retract(handle); }
      catch (InvalidMessageRetractionHandle expected) { return; }
      throw new IllegalStateException("discarded post-save retraction handle remained valid");
   }

   private static void await(RTIambassador member, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline)
         member.evokeCallback(0.0);
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static final class Recorder {
      private final List<String> saveLabels = new CopyOnWriteArrayList<>();
      private final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      private volatile int regulationEnabled;
      private volatile int savedCount;
      private volatile int restoreRequestSucceeded;
      private volatile int restoredCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) ++regulationEnabled;
               else if ("initiateFederateSave".equals(name)) saveLabels.add((String) args[0]);
               else if ("federationSaved".equals(name)) ++savedCount;
               else if ("requestFederationRestoreSucceeded".equals(name))
                  ++restoreRequestSucceeded;
               else if ("initiateFederateRestore".equals(name))
                  restoreLabels.add((String) args[0]);
               else if ("federationRestored".equals(name)) ++restoredCount;
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK terminal TSO attribute callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
