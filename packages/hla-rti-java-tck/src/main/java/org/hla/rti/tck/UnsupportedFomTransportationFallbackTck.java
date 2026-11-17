package org.hla.rti.tck;

import hla.rti1516_2025.AdditionalSettingsResultCode;
import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.lang.reflect.Proxy;
import java.util.HashSet;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies public transportation fallback after an adapter establishes unavailable FOM types. */
final class UnsupportedFomTransportationFallbackTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final long SETTLE_NANOS = 40_000_000L;

   private UnsupportedFomTransportationFallbackTck() { }

   static void run(RtiFactory factory, String[] fomModules, String objectClassName,
         String attributeName, String interactionName, String bestEffortCustomName,
         String reliableCustomName, String timeImplementation, String additionalSettings,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fomModules != null && fomModules.length > 1,
         "adapter must provide a model FOM and an extension FOM");
      JavaTckSupport.check(additionalSettings != null && !additionalSettings.isEmpty(),
         "adapter must provide additional settings for unavailable FOM transportation types");

      RTIambassador owner = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder();
      String federation = "java-tck-unsupported-transport-fallback-" + UUID.randomUUID();
      String federateName = "java-tck-unsupported-transport-fallback-owner";
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         RtiConfiguration configuration = RtiConfiguration.createConfiguration()
            .withAdditionalSettings(additionalSettings);
         ConfigurationResult configurationResult =
            owner.connect(callbacks.proxy(), callbackModel, configuration);
         connected = true;
         JavaTckSupport.check(configurationResult != null
               && configurationResult.additionalSettingsResultCode
                  == AdditionalSettingsResultCode.SETTINGS_APPLIED,
            "adapter settings did not establish the unavailable-transport precondition");

         owner.createFederationExecution(federation, fomModules, timeImplementation);
         created = true;
         FederateHandle ownerFederate = owner.joinFederationExecution(
            federateName, "JavaTck", federation);
         joined = true;

         ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
         InteractionClassHandle interaction = owner.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(objectClassName.equals(owner.getObjectClassName(objectClass))
               && attributeName.equals(owner.getAttributeName(objectClass, attribute))
               && interactionName.equals(owner.getInteractionClassName(interaction)),
            "adapter FOM class or member names did not round-trip through the standard API");

         TransportationTypeHandle bestEffortCustom =
            owner.getTransportationTypeHandle(bestEffortCustomName);
         TransportationTypeHandle reliableCustom =
            owner.getTransportationTypeHandle(reliableCustomName);
         TransportationTypeHandle bestEffort =
            owner.getTransportationTypeHandle("HLAbestEffort");
         TransportationTypeHandle reliable = owner.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(bestEffortCustom != null && reliableCustom != null
               && bestEffort != null && reliable != null
               && !bestEffortCustom.equals(reliableCustom)
               && !bestEffortCustom.equals(bestEffort)
               && !bestEffortCustom.equals(reliable)
               && !reliableCustom.equals(bestEffort)
               && !reliableCustom.equals(reliable)
               && !bestEffort.equals(reliable)
               && bestEffortCustomName.equals(owner.getTransportationTypeName(bestEffortCustom))
               && reliableCustomName.equals(owner.getTransportationTypeName(reliableCustom))
               && "HLAbestEffort".equals(owner.getTransportationTypeName(bestEffort))
               && "HLAreliable".equals(owner.getTransportationTypeName(reliable)),
            "standard and custom transportation handles did not round-trip distinctly");

         AttributeHandleSet attributes = JavaTckSupport.attributeSet(owner, attribute);
         owner.publishObjectClassAttributes(objectClass, attributes);
         owner.publishInteractionClass(interaction);
         ObjectInstanceHandle object = owner.registerObjectInstance(objectClass);
         JavaTckSupport.check(object != null,
            "fallback attribute test could not register its object");

         queryAttribute(owner, callbacks, callbackModel, object, attribute, bestEffort,
            "HLAbestEffort", "FOM-declared unavailable best-effort attribute default");
         queryInteraction(owner, callbacks, callbackModel, ownerFederate, interaction,
            bestEffort, "HLAbestEffort",
            "FOM-declared unavailable best-effort interaction default");

         callbacks.attributeConfirmations.clear();
         owner.requestAttributeTransportationTypeChange(object, attributes, reliableCustom);
         await(owner, callbackModel,
            () -> !callbacks.attributeConfirmations.isEmpty(),
            "unsupported reliable attribute transportation confirmation");
         settle(owner, callbackModel);
         JavaTckSupport.check(callbacks.attributeConfirmations.size() == 1,
            "attribute transportation change returned an unexpected confirmation count");
         AttributeConfirmation attributeConfirmation = callbacks.attributeConfirmations.get(0);
         JavaTckSupport.check(object.equals(attributeConfirmation.object)
               && attributeConfirmation.attributes.size() == 1
               && attributeConfirmation.attributes.contains(attribute),
            "attribute transportation confirmation changed its object or attribute set");
         checkTransportation(owner, attributeConfirmation.transportation, reliable,
            "unsupported reliable attribute transport did not fall back to HLAreliable");
         queryAttribute(owner, callbacks, callbackModel, object, attribute, reliable,
            "HLAreliable", "post-fallback attribute transportation query");

         callbacks.interactionConfirmations.clear();
         owner.requestInteractionTransportationTypeChange(interaction, reliableCustom);
         await(owner, callbackModel,
            () -> !callbacks.interactionConfirmations.isEmpty(),
            "unsupported reliable interaction transportation confirmation");
         settle(owner, callbackModel);
         JavaTckSupport.check(callbacks.interactionConfirmations.size() == 1,
            "interaction transportation change returned an unexpected confirmation count");
         InteractionConfirmation interactionConfirmation =
            callbacks.interactionConfirmations.get(0);
         JavaTckSupport.check(interaction.equals(interactionConfirmation.interaction),
            "interaction transportation confirmation changed its interaction class");
         checkTransportation(owner, interactionConfirmation.transportation, reliable,
            "unsupported reliable interaction transport did not fall back to HLAreliable");
         queryInteraction(owner, callbacks, callbackModel, ownerFederate, interaction,
            reliable, "HLAreliable", "post-fallback interaction transportation query");
      } finally {
         if (joined) {
            try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void queryAttribute(RTIambassador owner, Recorder callbacks,
         CallbackModel callbackModel, ObjectInstanceHandle object, AttributeHandle attribute,
         TransportationTypeHandle expected, String expectedName, String description)
         throws Exception {
      callbacks.attributeReports.clear();
      owner.queryAttributeTransportationType(object, attribute);
      await(owner, callbackModel, () -> !callbacks.attributeReports.isEmpty(), description);
      settle(owner, callbackModel);
      JavaTckSupport.check(callbacks.attributeReports.size() == 1,
         description + " returned an unexpected report count");
      AttributeReport report = callbacks.attributeReports.get(0);
      JavaTckSupport.check(object.equals(report.object) && attribute.equals(report.attribute),
         description + " returned the wrong object or attribute");
      checkTransportation(owner, report.transportation, expected,
         description + " reported the wrong effective transportation type");
      JavaTckSupport.check(expectedName.equals(owner.getTransportationTypeName(report.transportation)),
         description + " reported the wrong transportation name");
   }

   private static void queryInteraction(RTIambassador owner, Recorder callbacks,
         CallbackModel callbackModel, FederateHandle ownerFederate,
         InteractionClassHandle interaction, TransportationTypeHandle expected,
         String expectedName, String description) throws Exception {
      callbacks.interactionReports.clear();
      owner.queryInteractionTransportationType(ownerFederate, interaction);
      await(owner, callbackModel, () -> !callbacks.interactionReports.isEmpty(), description);
      settle(owner, callbackModel);
      JavaTckSupport.check(callbacks.interactionReports.size() == 1,
         description + " returned an unexpected report count");
      InteractionReport report = callbacks.interactionReports.get(0);
      JavaTckSupport.check(ownerFederate.equals(report.federate)
            && interaction.equals(report.interaction),
         description + " returned the wrong federate or interaction");
      checkTransportation(owner, report.transportation, expected,
         description + " reported the wrong effective transportation type");
      JavaTckSupport.check(expectedName.equals(owner.getTransportationTypeName(report.transportation)),
         description + " reported the wrong transportation name");
   }

   private static void checkTransportation(RTIambassador owner,
         TransportationTypeHandle actual, TransportationTypeHandle expected, String message)
         throws Exception {
      JavaTckSupport.check(actual != null && expected.equals(actual)
            && owner.getTransportationTypeName(actual).equals(
               owner.getTransportationTypeName(expected)), message);
   }

   private static void await(RTIambassador owner, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         service(owner, callbackModel);
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for " + description);
   }

   private static void settle(RTIambassador owner, CallbackModel callbackModel)
         throws Exception {
      long deadline = System.nanoTime() + SETTLE_NANOS;
      while (System.nanoTime() < deadline) {
         service(owner, callbackModel);
      }
   }

   private static void service(RTIambassador owner, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         owner.evokeMultipleCallbacks(0.0, 0.01);
      } else {
         Thread.sleep(1L);
      }
   }

   private static final class AttributeReport {
      private final ObjectInstanceHandle object;
      private final AttributeHandle attribute;
      private final TransportationTypeHandle transportation;

      private AttributeReport(ObjectInstanceHandle object, AttributeHandle attribute,
            TransportationTypeHandle transportation) {
         this.object = object;
         this.attribute = attribute;
         this.transportation = transportation;
      }
   }

   private static final class AttributeConfirmation {
      private final ObjectInstanceHandle object;
      private final Set<AttributeHandle> attributes;
      private final TransportationTypeHandle transportation;

      private AttributeConfirmation(ObjectInstanceHandle object, AttributeHandleSet attributes,
            TransportationTypeHandle transportation) {
         this.object = object;
         this.attributes = new HashSet<>(attributes);
         this.transportation = transportation;
      }
   }

   private static final class InteractionReport {
      private final FederateHandle federate;
      private final InteractionClassHandle interaction;
      private final TransportationTypeHandle transportation;

      private InteractionReport(FederateHandle federate, InteractionClassHandle interaction,
            TransportationTypeHandle transportation) {
         this.federate = federate;
         this.interaction = interaction;
         this.transportation = transportation;
      }
   }

   private static final class InteractionConfirmation {
      private final InteractionClassHandle interaction;
      private final TransportationTypeHandle transportation;

      private InteractionConfirmation(InteractionClassHandle interaction,
            TransportationTypeHandle transportation) {
         this.interaction = interaction;
         this.transportation = transportation;
      }
   }

   private static final class Recorder {
      private final CopyOnWriteArrayList<AttributeReport> attributeReports =
         new CopyOnWriteArrayList<>();
      private final CopyOnWriteArrayList<AttributeConfirmation> attributeConfirmations =
         new CopyOnWriteArrayList<>();
      private final CopyOnWriteArrayList<InteractionReport> interactionReports =
         new CopyOnWriteArrayList<>();
      private final CopyOnWriteArrayList<InteractionConfirmation> interactionConfirmations =
         new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("reportAttributeTransportationType".equals(name)) {
                  attributeReports.add(new AttributeReport(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandle) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               } else if ("confirmAttributeTransportationTypeChange".equals(name)) {
                  attributeConfirmations.add(new AttributeConfirmation(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandleSet) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               } else if ("reportInteractionTransportationType".equals(name)) {
                  interactionReports.add(new InteractionReport(
                     (FederateHandle) arguments[0], (InteractionClassHandle) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               } else if ("confirmInteractionTransportationTypeChange".equals(name)) {
                  interactionConfirmations.add(new InteractionConfirmation(
                     (InteractionClassHandle) arguments[0],
                     (TransportationTypeHandle) arguments[1]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK unavailable transportation callbacks";
                  }
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
