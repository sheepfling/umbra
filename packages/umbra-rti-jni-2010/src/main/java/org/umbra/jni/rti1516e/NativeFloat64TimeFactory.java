package org.umbra.jni.rti1516e;

import hla.rti1516e.time.HLAfloat64Interval;
import hla.rti1516e.time.HLAfloat64Time;
import hla.rti1516e.time.HLAfloat64TimeFactory;

/** Standard ServiceLoader entry point for the bounded 2010 floating-point time carrier. */
public final class NativeFloat64TimeFactory implements HLAfloat64TimeFactory {
   private final Object delegate;

   public NativeFloat64TimeFactory() {
      delegate = createDelegate();
   }

   private static Object createDelegate() {
      try {
         return NativeTypeRoundTrip.logicalTimeFactoryCarrier("HLAfloat64TimeFactory");
      } catch (Exception error) {
         throw new IllegalStateException("Cannot create 2010 floating logical-time provider", error);
      }
   }

   @Override
   public HLAfloat64Time decodeTime(byte[] buffer, int offset) {
      return (HLAfloat64Time) invoke(
         "decodeTime", new Class<?>[] { byte[].class, int.class }, buffer, offset);
   }

   @Override
   public HLAfloat64Interval decodeInterval(byte[] buffer, int offset) {
      return (HLAfloat64Interval) invoke(
         "decodeInterval", new Class<?>[] { byte[].class, int.class }, buffer, offset);
   }

   @Override
   public HLAfloat64Time makeInitial() {
      return (HLAfloat64Time) invoke("makeInitial", new Class<?>[0]);
   }

   @Override
   public HLAfloat64Time makeFinal() {
      return (HLAfloat64Time) invoke("makeFinal", new Class<?>[0]);
   }

   @Override
   public HLAfloat64Time makeTime(double value) {
      return (HLAfloat64Time) invoke("makeTime", new Class<?>[] { double.class }, value);
   }

   @Override
   public HLAfloat64Interval makeZero() {
      return (HLAfloat64Interval) invoke("makeZero", new Class<?>[0]);
   }

   @Override
   public HLAfloat64Interval makeEpsilon() {
      return (HLAfloat64Interval) invoke("makeEpsilon", new Class<?>[0]);
   }

   @Override
   public HLAfloat64Interval makeInterval(double value) {
      return (HLAfloat64Interval) invoke("makeInterval", new Class<?>[] { double.class }, value);
   }

   @Override
   public String getName() {
      return (String) invoke("getName", new Class<?>[0]);
   }

   private Object invoke(String method, Class<?>[] parameterTypes, Object... arguments) {
      try {
         return delegate.getClass().getMethod(method, parameterTypes).invoke(delegate, arguments);
      } catch (java.lang.reflect.InvocationTargetException error) {
         Throwable cause = error.getCause() == null ? error : error.getCause();
         if (cause instanceof RuntimeException) throw (RuntimeException) cause;
         if (cause instanceof Error) throw (Error) cause;
         return NativeFloat64TimeFactory.<RuntimeException, Object>rethrow(cause);
      } catch (ReflectiveOperationException error) {
         throw new IllegalStateException("2010 logical-time provider is missing " + method, error);
      }
   }

   @SuppressWarnings("unchecked")
   private static <T extends Throwable, R> R rethrow(Throwable error) throws T {
      throw (T) error;
   }
}
