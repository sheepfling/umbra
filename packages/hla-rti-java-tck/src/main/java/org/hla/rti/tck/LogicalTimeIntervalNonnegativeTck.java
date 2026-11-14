package org.hla.rti.tck;

import hla.rti1516_2025.exceptions.InvalidLogicalTimeInterval;
import hla.rti1516_2025.time.HLAfloat64Interval;
import hla.rti1516_2025.time.HLAfloat64TimeFactory;
import hla.rti1516_2025.time.HLAinteger64Interval;
import hla.rti1516_2025.time.HLAinteger64TimeFactory;
import hla.rti1516_2025.time.LogicalTimeFactoryFactory;
import java.util.ArrayList;
import java.util.List;

/** Verifies nonnegative interval creation through the official 2025 Java API. */
final class LogicalTimeIntervalNonnegativeTck {
   private LogicalTimeIntervalNonnegativeTck() { }

   static void run() throws Exception {
      HLAinteger64TimeFactory integerFactory =
         LogicalTimeFactoryFactory.getLogicalTimeFactory(HLAinteger64TimeFactory.class);
      HLAfloat64TimeFactory floatFactory =
         LogicalTimeFactoryFactory.getLogicalTimeFactory(HLAfloat64TimeFactory.class);
      JavaTckSupport.check(integerFactory != null,
         "standard HLAinteger64Time factory is unavailable");
      JavaTckSupport.check(floatFactory != null,
         "standard HLAfloat64Time factory is unavailable");

      List<String> failures = new ArrayList<>();
      verifyInteger64(integerFactory, failures);
      verifyFloat64(floatFactory, failures);
      JavaTckSupport.check(failures.isEmpty(), String.join("; ", failures));
   }

   private static void verifyInteger64(HLAinteger64TimeFactory factory,
         List<String> failures) throws Exception {
      JavaTckSupport.check(HLAinteger64TimeFactory.NAME.equals(factory.getName()),
         "integer64 factory reported a different standard representation name");
      HLAinteger64Interval zero = factory.makeInterval(0);
      HLAinteger64Interval positive = factory.makeInterval(1);
      JavaTckSupport.check(zero.getValue() == 0 && positive.getValue() == 1,
         "integer64 factory did not preserve zero and positive interval values");
      expectNegativeRejected(() -> factory.makeInterval(-1),
         "negative integer64 logical-time interval", failures);
   }

   private static void verifyFloat64(HLAfloat64TimeFactory factory,
         List<String> failures) throws Exception {
      JavaTckSupport.check(HLAfloat64TimeFactory.NAME.equals(factory.getName()),
         "float64 factory reported a different standard representation name");
      HLAfloat64Interval zero = factory.makeInterval(0.0);
      HLAfloat64Interval positive = factory.makeInterval(1.0);
      JavaTckSupport.check(zero.getValue() == 0.0 && positive.getValue() == 1.0,
         "float64 factory did not preserve zero and positive interval values");
      expectNegativeRejected(() -> factory.makeInterval(-1.0),
         "negative float64 logical-time interval", failures);
   }

   private static void expectNegativeRejected(JavaTckSupport.CheckedOperation operation,
         String description, List<String> failures) throws Exception {
      try {
         operation.run();
      } catch (Throwable error) {
         if (hasExpectedRejection(error)) return;
         failures.add(description + " failed with an unexpected exception: " + error);
         return;
      }
      failures.add(description + " was accepted");
   }

   private static boolean hasExpectedRejection(Throwable error) {
      for (Throwable current = error; current != null; current = current.getCause()) {
         if (current instanceof InvalidLogicalTimeInterval
               || current instanceof IllegalArgumentException) {
            return true;
         }
      }
      return false;
   }
}
