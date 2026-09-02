package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.nio.ByteBuffer;
import java.util.Arrays;

/**
 * Standard Java time carriers for the C++ selected logical-time factory.
 *
 * <p>The RTI owns factory selection, time validation at service boundaries,
 * and every time value that crosses JNI. These small Java carriers satisfy the
 * standard Java API's value-object contract so an application can pass that
 * native-selected time back to a standard RTI method. Their only wire format
 * is the C++ standard eight-octet representation.</p>
 */
public final class NativeLogicalTimeFactories {
   private NativeLogicalTimeFactories() { }

   /** Edition-neutral creators exposed only as an adapter convenience. */
   public interface IntegerFactorySurface {
      Object makeLogicalTime(long value);
      Object makeLogicalTimeInterval(long value);
      Object makeTime(long value);
      Object makeInterval(long value);
   }

   /** Edition-neutral creators exposed only as an adapter convenience. */
   public interface FloatFactorySurface {
      Object makeLogicalTime(double value);
      Object makeLogicalTimeInterval(double value);
      Object makeTime(double value);
      Object makeInterval(double value);
   }

   /** Compatibility accessors retained for the Python value normalizer. */
   public interface IntegerCarrierSurface {
      long getValue();
      long getTime();
      long getInterval();
   }

   /** Compatibility accessors retained for the Python value normalizer. */
   public interface FloatCarrierSurface {
      double getValue();
      double getTime();
      double getInterval();
   }

   public static LogicalTimeFactory forName(String factoryName, long nativeHandle) {
      if ("HLAinteger64Time".equals(factoryName)) {
         return factory(nativeHandle, false, true);
      }
      if ("HLAfloat64Time".equals(factoryName)) {
         return factory(nativeHandle, true, true);
      }
      throw new IllegalArgumentException("Unsupported C++ logical-time factory: " + factoryName);
   }

   /**
    * Build the exact time-factory shape used by the standard ServiceLoader.
    *
    * <p>This path is deliberately standalone: a top-level logical-time
    * provider is discovered before an RTI ambassador has joined a federation,
    * so it uses the Java value carrier for construction and decoding.  Once an
    * ambassador has selected a C++ time factory, {@link #forName(String, long)}
    * continues to use the native codec path.</p>
    */
   public static LogicalTimeFactory<?, ?> forServiceLoader(String factoryName) {
      if ("HLAinteger64Time".equals(factoryName)) {
         return factory(0L, false, false);
      }
      if ("HLAfloat64Time".equals(factoryName)) {
         return factory(0L, true, false);
      }
      throw new IllegalArgumentException("Unsupported standard logical-time factory: " + factoryName);
   }

   private static LogicalTimeFactory factory(
      long nativeHandle, boolean floatingPoint, boolean includeCompatibilityRoot) {
      Class<?> creators = floatingPoint ? FloatFactorySurface.class : IntegerFactorySurface.class;
      return (LogicalTimeFactory) Proxy.newProxyInstance(
         LogicalTimeFactory.class.getClassLoader(),
         factoryInterfaces(creators, floatingPoint, includeCompatibilityRoot),
         new Factory(nativeHandle, floatingPoint, includeCompatibilityRoot));
   }

   private static Class<?>[] factoryInterfaces(
      Class<?> creators, boolean floatingPoint, boolean includeCompatibilityRoot) {
      if (!includeCompatibilityRoot) {
         String factoryName = floatingPoint
            ? "hla.rti1516_2025.time.HLAfloat64TimeFactory"
            : "hla.rti1516_2025.time.HLAinteger64TimeFactory";
         Class<?> exact = optionalClass(factoryName);
         if (exact == null) {
            throw new IllegalStateException("Standard logical-time factory is unavailable: " + factoryName);
         }
         return new Class<?>[] { exact };
      }
      Class<?> root = optionalClass("hla.rti1516_2025.LogicalTimeFactory");
      if (root == null) {
         return new Class<?>[] { LogicalTimeFactory.class, creators };
      }
      return new Class<?>[] { LogicalTimeFactory.class, root, creators };
   }

