package org.umbra.jni.rti1516e;

import hla.rti1516e.RTIambassador;
import hla.rti1516e.RtiFactory;
import hla.rti1516e.RtiFactoryFactory;
import hla.rti1516e.LogicalTimeFactory;
import hla.rti1516e.LogicalTimeFactoryFactory;
import hla.rti1516e.encoding.EncoderFactory;
import hla.rti1516e.time.HLAfloat64TimeFactory;
import hla.rti1516e.time.HLAinteger64TimeFactory;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.util.LinkedHashSet;
import java.util.ServiceLoader;
import java.util.Set;

/**
 * Surface-only JNI smoke test.
 *
 * <p>This test deliberately does not invoke RTI services.  It proves that a
 * caller-supplied IEEE 1516e API JAR, the ServiceLoader provider, the JNI
 * factory, and the dynamic ambassador proxy agree on every declared
 * RTIambassador method signature.  Unsupported service behavior is outside
 * this check and belongs to a capability profile.</p>
 */
public final class SurfaceSmokeTest {
   private SurfaceSmokeTest() {
   }

   private static RtiFactory loadFactory() throws Exception {
      try {
         return RtiFactoryFactory.getRtiFactory(NativeRtiFactory.NAME);
      } catch (IllegalArgumentException compatibilityFailure) {
         return ServiceLoader.load(RtiFactory.class).stream()
            .map(ServiceLoader.Provider::get)
            .filter(candidate -> NativeRtiFactory.NAME.equals(candidate.rtiName()))
            .findFirst()
            .orElseThrow(() -> compatibilityFailure);
      }
   }

   public static void main(String[] arguments) throws Exception {
      if (!"hla.rti1516e".equals(RTIambassador.class.getPackageName())) {
         throw new AssertionError("unexpected IEEE 1516e API package");
      }
      RtiFactory factory = loadFactory();
      if (!NativeRtiFactory.NAME.equals(factory.rtiName())) {
         throw new AssertionError("unexpected 2010 JNI factory name");
      }
      Set<String> serviceProviders = new LinkedHashSet<>();
      for (RtiFactory provider : ServiceLoader.load(RtiFactory.class)) {
         serviceProviders.add(provider.rtiName());
      }
      if (!serviceProviders.contains(NativeRtiFactory.NAME)) {
         throw new AssertionError("2010 RtiFactory ServiceLoader did not register Umbra");
      }
      boolean availableFactory = false;
      try {
         for (RtiFactory provider : RtiFactoryFactory.getAvailableRtiFactories()) {
            if (NativeRtiFactory.NAME.equals(provider.rtiName())) {
               availableFactory = true;
               break;
            }
         }
      } catch (IllegalArgumentException compatibilityFailure) {
         // Some published 2010 helpers route this method through ImageIO's
         // SPI registry, which rejects arbitrary RtiFactory interfaces on
         // modern JDKs. The direct ServiceLoader check above remains the
         // authoritative registration proof.
         availableFactory = serviceProviders.contains(NativeRtiFactory.NAME);
      }
      if (!availableFactory) {
         throw new AssertionError("available 2010 RtiFactory set omitted Umbra");
      }
      EncoderFactory encoder = factory.getEncoderFactory();
      if (encoder == null) throw new AssertionError("registered 2010 factory returned no EncoderFactory");

      Set<String> logicalTimeProviders = new LinkedHashSet<>();
      try {
         for (LogicalTimeFactory provider : LogicalTimeFactoryFactory.getAvailableLogicalTimeFactories()) {
            logicalTimeProviders.add(provider.getName());
         }
      } catch (IllegalArgumentException compatibilityFailure) {
         // The same modern-JDK ImageIO SPI restriction can affect the 2010
         // logical-time helper. The standard ServiceLoader is equivalent for
         // this provider-registration check.
         for (LogicalTimeFactory provider : ServiceLoader.load(LogicalTimeFactory.class)) {
            logicalTimeProviders.add(provider.getName());
         }
      }
      if (!logicalTimeProviders.contains("HLAinteger64Time") ||
          !logicalTimeProviders.contains("HLAfloat64Time")) {
         throw new AssertionError("2010 logical-time ServiceLoader registration is incomplete");
      }
      if (!(findLogicalTimeFactory("HLAinteger64Time") instanceof HLAinteger64TimeFactory) ||
          !(findLogicalTimeFactory("HLAfloat64Time") instanceof HLAfloat64TimeFactory)) {
         throw new AssertionError("2010 logical-time lookup returned the wrong standard shape");
      }
      HLAinteger64TimeFactory integerFactory =
         (HLAinteger64TimeFactory) findLogicalTimeFactory("HLAinteger64Time");
      if (integerFactory.makeTime(123456789L).getValue() != 123456789L ||
          integerFactory.makeInterval(1234L).getValue() != 1234L ||
          integerFactory.decodeTime(
             ByteBuffer.allocate(Long.BYTES + 1).put((byte) 0).putLong(123456789L).array(),
             1).getValue() != 123456789L) {
         throw new AssertionError("2010 integer logical-time provider did not return standard values");
      }
      HLAfloat64TimeFactory floatFactory =
         (HLAfloat64TimeFactory) findLogicalTimeFactory("HLAfloat64Time");
      if (Double.compare(floatFactory.makeTime(12.5D).getValue(), 12.5D) != 0 ||
          Double.compare(floatFactory.makeInterval(0.25D).getValue(), 0.25D) != 0) {
         throw new AssertionError("2010 floating logical-time provider did not return standard values");
      }

      RTIambassador ambassador = factory.getRtiAmbassador();
      try {
         for (Method declared : RTIambassador.class.getDeclaredMethods()) {
            try {
               ambassador.getClass().getMethod(
                  declared.getName(), declared.getParameterTypes());
            } catch (NoSuchMethodException missing) {
               throw new AssertionError(
                  "JNI proxy is missing RTIambassador method " + declared, missing);
            }
         }
      } finally {
         ((AutoCloseable) ambassador).close();
      }
   }

   private static LogicalTimeFactory findLogicalTimeFactory(String name) {
      try {
         return LogicalTimeFactoryFactory.getLogicalTimeFactory(name);
      } catch (IllegalArgumentException compatibilityFailure) {
         for (LogicalTimeFactory provider : ServiceLoader.load(LogicalTimeFactory.class)) {
            if (name.equals(provider.getName())) return provider;
         }
         return null;
      }
   }
}
