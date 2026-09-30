package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** Reclaims a terminal timestamped-deletion tombstone and reuses its named instance. */
final class TimestampedObjectDeletionTombstoneTck {
   private TimestampedObjectDeletionTombstoneTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-deletion-tombstone-" + UUID.randomUUID();
      boolean connected = false, created = false, joined = false;
      boolean published = false, regulating = false;
      try {
         owner.connect(recorder.proxy(), callbackModel); connected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         owner.joinFederationExecution("java-tck-deletion-tombstone-owner", federation);
         joined = true;
         ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
         AttributeHandleSet attributes = JavaTckSupport.attributeSet(owner, attribute);
         owner.publishObjectClassAttributes(objectClass, attributes); published = true;

         String objectName = federation + "-terminal-deletion";
         recorder.reservations.clear();
         owner.reserveObjectInstanceName(objectName);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(recorder.reservations.isEmpty(),
               "evoked name-reservation callback arrived before callback servicing");
         }
         await(() -> recorder.reservations.contains(objectName), owner,
            "initial named-object reservation");
         ObjectInstanceHandle original = owner.registerObjectInstance(objectClass, objectName);
         JavaTckSupport.check(original != null,
            "named object registration returned no handle");

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = intervalOffset(epsilon, 5);
         owner.enableTimeRegulation(lookahead); regulating = true;
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(recorder.timeRegulationEnabled,
            "timeRegulationEnabled callback was not observed");
         LogicalTime initial = owner.queryLogicalTime();
         byte[] boundaryTag = new byte[] {0x42, 0x44, 0x42};
         ObjectInstanceHandle boundaryObject = owner.registerObjectInstance(objectClass);
         LogicalTime boundaryTime = offset(initial, epsilon, 5);
         MessageRetractionReturn boundary = owner.deleteObjectInstance(
            boundaryObject, boundaryTag, boundaryTime);
         JavaTckSupport.check(boundary != null && boundary.retractionHandleIsValid
               && boundary.handle != null,
            "lookahead-boundary deletion returned no valid retraction handle");
         expectException(() -> owner.retract(boundary.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting a timestamped deletion at the exact lookahead boundary");

         LogicalTime deletionTime = offset(initial, epsilon, 6);
         LogicalTime advanceTarget = offset(initial, epsilon, 1);
         byte[] tag = new byte[] {0x54, 0x4F, 0x4D};
         MessageRetractionReturn deletion = owner.deleteObjectInstance(original, tag, deletionTime);
         JavaTckSupport.check(deletion != null && deletion.retractionHandleIsValid
               && deletion.handle != null,
            "timestamped object deletion returned no valid retraction handle");
         owner.timeAdvanceRequest(advanceTarget);
         await(() -> !recorder.timeAdvanceGrantTimes.isEmpty(), owner,
            "deletion-tombstone terminalizing time advance");
         JavaTckSupport.check(advanceTarget.equals(recorder.timeAdvanceGrantTimes.get(0)),
            "terminalizing time advance granted the wrong logical time");
         expectException(() -> owner.retract(deletion.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting a terminal timestamped object-deletion tombstone");

         recorder.reservations.clear();
         owner.reserveObjectInstanceName(objectName);
         await(() -> recorder.reservations.contains(objectName), owner,
            "reuse of the terminally deleted instance name");
         ObjectInstanceHandle replacement = owner.registerObjectInstance(objectClass, objectName);
         JavaTckSupport.check(replacement != null
               && replacement.equals(owner.getObjectInstanceHandle(objectName)),
            "terminal tombstone did not permit named re-registration and handle lookup");

         owner.disableTimeRegulation(); regulating = false;
         owner.unpublishObjectClassAttributes(objectClass, attributes); published = false;
      } finally {
         if (regulating && joined) try { owner.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (published && joined) try {
            ObjectClassHandle cls = owner.getObjectClassHandle(objectClassName);
            owner.unpublishObjectClassAttributes(cls, JavaTckSupport.attributeSet(owner,
               owner.getAttributeHandle(cls, attributeName)));
         } catch (Exception ignored) { }
         if (joined) try {
            owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (connected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void expectException(JavaTckSupport.CheckedOperation operation,
         String expected, String description) throws Exception {
      try {
         operation.run();
      } catch (Exception exception) {
         for (Class<?> type = exception.getClass(); type != null; type = type.getSuperclass()) {
            if (expected.equals(type.getSimpleName())) return;
         }
         throw new AssertionError(description + " threw " + exception.getClass().getName()
            + " instead of " + expected, exception);
      }
      throw new AssertionError(description + " did not throw " + expected);
   }

   @FunctionalInterface
   private interface ReadyCheck {
      boolean isReady();
   }

   private static void await(ReadyCheck ready, RTIambassador owner, String description)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         JavaTckSupport.drain(owner);
         if (ready.isReady()) return;
         Thread.sleep(1L);
      }
      throw new AssertionError(description + " timed out");
   }

   private static final class Recorder {
      private boolean timeRegulationEnabled;
      private final List<String> reservations = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("objectInstanceNameReservationSucceeded".equals(name)
                     && arguments != null && arguments.length == 1) {
                  reservations.add((String) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK deletion tombstone callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
