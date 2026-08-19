package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.AttributeHandleFactory;
import hla.rti1516_2025.DimensionHandleFactory;
import hla.rti1516_2025.InteractionClassHandleFactory;
import hla.rti1516_2025.ObjectClassHandleFactory;
import hla.rti1516_2025.ObjectInstanceHandleFactory;
import hla.rti1516_2025.ParameterHandleFactory;
import hla.rti1516_2025.RegionHandleFactory;
import hla.rti1516_2025.TransportationTypeHandleFactory;

/** One typed fixture decoder used through each standard factory accessor. */
public final class MockSupportHandleFactory implements
   ObjectClassHandleFactory,
   ObjectInstanceHandleFactory,
   AttributeHandleFactory,
   InteractionClassHandleFactory,
   ParameterHandleFactory,
   TransportationTypeHandleFactory,
   DimensionHandleFactory,
   RegionHandleFactory
{
   @Override
   public MockSupportHandle decode(byte[] buffer, int offset) {
      return new MockSupportHandle(buffer, offset);
   }
}
