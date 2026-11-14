package org.hla.rti.tck;

/** Registers the Java public-API mirror for transport session cardinality. */
final class TransportSessionCardinalityScenarioRegistry {
   private static final String PROXY_ADDRESS_PROPERTY =
      "hla.rti.tck.transportSessionProxyAddress";

   private TransportSessionCardinalityScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.one-transport-connection-per-federate-session",
            "Use exactly one TCP transport connection for each Java federate session",
            "connection",
            "RTIambassador.connect;RTIambassador.createFederationExecution;"
               + "RTIambassador.joinFederationExecution;RTIambassador.listFederationExecutions;"
               + "RTIambassador.getFederateHandle;RTIambassador.evokeMultipleCallbacks;"
               + "RTIambassador.getObjectClassHandle;"
               + "RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;"
               + "RTIambassador.disconnect;FederateAmbassador.reportFederationExecutions",
            new String[] {
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-412-l9-2",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-397-l114-37",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-397-l117-38",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-398-l87-28",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-399-l29-9",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-399-l53-17",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-399-l62-20",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-399-l59-19",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-404-l18-5",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-403-l49-16",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-405-l22-6",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-405-l28-8",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-406-l12-2",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-406-l30-8",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-406-l36-10",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-408-l32-10",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-408-l56-18",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-408-l47-15",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-408-l122-40",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-408-l65-21",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-408-l74-24",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-408-l86-28"
            }, false),
         TransportSessionCardinalityScenarioRegistry::oneTransportConnectionPerFederateSession);
   }

   static void oneTransportConnectionPerFederateSession() throws Exception {
      TransportSessionCardinalityTck.run(RtiTckMain.factory(), RtiTckMain.requireFom(),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel(),
         System.getProperty(PROXY_ADDRESS_PROPERTY));
   }
}
