package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.DimensionHandleSetFactory;

public final class MockDimensionHandleSetFactory implements DimensionHandleSetFactory {
   @Override public DimensionHandleSet create() { return new MockDimensionHandleSet(); }
}
