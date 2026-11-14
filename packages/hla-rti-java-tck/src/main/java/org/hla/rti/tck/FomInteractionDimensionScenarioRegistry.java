package org.hla.rti.tck;

/** Owns Java TCK registrations for inherited interaction FDD dimensions. */
final class FomInteractionDimensionScenarioRegistry {
   private FomInteractionDimensionScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-interaction-inherited-available-dimensions",
            "expose inherited interaction-class dimensions through the public query",
            "fom-dimension-availability",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getAvailableDimensionsForInteractionClass;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-08-time-management-page-217-l119-38"
            }, false), FomInteractionInheritedAvailableDimensionsTck::runFromAdapter);
   }
}
