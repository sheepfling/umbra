package org.hla.rti.tck;

/** Owns scenario registration for Java DDM compatibility cases. */
final class RegionalDdmScenarioRegistry {
   private RegionalDdmScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.regional-object-attribute-dimension-subset",
            "reject object-attribute update regions containing dimensions unavailable to the instance attribute",
            "regional-object-attribute-dimension-subset",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getDimensionHandle;RTIambassador.getAvailableDimensionsForObjectClass;RTIambassador.getAttributeHandleSetFactory;RTIambassador.publishObjectClassAttributes;RTIambassador.getDimensionHandleSetFactory;RTIambassador.createRegion;RTIambassador.setRangeBounds;RTIambassador.getRegionHandleSetFactory;RTIambassador.commitRegionModifications;RTIambassador.getAttributeSetRegionSetPairListFactory;RTIambassador.registerObjectInstanceWithRegions;RTIambassador.associateRegionsForUpdates;RTIambassador.unassociateRegionsForUpdates;RTIambassador.unpublishObjectClassAttributes;RTIambassador.deleteObjectInstance;RTIambassador.deleteRegion;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-09-data-distribution-management-page-220-l8-2"
            }, false), RtiTckMain::regionalObjectAttributeDimensionSubset);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.regional-object-attribute-dimension-inheritance",
            "preserve FDD dimension associations for base and derived object attributes",
            "regional-object-management",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getDimensionHandle;RTIambassador.getAvailableDimensionsForObjectClass;RTIambassador.publishObjectClassAttributes;RTIambassador.getAttributeHandleSetFactory;RTIambassador.getDimensionHandleSetFactory;RTIambassador.createRegion;RTIambassador.getDimensionHandleSet;RTIambassador.setRangeBounds;RTIambassador.getRegionHandleSetFactory;RTIambassador.commitRegionModifications;RTIambassador.getAttributeSetRegionSetPairListFactory;RTIambassador.registerObjectInstanceWithRegions;RTIambassador.associateRegionsForUpdates;RTIambassador.subscribeObjectClassAttributesWithRegions;RTIambassador.requestAttributeValueUpdateWithRegions;RTIambassador.updateAttributeValues;RTIambassador.unassociateRegionsForUpdates;RTIambassador.unsubscribeObjectClassAttributesWithRegions;RTIambassador.unpublishObjectClassAttributes;RTIambassador.deleteRegion;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues",
            new String[] {
               "requirement-candidate-content-clauses-08-time-management-page-217-l107-34",
               "requirement-candidate-content-clauses-08-time-management-page-217-l131-42",
               "requirement-candidate-content-clauses-08-time-management-page-217-l110-35"
            }, false), RtiTckMain::regionalObjectAttributeDimensionInheritance);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.regional-object-attribute-default-region-fdd-dimensions",
            "derive object-attribute default-region routing from FDD dimensions and bounds",
            "regional-object-management",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getDimensionHandle;RTIambassador.getAvailableDimensionsForObjectClass;RTIambassador.getDimensionUpperBound;RTIambassador.publishObjectClassAttributes;RTIambassador.getAttributeHandleSetFactory;RTIambassador.getDimensionHandleSetFactory;RTIambassador.createRegion;RTIambassador.getDimensionHandleSet;RTIambassador.setRangeBounds;RTIambassador.getRegionHandleSetFactory;RTIambassador.commitRegionModifications;RTIambassador.getAttributeSetRegionSetPairListFactory;RTIambassador.subscribeObjectClassAttributesWithRegions;RTIambassador.registerObjectInstance;RTIambassador.updateAttributeValues;RTIambassador.unsubscribeObjectClassAttributesWithRegions;RTIambassador.unpublishObjectClassAttributes;RTIambassador.deleteRegion;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues",
            new String[] {
               "requirement-candidate-content-clauses-08-time-management-page-217-l101-32"
            }, false),
            RegionalObjectAttributeDimensionInheritanceTck::runDefaultRegionFddDimensionsFromAdapter);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.regional-object-attribute-unspecified-dimension-default-range",
            "apply the FDD default range to an omitted object-attribute region dimension",
            "regional-object-management",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getDimensionHandle;RTIambassador.getAvailableDimensionsForObjectClass;RTIambassador.getDimensionUpperBound;RTIambassador.publishObjectClassAttributes;RTIambassador.getAttributeHandleSetFactory;RTIambassador.getDimensionHandleSetFactory;RTIambassador.createRegion;RTIambassador.getDimensionHandleSet;RTIambassador.setRangeBounds;RTIambassador.getRegionHandleSetFactory;RTIambassador.commitRegionModifications;RTIambassador.getAttributeSetRegionSetPairListFactory;RTIambassador.subscribeObjectClassAttributesWithRegions;RTIambassador.registerObjectInstanceWithRegions;RTIambassador.updateAttributeValues;RTIambassador.unassociateRegionsForUpdates;RTIambassador.unsubscribeObjectClassAttributesWithRegions;RTIambassador.unpublishObjectClassAttributes;RTIambassador.deleteRegion;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues",
            new String[] {
               "requirement-candidate-content-clauses-08-time-management-page-217-l140-45"
            }, false),
            RegionalObjectAttributeDimensionInheritanceTck::runUnspecifiedDimensionDefaultRangeFromAdapter);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.regional-object-attribute-excluded-dimension-not-realized",
            "do not realize an omitted object-attribute region dimension marked Excluded",
            "regional-object-management",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getDimensionHandle;RTIambassador.getAvailableDimensionsForObjectClass;RTIambassador.getDimensionUpperBound;RTIambassador.publishObjectClassAttributes;RTIambassador.getAttributeHandleSetFactory;RTIambassador.getDimensionHandleSetFactory;RTIambassador.createRegion;RTIambassador.getDimensionHandleSet;RTIambassador.setRangeBounds;RTIambassador.getRegionHandleSetFactory;RTIambassador.commitRegionModifications;RTIambassador.getAttributeSetRegionSetPairListFactory;RTIambassador.subscribeObjectClassAttributesWithRegions;RTIambassador.registerObjectInstanceWithRegions;RTIambassador.updateAttributeValues;RTIambassador.unassociateRegionsForUpdates;RTIambassador.unsubscribeObjectClassAttributesWithRegions;RTIambassador.unpublishObjectClassAttributes;RTIambassador.deleteRegion;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues",
            new String[] {
               "requirement-candidate-content-clauses-08-time-management-page-217-l140-45"
            }, false),
            RegionalObjectAttributeDimensionInheritanceTck::runExcludedDimensionNotRealizedFromAdapter);
   }
}
