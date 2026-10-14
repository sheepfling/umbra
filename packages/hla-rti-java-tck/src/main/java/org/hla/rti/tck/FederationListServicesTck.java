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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Portable federation-listing service and callback-boundary contract. */
final class FederationListServicesTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;

   private FederationListServicesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      Member observer = new Member(factory.getRtiAmbassador());
      Member owner = new Member(factory.getRtiAmbassador());
      Member member = new Member(factory.getRtiAmbassador());
      Member stale = new Member(factory.getRtiAmbassador());
      String suffix = UUID.randomUUID().toString();
      String firstFederation = "java-tck-list-primary-" + suffix;
      String secondFederation = "java-tck-list-secondary-" + suffix;
      String missingFederation = "java-tck-list-missing-" + suffix;
      String ownerName = "java-tck-list-owner-" + suffix;
      String memberName = "java-tck-list-member-" + suffix;
      String federateType = "java-tck-list-type";
      try {
         JavaTckSupport.expectFailure(() -> observer.rti.listFederationExecutions(),
            "NotConnected", "list federation executions before connect");
         JavaTckSupport.expectFailure(
            () -> observer.rti.listFederationExecutionMembers(firstFederation),
            "NotConnected", "list federation members before connect");

         observer.connect(callbackModel);
         owner.connect(callbackModel);
         member.connect(callbackModel);
         stale.connect(callbackModel);
         owner.rti.createFederationExecution(firstFederation, fom, timeImplementation);
         owner.createdFederations.add(firstFederation);
         owner.rti.createFederationExecution(secondFederation, fom, timeImplementation);
         owner.createdFederations.add(secondFederation);
         owner.rti.joinFederationExecution(ownerName, federateType, firstFederation);
         owner.joined = true;
         member.rti.joinFederationExecution(memberName, federateType, firstFederation);
         member.joined = true;

         observer.rti.listFederationExecutions();
         verifyBoundary(observer, callbackModel,
            () -> observer.callbacks.hasFederation(firstFederation, timeImplementation)
               && observer.callbacks.hasFederation(secondFederation, timeImplementation),
            "federation execution report");

         observer.rti.listFederationExecutionMembers(firstFederation);
         verifyBoundary(observer, callbackModel,
            () -> observer.callbacks.hasMembers(firstFederation, ownerName, federateType,
               memberName, federateType), "federation member report");

         observer.rti.listFederationExecutionMembers(missingFederation);
         verifyBoundary(observer, callbackModel,
            () -> observer.callbacks.reportsMissing(missingFederation),
            "federation-does-not-exist report");

         stale.rti.listFederationExecutions();
         boolean staleReportDelivered = stale.callbacks.hasFederation(
            firstFederation, timeImplementation);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(!staleReportDelivered,
               "evoked federation report arrived before callback servicing");
         } else {
            JavaTckSupport.check(staleReportDelivered,
               "immediate federation report did not arrive at the service boundary");
         }
         stale.close();
         JavaTckSupport.check(
            stale.callbacks.hasFederation(firstFederation, timeImplementation)
               == (callbackModel == CallbackModel.HLA_IMMEDIATE),
            "disconnect changed the federation report callback boundary");

         member.rti.resignFederationExecution(ResignAction.NO_ACTION);
         member.joined = false;
         drain(member, callbackModel);
         JavaTckSupport.check(member.callbacks.federateResignedCount == 0,
            "federate-initiated resignation synthesized a federateResigned callback");
         observer.rti.listFederationExecutionMembers(firstFederation);
         verifyBoundary(observer, callbackModel,
            () -> observer.callbacks.hasMembers(firstFederation, ownerName, federateType),
            "member report after resignation");

         owner.rti.resignFederationExecution(ResignAction.NO_ACTION);
         owner.joined = false;
         owner.rti.destroyFederationExecution(firstFederation);
         owner.createdFederations.remove(firstFederation);
         owner.rti.destroyFederationExecution(secondFederation);
         owner.createdFederations.remove(secondFederation);
         observer.rti.listFederationExecutionMembers(firstFederation);
         verifyBoundary(observer, callbackModel,
            () -> observer.callbacks.reportsMissing(firstFederation),
            "destroyed-federation report");
      } finally {
         stale.close();
         member.close();
         owner.close();
         observer.close();
      }
   }

   private static void verifyBoundary(Member member, CallbackModel model,
         BooleanSupplier delivered, String description) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.check(!delivered.getAsBoolean(),
            description + " arrived before the evoke boundary");
      } else {
         JavaTckSupport.check(delivered.getAsBoolean(),
            description + " was not delivered at the immediate callback boundary");
      }
      await(member, model, delivered, description);
   }

   private static void await(Member member, CallbackModel model,
         BooleanSupplier delivered, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!delivered.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            member.rti.evokeMultipleCallbacks(0.0, 0.01);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(delivered.getAsBoolean(),
         "timed out waiting for " + description);
   }

   private static void drain(Member member, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         member.rti.evokeMultipleCallbacks(0.0, 0.01);
      }
   }

   private static final class Member {
      private final RTIambassador rti;
      private final Recorder callbacks = new Recorder();
      private final List<String> createdFederations = new CopyOnWriteArrayList<>();
      private boolean connected;
      private boolean joined;

      private Member(RTIambassador rti) { this.rti = rti; }

      private void connect(CallbackModel model) throws Exception {
         rti.connect(callbacks.proxy(), model);
         connected = true;
      }

      private void close() {
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
            joined = false;
         }
         for (String federation : createdFederations) {
            try { rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         createdFederations.clear();
         if (connected) {
            try { rti.disconnect(); } catch (Exception ignored) { }
            connected = false;
         }
      }
   }

   private static final class Recorder {
      private final List<FederationExecutionInformationSet> federationReports =
         new CopyOnWriteArrayList<>();
      private final List<MemberReport> memberReports = new CopyOnWriteArrayList<>();
      private final List<String> missingFederations = new CopyOnWriteArrayList<>();
      private volatile int federateResignedCount;

      private boolean hasFederation(String federation, String timeImplementation) {
         for (FederationExecutionInformationSet report : federationReports) {
            for (FederationExecutionInformation info : report) {
               if (federation.equals(info.federationExecutionName)
                     && timeImplementation.equals(info.logicalTimeImplementationName)) return true;
            }
         }
         return false;
      }

      private boolean hasMembers(String federation, String... expected) {
         for (MemberReport report : memberReports) {
            if (!federation.equals(report.federation) || report.members.size() != expected.length / 2) {
               continue;
            }
            boolean complete = true;
            for (int i = 0; i < expected.length; i += 2) {
               if (!containsMember(report.members, expected[i], expected[i + 1])) {
                  complete = false;
                  break;
               }
            }
            if (complete) return true;
         }
         return false;
      }

      private boolean reportsMissing(String federation) {
         return missingFederations.contains(federation);
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("reportFederationExecutions".equals(name) && arguments != null
                     && arguments.length == 1) {
                  federationReports.add(((FederationExecutionInformationSet) arguments[0]).clone());
               } else if ("reportFederationExecutionMembers".equals(name) && arguments != null
                     && arguments.length == 2) {
                  memberReports.add(new MemberReport((String) arguments[0],
                     ((FederationExecutionMemberInformationSet) arguments[1]).clone()));
               } else if ("reportFederationExecutionDoesNotExist".equals(name)
                     && arguments != null && arguments.length == 1) {
                  missingFederations.add((String) arguments[0]);
               } else if ("federateResigned".equals(name)) {
                  federateResignedCount++;
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK federation list callbacks";
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
