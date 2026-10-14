package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
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

/** Portable coverage for the generated-name federation join overload. */
final class UnnamedJoinOverloadTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 5_000_000_000L;

   private UnnamedJoinOverloadTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel, String federateType) throws Exception {
      Member owner = new Member(factory.getRtiAmbassador());
      Member namedMember = new Member(factory.getRtiAmbassador());
      Member observer = new Member(factory.getRtiAmbassador());
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-unnamed-join-" + suffix;
      String namedFederate = "java-tck-named-member-" + suffix;
      boolean created = false;
      try {
         owner.connect(callbackModel);
         namedMember.connect(callbackModel);
         observer.connect(callbackModel);

         owner.rti.createFederationExecution(federation, fom, timeImplementation);
         created = true;

         FederateHandle generatedHandle = owner.rti.joinFederationExecution(
            federateType, federation);
         owner.joined = true;
         String generatedName = owner.rti.getFederateName(generatedHandle);
         JavaTckSupport.check(generatedName != null && !generatedName.isEmpty(),
            "unnamed join did not assign a federate name");
         JavaTckSupport.check(generatedHandle.equals(owner.rti.getFederateHandle(generatedName)),
            "generated federate name did not resolve to the join handle");

         FederateHandle namedHandle = namedMember.rti.joinFederationExecution(
            namedFederate, federateType, federation);
         namedMember.joined = true;
         JavaTckSupport.check(namedHandle != null
               && !generatedHandle.equals(namedHandle)
               && namedHandle.equals(namedMember.rti.getFederateHandle(namedFederate)),
            "named control join did not retain a distinct reversible identity");

         observer.rti.listFederationExecutionMembers(federation);
         BooleanSupplier completeReport = () -> observer.callbacks.hasMembers(
            federation, generatedName, federateType, namedFederate, federateType);
         verifyCallbackBoundary(observer, callbackModel, completeReport);

         namedMember.rti.resignFederationExecution(ResignAction.NO_ACTION);
         namedMember.joined = false;
         owner.rti.resignFederationExecution(ResignAction.NO_ACTION);
         owner.joined = false;
         owner.rti.destroyFederationExecution(federation);
         created = false;
      } finally {
         namedMember.close();
         owner.close();
         if (created) {
            try { owner.rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         observer.close();
      }
   }

   private static void verifyCallbackBoundary(Member member, CallbackModel model,
         BooleanSupplier completeReport) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.check(!completeReport.getAsBoolean(),
            "evoked federation-member report arrived before callback servicing");
      } else {
         JavaTckSupport.check(completeReport.getAsBoolean(),
            "immediate federation-member report missed the service boundary");
      }
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!completeReport.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            member.rti.evokeMultipleCallbacks(0.0, 0.01);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(completeReport.getAsBoolean(),
         "timed out waiting for the complete federation-member report");
   }

   private static final class Member {
      private final RTIambassador rti;
      private final Recorder callbacks = new Recorder();
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
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
            connected = false;
         }
      }
   }

   private static final class Recorder {
      private final List<MemberReport> reports = new CopyOnWriteArrayList<>();

      private boolean hasMembers(String federation, String generatedName,
            String generatedType, String namedName, String namedType) {
         for (MemberReport report : reports) {
            if (federation.equals(report.federation) && report.members.size() == 2
                  && report.has(generatedName, generatedType)
                  && report.has(namedName, namedType)) return true;
         }
         return false;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("reportFederationExecutionMembers".equals(method.getName())
                     && arguments != null && arguments.length == 2) {
                  reports.add(new MemberReport((String) arguments[0],
                     ((FederationExecutionMemberInformationSet) arguments[1]).clone()));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java TCK unnamed-join callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }

   private static final class MemberReport {
      private final String federation;
      private final FederationExecutionMemberInformationSet members;

      private MemberReport(String federation, FederationExecutionMemberInformationSet members) {
         this.federation = federation;
         this.members = members;
      }

      private boolean has(String name, String type) {
         for (FederationExecutionMemberInformation member : members) {
            if (name.equals(member.federateName) && type.equals(member.federateType)) return true;
         }
         return false;
      }
   }
}
