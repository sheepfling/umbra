package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
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

   static void runAdditionalJoinCompatibility(RtiFactory factory, String baseFom,
         String extensionFom, String[] invalidFomModules,
         String[] standardInvalidFomModules, String extensionObjectClassName,
         String duplicateFom, String orderConflictFom, String interactionConflictFom,
         String transportationConflictFom, String dimensionBaseFom,
         String dimensionConflictFom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(invalidFomModules.length > 0,
         "adapter must supply at least one invalid FOM module input");
      String[] allInvalidFomModules = Arrays.copyOf(invalidFomModules,
         invalidFomModules.length + standardInvalidFomModules.length);
      System.arraycopy(standardInvalidFomModules, 0, allInvalidFomModules,
         invalidFomModules.length, standardInvalidFomModules.length);
      String[] requiredFoms = {duplicateFom, orderConflictFom, interactionConflictFom,
         transportationConflictFom, dimensionBaseFom, dimensionConflictFom};
      for (String requiredFom : requiredFoms) {
         JavaTckSupport.check(requiredFom != null && !requiredFom.isEmpty(),
            "adapter must supply all additional-module compatibility FOM fixtures");
      }

      runCurrentFddCompatibility(factory, baseFom, extensionFom, allInvalidFomModules,
         extensionObjectClassName, duplicateFom, orderConflictFom,
         interactionConflictFom, transportationConflictFom, timeImplementation,
         callbackModel);
      runDimensionConflict(factory, dimensionBaseFom, dimensionConflictFom,
         timeImplementation, callbackModel);
   }

   private static void runCurrentFddCompatibility(RtiFactory factory, String baseFom,
         String extensionFom, String[] invalidFomModules, String extensionObjectClassName,
         String duplicateFom, String orderConflictFom, String interactionConflictFom,
         String transportationConflictFom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-fom-join-compatibility-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, baseFom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-fom-compatibility-owner", federation);

         JavaTckSupport.expectFailure(() -> owner.getObjectClassHandle(
               extensionObjectClassName), "NameNotFound",
            "extension-class lookup before the extension module is joined");
         for (String invalidFom : invalidFomModules) {
            expectInvalidJoin(member, "java-tck-fom-compatibility-member", federation,
               extensionFom, invalidFom, true);
            assertRejectedJoinAbsent(owner, "java-tck-fom-compatibility-member",
               extensionObjectClassName, "rejected additional-module join");
         }

         ObjectClassHandle baseObjectClass = owner.getObjectClassHandle(
            "HLAobjectRoot.TckObject");
         AttributeHandle baseValue = owner.getAttributeHandle(baseObjectClass, "Value");
         member.joinFederationExecution("java-tck-fom-compatibility-member",
            "java-tck-fom-compatibility-member", federation, new String[] {duplicateFom});
         ObjectClassHandle duplicateObjectClass = member.getObjectClassHandle(
            "HLAobjectRoot.TckObject");
         AttributeHandle duplicateValue = member.getAttributeHandle(
            duplicateObjectClass, "Value");
         JavaTckSupport.check(baseObjectClass.equals(duplicateObjectClass)
               && baseValue.equals(duplicateValue),
            "matching duplicate current-FDD attribute declaration changed its handles");
         member.resignFederationExecution(ResignAction.NO_ACTION);

         expectInconsistentJoin(member, owner, "java-tck-fom-compatibility-member",
            federation, orderConflictFom, "attribute-order conflict");
         expectInconsistentJoin(member, owner, "java-tck-fom-compatibility-member",
            federation, interactionConflictFom, "interaction-order conflict");
         expectInconsistentJoin(member, owner, "java-tck-fom-compatibility-member",
            federation, transportationConflictFom, "attribute-transportation conflict");

         FederateHandle joinedHandle = member.joinFederationExecution(
            "java-tck-fom-compatibility-member", "java-tck-fom-compatibility-member",
            federation, new String[] {extensionFom});
         ObjectClassHandle ownerExtension = owner.getObjectClassHandle(
            extensionObjectClassName);
         ObjectClassHandle memberExtension = member.getObjectClassHandle(
            extensionObjectClassName);
         JavaTckSupport.check(ownerExtension.equals(memberExtension)
               && extensionObjectClassName.equals(owner.getObjectClassName(ownerExtension))
               && extensionObjectClassName.equals(member.getObjectClassName(memberExtension))
               && joinedHandle.equals(owner.getFederateHandle(
                  "java-tck-fom-compatibility-member")),
            "valid additional-module join did not compose shared declarations and membership");
      } finally {
         if (memberConnected) resign(member);
         if (ownerConnected) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (memberConnected) disconnect(member);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void runDimensionConflict(RtiFactory factory, String dimensionBaseFom,
         String dimensionConflictFom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-fom-dimension-conflict-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, dimensionBaseFom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-fom-dimension-owner", federation);
         DimensionHandle dimension = owner.getDimensionHandle("TckDuplicateDimension");
         long currentUpperBound = owner.getDimensionUpperBound(dimension);
         JavaTckSupport.check(currentUpperBound == 100L,
            "dimension-conflict base FOM did not expose its current upper bound");

         JavaTckSupport.expectFailure(() -> member.joinFederationExecution(
               "java-tck-fom-dimension-member", "java-tck-fom-dimension-member",
               federation, new String[] {dimensionConflictFom}), "InconsistentFOM",
            "additional-module Join with a conflicting dimension upper bound");
         JavaTckSupport.expectFailure(() -> owner.getFederateHandle(
               "java-tck-fom-dimension-member"), "NameNotFound",
            "member lookup after rejected dimension-conflicting additional-module join");
         JavaTckSupport.check(owner.getDimensionUpperBound(dimension) == currentUpperBound,
            "rejected dimension-conflicting join changed the current FDD upper bound");
      } finally {
         if (memberConnected) resign(member);
         if (ownerConnected) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (memberConnected) disconnect(member);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void expectInconsistentJoin(RTIambassador member, RTIambassador owner,
         String memberName, String federation, String conflictFom, String description)
         throws Exception {
      JavaTckSupport.expectFailure(() -> member.joinFederationExecution(memberName,
            "java-tck-fom-compatibility-member", federation, new String[] {conflictFom}),
         "InconsistentFOM", "additional-module Join with " + description);
      JavaTckSupport.expectFailure(() -> owner.getFederateHandle(memberName), "NameNotFound",
         "member lookup after rejected " + description);
   }

   private static void assertRejectedJoinAbsent(RTIambassador owner, String memberName,
         String extensionObjectClassName, String description) throws Exception {
      JavaTckSupport.expectFailure(() -> owner.getFederateHandle(memberName), "NameNotFound",
         "member lookup after " + description);
      JavaTckSupport.expectFailure(() -> owner.getObjectClassHandle(extensionObjectClassName),
         "NameNotFound", "extension-class lookup after " + description);
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
