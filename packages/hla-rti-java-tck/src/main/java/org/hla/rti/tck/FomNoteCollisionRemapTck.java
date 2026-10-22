package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.RtiFactory;

/** Annex C.9 note-label collision through the public composed FDD value. */
final class FomNoteCollisionRemapTck {
   private FomNoteCollisionRemapTck() { }

   static void run(RtiFactory factory, String first, String second, String mim,
         String timeImplementation, CallbackModel model, String federateType)
         throws Exception {
      JavaTckSupport.check(!first.equals(second),
         "note-collision remap requires distinct adapter FOM modules");
      for (String[] modules : new String[][] {{first, second}, {second, first}}) {
         String currentFdd = FomServiceUsageMergeTck.composedCurrentFdd(
            factory, modules, mim, timeImplementation, model, federateType);
         FederationMomCurrentFddTck.requireDistinctReferencedNotes(currentFdd);
      }
   }
}
