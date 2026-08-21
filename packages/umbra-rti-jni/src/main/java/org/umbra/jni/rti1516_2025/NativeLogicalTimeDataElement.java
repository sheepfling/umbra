package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.ByteWrapper;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

/** Java EncoderFactory logical-time carrier delegated to the C++ time factory. */
final class NativeLogicalTimeDataElement implements InvocationHandler {
   private final Class<?> interfaceType;
   private final Object factory;
   private final boolean interval;
   private Object value;
   private Object proxy;

   private NativeLogicalTimeDataElement(
      Class<?> interfaceType, Object factory, boolean interval, Object initial) {
      this.interfaceType = interfaceType;
      this.factory = factory;
      this.interval = interval;
      this.value = initial;
   }

   static Object create(Class<?> interfaceType, Object ambassador, boolean interval, Object initial)
      throws Throwable {
      if (interfaceType == null || ambassador == null) {
         throw new IllegalArgumentException("A logical-time encoder requires an RTIambassador");
      }
      Class<?> ambassadorType = Class.forName("hla.rti1516_2025.RTIambassador");
      Object factory = ambassadorType.getMethod("getTimeFactory").invoke(ambassador);
      Class<?> factoryType = Class.forName("hla.rti1516_2025.time.LogicalTimeFactory");
      Object value = initial;
      if (value == null) {
         value = factoryType.getMethod(interval ? "makeZero" : "makeInitial").invoke(factory);
      }
      NativeLogicalTimeDataElement handler = new NativeLogicalTimeDataElement(
         interfaceType, factory, interval, value);
      handler.proxy = Proxy.newProxyInstance(
         interfaceType.getClassLoader(), new Class<?>[] { interfaceType }, handler);
      return handler.proxy;
   }

   private byte[] encodedValue() throws Throwable {
      Class<?> valueType = Class.forName(
         "hla.rti1516_2025.time." + (interval ? "LogicalTimeInterval" : "LogicalTime"));
      Method lengthMethod = valueType.getMethod("encodedLength");
      Method encodeMethod = valueType.getMethod("encode", byte[].class, int.class);
      int length = ((Integer) lengthMethod.invoke(value)).intValue();
      if (length < 0) throw new IllegalStateException("The Java logical-time length is negative");
      byte[] encoded = new byte[length];
      encodeMethod.invoke(value, encoded, Integer.valueOf(0));
      return encoded;
   }

   private Object decode(byte[] encoded) throws Throwable {
      if (encoded == null) throw new IllegalArgumentException("Logical-time encoding must not be null");
      Class<?> factoryType = Class.forName("hla.rti1516_2025.time.LogicalTimeFactory");
      value = factoryType.getMethod(
         interval ? "decodeInterval" : "decodeTime", byte[].class, int.class)
         .invoke(factory, encoded, Integer.valueOf(0));
      return proxy;
   }

   private static Throwable unwrap(Throwable error) {
      if (error instanceof InvocationTargetException && error.getCause() != null) {
         return error.getCause();
      }
      return error;
   }

   @Override public Object invoke(Object ignored, Method method, Object[] arguments) throws Throwable {
      Object[] values = arguments == null ? new Object[0] : arguments;
      String name = method.getName();
      if (method.getDeclaringClass() == Object.class) {
         switch (name) {
            case "toString": return "Umbra JNI logical-time data element";
            case "hashCode": return Integer.valueOf(value.hashCode());
            case "equals": return values.length == 1 && values[0] == proxy;
            default: throw new AssertionError("Unexpected Object method " + method);
         }
      }
      if ("getValue".equals(name) && values.length == 0) return value;
      if ("setValue".equals(name) && values.length == 1) {
         value = values[0];
         return proxy;
      }
      if ("getOctetBoundary".equals(name) && values.length == 0) return Integer.valueOf(8);
      if ("getEncodedLength".equals(name) && values.length == 0) {
         return Integer.valueOf(encodedValue().length);
      }
      if ("toByteArray".equals(name) && values.length == 0) return encodedValue();
      if ("encode".equals(name) && values.length == 0) return new ByteWrapper(encodedValue());
      if ("encode".equals(name) && values.length == 1 && values[0] instanceof ByteWrapper) {
         ((ByteWrapper) values[0]).put(encodedValue());
         return null;
      }
      if ("decode".equals(name) && values.length == 1 && values[0] instanceof byte[]) {
         try {
            return decode((byte[]) values[0]);
         } catch (Throwable error) {
            throw unwrap(error);
         }
      }
      if ("decode".equals(name) && values.length == 1 && values[0] instanceof ByteWrapper) {
         ByteWrapper source = (ByteWrapper) values[0];
         byte[] encoded = new byte[source.remaining()];
         source.get(encoded);
         try {
            return decode(encoded);
         } catch (Throwable error) {
            throw unwrap(error);
         }
      }
      throw new UnsupportedOperationException("Unsupported Java logical-time data-element operation: " + method);
   }
}
