package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.AdditionalSettingsResultCode;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionInformationSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.auth.Credentials;
import hla.rti1516_2025.exceptions.AlreadyConnected;
import hla.rti1516_2025.exceptions.NotConnected;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Stateful fixture ambassador. It exposes ``queueConnectionLost`` solely so a
 * test can cause a callback through the normal public evoke operation.
 */
public final class MockRTIambassador implements RTIambassador {
   private FederateAmbassador federateAmbassador;
   private boolean connected;
   private boolean callbacksEnabled = true;
   private String queuedConnectionLoss;
   private FederationExecutionInformationSet queuedFederationExecutionReport;
   private CallbackModel callbackModel;
   private final Map<String, String> federationExecutions = new LinkedHashMap<>();

   public MockRTIambassador() {
      federationExecutions.put("Umbra Mock Federation", "HLAinteger64Time");
   }

   @Override
   public ConfigurationResult connect(
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel) throws AlreadyConnected
   {
      return connect(federateAmbassador, callbackModel, null, null);
   }

   @Override
   public ConfigurationResult connect(
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration configuration) throws AlreadyConnected
   {
      return connect(federateAmbassador, callbackModel, configuration, null);
   }

   @Override
   public ConfigurationResult connect(
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel,
      Credentials credentials) throws AlreadyConnected
   {
      return connect(federateAmbassador, callbackModel, null, credentials);
   }

   @Override
   public ConfigurationResult connect(
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration configuration,
      Credentials credentials) throws AlreadyConnected
   {
      if (connected) {
         throw new AlreadyConnected("already connected");
      }
      this.federateAmbassador = federateAmbassador;
      this.callbackModel = callbackModel;
      connected = true;
      callbacksEnabled = true;
      return new ConfigurationResult(
         false,
         false,
         AdditionalSettingsResultCode.SETTINGS_IGNORED,
         "connected through the mock Java RTI using " + callbackModel.name());
   }

   @Override
   public void disconnect() throws NotConnected {
      requireConnected();
      connected = false;
      federateAmbassador = null;
      queuedConnectionLoss = null;
      queuedFederationExecutionReport = null;
   }

   @Override
   public boolean evokeCallback(double approximateMinimumTimeInSeconds) throws NotConnected {
      requireConnected();
      if (callbacksEnabled && queuedConnectionLoss != null) {
         String description = queuedConnectionLoss;
         queuedConnectionLoss = null;
         federateAmbassador.connectionLost(description);
         return true;
      }
      if (callbacksEnabled && queuedFederationExecutionReport != null) {
         FederationExecutionInformationSet report = queuedFederationExecutionReport;
         queuedFederationExecutionReport = null;
         federateAmbassador.reportFederationExecutions(report);
         return true;
      }
      return false;
   }

   @Override
   public boolean evokeMultipleCallbacks(
      double approximateMinimumTimeInSeconds,
      double approximateMaximumTimeInSeconds) throws NotConnected
   {
      return evokeCallback(approximateMinimumTimeInSeconds);
   }

   @Override
   public void enableCallbacks() throws NotConnected {
      requireConnected();
      callbacksEnabled = true;
   }

   @Override
   public void disableCallbacks() throws NotConnected {
      requireConnected();
      callbacksEnabled = false;
   }

   public void queueConnectionLost(String description) throws NotConnected {
      requireConnected();
      queuedConnectionLoss = description;
   }

   @Override
   public void listFederationExecutions() throws NotConnected {
      requireConnected();
      MockFederationExecutionInformationSet report = new MockFederationExecutionInformationSet();
      for (Map.Entry<String, String> entry : federationExecutions.entrySet()) {
         report.add(new FederationExecutionInformation(entry.getKey(), entry.getValue()));
      }
      if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
         federateAmbassador.reportFederationExecutions(report);
      } else {
         queuedFederationExecutionReport = report;
      }
   }

   @Override
   public void createFederationExecution(
      String federationName,
      String fomModule,
      String logicalTimeImplementationName) throws NotConnected
   {
      requireConnected();
      federationExecutions.put(federationName, logicalTimeImplementationName);
   }

   @Override
   public void destroyFederationExecution(String federationName) throws NotConnected {
      requireConnected();
      federationExecutions.remove(federationName);
   }

   private void requireConnected() throws NotConnected {
      if (!connected) {
         throw new NotConnected("not connected");
      }
   }
}
