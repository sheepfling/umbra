package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.encoding.ByteWrapper;
import hla.rti1516_2025.encoding.DecoderException;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAlogicalTime;
import hla.rti1516_2025.encoding.HLAlogicalTimeInterval;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;

/** Java-visible subset of the standard logical-time DataElement wrapper contract. */
final class LogicalTimeDataElementsTck {
   private static final byte PREFIX = (byte) 0xA4;
   private static final byte SUFFIX = (byte) 0x5A;

   private LogicalTimeDataElementsTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      String federation = "java-tck-logical-time-data-elements-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         ambassador.connect(federateAmbassador(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution("java-tck-logical-time-data-elements", federation);
         joined = true;

         LogicalTimeFactory<?, ?> timeFactory = ambassador.getTimeFactory();
         check(timeFactory != null && timeImplementation.equals(timeFactory.getName()),
            "RTI returned a different logical-time factory than the federation selected");
         EncoderFactory encoder = factory.getEncoderFactory();
         verifyLogicalTime(encoder, ambassador, timeFactory);
         verifyLogicalTimeInterval(encoder, ambassador, timeFactory);
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static <T extends LogicalTime<T, U>, U extends LogicalTimeInterval<U>>
         void verifyLogicalTime(EncoderFactory encoder, RTIambassador ambassador,
               LogicalTimeFactory<T, U> factory) throws Exception {
      T initial = factory.makeInitial();
      T finalTime = factory.makeFinal();
      HLAlogicalTime<T, U> element = encoder.createHLAlogicalTime(ambassador);
      byte[] initialBytes = encodeTime(initial);
      check(Arrays.equals(element.toByteArray(), initialBytes),
         "HLAlogicalTime did not expose the selected factory's initial encoding");
      check(element.getEncodedLength() == initialBytes.length
            && element.getOctetBoundary() == 1,
         "HLAlogicalTime length or octet-boundary metadata changed");
      check(element.getValue().equals(initial),
         "HLAlogicalTime did not start with the selected factory's initial value");

      element.setValue(finalTime);
      byte[] expected = encodeTime(finalTime);
      check(Arrays.equals(element.toByteArray(), expected)
            && element.getEncodedLength() == expected.length,
         "HLAlogicalTime did not encode the assigned logical time");
      ByteWrapper encoded = element.encode();
      check(Arrays.equals(encoded.array(), expected) && encoded.getPos() == expected.length,
         "HLAlogicalTime.encode did not return its complete encoding");

      ByteWrapper appended = new ByteWrapper(expected.length + 2);
      appended.put(PREFIX);
      element.encode(appended);
      appended.put(SUFFIX);
      byte[] appendedBytes = appended.array();
      check(appended.getPos() == expected.length + 2
            && appendedBytes[0] == PREFIX
            && Arrays.equals(Arrays.copyOfRange(appendedBytes, 1, 1 + expected.length), expected)
            && appendedBytes[appendedBytes.length - 1] == SUFFIX,
         "HLAlogicalTime.encode(ByteWrapper) did not append its complete encoding");

      HLAlogicalTime<T, U> decoded = encoder.createHLAlogicalTime(ambassador);
      check(decoded.decode(expected) == decoded && decoded.getValue().equals(finalTime),
         "HLAlogicalTime did not round-trip through its Java DataElement decoder");
      ByteWrapper source = new ByteWrapper(appendedBytes);
      source.advance(1);
      HLAlogicalTime<T, U> nestedDecoded = encoder.createHLAlogicalTime(ambassador);
      check(nestedDecoded.decode(source) == nestedDecoded
            && source.getPos() == 1 + expected.length
            && source.array()[source.getPos()] == SUFFIX
            && nestedDecoded.getValue().equals(finalTime),
         "HLAlogicalTime.decode(ByteWrapper) did not preserve the surrounding cursor");

      expectDecoderFailure(() -> decoded.decode(Arrays.copyOf(expected, expected.length - 1)),
         "HLAlogicalTime accepted a truncated byte-array value");
      ByteWrapper truncated = new ByteWrapper(Arrays.copyOf(expected, expected.length - 1));
      expectDecoderFailure(() -> nestedDecoded.decode(truncated),
         "HLAlogicalTime accepted a truncated ByteWrapper value");
      check(truncated.getPos() == 0,
         "HLAlogicalTime advanced the ByteWrapper after a failed decode");
   }

