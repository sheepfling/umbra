package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;

/** Checks semantic FOM rejection and atomicity for a noncompliant zero update rate. */
final class FomNoncompliantModuleTck {
   private static final String[] NONCOMPLIANT_FOM_EXCEPTIONS = {
      "InvalidFOM", "InconsistentFOM"
   };
   private static final String REJECTED_CLASS_NAME =
      "HLAobjectRoot.TckRejectedModuleObject";

   private FomNoncompliantModuleTck() { }

   static void run(RtiFactory factory, String baseFom, String noncompliantUpdateRateFom,
         String noncompliantAtomicityFom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      verifyCreateAtomicity(factory, baseFom, noncompliantUpdateRateFom,
         timeImplementation, callbackModel);
      verifyJoinAtomicity(factory, baseFom, noncompliantAtomicityFom,
         timeImplementation, callbackModel);
   }

   private static void verifyCreateAtomicity(RtiFactory factory, String baseFom,
         String noncompliantUpdateRateFom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      String federation = "java-tck-fom-noncompliant-create-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         creator.connect(callbacks(), callbackModel);
         connected = true;
         expectNoncompliantFomError(() -> creator.createFederationExecution(federation,
               new String[] {baseFom, noncompliantUpdateRateFom}, timeImplementation),
            "valid-plus-noncompliant Create");

         creator.createFederationExecution(federation, baseFom, timeImplementation);
         created = true;
         FederateHandle handle;
         try {
            handle = creator.joinFederationExecution(
               "java-tck-fom-noncompliant-create-owner", federation);
         } catch (Throwable error) {
            throw new AssertionError("valid Create retry Join failed", error);
         }
         joined = true;
         JavaTckSupport.check(handle != null,
            "valid Create retry after noncompliant-module rejection did not permit Join");
      } finally {
         if (joined) resign(creator);
         if (created) destroy(creator, federation);
         if (connected) disconnect(creator);
      }
   }

   private static void verifyJoinAtomicity(RtiFactory factory, String baseFom,
         String noncompliantAtomicityFom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador joiner = factory.getRtiAmbassador();
      String federation = "java-tck-fom-noncompliant-join-" + UUID.randomUUID();
      String ownerName = "java-tck-fom-noncompliant-owner";
      String memberName = "java-tck-fom-noncompliant-member";
      boolean ownerConnected = false;
      boolean joinerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         joiner.connect(callbacks(), callbackModel);
         joinerConnected = true;
         owner.createFederationExecution(federation, baseFom, timeImplementation);
         created = true;
         owner.joinFederationExecution(ownerName, federation);
         ownerJoined = true;

         JavaTckSupport.expectFailure(() -> owner.getObjectClassHandle(REJECTED_CLASS_NAME),
            "NameNotFound", "lookup of the mixed-module class before Join");
         expectNoncompliantFomError(() -> joiner.joinFederationExecution(memberName,
               "java-tck-fom-noncompliant-member", federation,
               new String[] {baseFom, noncompliantAtomicityFom}),
            "valid-plus-noncompliant Join");
         JavaTckSupport.expectFailure(() -> owner.getFederateHandle(memberName),
            "NameNotFound", "member lookup after noncompliant-module Join rejection");
         JavaTckSupport.expectFailure(() -> owner.getObjectClassHandle(REJECTED_CLASS_NAME),
            "NameNotFound", "class lookup after noncompliant-module Join rejection");

         FederateHandle memberHandle;
         try {
            memberHandle = joiner.joinFederationExecution(memberName,
               "java-tck-fom-noncompliant-member", federation);
         } catch (Throwable error) {
            throw new AssertionError("valid Join retry after rejection failed", error);
         }
         memberJoined = true;
         FederateHandle activeMember = queryFederate(owner, memberName,
            "member lookup after valid Join retry");
         JavaTckSupport.check(memberHandle != null
               && memberHandle.equals(activeMember),
            "valid Join retry after noncompliant-module rejection did not establish membership");
         JavaTckSupport.expectFailure(() -> owner.getObjectClassHandle(REJECTED_CLASS_NAME),
            "NameNotFound", "class lookup after the valid Join retry");
      } finally {
         if (memberJoined) resign(joiner);
         if (ownerJoined) resign(owner);
         if (created) destroy(owner, federation);
         if (joinerConnected) disconnect(joiner);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void expectNoncompliantFomError(CheckedOperation operation,
         String description) throws Exception {
      try {
         operation.run();
      } catch (Throwable error) {
         for (Throwable current = error; current != null; current = current.getCause()) {
            if (Arrays.asList(NONCOMPLIANT_FOM_EXCEPTIONS)
                  .contains(current.getClass().getSimpleName())) {
               return;
            }
         }
         throw new AssertionError(description + " threw unexpected " + error, error);
      }
      throw new AssertionError(description + " succeeded for a noncompliant update rate");
   }

   private static FederateHandle queryFederate(RTIambassador ambassador, String federateName,
         String description) throws Exception {
      try {
         return ambassador.getFederateHandle(federateName);
      } catch (Throwable error) {
         throw new AssertionError(description + " failed", error);
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("toString".equals(method.getName())) {
               return "Java TCK noncompliant FOM callbacks";
            }
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

   private static void destroy(RTIambassador ambassador, String federation) {
      try { ambassador.destroyFederationExecution(federation); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   @FunctionalInterface
   private interface CheckedOperation {
      void run() throws Exception;
   }
}
