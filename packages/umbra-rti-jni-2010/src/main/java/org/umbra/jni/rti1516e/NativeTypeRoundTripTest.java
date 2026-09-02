package org.umbra.jni.rti1516e;

import hla.rti1516e.CallbackModel;
import hla.rti1516e.AttributeHandleSet;
import hla.rti1516e.AttributeRegionAssociation;
import hla.rti1516e.FederateAmbassador;
import hla.rti1516e.FederateHandleSaveStatusPair;
import hla.rti1516e.FederateRestoreStatus;
import hla.rti1516e.FederationExecutionInformation;
import hla.rti1516e.LogicalTime;
import hla.rti1516e.MessageRetractionReturn;
import hla.rti1516e.OrderType;
import hla.rti1516e.RangeBounds;
import hla.rti1516e.RegionHandleSet;
import hla.rti1516e.RestoreStatus;
import hla.rti1516e.SaveStatus;
import hla.rti1516e.TimeQueryReturn;
import hla.rti1516e.encoding.ByteWrapper;
import hla.rti1516e.encoding.DataElement;
import hla.rti1516e.encoding.DataElementFactory;
import hla.rti1516e.encoding.DecoderException;
import hla.rti1516e.encoding.EncoderFactory;
import hla.rti1516e.encoding.EncoderException;
import hla.rti1516e.encoding.HLAfixedArray;
import hla.rti1516e.encoding.HLAfixedRecord;
import hla.rti1516e.encoding.HLAvariableArray;
import hla.rti1516e.encoding.HLAvariantRecord;
import hla.rti1516e.exceptions.CouldNotDecode;
import hla.rti1516e.exceptions.RTIinternalError;
import hla.rti1516e.exceptions.RTIexception;
import java.util.Arrays;
import java.lang.reflect.Method;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.Set;

/** Executable Java-side evidence for the 2010 JNI type transport matrix. */
public final class NativeTypeRoundTripTest {
   private NativeTypeRoundTripTest() {
   }

   private static void require(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }

   private static void requireBytes(byte[] expected, byte[] actual, String label) {
      require(Arrays.equals(expected, actual), label + " changed across JNI");
   }

   private static byte[] longEncoding(long value) {
      return ByteBuffer.allocate(Long.BYTES).putLong(value).array();
   }

   private static byte[] doubleEncoding(double value) {
      return ByteBuffer.allocate(Long.BYTES)
         .putLong(Double.doubleToRawLongBits(value)).array();
   }

   private static byte[] withOffset(byte[] value) {
      byte[] result = new byte[value.length + 2];
      result[0] = (byte) 0x5a;
      System.arraycopy(value, 0, result, 1, value.length);
      result[result.length - 1] = (byte) 0xa5;
      return result;
   }

   private static boolean accepts(Class<?> parameter, Object argument) {
      if (argument == null) return !parameter.isPrimitive();
      if (!parameter.isPrimitive()) return parameter.isInstance(argument);
      if (parameter == boolean.class) return argument instanceof Boolean;
      if (parameter == char.class) return argument instanceof Character;
      return argument instanceof Number;
   }

   private static Object coerce(Class<?> parameter, Object argument) {
      if (argument == null || !parameter.isPrimitive()) return argument;
      if (parameter == byte.class) return ((Number) argument).byteValue();
      if (parameter == short.class) return ((Number) argument).shortValue();
      if (parameter == int.class) return ((Number) argument).intValue();
      if (parameter == long.class) return ((Number) argument).longValue();
      if (parameter == float.class) return ((Number) argument).floatValue();
      if (parameter == double.class) return ((Number) argument).doubleValue();
      return argument;
   }

   private static void requireTimeCarrier(
      Object carrier, byte[] expectedEncoding, Object expectedValue, String label)
      throws Exception {
      require(((Number) invoke(carrier, "encodedLength")).intValue() == expectedEncoding.length,
         label + " encodedLength");
      byte[] destination = withOffset(expectedEncoding);
      invoke(carrier, "encode", destination, 1);
      requireBytes(expectedEncoding,
         Arrays.copyOfRange(destination, 1, 1 + expectedEncoding.length), label + " encode");
      Object actualValue = invoke(carrier, "getValue");
      if (expectedValue instanceof Long) {
         require(((Number) actualValue).longValue() == ((Long) expectedValue).longValue(),
            label + " value");
      } else {
         require(Double.compare(((Number) actualValue).doubleValue(),
            ((Double) expectedValue).doubleValue()) == 0, label + " value");
      }
   }

