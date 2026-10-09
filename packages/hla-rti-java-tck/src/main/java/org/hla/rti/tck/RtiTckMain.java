package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.RtiFactoryFactory;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import hla.rti1516_2025.auth.HLAnoCredentials;
import java.io.IOException;
import java.io.StringWriter;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import java.util.UUID;

/** Standard HLA Java contract tests. No provider implementation types appear here. */
public final class RtiTckMain {
   private static final String FACTORY_PROPERTY = "hla.rti.tck.factory";
   private static final String FOM_PROPERTY = "hla.rti.tck.fom";
   private static final String MIM_PROPERTY = "hla.rti.tck.mim";
   private static final String ADDITIONAL_FOM_PROPERTY = "hla.rti.tck.additionalFom";
   private static final String TIME_PROPERTY = "hla.rti.tck.time";
   private static final String CALLBACK_MODEL_PROPERTY = "hla.rti.tck.callbackModel";
   private static final String OBJECT_CLASS_PROPERTY = "hla.rti.tck.objectClass";
   private static final String ATTRIBUTE_PROPERTY = "hla.rti.tck.attribute";
   private static final String INTERACTION_CLASS_PROPERTY = "hla.rti.tck.interactionClass";
   private static final String DIRECTED_INTERACTION_CLASS_PROPERTY =
      "hla.rti.tck.directedInteractionClass";
   private static final String TIMESTAMPED_DIRECTED_OBJECT_CLASS_PROPERTY =
      "hla.rti.tck.timestampedDirectedObjectClass";
   private static final String TIMESTAMPED_DIRECTED_ATTRIBUTE_PROPERTY =
      "hla.rti.tck.timestampedDirectedAttribute";
   private static final String TIMESTAMPED_DIRECTED_INTERACTION_CLASS_PROPERTY =
      "hla.rti.tck.timestampedDirectedInteractionClass";
   private static final String TIMESTAMPED_DIRECTED_PARAMETER_PROPERTY =
      "hla.rti.tck.timestampedDirectedParameter";
   private static final String PARAMETER_PROPERTY = "hla.rti.tck.parameter";
   private static final String DDM_OBJECT_CLASS_PROPERTY = "hla.rti.tck.ddmObjectClass";
   private static final String DDM_ATTRIBUTE_PROPERTY = "hla.rti.tck.ddmAttribute";
   private static final String DIMENSION_PROPERTY = "hla.rti.tck.dimension";
   private static final String PROFILE_PROPERTY = "hla.rti.tck.capabilityProfile";
   private static final String SCENARIO_PROPERTY = "hla.rti.tck.scenario";
   private static final String STACKTRACE_PROPERTY = "hla.rti.tck.stacktrace";
   private static final String RESULTS_PROPERTY = "hla.rti.tck.results";
   private static final String JUNIT_PROPERTY = "hla.rti.tck.junit";
   private static final String PROVIDER_PROPERTY = "hla.rti.tck.provider";
   private static final String API_JAR_PROPERTY = "hla.rti.tck.apiJar";
   private static final String PROVIDER_JARS_PROPERTY = "hla.rti.tck.providerJars";
   private static final List<TestResult> results = new ArrayList<>();
   private static Properties capabilityProfile;
   private static String discoveredProvider = "unknown";

   private RtiTckMain() {
   }

