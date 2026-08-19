package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ParameterHandleValueMapFactory;

/** Fixture factory used by the adapter's standard Java map construction path. */
public final class MockParameterHandleValueMapFactory implements ParameterHandleValueMapFactory {
   @Override
   public ParameterHandleValueMap create() {
      return new MockParameterHandleValueMap();
   }
}
