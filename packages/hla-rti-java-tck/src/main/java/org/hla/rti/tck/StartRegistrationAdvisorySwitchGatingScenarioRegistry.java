package org.hla.rti.tck;

/** Registers the portable object-class relevance switch gate for Start/Stop Registration. */
final class StartRegistrationAdvisorySwitchGatingScenarioRegistry {
   private StartRegistrationAdvisorySwitchGatingScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.start-registration-advisory-switch-gating",
            "Gate Start and Stop Registration callbacks on the object-class relevance switch",
            "declaration-relevance",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;"
               + "RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getDimensionHandle;"
               + "RTIambassador.getAvailableDimensionsForObjectClass;RTIambassador.getDimensionName;"
               + "RTIambassador.getDimensionUpperBound;RTIambassador.getObjectClassRelevanceAdvisorySwitch;"
               + "RTIambassador.setObjectClassRelevanceAdvisorySwitch;RTIambassador.getAttributeHandleSetFactory;"
               + "RTIambassador.getRegionHandleSetFactory;RTIambassador.getAttributeSetRegionSetPairListFactory;"
               + "RTIambassador.createRegion;RTIambassador.setRangeBounds;RTIambassador.commitRegionModifications;"
               + "RTIambassador.publishObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;"
               + "RTIambassador.subscribeObjectClassAttributesWithRegions;"
               + "RTIambassador.unsubscribeObjectClassAttributesWithRegions;RTIambassador.deleteRegion;"
               + "FederateAmbassador.startRegistrationForObjectClass;"
               + "FederateAmbassador.stopRegistrationForObjectClass",
            new String[] {
               "requirement-candidate-content-clauses-05-declaration-management-page-103-l117-31",
               "requirement-candidate-content-clauses-05-declaration-management-page-103-l132-36",
               "requirement-candidate-content-clauses-05-declaration-management-page-103-l135-37",
               "requirement-candidate-content-clauses-05-declaration-management-page-104-l77-19",
               "requirement-candidate-content-clauses-05-declaration-management-page-104-l92-24",
               "requirement-candidate-content-clauses-05-declaration-management-page-104-l95-25",
               "requirement-candidate-content-clauses-10-support-services-page-244-l17-5",
               "requirement-candidate-content-clauses-10-support-services-page-244-l5-1"
            }, false), RtiTckMain::startRegistrationAdvisorySwitchGating);
   }
}
