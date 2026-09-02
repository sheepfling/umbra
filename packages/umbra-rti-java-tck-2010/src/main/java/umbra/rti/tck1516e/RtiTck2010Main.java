package umbra.rti.tck1516e;

import hla.rti1516e.CallbackModel;
import hla.rti1516e.AttributeHandle;
import hla.rti1516e.AttributeHandleSet;
import hla.rti1516e.AttributeHandleValueMap;
import hla.rti1516e.AttributeRegionAssociation;
import hla.rti1516e.AttributeSetRegionSetPairList;
import hla.rti1516e.DimensionHandle;
import hla.rti1516e.DimensionHandleSet;
import hla.rti1516e.FederateHandle;
import hla.rti1516e.FederateAmbassador;
import hla.rti1516e.LogicalTime;
import hla.rti1516e.LogicalTimeFactory;
import hla.rti1516e.LogicalTimeInterval;
import hla.rti1516e.ObjectClassHandle;
import hla.rti1516e.ObjectInstanceHandle;
import hla.rti1516e.RangeBounds;
import hla.rti1516e.RegionHandle;
import hla.rti1516e.RegionHandleSet;
import hla.rti1516e.TransportationTypeHandle;
import hla.rti1516e.TransportationTypeHandleFactory;
import hla.rti1516e.RTIambassador;
import hla.rti1516e.RtiFactory;
import hla.rti1516e.RtiFactoryFactory;
import hla.rti1516e.ResignAction;
import hla.rti1516e.encoding.EncoderFactory;
import hla.rti1516e.encoding.DataElementFactory;
import hla.rti1516e.encoding.ByteWrapper;
import hla.rti1516e.encoding.HLAfixedRecord;
import hla.rti1516e.encoding.HLAinteger32BE;
import hla.rti1516e.encoding.HLAoctet;
import hla.rti1516e.encoding.HLAunicodeString;
import hla.rti1516e.encoding.HLAvariableArray;
import hla.rti1516e.exceptions.RTIexception;
import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.net.URL;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Properties;
import java.util.ServiceLoader;
import java.util.UUID;

/**
 * Provider-neutral IEEE 1516.1-2010 Java surface tests.
 *
 * <p>The source imports only {@code hla.rti1516e}.  A vendor or Umbra JNI
 * provider is selected by the standard {@code RtiFactoryFactory} service
 * registry at runtime.  Scenarios that need a FOM are explicit and skipped
 * when no FOM is configured; they never inherit a 2025 result.</p>
 */
public final class RtiTck2010Main {
   private static final String FACTORY = "umbra.rti.tck2010.factory";
   private static final String FOM = "umbra.rti.tck2010.fom";
   private static final String MIM = "umbra.rti.tck2010.mim";
   private static final String RESULTS = "umbra.rti.tck2010.results";
   private static final String PROFILE = "umbra.rti.tck2010.capabilityProfile";
   private static final List<Result> results = new ArrayList<>();
   private static Properties capabilityProfile;

   private RtiTck2010Main() {}

   public static void main(String[] args) throws Exception {
      run("java-2010-tck.factory-discovery", "lifecycle", RtiTck2010Main::factoryDiscovery);
      run("java-2010-tck.encoder-round-trip", "encoding", RtiTck2010Main::encoderRoundTrip);
      run("java-2010-tck.api-surface-inventory", "api-surface", RtiTck2010Main::apiSurfaceInventory);
      run("java-2010-tck.overloads-and-exceptions", "exceptions", RtiTck2010Main::overloadsAndExceptions);
      run("java-2010-tck.connect-disconnect", "lifecycle", RtiTck2010Main::connectDisconnect);
      run("java-2010-tck.federation-membership", "lifecycle", RtiTck2010Main::federationMembership);
      run("java-2010-tck.declaration-management", "declarations", RtiTck2010Main::declarationManagement);
      run("java-2010-tck.object-management", "object-management", RtiTck2010Main::objectManagement);
      run("java-2010-tck.interaction-management", "interactions", RtiTck2010Main::interactionManagement);
      run("java-2010-tck.synchronization-point", "synchronization", RtiTck2010Main::synchronizationPoint);
      run("java-2010-tck.ddm", "ddm", RtiTck2010Main::dataDistributionManagement);
      run("java-2010-tck.mom-service-reporting", "mom", RtiTck2010Main::momServiceReporting);
      run("java-2010-tck.mom-service-reporting-interlocks", "mom",
            RtiTck2010Main::momServiceReportingInterlocks);
      run("java-2010-tck.mom-federate-object", "mom", RtiTck2010Main::momFederateObject);
      run("java-2010-tck.mom-publication-report", "mom", RtiTck2010Main::momPublicationReport);
      run("java-2010-tck.mom-subscription-report", "mom", RtiTck2010Main::momSubscriptionReport);
      run("java-2010-tck.mom-interaction-publication-subscription-reports", "mom",
            RtiTck2010Main::momInteractionPublicationSubscriptionReports);
      run("java-2010-tck.mom-synchronization-reports", "mom",
            RtiTck2010Main::momSynchronizationReports);
      run("java-2010-tck.mom-exception-reporting", "mom", RtiTck2010Main::momExceptionReporting);
      run("java-2010-tck.mom-federation-object", "mom", RtiTck2010Main::momFederationObject);
      run("java-2010-tck.mom-mom-exception", "mom", RtiTck2010Main::momMomException);
      run("java-2010-tck.mom-timing", "mom", RtiTck2010Main::momTiming);
      run("java-2010-tck.mom-object-instance-information", "mom",
            RtiTck2010Main::momObjectInstanceInformation);
      run("java-2010-tck.mom-object-instances-can-be-deleted", "mom",
            RtiTck2010Main::momObjectInstancesCanBeDeleted);
      run("java-2010-tck.mom-object-instance-count-reports", "mom",
            RtiTck2010Main::momObjectInstanceCountReports);
      run("java-2010-tck.mom-transport-count-reports", "mom",
            RtiTck2010Main::momTransportCountReports);
      run("java-2010-tck.mom-interaction-count-reports", "mom",
            RtiTck2010Main::momInteractionCountReports);
      run("java-2010-tck.mom-fom-mim-data-reports", "mom",
            RtiTck2010Main::momFomMimDataReports);
      run("java-2010-tck.time-management", "time", RtiTck2010Main::timeAdvanceAndGrants);
      run("java-2010-tck.ownership-management", "ownership", RtiTck2010Main::ownershipManagement);
      run("java-2010-tck.save-restore", "save-restore", RtiTck2010Main::saveAndRestore);
      writeResults();
      long failed = results.stream().filter(result -> result.status.equals("fail")).count();
      long passed = results.stream().filter(result -> result.status.equals("pass")).count();
      long skipped = results.stream().filter(result -> result.status.equals("unsupported")).count();
      System.out.println("IEEE 1516e Java TCK: " + passed + " passed, " + failed
            + " failed, " + skipped + " unsupported");
      if (failed != 0) {
         throw new AssertionError("2010 Java TCK failed");
      }
   }

   private static void run(String id, String category, CheckedScenario scenario) {
      String mode = configuredMode(id);
      if ("unsupported".equals(mode) || "not applicable".equals(mode)) {
         String message = mode + " by capability profile; no provider assertion was made";
         results.add(new Result(id, category, mode, message));
         System.out.println(mode.toUpperCase(Locale.ROOT) + " " + id + " " + message);
         return;
      }
      try {
         scenario.run();
         results.add(new Result(id, category, "pass", ""));
         System.out.println("PASS " + id);
      } catch (UnsupportedScenario error) {
         results.add(new Result(id, category, "unsupported", error.getMessage()));
         System.out.println("UNSUPPORTED " + id + " " + error.getMessage());
      } catch (Throwable error) {
         results.add(new Result(id, category, "fail", error.toString()));
         System.err.println("FAIL " + id + " " + error);
      }
   }

   private static String configuredMode(String id) {
      Properties profile = capabilityProfile();
      String configured = profile.getProperty("scenario." + id);
      if (configured == null || configured.trim().isEmpty()) return "run";
      String normalized = configured.trim().toLowerCase(Locale.ROOT);
      if ("run".equals(normalized) || "pass".equals(normalized)) return "run";
      if ("unsupported".equals(normalized) || "skip".equals(normalized)) return "unsupported";
      if ("not_applicable".equals(normalized) || "not-applicable".equals(normalized)
            || "not applicable".equals(normalized)) return "not applicable";
      throw new IllegalArgumentException("unknown capability profile mode for " + id + ": " + configured);
   }

   private static Properties capabilityProfile() {
      if (capabilityProfile != null) return capabilityProfile;
      capabilityProfile = new Properties();
      String path = System.getProperty(PROFILE);
      if (path == null || path.trim().isEmpty()) return capabilityProfile;
      try (java.io.InputStream input = Files.newInputStream(Path.of(path))) {
         capabilityProfile.load(input);
      } catch (IOException error) {
         throw new IllegalArgumentException("cannot read capability profile " + path, error);
      }
      return capabilityProfile;
   }

   private static RtiFactory factory() throws RTIexception {
      String name = System.getProperty(FACTORY);
      RtiFactory result;
      try {
         result = name == null || name.isEmpty()
               ? RtiFactoryFactory.getRtiFactory()
               : RtiFactoryFactory.getRtiFactory(name);
      } catch (Throwable compatibilityFailure) {
         // The 2010 reference helper calls ImageIO's ServiceRegistry. Modern
         // JDKs reject an HLA RtiFactory as a non-ImageIO SPI category. Keep
         // the standard types and provider-neutral semantics, but use the
         // equivalent Java ServiceLoader path as an explicit compatibility
         // fallback and leave the issue visible in the result log.
         if (!isImageIoCategoryFailure(compatibilityFailure)) {
            if (compatibilityFailure instanceof RTIexception) throw (RTIexception) compatibilityFailure;
            if (compatibilityFailure instanceof RuntimeException) throw (RuntimeException) compatibilityFailure;
            throw new RuntimeException(compatibilityFailure);
         }
         ServiceLoader<RtiFactory> providers = ServiceLoader.load(RtiFactory.class);
         result = null;
         for (RtiFactory candidate : providers) {
            if (name == null || name.isEmpty() || candidate.rtiName().equals(name)) {
               result = candidate;
               break;
            }
         }
         if (result == null) throw compatibilityFailure;
         System.out.println("INFO java-2010-tck.factory-discovery compatibility=java.util.ServiceLoader");
      }
      check(result != null, "factory discovery returned null");
      check(result.rtiName() != null && !result.rtiName().isEmpty(), "factory name is empty");
      check(result.rtiVersion() != null && !result.rtiVersion().isEmpty(), "factory version is empty");
      return result;
   }

   private static boolean isImageIoCategoryFailure(Throwable error) {
      Throwable current = error;
      while (current != null) {
         if (current.toString().contains("not an ImageIO SPI class")) return true;
         current = current.getCause();
      }
      return false;
   }

   private static void factoryDiscovery() throws Exception {
      RtiFactory result = factory();
      check(RtiFactory.class.isAssignableFrom(result.getClass()), "factory is not standard RtiFactory");
      check(result.getEncoderFactory() != null, "factory returned no encoder");
      RTIambassador ambassador = result.getRtiAmbassador();
      check(ambassador != null, "factory returned no ambassador");
      check(ambassador.getFederateHandleFactory() != null, "missing FederateHandleFactory");
      check(ambassador.getObjectClassHandleFactory() != null, "missing ObjectClassHandleFactory");
      check(ambassador.getObjectInstanceHandleFactory() != null, "missing ObjectInstanceHandleFactory");
      check(ambassador.getAttributeHandleFactory() != null, "missing AttributeHandleFactory");
      check(ambassador.getInteractionClassHandleFactory() != null, "missing InteractionClassHandleFactory");
      check(ambassador.getParameterHandleFactory() != null, "missing ParameterHandleFactory");
      check(ambassador.getDimensionHandleFactory() != null, "missing DimensionHandleFactory");
      check(ambassador.getAttributeHandleSetFactory() != null, "missing AttributeHandleSetFactory");
      check(ambassador.getDimensionHandleSetFactory() != null, "missing DimensionHandleSetFactory");
      check(ambassador.getFederateHandleSetFactory() != null, "missing FederateHandleSetFactory");
      check(ambassador.getRegionHandleSetFactory() != null, "missing RegionHandleSetFactory");
      check(ambassador.getAttributeHandleValueMapFactory() != null, "missing AttributeHandleValueMapFactory");
      check(ambassador.getParameterHandleValueMapFactory() != null, "missing ParameterHandleValueMapFactory");
      check(ambassador.getAttributeSetRegionSetPairListFactory() != null,
            "missing AttributeSetRegionSetPairListFactory");
      TransportationTypeHandleFactory transportation = ambassador.getTransportationTypeHandleFactory();
      check(transportation != null, "missing TransportationTypeHandleFactory");
      check(transportation.getHLAdefaultReliable() != null, "missing default reliable transportation");
      check(transportation.getHLAdefaultBestEffort() != null, "missing default best-effort transportation");
      check(ambassador.getTimeFactory() != null && !ambassador.getTimeFactory().getName().isEmpty(),
            "missing logical-time factory name");
   }

   private static void encoderRoundTrip() throws Exception {
      EncoderFactory encoder = factory().getEncoderFactory();
      HLAinteger32BE value = encoder.createHLAinteger32BE(0x01020304);
      check(value.getValue() == 0x01020304, "integer value did not round-trip");
      byte[] encoded = value.toByteArray();
      check(encoded.length == 4, "integer encoding length is not four");
      check((encoded[0] & 0xff) == 1 && (encoded[3] & 0xff) == 4,
            "integer encoding is not big-endian");
      value.decode(new byte[] {4, 3, 2, 1});
      check(value.getValue() == 0x04030201, "integer decode did not round-trip");
      ByteWrapper wrapper = new ByteWrapper(new byte[] {0, 4, 3, 2, 1, 0}, 1, 4);
      value.decode(wrapper);
      check(value.getValue() == 0x04030201 && wrapper.getPos() == 5,
            "ByteWrapper decode did not preserve the provider cursor");
   }

   private static void apiSurfaceInventory() {
      check(RTIambassador.class.getPackageName().equals("hla.rti1516e"), "wrong RTI package");
      check(FederateAmbassador.class.getPackageName().equals("hla.rti1516e"), "wrong callback package");
      check(countAbstract(RTIambassador.class) == 172, "RTIambassador method count changed");
      check(countAbstract(FederateAmbassador.class) == 60, "FederateAmbassador method count changed");
      check(RtiFactory.class.getMethods().length >= 4, "RtiFactory surface is empty");
   }

   private static void overloadsAndExceptions() {
      check(overloads(RTIambassador.class, "connect") == 2, "connect overload count changed");
      check(overloads(RTIambassador.class, "createFederationExecution") == 5,
            "createFederationExecution overload count changed");
      check(overloads(RTIambassador.class, "joinFederationExecution") == 4,
            "joinFederationExecution overload count changed");
      check(RTIexception.class.getPackageName().equals("hla.rti1516e.exceptions"),
            "exception hierarchy escaped standard package");
   }

   private static void connectDisconnect() throws Exception {
      RtiFactory result = factory();
      RTIambassador ambassador = result.getRtiAmbassador();
      ambassador.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
      ambassador.disconnect();
   }

