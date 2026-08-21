package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.ByteWrapper;
import hla.rti1516_2025.encoding.DataElement;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

/**
 * Standard Java handle data-element projection backed by an RTI handle
 * factory.  The handle value and its wire validation remain owned by C++;
 * this class only supplies the Java EncoderFactory carrier shape.
 */
final class NativeHandleDataElement implements InvocationHandler {
   private final Class<?> interfaceType;
   private final Object ambassador;
   private final String handleFactoryName;
   private Object value;
   private Object proxy;

   private NativeHandleDataElement(
      Class<?> interfaceType, Object ambassador, String handleFactoryName, Object initial) {
      this.interfaceType = interfaceType;
      this.ambassador = ambassador;
      this.handleFactoryName = handleFactoryName;
      this.value = initial;
   }

   static Object create(Class<?> interfaceType, Object ambassador, Object initial) {
      if (interfaceType == null || ambassador == null) {
         throw new IllegalArgumentException("A handle encoder requires an RTIambassador");
      }
      String simpleName = interfaceType.getSimpleName();
      if (!simpleName.startsWith("HLA") || !simpleName.endsWith("Handle")) {
         throw new IllegalArgumentException("Unsupported Java handle encoder: " + interfaceType);
      }
      String baseName = baseName(simpleName);
      NativeHandleDataElement handler = new NativeHandleDataElement(
         interfaceType,
         ambassador,
         "get" + baseName + "Factory",
         initial);
      handler.proxy = Proxy.newProxyInstance(
         interfaceType.getClassLoader(), new Class<?>[] { interfaceType }, handler);
      return handler.proxy;
   }

   private Object requireValue() {
      if (value == null) {
         throw new IllegalStateException("The Java handle data element has no value");
      }
      return value;
   }

   private byte[] encodedValue() throws Throwable {
      Object handle = requireValue();
      Method lengthMethod = interfaceForHandle().getMethod("encodedLength");
      Method encodeMethod = interfaceForHandle().getMethod("encode", byte[].class, int.class);
      int length = ((Integer) lengthMethod.invoke(handle)).intValue();
      if (length < 0) throw new IllegalStateException("The Java handle length is negative");
      byte[] encoded = new byte[length];
      encodeMethod.invoke(handle, encoded, Integer.valueOf(0));
      return encoded;
   }

   private Class<?> interfaceForHandle() throws ClassNotFoundException {
      return Class.forName(
         "hla.rti1516_2025." + baseName(interfaceType.getSimpleName()));
   }

   private static String baseName(String simpleName) {
      String suffix = simpleName.substring(3);
      return Character.toUpperCase(suffix.charAt(0)) + suffix.substring(1);
   }

   private Object decode(byte[] encoded) throws Throwable {
      if (encoded == null) throw new IllegalArgumentException("Handle encoding must not be null");
      Class<?> ambassadorType = Class.forName("hla.rti1516_2025.RTIambassador");
      Object factory = ambassadorType.getMethod(handleFactoryName).invoke(ambassador);
      Class<?> factoryType = Class.forName(
         "hla.rti1516_2025." + baseName(interfaceType.getSimpleName()) + "Factory");
      Object decoded = factoryType.getMethod("decode", byte[].class, int.class)
         .invoke(factory, encoded, Integer.valueOf(0));
      value = decoded;
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
            case "toString": return "Umbra JNI " + interfaceType.getSimpleName() + " data element";
            case "hashCode": return Integer.valueOf(value == null ? 0 : value.hashCode());
            case "equals": return values.length == 1 && values[0] == proxy;
            default: throw new AssertionError("Unexpected Object method " + method);
         }
      }
      if ("getValue".equals(name) && values.length == 0) return value;
      if ("setValue".equals(name) && values.length == 1) {
         value = values[0];
         return proxy;
      }
      if ("getOctetBoundary".equals(name) && values.length == 0) return Integer.valueOf(1);
      if ("getEncodedLength".equals(name) && values.length == 0) {
         return Integer.valueOf(encodedValue().length);
      }
      if ("toByteArray".equals(name) && values.length == 0) return encodedValue();
      if ("encode".equals(name) && values.length == 0) {
         return new ByteWrapper(encodedValue());
      }
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
      throw new UnsupportedOperationException("Unsupported Java handle data-element operation: " + method);
   }
}
