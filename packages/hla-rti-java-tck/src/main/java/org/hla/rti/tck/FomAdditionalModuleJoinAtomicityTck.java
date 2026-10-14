package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;

/** Verifies invalid additional FOM joins do not partially change membership or declarations. */
final class FomAdditionalModuleJoinAtomicityTck {
   private static final String[] INVALID_FOM_EXCEPTIONS = {
      "CouldNotOpenFOM", "ErrorReadingFOM", "InvalidFOM", "InconsistentFOM"
   };

   private FomAdditionalModuleJoinAtomicityTck() { }

   static void run(RtiFactory factory, String baseFom, String extensionFom,
         String[] invalidFomModules, String extensionObjectClassName,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      run(factory, baseFom, extensionFom, invalidFomModules, extensionObjectClassName,
         timeImplementation, callbackModel, false);
   }

   static void runComposite(RtiFactory factory, String baseFom, String extensionFom,
         String[] invalidFomModules, String extensionObjectClassName,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      run(factory, baseFom, extensionFom, invalidFomModules, extensionObjectClassName,
         timeImplementation, callbackModel, true);
   }

   private static void run(RtiFactory factory, String baseFom, String extensionFom,
         String[] invalidFomModules, String extensionObjectClassName,
         String timeImplementation, CallbackModel callbackModel,
         boolean combineValidAndInvalidModules) throws Exception {
      JavaTckSupport.check(invalidFomModules.length > 0,
         "adapter must supply at least one invalid FOM module input");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String ownerName = "java-tck-fom-join-owner";
      String memberName = "java-tck-fom-join-member";
      String federation = "java-tck-fom-join-atomicity-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, baseFom, timeImplementation);
         created = true;
         owner.joinFederationExecution(ownerName, federation);
         ownerJoined = true;

         JavaTckSupport.expectFailure(() -> owner.getObjectClassHandle(
               extensionObjectClassName), "NameNotFound",
            "extension-class lookup before the extension module is joined");

         for (String invalidFom : invalidFomModules) {
            expectInvalidJoin(member, memberName, federation, extensionFom, invalidFom,
               combineValidAndInvalidModules);
            JavaTckSupport.expectFailure(() -> owner.getFederateHandle(memberName),
               "NameNotFound", "member lookup after rejected additional-module join");
            JavaTckSupport.expectFailure(() -> owner.getObjectClassHandle(
                  extensionObjectClassName), "NameNotFound",
               "extension-class lookup after rejected additional-module join");
         }

         FederateHandle joinedHandle = member.joinFederationExecution(memberName,
            "java-tck-fom-join-member", federation, new String[] {extensionFom});
         memberJoined = true;
         ObjectClassHandle ownerExtension = owner.getObjectClassHandle(extensionObjectClassName);
         ObjectClassHandle memberExtension =
            member.getObjectClassHandle(extensionObjectClassName);
         JavaTckSupport.check(ownerExtension.equals(memberExtension)
               && extensionObjectClassName.equals(owner.getObjectClassName(ownerExtension))
               && extensionObjectClassName.equals(member.getObjectClassName(memberExtension))
               && joinedHandle.equals(owner.getFederateHandle(memberName)),
            "valid additional-module join did not compose shared declarations and membership");
      } finally {
         if (memberJoined) resign(member);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (memberConnected) disconnect(member);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void expectInvalidJoin(RTIambassador member, String memberName,
         String federation, String extensionFom, String invalidFom,
         boolean combineValidAndInvalidModules) throws Exception {
      try {
         String[] modules = combineValidAndInvalidModules
            ? new String[] {extensionFom, invalidFom} : new String[] {invalidFom};
         member.joinFederationExecution(memberName, "java-tck-fom-join-member", federation,
            modules);
      } catch (Throwable error) {
         for (Throwable current = error; current != null; current = current.getCause()) {
            if (Arrays.asList(INVALID_FOM_EXCEPTIONS)
                  .contains(current.getClass().getSimpleName())) {
               return;
            }
         }
         throw new AssertionError("invalid additional FOM join threw unexpected " + error, error);
      }
      throw new AssertionError("invalid additional FOM join completed without a standard FOM error");
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("toString".equals(method.getName())) return "Java TCK FOM join callbacks";
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