   private static Class<?>[] carrierInterfaces(
      boolean time, boolean floatingPoint, Class<?> surface, boolean includeCompatibilityRoot) {
      if (!includeCompatibilityRoot) {
         String valueName = "hla.rti1516_2025.time." +
            (floatingPoint
               ? (time ? "HLAfloat64Time" : "HLAfloat64Interval")
               : (time ? "HLAinteger64Time" : "HLAinteger64Interval"));
         Class<?> exact = optionalClass(valueName);
         if (exact == null) {
            throw new IllegalStateException("Standard logical-time value is unavailable: " + valueName);
         }
         return new Class<?>[] { exact };
      }
      Class<?> standard = time ? LogicalTime.class : LogicalTimeInterval.class;
      Class<?> root = optionalClass(
         "hla.rti1516_2025." + (time ? "LogicalTime" : "LogicalTimeInterval"));
      if (root == null) return new Class<?>[] { standard, surface };
      return new Class<?>[] { standard, root, surface };
   }

   private static Class<?> optionalClass(String name) {
      try {
         return Class.forName(name);
      } catch (ClassNotFoundException ignored) {
         return null;
      }
   }

   /** Invoke a provider delegate without tying this bridge to one API revision's checked exception type. */
   static Object invokeProvider(
      Object delegate, String methodName, Class<?>[] parameterTypes, Object... arguments) {
      try {
         return delegate.getClass().getMethod(methodName, parameterTypes).invoke(delegate, arguments);
      } catch (java.lang.reflect.InvocationTargetException error) {
         Throwable cause = error.getCause() == null ? error : error.getCause();
         if (cause instanceof RuntimeException) throw (RuntimeException) cause;
         if (cause instanceof Error) throw (Error) cause;
         return NativeLogicalTimeFactories.<RuntimeException, Object>rethrow(cause);
      } catch (ReflectiveOperationException error) {
         throw new IllegalStateException(
            "Logical-time provider delegate is missing " + methodName, error);
      }
   }

   @SuppressWarnings("unchecked")
   private static <T extends Throwable, R> R rethrow(Throwable error) throws T {
      throw (T) error;
   }

   private static final class Factory implements InvocationHandler {
      private final long nativeHandle;
      private final boolean floatingPoint;
      private final boolean includeCompatibilityRoot;

      Factory(long nativeHandle, boolean floatingPoint, boolean includeCompatibilityRoot) {
         this.nativeHandle = nativeHandle;
         this.floatingPoint = floatingPoint;
         this.includeCompatibilityRoot = includeCompatibilityRoot;
      }

      @Override public Object invoke(Object proxy, Method method, Object[] arguments) {
         Object[] values = arguments == null ? new Object[0] : arguments;
         String name = method.getName();
         if (method.getDeclaringClass() == Object.class) {
            if ("toString".equals(name)) return floatingPoint ? "HLAfloat64Time" : "HLAinteger64Time";
            if ("hashCode".equals(name)) return System.identityHashCode(proxy);
            if ("equals".equals(name)) return proxy == values[0];
         }
         if ("getName".equals(name)) return floatingPoint ? "HLAfloat64Time" : "HLAinteger64Time";
         if ("makeInitial".equals(name)) {
            return floatingPoint ? makeFloatTimeValue(0.0d) : makeIntegerTimeValue(0L);
         }
         if ("makeFinal".equals(name)) {
            return floatingPoint ? makeFloatTimeValue(Double.MAX_VALUE) : makeIntegerTimeValue(Long.MAX_VALUE);
         }
         if ("makeLogicalTime".equals(name) || "makeTime".equals(name)) {
            Number value = (Number) values[0];
            return floatingPoint
               ? makeFloatTimeValue(value.doubleValue())
               : makeIntegerTimeValue(value.longValue());
         }
         if ("makeZero".equals(name)) {
            return floatingPoint ? makeFloatIntervalValue(0.0d) : makeIntegerIntervalValue(0L);
         }
         if ("makeEpsilon".equals(name)) {
            return floatingPoint
               ? makeFloatIntervalValue(Double.MIN_VALUE)
               : makeIntegerIntervalValue(1L);
         }
         if ("makeLogicalTimeInterval".equals(name) || "makeInterval".equals(name)) {
            Number value = (Number) values[0];
            return floatingPoint
               ? makeFloatIntervalValue(value.doubleValue())
               : makeIntegerIntervalValue(value.longValue());
         }
         if ("decodeLogicalTime".equals(name) || "decodeTime".equals(name)) {
            return decodeTimeValue((byte[]) values[0], ((Integer) values[1]).intValue());
         }
         if ("decodeLogicalTimeInterval".equals(name) || "decodeInterval".equals(name)) {
            return decodeIntervalValue((byte[]) values[0], ((Integer) values[1]).intValue());
         }
         throw new UnsupportedOperationException("Unsupported logical-time factory operation: " + method);
      }

