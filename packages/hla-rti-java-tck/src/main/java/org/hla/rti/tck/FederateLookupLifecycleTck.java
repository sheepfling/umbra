package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Federate handle/name lookup boundaries across connection, membership, and resignation. */
final class FederateLookupLifecycleTck {
   private FederateLookupLifecycleTck() { }

   static void run(RtiFactory factory, String fom, CallbackModel callbackModel,
         String federateType) throws Exception {
      Member unjoined = new Member(factory.getRtiAmbassador());
      Member owner = new Member(factory.getRtiAmbassador());
      Member peer = new Member(factory.getRtiAmbassador());
      Member foreignCreator = new Member(factory.getRtiAmbassador());
      Member foreignMember = new Member(factory.getRtiAmbassador());
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-federate-lookup-" + suffix;
      String foreignFederation = federation + "-foreign";
      String ownerName = "java-tck-owner-" + suffix;
      String peerName = "java-tck-peer-" + suffix;
      String foreignName = peerName + "-foreign";
      try {
         JavaTckSupport.expectFailure(
            () -> unjoined.rti.getFederateHandle(ownerName), "NotConnected",
            "getFederateHandle before connect");
         JavaTckSupport.expectFailure(
            () -> unjoined.rti.getFederateName(JavaTckSupport.invalidHandle(
               FederateHandle.class)), "NotConnected", "getFederateName before connect");

         unjoined.connect(callbackModel);
         JavaTckSupport.expectFailure(
            () -> unjoined.rti.getFederateHandle(ownerName), "FederateNotExecutionMember",
            "getFederateHandle before join");
         JavaTckSupport.expectFailure(
            () -> unjoined.rti.getFederateName(JavaTckSupport.invalidHandle(
               FederateHandle.class)), "FederateNotExecutionMember",
            "getFederateName before join");

         owner.connect(callbackModel);
         peer.connect(callbackModel);
         foreignCreator.connect(callbackModel);
         foreignMember.connect(callbackModel);
         owner.rti.createFederationExecution(federation, fom);
         owner.createdFederation = federation;
         foreignCreator.rti.createFederationExecution(foreignFederation, fom);
         foreignCreator.createdFederation = foreignFederation;
         FederateHandle ownerHandle = owner.rti.joinFederationExecution(
            ownerName, federateType, federation);
         owner.joined = true;
         FederateHandle peerHandle = peer.rti.joinFederationExecution(
            peerName, federateType, federation);
         peer.joined = true;
         FederateHandle foreignHandle = foreignMember.rti.joinFederationExecution(
            foreignName, federateType, foreignFederation);
         foreignMember.joined = true;

         JavaTckSupport.check(ownerHandle.equals(owner.rti.getFederateHandle(ownerName)),
            "owner federate name-to-handle lookup failed to round-trip");
         JavaTckSupport.check(peerHandle.equals(owner.rti.getFederateHandle(peerName)),
            "peer federate name-to-handle lookup failed to round-trip");
         JavaTckSupport.check(ownerName.equals(peer.rti.getFederateName(ownerHandle)),
            "owner federate handle-to-name lookup failed to round-trip");
         JavaTckSupport.check(peerName.equals(peer.rti.getFederateName(peerHandle)),
            "peer federate handle-to-name lookup failed to round-trip");
         JavaTckSupport.expectFailure(
            () -> owner.rti.getFederateHandle("java-tck-missing-federate-" + suffix),
            "NameNotFound", "getFederateHandle for an unknown federate name");
         JavaTckSupport.expectFailure(
            () -> owner.rti.getFederateName(JavaTckSupport.invalidHandle(
               FederateHandle.class)), "InvalidFederateHandle",
            "getFederateName for an invalid federate handle");
         JavaTckSupport.expectFailure(
            () -> owner.rti.getFederateName(foreignHandle), "FederateHandleNotKnown",
            "getFederateName for a handle from another federation");

         peer.rti.resignFederationExecution(ResignAction.NO_ACTION);
         peer.joined = false;
         JavaTckSupport.expectFailure(
            () -> owner.rti.getFederateHandle(peerName), "NameNotFound",
            "getFederateHandle after the federate resigns");
         JavaTckSupport.check(peerName.equals(owner.rti.getFederateName(peerHandle)),
            "departed federate handle did not retain its designator identity");

         foreignMember.rti.resignFederationExecution(ResignAction.NO_ACTION);
         foreignMember.joined = false;
         owner.rti.resignFederationExecution(ResignAction.NO_ACTION);
         owner.joined = false;
         owner.rti.destroyFederationExecution(federation);
         owner.createdFederation = null;
         foreignCreator.rti.destroyFederationExecution(foreignFederation);
         foreignCreator.createdFederation = null;
      } finally {
         unjoined.close();
         peer.close();
         foreignMember.close();
         owner.close();
         foreignCreator.close();
      }
   }

   private static FederateAmbassador emptyFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK federate lookup callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static final class Member {
      private final RTIambassador rti;
      private boolean connected;
      private boolean joined;
      private String createdFederation;

      private Member(RTIambassador rti) {
         this.rti = rti;
      }

      private void connect(CallbackModel model) throws Exception {
         rti.connect(emptyFederateAmbassador(), model);
         connected = true;
      }

      private void close() {
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
            joined = false;
         }
         if (createdFederation != null) {
            try { rti.destroyFederationExecution(createdFederation); }
            catch (Exception ignored) { }
            createdFederation = null;
         }
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
            connected = false;
         }
      }
   }
}
