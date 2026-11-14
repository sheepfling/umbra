package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies that the Table 7 local-delete request forgets an object only locally. */
final class Table7LocalDeleteObjectInstanceTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String REQUEST =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLAlocalDeleteObjectInstance";

   private Table7LocalDeleteObjectInstanceTck() {
   }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(fom != null && !fom.isEmpty()
            && mim != null && !mim.isEmpty()
            && timeImplementation != null && !timeImplementation.isEmpty()
            && objectClassName != null && !objectClassName.isEmpty()
            && attributeName != null && !attributeName.isEmpty(),
         "Table 7 local-delete testing requires adapter FOM, MIM, time, object-class, and attribute configuration");

      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder subjectRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-table7-local-delete-" + suffix;
      boolean subjectConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false;
      boolean observerJoined = false;
      boolean subjectSubscribed = false;
      boolean observerPublished = false;
      boolean requestPublished = false;
      ObjectClassHandle subjectObjectClass = null;
      ObjectClassHandle observerObjectClass = null;
      AttributeHandleSet subjectAttributes = null;
      AttributeHandleSet observerAttributes = null;
      InteractionClassHandle requestClass = null;
      try {
         subject.connect(subjectRecorder.proxy(), callbackModel);
         subjectConnected = true;
         observer.connect(SupportServicesTck.noOpFederateAmbassador(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         subject.joinFederationExecution("java-tck-table7-local-delete-subject-" + suffix,
            federation);
         subjectJoined = true;
         observer.joinFederationExecution("java-tck-table7-local-delete-observer-" + suffix,
            federation);
         observerJoined = true;

         observerObjectClass = observer.getObjectClassHandle(objectClassName);
         ObjectClassHandle subjectClass = subject.getObjectClassHandle(objectClassName);
         AttributeHandle observerAttribute =
            observer.getAttributeHandle(observerObjectClass, attributeName);
         AttributeHandle subjectAttribute = subject.getAttributeHandle(subjectClass, attributeName);
         JavaTckSupport.check(observerObjectClass != null && subjectClass != null
               && observerAttribute != null && subjectAttribute != null
               && observerObjectClass.equals(subjectClass)
               && observerAttribute.equals(subjectAttribute),
            "local-delete federates could not resolve matching adapter-FOM handles");
         subjectObjectClass = subjectClass;
         observerAttributes = JavaTckSupport.attributeSet(observer, observerAttribute);
         subjectAttributes = JavaTckSupport.attributeSet(subject, subjectAttribute);
         observer.publishObjectClassAttributes(observerObjectClass, observerAttributes);
         observerPublished = true;
         subject.subscribeObjectClassAttributes(subjectObjectClass, subjectAttributes);
         subjectSubscribed = true;

         ObjectInstanceHandle object = observer.registerObjectInstance(observerObjectClass);
         JavaTckSupport.check(object != null,
            "local-delete observer registration returned no object handle");
         await(subject, callbackModel,
            () -> subjectRecorder.discoveredObjects.contains(object),
            "object discovery before HLAlocalDeleteObjectInstance");
         JavaTckSupport.check(subjectRecorder.discoveredObjects.size() == 1,
            "subject received an unexpected number of object discoveries");
         String objectName = observer.getObjectInstanceName(object);

         requestClass = subject.getInteractionClassHandle(REQUEST);
         JavaTckSupport.check(requestClass != null,
            "standard MIM omitted HLAlocalDeleteObjectInstance");
         ParameterHandle objectParameter = subject.getParameterHandle(requestClass, "HLAobjectInstance");
         JavaTckSupport.check(objectParameter != null,
            "standard MIM omitted HLAobjectInstance from HLAlocalDeleteObjectInstance");
         subject.publishInteractionClass(requestClass);
         requestPublished = true;
         byte[] encodedObject = new byte[object.encodedLength()];
         object.encode(encodedObject, 0);
         ParameterHandleValueMap parameters =
            subject.getParameterHandleValueMapFactory().create(1);
         parameters.put(objectParameter, encodedObject);
         subject.sendInteraction(requestClass, parameters, new byte[0]);

         JavaTckSupport.expectFailure(
            () -> subject.getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown",
            "subject lookup after HLAlocalDeleteObjectInstance");
         JavaTckSupport.check(objectName.equals(observer.getObjectInstanceName(object)),
            "HLAlocalDeleteObjectInstance removed the object from another federate");
      } finally {
         if (requestPublished && requestClass != null) {
            try { subject.unpublishInteractionClass(requestClass); }
            catch (Exception ignored) { }
         }
         if (subjectSubscribed && subjectObjectClass != null && subjectAttributes != null) {
            try { subject.unsubscribeObjectClassAttributes(subjectObjectClass, subjectAttributes); }
            catch (Exception ignored) { }
         }
         if (observerPublished && observerObjectClass != null && observerAttributes != null) {
            try { observer.unpublishObjectClassAttributes(observerObjectClass, observerAttributes); }
            catch (Exception ignored) { }
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); }
            catch (Exception ignored) { }
         }
         if (subjectConnected) {
            try { subject.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador member, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            member.evokeCallback(0.01);
         } else {
            Thread.sleep(1L);
         }
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveredObjects = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments.length == 4) {
                  discoveredObjects.add((ObjectInstanceHandle) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Table 7 local-delete recorder";
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
