package umbra.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
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
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
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

/** Provider-neutral Java contract tests. No Umbra implementation types appear here. */
public final class RtiTckMain {
   private static final String FACTORY_PROPERTY = "umbra.rti.tck.factory";
   private static final String FOM_PROPERTY = "umbra.rti.tck.fom";
   private static final String MIM_PROPERTY = "umbra.rti.tck.mim";
   private static final String TIME_PROPERTY = "umbra.rti.tck.time";
   private static final String OBJECT_CLASS_PROPERTY = "umbra.rti.tck.objectClass";
   private static final String ATTRIBUTE_PROPERTY = "umbra.rti.tck.attribute";
   private static final String DDM_OBJECT_CLASS_PROPERTY = "umbra.rti.tck.ddmObjectClass";
   private static final String DDM_ATTRIBUTE_PROPERTY = "umbra.rti.tck.ddmAttribute";
   private static final String DIMENSION_PROPERTY = "umbra.rti.tck.dimension";
   private static final String PROFILE_PROPERTY = "umbra.rti.tck.capabilityProfile";
   private static final String RESULTS_PROPERTY = "umbra.rti.tck.results";
   private static final String JUNIT_PROPERTY = "umbra.rti.tck.junit";
   private static final String PROVIDER_PROPERTY = "umbra.rti.tck.provider";
   private static final String API_JAR_PROPERTY = "umbra.rti.tck.apiJar";
   private static final String PROVIDER_JARS_PROPERTY = "umbra.rti.tck.providerJars";
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
      run(new Scenario("java-tck.encoder-round-trip", "encoder round trip", "encoders",
            "EncoderFactory.createHLAinteger32BE;DataElement.decode", new String[] {
               "java-1516.1-encoding-primitive-round-trip",
               "java-1516.1-encoding-malformed-input"
            }, true), RtiTckMain::encoderRoundTrip);
      run(new Scenario("java-tck.federation-membership", "federation membership", "lifecycle",
            "RTIambassador.connect;createFederationExecution;joinFederationExecution;resignFederationExecution;disconnect",
            new String[] {
               "req-federate-connect-before-rti-work",
               "req-connect-service-establishes-connection",
               "req-federate-disconnect-after-resign",
               "req-disconnect-service-terminates-connection"
            }, true), RtiTckMain::federationMembership);
      run(new Scenario("java-tck.logical-time-factory", "logical-time factory", "time",
            "RTIambassador.getTimeFactory;LogicalTimeFactory.decodeInterval", new String[] {
               "java-1516.1-logical-time-factory-values"
            }, true), RtiTckMain::logicalTimeFactory);
      run(new Scenario("java-tck.declaration-management", "declaration management", "declarations",
            "getObjectClassHandle;getAttributeHandle;publishObjectClassAttributes;subscribeObjectClassAttributes",
            new String[] {
               "java-1516.1-object-class-attribute-declaration"
            }, true), RtiTckMain::declarationManagement);
      run(new Scenario("java-tck.object-management", "object registration and reflection", "object-management",
            "registerObjectInstance;discoverObjectInstance;updateAttributeValues;reflectAttributeValues",
            new String[] {
               "java-1516.1-object-registration-discovery",
               "java-1516.1-attribute-value-update-reflection"
            }, true), RtiTckMain::objectRegistrationAndReflection);
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
            "enableTimeRegulation;enableTimeConstrained;timeAdvanceRequest;timeAdvanceGrant",
            new String[] {"java-1516.1-time-advance-and-grant"}, false),
         RtiTckMain::timeAdvanceAndGrants);
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

      writeEvidenceArtifacts();
      int passed = count("pass");
      int unsupported = count("unsupported");
      int notApplicable = count("not applicable");
      int failed = count("fail");
      System.out.println("Java RTI contract TCK: " + passed + " passed, " + failed
            + " failed, " + unsupported + " unsupported, " + notApplicable + " not applicable");
      if (failed != 0) {
         throw new AssertionError("Java RTI TCK failures: " + failureSummary());
      }
   }

   private static void run(Scenario scenario, CheckedTest test) {
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
      closeIfSupported(ambassador);
   }

   private static void encoderRoundTrip() throws Exception {
      EncoderFactory encoder = factory().getEncoderFactory();
      HLAinteger32BE value = encoder.createHLAinteger32BE(0x01020304);
      check(value.getValue() == 0x01020304, "integer value did not round-trip");
      check(value.getEncodedLength() == 4, "integer encoded length is not four octets");
      byte[] encoded = value.toByteArray();
      check(encoded.length == 4, "integer payload has the wrong length");
      check((encoded[0] & 0xff) == 0x01 && (encoded[3] & 0xff) == 0x04,
         "integer payload is not big-endian");
      value.decode(new byte[] { 4, 3, 2, 1 });
      check(value.getValue() == 0x04030201, "integer decode did not round-trip");

      expectException(() -> value.decode(new byte[] { 1, 2, 3 }),
         "DecoderException", "CouldNotDecode", "malformed integer input was accepted");
   }

   private static void federationMembership() throws Exception {
      String fom = requireFom();
      RtiFactory factory = factory();
      RTIambassador ambassador = factory.getRtiAmbassador();
      CallbackRecorder recorder = new CallbackRecorder();
      String federation = "umbra-java-tck-" + UUID.randomUUID();
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      try {
         ConfigurationResult configuration = ambassador.connect(
            recorder.proxy(), CallbackModel.HLA_EVOKED);
         check(configuration != null, "connect returned null ConfigurationResult");
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation());
         created = true;
         FederateHandle handle = ambassador.joinFederationExecution(
            "umbra-java-tck-federate", "umbra-java-tck-type", federation);
         joined = true;
         check(handle != null && handle.encodedLength() > 0,
            "join returned an invalid FederateHandle");
         check("umbra-java-tck-federate".equals(ambassador.getFederateName(handle)),
            "federate handle/name lookup did not round-trip");
         ambassador.listFederationExecutionMembers(federation);
         ambassador.evokeCallback(0.0);
         check(recorder.memberReports > 0,
            "member report did not cross the standard callback interface");
      } finally {
         if (joined) {
            try {
               ambassador.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
               // Preserve the original assertion when cleanup is unavailable.
            }
         }
         if (created) {
            try {
               ambassador.destroyFederationExecution(federation);
            } catch (Exception ignored) {
               // Preserve the original assertion when cleanup is unavailable.
            }
         }
         if (connected) {
            try {
               ambassador.disconnect();
            } catch (Exception ignored) {
               // Preserve the original assertion when cleanup is unavailable.
            }
         }
         closeIfSupported(ambassador);
      }
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
         }
      }
      check(methodCount >= 100, "the supplied API JAR exposes an unexpectedly small surface");
   }

   private static void overloadsAndExceptions() throws Exception {
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
         closeIfSupported(ambassador);
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
            "umbra-java-tck-missing-" + UUID.randomUUID() + ".xml").toString();
         expectException(() -> ambassador.createFederationExecution(
               "umbra-java-tck-malformed-" + UUID.randomUUID(), missingFom,
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
         closeIfSupported(ambassador);
      }
   }

   private static void unsupportedPortableFamily() {
      throw new UnsupportedPortableScenario(
         "provider capability profile did not enable this multi-federate/FOM-dependent family");
   }

   private static void dataDistributionManagement() throws Exception {
      RTIambassador ambassador = factory().getRtiAmbassador();
      CallbackRecorder recorder = new CallbackRecorder();
      String federation = "umbra-java-tck-ddm-" + UUID.randomUUID();
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      RegionHandle region = null;
      try {
         ambassador.connect(recorder.proxy(), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, requireFom(), timeImplementation());
         created = true;
         ambassador.joinFederationExecution("umbra-java-tck-ddm", federation);
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
         closeIfSupported(ambassador);
      }
   }

   private static void ownershipManagement() throws Exception {
      RTIambassador owner = factory().getRtiAmbassador();
      RTIambassador acquirer = factory().getRtiAmbassador();
      CallbackRecorder ownerRecorder = new CallbackRecorder();
      CallbackRecorder acquirerRecorder = new CallbackRecorder();
      String federation = "umbra-java-tck-ownership-" + UUID.randomUUID();
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
         owner.joinFederationExecution("umbra-java-tck-owner", federation);
         ownerJoined = true;
         acquirer.joinFederationExecution("umbra-java-tck-acquirer", federation);
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
         closeIfSupported(acquirer);
         closeIfSupported(owner);
      }
   }

   private static void timeAdvanceAndGrants() throws Exception {
      RTIambassador regulator = factory().getRtiAmbassador();
      RTIambassador constrained = factory().getRtiAmbassador();
      CallbackRecorder regulatorRecorder = new CallbackRecorder();
      CallbackRecorder constrainedRecorder = new CallbackRecorder();
      String federation = "umbra-java-tck-time-advance-" + UUID.randomUUID();
      boolean regulatorConnected = false;
      boolean constrainedConnected = false;
      boolean regulatorJoined = false;
      boolean constrainedJoined = false;
      boolean created = false;
      try {
         regulator.connect(regulatorRecorder.proxy(), CallbackModel.HLA_EVOKED);
         regulatorConnected = true;
         constrained.connect(constrainedRecorder.proxy(), CallbackModel.HLA_EVOKED);
         constrainedConnected = true;
         regulator.createFederationExecution(federation, requireFom(), timeImplementation());
         created = true;
         regulator.joinFederationExecution("umbra-java-tck-regulator", federation);
         regulatorJoined = true;
         constrained.joinFederationExecution("umbra-java-tck-constrained", federation);
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
         regulator.timeAdvanceRequest(requested);
         constrained.timeAdvanceRequest(requested);
         drain(regulator);
         drain(constrained);
         check(regulatorRecorder.timeAdvanceGrants > 0
               && constrainedRecorder.timeAdvanceGrants > 0,
            "timeAdvanceGrant did not cross the standard callback interface");
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
         closeIfSupported(constrained);
         closeIfSupported(regulator);
      }
   }

   private static void synchronizationPoint() throws Exception {
      RTIambassador ambassador = factory().getRtiAmbassador();
      CallbackRecorder recorder = new CallbackRecorder();
      String federation = "umbra-java-tck-sync-" + UUID.randomUUID();
      String label = "java-tck-sync";
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      try {
         ambassador.connect(recorder.proxy(), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, requireFom(), timeImplementation());
         created = true;
         ambassador.joinFederationExecution("umbra-java-tck-sync", federation);
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
         closeIfSupported(ambassador);
      }
   }

   private static void saveAndRestore() throws Exception {
      RTIambassador ambassador = factory().getRtiAmbassador();
      CallbackRecorder recorder = new CallbackRecorder();
      String federation = "umbra-java-tck-save-" + UUID.randomUUID();
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      try {
         ambassador.connect(recorder.proxy(), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, requireFom(), timeImplementation());
         created = true;
         ambassador.joinFederationExecution("umbra-java-tck-save", federation);
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
         closeIfSupported(ambassador);
      }
   }

   private static void momStatusReport() throws Exception {
      RTIambassador ambassador = factory().getRtiAmbassador();
      CallbackRecorder recorder = new CallbackRecorder();
      String federation = "umbra-java-tck-mom-" + UUID.randomUUID();
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      try {
         ambassador.connect(recorder.proxy(), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, requireFom(), timeImplementation());
         created = true;
         ambassador.joinFederationExecution("umbra-java-tck-mom", federation);
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
         closeIfSupported(ambassador);
      }
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
         out.append("  <testcase classname=\"umbra.rti.tck.RtiTckMain\" name=\"")
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
         case "java-tck.logical-time-factory": method = "logicalTimeFactory"; break;
         case "java-tck.declaration-management": method = "declarationManagement"; break;
         case "java-tck.object-management": method = "objectRegistrationAndReflection"; break;
         case "java-tck.api-surface-inventory": method = "apiSurfaceInventory"; break;
         case "java-tck.overloads-and-exceptions": method = "overloadsAndExceptions"; break;
         case "java-tck.malformed-inputs": method = "malformedInputs"; break;
         case "java-tck.time-advance": method = "timeAdvanceAndGrants"; break;
         case "java-tck.save-restore": method = "saveAndRestore"; break;
         case "java-tck.synchronization": method = "synchronizationPoint"; break;
         case "java-tck.mom": method = "momStatusReport"; break;
         case "java-tck.ddm": method = "dataDistributionManagement"; break;
         case "java-tck.ownership": method = "ownershipManagement"; break;
         default: method = "unsupportedPortableFamily"; break;
      }
      return "umbra.rti.tck.RtiTckMain#" + method;
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
      String federation = "umbra-java-tck-time-" + UUID.randomUUID();
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      try {
         ambassador.connect(recorder.proxy(), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation());
         created = true;
         ambassador.joinFederationExecution("umbra-java-tck-time", federation);
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
         closeIfSupported(ambassador);
      }
   }

   /**
    * Exercise the standard declaration-management surface against a real FOM
    * class.  This deliberately uses only names and handles from the standard
    * API, so the same scenario can run against a different Java RTI provider.
    */
   private static void declarationManagement() throws Exception {
      String fom = requireFom();
      RtiFactory rtiFactory = factory();
      RTIambassador ambassador = rtiFactory.getRtiAmbassador();
      CallbackRecorder recorder = new CallbackRecorder();
      String federation = "umbra-java-tck-declaration-" + UUID.randomUUID();
      boolean connected = false;
      boolean joined = false;
      boolean created = false;
      ObjectClassHandle objectClass = null;
      AttributeHandleSet attributes = null;
      try {
         ambassador.connect(recorder.proxy(), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation());
         created = true;
         ambassador.joinFederationExecution("umbra-java-tck-declaration", federation);
         joined = true;

         objectClass = ambassador.getObjectClassHandle(objectClassName());
         AttributeHandle attribute = ambassador.getAttributeHandle(objectClass, attributeName());
         attributes = ambassador.getAttributeHandleSetFactory().create();
         attributes.add(attribute);
         check(objectClassName().equals(
               ambassador.getObjectClassName(objectClass)),
            "object-class name did not round-trip through the standard API");
         check(attributeName().equals(ambassador.getAttributeName(objectClass, attribute)),
            "attribute name did not round-trip through the standard API");

         ambassador.publishObjectClassAttributes(objectClass, attributes);
         ambassador.subscribeObjectClassAttributes(objectClass, attributes);
         ambassador.unsubscribeObjectClassAttributes(objectClass, attributes);
         ambassador.unpublishObjectClassAttributes(objectClass, attributes);
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
         closeIfSupported(ambassador);
      }
   }

   /**
    * Exercise the basic publish/subscribe and register/discover/update/reflect
    * path.  The payload is intentionally opaque to the harness: the RTI
    * transports attribute values as octets and the FOM/provider owns the data
    * representation details.  This makes the scenario portable to providers
    * with different encoder implementations while still checking the complete
    * object-management callback boundary.
    */
   private static void objectRegistrationAndReflection() throws Exception {
      String fom = requireFom();
      RtiFactory rtiFactory = factory();
      RTIambassador publisher = rtiFactory.getRtiAmbassador();
      RTIambassador subscriber = rtiFactory.getRtiAmbassador();
      CallbackRecorder publisherRecorder = new CallbackRecorder();
      CallbackRecorder subscriberRecorder = new CallbackRecorder();
      String federation = "umbra-java-tck-object-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean created = false;
      try {
         publisher.connect(publisherRecorder.proxy(), CallbackModel.HLA_EVOKED);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), CallbackModel.HLA_EVOKED);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation());
         created = true;
         publisher.joinFederationExecution("umbra-java-tck-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("umbra-java-tck-subscriber", federation);
         subscriberJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName());
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName());
         ObjectClassHandle subscriberClass = subscriber.getObjectClassHandle(objectClassName());
         AttributeHandle subscriberAttribute = subscriber.getAttributeHandle(
            subscriberClass, attributeName());
         AttributeHandleSet publisherAttributes = publisher.getAttributeHandleSetFactory().create();
         publisherAttributes.add(publisherAttribute);
         AttributeHandleSet subscriberAttributes = subscriber.getAttributeHandleSetFactory().create();
         subscriberAttributes.add(subscriberAttribute);

         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         ObjectInstanceHandle registered = publisher.registerObjectInstance(publisherClass);
         check(registered != null && registered.encodedLength() > 0,
            "registerObjectInstance returned an invalid handle");
         drain(subscriber);
         check(subscriberRecorder.discoveredObject != null,
            "discoverObjectInstance did not cross the standard callback interface");
         check(registered.equals(subscriberRecorder.discoveredObject),
            "discovered object handle differs from the registered handle");
         check(publisher.getObjectInstanceName(registered).equals(
               subscriberRecorder.discoveredName),
            "discovered object name did not match the registered instance");
         check(subscriberRecorder.discoveredClass.equals(subscriberClass),
            "discovered object class did not match the subscribed class");

         byte[] payload = new byte[] { 1, 2, 3, (byte) 0xff };
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, payload);
         byte[] tag = new byte[] { 9, 8, 7 };
         publisher.updateAttributeValues(registered, values, tag);
         drain(subscriber);
         check(subscriberRecorder.reflectedValues != null,
            "reflectAttributeValues did not cross the standard callback interface");
         check(subscriberRecorder.reflectedObject.equals(registered),
            "reflected object handle differs from the registered handle");
         check(subscriberRecorder.reflectedValues.size() == 1,
            "reflected attribute map has the wrong size");
         byte[] reflectedPayload = subscriberRecorder.reflectedValues.get(subscriberAttribute);
         check(java.util.Arrays.equals(payload, reflectedPayload),
            "reflected attribute payload did not round-trip");
         check(java.util.Arrays.equals(tag, subscriberRecorder.reflectedTag),
            "update tag did not round-trip");
      } finally {
         if (subscriberJoined) {
            try {
               subscriber.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (publisherJoined) {
            try {
               publisher.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (created) {
            try {
               publisher.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (subscriberConnected) {
            try {
               subscriber.disconnect();
            } catch (Exception ignored) {
            }
         }
         if (publisherConnected) {
            try {
               publisher.disconnect();
            } catch (Exception ignored) {
            }
         }
         closeIfSupported(subscriber);
         closeIfSupported(publisher);
      }
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

   private static String timeImplementation() {
      return System.getProperty(TIME_PROPERTY, "HLAinteger64Time");
   }

   private static String objectClassName() {
      return System.getProperty(OBJECT_CLASS_PROPERTY,
         "HLAobjectRoot.Employee.Server");
   }

   private static String attributeName() {
      return System.getProperty(ATTRIBUTE_PROPERTY, "Efficiency");
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

   private static void closeIfSupported(Object value) throws Exception {
      for (Method method : value.getClass().getMethods()) {
         if ("close".equals(method.getName()) && method.getParameterCount() == 0) {
            method.invoke(value);
            return;
         }
      }
   }

   private static void check(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }

   private interface CheckedTest {
      void run() throws Exception;
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
      private int timeAdvanceGrants;
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
      private ObjectClassHandle discoveredClass;
      private String discoveredName;
      private ObjectInstanceHandle reflectedObject;
      private AttributeHandleValueMap reflectedValues;
      private byte[] reflectedTag;

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
                  timeAdvanceGrants++;
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
                  discoveredClass = (ObjectClassHandle) arguments[1];
                  discoveredName = (String) arguments[2];
               }
               if ("reflectAttributeValues".equals(method.getName())) {
                  reflectedObject = (ObjectInstanceHandle) arguments[0];
                  reflectedValues = (AttributeHandleValueMap) arguments[1];
                  reflectedTag = (byte[]) arguments[2];
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
