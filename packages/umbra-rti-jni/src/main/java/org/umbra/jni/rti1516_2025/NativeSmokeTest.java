package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.RtiFactoryFactory;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.encoding.ByteWrapper;
import hla.rti1516_2025.encoding.DataElement;
import hla.rti1516_2025.encoding.DecoderException;
import hla.rti1516_2025.encoding.HLAfixedArray;
import hla.rti1516_2025.encoding.HLAinteger16BE;
import hla.rti1516_2025.encoding.HLAinteger16LE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAinteger32LE;
import hla.rti1516_2025.encoding.HLAinteger64BE;
import hla.rti1516_2025.encoding.HLAinteger64LE;
import hla.rti1516_2025.exceptions.FederateNotExecutionMember;
import java.lang.reflect.Proxy;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

/** Java-only verification of the C++ -> JNI -> Java callback path. */
public final class NativeSmokeTest {
   private NativeSmokeTest() {
   }

   public static void main(String[] arguments) throws Exception {
      RtiFactory factory = RtiFactoryFactory.getRtiFactory(NativeRtiFactory.NAME);
      HLAinteger32BE integer = factory.getEncoderFactory().createHLAinteger32BE(-7);
      if (integer.getValue() != -7 || integer.getOctetBoundary() != 4 ||
          integer.getEncodedLength() != 4 || integer.toByteArray()[3] != (byte) 0xf9) {
         throw new AssertionError("C++ HLAinteger32BE did not cross the JNI encoder boundary");
      }
      integer.decode(new byte[] { 1, 2, 3, 4 });
      if (integer.getValue() != 0x01020304) {
         throw new AssertionError("C++ HLAinteger32BE decode did not cross the JNI encoder boundary");
      }
      DataElement[] fixedValues = new DataElement[] {
         factory.getEncoderFactory().createHLAinteger32BE(17),
         factory.getEncoderFactory().createHLAinteger32BE(-23)
      };
      HLAfixedArray fixedArray = factory.getEncoderFactory().createHLAfixedArray(fixedValues);
      if (fixedArray.size() != 2 ||
          ((HLAinteger32BE) fixedArray.get(0)).getValue() != 17 ||
          ((HLAinteger32BE) fixedArray.get(1)).getValue() != -23) {
         throw new AssertionError("C++ fixed-array varargs indexing did not cross the JNI encoder boundary");
      }
      requireVendorDataElementCarrier();
      HLAinteger32LE integer32LE = factory.getEncoderFactory().createHLAinteger32LE(-7);
      if (integer32LE.getValue() != -7 || integer32LE.getOctetBoundary() != 4 ||
          integer32LE.getEncodedLength() != 4 || integer32LE.toByteArray()[0] != (byte) 0xf9) {
         throw new AssertionError("C++ HLAinteger32LE did not cross the JNI encoder boundary");
      }
      integer32LE.decode(new byte[] { 4, 3, 2, 1 });
      if (integer32LE.getValue() != 0x01020304) {
         throw new AssertionError("C++ HLAinteger32LE decode did not cross the JNI encoder boundary");
      }
      HLAinteger64BE integer64 = factory.getEncoderFactory().createHLAinteger64BE(-7L);
      if (integer64.getValue() != -7L || integer64.getOctetBoundary() != 8 ||
          integer64.getEncodedLength() != 8 || integer64.toByteArray()[7] != (byte) 0xf9) {
         throw new AssertionError("C++ HLAinteger64BE did not cross the JNI encoder boundary");
      }
      integer64.decode(new byte[] { 1, 2, 3, 4, 5, 6, 7, 8 });
      if (integer64.getValue() != 0x0102030405060708L) {
         throw new AssertionError("C++ HLAinteger64BE decode did not cross the JNI encoder boundary");
      }
      HLAinteger64LE integer64LE = factory.getEncoderFactory().createHLAinteger64LE(-7L);
      if (integer64LE.getValue() != -7L || integer64LE.getOctetBoundary() != 8 ||
          integer64LE.getEncodedLength() != 8 || integer64LE.toByteArray()[0] != (byte) 0xf9) {
         throw new AssertionError("C++ HLAinteger64LE did not cross the JNI encoder boundary");
      }
      integer64LE.decode(new byte[] { 8, 7, 6, 5, 4, 3, 2, 1 });
      if (integer64LE.getValue() != 0x0102030405060708L) {
         throw new AssertionError("C++ HLAinteger64LE decode did not cross the JNI encoder boundary");
      }
      HLAinteger16BE integer16 = factory.getEncoderFactory().createHLAinteger16BE((short) -7);
      if (integer16.getValue() != -7 || integer16.getOctetBoundary() != 2 ||
          integer16.getEncodedLength() != 2 || integer16.toByteArray()[1] != (byte) 0xf9) {
         throw new AssertionError("C++ HLAinteger16BE did not cross the JNI encoder boundary");
      }
      HLAinteger16LE integer16LE = factory.getEncoderFactory().createHLAinteger16LE((short) -7);
      if (integer16LE.getValue() != -7 || integer16LE.getOctetBoundary() != 2 ||
          integer16LE.getEncodedLength() != 2 || integer16LE.toByteArray()[0] != (byte) 0xf9) {
         throw new AssertionError("C++ HLAinteger16LE did not cross the JNI encoder boundary");
      }
      integer16LE.decode(new byte[] { 2, 1 });
      if (integer16LE.getValue() != 0x0102) {
         throw new AssertionError("C++ HLAinteger16LE decode did not cross the JNI encoder boundary");
      }
      RTIambassador ambassador = factory.getRtiAmbassador();
      AtomicInteger executionReports = new AtomicInteger();
      AtomicInteger memberReports = new AtomicInteger();
      AtomicReference<String> missingFederation = new AtomicReference<>();
      FederateAmbassador callbacks = (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] { FederateAmbassador.class },
         (proxy, method, invocationArguments) -> {
            if ("reportFederationExecutions".equals(method.getName())) {
               executionReports.incrementAndGet();
            } else if ("reportFederationExecutionMembers".equals(method.getName())) {
               memberReports.incrementAndGet();
            } else if ("reportFederationExecutionDoesNotExist".equals(method.getName())) {
               missingFederation.set((String) invocationArguments[0]);
            }
            return null;
         });

