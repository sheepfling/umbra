package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.time.HLAfloat64Interval;
import hla.rti1516_2025.time.HLAfloat64Time;
import hla.rti1516_2025.time.HLAfloat64TimeFactory;

/** Standard ServiceLoader entry point for the bounded floating-point logical-time carrier. */
public final class NativeFloat64TimeFactory implements HLAfloat64TimeFactory {
   private final Object delegate;

   public NativeFloat64TimeFactory() {
      delegate = NativeLogicalTimeFactories.forServiceLoader("HLAfloat64Time");
   }

   @Override
   public HLAfloat64Time decodeTime(byte[] buffer, int offset) {
      return (HLAfloat64Time) NativeLogicalTimeFactories.invokeProvider(
         delegate, "decodeTime", new Class<?>[] { byte[].class, int.class }, buffer, offset);
   }

   @Override
   public HLAfloat64Interval decodeInterval(byte[] buffer, int offset) {
      return (HLAfloat64Interval) NativeLogicalTimeFactories.invokeProvider(
         delegate, "decodeInterval", new Class<?>[] { byte[].class, int.class }, buffer, offset);
   }

   @Override
   public HLAfloat64Time makeInitial() {
      return (HLAfloat64Time) NativeLogicalTimeFactories.invokeProvider(
         delegate, "makeInitial", new Class<?>[0]);
   }

   @Override
   public HLAfloat64Time makeFinal() {
      return (HLAfloat64Time) NativeLogicalTimeFactories.invokeProvider(
         delegate, "makeFinal", new Class<?>[0]);
   }

   @Override
   public HLAfloat64Time makeTime(double value) {
      return (HLAfloat64Time) NativeLogicalTimeFactories.invokeProvider(
         delegate, "makeTime", new Class<?>[] { double.class }, value);
   }

   @Override
   public HLAfloat64Interval makeZero() {
      return (HLAfloat64Interval) NativeLogicalTimeFactories.invokeProvider(
         delegate, "makeZero", new Class<?>[0]);
   }

   @Override
   public HLAfloat64Interval makeEpsilon() {
      return (HLAfloat64Interval) NativeLogicalTimeFactories.invokeProvider(
         delegate, "makeEpsilon", new Class<?>[0]);
   }

   @Override
   public HLAfloat64Interval makeInterval(double value) {
      return (HLAfloat64Interval) NativeLogicalTimeFactories.invokeProvider(
         delegate, "makeInterval", new Class<?>[] { double.class }, value);
   }

   @Override
   public String getName() {
      return (String) NativeLogicalTimeFactories.invokeProvider(
         delegate, "getName", new Class<?>[0]);
   }
}
