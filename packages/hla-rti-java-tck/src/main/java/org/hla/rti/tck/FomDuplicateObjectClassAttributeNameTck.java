package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.nio.file.Path;
import java.util.UUID;

/** Rejects the focused FOM fixture that duplicates an attribute name. */
final class FomDuplicateObjectClassAttributeNameTck {
   private static final String FIXTURE_NAME = "invalid-duplicate-tck.xml";
   private static final String[] EXPECTED_FOM_ERRORS = {
      "CouldNotOpenFOM", "ErrorReadingFOM", "InvalidFOM", "InconsistentFOM"
   };

   private FomDuplicateObjectClassAttributeNameTck() { }

   static void run(RtiFactory factory, String[] invalidFomModules,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "duplicate-object-attribute FOM check requires an adapter logical-time name");
      String duplicateAttributeFom = null;
      if (invalidFomModules != null) {
         for (String module : invalidFomModules) {
            if (module != null && FIXTURE_NAME.equals(Path.of(module).getFileName().toString())) {
               duplicateAttributeFom = module;
               break;
            }
         }
      }
      JavaTckSupport.check(duplicateAttributeFom != null,
         "adapter did not supply the duplicate-object-attribute FOM fixture");

      RTIambassador rti = factory.getRtiAmbassador();
      boolean connected = false;
      boolean created = false;
      String federation = "java-tck-fom-duplicate-object-attribute-" + UUID.randomUUID();
      try {
         rti.connect(new NullFederateAmbassador(), callbackModel);
         connected = true;
         try {
            rti.createFederationExecution(federation,
               new String[] {duplicateAttributeFom}, timeImplementation);
            created = true;
            throw new AssertionError(
               "FOM that repeats an attribute name within one object class was accepted");
         } catch (Exception error) {
            String actual = error.getClass().getSimpleName();
            boolean expected = false;
            for (String expectedName : EXPECTED_FOM_ERRORS) {
               if (expectedName.equals(actual)) {
                  expected = true;
                  break;
               }
            }
            JavaTckSupport.check(expected,
               "duplicate object-class attribute FOM produced unexpected exception " + actual);
         }
      } finally {
         if (created) {
            try { rti.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (connected) {
            try { rti.disconnect(); } catch (Exception ignored) { }
         }
      }
   }
}
