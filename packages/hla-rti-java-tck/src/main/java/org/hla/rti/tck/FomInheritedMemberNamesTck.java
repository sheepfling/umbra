package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Inherited attribute and parameter name rules through the standard RTI API. */
final class FomInheritedMemberNamesTck {
   private FomInheritedMemberNamesTck() { }

   static void run(RtiFactory factory, String valid, String duplicateAttribute,
         String duplicateParameter, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-inherited-member-names-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         creator.createFederationExecution(federation,
            new String[] {valid}, timeImplementation);
         created = true;
         creator.joinFederationExecution("java-tck-inherited-member", federation);
         joined = true;
         ObjectClassHandle objectBase = creator.getObjectClassHandle(
            "HLAobjectRoot.TckInheritedAttributeBase");
         ObjectClassHandle objectChild = creator.getObjectClassHandle(
            "HLAobjectRoot.TckInheritedAttributeBase.TckInheritedAttributeChild");
         AttributeHandle baseAttribute = creator.getAttributeHandle(
            objectBase, "TckInheritedAttribute");
         AttributeHandle childInheritedAttribute = creator.getAttributeHandle(
            objectChild, "TckInheritedAttribute");
         AttributeHandle childAttribute = creator.getAttributeHandle(
            objectChild, "TckChildAttribute");
         JavaTckSupport.check(baseAttribute.equals(childInheritedAttribute)
               && !childAttribute.equals(childInheritedAttribute)
               && "TckInheritedAttribute".equals(
                  creator.getAttributeName(objectChild, childInheritedAttribute)),
            "valid object hierarchy did not preserve inherited attribute identity");

         InteractionClassHandle interactionBase = creator.getInteractionClassHandle(
            "HLAinteractionRoot.TckInheritedParameterBase");
         InteractionClassHandle interactionChild = creator.getInteractionClassHandle(
            "HLAinteractionRoot.TckInheritedParameterBase.TckInheritedParameterChild");
         ParameterHandle baseParameter = creator.getParameterHandle(
            interactionBase, "TckInheritedParameter");
         ParameterHandle childInheritedParameter = creator.getParameterHandle(
            interactionChild, "TckInheritedParameter");
         ParameterHandle childParameter = creator.getParameterHandle(
            interactionChild, "TckChildParameter");
         JavaTckSupport.check(baseParameter.equals(childInheritedParameter)
               && !childParameter.equals(childInheritedParameter)
               && "TckInheritedParameter".equals(
                  creator.getParameterName(interactionChild, childInheritedParameter)),
            "valid interaction hierarchy did not preserve inherited parameter identity");
         creator.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         creator.destroyFederationExecution(federation);
         created = false;

         for (String module : new String[] {duplicateAttribute, duplicateParameter}) {
            JavaTckSupport.expectFailure(
               () -> creator.createFederationExecution(federation,
                  new String[] {module}, timeImplementation),
               "InconsistentFOM", "duplicate inherited member name");
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
               if ("toString".equals(method.getName())) return "Java TCK inherited member callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
