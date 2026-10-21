package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Same-named class available-dimension union without region services. */
final class FomClassDimensionMergeTck {
   private FomClassDimensionMergeTck() { }

   static void run(RtiFactory factory, String first, String second,
         String timeImplementation, CallbackModel callbackModel,
         boolean interaction) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-class-dimension-merge-" + UUID.randomUUID();
      String firstName = interaction ? "TckInteractionMergeFirstDimension"
         : "TckObjectMergeFirstDimension";
      String secondName = interaction ? "TckInteractionMergeSecondDimension"
         : "TckObjectMergeSecondDimension";
      boolean creatorConnected = false;
      boolean memberConnected = false;
      boolean memberJoined = false;
      boolean created = false;
      try {
         creator.connect(callbacks(), callbackModel);
         creatorConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         for (String module : new String[] {first, second}) {
            creator.createFederationExecution(
               federation, new String[] {module}, timeImplementation);
            created = true;
            creator.destroyFederationExecution(federation);
            created = false;
         }
         creator.createFederationExecution(
            federation, new String[] {first, second}, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-dimension-merge-member", federation);
         memberJoined = true;

         DimensionHandle firstHandle = member.getDimensionHandle(firstName);
         DimensionHandle secondHandle = member.getDimensionHandle(secondName);
         JavaTckSupport.check(firstHandle != null && secondHandle != null
               && !firstHandle.equals(secondHandle)
               && firstName.equals(member.getDimensionName(firstHandle))
               && secondName.equals(member.getDimensionName(secondHandle)),
            "merged dimensions did not resolve as distinct named handles");
         DimensionHandleSet dimensions = interaction
            ? member.getAvailableDimensionsForInteractionClass(
               member.getInteractionClassHandle(
                  "HLAinteractionRoot.TckDimensionMergeInteraction"))
            : member.getAvailableDimensionsForObjectClass(
               member.getObjectClassHandle("HLAobjectRoot.TckDimensionMergeObject"));
         JavaTckSupport.check(dimensions.size() == 2
               && dimensions.contains(firstHandle)
               && dimensions.contains(secondHandle),
            "same-named class did not retain both available dimensions");

         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         creator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (memberJoined) try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (memberConnected) try { member.disconnect(); }
            catch (Exception ignored) { }
         if (creatorConnected) try { creator.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK class dimensions callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
