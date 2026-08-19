package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RegionHandleSetFactory;

public final class MockRegionHandleSetFactory implements RegionHandleSetFactory {
   @Override public RegionHandleSet create() { return new MockRegionHandleSet(); }
}
