package org.hla.rti.tck;

import hla.rti1516_2025.time.HLAfloat64Time;
import hla.rti1516_2025.time.HLAfloat64TimeFactory;
import hla.rti1516_2025.time.HLAinteger64TimeFactory;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeFactoryFactory;

/** Verifies the standard Java logical-time factory-factory selection contract. */
final class LogicalTimeFactoryFactoryTck {
   private LogicalTimeFactoryFactoryTck() { }

   static void run() {
      LogicalTimeFactory<?, ?> defaultFactory =
         LogicalTimeFactoryFactory.getLogicalTimeFactory("");
      JavaTckSupport.check(defaultFactory != null
            && HLAfloat64TimeFactory.NAME.equals(defaultFactory.getName())
            && defaultFactory instanceof HLAfloat64TimeFactory,
         "empty factory name did not select the standard HLAfloat64Time factory");
      HLAfloat64TimeFactory defaultFloatFactory = (HLAfloat64TimeFactory) defaultFactory;
      HLAfloat64Time initial = defaultFloatFactory.makeInitial();
      JavaTckSupport.check(initial.isInitial() && initial.getValue() == 0.0,
         "the standard default logical-time factory did not produce the initial value");

      LogicalTimeFactory<?, ?> integerFactory =
         LogicalTimeFactoryFactory.getLogicalTimeFactory(HLAinteger64TimeFactory.NAME);
      JavaTckSupport.check(integerFactory != null
            && HLAinteger64TimeFactory.NAME.equals(integerFactory.getName())
            && integerFactory instanceof HLAinteger64TimeFactory,
         "factory selection did not return the standard HLAinteger64Time factory");

      LogicalTimeFactory<?, ?> floatFactory =
         LogicalTimeFactoryFactory.getLogicalTimeFactory(HLAfloat64TimeFactory.NAME);
      JavaTckSupport.check(floatFactory != null
            && HLAfloat64TimeFactory.NAME.equals(floatFactory.getName())
            && floatFactory instanceof HLAfloat64TimeFactory,
         "factory selection did not return the standard HLAfloat64Time factory");

      JavaTckSupport.check(
         LogicalTimeFactoryFactory.getLogicalTimeFactory("missing") == null,
         "logical-time factory selection accepted an unknown implementation name");
   }
}