      private Object makeIntegerTimeValue(long value) {
         if (nativeHandle == 0L) return new Carrier(0L, true, value, false).proxy();
         return new Carrier(nativeHandle, true,
            decodeLong(NativeBridge.nativeMakeIntegerLogicalTime(nativeHandle, value)), true).proxy();
      }

      private Object makeFloatTimeValue(double value) {
         if (nativeHandle == 0L) return new Carrier(0L, true, value, false).proxy();
         return new Carrier(nativeHandle, true,
            decodeDouble(NativeBridge.nativeMakeFloatLogicalTime(nativeHandle, value)), true).proxy();
      }

      private Object makeIntegerIntervalValue(long value) {
         if (nativeHandle == 0L) return new Carrier(0L, false, value, false).proxy();
         return new Carrier(nativeHandle, false,
            decodeLong(NativeBridge.nativeMakeIntegerLogicalTimeInterval(nativeHandle, value)), true).proxy();
      }

      private Object makeFloatIntervalValue(double value) {
         if (nativeHandle == 0L) return new Carrier(0L, false, value, false).proxy();
         return new Carrier(nativeHandle, false,
            decodeDouble(NativeBridge.nativeMakeFloatLogicalTimeInterval(nativeHandle, value)), true).proxy();
      }

      private Object decodeTimeValue(byte[] encoded, int offset) {
         if (nativeHandle == 0L) {
            verifyEncoding(encoded, offset);
            ByteBuffer buffer = ByteBuffer.wrap(encoded, offset, Long.BYTES);
            return floatingPoint
               ? new Carrier(0L, true, buffer.getDouble(), false).proxy()
               : new Carrier(0L, true, buffer.getLong(), false).proxy();
         }
         byte[] result = nativeDecode(nativeHandle, encoded, offset, false);
         if (floatingPoint) return new Carrier(nativeHandle, true, decodeDouble(result), true).proxy();
         return new Carrier(nativeHandle, true, decodeLong(result), true).proxy();
      }

      private Object decodeIntervalValue(byte[] encoded, int offset) {
         if (nativeHandle == 0L) {
            verifyEncoding(encoded, offset);
            ByteBuffer buffer = ByteBuffer.wrap(encoded, offset, Long.BYTES);
            return floatingPoint
               ? new Carrier(0L, false, buffer.getDouble(), false).proxy()
               : new Carrier(0L, false, buffer.getLong(), false).proxy();
         }
         byte[] result = nativeDecode(nativeHandle, encoded, offset, true);
         if (floatingPoint) return new Carrier(nativeHandle, false, decodeDouble(result), true).proxy();
         return new Carrier(nativeHandle, false, decodeLong(result), true).proxy();
      }
   }

