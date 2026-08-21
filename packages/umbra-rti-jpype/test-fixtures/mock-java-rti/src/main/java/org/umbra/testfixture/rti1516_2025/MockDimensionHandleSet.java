package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import java.util.LinkedHashSet;
import java.util.Set;

public final class MockDimensionHandleSet extends LinkedHashSet<DimensionHandle>
   implements DimensionHandleSet {
   @Override public MockDimensionHandleSet clone() {
      MockDimensionHandleSet copy = new MockDimensionHandleSet();
      copy.addAll(this);
      return copy;
   }
}
