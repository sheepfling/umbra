package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/** Verifies hierarchy-aware registration discovery and subscription eligibility. */
final class ObjectRegistrationDiscoveryLifecycleTck {
   private static final int PUBLISHER = 0;
   private static final int EXACT = 1;
   private static final int BASE = 2;
   private static final int CANCELLED = 3;
   private static final int LATE = 4;

   private ObjectRegistrationDiscoveryLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String baseClassName,
         String derivedClassName, String identityAttributeName,
         String derivedAttributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador[] rti = new RTIambassador[5];
      Recorder[] recorders = new Recorder[5];
      boolean[] connected = new boolean[5];
      boolean[] joined = new boolean[5];
      boolean[] subscribed = new boolean[5];
      boolean published = false;
      boolean federationCreated = false;
      String federation = "java-tck-object-registration-discovery-" + UUID.randomUUID();
      String[] roles = {"publisher", "exact", "base", "cancelled", "late"};
      try {
         for (int i = 0; i < rti.length; ++i) {
            rti[i] = factory.getRtiAmbassador();
            recorders[i] = new Recorder();
            rti[i].connect(recorders[i].proxy(), callbackModel);
            connected[i] = true;
         }
         rti[PUBLISHER].createFederationExecution(federation, fom);
         federationCreated = true;
         FederateHandle producer = null;
         for (int i = 0; i < rti.length; ++i) {
            FederateHandle handle = rti[i].joinFederationExecution(
               "java-tck-object-registration-" + roles[i], federation);
            joined[i] = true;
            if (i == PUBLISHER) {
               producer = handle;
            }
         }

         ObjectClassHandle[] derivedClasses = new ObjectClassHandle[rti.length];
         ObjectClassHandle[] baseClasses = new ObjectClassHandle[rti.length];
         AttributeHandle[] identityAttributes = new AttributeHandle[rti.length];
         AttributeHandle[] derivedAttributes = new AttributeHandle[rti.length];
         for (int i = 0; i < rti.length; ++i) {
            derivedClasses[i] = rti[i].getObjectClassHandle(derivedClassName);
            baseClasses[i] = rti[i].getObjectClassHandle(baseClassName);
            identityAttributes[i] = rti[i].getAttributeHandle(
               derivedClasses[i], identityAttributeName);
            derivedAttributes[i] = rti[i].getAttributeHandle(
               derivedClasses[i], derivedAttributeName);
            JavaTckSupport.check(derivedClasses[i] != null && baseClasses[i] != null
                  && identityAttributes[i] != null && derivedAttributes[i] != null,
               "typed object registration FOM lookup returned a null handle");
         }
         JavaTckSupport.check(!derivedClasses[PUBLISHER].equals(baseClasses[PUBLISHER]),
            "typed object registration FOM did not preserve the class hierarchy");
         for (int i = 1; i < rti.length; ++i) {
            JavaTckSupport.check(derivedClasses[PUBLISHER].equals(derivedClasses[i])
                  && baseClasses[PUBLISHER].equals(baseClasses[i])
                  && identityAttributes[PUBLISHER].equals(identityAttributes[i])
                  && derivedAttributes[PUBLISHER].equals(derivedAttributes[i]),
               "object class or inherited attribute handles changed across federates");
         }
         JavaTckSupport.check(rti[PUBLISHER].getAttributeHandle(
               baseClasses[PUBLISHER], identityAttributeName)
               .equals(identityAttributes[PUBLISHER]),
            "inherited identity attribute handle changed between base and derived classes");

         JavaTckSupport.expectFailure(
            () -> rti[PUBLISHER].registerObjectInstance(derivedClasses[PUBLISHER]),
            "ObjectClassNotPublished",
            "registering the derived object class before publication");

         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(
            rti[PUBLISHER], identityAttributes[PUBLISHER], derivedAttributes[PUBLISHER]);
         AttributeHandleSet exactAttributes = JavaTckSupport.attributeSet(
            rti[EXACT], derivedAttributes[EXACT]);
         AttributeHandleSet baseAttributes = JavaTckSupport.attributeSet(
            rti[BASE], identityAttributes[BASE]);
         AttributeHandleSet cancelledAttributes = JavaTckSupport.attributeSet(
            rti[CANCELLED], derivedAttributes[CANCELLED]);
         AttributeHandleSet lateAttributes = JavaTckSupport.attributeSet(
            rti[LATE], derivedAttributes[LATE]);

         rti[EXACT].subscribeObjectClassAttributes(derivedClasses[EXACT], exactAttributes);
         subscribed[EXACT] = true;
         rti[BASE].subscribeObjectClassAttributes(baseClasses[BASE], baseAttributes);
         subscribed[BASE] = true;
         rti[CANCELLED].subscribeObjectClassAttributes(
            derivedClasses[CANCELLED], cancelledAttributes);
         subscribed[CANCELLED] = true;
         rti[PUBLISHER].publishObjectClassAttributes(
            derivedClasses[PUBLISHER], publisherAttributes);
         published = true;

         ObjectInstanceHandle object = rti[PUBLISHER].registerObjectInstance(
            derivedClasses[PUBLISHER]);
         String objectName = rti[PUBLISHER].getObjectInstanceName(object);
         JavaTckSupport.check(object != null && objectName != null && !objectName.isEmpty()
               && object.equals(rti[PUBLISHER].getObjectInstanceHandle(objectName))
               && derivedClasses[PUBLISHER].equals(
                  rti[PUBLISHER].getKnownObjectClassHandle(object)),
            "publisher object identity or actual-class lookup did not round-trip");
         JavaTckSupport.expectFailure(
            () -> rti[LATE].getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown",
            "looking up an undiscovered object by name");

         if (callbackModel == CallbackModel.HLA_EVOKED) {
            for (int index : new int[] {EXACT, BASE, CANCELLED}) {
               JavaTckSupport.check(recorders[index].discoveries.isEmpty(),
                  "evoked object discovery arrived before callback servicing");
            }
            JavaTckSupport.expectFailure(
               () -> rti[EXACT].getKnownObjectClassHandle(object),
               "ObjectInstanceNotKnown",
               "looking up an evoked object before discovery");

            rti[CANCELLED].unsubscribeObjectClassAttributes(
               derivedClasses[CANCELLED], cancelledAttributes);
            subscribed[CANCELLED] = false;
            JavaTckSupport.drain(rti[CANCELLED]);
            JavaTckSupport.check(recorders[CANCELLED].discoveries.isEmpty(),
               "cancelled evoked discovery remained queued after unsubscribe");
            JavaTckSupport.drain(rti[EXACT]);
            JavaTckSupport.drain(rti[BASE]);
         } else {
            JavaTckSupport.check(recorders[EXACT].discoveries.size() == 1
                  && recorders[BASE].discoveries.size() == 1
                  && recorders[CANCELLED].discoveries.size() == 1,
               "immediate registration did not discover the object once per subscriber");
            rti[CANCELLED].unsubscribeObjectClassAttributes(
               derivedClasses[CANCELLED], cancelledAttributes);
            subscribed[CANCELLED] = false;
         }

         assertDiscovery(rti[EXACT], recorders[EXACT], object, objectName,
            derivedClasses[EXACT], producer, "exact-class discovery");
         assertDiscovery(rti[BASE], recorders[BASE], object, objectName,
            baseClasses[BASE], producer, "base-class promoted discovery");

         rti[LATE].subscribeObjectClassAttributes(derivedClasses[LATE], lateAttributes);
         subscribed[LATE] = true;
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(recorders[LATE].discoveries.isEmpty(),
               "evoked late discovery arrived before callback servicing");
            JavaTckSupport.drain(rti[LATE]);
         }
         assertDiscovery(rti[LATE], recorders[LATE], object, objectName,
            derivedClasses[LATE], producer, "late exact-class discovery");

