package org.umbra.jni.rti1516e;

import hla.rti1516e.time.HLAinteger64Interval;
import hla.rti1516e.time.HLAinteger64Time;
import hla.rti1516e.time.HLAinteger64TimeFactory;

/** Standard ServiceLoader entry point for the bounded 2010 integer time carrier. */
public final class NativeInteger64TimeFactory implements HLAinteger64TimeFactory {
   private final Object delegate;

   public NativeInteger64TimeFactory() {
      delegate = createDelegate();
   }

   private static Object createDelegate() {
      try {
         return NativeTypeRoundTrip.logicalTimeFactoryCarrier("HLAinteger64TimeFactory");
      } catch (Exception error) {
         throw new IllegalStateException("Cannot create 2010 integer logical-time provider", error);
      }
   }

   @Override
   public HLAinteger64Time decodeTime(byte[] buffer, int offset) {
      return (HLAinteger64Time) invoke(
         "decodeTime", new Class<?>[] { byte[].class, int.class }, buffer, offset);
   }

   @Override
   public HLAinteger64Interval decodeInterval(byte[] buffer, int offset) {
      return (HLAinteger64Interval) invoke(
         "decodeInterval", new Class<?>[] { byte[].class, int.class }, buffer, offset);
   }

   @Override
   public HLAinteger64Time makeInitial() {
      return (HLAinteger64Time) invoke("makeInitial", new Class<?>[0]);
   }

   @Override
   public HLAinteger64Time makeFinal() {
      return (HLAinteger64Time) invoke("makeFinal", new Class<?>[0]);
   }

   @Override
   public HLAinteger64Time makeTime(long value) {
      return (HLAinteger64Time) invoke("makeTime", new Class<?>[] { long.class }, value);
   }

   @Override
   public HLAinteger64Interval makeZero() {
      return (HLAinteger64Interval) invoke("makeZero", new Class<?>[0]);
   }

   @Override
   public HLAinteger64Interval makeEpsilon() {
      return (HLAinteger64Interval) invoke("makeEpsilon", new Class<?>[0]);
   }

   @Override
   public HLAinteger64Interval makeInterval(long value) {
      return (HLAinteger64Interval) invoke("makeInterval", new Class<?>[] { long.class }, value);
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
         return NativeInteger64TimeFactory.<RuntimeException, Object>rethrow(cause);
      } catch (ReflectiveOperationException error) {
         throw new IllegalStateException("2010 logical-time provider is missing " + method, error);
      }
   }

   @SuppressWarnings("unchecked")
   private static <T extends Throwable, R> R rethrow(Throwable error) throws T {
      throw (T) error;
   }
}