   public static void main(String[] arguments) throws Exception {
      run(new Scenario("java-tck.factory-discovery", "factory discovery", "lifecycle",
            "RtiFactoryFactory.getRtiFactory", new String[] {
               "req-connect-service-establishes-connection"
            }, true), RtiTckMain::factoryDiscovery);
      run(new Scenario("java-tck.encoder-round-trip", "standard basic data elements", "encoders",
            "EncoderFactory.createHLAinteger16BE;EncoderFactory.createHLAinteger64LE;EncoderFactory.createHLAunicodeString;EncoderFactory.createHLAopaqueData;DataElement.decode;DataElement.toByteArray", new String[] {
               "java-1516.1-encoding-primitive-round-trip",
               "java-1516.1-encoding-malformed-input"
            }, true), RtiTckMain::encoderRoundTrip);
      run(new Scenario("java-tck.federation-membership", "federation membership and queries", "lifecycle",
            "RTIambassador.connect;createFederationExecution;destroyFederationExecution;listFederationExecutions;listFederationExecutionMembers;joinFederationExecution(4 overloads);getFederateHandle;getFederateName;resignFederationExecution;disconnect;FederateAmbassador.reportFederationExecutions;reportFederationExecutionMembers;reportFederationExecutionDoesNotExist",
            new String[] {
               "req-federate-connect-before-rti-work",
               "req-connect-service-establishes-connection",
               "req-federate-disconnect-after-resign",
               "req-disconnect-service-terminates-connection"
            }, true), RtiTckMain::federationMembership);
      run(new Scenario("java-tck.explicit-mim-creation",
            "explicit MIM federation creation and shared standard declarations", "fom-composition",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {"cpp-1516.1-explicit-mim-composition"}, false),
         RtiTckMain::explicitMimCreation);
      run(new Scenario("java-tck.federation-mom-current-fdd",
            "federation MOM current FDD composition and refresh", "federation-mom",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution(additional FOM);RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.subscribeObjectClassAttributes;RTIambassador.getKnownObjectClassHandle;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.queryAttributeTransportationType;RTIambassador.requestAttributeValueUpdate;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reportAttributeTransportationType;FederateAmbassador.reflectAttributeValues;HLAunicodeString.decode",
            new String[] {"cpp-1516.1-federation-mom"}, false),
         RtiTckMain::federationMomCurrentFdd);
      run(new Scenario("java-tck.federation-mom-fom-module-designator-list",
            "federation MOM FOM-module designator list", "federation-mom",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution(additional FOM);RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.getTransportationTypeHandle;RTIambassador.getTransportationTypeName;RTIambassador.subscribeObjectClassAttributes;RTIambassador.getKnownObjectClassHandle;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.requestAttributeValueUpdate;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues;HLAvariableArray.decode;HLAunicodeString.decode",
            new String[] {
               "cpp-1516.1-federation-mom",
               "cpp-1516.1-fom-module-composition",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::federationMomFomModuleDesignatorList);
      run(new Scenario("java-tck.federation-mom-static-identity",
            "federation MOM standard static identity values", "federation-mom",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.getTransportationTypeHandle;RTIambassador.getTransportationTypeName;RTIambassador.subscribeObjectClassAttributes;RTIambassador.getKnownObjectClassHandle;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.requestAttributeValueUpdate;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues;HLAunicodeString.decode",
            new String[] {
               "cpp-1516.1-federation-mom",
               "cpp-1516.1-federation-create-join-resign-destroy",
               "cpp-1516.1-attribute-update-reflect"
            }, false), RtiTckMain::federationMomStaticIdentity);
      run(new Scenario("java-tck.federation-mom-static-switches",
            "federation MOM standard static switch values", "federation-mom",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.getTransportationTypeHandle;RTIambassador.getTransportationTypeName;RTIambassador.subscribeObjectClassAttributes;RTIambassador.getKnownObjectClassHandle;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.requestAttributeValueUpdate;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues;HLAinteger32BE.decode",
            new String[] {
               "cpp-1516.1-federation-mom",
               "cpp-1516.1-federation-create-join-resign-destroy",
               "cpp-1516.1-attribute-update-reflect"
            }, false), RtiTckMain::federationMomStaticSwitches);
      run(new Scenario("java-tck.federation-mom-federates-in-federation",
            "federation MOM conditional federate roster", "federation-mom",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.getTransportationTypeHandle;RTIambassador.getTransportationTypeName;RTIambassador.subscribeObjectClassAttributes;RTIambassador.getKnownObjectClassHandle;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.getFederateHandleFactory;RTIambassador.requestAttributeValueUpdate;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateHandleFactory.decode;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues;HLAvariableArray.decode;HLAbyte",
            new String[] {
               "cpp-1516.1-federation-mom",
               "cpp-1516.1-federation-create-join-resign-destroy",
               "cpp-1516.1-attribute-update-reflect"
            }, false), RtiTckMain::federationMomFederatesInFederation);
      run(new Scenario("java-tck.federation-mom-content-reports",
            "federation MOM FOM-module and MIM content reports", "federation-mom",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;RTIambassador.getInteractionClassName;RTIambassador.getParameterHandle;RTIambassador.getParameterName;RTIambassador.getTransportationTypeHandle;RTIambassador.getTransportationTypeName;RTIambassador.getParameterHandleValueMapFactory;RTIambassador.subscribeInteractionClass;RTIambassador.unsubscribeInteractionClass;RTIambassador.sendInteraction;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.receiveInteraction;HLAinteger32BE.decode;HLAunicodeString.decode",
            new String[] {
               "cpp-1516.1-federation-mom-content-reports",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::federationMomContentReports);
      run(new Scenario("java-tck.callback-reentrancy",
            "callback servicing rejects re-entry from a federate callback", "connection",
            "RTIambassador.connect;RTIambassador.listFederationExecutions;RTIambassador.evokeCallback;RTIambassador.evokeMultipleCallbacks;RTIambassador.disconnect;FederateAmbassador.reportFederationExecutions",
            new String[] {}, false), RtiTckMain::callbackReentrancy);
      run(new Scenario("java-tck.logical-time-factory", "logical-time factory", "time",
            "RTIambassador.getTimeFactory;LogicalTimeFactory.decodeInterval", new String[] {
               "java-1516.1-logical-time-factory-values"
            }, true), RtiTckMain::logicalTimeFactory);
      run(new Scenario("java-tck.declaration-management", "declaration management", "declarations",
            "getObjectClassHandle;getObjectClassName;getAttributeHandle;getAttributeName;getUpdateRateValue;publishObjectClassAttributes;unpublishObjectClassAttributes;unpublishObjectClass;subscribeObjectClassAttributes(active);subscribeObjectClassAttributesPassively;unsubscribeObjectClassAttributes;unsubscribeObjectClass",
            new String[] {
               "java-1516.1-object-class-attribute-declaration"
            }, true), RtiTckMain::declarationManagement);
      run(new Scenario("java-tck.object-management", "object registration and reflection", "object-management",
            "reserveObjectInstanceName;releaseObjectInstanceName;reserveMultipleObjectInstanceNames;releaseMultipleObjectInstanceNames;objectInstanceNameReservationSucceeded;multipleObjectInstanceNameReservationSucceeded;registerObjectInstance(named and unnamed);getKnownObjectClassHandle;getObjectInstanceHandle;getObjectInstanceName;requestAttributeValueUpdate(instance and class);discoverObjectInstance;updateAttributeValues;reflectAttributeValues;localDeleteObjectInstance;deleteObjectInstance;removeObjectInstance;provideAttributeValueUpdate",
            new String[] {
               "java-1516.1-object-registration-discovery",
               "java-1516.1-attribute-value-update-reflection"
            }, true), RtiTckMain::objectRegistrationAndReflection);
      run(new Scenario("java-tck.object-attribute-declarations-valid-inputs-after-resignation",
            "check object-attribute declaration membership boundaries after resignation",
            "declarations",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.publishObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.unpublishObjectClass;RTIambassador.subscribeObjectClassAttributes;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unsubscribeObjectClass;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "cpp-1516.1-publish-object-class-attributes-membership-boundary-after-resignation",
               "cpp-1516.1-unpublish-object-class-membership-boundary-after-resignation",
               "cpp-1516.1-unpublish-object-class-attributes-membership-boundary-after-resignation",
               "cpp-1516.1-subscribe-object-class-attributes-membership-boundary-after-resignation",
               "cpp-1516.1-unsubscribe-object-class-membership-boundary-after-resignation",
               "cpp-1516.1-unsubscribe-object-class-attributes-membership-boundary-after-resignation"
            }, false), RtiTckMain::objectAttributeDeclarationsAfterResignation);
       run(new Scenario("java-tck.object-registration-discovery-lifecycle",
             "hierarchy-aware object discovery and callback-time subscription eligibility",
             "object-management",
             "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.getKnownObjectClassHandle;RTIambassador.getObjectInstanceHandle;RTIambassador.getObjectInstanceName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;RTIambassador.evokeCallback;FederateAmbassador.discoverObjectInstance",
             new String[] {"java-1516.1-object-registration-discovery"}, false),
          RtiTckMain::objectRegistrationDiscoveryLifecycle);
      run(new Scenario("java-tck.attribute-value-update-request-baseline",
            "instance-scoped attribute-value request and provider callback",
            "object-management",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.requestAttributeValueUpdate(instance);RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.provideAttributeValueUpdate;RTIambassador.evokeCallback",
            new String[] {
               "cpp-1516.1-object-registration-discovery",
               "cpp-1516.1-attribute-value-update-request"
            }, false), RtiTckMain::attributeValueUpdateRequestBaseline);
      run(new Scenario("java-tck.object-class-attribute-value-update-request-baseline",
            "expand a base-class attribute-value request over derived instances",
            "object-management",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.requestAttributeValueUpdate(class);RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;RTIambassador.evokeCallback;FederateAmbassador.discoverObjectInstance;FederateAmbassador.provideAttributeValueUpdate",
            new String[] {
               "cpp-1516.1-object-registration-discovery",
               "cpp-1516.1-attribute-value-update-request"
            }, false), RtiTckMain::objectClassAttributeValueUpdateRequestBaseline);
      run(new Scenario("java-tck.attribute-value-update-response",
            "answer an instance-scoped value request and verify receive-order reflection metadata",
            "object-management",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getTransportationTypeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.requestAttributeValueUpdate(instance);RTIambassador.updateAttributeValues;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;RTIambassador.evokeCallback;FederateAmbassador.discoverObjectInstance;FederateAmbassador.provideAttributeValueUpdate;FederateAmbassador.reflectAttributeValues",
            new String[] {
               "cpp-1516.1-object-registration-discovery",
               "cpp-1516.1-attribute-value-update-request",
               "cpp-1516.1-attribute-value-update-response",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::attributeValueUpdateResponse);
      run(new Scenario("java-tck.attribute-value-update-request-multi-requester",
            "deliver independent value requests to one provider and fan out its response",
            "object-management",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.getKnownObjectClassHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.requestAttributeValueUpdate(instance);RTIambassador.updateAttributeValues;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;RTIambassador.evokeCallback;FederateAmbassador.discoverObjectInstance;FederateAmbassador.provideAttributeValueUpdate;FederateAmbassador.reflectAttributeValues",
            new String[] {
               "cpp-1516.1-object-management",
               "cpp-1516.1-object-registration-discovery",
               "cpp-1516.1-attribute-value-update-request",
               "cpp-1516.1-attribute-value-update-response",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::attributeValueUpdateMultiRequester);
      run(new Scenario("java-tck.object-registration-discovery-multi-recipient",
            "deliver two ordinary object discoveries to two active subscribers and preserve identity",
            "object-management",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.getKnownObjectClassHandle;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;RTIambassador.evokeCallback;FederateAmbassador.discoverObjectInstance",
            new String[] {
               "cpp-1516.1-object-management",
               "cpp-1516.1-object-registration-discovery",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::objectRegistrationMultiRecipient);
      run(new Scenario("java-tck.ordinary-multi-attribute-subscription-projection",
            "project ordinary multi-attribute updates to each subscriber's declared attributes",
            "object-management",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.updateAttributeValues;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;RTIambassador.evokeCallback;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues",
            new String[] {
               "cpp-1516.1-attribute-update-reflect",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::multiAttributeSubscriptionProjection);
      run(new Scenario("java-tck.ordinary-multi-attribute-value-update-request-response",
            "request full and selective multi-attribute values and verify standard responses",
            "object-management",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getTransportationTypeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.requestAttributeValueUpdate(instance);RTIambassador.updateAttributeValues;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;RTIambassador.evokeCallback;FederateAmbassador.discoverObjectInstance;FederateAmbassador.provideAttributeValueUpdate;FederateAmbassador.reflectAttributeValues",
            new String[] {
               "cpp-1516.1-attribute-value-update-request",
               "cpp-1516.1-attribute-update-reflect",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::multiAttributeValueUpdateRequestResponse);
      run(new Scenario("java-tck.object-removal-multi-recipient-fifo",
            "deliver ordinary object removals to two active subscribers without owner loopback",
            "object-management",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.deleteObjectInstance;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;RTIambassador.evokeCallback;FederateAmbassador.discoverObjectInstance;FederateAmbassador.removeObjectInstance",
            new String[] {
               "cpp-1516.1-object-management",
               "cpp-1516.1-object-registration-discovery",
               "cpp-1516.1-object-instance-deletion",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, true), RtiTckMain::objectRemovalMultiRecipientFifo);
      run(new Scenario("java-tck.object-deletion-service-boundaries",
            "exercise ordinary object-deletion service boundaries and cleanup",
            "object-management",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.localDeleteObjectInstance;RTIambassador.deleteObjectInstance;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.removeObjectInstance",
            new String[] {
               "cpp-1516.1-object-registration-discovery",
               "cpp-1516.1-object-instance-deletion",
               "cpp-1516.1-object-deletion-negative-boundaries",
               "cpp-1516.1-local-object-instance-deletion",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::objectDeletionServiceBoundaries);
      run(new Scenario("java-tck.receive-order-object-removal-subscription-withdrawal",
            "deliver a queued terminal object removal after subscription withdrawal",
            "object-management",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.deleteObjectInstance;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;RTIambassador.evokeCallback;FederateAmbassador.discoverObjectInstance;FederateAmbassador.removeObjectInstance",
            new String[] {
               "cpp-1516.1-object-management",
               "cpp-1516.1-object-registration-discovery",
               "cpp-1516.1-object-instance-deletion",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::receiveOrderObjectRemovalSubscriptionWithdrawal);
      run(new Scenario("java-tck.resign-delete-objects-multi-recipient-fifo",
            "deliver resign-time object removals to two active subscribers in producer order",
            "object-management",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.resignFederationExecution;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;RTIambassador.evokeCallback;FederateAmbassador.discoverObjectInstance;FederateAmbassador.removeObjectInstance",
            new String[] {
               "cpp-1516.1-federation-create-join-resign-destroy",
               "cpp-1516.1-object-management",
               "cpp-1516.1-object-registration-discovery",
               "cpp-1516.1-object-instance-deletion"
            }, true), RtiTckMain::resignDeleteObjectsMultiRecipientFifo);
      run(new Scenario("java-tck.resign-delete-objects",
            "enforce NO_ACTION ownership failure and removal delivery with DELETE_OBJECTS",
            "object-management",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.resignFederationExecution;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;RTIambassador.evokeCallback;FederateAmbassador.discoverObjectInstance;FederateAmbassador.removeObjectInstance",
            new String[] {
               "cpp-1516.1-federation-create-join-resign-destroy",
               "cpp-1516.1-object-instance-deletion",
               "cpp-1516.1-object-deletion-negative-boundaries"
            }, true), RtiTckMain::resignDeleteObjects);
      run(new Scenario("java-tck.attribute-multi-recipient-fifo",
            "deliver ordinary attribute updates to two active subscribers in producer order",
            "object-management",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getTransportationTypeName;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.updateAttributeValues;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;RTIambassador.evokeCallback;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues",
            new String[] {
               "cpp-1516.1-attribute-update-reflect",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::attributeMultiRecipientFifo);
      run(new Scenario("java-tck.attribute-interaction", "ordinary attribute and interaction routes", "interaction-management",
            "getObjectClassHandle;getObjectClassName;getAttributeHandle;getAttributeName;getUpdateRateValue;publishObjectClassAttributes;unpublishObjectClassAttributes;unpublishObjectClass;subscribeObjectClassAttributes;subscribeObjectClassAttributesPassively;unsubscribeObjectClassAttributes;unsubscribeObjectClass;getInteractionClassHandle;getInteractionClassName;getParameterHandle;getParameterName;publishInteractionClass;unpublishInteractionClass;subscribeInteractionClass;subscribeInteractionClassPassively;unsubscribeInteractionClass;updateAttributeValues;reflectAttributeValues;sendInteraction;receiveInteraction;enableCallbacks;disableCallbacks;discoverObjectInstance",
            new String[] {
               "java-1516.1-attribute-update-and-callback-control",
               "java-1516.1-interaction-declaration-and-delivery"
            }, true), RtiTckMain::attributeInteraction);
      run(new Scenario("java-tck.support-services", "support lookups and handle identity", "support-services",
            "getOrderType;getOrderName;getTransportationTypeHandle;getTransportationTypeName;getAvailableDimensionsForObjectClass;getAvailableDimensionsForInteractionClass;getDimensionName;getDimensionUpperBound;normalizeServiceGroup;normalizeFederateHandle;normalizeObjectClassHandle;normalizeInteractionClassHandle;normalizeObjectInstanceHandle;handle-factory.decode",
            new String[] {
               "java-1516.1-support-lookups-and-handle-identity",
               "java-1516.1-order-type-lookup",
               "java-1516.1-transportation-type-lookup",
               "java-1516.1-dimension-lookup",
               "java-1516.1-handle-normalization"
            }, true), RtiTckMain::supportServices);
      run(new Scenario("java-tck.ordinary-edges", "ordinary edge and negative matrices", "interaction-management",
            "idempotent declaration transitions;ordinary empty passels;map replacement;invalid handles;nonpublisher send;nonowner update;unknown object queries",
            new String[] {
               "java-1516.1-ordinary-edge-matrix",
               "java-1516.1-standard-exception-boundary"
            }, true), RtiTckMain::ordinaryEdges);
      run(new Scenario("java-tck.relevance-advisories", "ordinary relevance advisories", "declarations",
            "get/setObjectClassRelevanceAdvisorySwitch;get/setAttributeRelevanceAdvisorySwitch;get/setInteractionRelevanceAdvisorySwitch;startRegistrationForObjectClass;stopRegistrationForObjectClass;turnInteractionsOn;turnInteractionsOff;turnUpdatesOnForObjectInstance;turnUpdatesOffForObjectInstance",
            new String[] {
               "java-1516.1-declaration-relevance-advisories",
               "java-1516.1-attribute-relevance-advisories"
            }, true), RtiTckMain::relevanceAdvisories);
      run(new Scenario("java-tck.directed-interactions", "ordinary directed interactions", "interaction-management",
            "publishObjectClassDirectedInteractions;unpublishObjectClassDirectedInteractions;subscribeObjectClassDirectedInteractions;subscribeObjectClassDirectedInteractionsUniversally;unsubscribeObjectClassDirectedInteractions;sendDirectedInteraction;receiveDirectedInteraction",
            new String[] {
               "java-1516.1-directed-interaction-publication",
               "java-1516.1-directed-interaction-subscription",
               "java-1516.1-directed-interaction-delivery"
            }, true), RtiTckMain::directedInteractions);
      run(new Scenario("java-tck.directed-interaction-declarations-valid-inputs-after-resignation",
            "check directed-interaction declaration membership boundaries after resignation",
            "directed-interaction",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getInteractionClassHandle;RTIambassador.getInteractionClassName;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassDirectedInteractions(set and whole class);RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.unsubscribeObjectClassDirectedInteractions(set and whole class);RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "cpp-1516.1-publish-object-class-directed-interactions-membership-boundary-after-resignation",
               "cpp-1516.1-unpublish-object-class-directed-interactions-set-membership-boundary-after-resignation",
               "cpp-1516.1-unpublish-object-class-directed-interactions-whole-class-membership-boundary-after-resignation",
               "cpp-1516.1-subscribe-object-class-directed-interactions-membership-boundary-after-resignation",
               "cpp-1516.1-unsubscribe-object-class-directed-interactions-set-membership-boundary-after-resignation",
               "cpp-1516.1-unsubscribe-object-class-directed-interactions-whole-class-membership-boundary-after-resignation"
            }, false), RtiTckMain::directedInteractionDeclarationsAfterResignation);
      run(new Scenario("java-tck.directed-interaction-target-lifecycle",
            "directed interaction delivery across target deletion", "interaction-management",
            "RTIambassador.publishObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassAttributes;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.registerObjectInstance;RTIambassador.sendDirectedInteraction;RTIambassador.deleteObjectInstance;RTIambassador.unsubscribeObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassDirectedInteractions;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction;FederateAmbassador.removeObjectInstance",
            new String[] {
               "java-1516.1-directed-interaction-delivery",
               "java-1516.1-object-registration-discovery"
            }, false), RtiTckMain::directedInteractionTargetLifecycle);
      run(new Scenario("java-tck.directed-interaction-derived-object-target",
            "directed interaction to a derived object target", "interaction-management",
            "RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.registerObjectInstance;RTIambassador.sendDirectedInteraction;RTIambassador.getTransportationTypeName;RTIambassador.unsubscribeObjectClassDirectedInteractions;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction",
            new String[] {
               "java-1516.1-directed-interaction-delivery",
               "java-1516.1-object-registration-discovery"
            }, true), RtiTckMain::directedInteractionDerivedObjectTarget);
      run(new Scenario("java-tck.transport-order", "ordinary transportation and order controls", "transportation-order",
            "changeDefaultAttributeOrderType;changeAttributeOrderType;changeInteractionOrderType;changeDefaultAttributeTransportationType;requestAttributeTransportationTypeChange;queryAttributeTransportationType;requestInteractionTransportationTypeChange;queryInteractionTransportationType;confirmAttributeTransportationTypeChange;reportAttributeTransportationType;confirmInteractionTransportationTypeChange;reportInteractionTransportationType",
            new String[] {
               "java-1516.1-order-type-control",
               "java-1516.1-transportation-type-control"
            }, true), RtiTckMain::transportOrder);
      run(new Scenario("java-tck.receive-order-attribute-update-callback-cancellation",
            "cancel a queued receive-order reflection when its active subscription is withdrawn",
            "attribute-value-update",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.updateAttributeValues;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues",
            new String[] {
               "java-1516.1-attribute-value-update-reflection",
               "java-1516.1-object-registration-discovery"
            }, false), RtiTckMain::receiveOrderAttributeUpdateCallbackCancellation);
       run(new Scenario("java-tck.receive-order-multi-attribute-update-callback-cancellation",
             "cancel a queued multi-attribute receive-order reflection after subscription withdrawal",
             "attribute-value-update",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.updateAttributeValues;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.getTransportationTypeName;RTIambassador.evokeCallback;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues",
             new String[] {
                "java-1516.1-attribute-value-update-reflection",
                "java-1516.1-object-registration-discovery"
             }, false), RtiTckMain::receiveOrderMultiAttributeUpdateCallbackCancellation);
       run(new Scenario("java-tck.receive-order-interaction-callback-cancellation",
             "cancel a queued receive-order interaction after its subscription is withdrawn",
             "interaction-management",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishInteractionClass;RTIambassador.subscribeInteractionClass;RTIambassador.sendInteraction;RTIambassador.unsubscribeInteractionClass;RTIambassador.unpublishInteractionClass;RTIambassador.getTransportationTypeName;RTIambassador.evokeCallback;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.receiveInteraction",
             new String[] {
                "java-1516.1-interaction-declaration-and-delivery",
                "java-1516.1-interaction-parameter-delivery"
             }, false), RtiTckMain::receiveOrderInteractionCallbackCancellation);
       run(new Scenario("java-tck.interaction-subscription-lifecycle",
             "exercise passive, active, downgraded, and withdrawn ordinary interaction subscriptions",
             "interaction-management",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishInteractionClass;RTIambassador.subscribeInteractionClassPassively;RTIambassador.subscribeInteractionClass;RTIambassador.sendInteraction;RTIambassador.unsubscribeInteractionClass;RTIambassador.unpublishInteractionClass;RTIambassador.getTransportationTypeName;RTIambassador.evokeCallback;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.receiveInteraction",
             new String[] {
                "java-1516.1-interaction-declaration-and-delivery",
                "java-1516.1-interaction-parameter-delivery"
             }, false), RtiTckMain::interactionSubscriptionLifecycle);
       run(new Scenario("java-tck.interaction-publication-send-fence",
             "fence ordinary interaction sends across unpublication and republication",
             "interaction-management",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishInteractionClass;RTIambassador.unpublishInteractionClass;RTIambassador.subscribeInteractionClass;RTIambassador.sendInteraction;RTIambassador.unsubscribeInteractionClass;RTIambassador.getTransportationTypeName;RTIambassador.evokeCallback;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.receiveInteraction",
             new String[] {
                "java-1516.1-interaction-declaration-and-delivery",
                "java-1516.1-interaction-parameter-delivery"
             }, false), RtiTckMain::interactionPublicationSendFence);
       run(new Scenario("java-tck.receive-order-interaction",
             "deliver consecutive ordinary receive-order interactions in producer order",
             "interaction-management",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;RTIambassador.getInteractionClassName;RTIambassador.getParameterHandle;RTIambassador.getParameterName;RTIambassador.publishInteractionClass;RTIambassador.subscribeInteractionClass;RTIambassador.sendInteraction;RTIambassador.getTransportationTypeName;RTIambassador.unsubscribeInteractionClass;RTIambassador.unpublishInteractionClass;RTIambassador.evokeCallback;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.receiveInteraction",
             new String[] {
                "java-1516.1-interaction-declaration-and-delivery",
                "java-1516.1-interaction-parameter-delivery"
             }, false), RtiTckMain::receiveOrderInteraction);
       run(new Scenario("java-tck.interaction-publication-send-transition",
             "verify ordinary sends before unpublication, failure while unpublished, and republish recovery",
             "interaction-management",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishInteractionClass;RTIambassador.subscribeInteractionClass;RTIambassador.sendInteraction;RTIambassador.unpublishInteractionClass;RTIambassador.getTransportationTypeName;RTIambassador.evokeCallback;RTIambassador.unsubscribeInteractionClass;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.receiveInteraction",
             new String[] {
                "java-1516.1-interaction-declaration-and-delivery",
                "java-1516.1-interaction-parameter-delivery"
             }, false), RtiTckMain::interactionPublicationSendTransition);
       run(new Scenario("java-tck.interaction-multi-recipient-fifo",
             "deliver ordinary receive-order interactions to two active subscribers in producer order",
             "interaction-management",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishInteractionClass;RTIambassador.subscribeInteractionClass;RTIambassador.sendInteraction;RTIambassador.getTransportationTypeName;RTIambassador.unsubscribeInteractionClass;RTIambassador.unpublishInteractionClass;RTIambassador.evokeCallback;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.receiveInteraction",
             new String[] {
                "java-1516.1-interaction-declaration-and-delivery",
                "java-1516.1-interaction-parameter-delivery"
             }, false), RtiTckMain::interactionMultiRecipientFifo);
       run(new Scenario("java-tck.directed-interaction-multi-recipient-fifo",
             "deliver ordinary directed interactions to two universal subscribers in producer order",
             "directed-interaction",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractionsUniversally;RTIambassador.registerObjectInstance;RTIambassador.sendDirectedInteraction;RTIambassador.getTransportationTypeName;RTIambassador.unsubscribeObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassDirectedInteractions;RTIambassador.unsubscribeObjectClass;RTIambassador.unpublishObjectClassAttributes;RTIambassador.evokeCallback;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction",
             new String[] {
                "java-1516.1-directed-interaction-publication",
                "java-1516.1-directed-interaction-subscription",
                "java-1516.1-directed-interaction-delivery",
                "java-1516.1-object-registration-discovery",
                "java-1516.1-interaction-parameter-delivery"
             }, false), RtiTckMain::directedInteractionMultiRecipientFifo);
       run(new Scenario("java-tck.directed-interaction-mixed-subscription-fanout",
             "route ordinary directed interactions by ownership and universal subscription kind",
             "directed-interaction",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractionsUniversally;RTIambassador.registerObjectInstance;RTIambassador.sendDirectedInteraction;RTIambassador.getTransportationTypeName;RTIambassador.unsubscribeObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassDirectedInteractions;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.evokeCallback;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction",
             new String[] {
                "java-1516.1-directed-interaction-publication",
                "java-1516.1-directed-interaction-subscription",
                "java-1516.1-directed-interaction-delivery",
                "java-1516.1-object-registration-discovery",
                "java-1516.1-interaction-parameter-delivery"
             }, false), RtiTckMain::directedInteractionMixedSubscriptionFanout);
      run(new Scenario("java-tck.directed-interaction-subscription-kind",
            "change directed subscription kinds between by-ownership and universal",
            "directed-interaction",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractionsUniversally;RTIambassador.registerObjectInstance;RTIambassador.sendDirectedInteraction;RTIambassador.unsubscribeObjectClassDirectedInteractions;RTIambassador.getTransportationTypeName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction",
            new String[] {
               "java-1516.1-directed-interaction-publication",
               "java-1516.1-directed-interaction-subscription",
               "java-1516.1-directed-interaction-delivery",
               "java-1516.1-object-registration-discovery",
               "java-1516.1-interaction-parameter-delivery"
         }, false), RtiTckMain::directedInteractionSubscriptionKind);
      run(new Scenario("java-tck.directed-derived-object-target-eligibility",
            "directed delivery eligibility for a derived-class target owner and universal subscriber",
            "directed-interaction",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractionsUniversally;RTIambassador.registerObjectInstance;RTIambassador.sendDirectedInteraction;RTIambassador.unsubscribeObjectClassDirectedInteractions;RTIambassador.getTransportationTypeName;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction",
            new String[] {
               "java-1516.1-directed-interaction-publication",
               "java-1516.1-directed-interaction-subscription",
               "java-1516.1-directed-interaction-delivery",
               "java-1516.1-object-registration-discovery",
               "java-1516.1-interaction-parameter-delivery"
            }, false), RtiTckMain::directedDerivedObjectTargetEligibility);
      run(new Scenario("java-tck.order-type-controls",
            "prospective attribute-order defaults, per-instance overrides, and receive-order interaction delivery",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishInteractionClass;RTIambassador.subscribeInteractionClass;RTIambassador.changeDefaultAttributeOrderType;RTIambassador.registerObjectInstance;RTIambassador.changeAttributeOrderType;RTIambassador.enableTimeConstrained;RTIambassador.enableAsynchronousDelivery;RTIambassador.enableTimeRegulation;RTIambassador.updateAttributeValues;RTIambassador.updateAttributeValues(time);RTIambassador.timeAdvanceRequest;RTIambassador.changeInteractionOrderType;RTIambassador.sendInteraction(time);RTIambassador.disableTimeConstrained;RTIambassador.disableTimeRegulation;RTIambassador.unsubscribeInteractionClass;RTIambassador.unpublishInteractionClass;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeConstrainedEnabled;FederateAmbassador.timeAdvanceGrant;FederateAmbassador.reflectAttributeValues;FederateAmbassador.reflectAttributeValues(time);FederateAmbassador.receiveInteraction(time)",
            new String[] {
               "java-1516.1-order-type-control",
               "java-1516.1-time-advance-and-grant",
               "java-1516.1-attribute-value-update-reflection",
               "java-1516.1-interaction-parameter-delivery"
            }, false), RtiTckMain::orderTypeControls);
      run(new Scenario("java-tck.api-surface-inventory", "standard API surface inventory", "api-surface",
            "RTIambassador.getMethods;FederateAmbassador.getMethods;EncoderFactory.getMethods",
            new String[] {
               "java-1516.1-api-surface-inventory"
            }, true), RtiTckMain::apiSurfaceInventory);
      run(new Scenario("java-tck.overloads-and-exceptions", "overloads and exceptions", "exceptions",
            "RTIambassador.connect(4 overloads);disconnect;standard exception types",
            new String[] {
               "java-1516.1-overload-dispatch",
               "java-1516.1-standard-exception-boundary"
            }, true), RtiTckMain::overloadsAndExceptions);
      run(new Scenario("java-tck.malformed-inputs", "malformed FOM and encoded input", "malformed-inputs",
            "createFederationExecution;DataElement.decode",
            new String[] {
               "java-1516.1-malformed-fom-input",
               "java-1516.1-malformed-encoded-input"
            }, true), RtiTckMain::malformedInputs);

      // These families are deliberately represented as capability-gated
      // scenarios.  A provider profile can enable them once the supplied FOM
      // and provider support the complete sequence.  The result remains an
      // explicit unsupported/not-applicable record instead of disappearing
      // from traceability.
      run(new Scenario("java-tck.ddm", "data distribution management", "ddm",
            "RegionHandle;createRegion;subscribeObjectClassAttributesWithRegions",
            new String[] {"java-1516.1-ddm-regional-delivery"}, false),
         RtiTckMain::dataDistributionManagement);
      run(new Scenario("java-tck.time-advance", "time advance and grants", "time",
            "enableTimeRegulation;enableTimeConstrained;timeAdvanceRequest;timeAdvanceRequestAvailable;nextMessageRequestAvailable;flushQueueRequest;timeAdvanceGrant;flushQueueGrant",
            new String[] {"java-1516.1-time-advance-and-grant"}, false),
         RtiTckMain::timeAdvanceAndGrants);
      run(new Scenario("java-tck.timestamped-interaction-mixed-advances",
            "timestamped interaction delivery through Flush Queue, TARA, and NMRA", "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.publishInteractionClass;RTIambassador.changeInteractionOrderType;RTIambassador.subscribeInteractionClass;RTIambassador.sendInteraction(time);RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.flushQueueRequest;RTIambassador.timeAdvanceRequestAvailable;RTIambassador.nextMessageRequestAvailable;RTIambassador.timeAdvanceRequest;RTIambassador.queryLogicalTime;FederateAmbassador.receiveInteraction(time);FederateAmbassador.flushQueueGrant;FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-interaction-declaration-and-delivery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedInteractionMixedAdvances);
       run(new Scenario("java-tck.timestamped-directed-alternate-advances",
             "timestamped directed delivery through Flush Queue, TARA, and NMRA", "time",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.registerObjectInstance;RTIambassador.changeInteractionOrderType;RTIambassador.getTimeFactory;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.sendDirectedInteraction(time);RTIambassador.flushQueueRequest;RTIambassador.timeAdvanceRequestAvailable;RTIambassador.nextMessageRequestAvailable;RTIambassador.timeAdvanceRequest;RTIambassador.queryLogicalTime;RTIambassador.getTransportationTypeName;RTIambassador.disableTimeConstrained;RTIambassador.disableTimeRegulation;RTIambassador.unsubscribeObjectClassDirectedInteractions;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeConstrainedEnabled;FederateAmbassador.receiveDirectedInteraction(time);FederateAmbassador.flushQueueGrant;FederateAmbassador.timeAdvanceGrant",
             new String[] {
                "java-1516.1-directed-interaction-delivery",
                "java-1516.1-time-advance-and-grant",
                "java-1516.1-object-registration-discovery"
             }, false), RtiTckMain::timestampedDirectedAlternateAdvances);
       run(new Scenario("java-tck.timestamped-interaction-retraction",
             "timestamped interaction delivery, Request Retraction, and terminal handles", "time",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.publishInteractionClass;RTIambassador.changeInteractionOrderType;RTIambassador.subscribeInteractionClass;RTIambassador.sendInteraction(time);RTIambassador.retract;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.timeAdvanceRequest;FederateAmbassador.receiveInteraction(time);FederateAmbassador.requestRetraction;FederateAmbassador.timeAdvanceGrant",
             new String[] {
                "java-1516.1-interaction-declaration-and-delivery",
                "java-1516.1-time-advance-and-grant"
             }, false), RtiTckMain::timestampedInteractionRetraction);
       run(new Scenario("java-tck.timestamped-interaction-cross-producer-order",
             "timestamp order across multiple producers and constrained receivers", "time",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.publishInteractionClass;RTIambassador.changeInteractionOrderType;RTIambassador.subscribeInteractionClass;RTIambassador.sendInteraction(time);RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.timeAdvanceRequest;RTIambassador.disableTimeRegulation;RTIambassador.disableTimeConstrained;FederateAmbassador.receiveInteraction(time);FederateAmbassador.timeAdvanceGrant",
             new String[] {
                "java-1516.1-interaction-declaration-and-delivery",
                "java-1516.1-time-advance-and-grant"
             }, false), RtiTckMain::timestampedInteractionCrossProducerOrder);
       run(new Scenario("java-tck.timestamped-interaction-no-fanout",
             "timestamped interaction retraction without eligible subscribers", "time",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.publishInteractionClass;RTIambassador.changeInteractionOrderType;RTIambassador.enableTimeRegulation;RTIambassador.sendInteraction(time);RTIambassador.retract;RTIambassador.timeAdvanceRequest;FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeAdvanceGrant",
             new String[] {
                "java-1516.1-interaction-declaration-and-delivery",
                "java-1516.1-time-advance-and-grant"
             }, false), RtiTckMain::timestampedInteractionNoFanout);
       run(new Scenario("java-tck.timestamped-interaction-retraction-fanout",
             "timestamped interaction retraction across delivered and queued recipients", "time",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.publishInteractionClass;RTIambassador.changeInteractionOrderType;RTIambassador.subscribeInteractionClass;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.sendInteraction(time);RTIambassador.retract;RTIambassador.timeAdvanceRequest;FederateAmbassador.receiveInteraction(time);FederateAmbassador.requestRetraction;FederateAmbassador.timeAdvanceGrant",
             new String[] {
                "java-1516.1-interaction-declaration-and-delivery",
                "java-1516.1-time-advance-and-grant"
             }, false), RtiTckMain::timestampedInteractionRetractionFanout);
       run(new Scenario("java-tck.timestamped-interaction-regulation-reenable",
             "queued timestamped interaction across changed-lookahead regulation re-enable",
             "time",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.publishInteractionClass;RTIambassador.changeInteractionOrderType;RTIambassador.subscribeInteractionClass;RTIambassador.enableTimeRegulation;RTIambassador.disableTimeRegulation;RTIambassador.queryLookahead;RTIambassador.enableTimeConstrained;RTIambassador.timeAdvanceRequest;RTIambassador.sendInteraction(time);RTIambassador.retract;FederateAmbassador.receiveInteraction(time);FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeRegulationDisabled;FederateAmbassador.timeAdvanceGrant",
             new String[] {
                "java-1516.1-interaction-declaration-and-delivery",
                "java-1516.1-time-advance-and-grant"
             }, false), RtiTckMain::timestampedInteractionRegulationReenable);
       run(new Scenario("java-tck.timestamped-interaction-reenable",
             "queued timestamped interaction across Time Constrained disable and re-enable",
             "time",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.publishInteractionClass;RTIambassador.changeInteractionOrderType;RTIambassador.subscribeInteractionClass;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.sendInteraction(time);RTIambassador.disableTimeConstrained;RTIambassador.timeAdvanceRequest;RTIambassador.retract;FederateAmbassador.receiveInteraction(time);FederateAmbassador.timeConstrainedEnabled;FederateAmbassador.timeAdvanceGrant",
             new String[] {
                "java-1516.1-interaction-declaration-and-delivery",
                "java-1516.1-time-advance-and-grant"
             }, false), RtiTckMain::timestampedInteractionConstrainedReenable);
      run(new Scenario("java-tck.timestamped-directed-interaction-reenable",
            "queued timestamped directed interaction across Time Constrained re-enable", "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.registerObjectInstance;RTIambassador.changeInteractionOrderType;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.sendDirectedInteraction(time);RTIambassador.disableTimeConstrained;RTIambassador.timeAdvanceRequest;RTIambassador.queryLogicalTime;RTIambassador.retract;RTIambassador.getTransportationTypeName;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction(time);FederateAmbassador.timeConstrainedEnabled;FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-directed-interaction-delivery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedDirectedInteractionConstrainedReenable);
      run(new Scenario("java-tck.timestamped-directed-interaction-regulation-reenable",
            "queued timestamped directed interaction across changed-lookahead regulation re-enable",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.registerObjectInstance;RTIambassador.changeInteractionOrderType;RTIambassador.enableTimeRegulation;RTIambassador.disableTimeRegulation;RTIambassador.queryLookahead;RTIambassador.enableTimeConstrained;RTIambassador.timeAdvanceRequest;RTIambassador.sendDirectedInteraction(time);RTIambassador.retract;RTIambassador.getTransportationTypeName;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction(time);FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeRegulationDisabled;FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-directed-interaction-delivery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedDirectedInteractionRegulationReenable);
      run(new Scenario("java-tck.timestamped-directed-interaction-tar-nmr",
            "timestamped directed interaction delivered through independent TAR and NMR grants",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.registerObjectInstance;RTIambassador.changeInteractionOrderType;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.sendDirectedInteraction(time);RTIambassador.timeAdvanceRequest;RTIambassador.nextMessageRequest;RTIambassador.queryLogicalTime;RTIambassador.retract;RTIambassador.getTransportationTypeName;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction(time);FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-directed-interaction-delivery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedDirectedInteractionTarNmr);
      run(new Scenario("java-tck.timestamped-directed-interaction-source-resignation",
            "deliver a queued timestamped directed interaction after producer resignation",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.registerObjectInstance;RTIambassador.changeInteractionOrderType;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.sendDirectedInteraction(time);RTIambassador.timeAdvanceRequest;RTIambassador.resignFederationExecution;RTIambassador.retract;RTIambassador.queryLogicalTime;RTIambassador.disableTimeConstrained;RTIambassador.disableTimeRegulation;RTIambassador.unsubscribeObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassAttributes;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction(time);FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-directed-interaction-delivery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedDirectedInteractionSourceResignation);
      run(new Scenario("java-tck.timestamped-directed-interaction-source-resignation-fanout",
            "deliver one queued timestamped directed interaction independently to two receivers after producer resignation",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.registerObjectInstance;RTIambassador.changeInteractionOrderType;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.sendDirectedInteraction(time);RTIambassador.timeAdvanceRequest;RTIambassador.resignFederationExecution;RTIambassador.retract;RTIambassador.queryLogicalTime;RTIambassador.disableTimeConstrained;RTIambassador.disableTimeRegulation;RTIambassador.unsubscribeObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassAttributes;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction(time);FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-directed-interaction-delivery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedDirectedInteractionSourceResignationFanout);
      run(new Scenario("java-tck.timestamped-directed-interaction-immediate-source-resignation",
            "deliver an accepted timestamped directed interaction after immediate producer resignation",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.registerObjectInstance;RTIambassador.changeInteractionOrderType;RTIambassador.enableTimeRegulation;RTIambassador.sendDirectedInteraction(time);RTIambassador.resignFederationExecution;RTIambassador.unsubscribeObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassAttributes;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction(time);FederateAmbassador.timeRegulationEnabled",
            new String[] {
               "java-1516.1-directed-interaction-delivery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedDirectedInteractionImmediateSourceResignation);
      run(new Scenario("java-tck.timestamped-directed-interaction-retraction-fanout",
            "retract queued timestamped directed copies across two receivers, then verify later delivery",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.registerObjectInstance;RTIambassador.changeInteractionOrderType;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.sendDirectedInteraction(time);RTIambassador.retract;RTIambassador.timeAdvanceRequest;RTIambassador.disableTimeRegulation;RTIambassador.disableTimeConstrained;RTIambassador.unsubscribeObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction(time);FederateAmbassador.requestRetraction;FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-directed-interaction-delivery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedDirectedInteractionRetractionFanout);
      run(new Scenario("java-tck.timestamped-directed-interaction-retraction",
            "retract a queued timestamped directed interaction before delivery and preserve subsequent delivery",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.registerObjectInstance;RTIambassador.changeInteractionOrderType;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.sendDirectedInteraction(time);RTIambassador.retract;RTIambassador.timeAdvanceRequest;RTIambassador.disableTimeRegulation;RTIambassador.disableTimeConstrained;RTIambassador.unsubscribeObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction(time);FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-directed-interaction-delivery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedDirectedInteractionRetraction);
      run(new Scenario("java-tck.timestamped-directed-interaction-derived-object-target",
            "deliver a timestamped directed interaction to a derived-class object after TAR",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions;RTIambassador.registerObjectInstance;RTIambassador.changeInteractionOrderType;RTIambassador.getTimeFactory;RTIambassador.enableTimeConstrained;RTIambassador.enableTimeRegulation;RTIambassador.sendDirectedInteraction(time);RTIambassador.timeAdvanceRequest;RTIambassador.disableTimeConstrained;RTIambassador.disableTimeRegulation;RTIambassador.getTransportationTypeName;RTIambassador.unsubscribeObjectClassDirectedInteractions;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.timeConstrainedEnabled;FederateAmbassador.timeRegulationEnabled;FederateAmbassador.receiveDirectedInteraction(time);FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-directed-interaction-delivery",
               "java-1516.1-time-advance-and-grant",
               "java-1516.1-object-registration-discovery"
            }, false), RtiTckMain::timestampedDirectedInteractionDerivedObjectTarget);
      run(new Scenario("java-tck.timestamped-attribute-update",
            "timestamped attribute update and reflection", "time",
            "RTIambassador.changeDefaultAttributeOrderType;RTIambassador.updateAttributeValues(time);RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.timeAdvanceRequest;FederateAmbassador.reflectAttributeValues(time)",
            new String[] {
               "java-1516.1-attribute-value-update-reflection",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedAttributeUpdate);
      run(new Scenario("java-tck.timestamped-attribute-ownership-transfer",
            "deliver an accepted timestamped attribute update after ownership transfers before grant",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.changeDefaultAttributeOrderType;RTIambassador.registerObjectInstance;RTIambassador.updateAttributeValues(time);RTIambassador.unconditionalAttributeOwnershipDivestiture;RTIambassador.attributeOwnershipAcquisitionIfAvailable;RTIambassador.isAttributeOwnedByFederate;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.timeAdvanceRequest;RTIambassador.disableTimeConstrained;RTIambassador.disableTimeRegulation;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.requestAttributeOwnershipAssumption;FederateAmbassador.attributeOwnershipAcquisitionNotification;FederateAmbassador.reflectAttributeValues(time);FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeConstrainedEnabled;FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-attribute-value-update-reflection",
               "java-1516.1-ownership-management",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedAttributeOwnershipTransfer);
      run(new Scenario("java-tck.timestamped-attribute-source-resignation",
            "deliver an accepted timestamped attribute update after its producer resigns",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.changeDefaultAttributeOrderType;RTIambassador.registerObjectInstance;RTIambassador.updateAttributeValues(time);RTIambassador.resignFederationExecution(UNCONDITIONALLY_DIVEST_ATTRIBUTES);RTIambassador.attributeOwnershipAcquisitionIfAvailable;RTIambassador.retract;RTIambassador.isAttributeOwnedByFederate;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.timeAdvanceRequest;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.requestAttributeOwnershipAssumption;FederateAmbassador.attributeOwnershipAcquisitionNotification;FederateAmbassador.reflectAttributeValues(time);FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeConstrainedEnabled;FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-attribute-value-update-reflection",
               "java-1516.1-ownership-management",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedAttributeSourceResignation);
      run(new Scenario("java-tck.timestamped-attribute-order-cohort",
            "preserve timestamp order and equal-time cohorts for constrained attribute recipients",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.changeDefaultAttributeOrderType;RTIambassador.registerObjectInstance;RTIambassador.subscribeObjectClassAttributes;RTIambassador.updateAttributeValues(time);RTIambassador.getTransportationTypeName;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.timeAdvanceRequest;RTIambassador.disableTimeRegulation;RTIambassador.disableTimeConstrained;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues(time);FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeConstrainedEnabled;FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-federation-create-join-resign-destroy",
               "java-1516.1-object-registration-discovery",
               "java-1516.1-attribute-value-update-reflection",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedAttributeOrderCohort);
       run(new Scenario("java-tck.timestamped-attribute-update-alternate-advances",
             "timestamped attribute reflection through Flush Queue, TARA, and NMRA", "time",
             "RTIambassador.connect(callback model supplied by adapter);RTIambassador.publishObjectClassAttributes;RTIambassador.changeDefaultAttributeOrderType;RTIambassador.registerObjectInstance;RTIambassador.subscribeObjectClassAttributes;RTIambassador.updateAttributeValues(time);RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.flushQueueRequest;RTIambassador.timeAdvanceRequestAvailable;RTIambassador.nextMessageRequestAvailable;RTIambassador.timeAdvanceRequest;RTIambassador.queryLogicalTime;RTIambassador.retract;RTIambassador.disableTimeConstrained;RTIambassador.disableTimeRegulation;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues(time);FederateAmbassador.flushQueueGrant;FederateAmbassador.timeAdvanceGrant",
             new String[] {
                "java-1516.1-attribute-value-update-reflection",
                "java-1516.1-time-advance-and-grant"
             }, false), RtiTckMain::timestampedAttributeUpdateAlternateAdvances);
      run(new Scenario("java-tck.timestamped-object-removal",
            "receive-order asynchronous and timestamped object removal", "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.deleteObjectInstance(receive-order);RTIambassador.deleteObjectInstance(time);RTIambassador.enableAsynchronousDelivery;RTIambassador.disableAsynchronousDelivery;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.timeAdvanceRequest;FederateAmbassador.removeObjectInstance(receive-order);FederateAmbassador.removeObjectInstance(time);FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-object-registration-discovery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedObjectRemoval);
      run(new Scenario("java-tck.timestamped-object-deletion-mixed-advances",
            "timestamped object removal through Flush Queue, TARA, and NMRA", "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.deleteObjectInstance(time);RTIambassador.retract;RTIambassador.getTimeFactory;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.timeAdvanceRequest;RTIambassador.flushQueueRequest;RTIambassador.timeAdvanceRequestAvailable;RTIambassador.nextMessageRequestAvailable;RTIambassador.queryLogicalTime;RTIambassador.disableTimeConstrained;RTIambassador.disableTimeRegulation;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.removeObjectInstance(time);FederateAmbassador.flushQueueGrant;FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-object-registration-discovery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedObjectDeletionMixedAdvances);
      run(new Scenario("java-tck.timestamped-object-deletion-no-fanout",
            "retract a timestamped deletion with no eligible recipient and restore object state",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.enableTimeRegulation;RTIambassador.getTimeFactory;RTIambassador.registerObjectInstance;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.deleteObjectInstance(time);RTIambassador.retract;RTIambassador.isAttributeOwnedByFederate;RTIambassador.disableTimeRegulation;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.timeRegulationEnabled;FederateAmbassador.removeObjectInstance;FederateAmbassador.requestRetraction",
            new String[] {
               "java-1516.1-object-registration-discovery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedObjectDeletionNoFanout);
      run(new Scenario("java-tck.timestamped-object-deletion-tombstone",
            "reuse a named object instance after a timestamped-deletion tombstone becomes terminal",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.reserveObjectInstanceName;RTIambassador.registerObjectInstance(named);RTIambassador.getObjectInstanceHandle;RTIambassador.getTimeFactory;RTIambassador.enableTimeRegulation;RTIambassador.deleteObjectInstance(time);RTIambassador.timeAdvanceRequest;RTIambassador.retract;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.objectInstanceNameReservationSucceeded;FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-object-registration-discovery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedObjectDeletionTombstone);
      run(new Scenario("java-tck.timestamped-object-deletion-retraction",
            "retract one timestamped removal before constrained delivery and reject retraction after delivery",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.getTimeFactory;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.deleteObjectInstance(time);RTIambassador.retract;RTIambassador.timeAdvanceRequest;RTIambassador.disableTimeConstrained;RTIambassador.disableTimeRegulation;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeConstrainedEnabled;FederateAmbassador.removeObjectInstance(time);FederateAmbassador.requestRetraction;FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-object-registration-discovery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedObjectDeletionRetraction);
      run(new Scenario("java-tck.timestamped-object-deletion-regulation-reenable",
            "preserve a queued timestamped removal across Time Regulation re-enable with changed lookahead",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.getTimeFactory;RTIambassador.enableTimeConstrained;RTIambassador.enableTimeRegulation;RTIambassador.deleteObjectInstance(time);RTIambassador.disableTimeRegulation;RTIambassador.queryLookahead;RTIambassador.timeAdvanceRequest;RTIambassador.queryLogicalTime;RTIambassador.retract;RTIambassador.disableTimeConstrained;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.removeObjectInstance(time);FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeRegulationDisabled;FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-object-registration-discovery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedObjectDeletionRegulationReenable);
      run(new Scenario("java-tck.timestamped-object-deletion-retraction-joined-owners",
            "retract timestamped object deletion after a former attribute owner resigns",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.attributeOwnershipAcquisitionIfAvailable;RTIambassador.attributeOwnershipAcquisition;RTIambassador.attributeOwnershipDivestitureIfWanted;RTIambassador.isAttributeOwnedByFederate;RTIambassador.getTimeFactory;RTIambassador.enableTimeConstrained;RTIambassador.enableTimeRegulation;RTIambassador.deleteObjectInstance(time);RTIambassador.resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST);RTIambassador.timeAdvanceRequest;RTIambassador.retract;RTIambassador.queryAttributeOwnership;RTIambassador.queryLogicalTime;RTIambassador.disableTimeConstrained;RTIambassador.disableTimeRegulation;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.attributeOwnershipAcquisitionNotification;FederateAmbassador.removeObjectInstance(time);FederateAmbassador.attributeIsNotOwned;FederateAmbassador.requestRetraction;FederateAmbassador.timeConstrainedEnabled;FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "java-1516.1-object-registration-discovery",
               "java-1516.1-ownership-management",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedObjectDeletionRetractionJoinedOwners);
      run(new Scenario("java-tck.timestamped-object-deletion-source-resignation-fanout",
            "deliver a queued timestamped object deletion independently to two constrained recipients after producer resignation",
            "time",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.getTimeFactory;RTIambassador.enableTimeConstrained;RTIambassador.enableTimeRegulation;RTIambassador.deleteObjectInstance(time);RTIambassador.timeAdvanceRequest;RTIambassador.nextMessageRequest;RTIambassador.resignFederationExecution(UNCONDITIONALLY_DIVEST_ATTRIBUTES);RTIambassador.retract;RTIambassador.queryLogicalTime;RTIambassador.disableTimeConstrained;RTIambassador.disableTimeRegulation;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.removeObjectInstance(time);FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeConstrainedEnabled;FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "req-federate-connect-before-rti-work",
               "req-disconnect-service-terminates-connection",
               "java-1516.1-object-registration-discovery",
               "java-1516.1-time-advance-and-grant"
            }, false), RtiTckMain::timestampedObjectDeletionSourceResignationFanout);
      run(new Scenario("java-tck.save-restore", "save and restore lifecycle", "save-restore",
            "requestFederationSave;federateSaveComplete;requestFederationRestore;federationRestored",
            new String[] {"java-1516.1-save-restore-lifecycle"}, false),
         RtiTckMain::saveAndRestore);
      run(new Scenario("java-tck.ownership", "ownership callbacks and exceptions", "ownership",
            "queryAttributeOwnership;attributeOwnershipAcquisition;attributeOwnershipDivestiture",
            new String[] {"java-1516.1-ownership-management"}, false),
         RtiTckMain::ownershipManagement);
      run(new Scenario("java-tck.synchronization", "synchronization points", "synchronization",
            "registerFederationSynchronizationPoint;announceSynchronizationPoint;federationSynchronized",
            new String[] {"java-1516.1-synchronization-point"}, false),
         RtiTckMain::synchronizationPoint);
      run(new Scenario("java-tck.mom", "MOM and service reporting callbacks", "mom",
            "HLAreportServiceInvocation;queryFederationSaveStatus;federationSaveStatusResponse",
            new String[] {"java-1516.1-mom-service-reporting"}, false),
         RtiTckMain::momStatusReport);
      run(new Scenario("java-tck.service-report-interaction",
            "standard service report for a successful ordinary interaction", "mom-service-reporting",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.setServiceReportingSwitch;RTIambassador.setSendServiceReportsToFileSwitch;RTIambassador.getInteractionClassHandle;RTIambassador.getInteractionClassName;RTIambassador.getParameterHandle;RTIambassador.getParameterName;RTIambassador.getTransportationTypeHandle;RTIambassador.getTransportationTypeName;RTIambassador.getParameterHandleValueMapFactory;RTIambassador.subscribeInteractionClass;RTIambassador.publishInteractionClass;RTIambassador.sendInteraction;RTIambassador.unsubscribeInteractionClass;RTIambassador.unpublishInteractionClass;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateHandleFactory.decode;FederateAmbassador.receiveInteraction;HLAunicodeString.decode;HLAinteger16BE.decode;HLAinteger32BE.decode;HLAboolean.decode",
            new String[] {
               "cpp-1516.1-mom-service-reporting",
               "cpp-1516.1-explicit-mim-composition"
            }, false), RtiTckMain::serviceReportInteraction);
      run(new Scenario("java-tck.service-report-resign-federation-execution",
            "standard service report for federation resignation", "mom-service-reporting",
            "RTIambassador.connect;RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.setServiceReportingSwitch;RTIambassador.setSendServiceReportsToFileSwitch;RTIambassador.getInteractionClassHandle;RTIambassador.getInteractionClassName;RTIambassador.getParameterHandle;RTIambassador.getParameterName;RTIambassador.getTransportationTypeHandle;RTIambassador.getTransportationTypeName;RTIambassador.subscribeInteractionClass;RTIambassador.resignFederationExecution;RTIambassador.unsubscribeInteractionClass;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateHandleFactory.decode;FederateAmbassador.receiveInteraction;HLAunicodeString.decode;HLAinteger16BE.decode;HLAinteger32BE.decode;HLAboolean.decode",
            new String[] {
               "cpp-1516.1-mom-service-reporting",
               "cpp-1516.1-federation-create-join-resign-destroy",
               "cpp-1516.1-explicit-mim-composition"
            }, false), RtiTckMain::serviceReportResignFederationExecution);

      writeEvidenceArtifacts();
      int passed = count("pass");
      int unsupported = count("unsupported");
      int notApplicable = count("not applicable");
      int filtered = count("filtered");
      int failed = count("fail");
      System.out.println("Java RTI contract TCK: " + passed + " passed, " + failed
            + " failed, " + unsupported + " unsupported, " + notApplicable
            + " not applicable, " + filtered + " filtered");
      String selectedScenario = System.getProperty(SCENARIO_PROPERTY, "");
      if (!selectedScenario.isEmpty() && filtered == results.size()) {
         throw new IllegalArgumentException("unknown Java TCK scenario: " + selectedScenario);
      }
      if (failed != 0) {
         throw new AssertionError("Java RTI TCK failures: " + failureSummary());
      }
   }

   private static void run(Scenario scenario, CheckedTest test) {
      String selectedScenario = System.getProperty(SCENARIO_PROPERTY, "");
      if (!selectedScenario.isEmpty() && !selectedScenario.equals(scenario.id)) {
         results.add(TestResult.skipped(scenario, "filtered",
               "excluded by focused scenario selector: " + selectedScenario));
         return;
      }
      String mode = configuredMode(scenario);
      if ("unsupported".equals(mode) || "not applicable".equals(mode)) {
         results.add(TestResult.skipped(scenario, mode,
               mode + " by capability profile; no provider assertion was made"));
         System.out.println(mode.toUpperCase() + " " + scenario.id + " provider=" + provider());
         return;
      }
      try {
         test.run();
         results.add(TestResult.passed(scenario));
         System.out.println("PASS " + scenario.id + " provider=" + provider());
      } catch (Throwable error) {
         if (Boolean.getBoolean(STACKTRACE_PROPERTY)) error.printStackTrace(System.err);
         String message = error.toString();
         if (error instanceof UnsupportedPortableScenario) {
            results.add(TestResult.skipped(scenario, "unsupported", message));
            System.out.println("UNSUPPORTED " + scenario.id + " provider=" + provider()
                  + " reason=" + message);
         } else {
            results.add(TestResult.failed(scenario, message));
            System.err.println("FAIL requirement=" + String.join(",", scenario.requirementIds)
                  + " method=" + scenario.apiMethod + " scenario=" + scenario.id
                  + " provider=" + provider() + " error=" + message);
         }
      }
   }

   private static RtiFactory factory() throws Exception {
      String name = System.getProperty(FACTORY_PROPERTY);
      RtiFactory result = name == null || name.isEmpty()
         ? RtiFactoryFactory.getRtiFactory()
         : RtiFactoryFactory.getRtiFactory(name);
      check(result != null, "RtiFactoryFactory returned null");
      check(result.rtiName() != null && !result.rtiName().isEmpty(),
         "factory name is empty");
      check(result.rtiVersion() != null && !result.rtiVersion().isEmpty(),
         "factory version is empty");
      discoveredProvider = result.rtiName() + " " + result.rtiVersion();
      return result;
   }

   private static void factoryDiscovery() throws Exception {
      RtiFactory result = factory();
      check(RtiFactory.class.isAssignableFrom(result.getClass()),
         "provider does not implement the standard RtiFactory");
      check(result.getEncoderFactory() != null, "provider returned no EncoderFactory");
      RTIambassador ambassador = result.getRtiAmbassador();
      check(ambassador != null, "provider returned no RTIambassador");
   }

   private static void encoderRoundTrip() throws Exception {
      BasicDataElementsTck.run(factory().getEncoderFactory());
   }

   private static void federationMembership() throws Exception {
      FederationBasicsTck.federationMembership(factory(), requireFom(), timeImplementation());
   }

   private static void explicitMimCreation() throws Exception {
      ExplicitMimCreationTck.run(factory(), requireFom(), requireMim(), timeImplementation(),
         objectClassName(), attributeName(), interactionClassName(), parameterName(),
         callbackModel());
   }

   private static void federationMomCurrentFdd() throws Exception {
      FederationMomCurrentFddTck.run(factory(), requireFom(), requireMim(),
         requireAdditionalFom(), timeImplementation(), callbackModel());
   }

   private static void federationMomFomModuleDesignatorList() throws Exception {
      FederationMomFomModuleDesignatorListTck.run(factory(), requireFom(), requireMim(),
         requireAdditionalFom(), timeImplementation(), callbackModel());
   }

   private static void federationMomStaticIdentity() throws Exception {
      FederationMomStaticIdentityTck.run(factory(), requireFom(), requireMim(),
         timeImplementation(), callbackModel());
   }

   private static void federationMomStaticSwitches() throws Exception {
      FederationMomStaticSwitchesTck.run(factory(), requireFom(), requireMim(),
         timeImplementation(), callbackModel());
   }

   private static void federationMomFederatesInFederation() throws Exception {
      FederationMomFederatesInFederationTck.run(factory(), requireFom(), requireMim(),
         timeImplementation(), callbackModel());
   }

   private static void federationMomContentReports() throws Exception {
      FederationMomContentReportsTck.run(factory(), requireFom(), requireMim(),
         timeImplementation(), callbackModel());
   }

   private static void callbackReentrancy() throws Exception {
      CallbackReentrancyTck.run(factory(), callbackModel());
   }

   private static void apiSurfaceInventory() {
      Class<?>[] standardTypes = new Class<?>[] {
         RtiFactoryFactory.class,
         RtiFactory.class,
         RTIambassador.class,
         FederateAmbassador.class,
         EncoderFactory.class,
         LogicalTimeFactory.class,
         LogicalTime.class,
         LogicalTimeInterval.class
      };
      int methodCount = 0;
      for (Class<?> type : standardTypes) {
         check(type.getName().startsWith("hla.rti1516_2025"),
            "non-standard type entered the portable TCK: " + type.getName());
         Method[] methods = type.getMethods();
         check(methods.length > 0, "standard API type has no public methods: " + type.getName());
         methodCount += methods.length;
         for (Method method : methods) {
            if (method.getDeclaringClass() == Object.class) {
               continue;
            }
            check(method.getDeclaringClass().getName().startsWith("hla.rti1516_2025"),
               "API method escaped the standard namespace: " + method);
            checkApiSignatureType(method.getReturnType(), method);
            for (Class<?> parameterType : method.getParameterTypes()) {
               checkApiSignatureType(parameterType, method);
            }
            for (Class<?> exceptionType : method.getExceptionTypes()) {
               checkApiSignatureType(exceptionType, method);
            }
         }
      }
      check(methodCount >= 100, "the supplied API JAR exposes an unexpectedly small surface");
   }

   private static void checkApiSignatureType(Class<?> signatureType, Method method) {
      Class<?> exposedType = signatureType;
      while (exposedType.isArray()) {
         exposedType = exposedType.getComponentType();
      }
      if (exposedType.isPrimitive()) {
         return;
      }
      String typeName = exposedType.getName();
      check(typeName.startsWith("hla.rti1516_2025.") || typeName.startsWith("java."),
         "standard API signature exposes a provider/non-standard type " + typeName
            + " in " + method);
   }

   private static void overloadsAndExceptions() throws Exception {
      RtiConfiguration configuration = RtiConfiguration.createConfiguration();
      HLAnoCredentials credentials = new HLAnoCredentials();

      connectAndDisconnect((ambassador, federate) ->
         ambassador.connect(federate, CallbackModel.HLA_EVOKED), "base connect overload");
      connectAndDisconnect((ambassador, federate) ->
         ambassador.connect(federate, CallbackModel.HLA_EVOKED, configuration),
         "configuration connect overload");
      connectAndDisconnect((ambassador, federate) ->
         ambassador.connect(federate, CallbackModel.HLA_EVOKED, credentials),
         "credentials connect overload");
      connectAndDisconnect((ambassador, federate) ->
         ambassador.connect(federate, CallbackModel.HLA_EVOKED, configuration, credentials),
         "configuration and credentials connect overload");

      RTIambassador ambassador = factory().getRtiAmbassador();
      CallbackRecorder recorder = new CallbackRecorder();
      boolean connected = false;
      try {
         ambassador.connect(recorder.proxy(), CallbackModel.HLA_EVOKED);
         connected = true;
         expectException(() -> ambassador.connect(recorder.proxy(), CallbackModel.HLA_EVOKED),
            "AlreadyConnected", "duplicate connect did not report AlreadyConnected");
      } finally {
         if (connected) {
            try {
               ambassador.disconnect();
            } catch (Exception ignored) {
            }
         }
      }
   }

   private static void connectAndDisconnect(ConnectOperation operation, String label)
         throws Exception {
      RTIambassador ambassador = factory().getRtiAmbassador();
      CallbackRecorder recorder = new CallbackRecorder();
      boolean connected = false;
      try {
         ConfigurationResult result = operation.connect(ambassador, recorder.proxy());
         connected = true;
         check(result != null, label + " returned no ConfigurationResult");
      } finally {
         if (connected) {
            try {
               ambassador.disconnect();
            } catch (Exception ignored) {
            }
         }
      }
   }

   private static void malformedInputs() throws Exception {
      RTIambassador ambassador = factory().getRtiAmbassador();
      CallbackRecorder recorder = new CallbackRecorder();
      boolean connected = false;
      try {
         ambassador.connect(recorder.proxy(), CallbackModel.HLA_EVOKED);
         connected = true;
         String missingFom = Path.of(System.getProperty("java.io.tmpdir"),
            "java-tck-missing-" + UUID.randomUUID() + ".xml").toString();
         expectException(() -> ambassador.createFederationExecution(
               "java-tck-malformed-" + UUID.randomUUID(), missingFom,
               timeImplementation()),
            "CouldNotOpenFOM", "ErrorReadingFOM", "InvalidFOM",
            "a missing FOM path was accepted");
      } finally {
         if (connected) {
            try {
               ambassador.disconnect();
            } catch (Exception ignored) {
            }
         }
      }
   }

   private static void unsupportedPortableFamily() {
      throw new UnsupportedPortableScenario(
         "provider capability profile did not enable this multi-federate/FOM-dependent family");
   }

   private static void dataDistributionManagement() throws Exception {
      RTIambassador ambassador = factory().getRtiAmbassador();
      CallbackRecorder recorder = new CallbackRecorder();
      String federation = "java-tck-ddm-" + UUID.randomUUID();
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      RegionHandle region = null;
      try {
         ambassador.connect(recorder.proxy(), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, requireFom(), timeImplementation());
         created = true;
         ambassador.joinFederationExecution("java-tck-ddm", federation);
         joined = true;

         DimensionHandle dimension = ambassador.getDimensionHandle(dimensionName());
         DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
         dimensions.add(dimension);
         region = ambassador.createRegion(dimensions);
         RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
         regions.add(region);
         RangeBounds expected = new RangeBounds(1, 10);
         ambassador.setRangeBounds(region, dimension, expected);
         check(expected.equals(ambassador.getRangeBounds(region, dimension)),
            "region range bounds did not round-trip through the standard API");
         ambassador.commitRegionModifications(regions);

         ObjectClassHandle objectClass = ambassador.getObjectClassHandle(ddmObjectClassName());
         AttributeHandle attribute = ambassador.getAttributeHandle(objectClass, ddmAttributeName());
         AttributeHandleSet attributes = ambassador.getAttributeHandleSetFactory().create();
         attributes.add(attribute);
         AttributeSetRegionSetPairList associations =
            ambassador.getAttributeSetRegionSetPairListFactory().create(1);
         associations.add(new AttributeRegionAssociation(attributes, regions));
         ambassador.subscribeObjectClassAttributesWithRegions(objectClass, associations);
         ambassador.unsubscribeObjectClassAttributesWithRegions(objectClass, associations);
         ambassador.deleteRegion(region);
         region = null;
      } finally {
         if (region != null) {
            try { ambassador.deleteRegion(region); } catch (Exception ignored) { }
         }
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void ownershipManagement() throws Exception {
      OwnershipTck.run(factory(), requireFom(), timeImplementation(), objectClassName(),
         attributeName());
   }

   private static void legacyOwnershipManagement() throws Exception {
      RTIambassador owner = factory().getRtiAmbassador();
      RTIambassador acquirer = factory().getRtiAmbassador();
      CallbackRecorder ownerRecorder = new CallbackRecorder();
      CallbackRecorder acquirerRecorder = new CallbackRecorder();
      String federation = "java-tck-ownership-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean acquirerConnected = false;
      boolean ownerJoined = false;
      boolean acquirerJoined = false;
      boolean created = false;
      try {
         owner.connect(ownerRecorder.proxy(), CallbackModel.HLA_EVOKED);
         ownerConnected = true;
         acquirer.connect(acquirerRecorder.proxy(), CallbackModel.HLA_EVOKED);
         acquirerConnected = true;
         owner.createFederationExecution(federation, requireFom(), timeImplementation());
         created = true;
         owner.joinFederationExecution("java-tck-owner", federation);
         ownerJoined = true;
         acquirer.joinFederationExecution("java-tck-acquirer", federation);
         acquirerJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName());
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName());
         ObjectClassHandle acquirerClass = acquirer.getObjectClassHandle(objectClassName());
         AttributeHandle acquirerAttribute = acquirer.getAttributeHandle(acquirerClass, attributeName());
         AttributeHandleSet ownerAttributes = owner.getAttributeHandleSetFactory().create();
         ownerAttributes.add(ownerAttribute);
         AttributeHandleSet acquirerAttributes = acquirer.getAttributeHandleSetFactory().create();
         acquirerAttributes.add(acquirerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         acquirer.publishObjectClassAttributes(acquirerClass, acquirerAttributes);
         acquirer.subscribeObjectClassAttributes(acquirerClass, acquirerAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         drain(acquirer);
         check(acquirerRecorder.discoveredObject != null,
            "ownership scenario did not discover the registered object");
         ObjectInstanceHandle acquirerObject = acquirerRecorder.discoveredObject;

         acquirer.queryAttributeOwnership(acquirerObject, acquirerAttributes);
         drain(acquirer);
         check(acquirerRecorder.ownershipInformReports > 0,
            "informAttributeOwnership did not cross the standard callback interface");
         owner.unconditionalAttributeOwnershipDivestiture(
            object, ownerAttributes, new byte[] { 4, 5, 6 });
         acquirer.attributeOwnershipAcquisition(
            acquirerObject, acquirerAttributes, new byte[] { 7, 8, 9 });
         drain(acquirer);
         check(acquirerRecorder.ownershipAcquisitions > 0,
            "attributeOwnershipAcquisitionNotification did not cross the standard callback interface");
      } finally {
         if (acquirerJoined) {
            try { acquirer.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (acquirerConnected) {
            try { acquirer.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void timeAdvanceAndGrants() throws Exception {
      RTIambassador regulator = factory().getRtiAmbassador();
      RTIambassador constrained = factory().getRtiAmbassador();
      CallbackRecorder regulatorRecorder = new CallbackRecorder();
      CallbackRecorder constrainedRecorder = new CallbackRecorder();
      String federation = "java-tck-time-advance-" + UUID.randomUUID();
      boolean regulatorConnected = false;
      boolean constrainedConnected = false;
      boolean regulatorJoined = false;
      boolean constrainedJoined = false;
      boolean created = false;
      try {
         CallbackModel model = callbackModel();
         regulator.connect(regulatorRecorder.proxy(), model);
         regulatorConnected = true;
         constrained.connect(constrainedRecorder.proxy(), model);
         constrainedConnected = true;
         regulator.createFederationExecution(federation, requireFom(), timeImplementation());
         created = true;
         regulator.joinFederationExecution("java-tck-regulator", federation);
         regulatorJoined = true;
         constrained.joinFederationExecution("java-tck-constrained", federation);
         constrainedJoined = true;
         LogicalTimeFactory<?, ?> timeFactory = regulator.getTimeFactory();
         LogicalTimeInterval interval = (LogicalTimeInterval) timeFactory.makeEpsilon();
         regulator.enableTimeRegulation(interval);
         drain(regulator);
         check(regulatorRecorder.timeRegulationEnabled > 0,
            "timeRegulationEnabled did not cross the callback interface");
         constrained.enableTimeConstrained();
         drain(constrained);
         check(constrainedRecorder.timeConstrainedEnabled > 0,
            "timeConstrainedEnabled did not cross the callback interface");
         LogicalTime current = regulator.queryLogicalTime();
         LogicalTime requested = (LogicalTime) current.add(interval);
         constrained.timeAdvanceRequest(requested);
          JavaTckSupport.expectFailure(
             () -> constrained.timeAdvanceRequestAvailable(requested),
             "InTimeAdvancingState",
             "timeAdvanceRequestAvailable while TAR is pending");
          JavaTckSupport.expectFailure(
             () -> constrained.nextMessageRequestAvailable(requested),
             "InTimeAdvancingState",
             "nextMessageRequestAvailable while TAR is pending");
          JavaTckSupport.expectFailure(
             () -> constrained.flushQueueRequest(requested),
             "InTimeAdvancingState",
             "flushQueueRequest while TAR is pending");
          JavaTckSupport.expectFailure(
             () -> constrained.timeAdvanceRequest(requested),
             "InTimeAdvancingState",
             "second timeAdvanceRequest while TAR is pending");
          check(constrainedRecorder.timeAdvanceGrantTimes.isEmpty(),
             "rejected alternate requests completed the original pending TAR");

          regulator.timeAdvanceRequest(requested);
         drain(regulator);
         drain(constrained);
         check(regulatorRecorder.timeAdvanceGrantTimes.size() == 1
               && constrainedRecorder.timeAdvanceGrantTimes.size() == 1,
            "timeAdvanceRequest did not produce exactly one standard grant per federate");
         check(requested.equals(regulatorRecorder.timeAdvanceGrantTimes.get(0))
               && requested.equals(constrainedRecorder.timeAdvanceGrantTimes.get(0)),
            "timeAdvanceGrant did not report the requested logical time");

         for (int advance = 0; advance < 3; ++advance) {
            LogicalTime regulatorCurrent = regulator.queryLogicalTime();
            LogicalTime constrainedCurrent = constrained.queryLogicalTime();
            LogicalTime regulatorTarget = (LogicalTime) regulatorCurrent.add(interval);
            LogicalTime constrainedTarget = (LogicalTime) constrainedCurrent.add(interval);
            if (advance == 0) {
               regulator.timeAdvanceRequestAvailable(regulatorTarget);
               constrained.timeAdvanceRequestAvailable(constrainedTarget);
            } else if (advance == 1) {
               regulator.nextMessageRequestAvailable(regulatorTarget);
               constrained.nextMessageRequestAvailable(constrainedTarget);
            } else {
               regulator.flushQueueRequest(regulatorTarget);
               constrained.flushQueueRequest(constrainedTarget);
            }
            drain(regulator);
            drain(constrained);
            if (advance < 2) {
               int expectedGrants = advance + 2;
               check(regulatorRecorder.timeAdvanceGrantTimes.size() == expectedGrants
                     && constrainedRecorder.timeAdvanceGrantTimes.size() == expectedGrants,
                  "available advance did not produce exactly one timeAdvanceGrant per federate");
               check(regulatorTarget.equals(regulatorRecorder.timeAdvanceGrantTimes
                           .get(expectedGrants - 1))
                     && constrainedTarget.equals(constrainedRecorder.timeAdvanceGrantTimes
                           .get(expectedGrants - 1)),
                  "available advance grant did not report the requested logical time");
            } else {
               check(regulatorRecorder.flushQueueGrantTimes.size() == 1
                     && constrainedRecorder.flushQueueGrantTimes.size() == 1,
                  "flushQueueRequest did not produce exactly one Flush Queue Grant per federate");
               check(regulatorTarget.equals(regulatorRecorder.flushQueueGrantTimes.get(0))
                     && constrainedTarget.equals(constrainedRecorder.flushQueueGrantTimes.get(0)),
                  "Flush Queue Grant did not report the requested logical time");
            }
         }
      } finally {
         if (constrainedJoined) {
            try { constrained.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         }
         if (regulatorJoined) {
            try { regulator.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         }
         if (created) {
            try { regulator.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (constrainedConnected) {
            try { constrained.disconnect(); } catch (Exception ignored) { }
         }
         if (regulatorConnected) {
            try { regulator.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void timestampedAttributeUpdate() throws Exception {
      TimestampedAttributeTck.deliver(factory(), requireFom(), timeImplementation(),
         objectClassName(), attributeName(), callbackModel());
   }

   private static void timestampedAttributeOwnershipTransfer() throws Exception {
      TimestampedAttributeOwnershipTransferTck.run(factory(), requireFom(),
         timeImplementation(), objectClassName(), attributeName(), callbackModel());
   }

   private static void timestampedAttributeSourceResignation() throws Exception {
      TimestampedAttributeSourceResignationTck.run(factory(), requireFom(),
         timeImplementation(), objectClassName(), attributeName(), callbackModel());
   }

   private static void timestampedAttributeOrderCohort() throws Exception {
      TimestampedAttributeOrderCohortTck.run(factory(), requireFom(),
         timeImplementation(), objectClassName(), attributeName(), callbackModel());
   }

    private static void timestampedAttributeUpdateAlternateAdvances() throws Exception {
       TimestampedAttributeAlternateAdvancesTck.run(factory(), requireFom(),
          timeImplementation(), objectClassName(), attributeName(), callbackModel());
    }

   private static void timestampedInteractionMixedAdvances() throws Exception {
      TimestampedInteractionMixedAdvancesTck.run(factory(), requireFom(),
         timeImplementation(), interactionClassName(), parameterName(), callbackModel());
   }

   private static void timestampedDirectedAlternateAdvances() throws Exception {
      TimestampedDirectedAlternateAdvancesTck.run(factory(), requireFom(),
         timeImplementation(), objectClassName(), attributeName(), interactionClassName(),
         parameterName(), callbackModel());
   }

    private static void timestampedInteractionRetraction() throws Exception {
       TimestampedInteractionRetractionTck.run(factory(), requireFom(),
          timeImplementation(), interactionClassName(), parameterName(), callbackModel());
    }

    private static void timestampedInteractionCrossProducerOrder() throws Exception {
       TimestampedInteractionCrossProducerOrderTck.run(factory(), requireFom(),
          timeImplementation(), interactionClassName(), parameterName(), callbackModel());
    }

     private static void timestampedInteractionNoFanout() throws Exception {
        TimestampedInteractionNoFanoutTck.run(factory(), requireFom(),
           timeImplementation(), interactionClassName(), parameterName(), callbackModel());
     }

     private static void timestampedInteractionRetractionFanout() throws Exception {
        TimestampedInteractionRetractionFanoutTck.run(factory(), requireFom(),
           timeImplementation(), interactionClassName(), parameterName(), callbackModel());
     }

     private static void timestampedInteractionRegulationReenable() throws Exception {
        TimestampedInteractionRegulationReenableTck.run(factory(), requireFom(),
           timeImplementation(), interactionClassName(), parameterName(), callbackModel());
     }

     private static void timestampedInteractionConstrainedReenable() throws Exception {
        TimestampedInteractionConstrainedReenableTck.run(factory(), requireFom(),
           timeImplementation(), interactionClassName(), parameterName(), callbackModel());
     }

     private static void timestampedDirectedInteractionConstrainedReenable() throws Exception {
        TimestampedDirectedInteractionConstrainedReenableTck.run(factory(), requireFom(),
           timeImplementation(), timestampedDirectedObjectClassName(),
           timestampedDirectedAttributeName(), timestampedDirectedInteractionClassName(),
           timestampedDirectedParameterName(), callbackModel());
     }

     private static void timestampedDirectedInteractionRegulationReenable() throws Exception {
        TimestampedDirectedInteractionRegulationReenableTck.run(factory(), requireFom(),
           timeImplementation(), timestampedDirectedObjectClassName(),
           timestampedDirectedAttributeName(), timestampedDirectedInteractionClassName(),
           timestampedDirectedParameterName(), callbackModel());
     }

     private static void timestampedDirectedInteractionTarNmr() throws Exception {
        TimestampedDirectedInteractionTarNmrTck.run(factory(), requireFom(),
           timeImplementation(), timestampedDirectedObjectClassName(),
           timestampedDirectedAttributeName(), timestampedDirectedInteractionClassName(),
           timestampedDirectedParameterName(), callbackModel());
     }

     private static void timestampedDirectedInteractionSourceResignation() throws Exception {
        TimestampedDirectedInteractionSourceResignationTck.run(factory(), requireFom(),
           timeImplementation(), timestampedDirectedObjectClassName(),
           timestampedDirectedAttributeName(), timestampedDirectedInteractionClassName(),
           timestampedDirectedParameterName(), callbackModel());
     }

     private static void timestampedDirectedInteractionSourceResignationFanout() throws Exception {
        TimestampedDirectedInteractionSourceResignationFanoutTck.run(factory(), requireFom(),
           timeImplementation(), timestampedDirectedObjectClassName(),
           timestampedDirectedAttributeName(), timestampedDirectedInteractionClassName(),
           timestampedDirectedParameterName(), callbackModel());
     }

     private static void timestampedDirectedInteractionImmediateSourceResignation() throws Exception {
        TimestampedDirectedInteractionImmediateSourceResignationTck.run(factory(), requireFom(),
           timeImplementation(), timestampedDirectedObjectClassName(),
           timestampedDirectedAttributeName(), timestampedDirectedInteractionClassName(),
           timestampedDirectedParameterName(), callbackModel());
     }

     private static void timestampedDirectedInteractionRetractionFanout() throws Exception {
        TimestampedDirectedInteractionRetractionFanoutTck.run(factory(), requireFom(),
           timeImplementation(), timestampedDirectedObjectClassName(),
           timestampedDirectedAttributeName(), timestampedDirectedInteractionClassName(),
           timestampedDirectedParameterName(), callbackModel());
     }

     private static void timestampedDirectedInteractionRetraction() throws Exception {
        TimestampedDirectedInteractionRetractionTck.run(factory(), requireFom(),
           timeImplementation(), timestampedDirectedObjectClassName(),
           timestampedDirectedAttributeName(), timestampedDirectedInteractionClassName(),
           timestampedDirectedParameterName(), callbackModel());
     }

     private static void timestampedDirectedInteractionDerivedObjectTarget() throws Exception {
        TimestampedDirectedDerivedObjectTargetTck.run(factory(), requireFom(),
           timeImplementation(), typedDerivedObjectClassName(),
           typedIdentityAttributeName(), typedInteractionClassName(),
           typedIntegerParameterName(), callbackModel());
     }

     private static void directedInteractionDerivedObjectTarget() throws Exception {
        DirectedDerivedObjectTargetTck.run(factory(), requireFom(),
           typedDerivedObjectClassName(), typedIdentityAttributeName(),
           typedInteractionClassName(), typedIntegerParameterName(), callbackModel());
     }

   private static void timestampedObjectRemoval() throws Exception {
      TimestampedObjectRemovalTck.deliver(factory(), requireFom(), timeImplementation(),
         objectClassName(), attributeName(), callbackModel());
   }

   private static void timestampedObjectDeletionMixedAdvances() throws Exception {
      TimestampedObjectDeletionAlternateAdvancesTck.run(factory(), requireFom(),
         timeImplementation(), objectClassName(), attributeName(), callbackModel());
   }

   private static void timestampedObjectDeletionNoFanout() throws Exception {
      TimestampedObjectDeletionNoFanoutTck.run(factory(), requireFom(),
         timeImplementation(), objectClassName(), attributeName(), callbackModel());
   }

   private static void timestampedObjectDeletionTombstone() throws Exception {
      TimestampedObjectDeletionTombstoneTck.run(factory(), requireFom(),
         timeImplementation(), objectClassName(), attributeName(), callbackModel());
   }

   private static void timestampedObjectDeletionRetraction() throws Exception {
      TimestampedObjectDeletionRetractionTck.run(factory(), requireFom(),
         timeImplementation(), objectClassName(), attributeName(), callbackModel());
   }

   private static void timestampedObjectDeletionRegulationReenable() throws Exception {
      TimestampedObjectDeletionRegulationReenableTck.run(factory(), requireFom(),
         timeImplementation(), objectClassName(), attributeName(), callbackModel());
   }

   private static void timestampedObjectDeletionRetractionJoinedOwners() throws Exception {
      TimestampedObjectDeletionJoinedOwnersTck.run(factory(), requireFom(),
         timeImplementation(), multiAttributeObjectClassName(), multiAttributeFirstName(),
         multiAttributeSecondName(), callbackModel());
   }

   private static void timestampedObjectDeletionSourceResignationFanout() throws Exception {
      TimestampedObjectDeletionSourceResignationFanoutTck.run(factory(), requireFom(),
         timeImplementation(), objectClassName(), attributeName(), callbackModel());
   }

   private static void synchronizationPoint() throws Exception {
      SynchronizationTck.run(factory(), requireFom(), timeImplementation());
   }

   private static void legacySynchronizationPoint() throws Exception {
      RTIambassador ambassador = factory().getRtiAmbassador();
      CallbackRecorder recorder = new CallbackRecorder();
      String federation = "java-tck-sync-" + UUID.randomUUID();
      String label = "java-tck-sync";
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      try {
         ambassador.connect(recorder.proxy(), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, requireFom(), timeImplementation());
         created = true;
         ambassador.joinFederationExecution("java-tck-sync", federation);
         joined = true;
         ambassador.registerFederationSynchronizationPoint(label, new byte[] { 1, 2, 3 });
         drain(ambassador);
         check(recorder.synchronizationAnnouncements > 0,
            "announceSynchronizationPoint did not cross the callback interface");
         ambassador.synchronizationPointAchieved(label, true);
         drain(ambassador);
         check(recorder.synchronizationsCompleted > 0,
            "federationSynchronized did not cross the callback interface");
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void saveAndRestore() throws Exception {
      RTIambassador ambassador = factory().getRtiAmbassador();
      CallbackRecorder recorder = new CallbackRecorder();
      String federation = "java-tck-save-" + UUID.randomUUID();
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      try {
         ambassador.connect(recorder.proxy(), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, requireFom(), timeImplementation());
         created = true;
         ambassador.joinFederationExecution("java-tck-save", federation);
         joined = true;
         ambassador.requestFederationSave("java-tck-save");
         drain(ambassador);
         check(recorder.saveInitiations > 0,
            "initiateFederateSave did not cross the callback interface");
         ambassador.federateSaveBegun();
         ambassador.federateSaveComplete();
         drain(ambassador);
         check(recorder.federationSaved > 0,
            "federationSaved did not cross the callback interface");

         ambassador.requestFederationRestore("java-tck-save");
         drain(ambassador);
         check(recorder.restoreRequestSuccesses > 0,
            "requestFederationRestoreSucceeded did not cross the callback interface");
         check(recorder.restoreInitiations > 0,
            "initiateFederateRestore did not cross the callback interface");
         ambassador.federateRestoreComplete();
         drain(ambassador);
         check(recorder.federationRestored > 0,
            "federationRestored did not cross the callback interface");
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void momStatusReport() throws Exception {
      RTIambassador ambassador = factory().getRtiAmbassador();
      CallbackRecorder recorder = new CallbackRecorder();
      String federation = "java-tck-mom-" + UUID.randomUUID();
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      try {
         ambassador.connect(recorder.proxy(), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, requireFom(), timeImplementation());
         created = true;
         ambassador.joinFederationExecution("java-tck-mom", federation);
         joined = true;
         ambassador.queryFederationSaveStatus();
         drain(ambassador);
         check(recorder.saveStatusResponses > 0,
            "federationSaveStatusResponse did not cross the callback interface");
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void serviceReportInteraction() throws Exception {
      ServiceReportInteractionTck.run(factory(), requireFom(), requireMim(),
         interactionClassName(), parameterName(), timeImplementation(), callbackModel());
   }

   private static void serviceReportResignFederationExecution() throws Exception {
      ServiceReportResignTck.run(factory(), requireFom(), requireMim(),
         timeImplementation(), callbackModel());
   }

   private static String configuredMode(Scenario scenario) {
      Properties profile = capabilityProfile();
      String configured = profile.getProperty("scenario." + scenario.id);
      if (configured == null || configured.trim().isEmpty()) {
         return scenario.defaultEnabled ? "run" : "unsupported";
      }
      String normalized = configured.trim().toLowerCase();
      if ("pass".equals(normalized) || "run".equals(normalized)) {
         return "run";
      }
      if ("not_applicable".equals(normalized) || "not-applicable".equals(normalized)) {
         return "not applicable";
      }
      if ("unsupported".equals(normalized) || "skip".equals(normalized)) {
         return "unsupported";
      }
      throw new IllegalArgumentException("unknown capability profile mode for "
         + scenario.id + ": " + configured);
   }

   private static Properties capabilityProfile() {
      if (capabilityProfile != null) {
         return capabilityProfile;
      }
      capabilityProfile = new Properties();
      String path = System.getProperty(PROFILE_PROPERTY);
      if (path == null || path.trim().isEmpty()) {
         return capabilityProfile;
      }
      try (java.io.InputStream input = Files.newInputStream(Path.of(path))) {
         capabilityProfile.load(input);
      } catch (IOException error) {
         throw new IllegalArgumentException("cannot read capability profile " + path, error);
      }
      return capabilityProfile;
   }

   private static String provider() {
      String configured = System.getProperty(PROVIDER_PROPERTY);
      if (configured != null && !configured.trim().isEmpty()) {
         return configured;
      }
      return discoveredProvider;
   }

   private static int count(String status) {
      int count = 0;
      for (TestResult result : results) {
         if (status.equals(result.status)) {
            count++;
         }
      }
      return count;
   }

   private static String failureSummary() {
      List<String> failures = new ArrayList<>();
      for (TestResult result : results) {
         if ("fail".equals(result.status)) {
            failures.add(result.scenario.id + ": " + result.message);
         }
      }
      return failures.toString();
   }

   private static void writeEvidenceArtifacts() throws IOException {
      String resultsPath = System.getProperty(RESULTS_PROPERTY);
      if (resultsPath != null && !resultsPath.trim().isEmpty()) {
         Path output = Path.of(resultsPath);
         if (output.getParent() != null) {
            Files.createDirectories(output.getParent());
         }
         Files.write(output, evidenceJson().getBytes(StandardCharsets.UTF_8));
      }
      String junitPath = System.getProperty(JUNIT_PROPERTY);
      if (junitPath != null && !junitPath.trim().isEmpty()) {
         Path output = Path.of(junitPath);
         if (output.getParent() != null) {
            Files.createDirectories(output.getParent());
         }
         Files.write(output, junitXml().getBytes(StandardCharsets.UTF_8));
      }
   }

   private static String evidenceJson() {
      StringWriter out = new StringWriter();
      out.append("{\n");
      jsonField(out, "schema_version", "1", true, 1);
      jsonField(out, "kind", quote("java-rti-tck-evidence"), true, 1);
      jsonField(out, "standard", quote("IEEE 1516.1-2025"), true, 1);
      jsonField(out, "provider", quote(provider()), true, 1);
      jsonField(out, "api_jar", quote(System.getProperty(API_JAR_PROPERTY, "")), true, 1);
      jsonField(out, "provider_jars", quote(System.getProperty(PROVIDER_JARS_PROPERTY, "")), true, 1);
      jsonField(out, "fom", quote(System.getProperty(FOM_PROPERTY, "")), true, 1);
      jsonField(out, "mim", quote(System.getProperty(MIM_PROPERTY, "")), true, 1);
      jsonField(out, "additional_fom",
         quote(System.getProperty(ADDITIONAL_FOM_PROPERTY, "")), true, 1);
      jsonField(out, "capability_profile", quote(System.getProperty(PROFILE_PROPERTY, "default")), true, 1);
      jsonField(out, "scenario_count", Integer.toString(results.size()), true, 1);
      out.append("  \"scenarios\": [\n");
      for (int i = 0; i < results.size(); i++) {
         TestResult result = results.get(i);
         out.append("    {\n");
         jsonField(out, "id", quote(result.scenario.id), true, 3);
         jsonField(out, "title", quote(result.scenario.title), true, 3);
         jsonField(out, "category", quote(result.scenario.category), true, 3);
         jsonField(out, "status", quote(result.status), true, 3);
         jsonField(out, "provider", quote(provider()), true, 3);
         jsonField(out, "java_test", quote(javaTestSelector(result.scenario)), true, 3);
         jsonField(out, "api_method", quote(result.scenario.apiMethod), true, 3);
         jsonArray(out, "requirement_ids", result.scenario.requirementIds, true, 3);
         jsonField(out, "message", quote(result.message), true, 3);
         jsonField(out, "evidence_artifact", quote(System.getProperty(RESULTS_PROPERTY, "")), false, 3);
         out.append("    }");
         if (i + 1 < results.size()) out.append(",");
         out.append("\n");
      }
      out.append("  ]\n}\n");
      return out.toString();
   }

   private static String junitXml() {
      int failures = count("fail");
      int skipped = count("unsupported") + count("not applicable");
      StringWriter out = new StringWriter();
      out.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
      out.append("<testsuite name=\"IEEE 1516.1-2025 Java RTI TCK\" tests=\"")
         .append(Integer.toString(results.size())).append("\" failures=\"")
         .append(Integer.toString(failures)).append("\" skipped=\"")
         .append(Integer.toString(skipped)).append("\">\n");
      for (TestResult result : results) {
         out.append("  <testcase classname=\"org.hla.rti.tck.RtiTckMain\" name=\"")
            .append(xml(result.scenario.id)).append("\">\n");
         out.append("    <properties><property name=\"requirement_ids\" value=\"")
            .append(xml(String.join(",", result.scenario.requirementIds)))
            .append("\"/><property name=\"api_method\" value=\"")
            .append(xml(result.scenario.apiMethod)).append("\"/><property name=\"provider\" value=\"")
            .append(xml(provider())).append("\"/></properties>\n");
         if ("fail".equals(result.status)) {
            out.append("    <failure message=\"").append(xml(result.message)).append("\">")
               .append(xml(result.message)).append("</failure>\n");
         } else if (!"pass".equals(result.status)) {
            out.append("    <skipped message=\"").append(xml(result.status + ": " + result.message))
               .append("\"/>\n");
         }
         out.append("  </testcase>\n");
      }
      out.append("</testsuite>\n");
      return out.toString();
   }

   private static void jsonField(StringWriter out, String name, String value,
         boolean comma, int indent) {
      out.append("  ".repeat(indent)).append(quote(name)).append(": ").append(value);
      if (comma) out.append(",");
      out.append("\n");
   }

   private static void jsonArray(StringWriter out, String name, String[] values,
         boolean comma, int indent) {
      out.append("  ".repeat(indent)).append(quote(name)).append(": [");
      for (int i = 0; i < values.length; i++) {
         if (i > 0) out.append(", ");
         out.append(quote(values[i]));
      }
      out.append("]");
      if (comma) out.append(",");
      out.append("\n");
   }

   private static String quote(String value) {
      if (value == null) return "null";
      return "\"" + value.replace("\\", "\\\\").replace("\"", "\\\"")
         .replace("\r", "\\r").replace("\n", "\\n") + "\"";
   }

   private static String xml(String value) {
      return value.replace("&", "&amp;").replace("\"", "&quot;")
         .replace("<", "&lt;").replace(">", "&gt;");
   }

   private static String javaTestSelector(Scenario scenario) {
      String method;
      switch (scenario.id) {
         case "java-tck.factory-discovery": method = "factoryDiscovery"; break;
         case "java-tck.encoder-round-trip": method = "encoderRoundTrip"; break;
         case "java-tck.federation-membership": method = "federationMembership"; break;
         case "java-tck.explicit-mim-creation": method = "explicitMimCreation"; break;
         case "java-tck.federation-mom-current-fdd": method = "federationMomCurrentFdd"; break;
         case "java-tck.federation-mom-fom-module-designator-list":
            method = "federationMomFomModuleDesignatorList"; break;
         case "java-tck.federation-mom-static-identity":
            method = "federationMomStaticIdentity"; break;
         case "java-tck.federation-mom-static-switches":
            method = "federationMomStaticSwitches"; break;
         case "java-tck.federation-mom-federates-in-federation":
            method = "federationMomFederatesInFederation"; break;
         case "java-tck.federation-mom-content-reports":
            method = "federationMomContentReports"; break;
         case "java-tck.callback-reentrancy": method = "callbackReentrancy"; break;
         case "java-tck.logical-time-factory": method = "logicalTimeFactory"; break;
         case "java-tck.declaration-management": method = "declarationManagement"; break;
         case "java-tck.object-management": method = "objectRegistrationAndReflection"; break;
         case "java-tck.object-attribute-declarations-valid-inputs-after-resignation":
            method = "objectAttributeDeclarationsAfterResignation"; break;
          case "java-tck.object-registration-discovery-lifecycle":
             method = "objectRegistrationDiscoveryLifecycle"; break;
         case "java-tck.attribute-value-update-request-baseline":
            method = "attributeValueUpdateRequestBaseline"; break;
         case "java-tck.object-class-attribute-value-update-request-baseline":
            method = "objectClassAttributeValueUpdateRequestBaseline"; break;
         case "java-tck.attribute-value-update-response":
            method = "attributeValueUpdateResponse"; break;
         case "java-tck.attribute-value-update-request-multi-requester":
            method = "attributeValueUpdateMultiRequester"; break;
         case "java-tck.object-registration-discovery-multi-recipient":
            method = "objectRegistrationMultiRecipient"; break;
         case "java-tck.ordinary-multi-attribute-subscription-projection":
            method = "multiAttributeSubscriptionProjection"; break;
         case "java-tck.ordinary-multi-attribute-value-update-request-response":
            method = "multiAttributeValueUpdateRequestResponse"; break;
         case "java-tck.object-removal-multi-recipient-fifo":
            method = "objectRemovalMultiRecipientFifo"; break;
         case "java-tck.object-deletion-service-boundaries":
            method = "objectDeletionServiceBoundaries"; break;
         case "java-tck.receive-order-object-removal-subscription-withdrawal":
            method = "receiveOrderObjectRemovalSubscriptionWithdrawal"; break;
         case "java-tck.resign-delete-objects-multi-recipient-fifo":
            method = "resignDeleteObjectsMultiRecipientFifo"; break;
         case "java-tck.resign-delete-objects":
            method = "resignDeleteObjects"; break;
         case "java-tck.attribute-multi-recipient-fifo":
            method = "attributeMultiRecipientFifo"; break;
         case "java-tck.attribute-interaction": method = "attributeInteraction"; break;
         case "java-tck.support-services": method = "supportServices"; break;
         case "java-tck.ordinary-edges": method = "ordinaryEdges"; break;
         case "java-tck.relevance-advisories": method = "relevanceAdvisories"; break;
         case "java-tck.directed-interactions": method = "directedInteractions"; break;
         case "java-tck.directed-interaction-declarations-valid-inputs-after-resignation":
            method = "directedInteractionDeclarationsAfterResignation"; break;
         case "java-tck.directed-interaction-target-lifecycle":
            method = "directedInteractionTargetLifecycle"; break;
         case "java-tck.transport-order": method = "transportOrder"; break;
         case "java-tck.receive-order-attribute-update-callback-cancellation":
            method = "receiveOrderAttributeUpdateCallbackCancellation"; break;
          case "java-tck.receive-order-multi-attribute-update-callback-cancellation":
             method = "receiveOrderMultiAttributeUpdateCallbackCancellation"; break;
          case "java-tck.receive-order-interaction-callback-cancellation":
             method = "receiveOrderInteractionCallbackCancellation"; break;
          case "java-tck.interaction-subscription-lifecycle":
             method = "interactionSubscriptionLifecycle"; break;
          case "java-tck.interaction-publication-send-fence":
             method = "interactionPublicationSendFence"; break;
          case "java-tck.receive-order-interaction":
             method = "receiveOrderInteraction"; break;
          case "java-tck.interaction-publication-send-transition":
             method = "interactionPublicationSendTransition"; break;
          case "java-tck.interaction-multi-recipient-fifo":
             method = "interactionMultiRecipientFifo"; break;
          case "java-tck.directed-interaction-multi-recipient-fifo":
             method = "directedInteractionMultiRecipientFifo"; break;
          case "java-tck.directed-interaction-mixed-subscription-fanout":
             method = "directedInteractionMixedSubscriptionFanout"; break;
         case "java-tck.directed-interaction-subscription-kind":
            method = "directedInteractionSubscriptionKind"; break;
         case "java-tck.directed-derived-object-target-eligibility":
            method = "directedDerivedObjectTargetEligibility"; break;
         case "java-tck.order-type-controls": method = "orderTypeControls"; break;
         case "java-tck.api-surface-inventory": method = "apiSurfaceInventory"; break;
         case "java-tck.overloads-and-exceptions": method = "overloadsAndExceptions"; break;
         case "java-tck.malformed-inputs": method = "malformedInputs"; break;
         case "java-tck.time-advance": method = "timeAdvanceAndGrants"; break;
         case "java-tck.timestamped-interaction-mixed-advances":
            method = "timestampedInteractionMixedAdvances"; break;
          case "java-tck.timestamped-interaction-retraction":
             method = "timestampedInteractionRetraction"; break;
          case "java-tck.timestamped-interaction-cross-producer-order":
             method = "timestampedInteractionCrossProducerOrder"; break;
           case "java-tck.timestamped-interaction-no-fanout":
              method = "timestampedInteractionNoFanout"; break;
           case "java-tck.timestamped-interaction-retraction-fanout":
              method = "timestampedInteractionRetractionFanout"; break;
           case "java-tck.timestamped-interaction-regulation-reenable":
              method = "timestampedInteractionRegulationReenable"; break;
           case "java-tck.timestamped-interaction-reenable":
              method = "timestampedInteractionConstrainedReenable"; break;
           case "java-tck.timestamped-directed-interaction-reenable":
              method = "timestampedDirectedInteractionConstrainedReenable"; break;
           case "java-tck.timestamped-directed-interaction-regulation-reenable":
              method = "timestampedDirectedInteractionRegulationReenable"; break;
           case "java-tck.timestamped-directed-interaction-tar-nmr":
              method = "timestampedDirectedInteractionTarNmr"; break;
           case "java-tck.timestamped-directed-interaction-source-resignation":
              method = "timestampedDirectedInteractionSourceResignation"; break;
           case "java-tck.timestamped-directed-interaction-source-resignation-fanout":
              method = "timestampedDirectedInteractionSourceResignationFanout"; break;
           case "java-tck.timestamped-directed-interaction-immediate-source-resignation":
              method = "timestampedDirectedInteractionImmediateSourceResignation"; break;
           case "java-tck.timestamped-directed-interaction-retraction-fanout":
              method = "timestampedDirectedInteractionRetractionFanout"; break;
           case "java-tck.timestamped-directed-interaction-retraction":
              method = "timestampedDirectedInteractionRetraction"; break;
           case "java-tck.timestamped-directed-interaction-derived-object-target":
              method = "timestampedDirectedInteractionDerivedObjectTarget"; break;
           case "java-tck.timestamped-directed-alternate-advances":
              method = "timestampedDirectedAlternateAdvances"; break;
           case "java-tck.directed-interaction-derived-object-target":
              method = "directedInteractionDerivedObjectTarget"; break;
         case "java-tck.timestamped-attribute-update": method = "timestampedAttributeUpdate"; break;
         case "java-tck.timestamped-attribute-ownership-transfer":
            method = "timestampedAttributeOwnershipTransfer"; break;
         case "java-tck.timestamped-attribute-source-resignation":
            method = "timestampedAttributeSourceResignation"; break;
         case "java-tck.timestamped-attribute-order-cohort":
            method = "timestampedAttributeOrderCohort"; break;
          case "java-tck.timestamped-attribute-update-alternate-advances":
             method = "timestampedAttributeUpdateAlternateAdvances"; break;
         case "java-tck.timestamped-object-removal": method = "timestampedObjectRemoval"; break;
         case "java-tck.timestamped-object-deletion-mixed-advances":
            method = "timestampedObjectDeletionMixedAdvances"; break;
         case "java-tck.timestamped-object-deletion-no-fanout":
            method = "timestampedObjectDeletionNoFanout"; break;
         case "java-tck.timestamped-object-deletion-tombstone":
            method = "timestampedObjectDeletionTombstone"; break;
         case "java-tck.timestamped-object-deletion-retraction":
            method = "timestampedObjectDeletionRetraction"; break;
         case "java-tck.timestamped-object-deletion-regulation-reenable":
            method = "timestampedObjectDeletionRegulationReenable"; break;
         case "java-tck.timestamped-object-deletion-retraction-joined-owners":
            method = "timestampedObjectDeletionRetractionJoinedOwners"; break;
         case "java-tck.timestamped-object-deletion-source-resignation-fanout":
            method = "timestampedObjectDeletionSourceResignationFanout"; break;
         case "java-tck.save-restore": method = "saveAndRestore"; break;
         case "java-tck.synchronization": method = "synchronizationPoint"; break;
         case "java-tck.mom": method = "momStatusReport"; break;
         case "java-tck.service-report-interaction": method = "serviceReportInteraction"; break;
         case "java-tck.service-report-resign-federation-execution":
            method = "serviceReportResignFederationExecution"; break;
         case "java-tck.ddm": method = "dataDistributionManagement"; break;
         case "java-tck.ownership": method = "ownershipManagement"; break;
         default: method = "unsupportedPortableFamily"; break;
      }
      return "org.hla.rti.tck.RtiTckMain#" + method;
   }

   private static void expectException(CheckedTest operation, String expected,
         String message) throws Exception {
      expectException(operation, new String[] {expected}, message);
   }

   private static void expectException(CheckedTest operation, String expectedOne,
         String expectedTwo, String message) throws Exception {
      expectException(operation, new String[] {expectedOne, expectedTwo}, message);
   }

   private static void expectException(CheckedTest operation, String expectedOne,
         String expectedTwo, String expectedThree, String message) throws Exception {
      expectException(operation, new String[] {expectedOne, expectedTwo, expectedThree}, message);
   }

   private static void expectException(CheckedTest operation, String[] expected,
         String message) throws Exception {
      try {
         operation.run();
      } catch (Throwable error) {
         Throwable current = error;
         while (current != null) {
            String simple = current.getClass().getSimpleName();
            for (String expectedName : expected) {
               if (expectedName.equals(simple)) return;
            }
            current = current.getCause();
         }
         throw new AssertionError(message + "; got " + error, error);
      }
      throw new AssertionError(message);
   }

   private static void logicalTimeFactory() throws Exception {
      String fom = requireFom();
      RtiFactory factory = factory();
      RTIambassador ambassador = factory.getRtiAmbassador();
      CallbackRecorder recorder = new CallbackRecorder();
      String federation = "java-tck-time-" + UUID.randomUUID();
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      try {
         ambassador.connect(recorder.proxy(), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation());
         created = true;
         ambassador.joinFederationExecution("java-tck-time", federation);
         joined = true;
         LogicalTimeFactory<?, ?> timeFactory = ambassador.getTimeFactory();
         check(timeFactory != null && !timeFactory.getName().isEmpty(),
            "logical-time factory has no standard name");
         LogicalTime<?, ?> initial = timeFactory.makeInitial();
         LogicalTime<?, ?> finalTime = timeFactory.makeFinal();
         LogicalTimeInterval<?> zero = timeFactory.makeZero();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         check(initial.isInitial(), "initial logical time is not initial");
         check(finalTime.isFinal(), "final logical time is not final");
         check(zero.isZero(), "zero interval is not zero");
         check(epsilon.isEpsilon(), "epsilon interval is not epsilon");
         byte[] encoded = new byte[zero.encodedLength()];
         zero.encode(encoded, 0);
         check(timeFactory.decodeInterval(encoded, 0).equals(zero),
            "logical-time interval did not encode/decode through the standard factory");
      } finally {
         if (joined) {
            try {
               ambassador.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (created) {
            try {
               ambassador.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (connected) {
            try {
               ambassador.disconnect();
            } catch (Exception ignored) {
            }
         }
      }
   }

   /**
    * Exercise the standard declaration-management surface against a real FOM
    * class.  This deliberately uses only names and handles from the standard
    * API, so the same scenario can run against a different Java RTI provider.
    */
   private static void declarationManagement() throws Exception {
      FederationBasicsTck.declarationManagement(factory(), requireFom(), timeImplementation(),
         objectClassName(), attributeName());
   }

   private static void objectRegistrationAndReflection() throws Exception {
      FederationBasicsTck.objectRegistrationAndReflection(factory(), requireFom(),
         timeImplementation(), objectClassName(), attributeName());
   }

   private static void attributeInteraction() throws Exception {
      AttributeInteractionTck.ordinaryRoutes(factory(), requireFom(), timeImplementation(),
         objectClassName(), attributeName(), interactionClassName(), parameterName());
   }

   private static void supportServices() throws Exception {
      SupportServicesTck.run(factory(), requireFom(), timeImplementation(),
         supportObjectClassName(), supportAttributeName(), supportInteractionClassName(),
         supportParameterName(), dimensionName());
   }

   private static void ordinaryEdges() throws Exception {
      OrdinaryEdgesTck.run(factory(), requireFom(), timeImplementation(), objectClassName(),
         attributeName(), interactionClassName(), parameterName());
   }

   private static void relevanceAdvisories() throws Exception {
      RelevanceAdvisoryTck.run(factory(), requireFom(), timeImplementation(), objectClassName(),
         attributeName(), directedInteractionClassName());
   }

   private static void directedInteractions() throws Exception {
      DirectedInteractionsTck.run(factory(), requireFom(), timeImplementation(), objectClassName(),
         attributeName(), directedInteractionClassName());
   }

   private static void directedInteractionDeclarationsAfterResignation() throws Exception {
      DirectedInteractionDeclarationsAfterResignationTck.run(factory(), requireFom(),
         timeImplementation(), objectClassName(), interactionClassName(), callbackModel());
   }

   private static void directedInteractionTargetLifecycle() throws Exception {
      DirectedInteractionTargetLifecycleTck.run(factory(), requireFom(), timeImplementation(),
         objectClassName(), attributeName(), directedInteractionClassName(), callbackModel());
   }

   private static void transportOrder() throws Exception {
      TransportOrderTck.run(factory(), requireFom(), timeImplementation(), objectClassName(),
         attributeName(), interactionClassName());
   }

   private static void receiveOrderAttributeUpdateCallbackCancellation() throws Exception {
      ReceiveOrderAttributeCallbackCancellationTck.run(factory(), requireFom(),
         objectClassName(), attributeName(), callbackModel());
   }

   private static void receiveOrderMultiAttributeUpdateCallbackCancellation() throws Exception {
      ReceiveOrderMultiAttributeCallbackCancellationTck.run(factory(), requireFom(),
         multiAttributeObjectClassName(), multiAttributeFirstName(), multiAttributeSecondName(),
         callbackModel());
   }

   private static void receiveOrderInteractionCallbackCancellation() throws Exception {
      ReceiveOrderInteractionCallbackCancellationTck.run(factory(), requireFom(),
         interactionClassName(), parameterName(), callbackModel());
   }

   private static void interactionSubscriptionLifecycle() throws Exception {
      InteractionSubscriptionLifecycleTck.run(factory(), requireFom(),
         interactionClassName(), parameterName(), callbackModel());
   }

   private static void interactionPublicationSendFence() throws Exception {
      InteractionPublicationSendFenceTck.run(factory(), requireFom(),
         interactionClassName(), parameterName(), callbackModel());
   }

   private static void receiveOrderInteraction() throws Exception {
      ReceiveOrderInteractionTck.run(factory(), requireFom(),
         interactionClassName(), parameterName(), callbackModel());
   }

   private static void interactionPublicationSendTransition() throws Exception {
      InteractionPublicationSendTransitionTck.run(factory(), requireFom(),
         interactionClassName(), parameterName(), callbackModel());
   }

   private static void interactionMultiRecipientFifo() throws Exception {
      InteractionMultiRecipientFifoTck.run(factory(), requireFom(),
         interactionClassName(), parameterName(), callbackModel());
   }

   private static void directedInteractionMultiRecipientFifo() throws Exception {
      DirectedInteractionMultiRecipientFifoTck.run(factory(), requireFom(),
         objectClassName(), attributeName(), interactionClassName(), parameterName(),
         callbackModel());
   }

   private static void directedInteractionMixedSubscriptionFanout() throws Exception {
      DirectedInteractionMixedSubscriptionFanoutTck.run(factory(), requireFom(),
         objectClassName(), attributeName(), interactionClassName(), parameterName(),
         callbackModel());
   }

   private static void directedInteractionSubscriptionKind() throws Exception {
      DirectedInteractionSubscriptionKindTck.run(factory(), requireFom(), objectClassName(),
         attributeName(), interactionClassName(), parameterName(), callbackModel());
   }

   private static void directedDerivedObjectTargetEligibility() throws Exception {
      DirectedInteractionSubscriptionKindTck.run(factory(), requireFom(),
         typedDerivedObjectClassName(), typedIdentityAttributeName(),
         typedInteractionClassName(), typedIntegerParameterName(), callbackModel());
   }

    private static void objectRegistrationDiscoveryLifecycle() throws Exception {
       ObjectRegistrationDiscoveryLifecycleTck.run(factory(), requireFom(),
          typedObjectClassName(), typedDerivedObjectClassName(),
          typedIdentityAttributeName(), typedDerivedAttributeName(), callbackModel());
    }

    private static void attributeValueUpdateRequestBaseline() throws Exception {
       AttributeValueUpdateRequestBaselineTck.run(factory(), requireFom(), timeImplementation(),
          multiAttributeObjectClassName(), multiAttributeFirstName(), multiAttributeSecondName(),
          callbackModel());
    }

    private static void objectClassAttributeValueUpdateRequestBaseline() throws Exception {
       ObjectClassAttributeValueUpdateRequestBaselineTck.run(factory(), requireFom(),
          timeImplementation(), typedObjectClassName(), typedDerivedObjectClassName(),
          typedIdentityAttributeName(), typedIntegerAttributeName(), typedDerivedAttributeName(),
          callbackModel());
    }

    private static void attributeValueUpdateResponse() throws Exception {
       AttributeValueUpdateResponseTck.run(factory(), requireFom(), timeImplementation(),
          objectClassName(), attributeName(), callbackModel());
    }

    private static void attributeValueUpdateMultiRequester() throws Exception {
       AttributeValueUpdateMultiRequesterTck.run(factory(), requireFom(), timeImplementation(),
          objectClassName(), attributeName(), callbackModel());
    }

    private static void objectRegistrationMultiRecipient() throws Exception {
       ObjectRegistrationMultiRecipientTck.run(factory(), requireFom(), objectClassName(),
          attributeName(), callbackModel());
    }

    private static void multiAttributeSubscriptionProjection() throws Exception {
       MultiAttributeSubscriptionProjectionTck.run(factory(), requireFom(), timeImplementation(),
          multiAttributeObjectClassName(), multiAttributeFirstName(),
          multiAttributeSecondName(), callbackModel());
    }

    private static void multiAttributeValueUpdateRequestResponse() throws Exception {
       MultiAttributeValueUpdateRequestResponseTck.run(factory(), requireFom(),
          timeImplementation(), multiAttributeObjectClassName(), multiAttributeFirstName(),
          multiAttributeSecondName(), callbackModel());
    }

    private static void objectRemovalMultiRecipientFifo() throws Exception {
       ObjectRemovalMultiRecipientFifoTck.run(factory(), requireFom(), timeImplementation(),
          objectClassName(), attributeName(), callbackModel());
    }

    private static void objectDeletionServiceBoundaries() throws Exception {
       ObjectDeletionServiceBoundariesTck.run(factory(), requireFom(), objectClassName(),
          attributeName(), callbackModel());
    }

    private static void receiveOrderObjectRemovalSubscriptionWithdrawal() throws Exception {
       ObjectRemovalSubscriptionWithdrawalTck.run(factory(), requireFom(), objectClassName(),
          attributeName(), callbackModel());
    }

    private static void resignDeleteObjectsMultiRecipientFifo() throws Exception {
       ResignDeleteObjectsMultiRecipientFifoTck.run(factory(), requireFom(), timeImplementation(),
          objectClassName(), attributeName(), callbackModel());
    }

    private static void resignDeleteObjects() throws Exception {
       ResignDeleteObjectsTck.run(factory(), requireFom(), timeImplementation(),
          objectClassName(), attributeName(), callbackModel());
    }

    private static void attributeMultiRecipientFifo() throws Exception {
       AttributeMultiRecipientFifoTck.run(factory(), requireFom(), objectClassName(),
          attributeName(), callbackModel());
    }

    private static void objectAttributeDeclarationsAfterResignation() throws Exception {
       ObjectAttributeDeclarationsAfterResignationTck.run(factory(), requireFom(),
          timeImplementation(), objectClassName(), attributeName(), callbackModel());
    }

   private static void orderTypeControls() throws Exception {
      OrderTypeControlsTck.run(factory(), requireFom(), timeImplementation(),
         objectClassName(), attributeName(), interactionClassName(), parameterName(),
         callbackModel());
   }

   private static void drain(RTIambassador ambassador) throws Exception {
      // Providers may enqueue discovery and reflection separately.  Keep the
      // bound finite so a broken provider cannot hang a portable TCK run.
      for (int i = 0; i < 64; ++i) {
         ambassador.evokeCallback(0.0);
      }
   }

   private static String requireFom() {
      String value = System.getProperty(FOM_PROPERTY);
      check(value != null && !value.isEmpty(),
         "set -D" + FOM_PROPERTY + " to a provider-compatible FOM module");
      check(Files.isRegularFile(Path.of(value)), "FOM module does not exist: " + value);
      return value;
   }

   private static String requireMim() {
      String value = System.getProperty(MIM_PROPERTY);
      check(value != null && !value.isEmpty(),
         "set -D" + MIM_PROPERTY + " to an adapter-supplied standard MIM module");
      check(Files.isRegularFile(Path.of(value)), "MIM module does not exist: " + value);
      return value;
   }

   private static String requireAdditionalFom() {
      String value = System.getProperty(ADDITIONAL_FOM_PROPERTY);
      check(value != null && !value.isEmpty(),
         "set -D" + ADDITIONAL_FOM_PROPERTY + " to an adapter-supplied additional FOM module");
      check(Files.isRegularFile(Path.of(value)),
         "additional FOM module does not exist: " + value);
      return value;
   }

   private static String timeImplementation() {
      return System.getProperty(TIME_PROPERTY, "HLAinteger64Time");
   }

   private static CallbackModel callbackModel() {
      String value = System.getProperty(CALLBACK_MODEL_PROPERTY, "HLA_EVOKED");
      try {
         return CallbackModel.valueOf(value);
      } catch (IllegalArgumentException error) {
         throw new IllegalArgumentException(
            "set -D" + CALLBACK_MODEL_PROPERTY + " to HLA_EVOKED or HLA_IMMEDIATE", error);
      }
   }

   private static String objectClassName() {
      return System.getProperty(OBJECT_CLASS_PROPERTY,
         "HLAobjectRoot.Employee.Server");
   }

   private static String attributeName() {
      return System.getProperty(ATTRIBUTE_PROPERTY, "Efficiency");
   }

   private static String multiAttributeObjectClassName() {
      return System.getProperty("hla.rti.tck.multiAttributeObjectClass",
         "HLAobjectRoot.TckMultiAttributeObject");
   }

   private static String multiAttributeFirstName() {
      return System.getProperty("hla.rti.tck.multiAttributeFirst", "FirstValue");
   }

   private static String multiAttributeSecondName() {
      return System.getProperty("hla.rti.tck.multiAttributeSecond", "SecondValue");
   }

   private static String interactionClassName() {
      return System.getProperty(INTERACTION_CLASS_PROPERTY,
         "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
   }

   private static String parameterName() {
      return System.getProperty(PARAMETER_PROPERTY, "TemperatureOk");
   }

   private static String supportObjectClassName() {
      return System.getProperty("hla.rti.tck.supportObjectClass",
         "HLAobjectRoot.Food.Drink");
   }

   private static String supportAttributeName() {
      return System.getProperty("hla.rti.tck.supportAttribute", "NumberCups");
   }

   private static String supportInteractionClassName() {
      return System.getProperty("hla.rti.tck.supportInteractionClass",
         "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
   }

   private static String supportParameterName() {
      return System.getProperty("hla.rti.tck.supportParameter", "TemperatureOk");
   }

   private static String directedInteractionClassName() {
      return System.getProperty(DIRECTED_INTERACTION_CLASS_PROPERTY,
         "HLAinteractionRoot.ServerAction.TakeOrder");
   }

   private static String timestampedDirectedObjectClassName() {
      return System.getProperty(TIMESTAMPED_DIRECTED_OBJECT_CLASS_PROPERTY,
         "HLAobjectRoot.TckSecondaryObject");
   }

   private static String timestampedDirectedAttributeName() {
      return System.getProperty(TIMESTAMPED_DIRECTED_ATTRIBUTE_PROPERTY, "Value");
   }

   private static String timestampedDirectedInteractionClassName() {
      return System.getProperty(TIMESTAMPED_DIRECTED_INTERACTION_CLASS_PROPERTY,
         "HLAinteractionRoot.TckSecondaryDirectedInteraction");
   }

   private static String timestampedDirectedParameterName() {
      return System.getProperty(TIMESTAMPED_DIRECTED_PARAMETER_PROPERTY, "SecondaryPayload");
   }

   private static String typedDerivedObjectClassName() {
      return System.getProperty("hla.rti.tck.typedDerivedObjectClass",
         System.getProperty("hla.rti.tck.timestampedDirectedDerivedObjectClass",
            "HLAobjectRoot.TckTypedObject.TckTypedDerivedObject"));
   }

    private static String typedObjectClassName() {
       return System.getProperty("hla.rti.tck.typedObjectClass",
          "HLAobjectRoot.TckTypedObject");
    }

   private static String typedIdentityAttributeName() {
      return System.getProperty("hla.rti.tck.typedIdentityAttribute", "Identity");
   }

   private static String typedIntegerAttributeName() {
      return System.getProperty("hla.rti.tck.typedIntegerAttribute", "IntegerValue");
   }

    private static String typedDerivedAttributeName() {
       return System.getProperty("hla.rti.tck.typedDerivedAttribute", "DerivedValue");
    }

   private static String typedInteractionClassName() {
      return System.getProperty("hla.rti.tck.typedInteractionClass",
         "HLAinteractionRoot.TckTypedInteraction");
   }

   private static String typedIntegerParameterName() {
      return System.getProperty("hla.rti.tck.typedIntegerParameter", "IntegerParameter");
   }

   private static String ddmObjectClassName() {
      return System.getProperty(DDM_OBJECT_CLASS_PROPERTY,
         "HLAobjectRoot.Food.Drink");
   }

   private static String ddmAttributeName() {
      return System.getProperty(DDM_ATTRIBUTE_PROPERTY, "NumberCups");
   }

   private static String dimensionName() {
      return System.getProperty(DIMENSION_PROPERTY, "BarQuantity");
   }

   private static void check(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }

   private interface CheckedTest {
      void run() throws Exception;
   }

   private interface ConnectOperation {
      ConfigurationResult connect(RTIambassador ambassador, FederateAmbassador federate)
            throws Exception;
   }

   private static final class Scenario {
      private final String id;
      private final String title;
      private final String category;
      private final String apiMethod;
      private final String[] requirementIds;
      private final boolean defaultEnabled;

      private Scenario(String id, String title, String category, String apiMethod,
            String[] requirementIds, boolean defaultEnabled) {
         this.id = id;
         this.title = title;
         this.category = category;
         this.apiMethod = apiMethod;
         this.requirementIds = requirementIds;
         this.defaultEnabled = defaultEnabled;
      }
   }

   private static final class TestResult {
      private final Scenario scenario;
      private final String status;
      private final String message;

      private TestResult(Scenario scenario, String status, String message) {
         this.scenario = scenario;
         this.status = status;
         this.message = message == null ? "" : message;
      }

      private static TestResult passed(Scenario scenario) {
         return new TestResult(scenario, "pass", "");
      }

      private static TestResult failed(Scenario scenario, String message) {
         return new TestResult(scenario, "fail", message);
      }

      private static TestResult skipped(Scenario scenario, String status, String message) {
         return new TestResult(scenario, status, message);
      }
   }

   private static final class UnsupportedPortableScenario extends RuntimeException {
      private UnsupportedPortableScenario(String message) {
         super(message);
      }
   }

   private static final class CallbackRecorder {
      private int memberReports;
      private int timeRegulationEnabled;
      private int timeConstrainedEnabled;
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();
      private final List<LogicalTime<?, ?>> flushQueueGrantTimes = new ArrayList<>();
      private int synchronizationAnnouncements;
      private int synchronizationsCompleted;
      private int saveInitiations;
      private int federationSaved;
      private int restoreRequestSuccesses;
      private int restoreInitiations;
      private int federationRestored;
      private int saveStatusResponses;
      private int ownershipInformReports;
      private int ownershipAcquisitions;
      private ObjectInstanceHandle discoveredObject;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] { FederateAmbassador.class },
            (proxy, method, arguments) -> {
               if ("reportFederationExecutionMembers".equals(method.getName())) {
                  memberReports++;
               }
               if ("timeRegulationEnabled".equals(method.getName())) {
                  timeRegulationEnabled++;
               }
               if ("timeConstrainedEnabled".equals(method.getName())) {
                  timeConstrainedEnabled++;
               }
               if ("timeAdvanceGrant".equals(method.getName())) {
                  if (arguments != null && arguments.length >= 1
                        && arguments[0] instanceof LogicalTime<?, ?>) {
                     timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  }
               }
               if ("flushQueueGrant".equals(method.getName())
                     && arguments != null && arguments.length >= 1
                     && arguments[0] instanceof LogicalTime<?, ?>) {
                  flushQueueGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               }
               if ("announceSynchronizationPoint".equals(method.getName())) {
                  synchronizationAnnouncements++;
               }
               if ("federationSynchronized".equals(method.getName())) {
                  synchronizationsCompleted++;
               }
               if ("initiateFederateSave".equals(method.getName())) {
                  saveInitiations++;
               }
               if ("federationSaved".equals(method.getName())) {
                  federationSaved++;
               }
               if ("requestFederationRestoreSucceeded".equals(method.getName())) {
                  restoreRequestSuccesses++;
               }
               if ("initiateFederateRestore".equals(method.getName())) {
                  restoreInitiations++;
               }
               if ("federationRestored".equals(method.getName())) {
                  federationRestored++;
               }
               if ("federationSaveStatusResponse".equals(method.getName())) {
                  saveStatusResponses++;
               }
               if ("informAttributeOwnership".equals(method.getName())) {
                  ownershipInformReports++;
               }
               if ("attributeOwnershipAcquisitionNotification".equals(method.getName())) {
                  ownershipAcquisitions++;
               }
               if ("discoverObjectInstance".equals(method.getName())) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(method.getName())) return "Java RTI TCK callbacks";
                  if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
                  if ("equals".equals(method.getName())) return proxy == arguments[0];
               }
               return null;
            });
      }
   }
}
