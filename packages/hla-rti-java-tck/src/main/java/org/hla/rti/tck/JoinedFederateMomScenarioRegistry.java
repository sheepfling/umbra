package org.hla.rti.tck;

/** Owns Java TCK registrations for joined-federate MOM lifecycle cases. */
final class JoinedFederateMomScenarioRegistry {
   private JoinedFederateMomScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.joined-federate-mom-added-attribute-same-rti-instance",
            "acquire and update a FOM-added HLAfederate attribute on the RTI-registered instance",
            "joined-federate-mom-object-lifecycle",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.getKnownObjectClassHandle;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.queryAttributeOwnership;RTIambassador.attributeOwnershipAcquisition;RTIambassador.isAttributeOwnedByFederate;RTIambassador.updateAttributeValues;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues;FederateAmbassador.attributeIsNotOwned;FederateAmbassador.attributeIsOwnedByRTI;FederateAmbassador.attributeOwnershipAcquisitionNotification;HLAinteger32BE",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-288-l66-21",
               "requirement-candidate-content-clauses-10-support-services-page-288-l69-22"
            }, false), JoinedFederateMomAddedAttributeSameRtiInstanceTck::runFromAdapter);
   }
}
