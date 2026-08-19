package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import java.util.LinkedHashSet;
import java.util.Iterator;
import java.util.Set;

public final class MockDimensionHandleSet implements DimensionHandleSet {
   private final Set<DimensionHandle> values = new LinkedHashSet<>();
   @Override public boolean add(DimensionHandle value) { return values.add(value); }
   @Override public Iterator<DimensionHandle> iterator() { return values.iterator(); }
}