         rti[EXACT].subscribeObjectClassAttributes(derivedClasses[EXACT], exactAttributes);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(rti[EXACT]);
         }
         JavaTckSupport.check(recorders[EXACT].discoveries.size() == 1,
            "repeating an active object subscription generated duplicate discovery");
      } finally {
         if (rti[PUBLISHER] != null && joined[PUBLISHER] && published) {
            try {
               rti[PUBLISHER].unpublishObjectClassAttributes(
                  rti[PUBLISHER].getObjectClassHandle(derivedClassName),
                  JavaTckSupport.attributeSet(rti[PUBLISHER],
                     rti[PUBLISHER].getAttributeHandle(
                        rti[PUBLISHER].getObjectClassHandle(derivedClassName),
                        identityAttributeName),
                     rti[PUBLISHER].getAttributeHandle(
                        rti[PUBLISHER].getObjectClassHandle(derivedClassName),
                        derivedAttributeName)));
            } catch (Exception ignored) { }
         }
         for (int i = 0; i < rti.length; ++i) {
            if (rti[i] != null && joined[i] && subscribed[i]) {
               try {
                  ObjectClassHandle subscribedClass = rti[i].getObjectClassHandle(
                     i == BASE ? baseClassName : derivedClassName);
                  AttributeHandle subscribedAttribute = rti[i].getAttributeHandle(
                     subscribedClass, i == BASE ? identityAttributeName : derivedAttributeName);
                  rti[i].unsubscribeObjectClassAttributes(subscribedClass,
                     JavaTckSupport.attributeSet(rti[i], subscribedAttribute));
               } catch (Exception ignored) { }
            }
         }
         for (int i = rti.length - 1; i >= 0; --i) {
            if (rti[i] != null && joined[i]) {
               try {
                  rti[i].resignFederationExecution(
                     i == PUBLISHER ? ResignAction.DELETE_OBJECTS : ResignAction.NO_ACTION);
               } catch (Exception ignored) { }
            }
         }
         if (federationCreated && rti[PUBLISHER] != null) {
            try { rti[PUBLISHER].destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int i = rti.length - 1; i >= 0; --i) {
            if (rti[i] != null && connected[i]) {
               try { rti[i].disconnect(); } catch (Exception ignored) { }
            }
         }
      }
   }

   private static void assertDiscovery(RTIambassador rti, Recorder recorder,
         ObjectInstanceHandle object, String objectName, ObjectClassHandle expectedClass,
         FederateHandle producer, String description) throws Exception {
      JavaTckSupport.check(recorder.discoveries.size() == 1,
         description + " did not deliver exactly one discovery callback");
      Discovery actual = recorder.discoveries.get(0);
      JavaTckSupport.check(object.equals(actual.object)
            && expectedClass.equals(actual.objectClass)
            && objectName.equals(actual.name)
            && producer.equals(actual.producer),
         description + " returned incorrect object, class, name, or producer metadata");
      JavaTckSupport.check(expectedClass.equals(rti.getKnownObjectClassHandle(object))
            && object.equals(rti.getObjectInstanceHandle(objectName))
            && objectName.equals(rti.getObjectInstanceName(object)),
         description + " did not establish stable known-object identity lookups");
   }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
      private String name;
      private FederateHandle producer;
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length == 4) {
                  Discovery event = new Discovery();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.objectClass = (ObjectClassHandle) arguments[1];
                  event.name = (String) arguments[2];
                  event.producer = (FederateHandle) arguments[3];
                  discoveries.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK object discovery recorder";
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
