package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import java.util.LinkedHashSet;

public final class MockRegionHandleSet extends LinkedHashSet<RegionHandle>
   implements RegionHandleSet {
   @Override public MockRegionHandleSet clone() {
      MockRegionHandleSet copy = new MockRegionHandleSet();
      copy.addAll(this);
      return copy;
   }
}
