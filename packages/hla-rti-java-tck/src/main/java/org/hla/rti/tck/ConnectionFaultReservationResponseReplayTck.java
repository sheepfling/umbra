package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Replays an accepted object-name reservation result and observes its public callback. */
final class ConnectionFaultReservationResponseReplayTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long CALLBACK_SETTLE_NANOS = 20_000_000L;

   private ConnectionFaultReservationResponseReplayTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel, RtiConfiguration ownerConfiguration) throws Exception {
      Federate owner = new Federate(factory.getRtiAmbassador());
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-reservation-response-replay-" + suffix;
      String federateName = "java-tck-reservation-owner-" + suffix;
      String objectName = federation + "-reserved-object";
      boolean created = false;
      try {
         owner.connect(callbackModel, ownerConfiguration);
         owner.rti.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.join(federateName, federation);

         ObjectClassHandle objectClass = owner.rti.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = owner.rti.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClass != null && objectClass.encodedLength() > 0
               && attribute != null && attribute.encodedLength() > 0,
            "ReserveObjectInstanceName response replay could not resolve its published class and attribute");
         owner.rti.publishObjectClassAttributes(
            objectClass, JavaTckSupport.attributeSet(owner.rti, attribute));

         owner.rti.reserveObjectInstanceName(objectName);
         owner.awaitReservation(objectName, objectClassName, callbackModel);
         owner.settleCallbacks(objectClassName, callbackModel);
         owner.checkSingleReservationSuccess(objectName,
            "reservation response replay delivered other than one success callback");

         ObjectInstanceHandle object = owner.rti.registerObjectInstance(objectClass, objectName);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "Named registration after reservation response replay returned an invalid handle");
         JavaTckSupport.check(objectName.equals(owner.rti.getObjectInstanceName(object))
               && object.equals(owner.rti.getObjectInstanceHandle(objectName)),
            "Named registration did not consume the replayed reservation with a stable object identity");
         owner.settleCallbacks(objectClassName, callbackModel);
         owner.checkSingleReservationSuccess(objectName,
            "reservation response replay produced a duplicate success callback");

         owner.resign(ResignAction.DELETE_OBJECTS_THEN_DIVEST);
         owner.rti.destroyFederationExecution(federation);
         created = false;
      } finally {
         owner.resignQuietly(ResignAction.DELETE_OBJECTS_THEN_DIVEST);
         if (created) {
            try { owner.rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         owner.disconnectQuietly();
      }
   }

   private static FederateAmbassador callbacks(Federate federate) {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if ("objectInstanceNameReservationSucceeded".equals(name)
                  && arguments != null && arguments.length > 0) {
               federate.reservationSucceeded.add((String) arguments[0]);
            } else if ("objectInstanceNameReservationFailed".equals(name)
                  && arguments != null && arguments.length > 0) {
               federate.reservationFailed.add((String) arguments[0]);
            }
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) return "Java TCK reservation-replay callbacks";
               if ("hashCode".equals(name)) return System.identityHashCode(proxy);
               if ("equals".equals(name)) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static final class Federate {
      private final RTIambassador rti;
      private final CopyOnWriteArrayList<String> reservationSucceeded =
         new CopyOnWriteArrayList<>();
      private final CopyOnWriteArrayList<String> reservationFailed =
         new CopyOnWriteArrayList<>();
      private boolean connected;
      private boolean joined;

      private Federate(RTIambassador rti) { this.rti = rti; }

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
            serviceCallbacks(objectClassName, callbackModel);
         }
         JavaTckSupport.check(!reservationFailed.contains(objectName),
            "objectInstanceNameReservationFailed arrived after reservation response replay");
         JavaTckSupport.check(reservationSucceeded.contains(objectName),
            "timed out waiting for objectInstanceNameReservationSucceeded after response replay");
      }

      private void settleCallbacks(String objectClassName, CallbackModel callbackModel)
            throws Exception {
         long deadline = System.nanoTime() + CALLBACK_SETTLE_NANOS;
         while (System.nanoTime() < deadline) {
            serviceCallbacks(objectClassName, callbackModel);
         }
      }

      private void serviceCallbacks(String objectClassName, CallbackModel callbackModel)
            throws Exception {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            rti.evokeMultipleCallbacks(0.0, 0.001);
         } else {
            rti.getObjectClassHandle(objectClassName);
            Thread.sleep(1L);
         }
      }

      private void checkSingleReservationSuccess(String objectName, String message) {
         JavaTckSupport.check(!reservationFailed.contains(objectName),
            "object-name reservation reported failure after successful response replay");
         JavaTckSupport.check(reservationSuccessCount(objectName) == 1L, message);
      }

      private long reservationSuccessCount(String objectName) {
         return reservationSucceeded.stream().filter(objectName::equals).count();
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