   private static Object invoke(Object carrier, String name, Object... arguments)
      throws Exception {
      for (Method method : carrier.getClass().getMethods()) {
         if (method.getName().equals(name) &&
             method.getParameterTypes().length == arguments.length) {
            Class<?>[] parameters = method.getParameterTypes();
            boolean compatible = true;
            for (int index = 0; index < parameters.length; ++index) {
               if (!accepts(parameters[index], arguments[index])) {
                  compatible = false;
                  break;
               }
            }
            if (!compatible) continue;
            Object[] converted = arguments.clone();
            for (int index = 0; index < parameters.length; ++index) {
               converted[index] = coerce(parameters[index], converted[index]);
            }
            return method.invoke(carrier, converted);
         }
      }
      throw new NoSuchMethodException(carrier.getClass().getName() + "." + name);
   }

   private static boolean scalar(String kind) {
      return kind.startsWith("HLAinteger") || kind.startsWith("HLAfloat") ||
         kind.equals("HLAbyte") || kind.equals("HLAoctet") ||
         kind.equals("HLAASCIIchar") || kind.equals("HLAunicodeChar") ||
         kind.startsWith("HLAoctetPair") || kind.equals("HLAboolean") ||
         kind.equals("HLAASCIIstring") || kind.equals("HLAunicodeString");
   }

   private static void requireEncoderFactorySurface() {
      Map<String, Set<Integer>> actual = new LinkedHashMap<>();
      for (Method method : EncoderFactory.class.getMethods()) {
         if (!method.getName().startsWith("createHLA")) continue;
         actual.computeIfAbsent(method.getName(), ignored -> new LinkedHashSet<>())
            .add(method.getParameterTypes().length);
      }

      Map<String, Set<Integer>> expected = new LinkedHashMap<>();
      String[] scalarNames = {
         "createHLAASCIIchar", "createHLAASCIIstring", "createHLAboolean",
         "createHLAbyte", "createHLAfloat32BE", "createHLAfloat32LE",
         "createHLAfloat64BE", "createHLAfloat64LE", "createHLAinteger16BE",
         "createHLAinteger16LE", "createHLAinteger32BE", "createHLAinteger32LE",
         "createHLAinteger64BE", "createHLAinteger64LE", "createHLAoctet",
         "createHLAoctetPairBE", "createHLAoctetPairLE", "createHLAopaqueData",
         "createHLAunicodeChar", "createHLAunicodeString"
      };
      for (String name : scalarNames) {
         expected.put(name, new LinkedHashSet<>(Arrays.asList(0, 1)));
      }
      expected.put("createHLAfixedArray", new LinkedHashSet<>(Arrays.asList(1, 2)));
      expected.put("createHLAfixedRecord", new LinkedHashSet<>(Arrays.asList(0)));
      expected.put("createHLAvariableArray", new LinkedHashSet<>(Arrays.asList(2)));
      expected.put("createHLAvariantRecord", new LinkedHashSet<>(Arrays.asList(1)));
      require(actual.equals(expected), "2010 EncoderFactory creator signatures changed: " + actual);

      EncoderFactory carrier = NativeTypeRoundTrip.encoderFactoryCarrier();
      for (Method method : EncoderFactory.class.getMethods()) {
         if (!method.getName().startsWith("createHLA") || method.getParameterTypes().length != 0) {
            continue;
         }
         try {
            require(method.invoke(carrier) instanceof DataElement,
               method.getName() + " no-argument carrier");
         } catch (Exception error) {
            throw new AssertionError(method.getName() + " no-argument carrier failed", error);
         }
      }
   }

