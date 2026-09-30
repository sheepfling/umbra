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
import java.util.UUID;

/** Checks no-recipient deletion retraction, object-name restoration, and terminal boundaries. */
final class TimestampedObjectDeletionNoFanoutTck {
   private TimestampedObjectDeletionNoFanoutTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-timestamped-deletion-no-fanout-" + UUID.randomUUID();
      boolean connected = false, created = false, joined = false, published = false;
      boolean regulating = false;
      try {
         owner.connect(recorder.proxy(), callbackModel); connected = true;
         owner.createFederationExecution(federation, fom, timeImplementation); created = true;
         owner.joinFederationExecution("java-tck-deletion-no-fanout-owner", federation);
         joined = true;
         ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
         AttributeHandleSet attributes = JavaTckSupport.attributeSet(owner, attribute);
         owner.publishObjectClassAttributes(objectClass, attributes); published = true;

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = intervalOffset(epsilon, 5);
         owner.enableTimeRegulation(lookahead); regulating = true;
         JavaTckSupport.drain(owner);
         JavaTckSupport.check(recorder.timeRegulationEnabled,
            "timeRegulationEnabled callback was not observed");
         LogicalTime initial = owner.queryLogicalTime();

         ObjectInstanceHandle boundaryObject = owner.registerObjectInstance(objectClass);
         byte[] boundaryTag = new byte[] {0x42, 0x44, 0x42};
         LogicalTime boundaryTime = offset(initial, epsilon, 5);
         MessageRetractionReturn boundaryDeletion = owner.deleteObjectInstance(
            boundaryObject, boundaryTag, boundaryTime);
         JavaTckSupport.check(boundaryDeletion != null
               && boundaryDeletion.retractionHandleIsValid && boundaryDeletion.handle != null,
            "lookahead-boundary timestamped deletion returned no valid retraction handle");
         expectException(() -> owner.retract(boundaryDeletion.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting a timestamped deletion at the exact lookahead boundary");

         ObjectInstanceHandle object = owner.registerObjectInstance(objectClass);
         String objectName = owner.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty(),
            "timestamped deletion object registration returned an empty name");
         LogicalTime deletionTime = offset(initial, epsilon, 6);
         byte[] tag = new byte[] {0x4E, 0x46, 0x44};
         MessageRetractionReturn deletion = owner.deleteObjectInstance(object, tag, deletionTime);
         JavaTckSupport.check(deletion != null && deletion.retractionHandleIsValid
               && deletion.handle != null,
            "no-fanout timestamped deletion returned no valid retraction handle");
         expectException(() -> owner.getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown",
            "looking up an instance with an unretracted timestamped deletion");

         owner.retract(deletion.handle);
         JavaTckSupport.check(object.equals(owner.getObjectInstanceHandle(objectName)),
            "retracting a no-fanout deletion did not restore the object-name mapping");
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, attribute),
            "retracting a no-fanout deletion did not restore local attribute ownership");
         JavaTckSupport.check(recorder.removalCallbacks == 0 && recorder.retractionCallbacks == 0,
            "no-fanout deletion unexpectedly delivered a removal or retraction callback");
         expectException(() -> owner.retract(deletion.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting an already retracted timestamped deletion");

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

   private static final class Recorder {
      private boolean timeRegulationEnabled;
      private int removalCallbacks, retractionCallbacks;
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) timeRegulationEnabled = true;
               else if ("removeObjectInstance".equals(name)) ++removalCallbacks;
               else if ("requestRetraction".equals(name)) ++retractionCallbacks;
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK deletion no-fanout callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
