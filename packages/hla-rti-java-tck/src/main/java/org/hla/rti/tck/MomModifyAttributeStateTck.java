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
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.BooleanSupplier;

/** Mirrors the portable HLAmodifyAttributeState request and its MIM failure report. */
final class MomModifyAttributeStateTck {
   private static final String MODIFY_ATTRIBUTE_STATE =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAmodifyAttributeState";
   private static final String MOM_EXCEPTION_REPORT =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception";
   private static final long TIMEOUT_NANOS = 15_000_000_000L;

   private MomModifyAttributeStateTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         CallbackModel callbackModel, String objectClassName, String attributeName)
         throws Exception {
      JavaTckSupport.check(timeImplementation != null && !timeImplementation.isEmpty(),
         "HLAmodifyAttributeState requires adapter-supplied logical-time configuration");
      JavaTckSupport.check(objectClassName != null && !objectClassName.isEmpty()
            && attributeName != null && !attributeName.isEmpty(),
         "HLAmodifyAttributeState requires adapter-supplied object-class and attribute names");

      RTIambassador manager = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador target = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder managerRecorder = new Recorder();
      Recorder ownerRecorder = new Recorder();
      Recorder targetRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-modify-attribute-state-" + suffix;
      String managerName = "java-tck-mom-modify-manager-" + suffix;
      String ownerName = "java-tck-mom-modify-owner-" + suffix;
      String targetName = "java-tck-mom-modify-target-" + suffix;
      boolean managerConnected = false;
      boolean ownerConnected = false;
      boolean targetConnected = false;
      boolean created = false;
      boolean managerJoined = false;
      boolean ownerJoined = false;
      boolean targetJoined = false;
      boolean reportSubscribed = false;
      ObjectInstanceHandle object = null;

      try {
         manager.connect(managerRecorder.proxy(), callbackModel);
         managerConnected = true;
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         target.connect(targetRecorder.proxy(), callbackModel);
         targetConnected = true;
         manager.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         manager.joinFederationExecution(managerName, "java-tck", federation);
         managerJoined = true;
         owner.joinFederationExecution(ownerName, "java-tck", federation);
         ownerJoined = true;
         target.joinFederationExecution(targetName, "java-tck", federation);
         targetJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         ObjectClassHandle targetClass = target.getObjectClassHandle(objectClassName);
         AttributeHandle targetAttribute = target.getAttributeHandle(targetClass, attributeName);
         InteractionClassHandle modifyClass = manager.getInteractionClassHandle(
            MODIFY_ATTRIBUTE_STATE);
         ParameterHandle targetFederateParameter =
            manager.getParameterHandle(modifyClass, "HLAfederate");
         ParameterHandle objectParameter =
            manager.getParameterHandle(modifyClass, "HLAobjectInstance");
         ParameterHandle attributeParameter =
            manager.getParameterHandle(modifyClass, "HLAattribute");
         ParameterHandle stateParameter =
            manager.getParameterHandle(modifyClass, "HLAattributeState");
         InteractionClassHandle reportClass =
            manager.getInteractionClassHandle(MOM_EXCEPTION_REPORT);
         ParameterHandle reportServiceParameter =
            manager.getParameterHandle(reportClass, "HLAservice");
         ParameterHandle reportExceptionParameter =
            manager.getParameterHandle(reportClass, "HLAexception");
         ParameterHandle reportParameterErrorParameter =
            manager.getParameterHandle(reportClass, "HLAparameterError");
         ParameterHandle reportFederateParameter =
            manager.getParameterHandle(reportClass, "HLAfederate");
         TransportationTypeHandle reliable = manager.getTransportationTypeHandle("HLAreliable");
         FederateHandle managerFederate = manager.getFederateHandle(managerName);
         FederateHandle targetFederate = target.getFederateHandle(targetName);
         JavaTckSupport.check(ownerClass != null && ownerAttribute != null
               && targetClass != null && targetAttribute != null && modifyClass != null
               && targetFederateParameter != null && objectParameter != null
               && attributeParameter != null && stateParameter != null && reportClass != null
               && reportServiceParameter != null && reportExceptionParameter != null
               && reportParameterErrorParameter != null && reportFederateParameter != null
               && reliable != null && managerFederate != null && targetFederate != null,
            "standard HLAmodifyAttributeState, HLAreportMOMexception, or application handle lookup returned an invalid handle");
         manager.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         AttributeHandleSet ownerAttributes = owner.getAttributeHandleSetFactory().create();
         ownerAttributes.add(ownerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         AttributeHandleSet targetAttributes = target.getAttributeHandleSetFactory().create();
         targetAttributes.add(targetAttribute);
         target.subscribeObjectClassAttributes(targetClass, targetAttributes);
         object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "HLAmodifyAttributeState setup returned a null object instance handle");
         ObjectInstanceHandle registeredObject = object;
         await(target, callbackModel, () -> targetRecorder.discoveries.contains(registeredObject),
            "HLAmodifyAttributeState target object discovery");
         JavaTckSupport.check(targetClass.equals(target.getKnownObjectClassHandle(object))
               && owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !target.isAttributeOwnedByFederate(object, targetAttribute),
            "HLAmodifyAttributeState setup did not establish a known, unpublished target and original owner");

         ParameterHandleValueMap parameters =
            manager.getParameterHandleValueMapFactory().create(4);
         HLAinteger32BE owned = encoder.createHLAinteger32BE(1);
         parameters.put(targetFederateParameter, encode(targetFederate));
         parameters.put(objectParameter, encode(object));
         parameters.put(attributeParameter, encode(targetAttribute));
         parameters.put(stateParameter, owned.toByteArray());

         try {
            manager.sendInteraction(modifyClass, parameters, new byte[0]);
         } catch (Exception synchronousFailure) {
            // The standard MOM report remains the portable result for this MIM precondition.
         }
         await(manager, callbackModel, () -> !managerRecorder.reports.isEmpty(),
            "HLAmodifyAttributeState target-publication precondition report");
         settleCallbacks(manager, callbackModel);
         JavaTckSupport.check(managerRecorder.reports.size() == 1,
            "an invalid HLAmodifyAttributeState did not deliver exactly one HLAreportMOMexception");
         verifyReport(manager, encoder, managerRecorder.reports.get(0), reportClass,
            reportServiceParameter, reportExceptionParameter, reportParameterErrorParameter,
            reportFederateParameter, reliable, encode(managerFederate));
         JavaTckSupport.check(owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !target.isAttributeOwnedByFederate(object, targetAttribute),
            "rejected HLAmodifyAttributeState changed ownership despite the target's missing publication");

         target.publishObjectClassAttributes(targetClass, targetAttributes);
         manager.sendInteraction(modifyClass, parameters, new byte[0]);
         JavaTckSupport.check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && target.isAttributeOwnedByFederate(object, targetAttribute),
            "HLAmodifyAttributeState did not transfer the published attribute to the specified federate");
         settleCallbacks(owner, callbackModel);
         settleCallbacks(target, callbackModel);
         settleCallbacks(manager, callbackModel);
         JavaTckSupport.check(ownerRecorder.divestitureConfirmations.get() == 0
               && targetRecorder.acquisitionNotifications.get() == 0,
            "HLAmodifyAttributeState emitted an ownership-change notification to a federate");
         JavaTckSupport.check(managerRecorder.reports.size() == 1,
            "the valid HLAmodifyAttributeState request produced an unexpected MOM exception report");
      } finally {
         if (reportSubscribed) {
            try {
               manager.unsubscribeInteractionClass(
                  manager.getInteractionClassHandle(MOM_EXCEPTION_REPORT));
            } catch (Exception ignored) { }
         }
         if (targetJoined) {
            try { target.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (managerJoined) {
            try { manager.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (managerConnected) {
            try { manager.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
         if (targetConnected) {
            try { target.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void verifyReport(RTIambassador manager, EncoderFactory encoder, Report report,
         InteractionClassHandle reportClass, ParameterHandle serviceParameter,
         ParameterHandle exceptionParameter, ParameterHandle parameterErrorParameter,
         ParameterHandle federateParameter, TransportationTypeHandle reliable,
         byte[] expectedManager) throws Exception {
      JavaTckSupport.check(report.interaction.equals(reportClass)
            && report.tag.length == 0 && reliable.equals(report.transportation)
            && report.regionsEmpty && report.parameters.size() == 4
            && report.parameters.containsKey(serviceParameter)
            && report.parameters.containsKey(exceptionParameter)
            && report.parameters.containsKey(parameterErrorParameter)
            && report.parameters.containsKey(federateParameter),
         "HLAmodifyAttributeState HLAreportMOMexception metadata or parameter set was not standard");
      JavaTckSupport.requireNoProducingFederate(manager, report.producer,
         "HLAmodifyAttributeState HLAreportMOMexception");
      HLAunicodeString reportedService = encoder.createHLAunicodeString();
      reportedService.decode(report.parameters.get(serviceParameter));
      HLAunicodeString reportedException = encoder.createHLAunicodeString();
      reportedException.decode(report.parameters.get(exceptionParameter));
      HLAboolean parameterError = encoder.createHLAboolean();
      parameterError.decode(report.parameters.get(parameterErrorParameter));
      JavaTckSupport.check(MODIFY_ATTRIBUTE_STATE.equals(reportedService.getValue())
            && !reportedException.getValue().isEmpty() && !parameterError.getValue()
            && Arrays.equals(expectedManager, report.parameters.get(federateParameter)),
         "HLAmodifyAttributeState report did not identify the service, managing federate, or non-parameter precondition failure");
   }

   private static byte[] encode(FederateHandle handle) {
      byte[] encoded = new byte[handle.encodedLength()];
      handle.encode(encoded, 0);
      return encoded;
   }

   private static byte[] encode(ObjectInstanceHandle handle) {
      byte[] encoded = new byte[handle.encodedLength()];
      handle.encode(encoded, 0);
      return encoded;
   }

   private static byte[] encode(AttributeHandle handle) {
      byte[] encoded = new byte[handle.encodedLength()];
      handle.encode(encoded, 0);
      return encoded;
   }

   private static void await(RTIambassador rti, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (!condition.getAsBoolean()) {
         if (System.nanoTime() >= deadline) {
            throw new AssertionError("timed out waiting for " + description);
         }
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(rti);
         else Thread.yield();
         Thread.sleep(1L);
      }
   }

   private static void settleCallbacks(RTIambassador rti, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(rti);
      else Thread.sleep(25L);
   }

   private static Object callbackObjectMethod(Object proxy, String name, Object[] args) {
      if ("toString".equals(name)) return "Java TCK HLAmodifyAttributeState observer";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
      return null;
   }

   private static final class Report {
      private InteractionClassHandle interaction;
      private Map<ParameterHandle, byte[]> parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
   }

   private static final class Recorder {
      private final List<Report> reports = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final AtomicInteger divestitureConfirmations = new AtomicInteger();
      private final AtomicInteger acquisitionNotifications = new AtomicInteger();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && args != null
                     && args.length >= 1 && args[0] instanceof ObjectInstanceHandle) {
                  discoveries.add((ObjectInstanceHandle) args[0]);
               }
               if ("requestDivestitureConfirmation".equals(name)) {
                  divestitureConfirmations.incrementAndGet();
               }
               if ("attributeOwnershipAcquisitionNotification".equals(name)) {
                  acquisitionNotifications.incrementAndGet();
               }
               if ("receiveInteraction".equals(name) && args != null && args.length == 6
                     && args[1] instanceof ParameterHandleValueMap) {
                  Report report = new Report();
                  report.interaction = (InteractionClassHandle) args[0];
                  ParameterHandleValueMap source = (ParameterHandleValueMap) args[1];
                  report.parameters = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     report.parameters.put(handle, source.get(handle).clone());
                  }
                  report.tag = args[2] instanceof byte[] ? ((byte[]) args[2]).clone() : new byte[0];
                  report.transportation = (TransportationTypeHandle) args[3];
                  report.producer = (FederateHandle) args[4];
                  report.regionsEmpty = args[5] == null || ((RegionHandleSet) args[5]).isEmpty();
                  reports.add(report);
               }
               return callbackObjectMethod(proxy, name, args);
            });
      }
   }
}