   private static void federationMembership() throws Exception {
      String fom = System.getProperty(FOM);
      if (fom == null || fom.isEmpty()) {
         throw new UnsupportedScenario("no 2010 FOM configured");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      RtiFactory result = factory();
      RTIambassador ambassador = result.getRtiAmbassador();
      String federation = "umbra-1516e-tck-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         ambassador.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, fomUrl);
         created = true;
         ambassador.joinFederationExecution("umbra-1516e-tck", federation);
         joined = true;
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (connected) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void declarationManagement() throws Exception {
      String fom = System.getProperty(FOM);
      if (fom == null || fom.isEmpty()) {
         throw new UnsupportedScenario("no 2010 FOM configured");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      RtiFactory result = factory();
      RTIambassador ambassador = result.getRtiAmbassador();
      String federation = "umbra-1516e-declaration-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         ambassador.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, fomUrl);
         created = true;
         ambassador.joinFederationExecution("java-2010-declaration", federation);
         joined = true;

         hla.rti1516e.ObjectClassHandle objectClass =
               ambassador.getObjectClassHandle("HLAobjectRoot.Employee.Server");
         AttributeHandle attribute = ambassador.getAttributeHandle(objectClass, "Efficiency");
         AttributeHandleSet attributes = ambassador.getAttributeHandleSetFactory().create();
         attributes.add(attribute);
         check("HLAobjectRoot.Employee.Server".equals(ambassador.getObjectClassName(objectClass)),
               "object-class name did not round-trip through the standard API");
         check("Efficiency".equals(ambassador.getAttributeName(objectClass, attribute)),
               "attribute name did not round-trip through the standard API");
         ambassador.publishObjectClassAttributes(objectClass, attributes);
         ambassador.subscribeObjectClassAttributes(objectClass, attributes);
         ambassador.unsubscribeObjectClassAttributes(objectClass, attributes);
         ambassador.unpublishObjectClassAttributes(objectClass, attributes);
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (connected) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void objectManagement() throws Exception {
      String fom = System.getProperty(FOM);
      if (fom == null || fom.isEmpty()) {
         throw new UnsupportedScenario("no 2010 FOM configured");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      final Object[] discovered = new Object[3];
      final Object[] reflected = new Object[3];
      FederateAmbassador subscriberCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void discoverObjectInstance(
               hla.rti1516e.ObjectInstanceHandle object,
               hla.rti1516e.ObjectClassHandle objectClass,
               String objectName) {
            discovered[0] = object;
            discovered[1] = objectClass;
            discovered[2] = objectName;
         }

         @Override public void reflectAttributeValues(
               hla.rti1516e.ObjectInstanceHandle object,
               hla.rti1516e.AttributeHandleValueMap attributes,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReflectInfo reflectInfo) {
            reflected[0] = object;
            reflected[1] = attributes;
            reflected[2] = tag;
         }
      };
      RtiFactory result = factory();
      RTIambassador publisher = result.getRtiAmbassador();
      RTIambassador subscriber = result.getRtiAmbassador();
      String federation = "umbra-1516e-object-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      try {
         publisher.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         publisherConnected = true;
         subscriber.connect(subscriberCallback, CallbackModel.HLA_EVOKED);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fomUrl);
         created = true;
         publisher.joinFederationExecution("java-2010-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-2010-subscriber", federation);
         subscriberJoined = true;

         hla.rti1516e.ObjectClassHandle publisherClass =
               publisher.getObjectClassHandle("HLAobjectRoot.Employee.Server");
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(publisherClass, "Efficiency");
         hla.rti1516e.ObjectClassHandle subscriberClass =
               subscriber.getObjectClassHandle("HLAobjectRoot.Employee.Server");
         AttributeHandle subscriberAttribute = subscriber.getAttributeHandle(subscriberClass, "Efficiency");
         AttributeHandleSet publisherAttributes = publisher.getAttributeHandleSetFactory().create();
         publisherAttributes.add(publisherAttribute);
         AttributeHandleSet subscriberAttributes = subscriber.getAttributeHandleSetFactory().create();
         subscriberAttributes.add(subscriberAttribute);
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscriberAttributes);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);

         hla.rti1516e.ObjectInstanceHandle registered = publisher.registerObjectInstance(publisherClass);
         check(registered != null && registered.encodedLength() > 0,
               "registerObjectInstance returned an invalid handle");
         check(discovered[0] != null, "discoverObjectInstance did not cross the standard callback");
         check(registered.equals(discovered[0]), "discovered object handle differed from registered handle");
         check(subscriberClass.equals(discovered[1]), "discovered object class differed from subscribed class");
         check(publisher.getObjectInstanceName(registered).equals(discovered[2]),
               "discovered object name did not match the registered instance");

         byte[] payload = new byte[] {1, 2, 3, (byte) 0xff};
         hla.rti1516e.AttributeHandleValueMap values =
               publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, payload);
         byte[] tag = new byte[] {9, 8, 7};
         publisher.updateAttributeValues(registered, values, tag);
         check(reflected[0] != null, "reflectAttributeValues did not cross the standard callback");
         check(registered.equals(reflected[0]), "reflected object handle differed from registered handle");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.AttributeHandle, byte[]> reflectedValues =
               (java.util.Map<hla.rti1516e.AttributeHandle, byte[]>) reflected[1];
         check(reflectedValues.size() == 1, "reflected attribute map has the wrong size");
         check(java.util.Arrays.equals(payload, reflectedValues.get(subscriberAttribute)),
               "reflected attribute payload did not round-trip");
         check(java.util.Arrays.equals(tag, (byte[]) reflected[2]), "update tag did not round-trip");
      } finally {
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (subscriberConnected) {
            try { subscriber.disconnect(); }
            catch (Exception ignored) {}
         }
         if (publisherConnected) {
            try { publisher.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void interactionManagement() throws Exception {
      String fom = System.getProperty(FOM);
      if (fom == null || fom.isEmpty()) {
         throw new UnsupportedScenario("no 2010 FOM configured");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      final Object[] received = new Object[5];
      FederateAmbassador subscriberCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            received[0] = interaction;
            received[1] = parameters;
            received[2] = tag;
            received[3] = sentOrdering;
            received[4] = transport;
         }
      };
      RtiFactory result = factory();
      RTIambassador publisher = result.getRtiAmbassador();
      RTIambassador subscriber = result.getRtiAmbassador();
      String federation = "umbra-1516e-interaction-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      try {
         publisher.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         publisherConnected = true;
         subscriber.connect(subscriberCallback, CallbackModel.HLA_EVOKED);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fomUrl);
         created = true;
         publisher.joinFederationExecution("java-2010-interaction-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-2010-interaction-subscriber", federation);
         subscriberJoined = true;

         hla.rti1516e.InteractionClassHandle publisherClass = publisher.getInteractionClassHandle(
               "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
         hla.rti1516e.InteractionClassHandle subscriberClass = subscriber.getInteractionClassHandle(
               "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
         check(publisherClass.equals(subscriberClass), "interaction-class handle identity did not round-trip");
         hla.rti1516e.ParameterHandle publisherParameter = publisher.getParameterHandle(
               publisherClass, "TemperatureOk");
         hla.rti1516e.ParameterHandle subscriberParameter = subscriber.getParameterHandle(
               subscriberClass, "TemperatureOk");
         check("TemperatureOk".equals(publisher.getParameterName(publisherClass, publisherParameter)),
               "parameter name did not round-trip");
         check(publisherParameter.equals(subscriberParameter), "parameter handle identity did not round-trip");

         publisher.publishInteractionClass(publisherClass);
         subscriber.subscribeInteractionClass(subscriberClass);
         hla.rti1516e.ParameterHandleValueMap values =
               publisher.getParameterHandleValueMapFactory().create(1);
         byte[] payload = new byte[] {5, 6, 7, (byte) 0xff};
         values.put(publisherParameter, payload);
         byte[] tag = new byte[] {10, 11, 12};
         publisher.sendInteraction(publisherClass, values, tag);
         check(received[0] != null, "receiveInteraction did not cross the standard callback");
         check(subscriberClass.equals(received[0]), "received interaction handle did not round-trip");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> receivedValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) received[1];
         check(receivedValues.size() == 1, "received parameter map has the wrong size");
         check(java.util.Arrays.equals(payload, receivedValues.get(subscriberParameter)),
               "received interaction payload did not round-trip");
         check(java.util.Arrays.equals(tag, (byte[]) received[2]), "interaction tag did not round-trip");
         check(received[3] == hla.rti1516e.OrderType.RECEIVE,
               "interaction receive order did not round-trip");
         check("transport:HLAdefaultReliable".equals(String.valueOf(received[4])),
               "interaction transportation handle did not round-trip");
         subscriber.unsubscribeInteractionClass(subscriberClass);
         publisher.unpublishInteractionClass(publisherClass);
      } finally {
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (subscriberConnected) {
            try { subscriber.disconnect(); }
            catch (Exception ignored) {}
         }
         if (publisherConnected) {
            try { publisher.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void synchronizationPoint() throws Exception {
      String fom = System.getProperty(FOM);
      if (fom == null || fom.isEmpty()) {
         throw new UnsupportedScenario("no 2010 FOM configured");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      final Object[] registration = new Object[1];
      final Object[] announcement = new Object[2];
      FederateAmbassador registrarCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void synchronizationPointRegistrationSucceeded(String label) {
            registration[0] = label;
         }
      };
      FederateAmbassador observerCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void announceSynchronizationPoint(String label, byte[] tag) {
            announcement[0] = label;
            announcement[1] = tag;
         }
      };
      RtiFactory result = factory();
      RTIambassador registrar = result.getRtiAmbassador();
      RTIambassador observer = result.getRtiAmbassador();
      String federation = "umbra-1516e-sync-" + UUID.randomUUID();
      boolean registrarConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean registrarJoined = false;
      boolean observerJoined = false;
      try {
         registrar.connect(registrarCallback, CallbackModel.HLA_EVOKED);
         registrarConnected = true;
         observer.connect(observerCallback, CallbackModel.HLA_EVOKED);
         observerConnected = true;
         registrar.createFederationExecution(federation, fomUrl);
         created = true;
         registrar.joinFederationExecution("java-2010-sync-registrar", federation);
         registrarJoined = true;
         observer.joinFederationExecution("java-2010-sync-observer", federation);
         observerJoined = true;

         String label = "java-2010-sync-point";
         byte[] tag = new byte[] {17, 18, 19};
         registrar.registerFederationSynchronizationPoint(label, tag);
         check(label.equals(registration[0]),
               "synchronizationPointRegistrationSucceeded did not cross the standard callback");
         check(label.equals(announcement[0]) && java.util.Arrays.equals(tag, (byte[]) announcement[1]),
               "announceSynchronizationPoint label/tag did not round-trip");
      } finally {
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (registrarJoined) {
            try { registrar.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { registrar.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (observerConnected) {
            try { observer.disconnect(); }
            catch (Exception ignored) {}
         }
         if (registrarConnected) {
            try { registrar.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void dataDistributionManagement() throws Exception {
      String fom = System.getProperty(FOM);
      if (fom == null || fom.isEmpty()) {
         throw new UnsupportedScenario("no 2010 FOM configured");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      RtiFactory result = factory();
      RTIambassador ambassador = result.getRtiAmbassador();
      String federation = "umbra-1516e-ddm-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      RegionHandle region = null;
      try {
         ambassador.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, fomUrl);
         created = true;
         ambassador.joinFederationExecution("java-2010-ddm", federation);
         joined = true;

         DimensionHandle flavorDimension = ambassador.getDimensionHandle("SodaFlavor");
         DimensionHandle quantityDimension = ambassador.getDimensionHandle("BarQuantity");
         check("SodaFlavor".equals(ambassador.getDimensionName(flavorDimension)),
               "SodaFlavor dimension name did not round-trip");
         check("BarQuantity".equals(ambassador.getDimensionName(quantityDimension)),
               "BarQuantity dimension name did not round-trip");
         check(ambassador.getDimensionUpperBound(flavorDimension) == 3L,
               "SodaFlavor upper bound did not match the 2010 FOM");
         check(ambassador.getDimensionUpperBound(quantityDimension) == 25L,
               "BarQuantity upper bound did not match the 2010 FOM");

         DimensionHandleSet dimensions = ambassador.getDimensionHandleSetFactory().create();
         dimensions.add(flavorDimension);
         dimensions.add(quantityDimension);
         region = ambassador.createRegion(dimensions);
         check(region != null && region.toString() != null && !region.toString().isEmpty(),
               "createRegion returned an invalid handle");
         DimensionHandleSet returnedDimensions = ambassador.getDimensionHandleSet(region);
         check(returnedDimensions != null && returnedDimensions.size() == 2
                     && returnedDimensions.contains(flavorDimension)
                     && returnedDimensions.contains(quantityDimension),
               "region dimension set did not round-trip");

         RangeBounds flavorBounds = new RangeBounds(0L, 3L);
         RangeBounds quantityBounds = new RangeBounds(5L, 20L);
         ambassador.setRangeBounds(region, flavorDimension, flavorBounds);
         ambassador.setRangeBounds(region, quantityDimension, quantityBounds);
         check(flavorBounds.equals(ambassador.getRangeBounds(region, flavorDimension)),
               "SodaFlavor range bounds did not round-trip");
         check(quantityBounds.equals(ambassador.getRangeBounds(region, quantityDimension)),
               "BarQuantity range bounds did not round-trip");

         RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
         regions.add(region);
         ambassador.commitRegionModifications(regions);

         hla.rti1516e.ObjectClassHandle sodaClass =
               ambassador.getObjectClassHandle("HLAobjectRoot.Drink.Soda");
         AttributeHandle flavor = ambassador.getAttributeHandle(sodaClass, "Flavor");
         AttributeHandleSet attributes = ambassador.getAttributeHandleSetFactory().create();
         attributes.add(flavor);
         AttributeSetRegionSetPairList pairs =
               ambassador.getAttributeSetRegionSetPairListFactory().create(1);
         pairs.add(new AttributeRegionAssociation(attributes, regions));
         ambassador.subscribeObjectClassAttributesWithRegions(sodaClass, pairs);
         ambassador.unsubscribeObjectClassAttributesWithRegions(sodaClass, pairs);
      } finally {
         if (region != null) {
            try { ambassador.deleteRegion(region); }
            catch (Exception ignored) {}
         }
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (connected) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void momServiceReporting() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final Object[] reported = new Object[6];
      FederateAmbassador observerCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            reported[0] = interaction;
            reported[1] = parameters;
            reported[2] = tag;
            reported[3] = sentOrdering;
            reported[4] = transport;
            reported[5] = receiveInfo;
         }
      };
      RtiFactory result = factory();
      RTIambassador controller = result.getRtiAmbassador();
      RTIambassador observer = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-" + UUID.randomUUID();
      boolean controllerConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean controllerJoined = false;
      boolean observerJoined = false;
      try {
         controller.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         controllerConnected = true;
         observer.connect(observerCallback, CallbackModel.HLA_EVOKED);
         observerConnected = true;
         controller.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle controllerHandle = controller.joinFederationExecution(
               "java-2010-mom-controller", federation);
         controllerJoined = true;
         observer.joinFederationExecution("java-2010-mom-observer", federation);
         observerJoined = true;

         String reportName =
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
         hla.rti1516e.InteractionClassHandle reportClass =
               observer.getInteractionClassHandle(reportName);
         check(reportName.equals(observer.getInteractionClassName(reportClass)),
               "MOM report interaction name did not round-trip");
         observer.subscribeInteractionClass(reportClass);

         hla.rti1516e.InteractionClassHandle setReporting = controller.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetServiceReporting");
         hla.rti1516e.ParameterHandle targetFederate = controller.getParameterHandle(
               setReporting, "HLAfederate");
         hla.rti1516e.ParameterHandle reportingState = controller.getParameterHandle(
               setReporting, "HLAreportingState");
         byte[] encodedController = new byte[controllerHandle.encodedLength()];
         controllerHandle.encode(encodedController, 0);
         hla.rti1516e.ParameterHandleValueMap switchValues =
               controller.getParameterHandleValueMapFactory().create(2);
         switchValues.put(targetFederate, encodedController);
         switchValues.put(reportingState, new byte[] {1});
         controller.sendInteraction(setReporting, switchValues, new byte[0]);

         // A normal standard service invocation must now be represented by a
         // MOM HLAreportServiceInvocation received by the other federate.
         controller.getObjectClassHandle("HLAobjectRoot.HLAmanager.HLAfederate");
         try { observer.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reported[0] != null && reportClass.equals(reported[0]),
               "MOM HLAreportServiceInvocation did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> reportValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reported[1];
         check(reportValues != null && reportValues.size() == 6,
               "MOM service report did not contain all six standard parameters");
         check(((byte[]) reported[2]).length == 0,
               "RTI-originated MOM report tag was not empty");
         check(reported[3] == hla.rti1516e.OrderType.RECEIVE,
               "MOM report receive order did not round-trip");
         check("transport:HLAdefaultReliable".equals(String.valueOf(reported[4])),
               "MOM report transportation handle did not round-trip");
         hla.rti1516e.ParameterHandle serialNumber = observer.getParameterHandle(
               reportClass, "HLAserialNumber");
         check(java.util.Arrays.equals(new byte[] {0, 0, 0, 0}, reportValues.get(serialNumber)),
               "MOM service report serial number did not start at zero");
         hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo =
               (hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo) reported[5];
         check(receiveInfo != null && receiveInfo.hasSentRegions()
               && receiveInfo.getSentRegions() != null
               && receiveInfo.getSentRegions().size() == 1,
               "MOM service report did not carry a single update region");

         controller.getObjectClassHandle("HLAobjectRoot.HLAmanager.HLAfederate");
         try { observer.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> secondReportValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reported[1];
         check(java.util.Arrays.equals(new byte[] {0, 0, 0, 1}, secondReportValues.get(serialNumber)),
               "MOM service report serial number did not increment per invocation");
         hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo secondReceiveInfo =
               (hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo) reported[5];
         check(secondReceiveInfo != null && secondReceiveInfo.hasSentRegions()
               && secondReceiveInfo.getSentRegions() != null
               && secondReceiveInfo.getSentRegions().size() == 1,
               "second MOM service report did not carry a single update region");
      } finally {
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (controllerJoined) {
            try { controller.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { controller.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (observerConnected) {
            try { observer.disconnect(); }
            catch (Exception ignored) {}
         }
         if (controllerConnected) {
            try { controller.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void momServiceReportingInterlocks() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<Object[]> reports = new ArrayList<>();
      FederateAmbassador observerCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            reports.add(new Object[] {interaction, parameters, tag, sentOrdering, transport});
         }
      };
      RtiFactory result = factory();
      RTIambassador controller = result.getRtiAmbassador();
      RTIambassador observer = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-interlocks-" + UUID.randomUUID();
      boolean controllerConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean controllerJoined = false;
      boolean observerJoined = false;
      try {
         controller.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         controllerConnected = true;
         observer.connect(observerCallback, CallbackModel.HLA_EVOKED);
         observerConnected = true;
         controller.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle controllerHandle = controller.joinFederationExecution(
               "java-2010-mom-interlock-controller", federation);
         controllerJoined = true;
         FederateHandle observerHandle = observer.joinFederationExecution(
               "java-2010-mom-interlock-observer", federation);
         observerJoined = true;

         hla.rti1516e.InteractionClassHandle serviceReport = observer.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportServiceInvocation");
         hla.rti1516e.InteractionClassHandle momException = observer.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportMOMexception");
         observer.subscribeInteractionClass(serviceReport);
         observer.subscribeInteractionClass(momException);

         hla.rti1516e.InteractionClassHandle setReporting = controller.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust."
               + "HLAsetServiceReporting");
         hla.rti1516e.ParameterHandle targetFederate = controller.getParameterHandle(
               setReporting, "HLAfederate");
         hla.rti1516e.ParameterHandle reportingState = controller.getParameterHandle(
               setReporting, "HLAreportingState");
         byte[] encodedController = new byte[controllerHandle.encodedLength()];
         controllerHandle.encode(encodedController, 0);
         byte[] encodedObserver = new byte[observerHandle.encodedLength()];
         observerHandle.encode(encodedObserver, 0);

         hla.rti1516e.ParameterHandleValueMap enableController =
               controller.getParameterHandleValueMapFactory().create(2);
         enableController.put(targetFederate, encodedController);
         enableController.put(reportingState, new byte[] {1});
         controller.sendInteraction(setReporting, enableController, new byte[0]);

         boolean rejected = false;
         try {
            controller.subscribeInteractionClass(serviceReport);
         } catch (hla.rti1516e.exceptions.FederateServiceInvocationsAreBeingReportedViaMOM expected) {
            rejected = true;
         }
         check(rejected,
               "a federate with service reporting enabled was allowed to subscribe to service reports");

         reports.clear();
         hla.rti1516e.ParameterHandleValueMap enableObserver =
               controller.getParameterHandleValueMapFactory().create(2);
         enableObserver.put(targetFederate, encodedObserver);
         enableObserver.put(reportingState, new byte[] {1});
         controller.sendInteraction(setReporting, enableObserver, new byte[0]);
         try { observer.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         Object[] momReport = reports.stream()
               .filter(report -> momException.equals(report[0]))
               .findFirst().orElse(null);
         check(momReport != null,
               "service-reporting subscription interlock did not produce HLAreportMOMexception");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> values =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) momReport[1];
         hla.rti1516e.ParameterHandle reportFederate = observer.getParameterHandle(
               momException, "HLAfederate");
         hla.rti1516e.ParameterHandle service = observer.getParameterHandle(
               momException, "HLAservice");
         hla.rti1516e.ParameterHandle exception = observer.getParameterHandle(
               momException, "HLAexception");
         hla.rti1516e.ParameterHandle parameterError = observer.getParameterHandle(
               momException, "HLAparameterError");
         check(values.size() == 4 && java.util.Arrays.equals(encodedObserver, values.get(reportFederate))
               && values.containsKey(service) && values.containsKey(exception)
               && values.containsKey(parameterError),
               "service-reporting interlock MOM exception parameters were incomplete");
         check(new String(values.get(service), StandardCharsets.UTF_8).equals(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetServiceReporting"),
               "service-reporting interlock service name was not fully qualified");
         check(values.get(exception) != null && values.get(exception).length > 0,
               "service-reporting interlock exception text was empty");
         check(java.util.Arrays.equals(new byte[] {0}, values.get(parameterError)),
               "service-reporting interlock incorrectly classified a precondition failure as a parameter error");
         check(((byte[]) momReport[2]).length == 0
               && momReport[3] == hla.rti1516e.OrderType.RECEIVE
               && "transport:HLAdefaultReliable".equals(String.valueOf(momReport[4])),
               "service-reporting interlock callback metadata did not round-trip");
      } finally {
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (controllerJoined) {
            try { controller.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { controller.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (observerConnected) {
            try { observer.disconnect(); }
            catch (Exception ignored) {}
         }
         if (controllerConnected) {
            try { controller.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void momFederateObject() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<ObjectInstanceHandle> discovered = new ArrayList<>();
      final List<java.util.Map<AttributeHandle, byte[]>> reflected = new ArrayList<>();
      FederateAmbassador observerCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void discoverObjectInstance(
               hla.rti1516e.ObjectInstanceHandle object,
               hla.rti1516e.ObjectClassHandle objectClass,
               String objectName) {
            discovered.add(object);
         }

         @Override public void reflectAttributeValues(
               hla.rti1516e.ObjectInstanceHandle object,
               hla.rti1516e.AttributeHandleValueMap attributes,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReflectInfo reflectInfo) {
            @SuppressWarnings("unchecked")
            java.util.Map<AttributeHandle, byte[]> values =
                  (java.util.Map<AttributeHandle, byte[]>) attributes;
            reflected.add(values);
         }
      };
      RtiFactory result = factory();
      RTIambassador registrar = result.getRtiAmbassador();
      RTIambassador observer = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-object-" + UUID.randomUUID();
      boolean registrarConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean registrarJoined = false;
      boolean observerJoined = false;
      try {
         registrar.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         registrarConnected = true;
         observer.connect(observerCallback, CallbackModel.HLA_EVOKED);
         observerConnected = true;
         registrar.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         registrar.joinFederationExecution("java-2010-mom-object-owner", federation);
         registrarJoined = true;
         observer.joinFederationExecution("java-2010-mom-object-observer", federation);
         observerJoined = true;

         hla.rti1516e.ObjectClassHandle momClass = observer.getObjectClassHandle(
               "HLAobjectRoot.HLAmanager.HLAfederate");
         AttributeHandle federateName = observer.getAttributeHandle(momClass, "HLAfederateName");
         AttributeHandleSet attributes = observer.getAttributeHandleSetFactory().create();
         attributes.add(federateName);
         observer.subscribeObjectClassAttributes(momClass, attributes);
         try { observer.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(discovered.size() >= 2,
               "MOM HLAfederate object discovery did not cover each joined federate");
         check(reflected.size() >= discovered.size(),
               "MOM HLAfederate static attributes were not reflected");
         boolean foundName = false;
         for (java.util.Map<AttributeHandle, byte[]> values : reflected) {
            if (values.containsKey(federateName)
                  && values.get(federateName) != null
                  && values.get(federateName).length > 0) {
               foundName = true;
               break;
            }
         }
         check(foundName, "MOM HLAfederateName attribute did not cross the callback interface");
      } finally {
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (registrarJoined) {
            try { registrar.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { registrar.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (observerConnected) {
            try { observer.disconnect(); }
            catch (Exception ignored) {}
         }
         if (registrarConnected) {
            try { registrar.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void momPublicationReport() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<Object[]> reports = new ArrayList<>();
      FederateAmbassador requesterCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            reports.add(new Object[] {interaction, parameters, tag, sentOrdering, transport});
         }
      };
      RtiFactory result = factory();
      RTIambassador requester = result.getRtiAmbassador();
      RTIambassador target = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-publication-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean targetConnected = false;
      boolean created = false;
      boolean requesterJoined = false;
      boolean targetJoined = false;
      try {
         requester.connect(requesterCallback, CallbackModel.HLA_EVOKED);
         requesterConnected = true;
         target.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         targetConnected = true;
         target.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle targetHandle = target.joinFederationExecution(
               "java-2010-mom-publication-target", federation);
         targetJoined = true;
         requester.joinFederationExecution("java-2010-mom-publication-requester", federation);
         requesterJoined = true;

         ObjectClassHandle objectClass = target.getObjectClassHandle(
               "HLAobjectRoot.Employee.Server");
         AttributeHandle attribute = target.getAttributeHandle(objectClass, "Efficiency");
         AttributeHandleSet attributes = target.getAttributeHandleSetFactory().create();
         attributes.add(attribute);
         target.publishObjectClassAttributes(objectClass, attributes);

         String reportName =
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportObjectClassPublication";
         hla.rti1516e.InteractionClassHandle reportClass =
               requester.getInteractionClassHandle(reportName);
         check(reportName.equals(requester.getInteractionClassName(reportClass)),
               "MOM publication report interaction name did not round-trip");
         requester.subscribeInteractionClass(reportClass);

         hla.rti1516e.InteractionClassHandle requestClass = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications");
         hla.rti1516e.ParameterHandle requestedFederate = requester.getParameterHandle(
               requestClass, "HLAfederate");
         byte[] encodedTarget = new byte[targetHandle.encodedLength()];
         targetHandle.encode(encodedTarget, 0);
         hla.rti1516e.ParameterHandleValueMap requestValues =
               requester.getParameterHandleValueMapFactory().create(1);
         requestValues.put(requestedFederate, encodedTarget);
         requester.sendInteraction(requestClass, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}

         check(reports.size() == 1 && reportClass.equals(reports.get(0)[0]),
               "MOM publication report did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> reportValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(0)[1];
         hla.rti1516e.ParameterHandle reportFederate = requester.getParameterHandle(
               reportClass, "HLAfederate");
         hla.rti1516e.ParameterHandle numberOfClasses = requester.getParameterHandle(
               reportClass, "HLAnumberOfClasses");
         hla.rti1516e.ParameterHandle reportedObjectClass = requester.getParameterHandle(
               reportClass, "HLAobjectClass");
         hla.rti1516e.ParameterHandle reportedAttributes = requester.getParameterHandle(
               reportClass, "HLAattributeList");
         check(reportValues.size() == 4 && reportValues.containsKey(reportFederate)
               && reportValues.containsKey(numberOfClasses)
               && reportValues.containsKey(reportedObjectClass)
               && reportValues.containsKey(reportedAttributes),
               "MOM publication report did not contain the standard parameters");
         check(java.util.Arrays.equals(encodedTarget, reportValues.get(reportFederate)),
               "MOM publication report targeted the wrong federate");
         check(ByteBuffer.wrap(reportValues.get(numberOfClasses)).getInt() == 1,
               "MOM publication report class count was not one");
         byte[] encodedObjectClass = new byte[objectClass.encodedLength()];
         objectClass.encode(encodedObjectClass, 0);
         check(java.util.Arrays.equals(encodedObjectClass, reportValues.get(reportedObjectClass)),
               "MOM publication report object class did not round-trip");
         check(reportValues.get(reportedAttributes) != null
               && reportValues.get(reportedAttributes).length > 0,
               "MOM publication report attribute list was empty");
         check(((byte[]) reports.get(0)[2]).length == 0,
               "MOM publication report tag was not empty");
         check(reports.get(0)[3] == hla.rti1516e.OrderType.RECEIVE,
               "MOM publication report receive order did not round-trip");
         check("transport:HLAdefaultReliable".equals(String.valueOf(reports.get(0)[4])),
               "MOM publication report transport did not round-trip");

         target.unpublishObjectClass(objectClass);
         requester.sendInteraction(requestClass, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 2 && reportClass.equals(reports.get(1)[0]),
               "MOM publication NULL response did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> nullValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(1)[1];
         check(nullValues.size() == 2 && ByteBuffer.wrap(nullValues.get(numberOfClasses)).getInt() == 0
               && !nullValues.containsKey(reportedObjectClass)
               && !nullValues.containsKey(reportedAttributes),
               "MOM publication NULL response did not omit class data");
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (targetJoined) {
            try { target.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { target.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (targetConnected) {
            try { target.disconnect(); }
            catch (Exception ignored) {}
         }
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void momSubscriptionReport() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<Object[]> reports = new ArrayList<>();
      FederateAmbassador requesterCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            reports.add(new Object[] {interaction, parameters, tag, sentOrdering, transport});
         }
      };
      RtiFactory result = factory();
      RTIambassador requester = result.getRtiAmbassador();
      RTIambassador target = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-subscription-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean targetConnected = false;
      boolean created = false;
      boolean requesterJoined = false;
      boolean targetJoined = false;
      try {
         requester.connect(requesterCallback, CallbackModel.HLA_EVOKED);
         requesterConnected = true;
         target.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         targetConnected = true;
         target.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle targetHandle = target.joinFederationExecution(
               "java-2010-mom-subscription-target", federation);
         targetJoined = true;
         requester.joinFederationExecution("java-2010-mom-subscription-requester", federation);
         requesterJoined = true;

         ObjectClassHandle objectClass = target.getObjectClassHandle(
               "HLAobjectRoot.Employee.Server");
         AttributeHandle attribute = target.getAttributeHandle(objectClass, "Efficiency");
         AttributeHandleSet attributes = target.getAttributeHandleSetFactory().create();
         attributes.add(attribute);
         target.subscribeObjectClassAttributes(objectClass, attributes);

         String reportName =
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportObjectClassSubscription";
         hla.rti1516e.InteractionClassHandle reportClass =
               requester.getInteractionClassHandle(reportName);
         check(reportName.equals(requester.getInteractionClassName(reportClass)),
               "MOM subscription report interaction name did not round-trip");
         requester.subscribeInteractionClass(reportClass);

         hla.rti1516e.InteractionClassHandle requestClass = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions");
         hla.rti1516e.ParameterHandle requestedFederate = requester.getParameterHandle(
               requestClass, "HLAfederate");
         byte[] encodedTarget = new byte[targetHandle.encodedLength()];
         targetHandle.encode(encodedTarget, 0);
         hla.rti1516e.ParameterHandleValueMap requestValues =
               requester.getParameterHandleValueMapFactory().create(1);
         requestValues.put(requestedFederate, encodedTarget);
         requester.sendInteraction(requestClass, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}

         check(reports.size() == 1 && reportClass.equals(reports.get(0)[0]),
               "MOM subscription report did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> reportValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(0)[1];
         hla.rti1516e.ParameterHandle reportFederate = requester.getParameterHandle(
               reportClass, "HLAfederate");
         hla.rti1516e.ParameterHandle numberOfClasses = requester.getParameterHandle(
               reportClass, "HLAnumberOfClasses");
         hla.rti1516e.ParameterHandle reportedObjectClass = requester.getParameterHandle(
               reportClass, "HLAobjectClass");
         hla.rti1516e.ParameterHandle active = requester.getParameterHandle(
               reportClass, "HLAactive");
         hla.rti1516e.ParameterHandle maxUpdateRate = requester.getParameterHandle(
               reportClass, "HLAmaxUpdateRate");
         hla.rti1516e.ParameterHandle reportedAttributes = requester.getParameterHandle(
               reportClass, "HLAattributeList");
         check(reportValues.size() == 6 && reportValues.containsKey(reportFederate)
               && reportValues.containsKey(numberOfClasses)
               && reportValues.containsKey(reportedObjectClass)
               && reportValues.containsKey(active)
               && reportValues.containsKey(maxUpdateRate)
               && reportValues.containsKey(reportedAttributes),
               "MOM subscription report did not contain the standard parameters");
         check(java.util.Arrays.equals(encodedTarget, reportValues.get(reportFederate)),
               "MOM subscription report targeted the wrong federate");
         check(ByteBuffer.wrap(reportValues.get(numberOfClasses)).getInt() == 1,
               "MOM subscription report class count was not one");
         byte[] encodedObjectClass = new byte[objectClass.encodedLength()];
         objectClass.encode(encodedObjectClass, 0);
         check(java.util.Arrays.equals(encodedObjectClass, reportValues.get(reportedObjectClass)),
               "MOM subscription report object class did not round-trip");
         check(java.util.Arrays.equals(new byte[] {1}, reportValues.get(active)),
               "MOM subscription report active flag was not true");
         check(reportValues.get(maxUpdateRate) != null && reportValues.get(maxUpdateRate).length > 0,
               "MOM subscription report maximum update rate was empty");
         check(reportValues.get(reportedAttributes) != null
               && reportValues.get(reportedAttributes).length > 0,
               "MOM subscription report attribute list was empty");
         check(((byte[]) reports.get(0)[2]).length == 0,
               "MOM subscription report tag was not empty");
         check(reports.get(0)[3] == hla.rti1516e.OrderType.RECEIVE,
               "MOM subscription report receive order did not round-trip");
         check("transport:HLAdefaultReliable".equals(String.valueOf(reports.get(0)[4])),
               "MOM subscription report transport did not round-trip");

         target.unsubscribeObjectClass(objectClass);
         requester.sendInteraction(requestClass, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 2 && reportClass.equals(reports.get(1)[0]),
               "MOM subscription NULL response did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> nullValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(1)[1];
         check(nullValues.size() == 2 && ByteBuffer.wrap(nullValues.get(numberOfClasses)).getInt() == 0
               && !nullValues.containsKey(reportedObjectClass)
               && !nullValues.containsKey(active)
               && !nullValues.containsKey(maxUpdateRate)
               && !nullValues.containsKey(reportedAttributes),
               "MOM subscription NULL response did not omit class data");
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (targetJoined) {
            try { target.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { target.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (targetConnected) {
            try { target.disconnect(); }
            catch (Exception ignored) {}
         }
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void momInteractionPublicationSubscriptionReports() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<Object[]> reports = new ArrayList<>();
      FederateAmbassador callback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            reports.add(new Object[] {interaction, parameters, tag, sentOrdering, transport});
         }
      };
      RtiFactory result = factory();
      RTIambassador requester = result.getRtiAmbassador();
      RTIambassador target = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-interaction-pub-sub-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean targetConnected = false;
      boolean created = false;
      boolean requesterJoined = false;
      boolean targetJoined = false;
      try {
         requester.connect(callback, CallbackModel.HLA_EVOKED);
         requesterConnected = true;
         target.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         targetConnected = true;
         target.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle targetHandle = target.joinFederationExecution(
               "java-2010-mom-interaction-pub-sub-target", federation);
         targetJoined = true;
         requester.joinFederationExecution("java-2010-mom-interaction-pub-sub-requester", federation);
         requesterJoined = true;

         hla.rti1516e.InteractionClassHandle targetInteraction = target.getInteractionClassHandle(
               "HLAinteractionRoot.CustomerTransactions.OrderTaken");
         hla.rti1516e.InteractionClassHandle requesterInteraction =
               requester.getInteractionClassHandle(
                     "HLAinteractionRoot.CustomerTransactions.OrderTaken");
         target.publishInteractionClass(targetInteraction);
         target.subscribeInteractionClass(targetInteraction);
         byte[] encodedInteraction = new byte[requesterInteraction.encodedLength()];
         requesterInteraction.encode(encodedInteraction, 0);
         byte[] encodedTarget = new byte[targetHandle.encodedLength()];
         targetHandle.encode(encodedTarget, 0);

         hla.rti1516e.InteractionClassHandle publicationReport = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportInteractionPublication");
         hla.rti1516e.InteractionClassHandle subscriptionReport = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportInteractionSubscription");
         requester.subscribeInteractionClass(publicationReport);
         requester.subscribeInteractionClass(subscriptionReport);
         hla.rti1516e.InteractionClassHandle publicationRequest = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications");
         hla.rti1516e.InteractionClassHandle subscriptionRequest = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions");
         hla.rti1516e.ParameterHandle publicationFederate = requester.getParameterHandle(
               publicationRequest, "HLAfederate");
         hla.rti1516e.ParameterHandle subscriptionFederate = requester.getParameterHandle(
               subscriptionRequest, "HLAfederate");
         hla.rti1516e.ParameterHandle publicationReportFederate = requester.getParameterHandle(
               publicationReport, "HLAfederate");
         hla.rti1516e.ParameterHandle publicationList = requester.getParameterHandle(
               publicationReport, "HLAinteractionClassList");
         hla.rti1516e.ParameterHandle subscriptionReportFederate = requester.getParameterHandle(
               subscriptionReport, "HLAfederate");
         hla.rti1516e.ParameterHandle subscriptionList = requester.getParameterHandle(
               subscriptionReport, "HLAinteractionClassList");

         hla.rti1516e.ParameterHandleValueMap values =
               requester.getParameterHandleValueMapFactory().create(1);
         values.put(publicationFederate, encodedTarget);
         requester.sendInteraction(publicationRequest, values, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 1 && publicationReport.equals(reports.get(0)[0]),
               "MOM interaction publication report did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> publicationValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(0)[1];
         check(publicationValues.size() == 2
               && java.util.Arrays.equals(encodedTarget, publicationValues.get(publicationReportFederate))
               && containsBytes(publicationValues.get(publicationList), encodedInteraction),
               "MOM interaction publication report payload was wrong");
         check(((byte[]) reports.get(0)[2]).length == 0
               && reports.get(0)[3] == hla.rti1516e.OrderType.RECEIVE
               && "transport:HLAdefaultReliable".equals(String.valueOf(reports.get(0)[4])),
               "MOM interaction publication report metadata was wrong");

         values = requester.getParameterHandleValueMapFactory().create(1);
         values.put(subscriptionFederate, encodedTarget);
         requester.sendInteraction(subscriptionRequest, values, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 2 && subscriptionReport.equals(reports.get(1)[0]),
               "MOM interaction subscription report did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> subscriptionValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(1)[1];
         check(subscriptionValues.size() == 2
               && java.util.Arrays.equals(encodedTarget, subscriptionValues.get(subscriptionReportFederate))
               && containsBytes(subscriptionValues.get(subscriptionList), encodedInteraction),
               "MOM interaction subscription report payload was wrong");

         target.unpublishInteractionClass(targetInteraction);
         target.unsubscribeInteractionClass(targetInteraction);
         requester.sendInteraction(publicationRequest, values, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 3 && publicationReport.equals(reports.get(2)[0]),
               "MOM interaction publication NULL report did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> publicationNull =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(2)[1];
         check(publicationNull.size() == 2
               && publicationNull.get(publicationList).length == 0,
               "MOM interaction publication NULL payload was not empty");

         requester.sendInteraction(subscriptionRequest, values, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 4 && subscriptionReport.equals(reports.get(3)[0]),
               "MOM interaction subscription NULL report did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> subscriptionNull =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(3)[1];
         check(subscriptionNull.size() == 2
               && subscriptionNull.get(subscriptionList).length == 0,
               "MOM interaction subscription NULL payload was not empty");
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (targetJoined) {
            try { target.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { target.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (targetConnected) {
            try { target.disconnect(); }
            catch (Exception ignored) {}
         }
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void momSynchronizationReports() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<Object[]> reports = new ArrayList<>();
      FederateAmbassador observerCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            reports.add(new Object[] {interaction, parameters, tag, sentOrdering, transport});
         }
      };
      RtiFactory result = factory();
      RTIambassador subject = result.getRtiAmbassador();
      RTIambassador observer = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-sync-reports-" + UUID.randomUUID();
      boolean subjectConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean subjectJoined = false;
      boolean observerJoined = false;
      try {
         subject.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         subjectConnected = true;
         observer.connect(observerCallback, CallbackModel.HLA_EVOKED);
         observerConnected = true;
         subject.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle subjectHandle = subject.joinFederationExecution(
               "java-2010-mom-sync-subject", federation);
         subjectJoined = true;
         FederateHandle observerHandle = observer.joinFederationExecution(
               "java-2010-mom-sync-observer", federation);
         observerJoined = true;

         String label = "java-2010-mom-sync-report";
         subject.registerFederationSynchronizationPoint(label, new byte[0]);

         hla.rti1516e.InteractionClassHandle pointsRequest = subject.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest."
               + "HLArequestSynchronizationPoints");
         hla.rti1516e.InteractionClassHandle pointsReport = observer.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport."
               + "HLAreportSynchronizationPoints");
         hla.rti1516e.ParameterHandle points = observer.getParameterHandle(
               pointsReport, "HLAsyncPoints");
         hla.rti1516e.InteractionClassHandle statusRequest = subject.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest."
               + "HLArequestSynchronizationPointStatus");
         hla.rti1516e.ParameterHandle statusRequestName = subject.getParameterHandle(
               statusRequest, "HLAsyncPointName");
         hla.rti1516e.InteractionClassHandle statusReport = observer.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport."
               + "HLAreportSynchronizationPointStatus");
         hla.rti1516e.ParameterHandle statusReportName = observer.getParameterHandle(
               statusReport, "HLAsyncPointName");
         hla.rti1516e.ParameterHandle statusReportFederates = observer.getParameterHandle(
               statusReport, "HLAsyncPointFederates");
         observer.subscribeInteractionClass(pointsReport);
         observer.subscribeInteractionClass(statusReport);
         EncoderFactory encoder = result.getEncoderFactory();

         subject.sendInteraction(pointsRequest,
               subject.getParameterHandleValueMapFactory().create(0), new byte[0]);
         check(reports.size() == 1 && pointsReport.equals(reports.get(0)[0]),
               "MOM synchronization-points report did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> pointValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(0)[1];
         check(pointValues.size() == 1 && pointValues.containsKey(points),
               "MOM synchronization-points report parameters were wrong");
         check(decodeSynchronizationPointLabels(encoder, pointValues.get(points)).contains(label),
               "MOM synchronization-points report did not contain the active label");
         check(((byte[]) reports.get(0)[2]).length == 0
               && reports.get(0)[3] == hla.rti1516e.OrderType.RECEIVE
               && "transport:HLAdefaultReliable".equals(String.valueOf(reports.get(0)[4])),
               "MOM synchronization-points report metadata was wrong");

         hla.rti1516e.ParameterHandleValueMap statusValues =
               subject.getParameterHandleValueMapFactory().create(1);
         statusValues.put(statusRequestName,
               encoder.createHLAunicodeString(label).toByteArray());
         subject.sendInteraction(statusRequest, statusValues, new byte[0]);
         check(reports.size() == 2 && statusReport.equals(reports.get(1)[0]),
               "MOM synchronization status report did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> beforeValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(1)[1];
         check(label.equals(decodeSynchronizationPointName(encoder, beforeValues.get(statusReportName))),
               "MOM synchronization status label did not round-trip");
         java.util.Map<FederateHandle, Integer> before = decodeSynchronizationStatuses(
               observer, encoder, beforeValues.get(statusReportFederates));
         check(before.get(subjectHandle) != null && before.get(subjectHandle) == 2
               && before.get(observerHandle) != null && before.get(observerHandle) == 2,
               "MOM synchronization status did not report both federates moving");

         subject.synchronizationPointAchieved(label);
         statusValues = subject.getParameterHandleValueMapFactory().create(1);
         statusValues.put(statusRequestName,
               encoder.createHLAunicodeString(label).toByteArray());
         subject.sendInteraction(statusRequest, statusValues, new byte[0]);
         check(reports.size() == 3 && statusReport.equals(reports.get(2)[0]),
               "MOM synchronization post-achievement report was missing");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> afterSubjectValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(2)[1];
         java.util.Map<FederateHandle, Integer> afterSubject = decodeSynchronizationStatuses(
               observer, encoder, afterSubjectValues.get(statusReportFederates));
         check(afterSubject.get(subjectHandle) != null && afterSubject.get(subjectHandle) == 3
               && afterSubject.get(observerHandle) != null && afterSubject.get(observerHandle) == 2,
               "MOM synchronization status did not preserve per-federate achievement");

         statusValues = subject.getParameterHandleValueMapFactory().create(1);
         statusValues.put(statusRequestName,
               encoder.createHLAunicodeString("missing-java-2010-sync").toByteArray());
         subject.sendInteraction(statusRequest, statusValues, new byte[0]);
         check(reports.size() == 4 && statusReport.equals(reports.get(3)[0]),
               "MOM missing synchronization status report was missing");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> missingValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(3)[1];
         check(decodeSynchronizationStatuses(observer, encoder,
               missingValues.get(statusReportFederates)).isEmpty(),
               "MOM missing synchronization status was not an empty list");

         observer.synchronizationPointAchieved(label);
         reports.clear();
         subject.sendInteraction(pointsRequest,
               subject.getParameterHandleValueMapFactory().create(0), new byte[0]);
         check(reports.size() == 1 && pointsReport.equals(reports.get(0)[0]),
               "MOM completed synchronization-points report was missing");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> completedValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(0)[1];
         check(decodeSynchronizationPointLabels(encoder, completedValues.get(points)).isEmpty(),
               "MOM completed synchronization point remained active");
      } finally {
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (subjectJoined) {
            try { subject.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { subject.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (observerConnected) {
            try { observer.disconnect(); }
            catch (Exception ignored) {}
         }
         if (subjectConnected) {
            try { subject.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static String decodeSynchronizationPointName(EncoderFactory encoder, byte[] encoded)
         throws Exception {
      HLAunicodeString value = encoder.createHLAunicodeString();
      value.decode(encoded == null ? new byte[0] : encoded);
      return value.getValue();
   }

   private static List<String> decodeSynchronizationPointLabels(
         EncoderFactory encoder, byte[] encoded) throws Exception {
      DataElementFactory<HLAunicodeString> factory = index -> encoder.createHLAunicodeString();
      HLAvariableArray<HLAunicodeString> values = encoder.createHLAvariableArray(factory);
      values.decode(encoded == null ? new byte[0] : encoded);
      List<String> result = new ArrayList<>();
      for (HLAunicodeString value : values) result.add(value.getValue());
      return result;
   }

   private static java.util.Map<FederateHandle, Integer> decodeSynchronizationStatuses(
         RTIambassador observer, EncoderFactory encoder, byte[] encoded) throws Exception {
      DataElementFactory<HLAoctet> octetFactory = index -> encoder.createHLAoctet();
      DataElementFactory<HLAfixedRecord> recordFactory = index -> {
         HLAfixedRecord record = encoder.createHLAfixedRecord();
         record.add(encoder.createHLAvariableArray(octetFactory));
         record.add(encoder.createHLAinteger32BE());
         return record;
      };
      HLAvariableArray<HLAfixedRecord> values = encoder.createHLAvariableArray(recordFactory);
      values.decode(encoded == null ? new byte[0] : encoded);
      java.util.Map<FederateHandle, Integer> result = new java.util.LinkedHashMap<>();
      for (HLAfixedRecord record : values) {
         @SuppressWarnings("unchecked")
         HLAvariableArray<HLAoctet> encodedFederate =
               (HLAvariableArray<HLAoctet>) record.get(0);
         byte[] federateBytes = new byte[encodedFederate.size()];
         for (int index = 0; index < federateBytes.length; index++) {
            federateBytes[index] = (byte) encodedFederate.get(index).getValue();
         }
         FederateHandle federate = observer.getFederateHandleFactory().decode(federateBytes, 0);
         HLAinteger32BE status = (HLAinteger32BE) record.get(1);
         result.put(federate, status.getValue());
      }
      return result;
   }

   private static void momExceptionReporting() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<Object[]> reports = new ArrayList<>();
      FederateAmbassador requesterCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            reports.add(new Object[] {interaction, parameters, tag, sentOrdering, transport});
         }
      };
      RtiFactory result = factory();
      RTIambassador requester = result.getRtiAmbassador();
      RTIambassador target = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-exception-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean targetConnected = false;
      boolean created = false;
      boolean requesterJoined = false;
      boolean targetJoined = false;
      try {
         requester.connect(requesterCallback, CallbackModel.HLA_EVOKED);
         requesterConnected = true;
         target.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         targetConnected = true;
         target.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle targetHandle = target.joinFederationExecution(
               "java-2010-mom-exception-target", federation);
         targetJoined = true;
         requester.joinFederationExecution("java-2010-mom-exception-requester", federation);
         requesterJoined = true;

         hla.rti1516e.InteractionClassHandle reportClass = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportException");
         requester.subscribeInteractionClass(reportClass);
         ObjectClassHandle objectClass = target.getObjectClassHandle(
               "HLAobjectRoot.Employee.Server");
         hla.rti1516e.ObjectInstanceHandle object = target.registerObjectInstance(objectClass);

         // The switch is disabled by default; an invalid second delete must not
         // produce a report before the standard enable interaction is received.
         target.deleteObjectInstance(object, new byte[0]);
         target.deleteObjectInstance(object, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.isEmpty(), "exception report was emitted while reporting was disabled");

         hla.rti1516e.InteractionClassHandle setExceptionReporting = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetExceptionReporting");
         hla.rti1516e.ParameterHandle requestedFederate = requester.getParameterHandle(
               setExceptionReporting, "HLAfederate");
         hla.rti1516e.ParameterHandle reportingState = requester.getParameterHandle(
               setExceptionReporting, "HLAreportingState");
         byte[] encodedTarget = new byte[targetHandle.encodedLength()];
         targetHandle.encode(encodedTarget, 0);
         hla.rti1516e.ParameterHandleValueMap switchValues =
               requester.getParameterHandleValueMapFactory().create(2);
         switchValues.put(requestedFederate, encodedTarget);
         switchValues.put(reportingState, new byte[] {1});
         requester.sendInteraction(setExceptionReporting, switchValues, new byte[0]);

         target.deleteObjectInstance(object, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 1 && reportClass.equals(reports.get(0)[0]),
               "HLAreportException did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> values =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(0)[1];
         hla.rti1516e.ParameterHandle reportFederate = requester.getParameterHandle(
               reportClass, "HLAfederate");
         hla.rti1516e.ParameterHandle service = requester.getParameterHandle(reportClass, "HLAservice");
         hla.rti1516e.ParameterHandle exception = requester.getParameterHandle(
               reportClass, "HLAexception");
         check(values.size() == 3 && values.containsKey(reportFederate)
               && values.containsKey(service) && values.containsKey(exception),
               "HLAreportException did not contain the standard parameters");
         check(java.util.Arrays.equals(encodedTarget, values.get(reportFederate)),
               "HLAreportException targeted the wrong federate");
         check(new String(values.get(service), StandardCharsets.UTF_8)
               .equals("RTIambassador.deleteObjectInstance"),
               "HLAreportException service name did not identify the failing service");
         check(values.get(exception) != null && values.get(exception).length > 0,
               "HLAreportException exception text was empty");
         check(((byte[]) reports.get(0)[2]).length == 0,
               "HLAreportException tag was not empty");
         check(reports.get(0)[3] == hla.rti1516e.OrderType.RECEIVE,
               "HLAreportException receive order did not round-trip");
         check("transport:HLAdefaultReliable".equals(String.valueOf(reports.get(0)[4])),
               "HLAreportException transport did not round-trip");
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (targetJoined) {
            try { target.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { target.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (targetConnected) {
            try { target.disconnect(); }
            catch (Exception ignored) {}
         }
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void momFederationObject() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<ObjectInstanceHandle> discovered = new ArrayList<>();
      final List<java.util.Map<AttributeHandle, byte[]>> reflected = new ArrayList<>();
      final List<Object[]> metadata = new ArrayList<>();
      FederateAmbassador observerCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void discoverObjectInstance(
               hla.rti1516e.ObjectInstanceHandle object,
               hla.rti1516e.ObjectClassHandle objectClass,
               String objectName) {
            discovered.add(object);
         }

         @Override public void reflectAttributeValues(
               hla.rti1516e.ObjectInstanceHandle object,
               hla.rti1516e.AttributeHandleValueMap attributes,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReflectInfo receiveInfo) {
            @SuppressWarnings("unchecked")
            java.util.Map<AttributeHandle, byte[]> values =
                  (java.util.Map<AttributeHandle, byte[]>) attributes;
            reflected.add(values);
            metadata.add(new Object[] {tag, sentOrdering, transport});
         }
      };
      RtiFactory result = factory();
      RTIambassador registrar = result.getRtiAmbassador();
      RTIambassador observer = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-federation-" + UUID.randomUUID();
      boolean registrarConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean registrarJoined = false;
      boolean observerJoined = false;
      try {
         registrar.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         registrarConnected = true;
         observer.connect(observerCallback, CallbackModel.HLA_EVOKED);
         observerConnected = true;
         registrar.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         registrar.joinFederationExecution("java-2010-mom-federation-owner", federation);
         registrarJoined = true;
         observer.joinFederationExecution("java-2010-mom-federation-observer", federation);
         observerJoined = true;

         ObjectClassHandle federationClass = observer.getObjectClassHandle(
               "HLAobjectRoot.HLAmanager.HLAfederation");
         AttributeHandle federationName = observer.getAttributeHandle(
               federationClass, "HLAfederationName");
         AttributeHandleSet attributes = observer.getAttributeHandleSetFactory().create();
         attributes.add(federationName);
         observer.subscribeObjectClassAttributes(federationClass, attributes);
         try { observer.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(discovered.size() == 1,
               "MOM HLAfederation object discovery did not return one federation object");
         check(reflected.size() == 1 && reflected.get(0).containsKey(federationName),
               "MOM HLAfederation static attributes were not reflected");
         check(federation.equals(new String(reflected.get(0).get(federationName), StandardCharsets.UTF_8)),
               "MOM HLAfederationName did not identify the federation");
         check(((byte[]) metadata.get(0)[0]).length == 0,
               "MOM HLAfederation reflection tag was not empty");
         check(metadata.get(0)[1] == hla.rti1516e.OrderType.RECEIVE,
               "MOM HLAfederation reflection order did not round-trip");
         check("transport:HLAdefaultReliable".equals(String.valueOf(metadata.get(0)[2])),
               "MOM HLAfederation reflection transport did not round-trip");
      } finally {
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (registrarJoined) {
            try { registrar.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { registrar.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (observerConnected) {
            try { observer.disconnect(); }
            catch (Exception ignored) {}
         }
         if (registrarConnected) {
            try { registrar.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void momMomException() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<Object[]> reports = new ArrayList<>();
      FederateAmbassador requesterCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            reports.add(new Object[] {interaction, parameters, tag, sentOrdering, transport});
         }
      };
      RtiFactory result = factory();
      RTIambassador requester = result.getRtiAmbassador();
      RTIambassador target = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-mom-exception-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean targetConnected = false;
      boolean created = false;
      boolean requesterJoined = false;
      boolean targetJoined = false;
      try {
         requester.connect(requesterCallback, CallbackModel.HLA_EVOKED);
         requesterConnected = true;
         target.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         targetConnected = true;
         target.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle targetHandle = target.joinFederationExecution(
               "java-2010-mom-mom-exception-target", federation);
         targetJoined = true;
         requester.joinFederationExecution("java-2010-mom-mom-exception-requester", federation);
         requesterJoined = true;

         String reportName =
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception";
         hla.rti1516e.InteractionClassHandle reportClass =
               requester.getInteractionClassHandle(reportName);
         requester.subscribeInteractionClass(reportClass);

         hla.rti1516e.InteractionClassHandle malformed = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
         hla.rti1516e.ParameterHandle requestedFederate = requester.getParameterHandle(
               malformed, "HLAfederate");
         byte[] encodedTarget = new byte[targetHandle.encodedLength()];
         targetHandle.encode(encodedTarget, 0);
         hla.rti1516e.ParameterHandleValueMap malformedValues =
               requester.getParameterHandleValueMapFactory().create(1);
         malformedValues.put(requestedFederate, encodedTarget);
         // Omit HLAreportPeriod deliberately: the standard MOM exception report
         // identifies a malformed service interaction, without throwing it back
         // through the ordinary RTI exception channel.
         requester.sendInteraction(malformed, malformedValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 1 && reportClass.equals(reports.get(0)[0]),
               "HLAreportMOMexception did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> values =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(0)[1];
         hla.rti1516e.ParameterHandle reportFederate = requester.getParameterHandle(
               reportClass, "HLAfederate");
         hla.rti1516e.ParameterHandle service = requester.getParameterHandle(reportClass, "HLAservice");
         hla.rti1516e.ParameterHandle exception = requester.getParameterHandle(
               reportClass, "HLAexception");
         hla.rti1516e.ParameterHandle parameterError = requester.getParameterHandle(
               reportClass, "HLAparameterError");
         check(values.size() == 4 && values.containsKey(reportFederate)
               && values.containsKey(service) && values.containsKey(exception)
               && values.containsKey(parameterError),
               "HLAreportMOMexception did not contain the standard parameters");
         check(java.util.Arrays.equals(encodedTarget, values.get(reportFederate)),
               "HLAreportMOMexception targeted the wrong federate");
         check(new String(values.get(service), StandardCharsets.UTF_8).equals(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming"),
               "HLAreportMOMexception service name was not fully qualified");
         check(values.get(exception) != null && values.get(exception).length > 0,
               "HLAreportMOMexception exception text was empty");
         check(java.util.Arrays.equals(new byte[] {1}, values.get(parameterError)),
               "HLAreportMOMexception parameter-error flag was not true");
         check(((byte[]) reports.get(0)[2]).length == 0,
               "HLAreportMOMexception tag was not empty");
         check(reports.get(0)[3] == hla.rti1516e.OrderType.RECEIVE,
               "HLAreportMOMexception receive order did not round-trip");
         check("transport:HLAdefaultReliable".equals(String.valueOf(reports.get(0)[4])),
               "HLAreportMOMexception transport did not round-trip");

         // The same standard MOM service is valid when its HLAreportPeriod
         // parameter is present and a non-negative HLAinteger32BE value.
         // A valid control interaction must not be reported as a MOM error.
         hla.rti1516e.ParameterHandle reportPeriod = requester.getParameterHandle(
               malformed, "HLAreportPeriod");
         malformedValues.put(reportPeriod,
               java.nio.ByteBuffer.allocate(4).order(java.nio.ByteOrder.BIG_ENDIAN).putInt(5).array());
         requester.sendInteraction(malformed, malformedValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 1,
               "valid HLAsetTiming was incorrectly reported through HLAreportMOMexception");
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (targetJoined) {
            try { target.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { target.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (targetConnected) {
            try { target.disconnect(); }
            catch (Exception ignored) {}
         }
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void momTiming() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<AttributeHandleValueMap> evokedReflections = new ArrayList<>();
      final List<AttributeHandleValueMap> immediateReflections = new ArrayList<>();
      FederateAmbassador evokedCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void reflectAttributeValues(
               hla.rti1516e.ObjectInstanceHandle object,
               hla.rti1516e.AttributeHandleValueMap attributes,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReflectInfo reflectInfo) {
            evokedReflections.add(attributes);
         }
      };
      FederateAmbassador immediateCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void reflectAttributeValues(
               hla.rti1516e.ObjectInstanceHandle object,
               hla.rti1516e.AttributeHandleValueMap attributes,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReflectInfo reflectInfo) {
            immediateReflections.add(attributes);
         }
      };
      RtiFactory result = factory();
      RTIambassador target = result.getRtiAmbassador();
      RTIambassador evoked = result.getRtiAmbassador();
      RTIambassador immediate = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-timing-" + UUID.randomUUID();
      boolean targetConnected = false;
      boolean evokedConnected = false;
      boolean immediateConnected = false;
      boolean created = false;
      boolean targetJoined = false;
      boolean evokedJoined = false;
      boolean immediateJoined = false;
      try {
         target.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         targetConnected = true;
         evoked.connect(evokedCallback, CallbackModel.HLA_EVOKED);
         evokedConnected = true;
         immediate.connect(immediateCallback, CallbackModel.HLA_IMMEDIATE);
         immediateConnected = true;
         target.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle targetHandle = target.joinFederationExecution(
               "java-2010-mom-timing-target", federation);
         targetJoined = true;
         evoked.joinFederationExecution("java-2010-mom-timing-evoked", federation);
         evokedJoined = true;
         immediate.joinFederationExecution("java-2010-mom-timing-immediate", federation);
         immediateJoined = true;

         String className = "HLAobjectRoot.HLAmanager.HLAfederate";
         ObjectClassHandle evokedClass = evoked.getObjectClassHandle(className);
         ObjectClassHandle immediateClass = immediate.getObjectClassHandle(className);
         AttributeHandle evokedLogicalTime = evoked.getAttributeHandle(evokedClass, "HLAlogicalTime");
         AttributeHandle evokedLookahead = evoked.getAttributeHandle(evokedClass, "HLAlookahead");
         AttributeHandle immediateLogicalTime = immediate.getAttributeHandle(
               immediateClass, "HLAlogicalTime");
         AttributeHandle immediateLookahead = immediate.getAttributeHandle(
               immediateClass, "HLAlookahead");
         AttributeHandleSet evokedAttributes = evoked.getAttributeHandleSetFactory().create();
         evokedAttributes.add(evokedLogicalTime);
         evokedAttributes.add(evokedLookahead);
         AttributeHandleSet immediateAttributes = immediate.getAttributeHandleSetFactory().create();
         immediateAttributes.add(immediateLogicalTime);
         immediateAttributes.add(immediateLookahead);
         evoked.subscribeObjectClassAttributes(evokedClass, evokedAttributes);
         immediate.subscribeObjectClassAttributes(immediateClass, immediateAttributes);
         // The subscription itself causes the bounded initial discovery route;
         // only later values belong to this periodic assertion.
         evokedReflections.clear();
         immediateReflections.clear();

         target.modifyLookahead(target.getTimeFactory().decodeInterval(
               ByteBuffer.allocate(8).order(java.nio.ByteOrder.BIG_ENDIAN).putLong(2L).array(), 0));
         hla.rti1516e.InteractionClassHandle setTiming = target.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
         hla.rti1516e.ParameterHandle federateParameter = target.getParameterHandle(
               setTiming, "HLAfederate");
         hla.rti1516e.ParameterHandle periodParameter = target.getParameterHandle(
               setTiming, "HLAreportPeriod");
         hla.rti1516e.ParameterHandleValueMap timing =
               target.getParameterHandleValueMapFactory().create(2);
         byte[] encodedTarget = new byte[targetHandle.encodedLength()];
         targetHandle.encode(encodedTarget, 0);
         timing.put(federateParameter, encodedTarget);
         timing.put(periodParameter,
               ByteBuffer.allocate(4).order(java.nio.ByteOrder.BIG_ENDIAN).putInt(1).array());
         target.sendInteraction(setTiming, timing, new byte[0]);

         Thread.sleep(1250L);
         check(evokedReflections.isEmpty(),
               "HLA_EVOKED periodic MOM reflection bypassed the callback boundary");
         check(!immediateReflections.isEmpty(),
               "HLA_IMMEDIATE periodic MOM reflection did not arrive automatically");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.AttributeHandle, byte[]> immediateValues =
               (java.util.Map<hla.rti1516e.AttributeHandle, byte[]>) immediateReflections.get(0);
         check(immediateValues.containsKey(immediateLogicalTime)
               && immediateValues.containsKey(immediateLookahead),
               "periodic MOM reflection omitted HLAlogicalTime/HLAlookahead");
         check(ByteBuffer.wrap(immediateValues.get(immediateLogicalTime))
                     .order(java.nio.ByteOrder.BIG_ENDIAN).getLong() == 0L,
               "periodic HLAlogicalTime did not use the target snapshot");
         check(ByteBuffer.wrap(immediateValues.get(immediateLookahead))
                     .order(java.nio.ByteOrder.BIG_ENDIAN).getLong() == 2L,
               "periodic HLAlookahead did not use the target snapshot");

         evoked.evokeMultipleCallbacks(0.0, 0.25);
         check(!evokedReflections.isEmpty(),
               "HLA_EVOKED periodic MOM reflection did not arrive at Evoke");
         evokedReflections.clear();
         immediateReflections.clear();
         timing.put(periodParameter,
               ByteBuffer.allocate(4).order(java.nio.ByteOrder.BIG_ENDIAN).putInt(0).array());
         target.sendInteraction(setTiming, timing, new byte[0]);
         Thread.sleep(1250L);
         evoked.evokeMultipleCallbacks(0.0, 0.25);
         check(evokedReflections.isEmpty() && immediateReflections.isEmpty(),
               "HLAreportPeriod=0 did not disable periodic MOM reflection");
      } finally {
         if (immediateJoined) {
            try { immediate.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (evokedJoined) {
            try { evoked.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (targetJoined) {
            try { target.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { target.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (immediateConnected) {
            try { immediate.disconnect(); }
            catch (Exception ignored) {}
         }
         if (evokedConnected) {
            try { evoked.disconnect(); }
            catch (Exception ignored) {}
         }
         if (targetConnected) {
            try { target.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void momObjectInstanceInformation() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<Object[]> reports = new ArrayList<>();
      FederateAmbassador requesterCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            reports.add(new Object[] {interaction, parameters, tag, sentOrdering, transport});
         }
      };
      RtiFactory result = factory();
      RTIambassador requester = result.getRtiAmbassador();
      RTIambassador target = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-object-information-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean targetConnected = false;
      boolean created = false;
      boolean requesterJoined = false;
      boolean targetJoined = false;
      try {
         requester.connect(requesterCallback, CallbackModel.HLA_EVOKED);
         requesterConnected = true;
         target.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         targetConnected = true;
         target.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle targetHandle = target.joinFederationExecution(
               "java-2010-mom-object-information-target", federation);
         targetJoined = true;
         requester.joinFederationExecution(
               "java-2010-mom-object-information-requester", federation);
         requesterJoined = true;

         ObjectClassHandle objectClass = target.getObjectClassHandle(
               "HLAobjectRoot.Employee.Server");
         AttributeHandle attribute = target.getAttributeHandle(objectClass, "Efficiency");
         AttributeHandleSet attributes = target.getAttributeHandleSetFactory().create();
         attributes.add(attribute);
         target.publishObjectClassAttributes(objectClass, attributes);
         ObjectInstanceHandle object = target.registerObjectInstance(
               objectClass, "java-2010-mom-object-information-instance");

         String reportName =
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportObjectInstanceInformation";
         hla.rti1516e.InteractionClassHandle reportClass =
               requester.getInteractionClassHandle(reportName);
         requester.subscribeInteractionClass(reportClass);
         hla.rti1516e.InteractionClassHandle requestClass = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
               + "HLArequestObjectInstanceInformation");
         hla.rti1516e.ParameterHandle requestedFederate = requester.getParameterHandle(
               requestClass, "HLAfederate");
         hla.rti1516e.ParameterHandle requestedObject = requester.getParameterHandle(
               requestClass, "HLAobjectInstance");
         byte[] encodedObject = new byte[object.encodedLength()];
         object.encode(encodedObject, 0);
         byte[] encodedTarget = new byte[targetHandle.encodedLength()];
         targetHandle.encode(encodedTarget, 0);
         hla.rti1516e.ParameterHandleValueMap requestValues =
               requester.getParameterHandleValueMapFactory().create(2);
         requestValues.put(requestedFederate, encodedTarget);
         requestValues.put(requestedObject, encodedObject);
         requester.sendInteraction(requestClass, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}

         check(reports.size() == 1 && reportClass.equals(reports.get(0)[0]),
               "MOM object-information report did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> values =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(0)[1];
         hla.rti1516e.ParameterHandle reportFederate = requester.getParameterHandle(
               reportClass, "HLAfederate");
         hla.rti1516e.ParameterHandle reportObject = requester.getParameterHandle(
               reportClass, "HLAobjectInstance");
         hla.rti1516e.ParameterHandle ownedAttributes = requester.getParameterHandle(
               reportClass, "HLAownedInstanceAttributeList");
         hla.rti1516e.ParameterHandle registeredClass = requester.getParameterHandle(
               reportClass, "HLAregisteredClass");
         hla.rti1516e.ParameterHandle knownClass = requester.getParameterHandle(
               reportClass, "HLAknownClass");
         check(values.size() == 5 && values.containsKey(reportFederate)
               && values.containsKey(reportObject) && values.containsKey(ownedAttributes)
               && values.containsKey(registeredClass) && values.containsKey(knownClass),
               "MOM object-information report did not contain the standard parameters");
         check(java.util.Arrays.equals(encodedTarget, values.get(reportFederate)),
               "MOM object-information report targeted the wrong federate");
         check(java.util.Arrays.equals(encodedObject, values.get(reportObject)),
               "MOM object-information object handle did not round-trip");
         byte[] encodedAttribute = new byte[attribute.encodedLength()];
         attribute.encode(encodedAttribute, 0);
         check(java.util.Arrays.equals(encodedAttribute, values.get(ownedAttributes)),
               "MOM object-information owned attribute list did not round-trip");
         byte[] encodedClass = new byte[objectClass.encodedLength()];
         objectClass.encode(encodedClass, 0);
         check(java.util.Arrays.equals(encodedClass, values.get(registeredClass))
               && java.util.Arrays.equals(encodedClass, values.get(knownClass)),
               "MOM object-information class handles did not round-trip");
         check(((byte[]) reports.get(0)[2]).length == 0,
               "MOM object-information report tag was not empty");
         check(reports.get(0)[3] == hla.rti1516e.OrderType.RECEIVE,
               "MOM object-information report receive order did not round-trip");
         check("transport:HLAdefaultReliable".equals(String.valueOf(reports.get(0)[4])),
               "MOM object-information report transport did not round-trip");

         ObjectInstanceHandle unknown = requester.getObjectInstanceHandle(
               "java-2010-mom-object-information-unknown");
         byte[] encodedUnknown = new byte[unknown.encodedLength()];
         unknown.encode(encodedUnknown, 0);
         requestValues.clear();
         requestValues.put(requestedFederate, encodedTarget);
         requestValues.put(requestedObject, encodedUnknown);
         requester.sendInteraction(requestClass, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 2 && reportClass.equals(reports.get(1)[0]),
               "MOM object-information NULL report did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> nullValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(1)[1];
         check(nullValues.size() == 3
               && java.util.Arrays.equals(encodedTarget, nullValues.get(reportFederate))
               && java.util.Arrays.equals(encodedUnknown, nullValues.get(reportObject))
               && nullValues.containsKey(ownedAttributes)
               && nullValues.get(ownedAttributes).length == 0
               && !nullValues.containsKey(registeredClass)
               && !nullValues.containsKey(knownClass),
               "MOM object-information NULL response did not omit class data");
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (targetJoined) {
            try { target.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { target.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (targetConnected) {
            try { target.disconnect(); }
            catch (Exception ignored) {}
         }
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void momObjectInstancesCanBeDeleted() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<Object[]> reports = new ArrayList<>();
      FederateAmbassador requesterCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            reports.add(new Object[] {interaction, parameters, tag, sentOrdering, transport});
         }
      };
      RtiFactory result = factory();
      RTIambassador requester = result.getRtiAmbassador();
      RTIambassador target = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-deletable-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean targetConnected = false;
      boolean created = false;
      boolean requesterJoined = false;
      boolean targetJoined = false;
      try {
         requester.connect(requesterCallback, CallbackModel.HLA_EVOKED);
         requesterConnected = true;
         target.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         targetConnected = true;
         target.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle targetHandle = target.joinFederationExecution(
               "java-2010-mom-deletable-target", federation);
         targetJoined = true;
         requester.joinFederationExecution("java-2010-mom-deletable-requester", federation);
         requesterJoined = true;

         ObjectClassHandle objectClass = target.getObjectClassHandle(
               "HLAobjectRoot.Employee.Server");
         AttributeHandle attribute = target.getAttributeHandle(objectClass, "Efficiency");
         AttributeHandleSet attributes = target.getAttributeHandleSetFactory().create();
         attributes.add(attribute);
         target.publishObjectClassAttributes(objectClass, attributes);
         ObjectInstanceHandle object = target.registerObjectInstance(
               objectClass, "java-2010-mom-deletable-instance");

         String reportName =
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportObjectInstancesThatCanBeDeleted";
         hla.rti1516e.InteractionClassHandle reportClass =
               requester.getInteractionClassHandle(reportName);
         requester.subscribeInteractionClass(reportClass);
         hla.rti1516e.InteractionClassHandle requestClass = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
               + "HLArequestObjectInstancesThatCanBeDeleted");
         hla.rti1516e.ParameterHandle requestedFederate = requester.getParameterHandle(
               requestClass, "HLAfederate");
         byte[] encodedTarget = new byte[targetHandle.encodedLength()];
         targetHandle.encode(encodedTarget, 0);
         hla.rti1516e.ParameterHandleValueMap requestValues =
               requester.getParameterHandleValueMapFactory().create(1);
         requestValues.put(requestedFederate, encodedTarget);
         requester.sendInteraction(requestClass, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}

         check(reports.size() == 1 && reportClass.equals(reports.get(0)[0]),
               "MOM deletable-object report did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> values =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(0)[1];
         hla.rti1516e.ParameterHandle reportFederate = requester.getParameterHandle(
               reportClass, "HLAfederate");
         hla.rti1516e.ParameterHandle objectCounts = requester.getParameterHandle(
               reportClass, "HLAobjectInstanceCounts");
         check(values.size() == 2 && values.containsKey(reportFederate)
               && values.containsKey(objectCounts),
               "MOM deletable-object report did not contain the standard parameters");
         check(java.util.Arrays.equals(encodedTarget, values.get(reportFederate)),
               "MOM deletable-object report targeted the wrong federate");
         byte[] encodedClass = new byte[objectClass.encodedLength()];
         objectClass.encode(encodedClass, 0);
         check(containsBytes(values.get(objectCounts), encodedClass)
               && containsBytes(values.get(objectCounts), new byte[] {0, 0, 0, 1}),
               "MOM deletable-object class/count payload did not round-trip");
         check(((byte[]) reports.get(0)[2]).length == 0,
               "MOM deletable-object report tag was not empty");
         check(reports.get(0)[3] == hla.rti1516e.OrderType.RECEIVE,
               "MOM deletable-object report receive order did not round-trip");
         check("transport:HLAdefaultReliable".equals(String.valueOf(reports.get(0)[4])),
               "MOM deletable-object report transport did not round-trip");

         target.deleteObjectInstance(object, new byte[0]);
         requester.sendInteraction(requestClass, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 2 && reportClass.equals(reports.get(1)[0]),
               "MOM deletable-object NULL report did not cross the callback interface");
         @SuppressWarnings("unchecked")
         java.util.Map<hla.rti1516e.ParameterHandle, byte[]> nullValues =
               (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) reports.get(1)[1];
         check(nullValues.size() == 2
               && java.util.Arrays.equals(encodedTarget, nullValues.get(reportFederate))
               && nullValues.containsKey(objectCounts)
               && nullValues.get(objectCounts).length == 0,
               "MOM deletable-object NULL response did not empty the class counts");
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (targetJoined) {
            try { target.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { target.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (targetConnected) {
            try { target.disconnect(); }
            catch (Exception ignored) {}
         }
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void momObjectInstanceCountReports() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<Object[]> reports = new ArrayList<>();
      FederateAmbassador requesterCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            reports.add(new Object[] {interaction, parameters, tag, sentOrdering, transport});
         }
      };
      RtiFactory result = factory();
      RTIambassador requester = result.getRtiAmbassador();
      RTIambassador target = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-counts-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean targetConnected = false;
      boolean created = false;
      boolean requesterJoined = false;
      boolean targetJoined = false;
      try {
         requester.connect(requesterCallback, CallbackModel.HLA_EVOKED);
         requesterConnected = true;
         target.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         targetConnected = true;
         target.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle targetHandle = target.joinFederationExecution(
               "java-2010-mom-counts-target", federation);
         targetJoined = true;
         FederateHandle requesterHandle = requester.joinFederationExecution(
               "java-2010-mom-counts-requester", federation);
         requesterJoined = true;

         ObjectClassHandle targetClass = target.getObjectClassHandle(
               "HLAobjectRoot.Employee.Server");
         AttributeHandle targetAttribute = target.getAttributeHandle(targetClass, "Efficiency");
         ObjectClassHandle requesterClass = requester.getObjectClassHandle(
               "HLAobjectRoot.Employee.Server");
         AttributeHandle requesterAttribute = requester.getAttributeHandle(
               requesterClass, "Efficiency");
         AttributeHandleSet subscribedAttributes = requester.getAttributeHandleSetFactory().create();
         subscribedAttributes.add(requesterAttribute);
         requester.subscribeObjectClassAttributes(requesterClass, subscribedAttributes);
         AttributeHandleSet publishedAttributes = target.getAttributeHandleSetFactory().create();
         publishedAttributes.add(targetAttribute);
         target.publishObjectClassAttributes(targetClass, publishedAttributes);
         ObjectInstanceHandle object = target.registerObjectInstance(
               targetClass, "java-2010-mom-counts-instance");
         AttributeHandleValueMap updateValues =
               target.getAttributeHandleValueMapFactory().create(1);
         updateValues.put(targetAttribute, new byte[] {1, 2, 3});
         target.updateAttributeValues(object, updateValues, new byte[0]);

         hla.rti1516e.InteractionClassHandle updatedReport = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportObjectInstancesUpdated");
         hla.rti1516e.InteractionClassHandle reflectedReport = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport."
               + "HLAreportObjectInstancesReflected");
         requester.subscribeInteractionClass(updatedReport);
         requester.subscribeInteractionClass(reflectedReport);
         hla.rti1516e.InteractionClassHandle updatedRequest = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
               + "HLArequestObjectInstancesUpdated");
         hla.rti1516e.InteractionClassHandle reflectedRequest = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest."
               + "HLArequestObjectInstancesReflected");
         hla.rti1516e.ParameterHandle updatedRequestFederate = requester.getParameterHandle(
               updatedRequest, "HLAfederate");
         hla.rti1516e.ParameterHandle reflectedRequestFederate = requester.getParameterHandle(
               reflectedRequest, "HLAfederate");
         hla.rti1516e.ParameterHandle updatedReportFederate = requester.getParameterHandle(
               updatedReport, "HLAfederate");
         hla.rti1516e.ParameterHandle updatedCounts = requester.getParameterHandle(
               updatedReport, "HLAobjectInstanceCounts");
         hla.rti1516e.ParameterHandle reflectedReportFederate = requester.getParameterHandle(
               reflectedReport, "HLAfederate");
         hla.rti1516e.ParameterHandle reflectedCounts = requester.getParameterHandle(
               reflectedReport, "HLAobjectInstanceCounts");
         byte[] encodedTarget = new byte[targetHandle.encodedLength()];
         targetHandle.encode(encodedTarget, 0);
         byte[] encodedRequester = new byte[requesterHandle.encodedLength()];
         requesterHandle.encode(encodedRequester, 0);
         byte[] encodedClass = new byte[targetClass.encodedLength()];
         targetClass.encode(encodedClass, 0);

         hla.rti1516e.ParameterHandleValueMap requestValues =
               requester.getParameterHandleValueMapFactory().create(1);
         requestValues.put(updatedRequestFederate, encodedTarget);
         requester.sendInteraction(updatedRequest, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 1, "MOM updated-object report count was wrong");
         checkMomObjectInstanceCountReport(reports.get(0), updatedReport,
               updatedReportFederate, updatedCounts, encodedTarget, encodedClass, true,
               "MOM updated-object");

         requestValues.clear();
         requestValues.put(reflectedRequestFederate, encodedRequester);
         requester.sendInteraction(reflectedRequest, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 2, "MOM reflected-object report count was wrong");
         checkMomObjectInstanceCountReport(reports.get(1), reflectedReport,
               reflectedReportFederate, reflectedCounts, encodedRequester, encodedClass, true,
               "MOM reflected-object");

         requestValues.clear();
         requestValues.put(updatedRequestFederate, encodedRequester);
         requester.sendInteraction(updatedRequest, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 3, "MOM updated-object NULL report count was wrong");
         checkMomObjectInstanceCountReport(reports.get(2), updatedReport,
               updatedReportFederate, updatedCounts, encodedRequester, encodedClass, false,
               "MOM updated-object NULL");

         requestValues.clear();
         requestValues.put(reflectedRequestFederate, encodedTarget);
         requester.sendInteraction(reflectedRequest, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 4, "MOM reflected-object NULL report count was wrong");
         checkMomObjectInstanceCountReport(reports.get(3), reflectedReport,
               reflectedReportFederate, reflectedCounts, encodedTarget, encodedClass, false,
               "MOM reflected-object NULL");
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (targetJoined) {
            try { target.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { target.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (targetConnected) {
            try { target.disconnect(); }
            catch (Exception ignored) {}
         }
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void checkMomObjectInstanceCountReport(Object[] report,
         hla.rti1516e.InteractionClassHandle reportClass,
         hla.rti1516e.ParameterHandle reportFederate,
         hla.rti1516e.ParameterHandle objectCounts,
         byte[] expectedFederate,
         byte[] encodedClass,
         boolean positive,
         String label) {
      check(reportClass.equals(report[0]), label + " report class did not round-trip");
      @SuppressWarnings("unchecked")
      java.util.Map<hla.rti1516e.ParameterHandle, byte[]> values =
            (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) report[1];
      check(values.size() == 2 && values.containsKey(reportFederate)
            && values.containsKey(objectCounts), label + " report parameters were wrong");
      check(java.util.Arrays.equals(expectedFederate, values.get(reportFederate)),
            label + " report targeted the wrong federate");
      if (positive) {
         check(containsBytes(values.get(objectCounts), encodedClass)
               && containsBytes(values.get(objectCounts), new byte[] {0, 0, 0, 1}),
               label + " class/count payload did not round-trip");
      } else {
         check(values.get(objectCounts).length == 0,
               label + " NULL response did not use an empty count list");
      }
      check(((byte[]) report[2]).length == 0, label + " report tag was not empty");
      check(report[3] == hla.rti1516e.OrderType.RECEIVE,
            label + " report receive order did not round-trip");
      check("transport:HLAdefaultReliable".equals(String.valueOf(report[4])),
            label + " report transport did not round-trip");
   }

   private static void momTransportCountReports() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<Object[]> reports = new ArrayList<>();
      FederateAmbassador requesterCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            reports.add(new Object[] {interaction, parameters, tag, sentOrdering, transport});
         }
      };
      RtiFactory result = factory();
      RTIambassador requester = result.getRtiAmbassador();
      RTIambassador target = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-transport-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean targetConnected = false;
      boolean created = false;
      boolean requesterJoined = false;
      boolean targetJoined = false;
      try {
         requester.connect(requesterCallback, CallbackModel.HLA_EVOKED);
         requesterConnected = true;
         target.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         targetConnected = true;
         target.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle targetHandle = target.joinFederationExecution(
               "java-2010-mom-transport-target", federation);
         targetJoined = true;
         FederateHandle requesterHandle = requester.joinFederationExecution(
               "java-2010-mom-transport-requester", federation);
         requesterJoined = true;

         ObjectClassHandle serverClass = target.getObjectClassHandle(
               "HLAobjectRoot.Employee.Server");
         AttributeHandle serverAttribute = target.getAttributeHandle(serverClass, "Efficiency");
         ObjectClassHandle sodaClass = target.getObjectClassHandle(
               "HLAobjectRoot.Food.Drink.Soda");
         AttributeHandle sodaAttribute = target.getAttributeHandle(sodaClass, "Flavor");
         ObjectClassHandle requesterServerClass = requester.getObjectClassHandle(
               "HLAobjectRoot.Employee.Server");
         ObjectClassHandle requesterSodaClass = requester.getObjectClassHandle(
               "HLAobjectRoot.Food.Drink.Soda");
         AttributeHandle requesterServerAttribute = requester.getAttributeHandle(
               requesterServerClass, "Efficiency");
         AttributeHandle requesterSodaAttribute = requester.getAttributeHandle(
               requesterSodaClass, "Flavor");
         AttributeHandleSet subscribedServerAttributes =
               requester.getAttributeHandleSetFactory().create();
         subscribedServerAttributes.add(requesterServerAttribute);
         requester.subscribeObjectClassAttributes(requesterServerClass,
               subscribedServerAttributes);
         AttributeHandleSet subscribedSodaAttributes =
               requester.getAttributeHandleSetFactory().create();
         subscribedSodaAttributes.add(requesterSodaAttribute);
         requester.subscribeObjectClassAttributes(requesterSodaClass,
               subscribedSodaAttributes);
         AttributeHandleSet publishedServerAttributes = target.getAttributeHandleSetFactory().create();
         publishedServerAttributes.add(serverAttribute);
         target.publishObjectClassAttributes(serverClass, publishedServerAttributes);
         AttributeHandleSet publishedSodaAttributes = target.getAttributeHandleSetFactory().create();
         publishedSodaAttributes.add(sodaAttribute);
         target.publishObjectClassAttributes(sodaClass, publishedSodaAttributes);

         ObjectInstanceHandle serverObject = target.registerObjectInstance(
               serverClass, "java-2010-mom-transport-server");
         ObjectInstanceHandle sodaObject = target.registerObjectInstance(
               sodaClass, "java-2010-mom-transport-soda");
         TransportationTypeHandleFactory transportationFactory =
               target.getTransportationTypeHandleFactory();
         TransportationTypeHandle bestEffort = transportationFactory.getHLAdefaultBestEffort();
         AttributeHandleSet bestEffortAttributes = target.getAttributeHandleSetFactory().create();
         bestEffortAttributes.add(serverAttribute);
         target.requestAttributeTransportationTypeChange(
               serverObject, bestEffortAttributes, bestEffort);
         AttributeHandleValueMap serverValues = target.getAttributeHandleValueMapFactory().create(1);
         serverValues.put(serverAttribute, new byte[] {0x51});
         target.updateAttributeValues(serverObject, serverValues, new byte[0]);
         serverValues.put(serverAttribute, new byte[] {0x52});
         target.updateAttributeValues(serverObject, serverValues, new byte[0]);
         AttributeHandleValueMap sodaValues = target.getAttributeHandleValueMapFactory().create(1);
         sodaValues.put(sodaAttribute, new byte[] {0x53});
         target.updateAttributeValues(sodaObject, sodaValues, new byte[0]);

         hla.rti1516e.InteractionClassHandle updatesReport = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportUpdatesSent");
         hla.rti1516e.InteractionClassHandle reflectionsReport = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportReflectionsReceived");
         requester.subscribeInteractionClass(updatesReport);
         requester.subscribeInteractionClass(reflectionsReport);
         hla.rti1516e.InteractionClassHandle updatesRequest = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestUpdatesSent");
         hla.rti1516e.InteractionClassHandle reflectionsRequest = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestReflectionsReceived");
         hla.rti1516e.ParameterHandle updatesRequestFederate = requester.getParameterHandle(
               updatesRequest, "HLAfederate");
         hla.rti1516e.ParameterHandle reflectionsRequestFederate = requester.getParameterHandle(
               reflectionsRequest, "HLAfederate");
         hla.rti1516e.ParameterHandle updatesTransportation = requester.getParameterHandle(
               updatesReport, "HLAtransportation");
         hla.rti1516e.ParameterHandle updatesCounts = requester.getParameterHandle(
               updatesReport, "HLAupdateCounts");
         hla.rti1516e.ParameterHandle reflectionsTransportation = requester.getParameterHandle(
               reflectionsReport, "HLAtransportation");
         hla.rti1516e.ParameterHandle reflectionsCounts = requester.getParameterHandle(
               reflectionsReport, "HLAreflectCounts");
         byte[] encodedTarget = new byte[targetHandle.encodedLength()];
         targetHandle.encode(encodedTarget, 0);
         byte[] encodedRequester = new byte[requesterHandle.encodedLength()];
         requesterHandle.encode(encodedRequester, 0);
         byte[] encodedServerClass = new byte[serverClass.encodedLength()];
         serverClass.encode(encodedServerClass, 0);
         byte[] encodedSodaClass = new byte[sodaClass.encodedLength()];
         sodaClass.encode(encodedSodaClass, 0);
         TransportationTypeHandle reliable = transportationFactory.getHLAdefaultReliable();
         byte[] reliableBytes = new byte[reliable.encodedLength()];
         reliable.encode(reliableBytes, 0);
         byte[] bestEffortBytes = new byte[bestEffort.encodedLength()];
         bestEffort.encode(bestEffortBytes, 0);

         hla.rti1516e.ParameterHandleValueMap requestValues =
               requester.getParameterHandleValueMapFactory().create(1);
         requestValues.put(updatesRequestFederate, encodedTarget);
         requester.sendInteraction(updatesRequest, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 2, "MOM updates-sent report count was wrong");
         checkMomTransportCountReport(reports.get(0), updatesReport, updatesTransportation,
               updatesCounts, reliableBytes, encodedSodaClass, 1, "MOM updates reliable");
         checkMomTransportCountReport(reports.get(1), updatesReport, updatesTransportation,
               updatesCounts, bestEffortBytes, encodedServerClass, 2, "MOM updates best-effort");

         requestValues.clear();
         // Reflections are received by the requester federate, not by the
         // publishing target.  Ask for that receiver's ledger here.
         requestValues.put(reflectionsRequestFederate, encodedRequester);
         requester.sendInteraction(reflectionsRequest, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 4, "MOM reflections-received report count was wrong");
         checkMomTransportCountReport(reports.get(2), reflectionsReport,
               reflectionsTransportation, reflectionsCounts, reliableBytes, encodedSodaClass, 1,
               "MOM reflections reliable");
         checkMomTransportCountReport(reports.get(3), reflectionsReport,
               reflectionsTransportation, reflectionsCounts, bestEffortBytes, encodedServerClass, 2,
               "MOM reflections best-effort");

         requestValues.clear();
         requestValues.put(updatesRequestFederate, encodedRequester);
         requester.sendInteraction(updatesRequest, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 6, "MOM empty updates-sent report count was wrong");
         checkMomTransportCountReport(reports.get(4), updatesReport, updatesTransportation,
               updatesCounts, reliableBytes, null, 0, "MOM empty updates reliable");
         checkMomTransportCountReport(reports.get(5), updatesReport, updatesTransportation,
               updatesCounts, bestEffortBytes, null, 0, "MOM empty updates best-effort");

         requestValues.clear();
         // The publishing target has no received reflections, so this is the
         // NULL-bucket projection for both transportation types.
         requestValues.put(reflectionsRequestFederate, encodedTarget);
         requester.sendInteraction(reflectionsRequest, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 8, "MOM empty reflections-received report count was wrong");
         checkMomTransportCountReport(reports.get(6), reflectionsReport,
               reflectionsTransportation, reflectionsCounts, reliableBytes, null, 0,
               "MOM empty reflections reliable");
         checkMomTransportCountReport(reports.get(7), reflectionsReport,
               reflectionsTransportation, reflectionsCounts, bestEffortBytes, null, 0,
               "MOM empty reflections best-effort");
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (targetJoined) {
            try { target.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { target.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (targetConnected) {
            try { target.disconnect(); }
            catch (Exception ignored) {}
         }
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void checkMomTransportCountReport(Object[] report,
         hla.rti1516e.InteractionClassHandle reportClass,
         hla.rti1516e.ParameterHandle transportation,
         hla.rti1516e.ParameterHandle counts,
         byte[] expectedTransportation,
         byte[] expectedClass,
         int expectedCount,
         String label) {
      check(reportClass.equals(report[0]), label + " report class did not round-trip");
      @SuppressWarnings("unchecked")
      java.util.Map<hla.rti1516e.ParameterHandle, byte[]> values =
            (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) report[1];
      check(values.size() == 2 && values.containsKey(transportation)
            && values.containsKey(counts), label + " report parameters were wrong");
      check(java.util.Arrays.equals(expectedTransportation, values.get(transportation)),
            label + " transportation parameter did not round-trip");
      byte[] encodedCounts = values.get(counts);
      if (expectedClass == null) {
         check(encodedCounts.length == 0, label + " NULL count list was not empty");
      } else {
         check(containsBytes(encodedCounts, expectedClass)
               && containsBytes(encodedCounts,
                     java.nio.ByteBuffer.allocate(4).putInt(expectedCount).array()),
               label + " class/count payload did not round-trip");
      }
      check(((byte[]) report[2]).length == 0, label + " report tag was not empty");
      check(report[3] == hla.rti1516e.OrderType.RECEIVE,
            label + " report receive order did not round-trip");
      check("transport:HLAdefaultReliable".equals(String.valueOf(report[4])),
            label + " report transport did not round-trip");
   }

   private static void momInteractionCountReports() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      final List<Object[]> reports = new ArrayList<>();
      FederateAmbassador requesterCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            reports.add(new Object[] {interaction, parameters, tag, sentOrdering, transport});
         }
      };
      RtiFactory result = factory();
      RTIambassador requester = result.getRtiAmbassador();
      RTIambassador target = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-interactions-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean targetConnected = false;
      boolean created = false;
      boolean requesterJoined = false;
      boolean targetJoined = false;
      try {
         requester.connect(requesterCallback, CallbackModel.HLA_EVOKED);
         requesterConnected = true;
         target.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         targetConnected = true;
         target.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle targetHandle = target.joinFederationExecution(
               "java-2010-mom-interactions-target", federation);
         targetJoined = true;
         FederateHandle requesterHandle = requester.joinFederationExecution(
               "java-2010-mom-interactions-requester", federation);
         requesterJoined = true;

         hla.rti1516e.InteractionClassHandle targetOrderTaken = target.getInteractionClassHandle(
               "HLAinteractionRoot.CustomerTransactions.OrderTaken");
         hla.rti1516e.InteractionClassHandle requesterOrderTaken = requester.getInteractionClassHandle(
               "HLAinteractionRoot.CustomerTransactions.OrderTaken");
         hla.rti1516e.InteractionClassHandle targetCustomerSeated = target.getInteractionClassHandle(
               "HLAinteractionRoot.CustomerTransactions.CustomerSeated");
         hla.rti1516e.InteractionClassHandle requesterCustomerSeated = requester.getInteractionClassHandle(
               "HLAinteractionRoot.CustomerTransactions.CustomerSeated");
         target.publishInteractionClass(targetOrderTaken);
         target.publishInteractionClass(targetCustomerSeated);
         requester.subscribeInteractionClass(requesterOrderTaken);
         requester.subscribeInteractionClass(requesterCustomerSeated);
         TransportationTypeHandleFactory transportationFactory =
               target.getTransportationTypeHandleFactory();
         TransportationTypeHandle bestEffort = transportationFactory.getHLAdefaultBestEffort();
         target.sendInteraction(targetOrderTaken,
               target.getParameterHandleValueMapFactory().create(0), new byte[0]);
         target.requestInteractionTransportationTypeChange(targetOrderTaken, bestEffort);
         target.sendInteraction(targetOrderTaken,
               target.getParameterHandleValueMapFactory().create(0), new byte[0]);
         target.sendInteraction(targetOrderTaken,
               target.getParameterHandleValueMapFactory().create(0), new byte[0]);
         target.sendInteraction(targetCustomerSeated,
               target.getParameterHandleValueMapFactory().create(0), new byte[0]);
         // The requester is subscribed to the application interactions so the
         // sends above exercise the receive path.  Keep only MOM report
         // callbacks in the ledger used by the assertions below.
         reports.clear();

         hla.rti1516e.InteractionClassHandle sentReport = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsSent");
         hla.rti1516e.InteractionClassHandle receivedReport = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsReceived");
         requester.subscribeInteractionClass(sentReport);
         requester.subscribeInteractionClass(receivedReport);
         hla.rti1516e.InteractionClassHandle sentRequest = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsSent");
         hla.rti1516e.InteractionClassHandle receivedRequest = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsReceived");
         hla.rti1516e.ParameterHandle sentRequestFederate = requester.getParameterHandle(
               sentRequest, "HLAfederate");
         hla.rti1516e.ParameterHandle receivedRequestFederate = requester.getParameterHandle(
               receivedRequest, "HLAfederate");
         hla.rti1516e.ParameterHandle sentTransportation = requester.getParameterHandle(
               sentReport, "HLAtransportation");
         hla.rti1516e.ParameterHandle sentCounts = requester.getParameterHandle(
               sentReport, "HLAinteractionCounts");
         hla.rti1516e.ParameterHandle receivedTransportation = requester.getParameterHandle(
               receivedReport, "HLAtransportation");
         hla.rti1516e.ParameterHandle receivedCounts = requester.getParameterHandle(
               receivedReport, "HLAinteractionCounts");
         byte[] encodedTarget = new byte[targetHandle.encodedLength()];
         targetHandle.encode(encodedTarget, 0);
         byte[] encodedRequester = new byte[requesterHandle.encodedLength()];
         requesterHandle.encode(encodedRequester, 0);
         byte[] encodedOrderTaken = new byte[requesterOrderTaken.encodedLength()];
         requesterOrderTaken.encode(encodedOrderTaken, 0);
         byte[] encodedCustomerSeated = new byte[requesterCustomerSeated.encodedLength()];
         requesterCustomerSeated.encode(encodedCustomerSeated, 0);
         TransportationTypeHandle reliable = transportationFactory.getHLAdefaultReliable();
         byte[] reliableBytes = new byte[reliable.encodedLength()];
         reliable.encode(reliableBytes, 0);
         byte[] bestEffortBytes = new byte[bestEffort.encodedLength()];
         bestEffort.encode(bestEffortBytes, 0);

         hla.rti1516e.ParameterHandleValueMap requestValues =
               requester.getParameterHandleValueMapFactory().create(1);
         requestValues.put(sentRequestFederate, encodedTarget);
         requester.sendInteraction(sentRequest, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 2, "MOM interactions-sent report count was wrong");
         checkMomInteractionCountReport(reports.get(0), sentReport, sentTransportation, sentCounts,
               reliableBytes, new byte[][] {encodedOrderTaken, encodedCustomerSeated},
               new int[] {1, 1}, "MOM interactions sent reliable");
         checkMomInteractionCountReport(reports.get(1), sentReport, sentTransportation, sentCounts,
               bestEffortBytes, new byte[][] {encodedOrderTaken}, new int[] {2},
               "MOM interactions sent best-effort");

         requestValues.clear();
         requestValues.put(receivedRequestFederate, encodedRequester);
         requester.sendInteraction(receivedRequest, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 4, "MOM interactions-received report count was wrong");
         checkMomInteractionCountReport(reports.get(2), receivedReport, receivedTransportation,
               receivedCounts, reliableBytes, new byte[][] {encodedOrderTaken, encodedCustomerSeated},
               new int[] {1, 1}, "MOM interactions received reliable");
         checkMomInteractionCountReport(reports.get(3), receivedReport, receivedTransportation,
               receivedCounts, bestEffortBytes, new byte[][] {encodedOrderTaken}, new int[] {2},
               "MOM interactions received best-effort");

         requestValues.clear();
         requestValues.put(sentRequestFederate, encodedRequester);
         requester.sendInteraction(sentRequest, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 6, "MOM empty interactions-sent report count was wrong");
         checkMomInteractionCountReport(reports.get(4), sentReport, sentTransportation, sentCounts,
               reliableBytes, null, null, "MOM empty interactions sent reliable");
         checkMomInteractionCountReport(reports.get(5), sentReport, sentTransportation, sentCounts,
               bestEffortBytes, null, null, "MOM empty interactions sent best-effort");

         requestValues.clear();
         requestValues.put(receivedRequestFederate, encodedTarget);
         requester.sendInteraction(receivedRequest, requestValues, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 8, "MOM empty interactions-received report count was wrong");
         checkMomInteractionCountReport(reports.get(6), receivedReport, receivedTransportation,
               receivedCounts, reliableBytes, null, null,
               "MOM empty interactions received reliable");
         checkMomInteractionCountReport(reports.get(7), receivedReport, receivedTransportation,
               receivedCounts, bestEffortBytes, null, null,
               "MOM empty interactions received best-effort");
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (targetJoined) {
            try { target.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { target.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (targetConnected) {
            try { target.disconnect(); }
            catch (Exception ignored) {}
         }
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void checkMomInteractionCountReport(Object[] report,
         hla.rti1516e.InteractionClassHandle reportClass,
         hla.rti1516e.ParameterHandle transportation,
         hla.rti1516e.ParameterHandle counts,
         byte[] expectedTransportation,
         byte[][] expectedClasses,
         int[] expectedCounts,
         String label) {
      check(reportClass.equals(report[0]), label + " report class did not round-trip");
      @SuppressWarnings("unchecked")
      java.util.Map<hla.rti1516e.ParameterHandle, byte[]> values =
            (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) report[1];
      check(values.size() == 2 && values.containsKey(transportation)
            && values.containsKey(counts), label + " report parameters were wrong");
      check(java.util.Arrays.equals(expectedTransportation, values.get(transportation)),
            label + " transportation parameter did not round-trip");
      byte[] encodedCounts = values.get(counts);
      if (expectedClasses == null) {
         check(encodedCounts.length == 0, label + " NULL count list was not empty");
      } else {
         for (int index = 0; index < expectedClasses.length; index++) {
            check(containsBytes(encodedCounts, expectedClasses[index]),
                  label + " interaction class was absent from the count list");
            check(containsBytes(encodedCounts,
                  ByteBuffer.allocate(4).putInt(expectedCounts[index]).array()),
                  label + " interaction count was absent from the count list");
         }
      }
      check(((byte[]) report[2]).length == 0, label + " report tag was not empty");
      check(report[3] == hla.rti1516e.OrderType.RECEIVE,
            label + " report receive order did not round-trip");
      check("transport:HLAdefaultReliable".equals(String.valueOf(report[4])),
            label + " report transport did not round-trip");
   }

   private static void momFomMimDataReports() throws Exception {
      String fom = System.getProperty(FOM);
      String mim = System.getProperty(MIM);
      if (fom == null || fom.isEmpty() || mim == null || mim.isEmpty()) {
         throw new UnsupportedScenario("2010 FOM and standard MIM are required");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      URL mimUrl = new File(mim).toURI().toURL();
      byte[] expectedFom = Files.readAllBytes(Path.of(fom));
      byte[] expectedMim = Files.readAllBytes(Path.of(mim));
      String fomText = new String(expectedFom, StandardCharsets.UTF_8);
      String mimText = new String(expectedMim, StandardCharsets.UTF_8);
      final List<Object[]> reports = new ArrayList<>();
      FederateAmbassador callback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void receiveInteraction(
               hla.rti1516e.InteractionClassHandle interaction,
               hla.rti1516e.ParameterHandleValueMap parameters,
               byte[] tag,
               hla.rti1516e.OrderType sentOrdering,
               hla.rti1516e.TransportationTypeHandle transport,
               hla.rti1516e.FederateAmbassador.SupplementalReceiveInfo receiveInfo) {
            reports.add(new Object[] {interaction, parameters, tag, sentOrdering, transport});
         }
      };
      RtiFactory result = factory();
      RTIambassador requester = result.getRtiAmbassador();
      RTIambassador target = result.getRtiAmbassador();
      String federation = "umbra-1516e-mom-fom-mim-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean targetConnected = false;
      boolean created = false;
      boolean requesterJoined = false;
      boolean targetJoined = false;
      try {
         requester.connect(callback, CallbackModel.HLA_EVOKED);
         requesterConnected = true;
         target.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         targetConnected = true;
         target.createFederationExecution(federation, new URL[] {fomUrl}, mimUrl);
         created = true;
         FederateHandle targetHandle = target.joinFederationExecution(
               "java-2010-mom-fom-mim-target", federation);
         targetJoined = true;
         requester.joinFederationExecution("java-2010-mom-fom-mim-requester", federation);
         requesterJoined = true;

         hla.rti1516e.InteractionClassHandle federateFomReport = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFOMmoduleData");
         hla.rti1516e.InteractionClassHandle federationFomReport = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportFOMmoduleData");
         hla.rti1516e.InteractionClassHandle mimReport = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata");
         requester.subscribeInteractionClass(federateFomReport);
         requester.subscribeInteractionClass(federationFomReport);
         requester.subscribeInteractionClass(mimReport);

         hla.rti1516e.InteractionClassHandle federateFomRequest = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestFOMmoduleData");
         hla.rti1516e.InteractionClassHandle federationFomRequest = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestFOMmoduleData");
         hla.rti1516e.InteractionClassHandle mimRequest = requester.getInteractionClassHandle(
               "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestMIMdata");
         hla.rti1516e.ParameterHandle federateFomTarget = requester.getParameterHandle(
               federateFomRequest, "HLAfederate");
         hla.rti1516e.ParameterHandle federateFomIndicator = requester.getParameterHandle(
               federateFomRequest, "HLAFOMmoduleIndicator");
         hla.rti1516e.ParameterHandle federationFomIndicator = requester.getParameterHandle(
               federationFomRequest, "HLAFOMmoduleIndicator");
         hla.rti1516e.ParameterHandle federateFomReportIndicator = requester.getParameterHandle(
               federateFomReport, "HLAFOMmoduleIndicator");
         hla.rti1516e.ParameterHandle federateFomReportData = requester.getParameterHandle(
               federateFomReport, "HLAFOMmoduleData");
         hla.rti1516e.ParameterHandle federationFomReportIndicator = requester.getParameterHandle(
               federationFomReport, "HLAFOMmoduleIndicator");
         hla.rti1516e.ParameterHandle federationFomReportData = requester.getParameterHandle(
               federationFomReport, "HLAFOMmoduleData");
         hla.rti1516e.ParameterHandle mimReportData = requester.getParameterHandle(
               mimReport, "HLAMIMdata");
         byte[] encodedTarget = new byte[targetHandle.encodedLength()];
         targetHandle.encode(encodedTarget, 0);
         byte[] zero = ByteBuffer.allocate(4).order(java.nio.ByteOrder.BIG_ENDIAN)
               .putInt(0).array();

         hla.rti1516e.ParameterHandleValueMap values =
               requester.getParameterHandleValueMapFactory().create(2);
         values.put(federateFomTarget, encodedTarget);
         values.put(federateFomIndicator, zero);
         requester.sendInteraction(federateFomRequest, values, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 1, "federate FOM report count was wrong");
         checkMomModuleReport(reports.get(0), federateFomReport,
               federateFomReportIndicator, federateFomReportData, zero,
               fomText.getBytes(StandardCharsets.UTF_16BE), "federate FOM module report");

         values = requester.getParameterHandleValueMapFactory().create(1);
         values.put(federationFomIndicator, zero);
         requester.sendInteraction(federationFomRequest, values, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 2, "federation FOM report count was wrong");
         checkMomModuleReport(reports.get(1), federationFomReport,
               federationFomReportIndicator, federationFomReportData, zero,
               fomText.getBytes(StandardCharsets.UTF_16BE), "federation FOM module report");

         values = requester.getParameterHandleValueMapFactory().create(0);
         requester.sendInteraction(mimRequest, values, new byte[0]);
         try { requester.evokeMultipleCallbacks(0.0, 0.25); }
         catch (Exception ignored) {}
         check(reports.size() == 3, "federation MIM report count was wrong");
         checkMomMimReport(reports.get(2), mimReport, mimReportData,
               mimText.getBytes(StandardCharsets.UTF_16BE), "federation MIM report");
      } finally {
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (targetJoined) {
            try { target.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { target.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (targetConnected) {
            try { target.disconnect(); }
            catch (Exception ignored) {}
         }
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void checkMomModuleReport(Object[] report,
         hla.rti1516e.InteractionClassHandle reportClass,
         hla.rti1516e.ParameterHandle indicator,
         hla.rti1516e.ParameterHandle data,
         byte[] expectedIndicator,
         byte[] expectedData,
         String label) {
      check(reportClass.equals(report[0]), label + " report class did not round-trip");
      @SuppressWarnings("unchecked")
      java.util.Map<hla.rti1516e.ParameterHandle, byte[]> values =
            (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) report[1];
      check(values.size() == 2 && values.containsKey(indicator) && values.containsKey(data),
            label + " report parameters were wrong");
      check(java.util.Arrays.equals(expectedIndicator, values.get(indicator)),
            label + " module indicator did not round-trip");
      check(java.util.Arrays.equals(expectedData, values.get(data)),
            label + " module text did not round-trip");
      check(((byte[]) report[2]).length == 0, label + " report tag was not empty");
      check(report[3] == hla.rti1516e.OrderType.RECEIVE,
            label + " report receive order did not round-trip");
      check("transport:HLAdefaultReliable".equals(String.valueOf(report[4])),
            label + " report transport did not round-trip");
   }

   private static void checkMomMimReport(Object[] report,
         hla.rti1516e.InteractionClassHandle reportClass,
         hla.rti1516e.ParameterHandle data,
         byte[] expectedData,
         String label) {
      check(reportClass.equals(report[0]), label + " report class did not round-trip");
      @SuppressWarnings("unchecked")
      java.util.Map<hla.rti1516e.ParameterHandle, byte[]> values =
            (java.util.Map<hla.rti1516e.ParameterHandle, byte[]>) report[1];
      check(values.size() == 1 && values.containsKey(data),
            label + " report parameters were wrong");
      check(java.util.Arrays.equals(expectedData, values.get(data)),
            label + " MIM text did not round-trip");
      check(((byte[]) report[2]).length == 0, label + " report tag was not empty");
      check(report[3] == hla.rti1516e.OrderType.RECEIVE,
            label + " report receive order did not round-trip");
      check("transport:HLAdefaultReliable".equals(String.valueOf(report[4])),
            label + " report transport did not round-trip");
   }

   private static void timeAdvanceAndGrants() throws Exception {
      String fom = System.getProperty(FOM);
      if (fom == null || fom.isEmpty()) {
         throw new UnsupportedScenario("no 2010 FOM configured");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      final LogicalTime[] regulation = new LogicalTime[1];
      final LogicalTime[] constrained = new LogicalTime[1];
      final LogicalTime[] grants = new LogicalTime[2];
      FederateAmbassador callback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void timeRegulationEnabled(LogicalTime time) {
            regulation[0] = time;
         }

         @Override public void timeConstrainedEnabled(LogicalTime time) {
            constrained[0] = time;
         }

         @Override public void timeAdvanceGrant(LogicalTime time) {
            if (grants[0] == null) grants[0] = time;
            else grants[1] = time;
         }
      };
      RtiFactory result = factory();
      RTIambassador ambassador = result.getRtiAmbassador();
      String federation = "umbra-1516e-time-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         ambassador.connect(callback, CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, fomUrl);
         created = true;
         ambassador.joinFederationExecution("umbra-1516e-time", federation);
         joined = true;

         LogicalTimeFactory timeFactory = ambassador.getTimeFactory();
         LogicalTime initial = timeFactory.makeInitial();
         LogicalTimeInterval epsilon = timeFactory.makeEpsilon();
         ambassador.enableTimeRegulation(epsilon);
         check(regulation[0] != null, "time-regulation callback was not delivered");
         check(regulation[0].compareTo(initial) == 0,
               "time-regulation callback time did not match initial time");
         ambassador.enableTimeConstrained();
         check(constrained[0] != null, "time-constrained callback was not delivered");
         check(constrained[0].compareTo(initial) == 0,
               "time-constrained callback time did not match initial time");

         LogicalTime requested = (LogicalTime) initial.add(epsilon);
         ambassador.timeAdvanceRequest(requested);
         check(grants[0] != null && grants[0].compareTo(requested) == 0,
               "time-advance grant did not match requested time");
         ambassador.timeAdvanceRequestAvailable((LogicalTime) requested.add(epsilon));
         check(grants[1] != null && grants[1].compareTo(requested.add(epsilon)) == 0,
               "available time-advance grant did not match requested time");
         check(ambassador.queryLogicalTime().compareTo(grants[1]) == 0,
               "query logical time did not reflect the grant");
         ambassador.disableTimeConstrained();
         ambassador.disableTimeRegulation();
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (connected) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void ownershipManagement() throws Exception {
      String fom = System.getProperty(FOM);
      if (fom == null || fom.isEmpty()) {
         throw new UnsupportedScenario("no 2010 FOM configured");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      final Object[] ownership = new Object[3];
      FederateAmbassador observerCallback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void discoverObjectInstance(
               hla.rti1516e.ObjectInstanceHandle object,
               hla.rti1516e.ObjectClassHandle objectClass,
               String objectName) {
            ownership[0] = object;
         }

         @Override public void informAttributeOwnership(
               hla.rti1516e.ObjectInstanceHandle object,
               AttributeHandle attribute,
               FederateHandle owner) {
            ownership[0] = object;
            ownership[1] = attribute;
            ownership[2] = owner;
         }
      };
      RtiFactory result = factory();
      RTIambassador publisher = result.getRtiAmbassador();
      RTIambassador observer = result.getRtiAmbassador();
      String federation = "umbra-1516e-ownership-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean observerConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean observerJoined = false;
      try {
         publisher.connect(new hla.rti1516e.NullFederateAmbassador(), CallbackModel.HLA_EVOKED);
         publisherConnected = true;
         observer.connect(observerCallback, CallbackModel.HLA_EVOKED);
         observerConnected = true;
         publisher.createFederationExecution(federation, fomUrl);
         created = true;
         publisher.joinFederationExecution("java-owner", federation);
         publisherJoined = true;
         observer.joinFederationExecution("java-observer", federation);
         observerJoined = true;

         hla.rti1516e.ObjectClassHandle publisherClass =
               publisher.getObjectClassHandle("HLAobjectRoot.Employee.Server");
         AttributeHandle publisherAttribute =
               publisher.getAttributeHandle(publisherClass, "Efficiency");
         hla.rti1516e.ObjectClassHandle observerClass =
               observer.getObjectClassHandle("HLAobjectRoot.Employee.Server");
         AttributeHandle observerAttribute =
               observer.getAttributeHandle(observerClass, "Efficiency");
         AttributeHandleSet observerAttributes = observer.getAttributeHandleSetFactory().create();
         observerAttributes.add(observerAttribute);
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes);
         AttributeHandleSet publisherAttributes = publisher.getAttributeHandleSetFactory().create();
         publisherAttributes.add(publisherAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);

         hla.rti1516e.ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         observer.queryAttributeOwnership(object, observerAttribute);
         check(ownership[0].equals(object), "ownership object did not round-trip");
         check(ownership[1].equals(observerAttribute), "ownership attribute did not round-trip");
         FederateHandle owner = publisher.getFederateHandle("java-owner");
         check(ownership[2].equals(owner), "ownership federate handle did not round-trip");
         check(publisher.getFederateName(owner).equals("java-owner"),
               "ownership federate name did not round-trip");
         check(publisher.isAttributeOwnedByFederate(object, publisherAttribute),
               "publisher did not report owning its attribute");
      } finally {
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (observerConnected) {
            try { observer.disconnect(); }
            catch (Exception ignored) {}
         }
         if (publisherConnected) {
            try { publisher.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static void saveAndRestore() throws Exception {
      String fom = System.getProperty(FOM);
      if (fom == null || fom.isEmpty()) {
         throw new UnsupportedScenario("no 2010 FOM configured");
      }
      URL fomUrl = new File(fom).toURI().toURL();
      final int[] saveInitiations = new int[1];
      final int[] federationSaved = new int[1];
      final int[] restoreSuccesses = new int[1];
      final int[] restoreBegun = new int[1];
      final int[] restoreInitiations = new int[1];
      final int[] federationRestored = new int[1];
      FederateAmbassador callback = new hla.rti1516e.NullFederateAmbassador() {
         @Override public void initiateFederateSave(String label) {
            if ("java-2010-save".equals(label)) saveInitiations[0]++;
         }

         @Override public void federationSaved() {
            federationSaved[0]++;
         }

         @Override public void requestFederationRestoreSucceeded(String label) {
            if ("java-2010-save".equals(label)) restoreSuccesses[0]++;
         }

         @Override public void federationRestoreBegun() {
            restoreBegun[0]++;
         }

         @Override public void initiateFederateRestore(
               String label, String federateName, FederateHandle federateHandle) {
            if ("java-2010-save".equals(label)) restoreInitiations[0]++;
         }

         @Override public void federationRestored() {
            federationRestored[0]++;
         }
      };
      RtiFactory result = factory();
      RTIambassador ambassador = result.getRtiAmbassador();
      String federation = "umbra-1516e-save-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         ambassador.connect(callback, CallbackModel.HLA_EVOKED);
         connected = true;
         ambassador.createFederationExecution(federation, fomUrl);
         created = true;
         ambassador.joinFederationExecution("java-save", federation);
         joined = true;

         ambassador.requestFederationSave("java-2010-save");
         check(saveInitiations[0] == 1,
               "initiateFederateSave did not cross the callback interface");
         ambassador.federateSaveBegun();
         ambassador.federateSaveComplete();
         check(federationSaved[0] == 1,
               "federationSaved did not cross the callback interface");

         ambassador.requestFederationRestore("java-2010-save");
         check(restoreSuccesses[0] == 1,
               "requestFederationRestoreSucceeded did not cross the callback interface");
         check(restoreBegun[0] == 1 && restoreInitiations[0] == 1,
               "restore initiation callbacks did not cross the callback interface");
         ambassador.federateRestoreComplete();
         check(federationRestored[0] == 1,
               "federationRestored did not cross the callback interface");
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) {}
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) {}
         }
         if (connected) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) {}
         }
      }
   }

   private static int countAbstract(Class<?> type) {
      int count = 0;
      for (Method method : type.getDeclaredMethods()) {
         if (Modifier.isAbstract(method.getModifiers())) count++;
      }
      return count;
   }

   private static int overloads(Class<?> type, String name) {
      int count = 0;
      for (Method method : type.getDeclaredMethods()) {
         if (method.getName().equals(name)) count++;
      }
      return count;
   }

   private static void check(boolean condition, String message) {
      if (!condition) throw new AssertionError(message);
   }

   private static boolean containsBytes(byte[] value, byte[] needle) {
      if (value == null || needle == null || needle.length == 0 || value.length < needle.length) {
         return false;
      }
      for (int start = 0; start <= value.length - needle.length; start++) {
         boolean match = true;
         for (int offset = 0; offset < needle.length; offset++) {
            if (value[start + offset] != needle[offset]) {
               match = false;
               break;
            }
         }
         if (match) return true;
      }
      return false;
   }

   private static void writeResults() throws IOException {
      String path = System.getProperty(RESULTS);
      if (path == null || path.isEmpty()) return;
      Path output = Path.of(path);
      Files.createDirectories(output.toAbsolutePath().getParent());
      try (PrintWriter writer = new PrintWriter(Files.newBufferedWriter(output, StandardCharsets.UTF_8))) {
         writer.println("{\"standard\":\"IEEE 1516.1-2010\",\"capability_profile\":\""
               + escape(System.getProperty(PROFILE, "default")) + "\",\"scenarios\":[");
         for (int index = 0; index < results.size(); index++) {
            Result result = results.get(index);
            writer.print("{\"id\":\"" + escape(result.id) + "\",\"category\":\""
                  + escape(result.category) + "\",\"status\":\"" + escape(result.status)
                  + "\",\"message\":\"" + escape(result.message) + "\"}");
            if (index + 1 < results.size()) writer.println(",");
         }
         writer.println("]}");
      }
   }

   private static String escape(String value) {
      return value.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", "\\n");
   }

   private interface CheckedScenario { void run() throws Exception; }
   private static final class UnsupportedScenario extends Exception {
      UnsupportedScenario(String message) { super(message); }
   }
   private static final class Result {
      final String id;
      final String category;
      final String status;
      final String message;
      Result(String id, String category, String status, String message) {
         this.id = id; this.category = category; this.status = status; this.message = message;
      }
   }
}