   /**
    * Return the standard implementation name carried by a logical-time value.
    *
    * <p>The IEEE interfaces intentionally do not expose an implementation-name
    * method.  Values produced by this provider retain that identity in their
    * invocation handler; ordinary standard carriers are classified by their
    * exact IEEE time interface.  The native bridge compares the result with the
    * C++ federation's selected factory before any bytes are dispatched.</p>
    */
   static String implementationName(Object value) {
      if (value == null) throw new IllegalArgumentException("Logical-time value is null");
      if (Proxy.isProxyClass(value.getClass())) {
         InvocationHandler handler = Proxy.getInvocationHandler(value);
         if (handler instanceof Carrier) {
            return ((Carrier) handler).floatingPoint ? "HLAfloat64Time" : "HLAinteger64Time";
         }
      }
      String typeName = value.getClass().getName();
      if (typeName.contains("HLAinteger64")) return "HLAinteger64Time";
      if (typeName.contains("HLAfloat64")) return "HLAfloat64Time";
      throw new IllegalArgumentException(
         "Unsupported standard logical-time implementation: " + value.getClass().getName());
   }

   private static final class Carrier implements InvocationHandler {
      private final long nativeHandle;
      private final boolean floatingPoint;
      private final boolean time;
      private final boolean includeCompatibilityRoot;
      private long integerValue;
      private double floatingValue;

      Carrier(long nativeHandle, boolean time, long value) {
         this(nativeHandle, time, value, true);
      }

      Carrier(long nativeHandle, boolean time, long value, boolean includeCompatibilityRoot) {
         this.nativeHandle = nativeHandle;
         this.floatingPoint = false;
         this.time = time;
         this.includeCompatibilityRoot = includeCompatibilityRoot;
         this.integerValue = value;
         this.floatingValue = 0.0d;
      }

      Carrier(long nativeHandle, boolean time, double value) {
         this(nativeHandle, time, value, true);
      }

      Carrier(long nativeHandle, boolean time, double value, boolean includeCompatibilityRoot) {
         this.nativeHandle = nativeHandle;
         this.floatingPoint = true;
         this.time = time;
         this.includeCompatibilityRoot = includeCompatibilityRoot;
         this.integerValue = 0L;
         this.floatingValue = value == 0.0d ? 0.0d : value;
      }

      private byte[] encoded() {
         byte[] result = new byte[Long.BYTES];
         if (floatingPoint) ByteBuffer.wrap(result).putDouble(floatingValue);
         else ByteBuffer.wrap(result).putLong(integerValue);
         return result;
      }

      private Carrier arithmeticResult(boolean resultTime, byte[] encoded) {
         if (encoded == null || encoded.length != Long.BYTES) {
            throw new IllegalArgumentException("C++ returned a malformed logical-time encoding");
         }
         ByteBuffer buffer = ByteBuffer.wrap(encoded);
         return floatingPoint
            ? new Carrier(nativeHandle, resultTime, buffer.getDouble(), includeCompatibilityRoot)
            : new Carrier(nativeHandle, resultTime, buffer.getLong(), includeCompatibilityRoot);
      }

      Object proxy() {
         Class<?> surface = floatingPoint ? FloatCarrierSurface.class : IntegerCarrierSurface.class;
         return Proxy.newProxyInstance(
            LogicalTime.class.getClassLoader(),
            carrierInterfaces(time, floatingPoint, surface, includeCompatibilityRoot), this);
      }

