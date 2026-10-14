package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** Callback enable/disable behavior for standard transportation reports and confirmations. */
final class CallbackControlsTransportationTck {
   private CallbackControlsTransportationTck() {
   }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String interactionClassName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder observerRecorder = new Recorder();
      String federation = "java-tck-callback-transport-" + UUID.randomUUID();
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
         owner.createFederationExecution(federation, fom);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            "java-tck-callback-transport-owner", federation);
         ownerJoined = true;
         observer.joinFederationExecution("java-tck-callback-transport-observer", federation);
         observerJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle observerClass = observer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle observerAttribute = observer.getAttributeHandle(observerClass,
            attributeName);
         InteractionClassHandle ownerInteraction =
            owner.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle observerInteraction =
            observer.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(ownerClass.equals(observerClass)
               && ownerAttribute.equals(observerAttribute)
               && ownerInteraction.equals(observerInteraction),
            "standard FOM lookups did not resolve matching transportation-test handles");

         TransportationTypeHandle reliable = owner.getTransportationTypeHandle("HLAreliable");
         TransportationTypeHandle bestEffort =
            owner.getTransportationTypeHandle("HLAbestEffort");
         JavaTckSupport.check(reliable != null && bestEffort != null
               && !reliable.equals(bestEffort)
               && "HLAreliable".equals(owner.getTransportationTypeName(reliable))
               && "HLAbestEffort".equals(owner.getTransportationTypeName(bestEffort)),
            "standard transportation type handle/name lookup was inconsistent");

         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet observerAttributes =
            JavaTckSupport.attributeSet(observer, observerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes);
         owner.publishInteractionClass(ownerInteraction);
         observer.subscribeInteractionClass(observerInteraction);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         await(owner, callbackModel, objectClassName,
            () -> observerRecorder.discoveries.contains(object),
            "observer object discovery");

         owner.queryAttributeTransportationType(object, ownerAttribute);
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.attributeReports.size() == 1,
            "baseline attribute transportation report");
         checkAttributeReport(owner, ownerRecorder.attributeReports.get(0), object,
            ownerAttribute, reliable);

         ownerRecorder.attributeReports.clear();
         owner.disableCallbacks();
         owner.queryAttributeTransportationType(object, ownerAttribute);
         service(owner, callbackModel, objectClassName);
         JavaTckSupport.check(ownerRecorder.attributeReports.isEmpty(),
            "disabled callbacks exposed an attribute transportation report");
         owner.enableCallbacks();
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.attributeReports.size() == 1,
            "re-enabled attribute transportation report");
         checkAttributeReport(owner, ownerRecorder.attributeReports.get(0), object,
            ownerAttribute, reliable);

         ownerRecorder.attributeConfirmations.clear();
         owner.disableCallbacks();
         owner.requestAttributeTransportationTypeChange(object, ownerAttributes, bestEffort);
         service(owner, callbackModel, objectClassName);
         JavaTckSupport.check(ownerRecorder.attributeConfirmations.isEmpty(),
            "disabled callbacks exposed an attribute transportation confirmation");
         owner.enableCallbacks();
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.attributeConfirmations.size() == 1,
            "re-enabled attribute transportation confirmation");
         AttributeTransportConfirmation attributeConfirmation =
            ownerRecorder.attributeConfirmations.get(0);
         JavaTckSupport.check(object.equals(attributeConfirmation.object)
               && attributeConfirmation.attributes.size() == 1
               && attributeConfirmation.attributes.contains(ownerAttribute)
               && sameTransport(owner, attributeConfirmation.transportation, bestEffort),
            "released attribute transportation confirmation returned incorrect metadata");

         ownerRecorder.attributeReports.clear();
         owner.queryAttributeTransportationType(object, ownerAttribute);
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.attributeReports.size() == 1,
            "committed attribute transportation report");
         JavaTckSupport.check(sameTransport(owner,
               ownerRecorder.attributeReports.get(0).transportation, bestEffort),
            "attribute transportation confirmation did not commit the requested type");

         ownerRecorder.interactionReports.clear();
         owner.queryInteractionTransportationType(ownerFederate, ownerInteraction);
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.interactionReports.size() == 1,
            "baseline interaction transportation report");
         checkInteractionReport(owner, ownerRecorder.interactionReports.get(0), ownerFederate,
            ownerInteraction, reliable);

         ownerRecorder.interactionReports.clear();
         owner.disableCallbacks();
         owner.queryInteractionTransportationType(ownerFederate, ownerInteraction);
         service(owner, callbackModel, objectClassName);
         JavaTckSupport.check(ownerRecorder.interactionReports.isEmpty(),
            "disabled callbacks exposed an interaction transportation report");
         owner.enableCallbacks();
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.interactionReports.size() == 1,
            "re-enabled interaction transportation report");
         checkInteractionReport(owner, ownerRecorder.interactionReports.get(0), ownerFederate,
            ownerInteraction, reliable);

         ownerRecorder.interactionConfirmations.clear();
         owner.disableCallbacks();
         owner.requestInteractionTransportationTypeChange(ownerInteraction, bestEffort);
         service(owner, callbackModel, objectClassName);
         JavaTckSupport.check(ownerRecorder.interactionConfirmations.isEmpty(),
            "disabled callbacks exposed an interaction transportation confirmation");
         owner.enableCallbacks();
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.interactionConfirmations.size() == 1,
            "re-enabled interaction transportation confirmation");
         InteractionTransportConfirmation interactionConfirmation =
            ownerRecorder.interactionConfirmations.get(0);
         JavaTckSupport.check(ownerInteraction.equals(interactionConfirmation.interaction)
               && sameTransport(owner, interactionConfirmation.transportation, bestEffort),
            "released interaction transportation confirmation returned incorrect metadata");

         ownerRecorder.interactionReports.clear();
         owner.queryInteractionTransportationType(ownerFederate, ownerInteraction);
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.interactionReports.size() == 1,
            "committed interaction transportation report");
         JavaTckSupport.check(sameTransport(owner,
               ownerRecorder.interactionReports.get(0).transportation, bestEffort),
            "interaction transportation confirmation did not commit the requested type");
      } finally {
         if (observerJoined) {
            try { observer.unsubscribeInteractionClass(observer.getInteractionClassHandle(
               interactionClassName)); } catch (Exception ignored) { }
            try {
               observer.unsubscribeObjectClassAttributes(
                  observer.getObjectClassHandle(objectClassName),
                  JavaTckSupport.attributeSet(observer,
                     observer.getAttributeHandle(observer.getObjectClassHandle(objectClassName),
                        attributeName)));
            } catch (Exception ignored) { }
            resign(observer);
         }
         if (ownerJoined) {
            try { owner.unpublishInteractionClass(owner.getInteractionClassHandle(
               interactionClassName)); } catch (Exception ignored) { }
            try {
               owner.unpublishObjectClassAttributes(
                  owner.getObjectClassHandle(objectClassName),
                  JavaTckSupport.attributeSet(owner,
                     owner.getAttributeHandle(owner.getObjectClassHandle(objectClassName),
                        attributeName)));
            } catch (Exception ignored) { }
            resign(owner);
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      for (int attempt = 0; attempt < 100 && !condition.isTrue(); attempt++) {
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

   private static void checkAttributeReport(RTIambassador ambassador,
         AttributeTransportReport report, ObjectInstanceHandle object,
         AttributeHandle attribute, TransportationTypeHandle expected) throws Exception {
      JavaTckSupport.check(object.equals(report.object) && attribute.equals(report.attribute)
            && sameTransport(ambassador, report.transportation, expected),
         "attribute transportation report returned incorrect metadata");
   }

   private static void checkInteractionReport(RTIambassador ambassador,
         InteractionTransportReport report, FederateHandle federate,
         InteractionClassHandle interaction, TransportationTypeHandle expected) throws Exception {
      JavaTckSupport.check(federate.equals(report.federate)
            && interaction.equals(report.interaction)
            && sameTransport(ambassador, report.transportation, expected),
         "interaction transportation report returned incorrect metadata");
   }

   private static boolean sameTransport(RTIambassador ambassador,
         TransportationTypeHandle actual, TransportationTypeHandle expected) throws Exception {
      return expected.equals(actual)
         && ambassador.getTransportationTypeName(actual)
            .equals(ambassador.getTransportationTypeName(expected));
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private interface Condition {
      boolean isTrue();
   }

   private static final class AttributeTransportReport {
      private final ObjectInstanceHandle object;
      private final AttributeHandle attribute;
      private final TransportationTypeHandle transportation;
      private AttributeTransportReport(ObjectInstanceHandle object, AttributeHandle attribute,
            TransportationTypeHandle transportation) {
         this.object = object;
         this.attribute = attribute;
         this.transportation = transportation;
      }
   }

   private static final class AttributeTransportConfirmation {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final TransportationTypeHandle transportation;
      private AttributeTransportConfirmation(ObjectInstanceHandle object,
            AttributeHandleSet attributes, TransportationTypeHandle transportation) {
         this.object = object;
         this.attributes = attributes;
         this.transportation = transportation;
      }
   }

   private static final class InteractionTransportReport {
      private final FederateHandle federate;
      private final InteractionClassHandle interaction;
      private final TransportationTypeHandle transportation;
      private InteractionTransportReport(FederateHandle federate,
            InteractionClassHandle interaction, TransportationTypeHandle transportation) {
         this.federate = federate;
         this.interaction = interaction;
         this.transportation = transportation;
      }
   }

   private static final class InteractionTransportConfirmation {
      private final InteractionClassHandle interaction;
      private final TransportationTypeHandle transportation;
      private InteractionTransportConfirmation(InteractionClassHandle interaction,
            TransportationTypeHandle transportation) {
         this.interaction = interaction;
         this.transportation = transportation;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      private final List<AttributeTransportReport> attributeReports = new ArrayList<>();
      private final List<AttributeTransportConfirmation> attributeConfirmations =
         new ArrayList<>();
      private final List<InteractionTransportReport> interactionReports = new ArrayList<>();
      private final List<InteractionTransportConfirmation> interactionConfirmations =
         new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reportAttributeTransportationType".equals(name)) {
                  attributeReports.add(new AttributeTransportReport(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandle) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               } else if ("confirmAttributeTransportationTypeChange".equals(name)) {
                  attributeConfirmations.add(new AttributeTransportConfirmation(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandleSet) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               } else if ("reportInteractionTransportationType".equals(name)) {
                  interactionReports.add(new InteractionTransportReport(
                     (FederateHandle) arguments[0], (InteractionClassHandle) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               } else if ("confirmInteractionTransportationTypeChange".equals(name)) {
                  interactionConfirmations.add(new InteractionTransportConfirmation(
                     (InteractionClassHandle) arguments[0],
                     (TransportationTypeHandle) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK transportation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
