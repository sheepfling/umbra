package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionInformationSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** Exercises two Java public federate sessions through separate transport connections. */
final class TransportSessionCardinalityTck {
   private static final String HEARTBEAT_SETTINGS =
      "FED_INT_HEART=0.1;FED_TIMEOUT_HEART=2;FED_TIMEOUT_RECONNECT=0.25";

   private TransportSessionCardinalityTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel, String address) throws Exception {
      JavaTckSupport.check(address != null && address.startsWith("tcp://127.0.0.1:"),
         "the session-cardinality case requires its adapter-supplied loopback proxy");

      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder memberRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-session-cardinality-" + suffix;
      String ownerName = "java-tck-session-owner-" + suffix;
      String memberName = "java-tck-session-member-" + suffix;
      String federateType = "java-tck-session-cardinality";
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         ConfigurationResult ownerConnection = connect(owner, ownerRecorder.proxy(),
            callbackModel, address);
         ownerConnected = true;
         checkConnection(ownerConnection, "owner");
         ConfigurationResult memberConnection = connect(member, memberRecorder.proxy(),
            callbackModel, address);
         memberConnected = true;
         checkConnection(memberConnection, "member");

         // Let the adapter release the process peer after both New Session statuses;
         // this interval also gives the configured heartbeat path observable traffic.
         Thread.sleep(500L);

         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution(ownerName, federateType, federation);
         ownerJoined = true;
         member.joinFederationExecution(memberName, federateType, federation);
         memberJoined = true;
         JavaTckSupport.check(!owner.getFederateHandle(ownerName)
               .equals(owner.getFederateHandle(memberName)),
            "two RTIambassador sessions joined as the same federate");

         owner.listFederationExecutions();
         member.listFederationExecutions();
         awaitReport(owner, ownerRecorder, callbackModel, federation, timeImplementation,
            "owner federation execution report");
         awaitReport(member, memberRecorder, callbackModel, federation, timeImplementation,
            "member federation execution report");
      } finally {
         if (memberJoined) resign(member);
         if (ownerJoined) resign(owner);
         if (created) {
            try {
               owner.destroyFederationExecution(federation);
            } catch (Exception ignored) { }
         }
         if (memberConnected) disconnect(member);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static ConfigurationResult connect(RTIambassador ambassador,
         FederateAmbassador federate, CallbackModel callbackModel, String address)
         throws Exception {
      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withRtiAddress(address)
         .withAdditionalSettings(HEARTBEAT_SETTINGS);
      return ambassador.connect(federate, callbackModel, configuration);
   }

   private static void checkConnection(ConfigurationResult result, String role) {
      JavaTckSupport.check(result != null, role + " public Connect returned null");
      JavaTckSupport.check(result.addressUsed,
         role + " Connect did not use the adapter-supplied TCP endpoint");
      JavaTckSupport.check(result.additionalSettingsResultCode
            == hla.rti1516_2025.AdditionalSettingsResultCode.SETTINGS_APPLIED,
         role + " Connect did not apply the bounded heartbeat settings");
   }

   private static void awaitReport(RTIambassador ambassador, Recorder recorder,
         CallbackModel callbackModel, String federation, String timeImplementation,
         String description) throws Exception {
      for (int attempt = 0; attempt < 1000
            && !recorder.listsFederation(federation, timeImplementation); attempt++) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            ambassador.evokeMultipleCallbacks(0.0, 0.01);
         } else {
            ambassador.getObjectClassHandle("HLAobjectRoot");
         }
         Thread.sleep(10L);
      }
      JavaTckSupport.check(recorder.listsFederation(federation, timeImplementation),
         "timed out waiting for the " + description);
   }

   private static void resign(RTIambassador ambassador) {
      try {
         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
      } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try {
         ambassador.disconnect();
      } catch (Exception ignored) { }
   }

   private static final class Recorder {
      private final List<FederationExecutionInformationSet> federationReports =
         new ArrayList<>();

      private boolean listsFederation(String federation, String timeImplementation) {
         for (FederationExecutionInformationSet report : federationReports) {
            for (FederationExecutionInformation information : report) {
               if (federation.equals(information.federationExecutionName)
                     && timeImplementation.equals(information.logicalTimeImplementationName)) {
                  return true;
               }
            }
         }
         return false;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("reportFederationExecutions".equals(name)
                     && arguments != null && arguments.length == 1) {
                  federationReports.add(
                     ((FederationExecutionInformationSet) arguments[0]).clone());
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK session-cardinality callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null
                     ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
