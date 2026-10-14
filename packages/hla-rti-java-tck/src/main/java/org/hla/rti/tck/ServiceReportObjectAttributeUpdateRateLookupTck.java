package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Verifies object, attribute, and update-rate lookup results through standard MOM reports. */
final class ServiceReportObjectAttributeUpdateRateLookupTck {
   private static final String REPORT_CLASS =
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
   private static final String[] REPORT_PARAMETERS = {
      "HLAservice", "HLAserviceType", "HLAsuccessIndicator", "HLAsuppliedArguments",
      "HLAreturnedArgument", "HLAexception", "HLAserialNumber", "HLAfederate"
   };
   private static final long TIMEOUT_NANOS = 12_000_000_000L;

   private ServiceReportObjectAttributeUpdateRateLookupTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, String updateRateName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-object-attribute-rate-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean published = false;
      InteractionClassHandle reportClass = null;
      ObjectClassHandle objectClass = null;
      AttributeHandle attribute = null;
      ObjectInstanceHandle object = null;
      AttributeHandleSet publishedAttributes = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-object-attribute-rate-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution("java-tck-object-attribute-rate-observer", "java-tck",
            federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         objectClass = subject.getObjectClassHandle(objectClassName);
         attribute = subject.getAttributeHandle(objectClass, attributeName);
         JavaTckSupport.check(objectClass != null && attribute != null
               && objectClassName.equals(subject.getObjectClassName(objectClass))
               && attributeName.equals(subject.getAttributeName(objectClass, attribute)),
            "adapter-supplied object class or attribute lookup/name did not round-trip");
         publishedAttributes = JavaTckSupport.attributeSet(subject, attribute);
         subject.publishObjectClassAttributes(objectClass, publishedAttributes);
         published = true;
         object = subject.registerObjectInstance(objectClass);
         JavaTckSupport.check(object != null,
            "adapter-supplied object class registration returned a null handle");
         String objectName = subject.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty(),
            "registered object did not have a standard instance name");

