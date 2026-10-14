package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleSet;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.LinkedHashSet;
import java.util.Set;
import java.util.UUID;
import java.util.function.BooleanSupplier;

/** Standard service-boundary checks after a member resigns. */
final class PostResignationServiceBoundariesTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private PostResignationServiceBoundariesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      DiscoveryRecorder recorder = new DiscoveryRecorder();
      String federation = "java-tck-post-resign-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         member.connect(recorder.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle ownerHandle = owner.joinFederationExecution(
            "java-tck-post-resign-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-post-resign-member", federation);
         memberJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle memberClass = member.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle memberAttribute = member.getAttributeHandle(memberClass, attributeName);
         InteractionClassHandle memberInteraction = member.getInteractionClassHandle(
            interactionClassName);
         ParameterHandle memberParameter = member.getParameterHandle(
            memberInteraction, parameterName);
         check(ownerClass != null && memberClass != null && ownerAttribute != null
               && memberAttribute != null && memberInteraction != null && memberParameter != null,
            "post-resignation setup returned a null standard handle");

         AttributeHandleSet ownerAttributes = owner.getAttributeHandleSetFactory().create();
         ownerAttributes.add(ownerAttribute);
         AttributeHandleSet memberAttributes = member.getAttributeHandleSetFactory().create();
         memberAttributes.add(memberAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         member.subscribeObjectClassAttributes(memberClass, memberAttributes, "");
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         await(member, callbackModel, () -> recorder.discovered == object,
            "pre-resignation object discovery");
         String objectName = member.getObjectInstanceName(object);
         check(objectName != null && !objectName.isEmpty()
               && object.equals(member.getObjectInstanceHandle(objectName))
               && memberClass.equals(member.getKnownObjectClassHandle(object)),
            "pre-resignation object identity did not round-trip");

         hla.rti1516_2025.TransportationTypeHandle reliable =
            member.getTransportationTypeHandle("HLAreliable");
         LogicalTimeFactory<?, ?> timeFactory = member.getTimeFactory();
         LogicalTime<?, ?> initial = (LogicalTime<?, ?>) timeFactory.makeInitial();
         LogicalTimeInterval<?> zero = (LogicalTimeInterval<?>) timeFactory.makeZero();
         LogicalTimeInterval<?> epsilon = (LogicalTimeInterval<?>) timeFactory.makeEpsilon();
         byte[] emptyTag = new byte[0];
         AttributeHandleValueMap attributeValues =
            member.getAttributeHandleValueMapFactory().create(1);
         attributeValues.put(memberAttribute, new byte[] {0x50, 0x52});
         ParameterHandleValueMap parameterValues =
            member.getParameterHandleValueMapFactory().create(1);
         parameterValues.put(memberParameter, new byte[] {0x50, 0x41});
         FederateHandleSet synchronizationSet =
            member.getFederateHandleSetFactory().create();
         synchronizationSet.add(ownerHandle);
         Set<String> reservedNames = new LinkedHashSet<>();
         reservedNames.add(federation + "-post-resignation-a");
         reservedNames.add(federation + "-post-resignation-b");

         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;

         expectNotMember(() -> member.getFederateHandle("java-tck-post-resign-owner"),
            "federate name lookup");
         expectNotMember(() -> member.getFederateName(ownerHandle), "federate handle lookup");
         expectNotMember(() -> member.getObjectClassHandle(objectClassName),
            "object-class handle lookup");
         expectNotMember(() -> member.getObjectClassName(memberClass), "object-class name lookup");
         expectNotMember(() -> member.getKnownObjectClassHandle(object), "known class lookup");
         expectNotMember(() -> member.getObjectInstanceHandle(objectName), "object handle lookup");
         expectNotMember(() -> member.getObjectInstanceName(object), "object name lookup");
         expectNotMember(() -> member.getAttributeHandle(memberClass, attributeName),
            "attribute handle lookup");
         expectNotMember(() -> member.getAttributeName(memberClass, memberAttribute),
            "attribute name lookup");
         expectNotMember(() -> member.getInteractionClassHandle(interactionClassName),
            "interaction handle lookup");
         expectNotMember(() -> member.getInteractionClassName(memberInteraction),
            "interaction name lookup");
         expectNotMember(() -> member.getParameterHandle(memberInteraction, parameterName),
            "parameter handle lookup");
         expectNotMember(() -> member.getParameterName(memberInteraction, memberParameter),
            "parameter name lookup");
         expectNotMember(() -> member.getAvailableDimensionsForObjectClass(memberClass),
            "object-class dimensions lookup");
         expectNotMember(() -> member.getAvailableDimensionsForInteractionClass(memberInteraction),
            "interaction-class dimensions lookup");
         expectNotMember(() -> member.getDimensionHandle(dimensionName), "dimension lookup");
         expectNotMember(() -> member.getOrderType("Receive"), "order lookup");
         expectNotMember(() -> member.getOrderName(OrderType.RECEIVE), "order name lookup");
         expectNotMember(() -> member.getTransportationTypeHandle("HLAreliable"),
            "transportation lookup");
         expectNotMember(() -> member.getTransportationTypeName(reliable),
            "transportation name lookup");

         expectNotMember(() -> member.publishObjectClassAttributes(memberClass, memberAttributes),
            "object-attribute publication");
         expectNotMember(() -> member.unpublishObjectClassAttributes(memberClass, memberAttributes),
            "object-attribute unpublication");
         expectNotMember(() -> member.unpublishObjectClass(memberClass), "object-class unpublication");
         expectNotMember(() -> member.subscribeObjectClassAttributes(memberClass, memberAttributes,
            ""), "object-attribute subscription");
         expectNotMember(() -> member.unsubscribeObjectClassAttributes(memberClass, memberAttributes),
            "object-attribute unsubscription");
         expectNotMember(() -> member.unsubscribeObjectClass(memberClass),
            "object-class unsubscription");
         expectNotMember(() -> member.publishInteractionClass(memberInteraction),
            "interaction publication");
         expectNotMember(() -> member.unpublishInteractionClass(memberInteraction),
            "interaction unpublication");
         expectNotMember(() -> member.subscribeInteractionClass(memberInteraction),
            "interaction subscription");
         expectNotMember(() -> member.unsubscribeInteractionClass(memberInteraction),
            "interaction unsubscription");

         String singleName = federation + "-post-resignation-single";
         expectNotMember(() -> member.reserveObjectInstanceName(singleName), "name reservation");
         expectNotMember(() -> member.releaseObjectInstanceName(singleName), "name release");
         expectNotMember(() -> member.reserveMultipleObjectInstanceNames(reservedNames),
            "multiple name reservation");
         expectNotMember(() -> member.releaseMultipleObjectInstanceNames(reservedNames),
            "multiple name release");
         expectNotMember(() -> member.registerObjectInstance(memberClass), "object registration");
         expectNotMember(() -> member.registerObjectInstance(memberClass, singleName),
            "named object registration");
         expectNotMember(() -> member.updateAttributeValues(object, attributeValues, emptyTag),
            "attribute update");
         expectNotMember(() -> member.requestAttributeValueUpdate(object, memberAttributes, emptyTag),
            "object attribute-value request");
         expectNotMember(() -> member.requestAttributeValueUpdate(memberClass, memberAttributes,
            emptyTag), "class attribute-value request");
         expectNotMember(() -> member.deleteObjectInstance(object, emptyTag), "object deletion");
         expectNotMember(() -> member.localDeleteObjectInstance(object), "local object deletion");
         expectNotMember(() -> member.sendInteraction(memberInteraction, parameterValues, emptyTag),
            "interaction send");
         expectNotMember(() -> member.sendDirectedInteraction(memberInteraction, object,
            parameterValues, emptyTag), "directed interaction send");

         expectNotMember(() -> member.queryAttributeOwnership(object, memberAttributes),
            "attribute ownership query");
         expectNotMember(() -> member.isAttributeOwnedByFederate(object, memberAttribute),
            "attribute ownership status");
         expectNotMember(() -> member.enableAsynchronousDelivery(), "asynchronous-delivery enable");
         expectNotMember(() -> member.disableAsynchronousDelivery(), "asynchronous-delivery disable");
         expectNotMember(() -> member.enableTimeConstrained(), "time-constrained enable");
         expectNotMember(() -> member.disableTimeConstrained(), "time-constrained disable");
         expectNotMember(() -> member.enableTimeRegulation(zero), "time-regulation enable");
         expectNotMember(() -> member.disableTimeRegulation(), "time-regulation disable");
         expectNotMember(() -> member.modifyLookahead(epsilon), "lookahead modification");
         expectNotMember(() -> member.queryLookahead(), "lookahead query");
         expectNotMember(() -> member.queryLogicalTime(), "logical-time query");
         expectNotMember(() -> member.queryGALT(), "GALT query");
         expectNotMember(() -> member.queryLITS(), "LITS query");
         expectNotMember(() -> member.timeAdvanceRequest(initial), "time-advance request");
         expectNotMember(() -> member.timeAdvanceRequestAvailable(initial),
            "available time-advance request");
         expectNotMember(() -> member.nextMessageRequest(initial), "next-message request");
         expectNotMember(() -> member.nextMessageRequestAvailable(initial),
            "available next-message request");
         expectNotMember(() -> member.flushQueueRequest(initial), "flush-queue request");

         expectNotMember(() -> member.registerFederationSynchronizationPoint(
            federation + "-post-resignation-global", emptyTag), "global synchronization registration");
         expectNotMember(() -> member.registerFederationSynchronizationPoint(
            federation + "-post-resignation-explicit", emptyTag, synchronizationSet),
            "explicit synchronization registration");
         expectNotMember(() -> member.synchronizationPointAchieved(
            federation + "-post-resignation-achieved"), "synchronization achievement");
         expectNotMember(() -> member.requestFederationSave(
            federation + "-post-resignation-save"), "untimed federation save request");
         expectNotMember(() -> member.requestFederationSave(
            federation + "-post-resignation-timed-save", initial), "timed federation save request");

         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (memberJoined) resign(member);
         if (ownerJoined) resign(owner);
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (memberConnected) disconnect(member);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void expectNotMember(JavaTckSupport.CheckedOperation operation,
         String description) throws Exception {
      JavaTckSupport.expectFailure(operation, "FederateNotExecutionMember",
         description + " after resignation");
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) throw new AssertionError("timed out waiting for " + description);
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.01);
         else Thread.sleep(1L);
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> null);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static void check(boolean condition, String message) {
      if (!condition) throw new AssertionError(message);
   }

   private static final class DiscoveryRecorder {
      private volatile ObjectInstanceHandle discovered;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               if ("discoverObjectInstance".equals(method.getName()) && args != null
                     && args.length >= 1) discovered = (ObjectInstanceHandle) args[0];
               return null;
            });
      }
   }
}
