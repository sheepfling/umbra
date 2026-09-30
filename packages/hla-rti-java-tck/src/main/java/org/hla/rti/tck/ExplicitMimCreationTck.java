package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

final class ExplicitMimCreationTck {
   private ExplicitMimCreationTck() {
   }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-explicit-mim-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      boolean created = false;
      try {
         check(owner.connect(callbacks(), callbackModel) != null,
            "MIM owner connect returned null ConfigurationResult");
         ownerConnected = true;
         check(member.connect(callbacks(), callbackModel) != null,
            "MIM member connect returned null ConfigurationResult");
         memberConnected = true;

         JavaTckSupport.expectFailure(() -> owner.createFederationExecutionWithMIM(
               federation, new String[] {fom}, "HLAstandardMIM", timeImplementation),
            "DesignatorIsHLAstandardMIM", "reserved standard-MIM designator");

         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-mim-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-mim-member", federation);
         memberJoined = true;

         ObjectClassHandle ownerFederationClass = owner.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederation");
         ObjectClassHandle memberFederationClass = member.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederation");
         ObjectClassHandle ownerFederateClass = owner.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederate");
         ObjectClassHandle memberFederateClass = member.getObjectClassHandle(
            "HLAobjectRoot.HLAmanager.HLAfederate");
         check(valid(ownerFederationClass) && valid(memberFederationClass)
               && valid(ownerFederateClass) && valid(memberFederateClass),
            "explicit MIM did not compose the standard MOM object classes");
         check(ownerFederationClass.equals(memberFederationClass)
               && ownerFederateClass.equals(memberFederateClass),
            "federates resolved different handles for shared MOM object classes");
         check("HLAobjectRoot.HLAmanager.HLAfederation".equals(
               owner.getObjectClassName(ownerFederationClass))
               && "HLAobjectRoot.HLAmanager.HLAfederate".equals(
                  owner.getObjectClassName(ownerFederateClass)),
            "standard MOM object-class names did not round-trip");

         AttributeHandle ownerFederationName = owner.getAttributeHandle(
            ownerFederationClass, "HLAfederationName");
         AttributeHandle memberFederationName = member.getAttributeHandle(
            memberFederationClass, "HLAfederationName");
         AttributeHandle ownerFederateName = owner.getAttributeHandle(
            ownerFederateClass, "HLAfederateName");
         AttributeHandle memberFederateName = member.getAttributeHandle(
            memberFederateClass, "HLAfederateName");
         check(valid(ownerFederationName) && valid(memberFederationName)
               && valid(ownerFederateName) && valid(memberFederateName),
            "explicit MIM did not compose the standard MOM name attributes");
         check(ownerFederationName.equals(memberFederationName)
               && ownerFederateName.equals(memberFederateName),
            "federates resolved different handles for shared MOM attributes");
         check("HLAfederationName".equals(owner.getAttributeName(
                  ownerFederationClass, ownerFederationName))
               && "HLAfederateName".equals(owner.getAttributeName(
                  ownerFederateClass, ownerFederateName)),
            "standard MOM attribute names did not round-trip");

         ObjectClassHandle ordinaryClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle ordinaryAttribute = owner.getAttributeHandle(
            ordinaryClass, attributeName);
         check(valid(ordinaryClass) && valid(ordinaryAttribute),
            "explicit MIM composition displaced caller FOM object declarations");
         check(valid(owner.getInteractionClassHandle(interactionClassName))
               && valid(owner.getParameterHandle(
                  owner.getInteractionClassHandle(interactionClassName), parameterName)),
            "explicit MIM composition displaced caller FOM interaction declarations");
      } finally {
         if (memberJoined) {
            try {
               member.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (ownerJoined) {
            try {
               owner.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (created) {
            try {
               owner.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (memberConnected) disconnect(member);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java RTI TCK callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == arguments[0];
            }
            return null;
         });
   }

   private static boolean valid(Object handle) {
      return handle != null;
   }

   private static void check(boolean condition, String message) {
      if (!condition) throw new AssertionError(message);
   }

   private static void disconnect(RTIambassador ambassador) {
      try {
         ambassador.disconnect();
      } catch (Exception ignored) {
      }
   }
}
