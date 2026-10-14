package org.hla.rti.tck;

import hla.rti1516_2025.exceptions.CouldNotDecode;
import hla.rti1516_2025.exceptions.IllegalTimeArithmetic;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.util.Arrays;

/** Exercises logical-time values using only the generic IEEE 1516.1-2025 Java API. */
final class LogicalTimeValueTck {
   private static final byte SENTINEL = (byte) 0x5a;

   private LogicalTimeValueTck() { }

   static <T extends LogicalTime<T, U>, U extends LogicalTimeInterval<U>>
         void verify(LogicalTimeFactory<T, U> factory) throws Exception {
      T initial = factory.makeInitial();
      T finalTime = factory.makeFinal();
      U zero = factory.makeZero();
      U epsilon = factory.makeEpsilon();
      JavaTckSupport.check(initial.isInitial() && finalTime.isFinal(),
         "logical-time factory returned incorrect boundary values");
      JavaTckSupport.check(zero.isZero() && epsilon.isEpsilon(),
         "logical-time factory returned incorrect interval boundary values");

      verifyTimeRoundTrip(factory, initial, "initial time");
      verifyTimeRoundTrip(factory, finalTime, "final time");
      verifyIntervalRoundTrip(factory, zero, "zero interval");
      verifyIntervalRoundTrip(factory, epsilon, "epsilon interval");

      T oneEpsilonAfterInitial = initial.add(epsilon);
      T twoEpsilonAfterInitial = oneEpsilonAfterInitial.add(epsilon);
      JavaTckSupport.check(oneEpsilonAfterInitial.compareTo(initial) > 0
            && twoEpsilonAfterInitial.compareTo(oneEpsilonAfterInitial) > 0
            && twoEpsilonAfterInitial.distance(oneEpsilonAfterInitial).equals(epsilon),
         "logical-time ordering or distance did not preserve epsilon advancement");
      JavaTckSupport.check(epsilon.compareTo(zero) > 0
            && zero.add(epsilon).equals(epsilon)
            && epsilon.subtract(epsilon).equals(zero),
         "logical-time interval ordering or arithmetic was inconsistent");
      expectIllegalTimeArithmetic(() -> initial.subtract(epsilon),
         "subtracting epsilon before initial time");
      expectIllegalTimeArithmetic(() -> finalTime.add(epsilon),
         "adding epsilon after final time");
      expectDecodeFailure(() -> factory.decodeTime(new byte[0], 0), "empty logical time");
      expectDecodeFailure(() -> factory.decodeInterval(new byte[0], 0),
         "empty logical-time interval");
   }

   private static <T extends LogicalTime<T, U>, U extends LogicalTimeInterval<U>>
         void verifyTimeRoundTrip(LogicalTimeFactory<T, U> factory, T value, String name)
         throws Exception {
      int length = value.encodedLength();
      JavaTckSupport.check(length > 0, name + " has no encoded representation");
      byte[] buffer = new byte[length + 2];
      Arrays.fill(buffer, SENTINEL);
      value.encode(buffer, 1);
      JavaTckSupport.check(buffer[0] == SENTINEL && buffer[length + 1] == SENTINEL,
         name + " encoding wrote outside its requested buffer region");
      JavaTckSupport.check(value.equals(factory.decodeTime(buffer, 1)),
         name + " did not round-trip through the logical-time factory");
   }

   private static <T extends LogicalTime<T, U>, U extends LogicalTimeInterval<U>>
         void verifyIntervalRoundTrip(LogicalTimeFactory<T, U> factory, U value, String name)
         throws Exception {
      int length = value.encodedLength();
      JavaTckSupport.check(length > 0, name + " has no encoded representation");
      byte[] buffer = new byte[length + 2];
      Arrays.fill(buffer, SENTINEL);
      value.encode(buffer, 1);
      JavaTckSupport.check(buffer[0] == SENTINEL && buffer[length + 1] == SENTINEL,
         name + " encoding wrote outside its requested buffer region");
      JavaTckSupport.check(value.equals(factory.decodeInterval(buffer, 1)),
         name + " did not round-trip through the logical-time factory");
   }

   private static void expectIllegalTimeArithmetic(CheckedOperation operation, String name)
         throws Exception {
      try {
         operation.run();
      } catch (IllegalTimeArithmetic expected) {
         return;
      }
      throw new AssertionError(name + " was not rejected with IllegalTimeArithmetic");
   }

   private static void expectDecodeFailure(CheckedOperation operation, String name)
         throws Exception {
      try {
         operation.run();
      } catch (CouldNotDecode expected) {
         return;
      }
      throw new AssertionError(name + " was not rejected with CouldNotDecode");
   }

   @FunctionalInterface
   private interface CheckedOperation { void run() throws Exception; }
}