         subject.setServiceReportingSwitch(true);
         ObjectClassHandle knownObjectClass = subject.getKnownObjectClassHandle(object);
         ObjectInstanceHandle objectByName = subject.getObjectInstanceHandle(objectName);
         String resolvedObjectName = subject.getObjectInstanceName(object);
         AttributeHandle attributeByClass = subject.getAttributeHandle(objectClass, attributeName);
         String resolvedAttributeName = subject.getAttributeName(objectClass, attribute);
         double maximumRate = subject.getUpdateRateValue(updateRateName);
         double attributeMaximumRate = subject.getUpdateRateValueForAttribute(object, attribute);
         JavaTckSupport.check(objectClass.equals(knownObjectClass)
               && object.equals(objectByName) && objectName.equals(resolvedObjectName)
               && attribute.equals(attributeByClass)
               && attributeName.equals(resolvedAttributeName)
               && maximumRate > 0.0 && attributeMaximumRate == 0.0,
            "object, attribute, or update-rate lookup returned an unexpected value");
         await(observer, recorder, callbackModel);
         JavaTckSupport.check(recorder.events.size() == 7,
            "object/attribute/update-rate lookups produced an unexpected report count");

         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetKnownObjectClassHandle",
            new Argument[] {new Argument(37, "Object instance handle", quoted(object.toString()))},
            new Argument(36, "Object class handle", quoted(knownObjectClass.toString())), 0);
         verifyReport(observer, subjectFederate, recorder.events.get(1), encoder, reportClass,
            reportParameters, reliable, "GetObjectInstanceHandle",
            new Argument[] {new Argument(53, "Object instance name", quoted(objectName))},
            new Argument(37, "Object instance handle", quoted(objectByName.toString())), 1);
         verifyReport(observer, subjectFederate, recorder.events.get(2), encoder, reportClass,
            reportParameters, reliable, "GetObjectInstanceName",
            new Argument[] {new Argument(37, "Object instance handle", quoted(object.toString()))},
            new Argument(53, "Object instance name", quoted(resolvedObjectName)), 2);
         verifyReport(observer, subjectFederate, recorder.events.get(3), encoder, reportClass,
            reportParameters, reliable, "GetAttributeHandle",
            new Argument[] {
               new Argument(36, "Object class handle", quoted(objectClass.toString())),
               new Argument(53, "Class attribute name", quoted(attributeName))
            },
            new Argument(0, "Class attribute handle", quoted(attributeByClass.toString())), 3);
         verifyReport(observer, subjectFederate, recorder.events.get(4), encoder, reportClass,
            reportParameters, reliable, "GetAttributeName",
            new Argument[] {
               new Argument(36, "Object class handle", quoted(objectClass.toString())),
               new Argument(0, "Class attribute handle", quoted(attribute.toString()))
            },
            new Argument(53, "Class attribute name", quoted(resolvedAttributeName)), 4);
         verifyReport(observer, subjectFederate, recorder.events.get(5), encoder, reportClass,
            reportParameters, reliable, "GetUpdateRateValue",
            new Argument[] {new Argument(53, "Update rate name", quoted(updateRateName))},
            new Argument(35, "Maximum update rate value", formatRate(maximumRate)), 5);
         verifyReport(observer, subjectFederate, recorder.events.get(6), encoder, reportClass,
            reportParameters, reliable, "GetUpdateRateValueForAttribute",
            new Argument[] {
               new Argument(37, "Object instance handle", quoted(object.toString())),
               new Argument(0, "Attribute handle", quoted(attribute.toString()))
            },
            new Argument(35, "Maximum update rate value", formatRate(attributeMaximumRate)), 6);

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         if (published) {
            subject.unpublishObjectClassAttributes(objectClass, publishedAttributes);
            published = false;
         }
         subject.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (published && subjectJoined && objectClass != null && publishedAttributes != null) {
            try { subject.unpublishObjectClassAttributes(objectClass, publishedAttributes); }
            catch (Exception ignored) { }
         }
         if (observerJoined) resign(observer);
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runDimensionUpperBound(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-dimension-upper-bound-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-dimension-upper-bound-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-dimension-upper-bound-observer", "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null
               && dimensionName.equals(subject.getDimensionName(dimension)),
            "adapter-supplied dimension lookup/name round-trip failed");
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "dimension upper-bound test could not select MOM delivery");
         subject.setServiceReportingSwitch(true);
         long upperBound = subject.getDimensionUpperBound(dimension);
         JavaTckSupport.check(upperBound == 64L,
            "dimension upper-bound lookup did not return the adapter FOM's bound of 64");

         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "GetDimensionUpperBound delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetDimensionUpperBound",
            new Argument[] {new Argument(10, "Dimension handle", quoted(dimension.toString()))},
            new Argument(35, "Dimension upper bound", Long.toString(upperBound)), 0);

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runDimensionName(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-dimension-name-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-dimension-name-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-dimension-name-observer", "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter-supplied dimension lookup returned a null handle");
         String resolvedName = subject.getDimensionName(dimension);
         JavaTckSupport.check(dimensionName.equals(resolvedName),
            "dimension-name lookup did not return the adapter FOM's dimension name");
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "dimension-name test could not select MOM delivery");
         subject.setServiceReportingSwitch(true);
         String reportedName = subject.getDimensionName(dimension);
         JavaTckSupport.check(resolvedName.equals(reportedName),
            "reported dimension-name lookup returned a different name");

         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "GetDimensionName delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetDimensionName",
            new Argument[] {new Argument(10, "Dimension handle", quoted(dimension.toString()))},
            new Argument(53, "Dimension name", quoted(reportedName)), 0);

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runDimensionHandle(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-dimension-handle-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-dimension-handle-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-dimension-handle-observer", "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "dimension-handle test could not select MOM delivery");
         subject.setServiceReportingSwitch(true);

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "GetDimensionHandle delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetDimensionHandle",
            new Argument[] {new Argument(53, "Dimension name", quoted(dimensionName))},
            new Argument(10, "Dimension handle", quoted(dimension.toString())), 0);

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runDimensionHandleSet(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-dimension-handle-set-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean regionCreated = false;
      RegionHandle region = null;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-dimension-handle-set-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-dimension-handle-set-observer", "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         DimensionHandleSet regionDimensions = subject.getDimensionHandleSetFactory().create();
         regionDimensions.add(dimension);
         region = subject.createRegion(regionDimensions);
         regionCreated = true;
         JavaTckSupport.check(region != null,
            "standard region creation returned a null handle");
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "dimension-handle-set test could not select MOM delivery");
         subject.setServiceReportingSwitch(true);

         DimensionHandleSet returnedDimensions = subject.getDimensionHandleSet(region);
         JavaTckSupport.check(returnedDimensions.size() == 1
               && returnedDimensions.contains(dimension),
            "GetDimensionHandleSet did not return exactly the region's configured dimension");
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "GetDimensionHandleSet delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetDimensionHandleSet",
            new Argument[] {new Argument(42, "Region handle", quoted(region.toString()))},
            new Argument(11, "A set of dimensions",
               "[\"" + dimension.toString() + "\"]"), 0);

         subject.setServiceReportingSwitch(false);
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.deleteRegion(region);
         regionCreated = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (regionCreated && region != null) {
            try { subject.deleteRegion(region); }
            catch (Exception ignored) { }
         }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runRangeBounds(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-range-bounds-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean regionCreated = false;
      RegionHandle region = null;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-range-bounds-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-range-bounds-observer", "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         long upperBound = subject.getDimensionUpperBound(dimension);
         JavaTckSupport.check(upperBound > 0,
            "range-bounds report requires a positive adapter dimension upper bound");
         DimensionHandleSet regionDimensions = subject.getDimensionHandleSetFactory().create();
         regionDimensions.add(dimension);
         region = subject.createRegion(regionDimensions);
         regionCreated = true;
         JavaTckSupport.check(region != null,
            "standard region creation returned a null handle");
         long lowerBound = upperBound > 1 ? 1 : 0;
         subject.setRangeBounds(region, dimension,
            new hla.rti1516_2025.RangeBounds(lowerBound, upperBound));
         RegionHandleSet committedRegions = subject.getRegionHandleSetFactory().create();
         committedRegions.add(region);
         subject.commitRegionModifications(committedRegions);
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "range-bounds test could not select MOM delivery");
         subject.setServiceReportingSwitch(true);

         hla.rti1516_2025.RangeBounds actualBounds = subject.getRangeBounds(region, dimension);
         JavaTckSupport.check(actualBounds.lower == lowerBound
               && actualBounds.upper == upperBound,
            "GetRangeBounds did not return the adapter region's committed bounds");
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "GetRangeBounds delivered an unexpected number of service reports");
         String boundsText = "{\"lower\":" + lowerBound + ",\"upper\":" + upperBound + "}";
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetRangeBounds",
            new Argument[] {
               new Argument(42, "Region handle", quoted(region.toString())),
               new Argument(10, "Dimension handle", quoted(dimension.toString()))
            }, new Argument(41, "Range bounds", boundsText), 0, 5);

         subject.setServiceReportingSwitch(false);
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.deleteRegion(region);
         regionCreated = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (regionCreated && region != null) {
            try { subject.deleteRegion(region); }
            catch (Exception ignored) { }
         }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runSetRangeBounds(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-set-range-bounds-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean regionCreated = false;
      RegionHandle region = null;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-set-range-bounds-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-set-range-bounds-observer", "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         long upperBound = subject.getDimensionUpperBound(dimension);
         JavaTckSupport.check(upperBound > 0,
            "SetRangeBounds report requires a positive adapter dimension upper bound");
         DimensionHandleSet regionDimensions = subject.getDimensionHandleSetFactory().create();
         regionDimensions.add(dimension);
         region = subject.createRegion(regionDimensions);
         regionCreated = true;
         JavaTckSupport.check(region != null,
            "standard region creation returned a null handle");
         subject.setRangeBounds(region, dimension,
            new hla.rti1516_2025.RangeBounds(0, upperBound));
         RegionHandleSet committedRegions = subject.getRegionHandleSetFactory().create();
         committedRegions.add(region);
         subject.commitRegionModifications(committedRegions);
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "SetRangeBounds test could not select MOM delivery");

         long lowerBound = upperBound > 1 ? 1 : 0;
         subject.setServiceReportingSwitch(true);
         subject.setRangeBounds(region, dimension,
            new hla.rti1516_2025.RangeBounds(lowerBound, upperBound));
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "SetRangeBounds delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "SetRangeBounds",
            new Argument[] {
               new Argument(42, "Region handle", quoted(region.toString())),
               new Argument(10, "Dimension handle", quoted(dimension.toString())),
               new Argument(35, "Range lower bound", Long.toString(lowerBound)),
               new Argument(35, "Range upper bound", Long.toString(upperBound))
            }, new Argument(34, "", "null"), 0, 5);

         subject.setServiceReportingSwitch(false);
         hla.rti1516_2025.RangeBounds actualBounds = subject.getRangeBounds(region, dimension);
         JavaTckSupport.check(actualBounds.lower == lowerBound
               && actualBounds.upper == upperBound,
            "SetRangeBounds did not retain the adapter-valid bounds");
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.deleteRegion(region);
         regionCreated = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (regionCreated && region != null) {
            try { subject.deleteRegion(region); }
            catch (Exception ignored) { }
         }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runCommitRegionModifications(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-commit-region-modifications-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean regionCreated = false;
      RegionHandle region = null;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-commit-region-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-commit-region-observer", "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         long upperBound = subject.getDimensionUpperBound(dimension);
         JavaTckSupport.check(upperBound > 0,
            "CommitRegionModifications report requires a positive adapter dimension upper bound");
         DimensionHandleSet regionDimensions = subject.getDimensionHandleSetFactory().create();
         regionDimensions.add(dimension);
         region = subject.createRegion(regionDimensions);
         regionCreated = true;
         JavaTckSupport.check(region != null,
            "standard region creation returned a null handle");
         subject.setRangeBounds(region, dimension,
            new hla.rti1516_2025.RangeBounds(0, upperBound));
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "CommitRegionModifications test could not select MOM delivery");

         subject.setServiceReportingSwitch(true);
         RegionHandleSet committedRegions = subject.getRegionHandleSetFactory().create();
         committedRegions.add(region);
         subject.commitRegionModifications(committedRegions);
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "CommitRegionModifications delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "CommitRegionModifications",
            new Argument[] {
               new Argument(43, "Set of region designators",
                  "[\"" + region.toString() + "\"]")
            }, new Argument(34, "", "null"), 0, 5);

         subject.setServiceReportingSwitch(false);
         hla.rti1516_2025.RangeBounds committedBounds = subject.getRangeBounds(region, dimension);
         JavaTckSupport.check(committedBounds.lower == 0
               && committedBounds.upper == upperBound,
            "CommitRegionModifications did not commit the adapter-valid bounds");
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.deleteRegion(region);
         regionCreated = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (regionCreated && region != null) {
            try { subject.deleteRegion(region); }
            catch (Exception ignored) { }
         }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runDeleteRegion(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-delete-region-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean regionCreated = false;
      RegionHandle region = null;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-delete-region-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-delete-region-observer", "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         DimensionHandleSet regionDimensions = subject.getDimensionHandleSetFactory().create();
         regionDimensions.add(dimension);
         region = subject.createRegion(regionDimensions);
         regionCreated = true;
         JavaTckSupport.check(region != null,
            "standard region creation returned a null handle");
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "DeleteRegion test could not select MOM delivery");

         subject.setServiceReportingSwitch(true);
         subject.deleteRegion(region);
         regionCreated = false;
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "DeleteRegion delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "DeleteRegion",
            new Argument[] {
               new Argument(42, "Region designator", quoted(region.toString()))
            }, new Argument(34, "", "null"), 0, 5);

         subject.setServiceReportingSwitch(false);
         RegionHandle deletedRegion = region;
         JavaTckSupport.expectFailure(() -> subject.getDimensionHandleSet(deletedRegion),
            "InvalidRegion", "looking up dimensions after region deletion");
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (regionCreated && region != null) {
            try { subject.deleteRegion(region); }
            catch (Exception ignored) { }
         }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runCreateRegion(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-create-region-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      boolean regionCreated = false;
      RegionHandle region = null;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-create-region-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-create-region-observer", "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "CreateRegion test could not select MOM delivery");
         DimensionHandleSet regionDimensions = subject.getDimensionHandleSetFactory().create();
         regionDimensions.add(dimension);

         subject.setServiceReportingSwitch(true);
         region = subject.createRegion(regionDimensions);
         regionCreated = true;
         JavaTckSupport.check(region != null,
            "standard region creation returned a null handle");
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "CreateRegion delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "CreateRegion",
            new Argument[] {
               new Argument(11, "Set of dimension designators",
                  "[\"" + dimension.toString() + "\"]")
            }, new Argument(42, "Region designator", quoted(region.toString())), 0, 5);

         subject.setServiceReportingSwitch(false);
         subject.deleteRegion(region);
         regionCreated = false;
         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (regionCreated && region != null) {
            try { subject.deleteRegion(region); }
            catch (Exception ignored) { }
         }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runInteractionDimensions(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-interaction-dimensions-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-interaction-dimensions-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-interaction-dimensions-observer", "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         InteractionClassHandle interactionClass =
            subject.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(interactionClass != null
               && interactionClassName.equals(subject.getInteractionClassName(interactionClass)),
            "adapter FOM interaction-class lookup/name round-trip failed");
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "interaction-dimensions test could not select MOM delivery");

         subject.setServiceReportingSwitch(true);
         DimensionHandleSet availableDimensions =
            subject.getAvailableDimensionsForInteractionClass(interactionClass);
         JavaTckSupport.check(availableDimensions != null
               && availableDimensions.size() == 1 && availableDimensions.contains(dimension),
            "adapter FOM interaction did not expose exactly its configured dimension");
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "GetAvailableDimensionsForInteractionClass delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetAvailableDimensionsForInteractionClass",
            new Argument[] {
               new Argument(27, "Interaction class handle", quoted(interactionClass.toString()))
            }, new Argument(11, "A set of dimension handles",
               "[\"" + dimension.toString() + "\"]"), 0);

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   static void runObjectDimensions(RtiFactory factory, String fom, String mim,
         String timeImplementation, String dimensionName, String objectClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador subject = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      EncoderFactory encoder = factory.getEncoderFactory();
      String federation = "java-tck-service-report-object-dimensions-" + UUID.randomUUID();
      boolean subjectConnected = false, observerConnected = false, created = false;
      boolean subjectJoined = false, observerJoined = false, reportSubscribed = false;
      InteractionClassHandle reportClass = null;
      try {
         subject.connect(emptyCallbacks(), callbackModel);
         subjectConnected = true;
         observer.connect(recorder.proxy(), callbackModel);
         observerConnected = true;
         subject.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         FederateHandle subjectFederate = subject.joinFederationExecution(
            "java-tck-object-dimensions-subject", "java-tck", federation);
         subjectJoined = true;
         observer.joinFederationExecution(
            "java-tck-object-dimensions-observer", "java-tck", federation);
         observerJoined = true;

         reportClass = observer.getInteractionClassHandle(REPORT_CLASS);
         JavaTckSupport.check(reportClass != null
               && REPORT_CLASS.equals(observer.getInteractionClassName(reportClass)),
            "standard HLAreportServiceInvocation lookup/name round-trip failed");
         Map<String, ParameterHandle> reportParameters = new LinkedHashMap<>();
         for (String name : REPORT_PARAMETERS) {
            ParameterHandle parameter = observer.getParameterHandle(reportClass, name);
            JavaTckSupport.check(parameter != null
                  && name.equals(observer.getParameterName(reportClass, parameter)),
               "service-report parameter lookup/name round-trip failed for " + name);
            reportParameters.put(name, parameter);
         }
         TransportationTypeHandle reliable = observer.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliable != null
               && "HLAreliable".equals(observer.getTransportationTypeName(reliable)),
            "standard reliable transportation lookup did not round-trip");

         subject.setServiceReportingSwitch(false);
         subject.setSendServiceReportsToFileSwitch(false);
         observer.setServiceReportingSwitch(false);
         observer.setSendServiceReportsToFileSwitch(false);
         observer.subscribeInteractionClass(reportClass);
         reportSubscribed = true;

         DimensionHandle dimension = subject.getDimensionHandle(dimensionName);
         JavaTckSupport.check(dimension != null,
            "adapter FOM dimension-name lookup returned a null handle");
         ObjectClassHandle objectClass = subject.getObjectClassHandle(objectClassName);
         JavaTckSupport.check(objectClass != null
               && objectClassName.equals(subject.getObjectClassName(objectClass)),
            "adapter FOM object-class lookup/name round-trip failed");
         JavaTckSupport.check(!subject.getSendServiceReportsToFileSwitch(),
            "object-dimensions test could not select MOM delivery");

         subject.setServiceReportingSwitch(true);
         DimensionHandleSet availableDimensions =
            subject.getAvailableDimensionsForObjectClass(objectClass);
         JavaTckSupport.check(availableDimensions != null
               && availableDimensions.size() == 1 && availableDimensions.contains(dimension),
            "adapter FOM object class did not expose exactly its configured dimension");
         awaitOne(observer, recorder, callbackModel);
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(observer);
         JavaTckSupport.check(recorder.events.size() == 1,
            "GetAvailableDimensionsForObjectClass delivered an unexpected number of service reports");
         verifyReport(observer, subjectFederate, recorder.events.get(0), encoder, reportClass,
            reportParameters, reliable, "GetAvailableDimensionsForObjectClass",
            new Argument[] {
               new Argument(36, "Object class handle", quoted(objectClass.toString()))
            }, new Argument(11, "A set of dimension handles",
               "[\"" + dimension.toString() + "\"]"), 0);

         observer.unsubscribeInteractionClass(reportClass);
         reportSubscribed = false;
         subject.setServiceReportingSwitch(false);
         subject.resignFederationExecution(ResignAction.NO_ACTION);
         subjectJoined = false;
         observer.resignFederationExecution(ResignAction.NO_ACTION);
         observerJoined = false;
         subject.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reportSubscribed && observerJoined && reportClass != null) {
            try { observer.unsubscribeInteractionClass(reportClass); }
            catch (Exception ignored) { }
         }
         try { subject.setServiceReportingSwitch(false); } catch (Exception ignored) { }
         if (observerJoined) resign(observer);
         if (subjectJoined) resign(subject);
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) disconnect(observer);
         if (subjectConnected) disconnect(subject);
      }
   }

   private static void verifyReport(RTIambassador observer, FederateHandle expectedFederate,
         Interaction event, EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> parameters, TransportationTypeHandle reliable,
         String service, Argument[] expectedSupplied, Argument expectedReturned, int serial)
         throws Exception {
      verifyReport(observer, expectedFederate, event, encoder, reportClass, parameters,
         reliable, service, expectedSupplied, expectedReturned, serial, 6);
   }

   private static void verifyReport(RTIambassador observer, FederateHandle expectedFederate,
         Interaction event, EncoderFactory encoder, InteractionClassHandle reportClass,
         Map<String, ParameterHandle> parameters, TransportationTypeHandle reliable,
         String service, Argument[] expectedSupplied, Argument expectedReturned, int serial,
         int expectedServiceType) throws Exception {
      JavaTckSupport.check(reportClass.equals(event.interaction)
            && event.parameters.size() == REPORT_PARAMETERS.length
            && event.tag.length == 0 && event.producer == null && event.regionsEmpty
            && reliable.equals(event.transportation),
         service + " report had unexpected interaction, metadata, or parameter count");
      for (String name : REPORT_PARAMETERS) {
         JavaTckSupport.check(event.parameters.containsKey(parameters.get(name)),
            service + " report omitted standard parameter " + name);
      }
      HLAunicodeString decodedService = encoder.createHLAunicodeString();
      decodedService.decode(event.parameters.get(parameters.get("HLAservice")));
      HLAinteger16BE serviceType = encoder.createHLAinteger16BE();
      serviceType.decode(event.parameters.get(parameters.get("HLAserviceType")));
      HLAboolean success = encoder.createHLAboolean();
      success.decode(event.parameters.get(parameters.get("HLAsuccessIndicator")));
      HLAunicodeString exception = encoder.createHLAunicodeString();
      exception.decode(event.parameters.get(parameters.get("HLAexception")));
      HLAinteger32BE serialNumber = encoder.createHLAinteger32BE();
      serialNumber.decode(event.parameters.get(parameters.get("HLAserialNumber")));
      JavaTckSupport.check(service.equals(decodedService.getValue())
            && serviceType.getValue() == expectedServiceType && success.getValue()
            && exception.getValue().isEmpty() && serialNumber.getValue() == serial,
         service + " report returned incorrect status, exception, or serial number");
      FederateHandle reportedFederate = observer.getFederateHandleFactory().decode(
         event.parameters.get(parameters.get("HLAfederate")), 0);
      JavaTckSupport.check(expectedFederate.equals(reportedFederate),
         service + " report identified the wrong joined federate");

      HLAfixedRecord prototype = argumentPrototype(encoder);
      HLAvariableArray<HLAfixedRecord> supplied = encoder.createHLAvariableArray(
         index -> argumentPrototype(encoder), prototype);
      supplied.decode(event.parameters.get(parameters.get("HLAsuppliedArguments")));
      JavaTckSupport.check(supplied.size() == expectedSupplied.length,
         service + " report returned the wrong supplied-argument count");
      for (int index = 0; index < expectedSupplied.length; index++) {
         verifyArgument(supplied.get(index), expectedSupplied[index],
            service + " supplied argument " + index);
      }
      HLAfixedRecord returned = argumentPrototype(encoder);
      returned.decode(event.parameters.get(parameters.get("HLAreturnedArgument")));
      verifyArgument(returned, expectedReturned, service + " returned argument");
   }

   private static void verifyArgument(HLAfixedRecord record, Argument expected,
         String description) throws Exception {
      JavaTckSupport.check(record.size() == 3
            && ((HLAinteger32BE) record.get(0)).getValue() == expected.type
            && expected.name.equals(((HLAunicodeString) record.get(1)).getValue())
            && expected.value.equals(((HLAunicodeString) record.get(2)).getValue()),
         description + " had the wrong argument type, name, or value");
   }

   private static HLAfixedRecord argumentPrototype(EncoderFactory encoder) {
      HLAfixedRecord result = encoder.createHLAfixedRecord();
      result.add(encoder.createHLAinteger32BE());
      result.add(encoder.createHLAunicodeString());
      result.add(encoder.createHLAunicodeString());
      return result;
   }

   private static String formatRate(double value) {
      return String.format(Locale.ROOT, "%.6f", value);
   }

   private static String quoted(String value) { return "\"" + value + "\""; }

   private static void await(RTIambassador observer, Recorder recorder, CallbackModel model)
         throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (recorder.events.size() < 7 && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) observer.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(recorder.events.size() >= 7,
         "timed out waiting for object/attribute/update-rate service reports");
   }

   private static void awaitOne(RTIambassador observer, Recorder recorder, CallbackModel model)
         throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (recorder.events.isEmpty() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) observer.evokeCallback(0.01);
         else Thread.yield();
      }
      JavaTckSupport.check(!recorder.events.isEmpty(),
         "timed out waiting for GetDimensionUpperBound service report");
   }

   private static FederateAmbassador emptyCallbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> objectMethod(proxy, method.getName(), arguments));
   }

   private static Object objectMethod(Object proxy, String name, Object[] arguments) {
      if ("toString".equals(name)) return "Java TCK object lookup callbacks";
      if ("hashCode".equals(name)) return System.identityHashCode(proxy);
      if ("equals".equals(name)) return proxy == arguments[0];
      return null;
   }

   private static void resign(RTIambassador rti) {
      try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Argument {
      private final int type;
      private final String name;
      private final String value;
      private Argument(int type, String name, String value) {
         this.type = type;
         this.name = name;
         this.value = value;
      }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final Map<ParameterHandle, byte[]> parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;
      private Interaction(InteractionClassHandle interaction,
            Map<ParameterHandle, byte[]> parameters, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
            boolean regionsEmpty) {
         this.interaction = interaction;
         this.parameters = parameters;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regionsEmpty;
      }
   }

   private static final class Recorder {
      private final List<Interaction> events = new CopyOnWriteArrayList<>();
      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               if ("receiveInteraction".equals(method.getName()) && arguments != null
                     && arguments.length == 6) {
                  ParameterHandleValueMap source = (ParameterHandleValueMap) arguments[1];
                  Map<ParameterHandle, byte[]> values = new LinkedHashMap<>();
                  for (ParameterHandle handle : source.keySet()) {
                     values.put(handle, source.get(handle).clone());
                  }
                  RegionHandleSet regions = (RegionHandleSet) arguments[5];
                  events.add(new Interaction((InteractionClassHandle) arguments[0], values,
                     ((byte[]) arguments[2]).clone(), (TransportationTypeHandle) arguments[3],
                     (FederateHandle) arguments[4], regions == null || regions.isEmpty()));
               }
               return objectMethod(proxy, method.getName(), arguments);
            });
      }
   }
}
