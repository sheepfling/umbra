package org.umbra.jni.rti1516e;

import hla.rti1516e.CallbackModel;
import hla.rti1516e.NullFederateAmbassador;
import hla.rti1516e.RTIambassador;
import hla.rti1516e.RtiFactory;
import hla.rti1516e.RtiFactoryFactory;
import hla.rti1516e.encoding.DecoderException;
import hla.rti1516e.encoding.EncoderFactory;
import hla.rti1516e.encoding.HLAinteger32BE;
import hla.rti1516e.exceptions.RTIinternalError;
import java.util.ServiceLoader;

/** Small release-artifact smoke test; this is not conformance evidence. */
public final class NativeSmokeTest {
   private NativeSmokeTest() {
   }

   public static void main(String[] arguments) throws Exception {
      RtiFactory factory;
      try {
         factory = RtiFactoryFactory.getRtiFactory(NativeRtiFactory.NAME);
      } catch (IllegalArgumentException compatibilityFailure) {
         // The published 2010 helper routes through ImageIO's SPI registry,
         // which rejects arbitrary providers on modern JDKs. Keep the smoke
         // test on the same standard service descriptor via ServiceLoader.
         factory = ServiceLoader.load(RtiFactory.class).stream()
            .map(ServiceLoader.Provider::get)
            .filter(candidate -> NativeRtiFactory.NAME.equals(candidate.rtiName()))
            .findFirst()
            .orElseThrow(() -> compatibilityFailure);
      }
      if (!NativeRtiFactory.NAME.equals(factory.rtiName())) {
         throw new AssertionError("unexpected 2010 JNI factory name");
      }
      RTIambassador ambassador = factory.getRtiAmbassador();
      try {
         ambassador.connect(new NullFederateAmbassador(), CallbackModel.HLA_IMMEDIATE);
         try {
            ambassador.connect(new NullFederateAmbassador(), CallbackModel.HLA_IMMEDIATE);
            throw new AssertionError("duplicate JNI connect unexpectedly succeeded");
         } catch (hla.rti1516e.exceptions.AlreadyConnected expected) {
            // The connection lifecycle slice preserves the standard identity.
         }
         ambassador.disconnect();
         try {
            ambassador.disconnect();
            throw new AssertionError("duplicate JNI disconnect unexpectedly succeeded");
         } catch (RTIinternalError expected) {
            if (!expected.getMessage().contains("not connected")) {
               throw new AssertionError("unexpected disconnected message", expected);
            }
         }
         EncoderFactory encoder = factory.getEncoderFactory();
         HLAinteger32BE integer = encoder.createHLAinteger32BE(0x01020304);
         if (integer.getValue() != 0x01020304 || integer.toByteArray().length != 4) {
            throw new AssertionError("JNI factory encoder did not preserve integer value");
         }
         integer.decode(new byte[] {4, 3, 2, 1});
         if (integer.getValue() != 0x04030201) {
            throw new AssertionError("JNI factory encoder decode did not round-trip");
         }
         try {
            integer.decode(new byte[] {0});
            throw new AssertionError("JNI factory encoder accepted truncated input");
         } catch (DecoderException expected) {
            // The registered factory preserves the standard malformed-input type.
         }
      } finally {
         ((AutoCloseable) ambassador).close();
      }
   }
}
