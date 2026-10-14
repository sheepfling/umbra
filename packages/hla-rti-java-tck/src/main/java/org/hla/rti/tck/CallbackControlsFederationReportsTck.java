package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionInformationSet;
import hla.rti1516_2025.FederationExecutionMemberInformation;
import hla.rti1516_2025.FederationExecutionMemberInformationSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** Callback enable/disable behavior for standard federation listing reports. */
final class CallbackControlsFederationReportsTck {
   private CallbackControlsFederationReportsTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-callback-reports-" + UUID.randomUUID();
      String missingFederation = federation + "-missing";
      String ownerName = "java-tck-callback-reports-owner";
      String observerName = "java-tck-callback-reports-observer";
      String ownerType = "callback-reports-owner";
      String observerType = "callback-reports-observer";
      boolean ownerConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean observerJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         observer.connect(observerRecorder.proxy(), callbackModel);
         observerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution(ownerName, ownerType, federation);
         ownerJoined = true;
         observer.joinFederationExecution(observerName, observerType, federation);
         observerJoined = true;

         observer.disableCallbacks();
         observer.listFederationExecutions();
         service(observer, callbackModel, objectClassName);
         JavaTckSupport.check(!observerRecorder.listsFederation(federation,
               timeImplementation),
            "disabled callbacks exposed a federation execution report");
         observer.enableCallbacks();
         await(observer, callbackModel, objectClassName,
            () -> observerRecorder.listsFederation(federation, timeImplementation),
            "re-enabled federation execution report");

         observer.disableCallbacks();
         observer.listFederationExecutionMembers(federation);
         service(observer, callbackModel, objectClassName);
         JavaTckSupport.check(!observerRecorder.listsMembers(federation,
               ownerName, ownerType, observerName, observerType),
            "disabled callbacks exposed a federation member report");
         observer.enableCallbacks();
         await(observer, callbackModel, objectClassName,
            () -> observerRecorder.listsMembers(federation,
               ownerName, ownerType, observerName, observerType),
            "re-enabled federation member report");

         observer.disableCallbacks();
         observer.listFederationExecutionMembers(missingFederation);
         service(observer, callbackModel, objectClassName);
         JavaTckSupport.check(!observerRecorder.reportsMissing(missingFederation),
            "disabled callbacks exposed a federation-does-not-exist report");
         observer.enableCallbacks();
         await(observer, callbackModel, objectClassName,
            () -> observerRecorder.reportsMissing(missingFederation),
            "re-enabled federation-does-not-exist report");
      } finally {
         if (observerJoined) resign(observer);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition,
         String description) throws Exception {
      for (int attempt = 0; attempt < 1000 && !condition.isTrue(); attempt++) {
         service(ambassador, model, objectClassName);
         Thread.sleep(10);
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, CallbackModel model,
         String objectClassName) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         ambassador.evokeMultipleCallbacks(0.0, 0.01);
      } else {
         ambassador.getObjectClassHandle(objectClassName);
      }
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private interface Condition {
      boolean isTrue();
   }

   private static final class Recorder {
      private final List<FederationExecutionInformationSet> federationReports =
         new ArrayList<>();
      private final List<MemberReport> memberReports = new ArrayList<>();
      private final List<String> missingFederations = new ArrayList<>();

      private boolean listsFederation(String federation, String timeImplementation) {
         for (FederationExecutionInformationSet report : federationReports) {
            for (FederationExecutionInformation info : report) {
               if (federation.equals(info.federationExecutionName)
                     && timeImplementation.equals(info.logicalTimeImplementationName)) {
                  return true;
               }
            }
         }
         return false;
      }

      private boolean listsMembers(String federation, String ownerName, String ownerType,
            String observerName, String observerType) {
         for (MemberReport report : memberReports) {
            if (!federation.equals(report.federation) || report.members.size() != 2) continue;
            if (containsMember(report.members, ownerName, ownerType)
                  && containsMember(report.members, observerName, observerType)) {
               return true;
            }
         }
         return false;
      }

      private boolean reportsMissing(String federation) {
         return missingFederations.contains(federation);
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
               } else if ("reportFederationExecutionMembers".equals(name)
                     && arguments != null && arguments.length == 2) {
                  memberReports.add(new MemberReport((String) arguments[0],
                     ((FederationExecutionMemberInformationSet) arguments[1]).clone()));
               } else if ("reportFederationExecutionDoesNotExist".equals(name)
                     && arguments != null && arguments.length == 1) {
                  missingFederations.add((String) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK federation report callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private static boolean containsMember(FederationExecutionMemberInformationSet members,
            String name, String type) {
         for (FederationExecutionMemberInformation member : members) {
            if (name.equals(member.federateName) && type.equals(member.federateType)) return true;
         }
         return false;
      }
   }

   private static final class MemberReport {
      private final String federation;
      private final FederationExecutionMemberInformationSet members;
      private MemberReport(String federation, FederationExecutionMemberInformationSet members) {
         this.federation = federation;
         this.members = members;
      }
   }
}
