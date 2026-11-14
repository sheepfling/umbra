package org.hla.rti.tck;

/** Registers the Java public-API witness for the 2025 transport session handshake. */
final class TransportPublicNewSessionInteropScenarioRegistry {
   private static final String PEER_ADDRESS_PROPERTY =
      "hla.rti.tck.transportPeerAddress";

   private TransportPublicNewSessionInteropScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.transport-public-new-session-interoperability",
            "Exchange the 2025 New Session, heartbeat, and termination frames through the Java API",
            "connection",
            "RTIambassador.connect(FederateAmbassador, CallbackModel, RtiConfiguration);RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-402-l26-8",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-403-l4-1",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-403-l22-7",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-403-l25-8",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-403-l31-10",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-403-l52-17",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-403-l55-18",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-403-l70-23",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-403-l76-25",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-404-l9-2",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-404-l12-3",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-405-l7-1",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-405-l16-4",
               "requirement-candidate-content-clauses-12-programming-language-mappings-page-413-l51-16"
            }, false),
         TransportPublicNewSessionInteropScenarioRegistry::transportPublicNewSessionInteroperability);
   }

   static void transportPublicNewSessionInteroperability() throws Exception {
      TransportPublicNewSessionInteropTck.run(RtiTckMain.factory(),
         RtiTckMain.callbackModel(), System.getProperty(PEER_ADDRESS_PROPERTY));
   }
}
