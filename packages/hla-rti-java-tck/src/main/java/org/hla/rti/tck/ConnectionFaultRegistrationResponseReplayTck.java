package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Replays an accepted named registration result after an adapter-managed response fault. */
final class ConnectionFaultRegistrationResponseReplayTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private ConnectionFaultRegistrationResponseReplayTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel, RtiConfiguration ownerConfiguration,
         RtiConfiguration recoveringConfiguration) throws Exception {
      Member owner = new Member(factory.getRtiAmbassador());
      Member recovering = new Member(factory.getRtiAmbassador());
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-registration-response-replay-" + suffix;
      String ownerName = "java-tck-registration-owner-" + suffix;
      String recoveringName = "java-tck-registration-member-" + suffix;
      String objectName = federation + "-registered-object";
      boolean created = false;
      try {
         owner.connect(callbackModel, ownerConfiguration);
         owner.rti.createFederationExecutionWithMIM(federation,
            new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.join(ownerName, federation);

         recovering.connect(callbackModel, recoveringConfiguration);
         recovering.join(recoveringName, federation);

         var objectClass = recovering.rti.getObjectClassHandle(objectClassName);
         var attribute = recovering.rti.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClass != null && objectClass.encodedLength() > 0
               && attribute != null && attribute.encodedLength() > 0,
            "registration response replay could not resolve its published class and attribute");
         recovering.rti.publishObjectClassAttributes(
            objectClass, JavaTckSupport.attributeSet(recovering.rti, attribute));

         recovering.rti.reserveObjectInstanceName(objectName);
         recovering.awaitReservation(objectName, objectClassName, callbackModel);

         var recoveredObject = recovering.rti.registerObjectInstance(objectClass, objectName);
         JavaTckSupport.check(recoveredObject != null && recoveredObject.encodedLength() > 0,
            "RegisterObjectInstance response replay returned an invalid object handle");
         JavaTckSupport.check(objectName.equals(
               recovering.rti.getObjectInstanceName(recoveredObject)),
            "RegisterObjectInstance response replay returned a handle for another object name");
         JavaTckSupport.check(recoveredObject.equals(
               recovering.rti.getObjectInstanceHandle(objectName)),
            "RegisterObjectInstance response replay did not preserve the registered object's stable handle");

         recovering.resign(ResignAction.DELETE_OBJECTS_THEN_DIVEST);
         owner.resign(ResignAction.NO_ACTION);
         owner.rti.destroyFederationExecution(federation);
         created = false;
      } finally {
         recovering.resignQuietly(ResignAction.DELETE_OBJECTS_THEN_DIVEST);
         owner.resignQuietly(ResignAction.NO_ACTION);
         if (created) {
            try { owner.rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         recovering.disconnectQuietly();
         owner.disconnectQuietly();
      }
   }

   private static FederateAmbassador callbacks(Member member) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if ("objectInstanceNameReservationSucceeded".equals(name)
                  && arguments != null && arguments.length > 0) {
               member.reservationSucceeded.add((String) arguments[0]);
            } else if ("objectInstanceNameReservationFailed".equals(name)
                  && arguments != null && arguments.length > 0) {
               member.reservationFailed.add((String) arguments[0]);
            }
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) return "Java TCK registration-replay callbacks";
               if ("hashCode".equals(name)) return System.identityHashCode(proxy);
               if ("equals".equals(name)) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static final class Member {
      private final RTIambassador rti;
      private final CopyOnWriteArrayList<String> reservationSucceeded =
         new CopyOnWriteArrayList<>();
      private final CopyOnWriteArrayList<String> reservationFailed =
         new CopyOnWriteArrayList<>();
      private boolean connected;
      private boolean joined;

      private Member(RTIambassador rti) { this.rti = rti; }

      private void connect(CallbackModel callbackModel,
            RtiConfiguration configuration) throws Exception {
         rti.connect(callbacks(this), callbackModel, configuration);
         connected = true;
      }

      private void join(String name, String federation) throws Exception {
         rti.joinFederationExecution(name, "java-tck-federate", federation);
         joined = true;
      }

      private void awaitReservation(String objectName, String objectClassName,
            CallbackModel callbackModel) throws Exception {
         long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
         while (System.nanoTime() < deadline
               && !reservationSucceeded.contains(objectName)
               && !reservationFailed.contains(objectName)) {
            if (callbackModel == CallbackModel.HLA_EVOKED) {
               rti.evokeMultipleCallbacks(0.0, 1.0);
            } else {
               rti.getObjectClassHandle(objectClassName);
            }
            Thread.yield();
         }
         JavaTckSupport.check(!reservationFailed.contains(objectName),
            "object-name reservation failed before RegisterObjectInstance response replay");
         JavaTckSupport.check(reservationSucceeded.contains(objectName),
            "timed out waiting for objectInstanceNameReservationSucceeded before registration");
      }

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
