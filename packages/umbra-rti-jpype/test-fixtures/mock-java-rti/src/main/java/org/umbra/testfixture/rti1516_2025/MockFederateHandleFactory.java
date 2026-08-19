package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleFactory;

public final class MockFederateHandleFactory implements FederateHandleFactory {
   @Override public FederateHandle decode(byte[] buffer, int offset) {
      return new MockFederateHandle(buffer, offset);
   }
}
