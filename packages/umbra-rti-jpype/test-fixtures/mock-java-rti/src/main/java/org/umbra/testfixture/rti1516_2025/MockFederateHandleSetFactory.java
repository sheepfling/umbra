package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.FederateHandleSet;
import hla.rti1516_2025.FederateHandleSetFactory;

/** Fixture factory for explicit synchronization federate sets. */
public final class MockFederateHandleSetFactory implements FederateHandleSetFactory {
   @Override
   public FederateHandleSet create() {
      return new MockFederateHandleSet();
   }
}