   private static void requireEncoderFactoryCompositeArguments(
      EncoderFactory encoderFactory) throws Exception {
      @SuppressWarnings("unchecked")
      DataElementFactory<DataElement> elementFactory =
         (DataElementFactory<DataElement>) (DataElementFactory<?>)
            NativeTypeRoundTrip.dataElementFactoryCarrier("HLAinteger32BE");
      DataElement first = encoderFactory.createHLAinteger32BE(17);
      DataElement second = encoderFactory.createHLAinteger32BE(-23);
      HLAfixedArray<DataElement> fixedFromValues =
         encoderFactory.createHLAfixedArray(first, second);
      require(fixedFromValues.size() == 2, "EncoderFactory fixed-array varargs size");
      require(((Number) invoke(fixedFromValues.get(0), "getValue")).intValue() == 17,
         "EncoderFactory fixed-array first value");
      require(((Number) invoke(fixedFromValues.get(1), "getValue")).intValue() == -23,
         "EncoderFactory fixed-array second value");
      HLAfixedArray<DataElement> fixedFromFactory =
         encoderFactory.createHLAfixedArray(elementFactory, 3);
      require(fixedFromFactory.size() == 3, "EncoderFactory fixed-array factory size");
      HLAvariableArray<DataElement> variable =
         encoderFactory.createHLAvariableArray(elementFactory, first, second);
      require(variable.size() == 2, "EncoderFactory variable-array varargs size");
      require(((Number) invoke(variable.get(0), "getValue")).intValue() == 17,
         "EncoderFactory variable-array first value");
      require(((Number) invoke(variable.get(1), "getValue")).intValue() == -23,
         "EncoderFactory variable-array second value");
      HLAvariableArray<DataElement> emptyVariable =
         encoderFactory.createHLAvariableArray(elementFactory);
      require(emptyVariable.size() == 0, "EncoderFactory empty variable-array size");
      HLAfixedRecord record = encoderFactory.createHLAfixedRecord();
      record.add(first);
      record.add(second);
      require(record.size() == 2, "EncoderFactory fixed-record size");
      HLAvariantRecord<DataElement> variant = encoderFactory.createHLAvariantRecord(first);
      require(((Number) invoke(variant.getDiscriminant(), "getValue")).intValue() == 17,
         "EncoderFactory variant discriminant");
      variant.setVariant(first, second);
      require(((Number) invoke(variant.getValue(), "getValue")).intValue() == -23,
         "EncoderFactory variant value");
   }

   private static void requireVendorDataElementCarrier() throws Exception {
      DataElement carrier = NativeTypeRoundTrip.vendorDataElementCarrier();
      requireBytes(new byte[] { 'a', 'b', 'c', 'd' }, carrier.toByteArray(),
         "vendor DataElement initial payload");
      ByteWrapper window = new ByteWrapper(
         new byte[] { '!', 'w', 'x', 'y', 'z', '?' }, 1, 5);
      carrier.decode(window);
      requireBytes(new byte[] { 'w', 'x', 'y', 'z' }, carrier.toByteArray(),
         "vendor DataElement ByteWrapper decode");
      require(window.getPos() == 5, "vendor DataElement ByteWrapper position");
      require(window.remaining() == 1, "vendor DataElement ByteWrapper suffix");
      try {
         carrier.decode(new byte[] { 'b', 'a', 'd' });
         throw new AssertionError("vendor DataElement accepted malformed payload");
      } catch (DecoderException expected) {
         // Provider-selected malformed behavior remains a typed Java error.
      }
      Object returned = NativeTypeRoundTrip.roundTripObject(carrier);
      require(returned.getClass() == carrier.getClass(),
         "vendor DataElement implementation changed across JNI");
   }

