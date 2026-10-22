package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies final-member NO_ACTION cleanup releases owned objects and names. */
final class FinalFederateResignationCleanupTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private FinalFederateResignationCleanupTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder();
      String federation = "java-tck-final-resignation-cleanup-" + UUID.randomUUID();
      boolean connected = false, created = false, joined = false;
      boolean attributesPublished = false;
      ObjectClassHandle objectClass = null;
      AttributeHandleSet attributes = null;
      try {
         ambassador.connect(callbacks.proxy(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution("java-tck-final-resignation-first", federation);
         joined = true;
         objectClass = ambassador.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = ambassador.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClassName.equals(ambassador.getObjectClassName(objectClass))
               && attributeName.equals(ambassador.getAttributeName(objectClass, attribute)),
            "adapter FOM object class or attribute name did not round-trip");
         attributes = JavaTckSupport.attributeSet(ambassador, attribute);
         ambassador.publishObjectClassAttributes(objectClass, attributes);
         attributesPublished = true;
         ObjectInstanceHandle original = ambassador.registerObjectInstance(objectClass);
         JavaTckSupport.check(original != null,
            "object registration returned a null instance handle");
         String reusableName = ambassador.getObjectInstanceName(original);
         JavaTckSupport.check(reusableName != null && !reusableName.isEmpty(),
            "object registration returned an empty instance name");

         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         attributesPublished = false;
         ambassador.joinFederationExecution("java-tck-final-resignation-rejoined", federation);
         joined = true;
         objectClass = ambassador.getObjectClassHandle(objectClassName);
         attribute = ambassador.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClassName.equals(ambassador.getObjectClassName(objectClass))
               && attributeName.equals(ambassador.getAttributeName(objectClass, attribute)),
            "FOM handles did not remain queryable after final-member rejoin");
         attributes = JavaTckSupport.attributeSet(ambassador, attribute);
         ambassador.publishObjectClassAttributes(objectClass, attributes);
         attributesPublished = true;

         callbacks.reservations.clear();
         ambassador.reserveObjectInstanceName(reusableName);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(callbacks.reservations.isEmpty(),
               "evoked reservation callback arrived before callback servicing");
         }
         await(ambassador, callbackModel,
            () -> callbacks.reservations.contains(reusableName),
            "name reuse after final-member resignation");
         JavaTckSupport.check(callbacks.reservations.size() == 1,
            "name reuse produced duplicate reservation callbacks");
         ObjectInstanceHandle replacement = ambassador.registerObjectInstance(objectClass,
            reusableName);
         JavaTckSupport.check(replacement != null
               && replacement.equals(ambassador.getObjectInstanceHandle(reusableName))
               && reusableName.equals(ambassador.getObjectInstanceName(replacement)),
            "final-member resignation did not release the former object's name");

         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         attributesPublished = false;
         ambassador.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined && attributesPublished) {
            try { ambassador.unpublishObjectClassAttributes(objectClass, attributes); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) { try { ambassador.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Recorder {
      private final List<String> reservations = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("objectInstanceNameReservationSucceeded".equals(name)) {
                  reservations.add((String) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK final-resignation callbacks";
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
