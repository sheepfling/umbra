package hla.rti1516_2025;

import hla.rti1516_2025.exceptions.AlreadyConnected;
import hla.rti1516_2025.exceptions.NotConnected;
import hla.rti1516_2025.auth.Credentials;

/** Test-fixture subset of the standard ambassador interface. */
public interface RTIambassador {
   ConfigurationResult connect(FederateAmbassador federateAmbassador, CallbackModel callbackModel)
      throws AlreadyConnected;

   ConfigurationResult connect(
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration configuration) throws AlreadyConnected;

   ConfigurationResult connect(
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel,
      Credentials credentials) throws AlreadyConnected;

   ConfigurationResult connect(
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration configuration,
      Credentials credentials) throws AlreadyConnected;

   void disconnect() throws NotConnected;

   boolean evokeCallback(double approximateMinimumTimeInSeconds) throws NotConnected;

   boolean evokeMultipleCallbacks(
      double approximateMinimumTimeInSeconds,
      double approximateMaximumTimeInSeconds) throws NotConnected;

   void enableCallbacks() throws NotConnected;

   void disableCallbacks() throws NotConnected;

   void listFederationExecutions() throws NotConnected;

   void createFederationExecution(
      String federationName,
      String fomModule,
      String logicalTimeImplementationName) throws NotConnected;

   void destroyFederationExecution(String federationName) throws NotConnected;
}
