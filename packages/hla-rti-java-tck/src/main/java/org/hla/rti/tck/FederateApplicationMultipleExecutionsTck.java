package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks one application joining federates in multiple executions. */
final class FederateApplicationMultipleExecutionsTck {
   private FederateApplicationMultipleExecutionsTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String federateType, CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador firstOne = factory.getRtiAmbassador();
      RTIambassador firstTwo = factory.getRtiAmbassador();
      RTIambassador secondOne = factory.getRtiAmbassador();
      RTIambassador secondTwo = factory.getRtiAmbassador();
      RTIambassador[] ambassadors = {creator, firstOne, firstTwo, secondOne, secondTwo};
      String firstFederation = "java-tck-multiple-executions-" + UUID.randomUUID();
      String secondFederation = firstFederation + "-second";

      try {
         for (RTIambassador ambassador : ambassadors) {
            ambassador.connect(callbacks(), callbackModel);
         }
         creator.createFederationExecution(firstFederation, fom, timeImplementation);
         creator.createFederationExecution(secondFederation, fom, timeImplementation);

         FederateHandle firstOneHandle = firstOne.joinFederationExecution(
            "multi-execution-first-one", federateType, firstFederation);
         FederateHandle secondOneHandle = secondOne.joinFederationExecution(
            "multi-execution-second-one", federateType, secondFederation);
         FederateHandle firstTwoHandle = firstTwo.joinFederationExecution(
            "multi-execution-first-two", federateType, firstFederation);
         FederateHandle secondTwoHandle = secondTwo.joinFederationExecution(
            "multi-execution-second-two", federateType, secondFederation);
         checkHandle(firstOneHandle, "first execution first federate");
         checkHandle(secondOneHandle, "second execution first federate");
         checkHandle(firstTwoHandle, "first execution second federate");
         checkHandle(secondTwoHandle, "second execution second federate");

         firstTwo.resignFederationExecution(ResignAction.NO_ACTION);
         firstTwoHandle = firstTwo.joinFederationExecution(
            "multi-execution-first-two-rejoined", federateType, secondFederation);
         checkHandle(firstTwoHandle, "reused RTIambassador in second execution");

         secondOne.resignFederationExecution(ResignAction.NO_ACTION);
         firstOne.resignFederationExecution(ResignAction.NO_ACTION);
         secondTwo.resignFederationExecution(ResignAction.NO_ACTION);
         firstTwo.resignFederationExecution(ResignAction.NO_ACTION);
         creator.destroyFederationExecution(firstFederation);
         creator.destroyFederationExecution(secondFederation);
      } finally {
         for (RTIambassador ambassador : ambassadors) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         try { creator.destroyFederationExecution(firstFederation); }
         catch (Exception ignored) { }
         try { creator.destroyFederationExecution(secondFederation); }
         catch (Exception ignored) { }
         for (RTIambassador ambassador : ambassadors) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK multi-execution callbacks";
               }
               if ("hashCode".equals(method.getName())) {
                  return System.identityHashCode(proxy);
               }
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static void checkHandle(FederateHandle handle, String description) {
      JavaTckSupport.check(handle != null && handle.encodedLength() > 0,
         description + " returned an invalid FederateHandle");
   }
}