   private static <T extends LogicalTime<T, U>, U extends LogicalTimeInterval<U>>
         void verifyLogicalTimeInterval(EncoderFactory encoder, RTIambassador ambassador,
               LogicalTimeFactory<T, U> factory) throws Exception {
      U zero = factory.makeZero();
      U epsilon = factory.makeEpsilon();
      HLAlogicalTimeInterval<T, U> element =
         encoder.createHLAlogicalTimeInterval(ambassador);
      byte[] zeroBytes = encodeInterval(zero);
      check(Arrays.equals(element.toByteArray(), zeroBytes),
         "HLAlogicalTimeInterval did not expose the selected factory's zero encoding");
      check(element.getEncodedLength() == zeroBytes.length
            && element.getOctetBoundary() == 1,
         "HLAlogicalTimeInterval length or octet-boundary metadata changed");
      check(element.getValue().equals(zero),
         "HLAlogicalTimeInterval did not start with the selected factory's zero value");

      element.setValue(epsilon);
      byte[] expected = encodeInterval(epsilon);
      check(Arrays.equals(element.toByteArray(), expected)
            && element.getEncodedLength() == expected.length,
         "HLAlogicalTimeInterval did not encode the assigned interval");
      ByteWrapper encoded = element.encode();
      check(Arrays.equals(encoded.array(), expected) && encoded.getPos() == expected.length,
         "HLAlogicalTimeInterval.encode did not return its complete encoding");

      ByteWrapper appended = new ByteWrapper(expected.length + 2);
      appended.put(PREFIX);
      element.encode(appended);
      appended.put(SUFFIX);
      byte[] appendedBytes = appended.array();
      check(appended.getPos() == expected.length + 2
            && appendedBytes[0] == PREFIX
            && Arrays.equals(Arrays.copyOfRange(appendedBytes, 1, 1 + expected.length), expected)
            && appendedBytes[appendedBytes.length - 1] == SUFFIX,
         "HLAlogicalTimeInterval.encode(ByteWrapper) did not append its complete encoding");

      HLAlogicalTimeInterval<T, U> decoded =
         encoder.createHLAlogicalTimeInterval(ambassador);
      check(decoded.decode(expected) == decoded && decoded.getValue().equals(epsilon),
         "HLAlogicalTimeInterval did not round-trip through its Java DataElement decoder");
      ByteWrapper source = new ByteWrapper(appendedBytes);
      source.advance(1);
      HLAlogicalTimeInterval<T, U> nestedDecoded =
         encoder.createHLAlogicalTimeInterval(ambassador);
      check(nestedDecoded.decode(source) == nestedDecoded
            && source.getPos() == 1 + expected.length
            && source.array()[source.getPos()] == SUFFIX
            && nestedDecoded.getValue().equals(epsilon),
         "HLAlogicalTimeInterval.decode(ByteWrapper) did not preserve the surrounding cursor");

      expectDecoderFailure(() -> decoded.decode(Arrays.copyOf(expected, expected.length - 1)),
         "HLAlogicalTimeInterval accepted a truncated byte-array value");
      ByteWrapper truncated = new ByteWrapper(Arrays.copyOf(expected, expected.length - 1));
      expectDecoderFailure(() -> nestedDecoded.decode(truncated),
         "HLAlogicalTimeInterval accepted a truncated ByteWrapper value");
      check(truncated.getPos() == 0,
         "HLAlogicalTimeInterval advanced the ByteWrapper after a failed decode");
   }

   private static byte[] encodeTime(LogicalTime<?, ?> value) throws Exception {
      byte[] encoded = new byte[value.encodedLength()];
      value.encode(encoded, 0);
      return encoded;
   }

   private static byte[] encodeInterval(LogicalTimeInterval<?> value) throws Exception {
      byte[] encoded = new byte[value.encodedLength()];
      value.encode(encoded, 0);
      return encoded;
   }

   private static FederateAmbassador federateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK logical-time data element callbacks";
               }
               if ("hashCode".equals(method.getName())) {
                  return Integer.valueOf(System.identityHashCode(proxy));
               }
               if ("equals".equals(method.getName())) {
                  return Boolean.valueOf(proxy == (arguments == null ? null : arguments[0]));
               }
            }
            return null;
         });
   }

   private static void expectDecoderFailure(DecoderOperation operation, String message)
         throws Exception {
      try {
         operation.run();
      } catch (DecoderException expected) {
         return;
      }
      throw new AssertionError(message);
   }

   private static void check(boolean condition, String message) {
      if (!condition) throw new AssertionError(message);
   }

   @FunctionalInterface
   private interface DecoderOperation {
      Object run() throws Exception;
   }
}
