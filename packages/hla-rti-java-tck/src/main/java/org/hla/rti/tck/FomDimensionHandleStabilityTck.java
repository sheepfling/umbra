package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.AttributeHandle;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks FOM dimension identity and class associations across module composition, without regions. */
final class FomDimensionHandleStabilityTck {
   private FomDimensionHandleStabilityTck() { }

   static void run(RtiFactory factory, String fom, String additionalFom,
         String baseDimensionName, String baseObjectClassName, String baseInteractionClassName,
         String additionalDimensionName, String additionalObjectClassName,
         String additionalAttributeName, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador extension = factory.getRtiAmbassador();
      String federation = "java-tck-fom-dimension-stability-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean extensionConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean extensionJoined = false;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         extension.connect(callbacks(), callbackModel);
         extensionConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-fom-dimension-owner", federation);
         ownerJoined = true;

         DimensionHandle baseDimension = owner.getDimensionHandle(baseDimensionName);
         JavaTckSupport.check(baseDimension != null
               && baseDimensionName.equals(owner.getDimensionName(baseDimension))
               && owner.getDimensionUpperBound(baseDimension) > 0,
            "adapter FOM did not expose a valid base dimension lookup");
         ObjectClassHandle baseObjectClass = owner.getObjectClassHandle(baseObjectClassName);
         InteractionClassHandle baseInteractionClass =
            owner.getInteractionClassHandle(baseInteractionClassName);
         JavaTckSupport.check(baseObjectClass != null && baseInteractionClass != null,
            "adapter FOM omitted the base classes used for dimension association");
         DimensionHandleSet baseObjectDimensions =
            owner.getAvailableDimensionsForObjectClass(baseObjectClass);
         DimensionHandleSet baseInteractionDimensions =
            owner.getAvailableDimensionsForInteractionClass(baseInteractionClass);
         JavaTckSupport.check(baseObjectDimensions.contains(baseDimension)
               && baseInteractionDimensions.contains(baseDimension),
            "adapter FOM did not retain the base dimension associations");
         JavaTckSupport.expectFailure(() -> owner.getDimensionHandle(additionalDimensionName),
            "NameNotFound", "looking up the extension dimension before its module is joined");
         JavaTckSupport.expectFailure(
            () -> owner.getObjectClassHandle(additionalObjectClassName), "NameNotFound",
            "looking up the dimensional extension class before its module is joined");

         extension.joinFederationExecution("java-tck-fom-dimension-extension",
            "java-tck-fom-dimension-extension", federation, new String[] {additionalFom});
         extensionJoined = true;

         DimensionHandle ownerBaseAfter = owner.getDimensionHandle(baseDimensionName);
         DimensionHandle extensionBaseAfter = extension.getDimensionHandle(baseDimensionName);
         JavaTckSupport.check(baseDimension.equals(ownerBaseAfter)
               && baseDimension.equals(extensionBaseAfter)
               && baseDimensionName.equals(owner.getDimensionName(ownerBaseAfter))
               && baseDimensionName.equals(extension.getDimensionName(extensionBaseAfter)),
            "additional-module join renumbered or split the existing base dimension");

         DimensionHandle ownerAdditional = owner.getDimensionHandle(additionalDimensionName);
         DimensionHandle extensionAdditional =
            extension.getDimensionHandle(additionalDimensionName);
         long additionalUpperBound = owner.getDimensionUpperBound(ownerAdditional);
         JavaTckSupport.check(ownerAdditional.equals(extensionAdditional)
               && !ownerAdditional.equals(baseDimension)
               && additionalDimensionName.equals(owner.getDimensionName(ownerAdditional))
               && additionalDimensionName.equals(extension.getDimensionName(extensionAdditional))
               && additionalUpperBound > 0
               && additionalUpperBound == extension.getDimensionUpperBound(extensionAdditional),
            "additional dimension did not resolve to a shared distinct handle and stable metadata");

         ObjectClassHandle ownerAdditionalClass =
            owner.getObjectClassHandle(additionalObjectClassName);
         ObjectClassHandle extensionAdditionalClass =
            extension.getObjectClassHandle(additionalObjectClassName);
         AttributeHandle ownerAdditionalAttribute = owner.getAttributeHandle(
            ownerAdditionalClass, additionalAttributeName);
         AttributeHandle extensionAdditionalAttribute = extension.getAttributeHandle(
            extensionAdditionalClass, additionalAttributeName);
         DimensionHandleSet ownerAdditionalDimensions =
            owner.getAvailableDimensionsForObjectClass(ownerAdditionalClass);
         DimensionHandleSet extensionAdditionalDimensions =
            extension.getAvailableDimensionsForObjectClass(extensionAdditionalClass);
         JavaTckSupport.check(ownerAdditionalClass.equals(extensionAdditionalClass)
               && ownerAdditionalAttribute.equals(extensionAdditionalAttribute)
               && ownerAdditionalDimensions.size() == 1
               && ownerAdditionalDimensions.contains(ownerAdditional)
               && extensionAdditionalDimensions.equals(ownerAdditionalDimensions),
            "extension dimensional class/attribute or its sole dimension association differed");
      } finally {
         if (extensionJoined) resign(extension);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (extensionConnected) disconnect(extension);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("toString".equals(method.getName())) return "Java TCK FOM dimension callbacks";
            if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
            if ("equals".equals(method.getName())) {
               return proxy == (arguments == null ? null : arguments[0]);
            }
            return null;
         });
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }
}