      ConfigurationResult result = ambassador.connect(callbacks, CallbackModel.HLA_EVOKED);
      if (result.configurationUsed || result.addressUsed || !"2025.jni-smoke".equals(factory.rtiVersion())) {
         throw new AssertionError("JNI factory did not return the native connection result");
      }
      ambassador.listFederationExecutions();
      ambassador.evokeCallback(0.0);
      if (executionReports.get() != 1) {
         throw new AssertionError("C++ federation-execution report did not reach Java");
      }
      try {
         ambassador.getFederateHandle("not-bound-by-jni");
         throw new AssertionError("Expected explicitly unbound JNI service");
      } catch (FederateNotExecutionMember expected) {
         // The native C++ membership precondition reaches Java as its
         // standard checked type instead of fixture-owned behavior.
      }
      ambassador.listFederationExecutionMembers("jni-missing-federation");
      ambassador.evokeCallback(0.0);
      if (!"jni-missing-federation".equals(missingFederation.get())) {
         throw new AssertionError("C++ missing-federation callback did not reach Java");
      }
      if (arguments.length > 0) {
         String federationName = "jni-smoke-membership-" + System.nanoTime();
         ambassador.createFederationExecution(
            federationName,
            arguments[0],
            "HLAinteger64Time");
         FederateHandle member = ambassador.joinFederationExecution(
            "jni-smoke-member",
            "jni-smoke-type",
            federationName);
         if (member == null || member.encodedLength() == 0) {
            throw new AssertionError("C++ membership did not return a federate handle");
         }
         ambassador.listFederationExecutionMembers(federationName);
         ambassador.evokeCallback(0.0);
         if (memberReports.get() != 1) {
            throw new AssertionError("C++ membership report did not reach Java");
         }
         if (!"jni-smoke-member".equals(ambassador.getFederateName(member))) {
            throw new AssertionError("C++ federate handle/name lookup did not round-trip");
         }
         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         ambassador.destroyFederationExecution(federationName);
      }
      ambassador.disconnect();
      ambassador.disconnect();
      ((AutoCloseable) ambassador).close();
      System.out.println("Umbra JNI Java RTI smoke test: OK");
   }

   /** Return an unknown provider-owned DataElement for raw-carrier tests. */
   public static DataElement vendorDataElementCarrier() {
      return new NativeVendorDataElement(new byte[] { 'a', 'b', 'c', 'd' });
   }

   private static void requireVendorDataElementCarrier() throws Exception {
      DataElement carrier = vendorDataElementCarrier();
      if (carrier.getOctetBoundary() != 1 || carrier.getEncodedLength() != 4 ||
          !java.util.Arrays.equals(carrier.toByteArray(), new byte[] { 'a', 'b', 'c', 'd' })) {
         throw new AssertionError("provider-owned 2025 DataElement carrier did not retain its payload");
      }
      ByteWrapper cursor = new ByteWrapper(new byte[] { '!', 'w', 'x', 'y', 'z', '?' }, 1, 5);
      carrier.decode(cursor);
      if (!java.util.Arrays.equals(carrier.toByteArray(), new byte[] { 'w', 'x', 'y', 'z' }) ||
          cursor.getPos() != 5 || cursor.remaining() != 1) {
         throw new AssertionError("provider-owned 2025 DataElement cursor contract did not advance exactly");
      }
      try {
         carrier.decode(new byte[] { 'x', 'y', 'z' });
         throw new AssertionError("provider-owned 2025 DataElement accepted a truncated payload");
      } catch (DecoderException expected) {
         // The carrier retains its previous value after malformed input.
      }
      if (!java.util.Arrays.equals(carrier.toByteArray(), new byte[] { 'w', 'x', 'y', 'z' })) {
         throw new AssertionError("provider-owned 2025 DataElement exposed partial malformed state");
      }
   }
}