      @Override public Object invoke(Object proxy, Method method, Object[] arguments) throws Throwable {
         Object[] values = arguments == null ? new Object[0] : arguments;
         String name = method.getName();
         if ("getValue".equals(name) && values.length == 0) {
            if (floatingPoint) return Double.valueOf(floatingValue);
            return Long.valueOf(integerValue);
         }
         if ("implementationName".equals(name) && values.length == 0) {
            return floatingPoint ? "HLAfloat64Time" : "HLAinteger64Time";
         }
         if ("getTime".equals(name) && values.length == 0) {
            if (floatingPoint) return Double.valueOf(floatingValue);
            return Long.valueOf(integerValue);
         }
         if ("getInterval".equals(name) && values.length == 0) {
            if (floatingPoint) return Double.valueOf(floatingValue);
            return Long.valueOf(integerValue);
         }
         if ("isInitial".equals(name) && values.length == 0) {
            return Boolean.valueOf(time && (floatingPoint ? floatingValue == 0.0d : integerValue == 0L));
         }
         if ("isFinal".equals(name) && values.length == 0) {
            return Boolean.valueOf(time && (floatingPoint ? floatingValue == Double.MAX_VALUE : integerValue == Long.MAX_VALUE));
         }
         if ("isZero".equals(name) && values.length == 0) {
            return Boolean.valueOf(!time && (floatingPoint ? floatingValue == 0.0d : integerValue == 0L));
         }
         if ("isEpsilon".equals(name) && values.length == 0) {
            return Boolean.valueOf(!time && (floatingPoint ? floatingValue == Double.MIN_VALUE : integerValue == 1L));
         }
         if ("encodedLength".equals(name) && values.length == 0) return Integer.valueOf(Long.BYTES);
         if ("encode".equals(name) && values.length == 2) {
            byte[] buffer = (byte[]) values[0];
            int offset = ((Integer) values[1]).intValue();
            if (buffer == null || offset < 0 || buffer.length - offset < Long.BYTES) {
               throw new IndexOutOfBoundsException("Logical-time encoding buffer is too small");
            }
            if (floatingPoint) ByteBuffer.wrap(buffer, offset, Double.BYTES).putDouble(floatingValue);
            else ByteBuffer.wrap(buffer, offset, Long.BYTES).putLong(integerValue);
            return null;
         }
         if ("compareTo".equals(name) && values.length == 1) {
            Carrier other = requireCarrier(values[0], time);
            return Integer.valueOf(floatingPoint
               ? Double.compare(floatingValue, other.floatingValue)
               : Long.compare(integerValue, other.integerValue));
         }
         if ("setToDifference".equals(name) && values.length == 2) {
            if (time) throw new IllegalArgumentException("Only a logical-time interval can hold a difference");
            Carrier minuend = requireCarrier(values[0], true);
            Carrier subtrahend = requireCarrier(values[1], true);
            Carrier result = nativeHandle == 0L
               ? differenceResult(minuend, subtrahend)
               : arithmeticResult(
                  false,
                  NativeBridge.nativeDifferenceLogicalTime(
                     nativeHandle, minuend.encoded(), subtrahend.encoded()));
            integerValue = result.integerValue;
            floatingValue = result.floatingValue;
            return null;
         }
         if (("add".equals(name) || "subtract".equals(name)) && values.length == 1) {
            // LogicalTime.add/subtract takes an interval, and
            // LogicalTimeInterval.add/subtract also takes an interval. The
            // operand is therefore never a logical-time carrier; using
            // !time here incorrectly rejected interval arithmetic.
            Carrier other = requireCarrier(values[0], false);
            if (nativeHandle == 0L) {
               double sign = "add".equals(name) ? 1.0d : -1.0d;
               return floatingPoint
                  ? new Carrier(0L, time, floatingValue + sign * other.floatingValue,
                     includeCompatibilityRoot).proxy()
                  : new Carrier(0L, time, integerValue + ("add".equals(name)
                     ? other.integerValue : -other.integerValue), includeCompatibilityRoot).proxy();
            }
            byte[] result;
            if (time) {
               result = "add".equals(name)
                  ? NativeBridge.nativeAddLogicalTime(nativeHandle, encoded(), other.encoded())
                  : NativeBridge.nativeSubtractLogicalTime(nativeHandle, encoded(), other.encoded());
            } else {
               result = "add".equals(name)
                  ? NativeBridge.nativeAddLogicalTimeInterval(nativeHandle, encoded(), other.encoded())
                  : NativeBridge.nativeSubtractLogicalTimeInterval(nativeHandle, encoded(), other.encoded());
            }
            return arithmeticResult(time, result).proxy();
         }
         if ("distance".equals(name) && values.length == 1) {
            Carrier other = requireCarrier(values[0], true);
            if (nativeHandle == 0L) return differenceResult(this, other).proxy();
            return arithmeticResult(
               false,
               NativeBridge.nativeDifferenceLogicalTime(nativeHandle, encoded(), other.encoded())
            ).proxy();
         }
         if ("equals".equals(name) && values.length == 1) {
            if (values[0] == proxy) return Boolean.TRUE;
            if (values[0] == null || !Proxy.isProxyClass(values[0].getClass())) return Boolean.FALSE;
            InvocationHandler handler = Proxy.getInvocationHandler(values[0]);
            if (!(handler instanceof Carrier)) return Boolean.FALSE;
            Carrier other = (Carrier) handler;
            return Boolean.valueOf(floatingPoint == other.floatingPoint && time == other.time
               && (floatingPoint ? Double.doubleToLongBits(floatingValue) == Double.doubleToLongBits(other.floatingValue)
                  : integerValue == other.integerValue));
         }
         if ("hashCode".equals(name) && values.length == 0) {
            return Integer.valueOf((floatingPoint ? Double.valueOf(floatingValue).hashCode() : Long.valueOf(integerValue).hashCode())
               * 31 + (time ? 1 : 0));
         }
         if ("toString".equals(name) && values.length == 0) {
            if (time && (floatingPoint ? floatingValue == 0.0d : integerValue == 0L)) return "initial";
            if (time && (floatingPoint ? floatingValue == Double.MAX_VALUE : integerValue == Long.MAX_VALUE)) return "final";
            if (!time && (floatingPoint ? floatingValue == 0.0d : integerValue == 0L)) return "zero";
            if (!time && (floatingPoint ? floatingValue == Double.MIN_VALUE : integerValue == 1L)) return "epsilon";
            return floatingPoint ? Double.toString(floatingValue) : Long.toString(integerValue);
         }
         throw new UnsupportedOperationException("Unsupported standard logical-time operation: " + method);
      }

