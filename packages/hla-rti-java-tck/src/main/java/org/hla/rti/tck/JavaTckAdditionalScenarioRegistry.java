package org.hla.rti.tck;

/** Aggregates registrations kept outside the central Java TCK runner. */
final class JavaTckAdditionalScenarioRegistry {
   private JavaTckAdditionalScenarioRegistry() { }

   static void register() {
      SupportServicesScenarioRegistry.register();
      RegionalDdmScenarioRegistry.register();
      RegionalInteractionMixedAbsentUnsubscribeScenarioRegistry.register();
      RegionalInteractionDefaultRangeScenarioRegistry.register();
      FomInteractionDimensionScenarioRegistry.register();
      FomHandleUniquenessScenarioRegistry.register();
      JoinedFederateMomScenarioRegistry.register();
      LogicalTimeDataElementsScenarioRegistry.register();
      FederationTimeScenarioRegistry.register();
      StartRegistrationAdvisorySwitchGatingScenarioRegistry.register();
   }
}
