package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.RtiFactoryFactory;
import hla.rti1516_2025.encoding.ByteWrapper;
import hla.rti1516_2025.encoding.DataElement;
import hla.rti1516_2025.encoding.DataElementFactory;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAextendableVariantRecord;
import hla.rti1516_2025.encoding.HLAfixedArray;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAvariableArray;
import hla.rti1516_2025.encoding.HLAvariantRecord;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.LinkedHashSet;
import java.util.ServiceLoader;
import java.util.Set;

/**
 * Verifies the standard 2025 Java product boundary without claiming RTI
 * service conformance.
 *
 * <p>The check intentionally exercises discovery and declared method shape:
 * the API JAR owns the standard classes, this JAR owns provider registrations,
 * and the native-backed ambassador can be constructed.  The optional
 * reflection checks keep the repository's small compile fixture usable while
 * requiring the complete factory surface when run with the official API.</p>
 */
public final class StandardSurfaceSmokeTest {
   private StandardSurfaceSmokeTest() {
   }

   public static void main(String[] arguments) throws Exception {
      if (!"hla.rti1516_2025".equals(RTIambassador.class.getPackageName())) {
         throw new AssertionError("unexpected IEEE 1516.1-2025 API package");
      }

      Set<String> serviceProviders = new LinkedHashSet<>();
      for (RtiFactory provider : ServiceLoader.load(RtiFactory.class)) {
         serviceProviders.add(provider.rtiName());
      }
      if (!serviceProviders.contains(NativeRtiFactory.NAME)) {
         throw new AssertionError("RtiFactory ServiceLoader did not register Umbra");
      }

      RtiFactory factory = RtiFactoryFactory.getRtiFactory(NativeRtiFactory.NAME);
      if (!NativeRtiFactory.NAME.equals(factory.rtiName())) {
         throw new AssertionError("unexpected 2025 JNI factory name");
      }
      EncoderFactory encoderFactory = factory.getEncoderFactory();
      if (encoderFactory == null) {
         throw new AssertionError("registered 2025 factory returned no EncoderFactory");
      }
      verifyNativeDataElementCarriers(encoderFactory);
      verifyAvailableRtiFactories(factory);
      verifyLogicalTimeFactories();
      verifyAuthorizerFactory();

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
      System.out.println("Umbra JNI 2025 standard surface smoke: OK");
   }

   private static final String[] NATIVE_SCALAR_KINDS = {
      "HLAASCIIchar", "HLAASCIIstring", "HLAboolean", "HLAbyte",
      "HLAfloat32BE", "HLAfloat32LE", "HLAfloat64BE", "HLAfloat64LE",
      "HLAinteger16BE", "HLAinteger16LE", "HLAinteger32BE", "HLAinteger32LE",
      "HLAinteger64BE", "HLAinteger64LE", "HLAunsignedInteger16BE",
      "HLAunsignedInteger16LE", "HLAunsignedInteger32BE", "HLAunsignedInteger32LE",
      "HLAunsignedInteger64BE", "HLAunsignedInteger64LE", "HLAoctet",
      "HLAoctetPairBE", "HLAoctetPairLE", "HLAopaqueData", "HLAunicodeChar",
      "HLAunicodeString"
   };

   private static void verifyNativeDataElementCarriers(EncoderFactory encoder)
      throws Exception {
      // Keep the list tied to the standard creator names rather than to the
      // concrete JNI classes.  The same helper can therefore be transplanted
      // to another provider JAR while preserving the Java cursor contract.
      for (String kind : NATIVE_SCALAR_KINDS) {
         DataElement source = invokeScalarCreator(
            encoder, "create" + kind, scalarArgument(kind), true);
         DataElement decoded = invokeScalarCreator(
            encoder, "create" + kind, null, false);
         verifyNativeCarrier(source, decoded, kind);
      }

      DataElement first = encoder.createHLAinteger32BE(17);
      DataElement second = encoder.createHLAinteger32BE(-23);

      HLAfixedRecord record = encoder.createHLAfixedRecord();
      record.add(first);
      record.add(second);
      HLAfixedRecord decodedRecord = encoder.createHLAfixedRecord();
      decodedRecord.add(encoder.createHLAinteger32BE());
      decodedRecord.add(encoder.createHLAinteger32BE());
      verifyNativeCarrier(record, decodedRecord, "HLAfixedRecord");

      HLAfixedArray<DataElement> fixed = encoder.createHLAfixedArray(first, second);
      HLAfixedArray<DataElement> decodedFixed = encoder.createHLAfixedArray(
         encoder.createHLAinteger32BE(), encoder.createHLAinteger32BE());
      verifyNativeCarrier(fixed, decodedFixed, "HLAfixedArray");

      DataElementFactory<DataElement> elementFactory =
         new DataElementFactory<DataElement>() {
            @Override public DataElement createElement(int index) {
               return encoder.createHLAinteger32BE();
            }
         };
      HLAvariableArray<DataElement> variable = encoder.createHLAvariableArray(
         elementFactory, first, second);
      HLAvariableArray<DataElement> decodedVariable = encoder.createHLAvariableArray(
         elementFactory);
      verifyNativeCarrier(variable, decodedVariable, "HLAvariableArray");

      HLAvariantRecord<DataElement> variant = encoder.createHLAvariantRecord(
         encoder.createHLAoctet());
      variant.setVariant(encoder.createHLAoctet((byte) 1), first);
      HLAvariantRecord<DataElement> decodedVariant = encoder.createHLAvariantRecord(
         encoder.createHLAoctet());
      decodedVariant.setVariant(
         encoder.createHLAoctet((byte) 1), encoder.createHLAinteger32BE());
      verifyNativeCarrier(variant, decodedVariant, "HLAvariantRecord");

      HLAextendableVariantRecord<DataElement> extendable =
         encoder.createHLAextendableVariantRecord(encoder.createHLAoctet());
      extendable.setVariant(encoder.createHLAoctet((byte) 1), first);
      HLAextendableVariantRecord<DataElement> decodedExtendable =
         encoder.createHLAextendableVariantRecord(encoder.createHLAoctet());
      decodedExtendable.setVariant(
         encoder.createHLAoctet((byte) 1), encoder.createHLAinteger32BE());
      verifyNativeCarrier(
         extendable, decodedExtendable, "HLAextendableVariantRecord");
   }