      private Carrier requireCarrier(Object candidate, boolean expectedTime) {
         if (candidate == null || !Proxy.isProxyClass(candidate.getClass())) {
            throw new IllegalArgumentException("Logical-time implementation mismatch");
         }
         InvocationHandler handler = Proxy.getInvocationHandler(candidate);
         if (!(handler instanceof Carrier)) throw new IllegalArgumentException("Logical-time implementation mismatch");
         Carrier result = (Carrier) handler;
         if (result.floatingPoint != floatingPoint || result.time != expectedTime) {
            throw new IllegalArgumentException("Logical-time implementation mismatch");
         }
         return result;
      }

      private Carrier differenceResult(Carrier minuend, Carrier subtrahend) {
         if (floatingPoint) {
            return new Carrier(0L, false,
               minuend.floatingValue - subtrahend.floatingValue,
               includeCompatibilityRoot);
         }
         return new Carrier(0L, false,
            minuend.integerValue - subtrahend.integerValue,
            includeCompatibilityRoot);
      }
   }

   private static byte[] nativeDecode(
      long nativeHandle, byte[] encoded, int offset, boolean interval) {
      verifyEncoding(encoded, offset);
      byte[] window = Arrays.copyOfRange(encoded, offset, offset + Long.BYTES);
      return interval
         ? NativeBridge.nativeDecodeLogicalTimeInterval(nativeHandle, window)
         : NativeBridge.nativeDecodeLogicalTime(nativeHandle, window);
   }

   private static long decodeLong(byte[] encoded) {
      verifyResultEncoding(encoded);
      return ByteBuffer.wrap(encoded).getLong();
   }

   private static double decodeDouble(byte[] encoded) {
      verifyResultEncoding(encoded);
      return ByteBuffer.wrap(encoded).getDouble();
   }

   private static void verifyEncoding(byte[] encoded, int offset) {
      if (encoded == null || offset < 0 || encoded.length - offset < Long.BYTES) {
         throw new IllegalArgumentException("Logical-time encoding requires eight octets from the specified offset");
      }
   }

   private static void verifyResultEncoding(byte[] encoded) {
      if (encoded == null || encoded.length != Long.BYTES) {
         throw new IllegalArgumentException("C++ returned a malformed logical-time encoding");
      }
   }
}
