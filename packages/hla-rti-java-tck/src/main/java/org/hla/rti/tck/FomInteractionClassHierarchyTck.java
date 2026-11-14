package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Checks an adapter FOM's three-level interaction-class and parameter hierarchy. */
final class FomInteractionClassHierarchyTck {
   private static final String ROOT = "HLAinteractionRoot";
   private static final String BASE = ROOT + ".TckTypedInteraction";
   private static final String DERIVED = BASE + ".TckTypedDerivedInteraction";
   private static final String GRANDCHILD = DERIVED + ".TckTypedGrandchildInteraction";

   private FomInteractionClassHierarchyTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty(),
         "adapter-supplied model FOM is required");
      RTIambassador ambassador = factory.getRtiAmbassador();
      String federation = "java-tck-interaction-class-hierarchy-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         ambassador.connect(new NullFederateAmbassador(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(
            federation, new String[] {fom}, timeImplementation);
         created = true;
         ambassador.joinFederationExecution("java-tck-interaction-hierarchy-member", federation);
         joined = true;

         InteractionClassHandle root = ambassador.getInteractionClassHandle(ROOT);
         InteractionClassHandle base = ambassador.getInteractionClassHandle(BASE);
         InteractionClassHandle derived = ambassador.getInteractionClassHandle(DERIVED);
         InteractionClassHandle grandchild = ambassador.getInteractionClassHandle(GRANDCHILD);
         JavaTckSupport.check(root != null && base != null && derived != null && grandchild != null
               && !root.equals(base) && !base.equals(derived) && !derived.equals(grandchild),
            "FOM interaction-class hierarchy did not resolve to distinct public handles");
         JavaTckSupport.check(ROOT.equals(ambassador.getInteractionClassName(root))
               && BASE.equals(ambassador.getInteractionClassName(base))
               && DERIVED.equals(ambassador.getInteractionClassName(derived))
               && GRANDCHILD.equals(ambassador.getInteractionClassName(grandchild)),
            "FOM interaction-class paths did not round-trip through their handles");

         ParameterHandle baseParameter =
            ambassador.getParameterHandle(base, "IntegerParameter");
         ParameterHandle derivedParameter =
            ambassador.getParameterHandle(derived, "DerivedParameter");
         JavaTckSupport.check(baseParameter != null && derivedParameter != null
               && !baseParameter.equals(derivedParameter),
            "FOM interaction parameter rows did not resolve to distinct handles");
         JavaTckSupport.check(baseParameter.equals(
                  ambassador.getParameterHandle(derived, "IntegerParameter"))
               && baseParameter.equals(
                  ambassador.getParameterHandle(grandchild, "IntegerParameter"))
               && derivedParameter.equals(
                  ambassador.getParameterHandle(grandchild, "DerivedParameter")),
            "interaction descendants did not preserve inherited parameter identity");
         JavaTckSupport.check("IntegerParameter".equals(
                  ambassador.getParameterName(base, baseParameter))
               && "IntegerParameter".equals(
                  ambassador.getParameterName(derived, baseParameter))
               && "DerivedParameter".equals(
                  ambassador.getParameterName(derived, derivedParameter))
               && "IntegerParameter".equals(
                  ambassador.getParameterName(grandchild, baseParameter))
               && "DerivedParameter".equals(
                  ambassador.getParameterName(grandchild, derivedParameter)),
            "interaction parameter names did not round-trip across inherited class paths");
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }
}
