package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.time.HLAfloat64Interval;
import hla.rti1516_2025.time.HLAfloat64Time;
import hla.rti1516_2025.time.HLAfloat64TimeFactory;
import hla.rti1516_2025.time.HLAinteger64Interval;
import hla.rti1516_2025.time.HLAinteger64Time;
import hla.rti1516_2025.time.HLAinteger64TimeFactory;
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

   public static LogicalTimeFactory<?, ?> forName(String factoryName, long nativeHandle) {
      if ("HLAinteger64Time".equals(factoryName)) return new Integer64Factory(nativeHandle);
      if ("HLAfloat64Time".equals(factoryName)) return new Float64Factory(nativeHandle);
      throw new IllegalArgumentException("Unsupported C++ logical-time factory: " + factoryName);
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
      if (value instanceof HLAinteger64Time || value instanceof HLAinteger64Interval) {
         return "HLAinteger64Time";
      }
      if (value instanceof HLAfloat64Time || value instanceof HLAfloat64Interval) {
         return "HLAfloat64Time";
      }
      throw new IllegalArgumentException(
         "Unsupported standard logical-time implementation: " + value.getClass().getName());
   }

   private static final class Carrier implements InvocationHandler {
      private final long nativeHandle;
      private final boolean floatingPoint;
      private final boolean time;
      private long integerValue;
      private double floatingValue;

      Carrier(long nativeHandle, boolean time, long value) {
         this.nativeHandle = nativeHandle;
         this.floatingPoint = false;
         this.time = time;
         this.integerValue = value;
         this.floatingValue = 0.0d;
      }

      Carrier(long nativeHandle, boolean time, double value) {
         this.nativeHandle = nativeHandle;
         this.floatingPoint = true;
         this.time = time;
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
            ? new Carrier(nativeHandle, resultTime, buffer.getDouble())
            : new Carrier(nativeHandle, resultTime, buffer.getLong());
      }

      Object proxy() {
         Class<?> type;
         if (floatingPoint) type = time ? HLAfloat64Time.class : HLAfloat64Interval.class;
         else type = time ? HLAinteger64Time.class : HLAinteger64Interval.class;
         return Proxy.newProxyInstance(type.getClassLoader(), new Class<?>[] { type }, this);
      }

      @Override public Object invoke(Object proxy, Method method, Object[] arguments) throws Throwable {
         Object[] values = arguments == null ? new Object[0] : arguments;
         String name = method.getName();
         if ("getValue".equals(name) && values.length == 0) {
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
            Carrier result = arithmeticResult(
               false,
               NativeBridge.nativeDifferenceLogicalTime(
                  nativeHandle, minuend.encoded(), subtrahend.encoded())
            );
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
   }

   private static final class Integer64Factory implements HLAinteger64TimeFactory {
      private final long nativeHandle;

      Integer64Factory(long nativeHandle) { this.nativeHandle = nativeHandle; }

      @Override public HLAinteger64Time decodeTime(byte[] encoded, int offset) {
         return (HLAinteger64Time) new Carrier(
            nativeHandle, true,
            decodeLong(nativeDecode(nativeHandle, encoded, offset, false))).proxy();
      }
      @Override public HLAinteger64Interval decodeInterval(byte[] encoded, int offset) {
         return (HLAinteger64Interval) new Carrier(
            nativeHandle, false,
            decodeLong(nativeDecode(nativeHandle, encoded, offset, true))).proxy();
      }
      @Override public HLAinteger64Time makeInitial() { return makeTime(0L); }
      @Override public HLAinteger64Time makeFinal() { return makeTime(Long.MAX_VALUE); }
      @Override public HLAinteger64Time makeTime(long value) {
         return (HLAinteger64Time) new Carrier(
            nativeHandle, true,
            decodeLong(NativeBridge.nativeMakeIntegerLogicalTime(nativeHandle, value))).proxy();
      }
      @Override public HLAinteger64Interval makeZero() { return makeInterval(0L); }
      @Override public HLAinteger64Interval makeEpsilon() { return makeInterval(1L); }
      @Override public HLAinteger64Interval makeInterval(long value) {
         return (HLAinteger64Interval) new Carrier(
            nativeHandle, false,
            decodeLong(NativeBridge.nativeMakeIntegerLogicalTimeInterval(nativeHandle, value))).proxy();
      }
      @Override public String getName() { return "HLAinteger64Time"; }
   }

   private static final class Float64Factory implements HLAfloat64TimeFactory {
      private final long nativeHandle;

      Float64Factory(long nativeHandle) { this.nativeHandle = nativeHandle; }

      @Override public HLAfloat64Time decodeTime(byte[] encoded, int offset) {
         return (HLAfloat64Time) new Carrier(
            nativeHandle, true,
            decodeDouble(nativeDecode(nativeHandle, encoded, offset, false))).proxy();
      }
      @Override public HLAfloat64Interval decodeInterval(byte[] encoded, int offset) {
         return (HLAfloat64Interval) new Carrier(
            nativeHandle, false,
            decodeDouble(nativeDecode(nativeHandle, encoded, offset, true))).proxy();
      }
      @Override public HLAfloat64Time makeInitial() { return makeTime(0.0d); }
      @Override public HLAfloat64Time makeFinal() { return makeTime(Double.MAX_VALUE); }
      @Override public HLAfloat64Time makeTime(double value) {
         return (HLAfloat64Time) new Carrier(
            nativeHandle, true,
            decodeDouble(NativeBridge.nativeMakeFloatLogicalTime(nativeHandle, value))).proxy();
      }
      @Override public HLAfloat64Interval makeZero() { return makeInterval(0.0d); }
      @Override public HLAfloat64Interval makeEpsilon() { return makeInterval(Double.MIN_VALUE); }
      @Override public HLAfloat64Interval makeInterval(double value) {
         return (HLAfloat64Interval) new Carrier(
            nativeHandle, false,
            decodeDouble(NativeBridge.nativeMakeFloatLogicalTimeInterval(nativeHandle, value))).proxy();
      }
      @Override public String getName() { return "HLAfloat64Time"; }
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
