package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.time.HLAinteger64Interval;
import hla.rti1516_2025.time.HLAinteger64Time;
import hla.rti1516_2025.time.HLAinteger64TimeFactory;

/** Standard ServiceLoader entry point for the bounded integer logical-time carrier. */
public final class NativeInteger64TimeFactory implements HLAinteger64TimeFactory {
   private final Object delegate;

   public NativeInteger64TimeFactory() {
      delegate = NativeLogicalTimeFactories.forServiceLoader("HLAinteger64Time");
   }

   @Override
   public HLAinteger64Time decodeTime(byte[] buffer, int offset) {
      return (HLAinteger64Time) NativeLogicalTimeFactories.invokeProvider(
         delegate, "decodeTime", new Class<?>[] { byte[].class, int.class }, buffer, offset);
   }

   @Override
   public HLAinteger64Interval decodeInterval(byte[] buffer, int offset) {
      return (HLAinteger64Interval) NativeLogicalTimeFactories.invokeProvider(
         delegate, "decodeInterval", new Class<?>[] { byte[].class, int.class }, buffer, offset);
   }

   @Override
   public HLAinteger64Time makeInitial() {
      return (HLAinteger64Time) NativeLogicalTimeFactories.invokeProvider(
         delegate, "makeInitial", new Class<?>[0]);
   }

   @Override
   public HLAinteger64Time makeFinal() {
      return (HLAinteger64Time) NativeLogicalTimeFactories.invokeProvider(
         delegate, "makeFinal", new Class<?>[0]);
   }

   @Override
   public HLAinteger64Time makeTime(long value) {
      return (HLAinteger64Time) NativeLogicalTimeFactories.invokeProvider(
         delegate, "makeTime", new Class<?>[] { long.class }, value);
   }

   @Override
   public HLAinteger64Interval makeZero() {
      return (HLAinteger64Interval) NativeLogicalTimeFactories.invokeProvider(
         delegate, "makeZero", new Class<?>[0]);
   }

   @Override
   public HLAinteger64Interval makeEpsilon() {
      return (HLAinteger64Interval) NativeLogicalTimeFactories.invokeProvider(
         delegate, "makeEpsilon", new Class<?>[0]);
   }

   @Override
   public HLAinteger64Interval makeInterval(long value) {
      return (HLAinteger64Interval) NativeLogicalTimeFactories.invokeProvider(
         delegate, "makeInterval", new Class<?>[] { long.class }, value);
   }

   @Override
   public String getName() {
      return (String) NativeLogicalTimeFactories.invokeProvider(
         delegate, "getName", new Class<?>[0]);
   }
}
