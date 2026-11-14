package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks inherited FDD dimension associations on interaction classes. */
final class FomInteractionInheritedAvailableDimensionsTck {
   private static final String BASE = "HLAinteractionRoot.TckDimensionBaseInteraction";
   private static final String DERIVED =
      "HLAinteractionRoot.TckDimensionBaseInteraction.TckDimensionDerivedInteraction";

   private FomInteractionInheritedAvailableDimensionsTck() { }

   static void runFromAdapter() throws Exception {
      run(RtiTckMain.factory(), RtiTckMain.requireAdapterFile("hla.rti.tck.ddmFom"),
         RtiTckMain.callbackModel());
   }

   static void run(RtiFactory factory, String fom, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-inherited-interaction-dimensions-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean joined = false;
      boolean created = false;
      try {
         JavaTckSupport.check(fom != null && !fom.isEmpty(),
            "adapter-supplied portable DDM FOM is required");
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         member.joinFederationExecution("inherited-interaction-dimensions-member", federation);
         joined = true;

         DimensionHandle x = member.getDimensionHandle("TckDimensionX");
         DimensionHandle y = member.getDimensionHandle("TckDimensionY");
         InteractionClassHandle base = member.getInteractionClassHandle(BASE);
         InteractionClassHandle derived = member.getInteractionClassHandle(DERIVED);
         JavaTckSupport.check(x != null && y != null && base != null && derived != null,
            "inherited interaction dimension setup returned an invalid handle");

         DimensionHandleSet baseDimensions =
            member.getAvailableDimensionsForInteractionClass(base);
         JavaTckSupport.check(baseDimensions.size() == 1 && baseDimensions.contains(x),
            "base interaction class did not expose exactly its FDD dimension X");
         DimensionHandleSet derivedDimensions =
            member.getAvailableDimensionsForInteractionClass(derived);
         JavaTckSupport.check(derivedDimensions.size() == 2
               && derivedDimensions.contains(x) && derivedDimensions.contains(y),
            "derived interaction class did not inherit X and add FDD dimension Y");

         member.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (memberConnected) try { member.disconnect(); }
            catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK inherited interaction dimensions callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
