package org.hla.rti.tck;

/** Registers federation-selected logical-time factory scenarios. */
final class FederationTimeScenarioRegistry {
   private FederationTimeScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.federation-supplied-custom-tradt",
            "select and exercise a federation-supplied two-kind TRADT factory",
            "time",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect;LogicalTimeFactoryFactory.getLogicalTimeFactory;RTIambassador.createFederationExecution(fomModules,logicalTimeImplementation);RTIambassador.joinFederationExecution;RTIambassador.getTimeFactory;LogicalTimeFactory.getName;HLAinteger64TimeFactory.makeInitial;HLAinteger64Time.isInitial;HLAinteger64Time.getValue;HLAinteger64TimeFactory.makeTime;HLAinteger64Time.encodedLength;HLAinteger64Time.encode;HLAinteger64TimeFactory.decodeTime;HLAinteger64Time.equals;RTIambassador.queryLogicalTime;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-07-ownership-management-page-175-l140-43"
            }, false), FederationTimeScenarioRegistry::federationSuppliedCustomTradt);
   }

   static void federationSuppliedCustomTradt() throws Exception {
      FederationSuppliedCustomTradtTck.run(RtiTckMain.factory(), RtiTckMain.requireFom(),
         RtiTckMain.callbackModel());
   }
}
