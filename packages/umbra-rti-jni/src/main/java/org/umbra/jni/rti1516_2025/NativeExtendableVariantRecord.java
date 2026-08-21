package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.encoding.ByteWrapper;
import hla.rti1516_2025.encoding.DataElement;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.lang.ref.Cleaner;
import java.util.Base64;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Java 2025 extendable-variant carrier over the native C++ implementation.
 * The standard Java interface has no addVariant operation, so the first
 * setVariant call registers the C++ alternative and subsequent calls replace
 * its value while retaining C++ type checks.
 */
final class NativeExtendableVariantRecord implements InvocationHandler {
   private static final Cleaner CLEANER = Cleaner.create();

   private static final class NativeHandle implements Runnable {
      private long value;
      NativeHandle(long value) { this.value = value; }
      long require() {
         if (value == 0L) throw new IllegalStateException(
            "Native HLAextendableVariantRecord is closed");
         return value;
      }
      @Override public void run() {
         if (value != 0L) {
            NativeBridge.nativeDestroyHLAextendableVariantRecord(value);
            value = 0L;
         }
      }
   }

   private final DataElement discriminantPrototype;
   private final Map<String, DataElement> variants = new LinkedHashMap<String, DataElement>();
   private final NativeHandle nativeHandle;
   @SuppressWarnings("unused") private final Cleaner.Cleanable cleanable;
   private Object proxy;

   private NativeExtendableVariantRecord(DataElement discriminantPrototype) {
      if (discriminantPrototype == null) {
         throw new IllegalArgumentException(
            "HLAextendableVariantRecord discriminant prototype must not be null");
      }
      this.discriminantPrototype = discriminantPrototype;
      nativeHandle = new NativeHandle(
         NativeBridge.nativeCreateHLAextendableVariantRecord(discriminantPrototype));
      cleanable = CLEANER.register(this, nativeHandle);
   }

   static Object create(Class<?> interfaceType, DataElement discriminantPrototype) {
      NativeExtendableVariantRecord handler =
         new NativeExtendableVariantRecord(discriminantPrototype);
      handler.proxy = Proxy.newProxyInstance(
         interfaceType.getClassLoader(),
         new Class<?>[] { interfaceType, NativeCompositionDataElement.class },
         handler);
      return handler.proxy;
   }

   private static String key(DataElement discriminant) {
      return Base64.getEncoder().encodeToString(discriminant.toByteArray());
   }

   private DataElement discriminant() {
      return NativeDataElementEncoding.decode(
         discriminantPrototype,
         NativeBridge.nativeHLAextendableVariantRecordDiscriminantEncoding(nativeHandle.require()));
   }

   private DataElement value() {
      DataElement prototype = variants.get(key(discriminant()));
      if (prototype == null) return null;
      return NativeDataElementEncoding.decode(
         prototype,
         NativeBridge.nativeHLAextendableVariantRecordValueEncoding(nativeHandle.require()));
   }

   @Override public Object invoke(Object ignored, Method method, Object[] arguments) throws Throwable {
      Object[] values = arguments == null ? new Object[0] : arguments;
      String name = method.getName();
      if ("nativeHandleForComposition".equals(name) && values.length == 0) {
         return Long.valueOf(nativeHandle.require());
      }
      if (method.getDeclaringClass() == Object.class) {
         switch (name) {
            case "toString": return "Umbra JNI HLAextendableVariantRecord";
            case "hashCode": return Integer.valueOf(System.identityHashCode(proxy));
            case "equals": return values.length == 1 && values[0] == proxy;
            default: throw new AssertionError("Unexpected Object method " + method);
         }
      }
      if ("setVariant".equals(name) && values.length == 2) {
         DataElement discriminantValue = (DataElement) values[0];
         DataElement variantValue = (DataElement) values[1];
         if (discriminantValue == null || variantValue == null) {
            throw new IllegalArgumentException(
               "HLAextendableVariantRecord discriminant and value must not be null");
         }
         String variantKey = key(discriminantValue);
         if (!variants.containsKey(variantKey)) {
            NativeBridge.nativeAddHLAextendableVariantRecordVariant(
               nativeHandle.require(), discriminantValue, variantValue);
            variants.put(variantKey, variantValue);
         }
         NativeBridge.nativeSetHLAextendableVariantRecordVariant(
            nativeHandle.require(), discriminantValue, variantValue);
         variants.put(variantKey, variantValue);
         return proxy;
      }
      if ("setDiscriminant".equals(name) && values.length == 1) {
         if (values[0] == null) {
            throw new IllegalArgumentException(
               "HLAextendableVariantRecord discriminant must not be null");
         }
         NativeBridge.nativeSetHLAextendableVariantRecordDiscriminant(
            nativeHandle.require(), (DataElement) values[0]);
         return proxy;
      }
      if ("getDiscriminant".equals(name) && values.length == 0) return discriminant();
      if ("getValue".equals(name) && values.length == 0) return value();
      if ("getOctetBoundary".equals(name) && values.length == 0) {
         return Integer.valueOf(NativeBridge.nativeHLAextendableVariantRecordOctetBoundary(nativeHandle.require()));
      }
      if ("getEncodedLength".equals(name) && values.length == 0) {
         return Integer.valueOf(NativeBridge.nativeHLAextendableVariantRecordEncodedLength(nativeHandle.require()));
      }
      if ("toByteArray".equals(name) && values.length == 0) {
         return NativeBridge.nativeHLAextendableVariantRecordToByteArray(nativeHandle.require());
      }
      if ("encode".equals(name) && values.length == 0) {
         return new ByteWrapper(NativeBridge.nativeHLAextendableVariantRecordToByteArray(nativeHandle.require()));
      }
      if ("encode".equals(name) && values.length == 1 && values[0] instanceof ByteWrapper) {
         ((ByteWrapper) values[0]).put(
            NativeBridge.nativeHLAextendableVariantRecordToByteArray(nativeHandle.require()));
         return null;
      }
      if ("decode".equals(name) && values.length == 1 && values[0] instanceof byte[]) {
         NativeBridge.nativeDecodeHLAextendableVariantRecord(nativeHandle.require(), (byte[]) values[0]);
         return proxy;
      }
      if ("decode".equals(name) && values.length == 1 && values[0] instanceof ByteWrapper) {
         ByteWrapper source = (ByteWrapper) values[0];
         byte[] encoded = new byte[source.remaining()];
         source.get(encoded);
         NativeBridge.nativeDecodeHLAextendableVariantRecord(nativeHandle.require(), encoded);
         return proxy;
      }
      throw new UnsupportedOperationException(
         "Unsupported Java HLAextendableVariantRecord operation: " + method);
   }
}
