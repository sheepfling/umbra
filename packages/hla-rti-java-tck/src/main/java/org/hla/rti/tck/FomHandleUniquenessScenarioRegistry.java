package org.hla.rti.tck;

/** Registers the Java counterpart of the public FOM-handle uniqueness witness. */
final class FomHandleUniquenessScenarioRegistry {
   private FomHandleUniquenessScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-handle-uniqueness",
            "assign distinct handles and repeat FDD-derived values across executions",
            "support-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.getDimensionHandle;RTIambassador.getTransportationTypeHandle;RTIambassador.getOrderType;ObjectClassHandle.encodedLength;ObjectClassHandle.encode;AttributeHandle.encodedLength;AttributeHandle.encode;InteractionClassHandle.encodedLength;InteractionClassHandle.encode;ParameterHandle.encodedLength;ParameterHandle.encode;DimensionHandle.encodedLength;DimensionHandle.encode;TransportationTypeHandle.encodedLength;TransportationTypeHandle.encode;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-243-l56-15",
               "requirement-candidate-content-clauses-11-management-object-model-page-353-l84-27",
               "requirement-candidate-content-clauses-11-management-object-model-page-353-l90-29",
               "requirement-candidate-content-clauses-11-management-object-model-page-353-l99-32"
            }, true), FomHandleUniquenessTck::run);
   }
}