   public static void main(String[] arguments) throws Exception {
      require(NativeTypeRoundTrip.roundTripByte((byte) -7) == (byte) -7, "byte");
      require(NativeTypeRoundTrip.roundTripShort((short) -1234) == (short) -1234, "short");
      require(NativeTypeRoundTrip.roundTripInt(-123456789) == -123456789, "int");
      require(NativeTypeRoundTrip.roundTripLong(-123456789012345L) == -123456789012345L, "long");
      require(Float.floatToIntBits(NativeTypeRoundTrip.roundTripFloat(1.25f)) ==
         Float.floatToIntBits(1.25f), "float");
      require(Double.doubleToLongBits(NativeTypeRoundTrip.roundTripDouble(-2.5)) ==
         Double.doubleToLongBits(-2.5), "double");
      require(NativeTypeRoundTrip.roundTripBoolean(true), "boolean");
      require("JNI Ω 🚀".equals(NativeTypeRoundTrip.roundTripString("JNI Ω 🚀")), "string");
      requireBytes(new byte[] { 0, 1, -2, 127 },
         NativeTypeRoundTrip.roundTripBytes(new byte[] { 0, 1, -2, 127 }), "bytes");
      requireBytes(new byte[] { 0, 1, -2, 127 },
         NativeTypeRoundTrip.roundTripByteWrapper(new byte[] { 0, 1, -2, 127 }), "ByteWrapper");

      for (String name : NativeTypeRoundTrip.standardExceptionNames()) {
                String message = "jni-2010:" + name + ": Ω 🚀\u0000";
         try {
            NativeTypeRoundTrip.throwStandardException(name, message);
            throw new AssertionError(name + " was not thrown");
         } catch (RTIexception error) {
            require(error.getClass().getSimpleName().equals(name),
               name + " Java exception identity");
            require(message.equals(error.getMessage()), name + " Java exception message");
            require(error.getCause() == null, name + " unexpected native cause");
         }
      }
      for (String name : new String[] { "EncoderException", "DecoderException" }) {
            String message = "jni-2010:" + name + ": Ω 🚀\u0000";
         try {
            NativeTypeRoundTrip.throwStandardException(name, message);
            throw new AssertionError(name + " was not thrown");
         } catch (Exception error) {
            require(error.getClass().getSimpleName().equals(name),
               name + " Java encoding exception identity");
            require(message.equals(error.getMessage()), name + " Java encoding exception message");
            require(error.getCause() == null, name + " unexpected encoding cause");
         }
      }

      for (String kind : NativeTypeRoundTrip.dataElementKinds()) {
         byte[] seed = NativeTypeRoundTrip.seedDataElement(kind);
         require(seed != null && seed.length > 0, kind + " has no C++ seed encoding");
         requireBytes(seed, NativeTypeRoundTrip.roundTripDataElement(kind, seed), kind);
         DataElement carrier = NativeTypeRoundTrip.dataElementCarrier(kind);
         requireBytes(seed, carrier.toByteArray(), kind + " standard carrier");
         DataElement decodedCarrier = NativeTypeRoundTrip.dataElementCarrier(kind);
         decodedCarrier.decode(seed);
         requireBytes(seed, decodedCarrier.toByteArray(), kind + " byte-array decode");
         // Exercise the reverse cursor direction through the exact standard
         // Java overload as well.  Keep a non-zero offset and a trailing
         // sentinel so the native carrier must advance precisely one value
         // without touching caller-owned bytes outside the active window.
         byte[] encodeWindow = withOffset(seed);
         ByteWrapper wrapper = new ByteWrapper(
            encodeWindow, 1, seed.length + 1);
         carrier.encode(wrapper);
         requireBytes(seed,
            Arrays.copyOfRange(wrapper.array(), 1, seed.length + 1),
            kind + " ByteWrapper carrier");
         require(wrapper.getPos() == seed.length + 1,
            kind + " ByteWrapper encode consumed length");
         require(wrapper.remaining() == 1,
            kind + " ByteWrapper encode trailing window");
         require((wrapper.array()[wrapper.getPos()] & 0xff) == 0xa5,
            kind + " ByteWrapper encode trailing sentinel");
         byte[] decodeWindow = withOffset(seed);
         ByteWrapper decodeWrapper = new ByteWrapper(
            decodeWindow, 1, seed.length + 1);
         DataElement windowCarrier = NativeTypeRoundTrip.dataElementCarrier(kind);
         windowCarrier.decode(decodeWrapper);
         requireBytes(seed, windowCarrier.toByteArray(),
            kind + " ByteWrapper decode");
         require(decodeWrapper.getPos() == seed.length + 1,
            kind + " ByteWrapper consumed length");
         require(decodeWrapper.remaining() == 1,
            kind + " ByteWrapper trailing window");
         require((decodeWrapper.array()[decodeWrapper.getPos()] & 0xff) == 0xa5,
            kind + " ByteWrapper trailing sentinel");
         if (scalar(kind)) {
            Object value = invoke(carrier, "getValue");
            require(value != null, kind + " typed getValue");
            invoke(carrier, "setValue", value);
            requireBytes(seed, carrier.toByteArray(), kind + " typed setValue");
         } else if ("HLAopaqueData".equals(kind)) {
            byte[] value = (byte[]) invoke(carrier, "getValue");
            require(value.length > 0, kind + " typed payload");
            require(((Number) invoke(carrier, "size")).intValue() == value.length,
               kind + " typed size");
            require(((Byte) invoke(carrier, "get", 0)).byteValue() == value[0],
               kind + " typed get");
            invoke(carrier, "setValue", value);
            requireBytes(seed, carrier.toByteArray(), kind + " typed setValue");
         } else if ("HLAvariantRecord".equals(kind)) {
            require(invoke(carrier, "getDiscriminant") != null,
               kind + " typed discriminant");
            require(invoke(carrier, "getValue") != null, kind + " typed variant value");
         } else {
            require(((Number) invoke(carrier, "size")).intValue() > 0,
               kind + " typed composite size");
            require(invoke(carrier, "get", 0) != null, kind + " typed composite get");
         }
      }
      requireEncoderFactorySurface();
      EncoderFactory encoderFactory = NativeTypeRoundTrip.encoderFactoryCarrier();
      requireBytes(new byte[] {0, 0, 0, 7},
         encoderFactory.createHLAinteger32BE(7).toByteArray(),
         "EncoderFactory HLAinteger32BE");
      requireBytes(new byte[] {0, 0, 0, 4, 0, 1, 2, 3},
         encoderFactory.createHLAopaqueData(new byte[] {0, 1, 2, 3}).toByteArray(),
         "EncoderFactory HLAopaqueData");
      for (String elementKind : NativeTypeRoundTrip.dataElementFactoryKinds()) {
         DataElement element = NativeTypeRoundTrip.dataElementFactoryCarrier(elementKind).createElement(0);
         require(element != null, elementKind + " DataElementFactory carrier");
         requireBytes(NativeTypeRoundTrip.seedDataElement(elementKind), element.toByteArray(),
            elementKind + " DataElementFactory encoding");
      }
      requireEncoderFactoryCompositeArguments(encoderFactory);
      requireVendorDataElementCarrier();

      for (String kind : NativeTypeRoundTrip.handleKinds()) {
         byte[] seed = NativeTypeRoundTrip.seedHandle(kind, 0x0123456789ABCDEFL);
         requireBytes(seed, NativeTypeRoundTrip.roundTripHandle(kind, seed), kind);
         byte[][] collection = NativeTypeRoundTrip.roundTripHandleCollection(
            kind, new byte[][] { seed, NativeTypeRoundTrip.seedHandle(kind, 7L) });
         require(collection.length == 2, kind + " collection length");
         requireBytes(seed, collection[0], kind + " collection[0]");
         NativeHandleCarrier carrier = NativeTypeRoundTrip.handleCarrier(kind, 7L);
         require(carrier.encodedLength() == carrier.encodedValue().length,
            kind + " standard handle carrier length");
         requireBytes(carrier.encodedValue(),
            NativeTypeRoundTrip.roundTripHandleCarrier(kind, carrier).encodedValue(),
            kind + " standard handle carrier");
      }
      for (String kind : NativeTypeRoundTrip.javaOnlyHandleKinds()) {
         NativeHandleCarrier carrier = NativeTypeRoundTrip.handleCarrier(kind, 7L);
         requireBytes(carrier.encodedValue(),
            NativeTypeRoundTrip.roundTripHandleCarrier(kind, carrier).encodedValue(),
            kind + " Java-only standard handle carrier");
      }

      for (String kind : NativeTypeRoundTrip.logicalTimeKinds()) {
         byte[] seed = NativeTypeRoundTrip.seedLogicalTime(kind);
         require(seed != null && seed.length > 0, kind + " has no C++ seed encoding");
         requireBytes(seed, NativeTypeRoundTrip.roundTripLogicalTime(kind, seed), kind);
         try {
            NativeTypeRoundTrip.roundTripLogicalTime(kind, new byte[Long.BYTES - 1]);
            throw new AssertionError(kind + " accepted a short encoding");
         } catch (RTIexception expectedError) {
            require(expectedError.getClass() == CouldNotDecode.class,
               kind + " malformed error identity");
            require(expectedError.getMessage() != null, kind + " malformed error message");
         }
         Object carrier = NativeTypeRoundTrip.logicalTimeCarrier(kind);
         Object expected = "HLAinteger64Time".equals(kind) ? 123456789L :
            "HLAinteger64Interval".equals(kind) ? 1234L :
            "HLAfloat64Time".equals(kind) ? 12.5D : 0.25D;
         requireTimeCarrier(carrier, seed, expected, kind + " standard carrier");
         require(carrier.toString().contains(kind), kind + " standard time carrier");
      }
      for (String factoryKind : NativeTypeRoundTrip.logicalTimeFactoryKinds()) {
         Object factory = NativeTypeRoundTrip.logicalTimeFactoryCarrier(factoryKind);
         String timeKind = factoryKind.substring(0, factoryKind.length() - "Factory".length());
         boolean integer = timeKind.startsWith("HLAinteger64");
         require(timeKind.equals(invoke(factory, "getName")), factoryKind + " name");
         Object initial = invoke(factory, "makeInitial");
         Object finalValue = invoke(factory, "makeFinal");
         Object zero = invoke(factory, "makeZero");
         Object epsilon = invoke(factory, "makeEpsilon");
         Object timeValue = integer ? 123456789L : 12.5D;
         Object intervalValue = integer ? 1234L : 0.25D;
         Object time = invoke(factory, "makeTime", timeValue);
         Object interval = invoke(factory, "makeInterval", intervalValue);
         require(initial != null && finalValue != null && zero != null && epsilon != null &&
            time != null && interval != null, factoryKind + " constructors");
         require((Boolean) invoke(initial, "isInitial"), factoryKind + " initial flag");
         require((Boolean) invoke(finalValue, "isFinal"), factoryKind + " final flag");
         require((Boolean) invoke(zero, "isZero"), factoryKind + " zero flag");
         require((Boolean) invoke(epsilon, "isEpsilon"), factoryKind + " epsilon flag");
         requireTimeCarrier(initial, integer ? longEncoding(0L) : doubleEncoding(0.0D),
            integer ? 0L : 0.0D, factoryKind + " initial");
         requireTimeCarrier(finalValue,
            integer ? longEncoding(Long.MAX_VALUE) : doubleEncoding(Double.MAX_VALUE),
            integer ? Long.MAX_VALUE : Double.MAX_VALUE, factoryKind + " final");
         requireTimeCarrier(zero, integer ? longEncoding(0L) : doubleEncoding(0.0D),
            integer ? 0L : 0.0D, factoryKind + " zero");
         requireTimeCarrier(epsilon, integer ? longEncoding(1L) : doubleEncoding(Double.longBitsToDouble(1L)),
            integer ? 1L : Double.longBitsToDouble(1L), factoryKind + " epsilon");
         byte[] timeEncoding = integer ? longEncoding(123456789L) : doubleEncoding(12.5D);
         byte[] intervalEncoding = integer ? longEncoding(1234L) : doubleEncoding(0.25D);
         requireTimeCarrier(time, timeEncoding, timeValue, factoryKind + " makeTime");
         requireTimeCarrier(interval, intervalEncoding, intervalValue, factoryKind + " makeInterval");
         Object decodedTime = invoke(factory, "decodeTime", withOffset(timeEncoding), 1);
         Object decodedInterval = invoke(factory, "decodeInterval", withOffset(intervalEncoding), 1);
         requireTimeCarrier(decodedTime, timeEncoding, timeValue, factoryKind + " decodeTime");
         requireTimeCarrier(decodedInterval, intervalEncoding, intervalValue,
            factoryKind + " decodeInterval");
      }
      for (String kind : NativeTypeRoundTrip.collectionKinds()) {
         Object carrier = NativeTypeRoundTrip.collectionCarrier(kind);
         if (kind.endsWith("Map")) {
            Map<Object, Object> map = castMap(carrier);
            NativeHandleCarrier key = NativeTypeRoundTrip.handleCarrier(
               kind.startsWith("Parameter") ? "ParameterHandle" : "AttributeHandle", 11L);
            map.put(key, new byte[] { 3, 1, 4 });
            Map<?, ?> returned = castMap(NativeTypeRoundTrip.roundTripCollectionCarrier(kind, carrier));
            require(returned.size() == 1, kind + " round-trip map size");
            requireBytes(new byte[] { 3, 1, 4 }, (byte[]) returned.values().iterator().next(),
               kind + " round-trip map value");
            require(invoke(returned, "getValueReference", key) != null,
               kind + " value reference");
         } else if (kind.endsWith("List")) {
            List<Object> list = castList(carrier);
            AttributeHandleSet attributes = (AttributeHandleSet) NativeTypeRoundTrip.collectionCarrier(
               "AttributeHandleSet");
            RegionHandleSet regions = (RegionHandleSet) NativeTypeRoundTrip.collectionCarrier(
               "RegionHandleSet");
            attributes.add(NativeTypeRoundTrip.handleCarrier("AttributeHandle", 1L));
            regions.add(NativeTypeRoundTrip.handleCarrier("RegionHandle", 2L));
            list.add(new AttributeRegionAssociation(attributes, regions));
            List<?> returned = castList(NativeTypeRoundTrip.roundTripCollectionCarrier(kind, carrier));
            require(returned.size() == 1, kind + " round-trip list size");
            require(returned.get(0) instanceof AttributeRegionAssociation,
               kind + " round-trip list record");
            AttributeRegionAssociation association = (AttributeRegionAssociation) returned.get(0);
            require(association.ahset.size() == 1 && association.rhset.size() == 1,
               kind + " round-trip association fields");
         } else {
            Set<Object> set = castSet(carrier);
            if (kind.startsWith("FederationExecution")) {
               set.add(new FederationExecutionInformation("federation", "HLAinteger64Time"));
            } else {
               String handleKind = kind.startsWith("Attribute") ? "AttributeHandle" :
                  kind.startsWith("Dimension") ? "DimensionHandle" :
                  kind.startsWith("Federate") ? "FederateHandle" : "RegionHandle";
               set.add(NativeTypeRoundTrip.handleCarrier(handleKind, 5L));
            }
            Set<?> returned = castSet(NativeTypeRoundTrip.roundTripCollectionCarrier(kind, carrier));
            require(returned.size() == 1, kind + " round-trip set size");
         }
      }
      for (String factoryKind : NativeTypeRoundTrip.collectionFactoryKinds()) {
         Object factory = NativeTypeRoundTrip.collectionFactoryCarrier(factoryKind);
         Object created = factoryKind.endsWith("SetFactory")
            ? invoke(factory, "create") : invoke(factory, "create", 2);
         require(created != null, factoryKind + " create");
      }
      for (String factoryKind : NativeTypeRoundTrip.handleFactoryKinds()) {
         String handleKind = factoryKind.substring(0, factoryKind.length() - "Factory".length());
         Object factory = NativeTypeRoundTrip.handleFactoryCarrier(factoryKind);
         NativeHandleCarrier carrier = NativeTypeRoundTrip.handleCarrier(handleKind, 19L);
         NativeHandleCarrier decoded = (NativeHandleCarrier) invoke(
            factory, "decode", carrier.encodedValue(), 0);
         requireBytes(carrier.encodedValue(), decoded.encodedValue(), factoryKind + " decode");
         if (factoryKind.endsWith("TransportationTypeHandleFactory")) {
            require(invoke(factory, "getHLAdefaultReliable") != null,
               factoryKind + " reliable default");
            require(invoke(factory, "getHLAdefaultBestEffort") != null,
               factoryKind + " best-effort default");
         }
      }

      require(NativeTypeRoundTrip.roundTripEnum(CallbackModel.HLA_EVOKED) ==
         CallbackModel.HLA_EVOKED, "CallbackModel enum");
      require(NativeTypeRoundTrip.roundTripEnum(OrderType.RECEIVE) == OrderType.RECEIVE,
         "OrderType enum");
      for (String enumType : NativeTypeRoundTrip.enumTypes()) {
         Class<?> type = Class.forName("hla.rti1516e." + enumType);
         Object[] values = type.getEnumConstants();
         require(values != null && values.length > 0, enumType + " enum values");
         require(values[0] == NativeTypeRoundTrip.roundTripObject(values[0]),
            enumType + " enum carrier");
      }
      RangeBounds bounds = new RangeBounds(4L, 9L);
      require(bounds.equals(NativeTypeRoundTrip.roundTripObject(bounds)), "RangeBounds record");
      FederationExecutionInformation information =
         new FederationExecutionInformation("federation", "HLAinteger64Time");
      require(information.equals(NativeTypeRoundTrip.roundTripObject(information)),
         "FederationExecutionInformation record");
      NativeHandleCarrier federateHandle = NativeTypeRoundTrip.handleCarrier("FederateHandle", 31L);
      FederateHandleSaveStatusPair savePair = new FederateHandleSaveStatusPair(
         federateHandle, SaveStatus.values()[0]);
      require(savePair == NativeTypeRoundTrip.roundTripObject(savePair),
         "FederateHandleSaveStatusPair record");
      FederateRestoreStatus restoreStatus = new FederateRestoreStatus(
         federateHandle, NativeTypeRoundTrip.handleCarrier("FederateHandle", 32L),
         RestoreStatus.values()[0]);
      require(restoreStatus == NativeTypeRoundTrip.roundTripObject(restoreStatus),
         "FederateRestoreStatus record");
      MessageRetractionReturn retraction = new MessageRetractionReturn(
         true, NativeTypeRoundTrip.handleCarrier("MessageRetractionHandle", 33L));
      require(retraction == NativeTypeRoundTrip.roundTripObject(retraction),
         "MessageRetractionReturn record");
      LogicalTime<?, ?> logicalTime = (LogicalTime<?, ?>) NativeTypeRoundTrip.logicalTimeCarrier(
         "HLAinteger64Time");
      TimeQueryReturn query = new TimeQueryReturn(true, logicalTime);
      require(query == NativeTypeRoundTrip.roundTripObject(query), "TimeQueryReturn record");
      RTIinternalError exception = new RTIinternalError("JNI exception carrier");
      require(exception == NativeTypeRoundTrip.roundTripObject(exception),
         "exception carrier identity");
      EncoderException encoderException = new EncoderException("JNI encoder exception carrier");
      require(encoderException == NativeTypeRoundTrip.roundTripObject(encoderException),
         "EncoderException carrier identity");
      DecoderException decoderException = new DecoderException("JNI decoder exception carrier");
      require(decoderException == NativeTypeRoundTrip.roundTripObject(decoderException),
         "DecoderException carrier identity");
      Object callback = NativeTypeRoundTrip.roundTripObject(
         NativeTypeRoundTrip.callbackCarrier("SupplementalReflectInfo"));
      FederateAmbassador.SupplementalReflectInfo supplemental =
         (FederateAmbassador.SupplementalReflectInfo) callback;
      require(supplemental.hasProducingFederate(), "callback record producing federate");
      require(!supplemental.hasSentRegions(), "callback record sent regions");
      require(NativeTypeRoundTrip.roundTripObject(
         NativeTypeRoundTrip.callbackCarrier("SupplementalReceiveInfo")) != null,
         "callback receive carrier");
      require(NativeTypeRoundTrip.roundTripObject(
         NativeTypeRoundTrip.callbackCarrier("SupplementalRemoveInfo")) != null,
         "callback remove carrier");
      try {
         NativeTypeRoundTrip.roundTripDataElement("HLAinteger32BE", new byte[] { 1 });
         throw new AssertionError("malformed data element did not map to DecoderException");
      } catch (DecoderException expected) {
         require(expected.getMessage() != null, "malformed data element error message");
      }
      for (String kind : NativeTypeRoundTrip.dataElementKinds()) {
         byte[] seed = NativeTypeRoundTrip.seedDataElement(kind);
         require(seed.length > 0, kind + " malformed matrix seed");
         try {
            NativeTypeRoundTrip.roundTripDataElement(
               kind, Arrays.copyOf(seed, seed.length - 1));
            throw new AssertionError(kind + " accepted truncated encoding");
         } catch (DecoderException expected) {
            require(expected.getMessage() != null,
               kind + " truncated error message");
         }
         if (kind.equals("HLAASCIIstring") || kind.equals("HLAunicodeString") ||
            kind.equals("HLAopaqueData") || kind.equals("HLAvariableArray")) {
            byte[] overrun = Arrays.copyOf(seed, seed.length);
            int declared = ByteBuffer.wrap(overrun).getInt(0);
            ByteBuffer.wrap(overrun).putInt(0, declared + 1);
            try {
               NativeTypeRoundTrip.roundTripDataElement(kind, overrun);
               throw new AssertionError(kind + " accepted declared-length overrun");
            } catch (DecoderException expected) {
               require(expected.getMessage() != null,
                  kind + " declared-length error message");
            }
         }
      }

      System.out.println("2010 JNI type round-trip passed: primitives, strings, bytes, " +
         "ByteWrapper, 24 standard data-element carriers and EncoderFactory creators, " +
         "decode overloads, " +
         "malformed-wire matrix, 9 handle families, collections, 4 times, enums, " +
         "records, callback records, and exceptions");
   }

   @SuppressWarnings("unchecked")
   private static Set<Object> castSet(Object value) {
      return (Set<Object>) value;
   }

   @SuppressWarnings("unchecked")
   private static Map<Object, Object> castMap(Object value) {
      return (Map<Object, Object>) value;
   }

   @SuppressWarnings("unchecked")
   private static List<Object> castList(Object value) {
      return (List<Object>) value;
   }
}
