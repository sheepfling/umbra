package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Queries an attribute before and after standard unconditional divestiture. */
final class OwnershipTransitionQueryTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private OwnershipTransitionQueryTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder();
      String federation = "java-tck-ownership-transition-query-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         owner.connect(callbacks.proxy(), callbackModel);
         connected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "java-tck-ownership-transition-owner", federation);
         joined = true;

         ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClass != null && attribute != null,
            "ownership-transition fixture did not resolve the configured class and attribute");
         AttributeHandleSet attributes = JavaTckSupport.attributeSet(owner, attribute);
         owner.publishObjectClassAttributes(objectClass, attributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(objectClass);
         JavaTckSupport.check(object != null
               && owner.isAttributeOwnedByFederate(object, attribute),
            "registrant did not initially own the published attribute");

         owner.queryAttributeOwnership(object, attributes);
         await(owner, callbackModel, () -> !callbacks.information.isEmpty(),
            "initial owner report");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(owner);
         JavaTckSupport.check(callbacks.information.size() == 1
               && callbacks.notOwned.isEmpty(),
            "initial ownership query did not report one exclusive owner");
         OwnerReport initial = callbacks.information.get(0);
         JavaTckSupport.check(object.equals(initial.object)
               && attributes.equals(initial.attributes)
               && ownerHandle.equals(initial.owner),
            "initial ownership query identified the wrong owner or attribute");

         owner.unconditionalAttributeOwnershipDivestiture(
            object, attributes, new byte[] {0x54, (byte) 0xA7});
         callbacks.information.clear();
         callbacks.notOwned.clear();
         owner.queryAttributeOwnership(object, attributes);
         await(owner, callbackModel, () -> !callbacks.notOwned.isEmpty(),
            "post-divestiture unowned report");
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(owner);
         JavaTckSupport.check(callbacks.notOwned.size() == 1
               && callbacks.information.isEmpty(),
            "post-divestiture query did not report the attribute as unowned exclusively");
         UnownedReport after = callbacks.notOwned.get(0);
         JavaTckSupport.check(object.equals(after.object)
               && attributes.equals(after.attributes)
               && !owner.isAttributeOwnedByFederate(object, attribute),
            "post-divestiture query or ownership predicate retained the old owner");
      } finally {
         if (joined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for " + description);
   }

   private static final class OwnerReport {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final FederateHandle owner;

      private OwnerReport(ObjectInstanceHandle object, AttributeHandleSet attributes,
            FederateHandle owner) {
         this.object = object;
         this.attributes = attributes;
         this.owner = owner;
      }
   }

   private static final class UnownedReport {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;

      private UnownedReport(ObjectInstanceHandle object, AttributeHandleSet attributes) {
         this.object = object;
         this.attributes = attributes;
      }
   }

   private static final class Recorder {
      private final List<OwnerReport> information = new CopyOnWriteArrayList<>();
      private final List<UnownedReport> notOwned = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("informAttributeOwnership".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  information.add(new OwnerReport((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], (FederateHandle) arguments[2]));
               } else if ("attributeIsNotOwned".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  notOwned.add(new UnownedReport((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK ownership-transition callbacks";
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
