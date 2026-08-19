package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.Set;

public final class MockRegionHandleSet implements RegionHandleSet {
   private final Set<RegionHandle> values = new LinkedHashSet<>();
   @Override public boolean add(RegionHandle value) { return values.add(value); }
   @Override public Iterator<RegionHandle> iterator() { return values.iterator(); }
}
