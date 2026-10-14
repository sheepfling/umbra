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

/** Checks update-rate lookups with valid FOM inputs become membership-gated after resignation. */
final class UpdateRateLookupAfterResignationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private UpdateRateLookupAfterResignationTck() { }

   static void run(RtiFactory factory, String rateFom, String timeImplementation,
         String objectClassName, String attributeName, String updateRateDesignator,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder memberCallbacks = new Recorder();
      String federation = "java-tck-update-rate-resignation-" + UUID.randomUUID();
      boolean[] connected = new boolean[2], joined = new boolean[2];
      boolean created = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected[0] = true;
         member.connect(memberCallbacks.proxy(), callbackModel);
         connected[1] = true;
         owner.createFederationExecution(federation, rateFom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-update-rate-owner", federation);
         joined[0] = true;
         member.joinFederationExecution("java-tck-update-rate-member", federation);
         joined[1] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle memberClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         JavaTckSupport.check(objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(member.getObjectClassName(memberClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute))
               && attributeName.equals(member.getAttributeName(memberClass, memberAttribute)),
            "adapter rate FOM class or attribute lookup did not round-trip");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet memberAttributes = JavaTckSupport.attributeSet(member, memberAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         member.subscribeObjectClassAttributes(memberClass, memberAttributes,
            updateRateDesignator);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         await(member, callbackModel, () -> memberCallbacks.discoveries.contains(object),
            "update-rate fixture object discovery");

         double namedRate = member.getUpdateRateValue(updateRateDesignator);
         double attributeRate = member.getUpdateRateValueForAttribute(object, memberAttribute);
         JavaTckSupport.check(namedRate > 0.0 && attributeRate == namedRate,
            "valid adapter update-rate designator and object attribute returned inconsistent rates");

         member.resignFederationExecution(ResignAction.NO_ACTION);
         joined[1] = false;
         JavaTckSupport.expectFailure(() -> member.getUpdateRateValue(updateRateDesignator),
            "FederateNotExecutionMember",
            "looking up a valid update-rate designator after resignation");
         JavaTckSupport.expectFailure(
            () -> member.getUpdateRateValueForAttribute(object, memberAttribute),
            "FederateNotExecutionMember",
            "looking up a valid object-attribute update rate after resignation");

         owner.resignFederationExecution(ResignAction.NO_ACTION);
         joined[0] = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[1]) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (joined[0]) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected[1]) { try { member.disconnect(); } catch (Exception ignored) { } }
         if (connected[0]) { try { owner.disconnect(); } catch (Exception ignored) { } }
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
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK update-rate callbacks";
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
