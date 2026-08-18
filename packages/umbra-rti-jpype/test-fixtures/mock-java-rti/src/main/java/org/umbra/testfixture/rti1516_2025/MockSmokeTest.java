package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederationExecutionInformationSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.RtiFactoryFactory;
import hla.rti1516_2025.exceptions.AlreadyConnected;

public final class MockSmokeTest {
   private MockSmokeTest() {
   }

   public static void main(String[] arguments) throws Exception {
      RtiFactory factory = RtiFactoryFactory.getRtiFactory(MockRtiFactory.NAME);
      RTIambassador ambassador = factory.getRtiAmbassador();
      RecordingFederateAmbassador callbacks = new RecordingFederateAmbassador();
      ConfigurationResult result = ambassador.connect(callbacks, CallbackModel.HLA_EVOKED);
      if (result.configurationUsed || result.addressUsed || !"2025.mock".equals(factory.rtiVersion())) {
         throw new AssertionError("Mock factory did not return its expected configuration result");
      }
      try {
         ambassador.connect(callbacks, CallbackModel.HLA_EVOKED);
         throw new AssertionError("Expected AlreadyConnected");
      } catch (AlreadyConnected expected) {
         // Expected state-transition check.
      }
      MockRTIambassador mockAmbassador = (MockRTIambassador) ambassador;
      mockAmbassador.queueConnectionLost("mock connection loss");
      if (!ambassador.evokeCallback(0.0) || !"mock connection loss".equals(callbacks.description)) {
         throw new AssertionError("Mock callback was not delivered");
      }
      ambassador.disconnect();
      System.out.println("Mock Java RTI smoke test: OK");
   }

   private static final class RecordingFederateAmbassador implements FederateAmbassador {
      private String description;

      @Override
      public void connectionLost(String faultDescription) {
         description = faultDescription;
      }

      @Override
      public void reportFederationExecutions(FederationExecutionInformationSet report) {
         // The Java-only smoke test covers the connection callback; Python
         // integration tests exercise the typed federation report callback.
      }
   }
}
