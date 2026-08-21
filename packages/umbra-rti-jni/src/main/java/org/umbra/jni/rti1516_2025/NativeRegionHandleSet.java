package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import java.util.HashSet;

/** Standard Java collection carrier for copied region-handle values. */
final class NativeRegionHandleSet extends HashSet<RegionHandle> implements RegionHandleSet {
   private static final long serialVersionUID = 1L;

   @Override public NativeRegionHandleSet clone() {
      NativeRegionHandleSet copy = new NativeRegionHandleSet();
      copy.addAll(this);
      return copy;
   }
}
