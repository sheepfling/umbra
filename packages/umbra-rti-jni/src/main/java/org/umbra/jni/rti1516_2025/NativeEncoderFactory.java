package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.DataElement;
import hla.rti1516_2025.encoding.DataElementFactory;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

/** Native-backed encoder factory. */
final class NativeEncoderFactory implements InvocationHandler {
   static EncoderFactory create() {
      return (EncoderFactory) Proxy.newProxyInstance(
         EncoderFactory.class.getClassLoader(),
         new Class<?>[] { EncoderFactory.class },
         new NativeEncoderFactory());
   }

   @Override
   public Object invoke(Object proxy, Method method, Object[] arguments) throws Throwable {
      Object[] values = arguments == null ? new Object[0] : arguments;
      if (method.getDeclaringClass() == Object.class) {
         switch (method.getName()) {
            case "toString": return "Umbra JNI C++ encoder factory";
            case "hashCode": return System.identityHashCode(proxy);
            case "equals": return values.length == 1 && proxy == values[0];
            default: throw new AssertionError("Unexpected Object method " + method);
         }
      }
      if ("createHLAinteger32BE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAinteger32BE(0);
         if (values.length == 1) return new NativeHLAinteger32BE(((Integer) values[0]).intValue());
      }
      if ("createHLAinteger32LE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAinteger32LE(0);
         if (values.length == 1) return new NativeHLAinteger32LE(((Integer) values[0]).intValue());
      }
      if ("createHLAinteger64BE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAinteger64BE(0L);
         if (values.length == 1) return new NativeHLAinteger64BE(((Long) values[0]).longValue());
      }
      if ("createHLAinteger64LE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAinteger64LE(0L);
         if (values.length == 1) return new NativeHLAinteger64LE(((Long) values[0]).longValue());
      }
      if ("createHLAinteger16BE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAinteger16BE((short) 0);
         if (values.length == 1) return new NativeHLAinteger16BE(((Short) values[0]).shortValue());
      }
      if ("createHLAinteger16LE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAinteger16LE((short) 0);
         if (values.length == 1) return new NativeHLAinteger16LE(((Short) values[0]).shortValue());
      }
      if ("createHLAfloat32BE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAfloat32BE(0.0f);
         if (values.length == 1) return new NativeHLAfloat32BE(((Float) values[0]).floatValue());
      }
      if ("createHLAfloat32LE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAfloat32LE(0.0f);
         if (values.length == 1) return new NativeHLAfloat32LE(((Float) values[0]).floatValue());
      }
      if ("createHLAfloat64BE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAfloat64BE(0.0d);
         if (values.length == 1) return new NativeHLAfloat64BE(((Double) values[0]).doubleValue());
      }
      if ("createHLAfloat64LE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAfloat64LE(0.0d);
         if (values.length == 1) return new NativeHLAfloat64LE(((Double) values[0]).doubleValue());
      }
      if ("createHLAunsignedInteger16BE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAunsignedInteger16BE((short) 0);
         if (values.length == 1) return new NativeHLAunsignedInteger16BE(((Short) values[0]).shortValue());
      }
      if ("createHLAunsignedInteger16LE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAunsignedInteger16LE((short) 0);
         if (values.length == 1) return new NativeHLAunsignedInteger16LE(((Short) values[0]).shortValue());
      }
      if ("createHLAunsignedInteger32BE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAunsignedInteger32BE(0);
         if (values.length == 1) return new NativeHLAunsignedInteger32BE(((Integer) values[0]).intValue());
      }
      if ("createHLAunsignedInteger32LE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAunsignedInteger32LE(0);
         if (values.length == 1) return new NativeHLAunsignedInteger32LE(((Integer) values[0]).intValue());
      }
      if ("createHLAunsignedInteger64BE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAunsignedInteger64BE(0L);
         if (values.length == 1) return new NativeHLAunsignedInteger64BE(((Long) values[0]).longValue());
      }
      if ("createHLAunsignedInteger64LE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAunsignedInteger64LE(0L);
         if (values.length == 1) return new NativeHLAunsignedInteger64LE(((Long) values[0]).longValue());
      }
      if ("createHLAbyte".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAbyte((byte) 0);
         if (values.length == 1) return new NativeHLAbyte(((Byte) values[0]).byteValue());
      }
      if ("createHLAoctet".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAoctet((byte) 0);
         if (values.length == 1) return new NativeHLAoctet(((Byte) values[0]).byteValue());
      }
      if ("createHLAoctetPairBE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAoctetPairBE((short) 0);
         if (values.length == 1) return new NativeHLAoctetPairBE(((Short) values[0]).shortValue());
      }
      if ("createHLAoctetPairLE".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAoctetPairLE((short) 0);
         if (values.length == 1) return new NativeHLAoctetPairLE(((Short) values[0]).shortValue());
      }
      if ("createHLAASCIIchar".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAASCIIchar((byte) 0);
         if (values.length == 1) return new NativeHLAASCIIchar(((Byte) values[0]).byteValue());
      }
      if ("createHLAunicodeChar".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAunicodeChar((short) 0);
         if (values.length == 1) return new NativeHLAunicodeChar(((Short) values[0]).shortValue());
      }
      if ("createHLAboolean".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAboolean(false);
         if (values.length == 1) return new NativeHLAboolean(((Boolean) values[0]).booleanValue());
      }
      if ("createHLAASCIIstring".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAASCIIstring("");
         if (values.length == 1) return new NativeHLAASCIIstring((String) values[0]);
      }
      if ("createHLAunicodeString".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAunicodeString("");
         if (values.length == 1) return new NativeHLAunicodeString((String) values[0]);
      }
      if ("createHLAopaqueData".equals(method.getName())) {
         if (values.length == 0) return new NativeHLAopaqueData(new byte[0]);
         if (values.length == 1) return new NativeHLAopaqueData((byte[]) values[0]);
      }
      if ("createHLAvariableArray".equals(method.getName()) && values.length == 2) {
         return new NativeHLAvariableArray(
            (DataElementFactory) values[0], (DataElement[]) values[1]);
      }
      if ("createHLAfixedArray".equals(method.getName()) && values.length == 2) {
         return new NativeHLAfixedArray((DataElementFactory) values[0], ((Integer) values[1]).intValue());
      }
      if ("createHLAfixedArray".equals(method.getName()) && values.length == 1
         && values[0] instanceof DataElement[]) {
         DataElement[] elements = (DataElement[]) values[0];
         if (elements.length == 0) {
            throw new IllegalArgumentException("HLAfixedArray requires at least one element");
         }
         DataElementFactory factory = new DataElementFactory() {
            @Override public DataElement createElement(int index) {
               return elements[0];
            }
         };
         NativeHLAfixedArray result = new NativeHLAfixedArray(factory, elements.length);
         for (int index = 0; index < elements.length; ++index) {
            result.set(index, elements[index]);
         }
         return result;
      }
      if ("createHLAfixedRecord".equals(method.getName()) && values.length == 0) {
         return new NativeHLAfixedRecord();
      }
      if ("createHLAvariantRecord".equals(method.getName()) && values.length == 1) {
         return new NativeHLAvariantRecord((DataElement) values[0]);
      }
      if ("createHLAextendableVariantRecord".equals(method.getName()) && values.length == 1) {
         return NativeExtendableVariantRecord.create(
            method.getReturnType(), (DataElement) values[0]);
      }
      if (isHandleEncoder(method.getName()) && values.length == 1) {
         return NativeHandleDataElement.create(method.getReturnType(), values[0], null);
      }
      if (isHandleEncoder(method.getName()) && values.length == 2) {
         return NativeHandleDataElement.create(method.getReturnType(), values[0], values[1]);
      }
      if ("createHLAlogicalTime".equals(method.getName()) && values.length == 1) {
         return NativeLogicalTimeDataElement.create(method.getReturnType(), values[0], false, null);
      }
      if ("createHLAlogicalTime".equals(method.getName()) && values.length == 2) {
         return NativeLogicalTimeDataElement.create(method.getReturnType(), values[0], false, values[1]);
      }
      if ("createHLAlogicalTimeInterval".equals(method.getName()) && values.length == 1) {
         return NativeLogicalTimeDataElement.create(method.getReturnType(), values[0], true, null);
      }
      if ("createHLAlogicalTimeInterval".equals(method.getName()) && values.length == 2) {
         return NativeLogicalTimeDataElement.create(method.getReturnType(), values[0], true, values[1]);
      }
      throw new hla.rti1516_2025.exceptions.RTIinternalError(
         "Umbra JNI encoder factory does not yet bind " + method.toGenericString());
   }

   private static boolean isHandleEncoder(String name) {
      return name.equals("createHLAfederateHandle")
         || name.equals("createHLAobjectClassHandle")
         || name.equals("createHLAinteractionClassHandle")
         || name.equals("createHLAobjectInstanceHandle")
         || name.equals("createHLAattributeHandle")
         || name.equals("createHLAparameterHandle")
         || name.equals("createHLAdimensionHandle")
         || name.equals("createHLAmessageRetractionHandle")
         || name.equals("createHLAregionHandle")
         || name.equals("createHLAtransportationTypeHandle");
   }
}
