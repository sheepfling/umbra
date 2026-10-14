package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.ServiceGroup;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks normalization call-state boundaries using standard HLA handles. */
final class HandleNormalizationMembershipBoundariesTck {
   private HandleNormalizationMembershipBoundariesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      RTIambassador probe = factory.getRtiAmbassador();
      String federation = "java-tck-normalize-membership-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean probeConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      boolean probeJoined = false;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerFederate =
            owner.joinFederationExecution("java-tck-normalize-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-normalize-member", federation);
         memberJoined = true;

         ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
         InteractionClassHandle interactionClass =
            owner.getInteractionClassHandle(interactionClassName);
         owner.publishObjectClassAttributes(objectClass,
            JavaTckSupport.attributeSet(owner, attribute));
         ObjectInstanceHandle object = owner.registerObjectInstance(objectClass);
         JavaTckSupport.check(object != null,
            "normalization boundary fixture returned a null object-instance handle");

         expectNormalizationState(probe, ownerFederate, objectClass, interactionClass, object,
            "NotConnected", "before connect");
         probe.connect(callbacks(), callbackModel);
         probeConnected = true;
         expectNormalizationState(probe, ownerFederate, objectClass, interactionClass, object,
            "FederateNotExecutionMember", "after connect before join");

         probe.joinFederationExecution("java-tck-normalize-probe", federation);
         probeJoined = true;
         probe.resignFederationExecution(ResignAction.NO_ACTION);
         probeJoined = false;
         expectNormalizationState(probe, ownerFederate, objectClass, interactionClass, object,
            "FederateNotExecutionMember", "after resignation while connected");
         probe.disconnect();
         probeConnected = false;

         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (probeJoined) {
            try { probe.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (probeConnected) { try { probe.disconnect(); } catch (Exception ignored) { } }
         if (memberConnected) { try { member.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void expectNormalizationState(RTIambassador ambassador,
         FederateHandle federate, ObjectClassHandle objectClass,
         InteractionClassHandle interactionClass, ObjectInstanceHandle object,
         String exception, String phase) throws Exception {
      JavaTckSupport.expectFailure(
         () -> ambassador.normalizeServiceGroup(ServiceGroup.SUPPORT_SERVICES), exception,
         "normalizing the support service group " + phase);
      JavaTckSupport.expectFailure(
         () -> ambassador.normalizeFederateHandle(federate), exception,
         "normalizing a federate handle " + phase);
      JavaTckSupport.expectFailure(
         () -> ambassador.normalizeObjectClassHandle(objectClass), exception,
         "normalizing an object-class handle " + phase);
      JavaTckSupport.expectFailure(
         () -> ambassador.normalizeInteractionClassHandle(interactionClass), exception,
         "normalizing an interaction-class handle " + phase);
      JavaTckSupport.expectFailure(
         () -> ambassador.normalizeObjectInstanceHandle(object), exception,
         "normalizing an object-instance handle " + phase);
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK handle-normalization callback sink";
               }
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
