package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.UUID;

/** Verifies that public FOM name lookups preserve case and complete class paths. */
final class FomNameCaseSensitivityTck {
   private static final String FOM_PROPERTY =
      "hla.rti.tck.supportCaseSensitivityFom";
   private static final String DEFAULT_FOM =
      "packages/hla-rti-cpp-tck/fom/p0-tck.xml";
   private static final String ROOTLESS_FOM_PROPERTY =
      "hla.rti.tck.supportCaseSensitivityRootlessFom";
   private static final String DEFAULT_ROOTLESS_FOM =
      "packages/hla-rti-cpp-tck/fom/fom-root-classes-omitted-tck.xml";
   private static final String OBJECT_CLASS = "HLAobjectRoot.TckObject";
   private static final String ATTRIBUTE = "Value";
   private static final String INTERACTION_CLASS = "HLAinteractionRoot.TckInteraction";
   private static final String PARAMETER = "Payload";

   private FomNameCaseSensitivityTck() {
   }

   static void run(RtiFactory factory, String mim, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      String fom = System.getProperty(FOM_PROPERTY, DEFAULT_FOM);
      JavaTckSupport.check(Files.isRegularFile(Path.of(fom)),
         "FOM name case-sensitivity fixture does not exist: " + fom);
      runP0Fom(factory, fom, mim, timeImplementation, callbackModel);

      String rootlessFom = System.getProperty(ROOTLESS_FOM_PROPERTY, DEFAULT_ROOTLESS_FOM);
      JavaTckSupport.check(Files.isRegularFile(Path.of(rootlessFom)),
         "root-class-omitted FOM fixture does not exist: " + rootlessFom);
      runRootlessFom(factory, rootlessFom, timeImplementation, callbackModel);
   }

   private static void runP0Fom(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador member = factory.getRtiAmbassador();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-fom-name-case-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         member.connect(SupportServicesTck.noOpFederateAmbassador(), callbackModel);
         connected = true;
         member.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-fom-name-case-member-" + suffix, federation);
         joined = true;

         ObjectClassHandle objectClass = member.getObjectClassHandle(OBJECT_CLASS);
         JavaTckSupport.check(objectClass != null
               && OBJECT_CLASS.equals(member.getObjectClassName(objectClass)),
            "exact-case object-class lookup did not preserve its FOM name");
         ObjectClassHandle objectRoot = member.getObjectClassHandle("HLAobjectRoot");
         JavaTckSupport.check(objectRoot != null
               && "HLAobjectRoot".equals(member.getObjectClassName(objectRoot)),
            "the HLA object root class was not accepted by its root name");
         JavaTckSupport.expectFailure(() -> member.getObjectClassHandle("TckObject"),
            "NameNotFound", "object-class lookup accepted an unqualified name");
         JavaTckSupport.expectFailure(
            () -> member.getObjectClassHandle(firstLetterCaseVariant(OBJECT_CLASS)),
            "NameNotFound", "case-mismatched object-class name was accepted");

         var attribute = member.getAttributeHandle(objectClass, ATTRIBUTE);
         JavaTckSupport.check(attribute != null
               && ATTRIBUTE.equals(member.getAttributeName(objectClass, attribute)),
            "exact-case attribute lookup did not preserve its FOM name");
         JavaTckSupport.expectFailure(
            () -> member.getAttributeHandle(objectClass, firstLetterCaseVariant(ATTRIBUTE)),
            "NameNotFound", "case-mismatched attribute name was accepted");

         InteractionClassHandle interactionClass =
            member.getInteractionClassHandle(INTERACTION_CLASS);
         JavaTckSupport.check(interactionClass != null
               && INTERACTION_CLASS.equals(member.getInteractionClassName(interactionClass)),
            "exact-case interaction-class lookup did not preserve its FOM name");
         InteractionClassHandle interactionRoot =
            member.getInteractionClassHandle("HLAinteractionRoot");
         JavaTckSupport.check(interactionRoot != null
               && "HLAinteractionRoot".equals(member.getInteractionClassName(interactionRoot)),
            "the HLA interaction root class was not accepted by its root name");
         JavaTckSupport.expectFailure(
            () -> member.getInteractionClassHandle("TckInteraction"),
            "NameNotFound", "interaction-class lookup accepted an unqualified name");
         JavaTckSupport.expectFailure(
            () -> member.getInteractionClassHandle(
               firstLetterCaseVariant(INTERACTION_CLASS)),
            "NameNotFound", "case-mismatched interaction-class name was accepted");

         ParameterHandle parameter = member.getParameterHandle(interactionClass, PARAMETER);
         JavaTckSupport.check(parameter != null
               && PARAMETER.equals(member.getParameterName(interactionClass, parameter)),
            "exact-case parameter lookup did not preserve its FOM name");
         JavaTckSupport.expectFailure(
            () -> member.getParameterHandle(
               interactionClass, firstLetterCaseVariant(PARAMETER)),
            "NameNotFound", "case-mismatched parameter name was accepted");
      } finally {
         if (joined) {
            try {
               member.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
            } catch (Exception ignored) {
            }
         }
         if (created) {
            try {
               member.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (connected) {
            try {
               member.disconnect();
            } catch (Exception ignored) {
            }
         }
      }
   }

   private static void runRootlessFom(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador member = factory.getRtiAmbassador();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-fom-rootless-name-case-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         member.connect(SupportServicesTck.noOpFederateAmbassador(), callbackModel);
         connected = true;
         member.createFederationExecution(federation, new String[] {fom}, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-fom-rootless-member-" + suffix, federation);
         joined = true;

         ObjectClassHandle objectRoot = member.getObjectClassHandle("HLAobjectRoot");
         InteractionClassHandle interactionRoot =
            member.getInteractionClassHandle("HLAinteractionRoot");
         ObjectClassHandle objectClass =
            member.getObjectClassHandle("HLAobjectRoot.TckObject");
         InteractionClassHandle interactionClass =
            member.getInteractionClassHandle("HLAinteractionRoot.TckInteraction");
         JavaTckSupport.check(objectRoot != null && objectRoot.encodedLength() > 0
               && interactionRoot != null && interactionRoot.encodedLength() > 0
               && objectClass != null && objectClass.encodedLength() > 0
               && interactionClass != null && interactionClass.encodedLength() > 0
               && "HLAobjectRoot".equals(member.getObjectClassName(objectRoot))
               && "HLAinteractionRoot".equals(member.getInteractionClassName(interactionRoot))
               && "HLAobjectRoot.TckObject".equals(member.getObjectClassName(objectClass))
               && "HLAinteractionRoot.TckInteraction".equals(
                  member.getInteractionClassName(interactionClass)),
            "FOMs omitting standard root declarations did not resolve full class paths");
      } finally {
         if (joined) {
            try {
               member.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
            } catch (Exception ignored) {
            }
         }
         if (created) {
            try {
               member.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (connected) {
            try {
               member.disconnect();
            } catch (Exception ignored) {
            }
         }
      }
   }

   private static String firstLetterCaseVariant(String name) {
      JavaTckSupport.check(name != null && !name.isEmpty(),
         "case-sensitivity fixture name was empty");
      char first = name.charAt(0);
      if (first >= 'A' && first <= 'Z') {
         first = (char) (first - 'A' + 'a');
      } else if (first >= 'a' && first <= 'z') {
         first = (char) (first - 'a' + 'A');
      } else {
         throw new AssertionError("FOM fixture name must begin with an ASCII letter: " + name);
      }
      return first + name.substring(1);
   }
}
