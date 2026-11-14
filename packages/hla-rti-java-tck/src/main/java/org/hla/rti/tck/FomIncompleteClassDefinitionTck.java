package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;

/** Verifies incomplete object and interaction class scaffolds fail atomically at Create and Join. */
final class FomIncompleteClassDefinitionTck {
   private static final String[] INCOMPLETE_FOM_EXCEPTIONS = {
      "InvalidFOM", "InconsistentFOM"
   };

   private FomIncompleteClassDefinitionTck() { }

   static void run(RtiFactory factory, String baseFom, String timeImplementation,
         String incompleteObjectClassFom, String completeObjectClassProviderFom,
         String incompleteInteractionClassFom, String completeInteractionClassProviderFom,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      boolean creatorConnected = false;
      try {
         creator.connect(callbacks(), callbackModel);
         creatorConnected = true;
         verifyCreateRejectsUnfinishedClass(creator, timeImplementation,
            incompleteObjectClassFom, completeObjectClassProviderFom,
            "object-class");
         verifyCreateRejectsUnfinishedClass(creator, timeImplementation,
            incompleteInteractionClassFom, completeInteractionClassProviderFom,
            "interaction-class");
      } finally {
         if (creatorConnected) disconnect(creator);
      }

      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador joiner = factory.getRtiAmbassador();
      String federation = "java-tck-fom-incomplete-join-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean joinerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         joiner.connect(callbacks(), callbackModel);
         joinerConnected = true;
         owner.createFederationExecution(federation, baseFom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-fom-incomplete-owner", federation);
         ownerJoined = true;

         verifyJoinRejectsUnfinishedObjectClass(owner, joiner, federation,
            incompleteObjectClassFom, completeObjectClassProviderFom);
         verifyJoinRejectsUnfinishedInteractionClass(owner, joiner, federation,
            incompleteInteractionClassFom, completeInteractionClassProviderFom);
      } finally {
         resign(joiner);
         if (ownerJoined) resign(owner);
         if (created) destroy(owner, federation);
         if (joinerConnected) disconnect(joiner);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void verifyCreateRejectsUnfinishedClass(RTIambassador creator,
         String timeImplementation, String incompleteFom, String completeProviderFom,
         String classKind) throws Exception {
      String federation = "java-tck-fom-incomplete-create-" + UUID.randomUUID();
      boolean created = false;
      try {
         try {
            creator.createFederationExecution(federation, new String[] {incompleteFom},
               timeImplementation);
            created = true;
         } catch (Throwable error) {
            if (!hasIncompleteFomException(error)) {
               throw new AssertionError("incomplete " + classKind
                  + " Create threw unexpected " + error, error);
            }
            creator.createFederationExecution(federation,
               new String[] {incompleteFom, completeProviderFom}, timeImplementation);
            created = true;
            return;
         }
         throw new AssertionError("Create accepted an unfinished " + classKind + " scaffold");
      } finally {
         if (created) destroy(creator, federation);
      }
   }

   private static void verifyJoinRejectsUnfinishedObjectClass(RTIambassador owner,
         RTIambassador joiner, String federation, String incompleteFom,
         String completeProviderFom) throws Exception {
      String className = "HLAobjectRoot.TckScaffoldObjectClass";
      String federateName = "java-tck-fom-incomplete-joiner";
      expectRejectedJoin(joiner, federateName, federation, incompleteFom,
         "object-class");
      JavaTckSupport.expectFailure(() -> owner.getFederateHandle(federateName),
         "NameNotFound", "rejected incomplete-object-class Join membership lookup");
      JavaTckSupport.expectFailure(() -> owner.getObjectClassHandle(className),
         "NameNotFound", "rejected incomplete-object-class Join declaration lookup");

      boolean joined = false;
      try {
         FederateHandle joinHandle = joiner.joinFederationExecution(federateName,
            "java-tck-fom-incomplete-object-joiner", federation,
            new String[] {incompleteFom, completeProviderFom});
         joined = true;
         ObjectClassHandle ownerClass = owner.getObjectClassHandle(className);
         ObjectClassHandle joinerClass = joiner.getObjectClassHandle(className);
         JavaTckSupport.check(ownerClass.equals(joinerClass)
               && className.equals(owner.getObjectClassName(ownerClass))
               && className.equals(joiner.getObjectClassName(joinerClass))
               && joinHandle.equals(owner.getFederateHandle(federateName)),
            "complete additional-module Join did not compose the object class and member");
      } finally {
         if (joined) resign(joiner);
      }
   }

   private static void verifyJoinRejectsUnfinishedInteractionClass(RTIambassador owner,
         RTIambassador joiner, String federation, String incompleteFom,
         String completeProviderFom) throws Exception {
      String className = "HLAinteractionRoot.TckScaffoldInteractionClass";
      String federateName = "java-tck-fom-incomplete-joiner";
      expectRejectedJoin(joiner, federateName, federation, incompleteFom,
         "interaction-class");
      JavaTckSupport.expectFailure(() -> owner.getFederateHandle(federateName),
         "NameNotFound", "rejected incomplete-interaction-class Join membership lookup");
      JavaTckSupport.expectFailure(() -> owner.getInteractionClassHandle(className),
         "NameNotFound", "rejected incomplete-interaction-class Join declaration lookup");

      boolean joined = false;
      try {
         FederateHandle joinHandle = joiner.joinFederationExecution(federateName,
            "java-tck-fom-incomplete-interaction-joiner", federation,
            new String[] {incompleteFom, completeProviderFom});
         joined = true;
         InteractionClassHandle ownerClass = owner.getInteractionClassHandle(className);
         InteractionClassHandle joinerClass = joiner.getInteractionClassHandle(className);
         JavaTckSupport.check(ownerClass.equals(joinerClass)
               && className.equals(owner.getInteractionClassName(ownerClass))
               && className.equals(joiner.getInteractionClassName(joinerClass))
               && joinHandle.equals(owner.getFederateHandle(federateName)),
            "complete additional-module Join did not compose the interaction class and member");
      } finally {
         if (joined) resign(joiner);
      }
   }

   private static void expectRejectedJoin(RTIambassador joiner, String federateName,
         String federation, String incompleteFom, String classKind) throws Exception {
      try {
         joiner.joinFederationExecution(federateName,
            "java-tck-fom-incomplete-" + classKind + "-joiner", federation,
            new String[] {incompleteFom});
      } catch (Throwable error) {
         if (hasIncompleteFomException(error)) return;
         throw new AssertionError("incomplete " + classKind
            + " Join threw unexpected " + error, error);
      }
      throw new AssertionError("Join accepted an unfinished " + classKind + " scaffold");
   }

   private static boolean hasIncompleteFomException(Throwable error) {
      for (Throwable current = error; current != null; current = current.getCause()) {
         if (Arrays.asList(INCOMPLETE_FOM_EXCEPTIONS)
               .contains(current.getClass().getSimpleName())) {
            return true;
         }
      }
      return false;
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if ("toString".equals(method.getName())) {
               return "Java TCK incomplete FOM class callbacks";
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
}