   private static Object scalarArgument(String kind) {
      if (kind.equals("HLAASCIIstring")) return "A!?";
      if (kind.equals("HLAunicodeString")) return "AΩ";
      if (kind.equals("HLAopaqueData")) {
         return new byte[] { 0, 1, (byte) 0xfe, 0x7f };
      }
      if (kind.equals("HLAboolean")) return Boolean.TRUE;
      if (kind.startsWith("HLAfloat32")) return Float.valueOf(1.25f);
      if (kind.startsWith("HLAfloat64")) return Double.valueOf(-2.5d);
      if (kind.startsWith("HLAinteger16") || kind.startsWith("HLAunsignedInteger16") ||
          kind.equals("HLAunicodeChar") || kind.startsWith("HLAoctetPair")) {
         return Short.valueOf((short) 0x1234);
      }
      if (kind.startsWith("HLAinteger32") || kind.startsWith("HLAunsignedInteger32")) {
         return Integer.valueOf(0x12345678);
      }
      if (kind.startsWith("HLAinteger64") || kind.startsWith("HLAunsignedInteger64")) {
         return Long.valueOf(0x0123456789ABCDEFL);
      }
      return Byte.valueOf((byte) 0x41);
   }

   private static boolean accepts(Class<?> parameter, Object argument) {
      if (argument == null) return !parameter.isPrimitive();
      if (!parameter.isPrimitive()) return parameter.isInstance(argument);
      if (parameter == boolean.class) return argument instanceof Boolean;
      if (parameter == char.class) return argument instanceof Character;
      return argument instanceof Number;
   }

   private static DataElement invokeScalarCreator(
      EncoderFactory encoder, String name, Object argument, boolean withArgument)
      throws Exception {
      int arity = withArgument ? 1 : 0;
      for (Method method : EncoderFactory.class.getMethods()) {
         if (!method.getName().equals(name) ||
             method.getParameterTypes().length != arity) {
            continue;
         }
         if (withArgument && !accepts(method.getParameterTypes()[0], argument)) {
            continue;
         }
         try {
            Object result = withArgument
               ? method.invoke(encoder, argument)
               : method.invoke(encoder);
            return (DataElement) result;
         } catch (IllegalArgumentException incompatible) {
            // Continue to the next reflected overload if a compatibility
            // fixture uses a boxed primitive signature.
         }
      }
      throw new NoSuchMethodException(name + "(" + arity + ")");
   }

   private static void verifyNativeCarrier(
      DataElement source, DataElement decoded, String label) throws Exception {
      byte[] encoded = source.toByteArray();
      require(source.getEncodedLength() == encoded.length,
         label + " encoded length");

      byte[] destination = withOffset(encoded);
      ByteWrapper writer = new ByteWrapper(destination, 1, encoded.length + 1);
      source.encode(writer);
      require(writer.getPos() == encoded.length + 1,
         label + " encode cursor");
      require(writer.remaining() == 1, label + " encode suffix");
      require(Arrays.equals(
         encoded, Arrays.copyOfRange(destination, 1, encoded.length + 1)),
         label + " encode bytes");
      require((destination[writer.getPos()] & 0xff) == 0xa5,
         label + " encode sentinel");

      ByteWrapper reader = new ByteWrapper(destination, 1, encoded.length + 1);
      decoded.decode(reader);
      require(Arrays.equals(encoded, decoded.toByteArray()),
         label + " decode bytes");
      require(reader.getPos() == encoded.length + 1,
         label + " decode cursor");
      require(reader.remaining() == 1, label + " decode suffix");
      require((destination[reader.getPos()] & 0xff) == 0xa5,
         label + " decode sentinel");
   }

