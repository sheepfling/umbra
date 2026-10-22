package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Object and interaction root hierarchy through the standard RTI API. */
final class FomRootHierarchyTck {
   private FomRootHierarchyTck() { }

   static void run(RtiFactory factory, String valid, String invalidObject,
         String invalidInteraction, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-root-hierarchy-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         creator.createFederationExecution(federation,
            new String[] {valid}, timeImplementation);
         created = true;
         creator.joinFederationExecution("java-tck-root-member", federation);
         joined = true;
         String objectName = "HLAobjectRoot.TckRootObject";
         String interactionName = "HLAinteractionRoot.TckRootInteraction";
         ObjectClassHandle object = creator.getObjectClassHandle(objectName);
         InteractionClassHandle interaction =
            creator.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(objectName.equals(creator.getObjectClassName(object))
               && interactionName.equals(creator.getInteractionClassName(interaction)),
            "accepted standard roots did not expose their child class names");
         creator.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         creator.destroyFederationExecution(federation);
         created = false;
         for (String module : new String[] {invalidObject, invalidInteraction}) {
            JavaTckSupport.expectFailure(
               () -> creator.createFederationExecution(federation,
                  new String[] {module}, timeImplementation),
               "InconsistentFOM", "nonstandard top-level class");
            creator.createFederationExecution(federation,
               new String[] {valid}, timeImplementation);
            created = true;
            creator.destroyFederationExecution(federation);
            created = false;
         }
      } finally {
         if (joined) try { creator.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (connected) try { creator.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK root callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
