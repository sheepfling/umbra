package org.hla.rti.tck;

import hla.rti1516_2025.exceptions.CouldNotDecode;
import hla.rti1516_2025.time.HLAinteger64Interval;
import hla.rti1516_2025.time.HLAinteger64Time;
import hla.rti1516_2025.time.HLAinteger64TimeFactory;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeFactoryFactory;

/** Java service-provider fixture for the federation-supplied custom TRADT case. */
public final class TckTaggedTimeFactory implements HLAinteger64TimeFactory {
   public static final String NAME = "TckTaggedTime";

   public TckTaggedTimeFactory() { }

   @Override
   public HLAinteger64Time decodeTime(byte[] buffer, int offset) throws CouldNotDecode {
      return standardFactory().decodeTime(buffer, offset);
   }

   @Override
   public HLAinteger64Interval decodeInterval(byte[] buffer, int offset)
         throws CouldNotDecode {
      return standardFactory().decodeInterval(buffer, offset);
   }

   @Override
   public HLAinteger64Time makeInitial() {
      return standardFactory().makeInitial();
   }

   @Override
   public HLAinteger64Time makeFinal() {
      return standardFactory().makeFinal();
   }

   @Override
   public HLAinteger64Interval makeZero() {
      return standardFactory().makeZero();
   }

   @Override
   public HLAinteger64Interval makeEpsilon() {
      return standardFactory().makeEpsilon();
   }

   @Override
   public HLAinteger64Time makeTime(long value) {
      return standardFactory().makeTime(value);
   }

   @Override
   public HLAinteger64Interval makeInterval(long value) {
      return standardFactory().makeInterval(value);
   }

   @Override
   public String getName() {
      return NAME;
   }

   private static HLAinteger64TimeFactory standardFactory() {
      LogicalTimeFactory<?, ?> factory =
         LogicalTimeFactoryFactory.getLogicalTimeFactory(HLAinteger64TimeFactory.NAME);
      if (!(factory instanceof HLAinteger64TimeFactory)) {
         throw new IllegalStateException("standard HLAinteger64Time factory is unavailable");
      }
      return (HLAinteger64TimeFactory) factory;
   }
}