   private static byte[] withOffset(byte[] value) {
      byte[] result = new byte[value.length + 2];
      result[0] = (byte) 0x55;
      System.arraycopy(value, 0, result, 1, value.length);
      result[result.length - 1] = (byte) 0xa5;
      return result;
   }

   private static void require(boolean condition, String message) {
      if (!condition) throw new AssertionError(message);
   }

   private static void verifyAvailableRtiFactories(RtiFactory expected) throws Exception {
      try {
         Method available = RtiFactoryFactory.class.getMethod("getAvailableRtiFactories");
         Object result = available.invoke(null);
         if (!(result instanceof Iterable)) {
            throw new AssertionError("RtiFactoryFactory returned a non-iterable provider set");
         }
         boolean found = false;
         for (Object candidate : (Iterable<?>) result) {
            if (candidate == expected ||
                (candidate instanceof RtiFactory &&
                 NativeRtiFactory.NAME.equals(((RtiFactory) candidate).rtiName()))) {
               found = true;
               break;
            }
         }
         if (!found) throw new AssertionError("available 2025 RtiFactory set omitted Umbra");
      } catch (NoSuchMethodException compatibilityFixture) {
         // The repository compile fixture predates this optional convenience method.
      }
   }

   private static void verifyLogicalTimeFactories() throws Exception {
      Class<?> factoryFactory;
      try {
         factoryFactory = Class.forName(
            "hla.rti1516_2025.time.LogicalTimeFactoryFactory");
      } catch (ClassNotFoundException compatibilityFixture) {
         // The small repository fixture has the time interfaces but not the
         // official discovery helper; the product verifier checks it with the
         // official API JAR.
         return;
      }

      Method available = factoryFactory.getMethod("getAvailableLogicalTimeFactories");
      Object result = available.invoke(null);
      Set<String> names = new LinkedHashSet<>();
      for (Object provider : (Iterable<?>) result) {
         Method getName = provider.getClass().getMethod("getName");
         names.add((String) getName.invoke(provider));
      }
      requireTimeFactory(factoryFactory, names, "HLAinteger64Time");
      requireTimeFactory(factoryFactory, names, "HLAfloat64Time");
   }

   private static void requireTimeFactory(
      Class<?> factoryFactory, Set<String> available, String name) throws Exception {
      if (!available.contains(name)) {
         throw new AssertionError("logical-time ServiceLoader omitted " + name);
      }
      Object provider = factoryFactory.getMethod(
         "getLogicalTimeFactory", String.class).invoke(null, name);
      if (provider == null) throw new AssertionError("logical-time lookup returned null: " + name);
      if (!provider.getClass().getName().contains("Native")) {
         throw new AssertionError("logical-time lookup returned an unexpected provider: " + provider);
      }
      boolean integer = "HLAinteger64Time".equals(name);
      Class<?> numericType = integer ? long.class : double.class;
      Object value;
      Object intervalValue;
      if (integer) {
         value = Long.valueOf(123456789L);
         intervalValue = Long.valueOf(1234L);
      } else {
         value = Double.valueOf(12.5D);
         intervalValue = Double.valueOf(0.25D);
      }
      Object time = provider.getClass().getMethod("makeTime", numericType)
         .invoke(provider, value);
      Object interval = provider.getClass().getMethod("makeInterval", numericType)
         .invoke(provider, intervalValue);
      if (time == null || interval == null) {
         throw new AssertionError("logical-time provider did not create standard values: " + name);
      }
      byte[] encoded = new byte[Long.BYTES + 1];
      ByteBuffer buffer = ByteBuffer.wrap(encoded, 1, Long.BYTES);
      if (integer) buffer.putLong(((Long) value).longValue());
      else buffer.putDouble(((Double) value).doubleValue());
      Object decoded = provider.getClass().getMethod(
         "decodeTime", byte[].class, int.class).invoke(provider, encoded, 1);
      if (decoded == null) throw new AssertionError("logical-time decode returned null: " + name);
   }

   private static void verifyAuthorizerFactory() throws Exception {
      Class<?> factoryFactory;
      try {
         factoryFactory = Class.forName(
            "hla.rti1516_2025.auth.AuthorizerFactoryFactory");
      } catch (ClassNotFoundException compatibilityFixture) {
         return;
      }
      Object provider = factoryFactory.getMethod(
         "getAuthorizerFactory", String.class).invoke(null, NativeAuthorizerFactory.NAME);
      if (provider == null) throw new AssertionError("authorizer ServiceLoader omitted Umbra");
   }
}
