package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import java.util.HashSet;

/** Standard Java collection carrier for copied dimension-handle values. */
final class NativeDimensionHandleSet extends HashSet<DimensionHandle>
   implements DimensionHandleSet {
   private static final long serialVersionUID = 1L;

   @Override public NativeDimensionHandleSet clone() {
      NativeDimensionHandleSet copy = new NativeDimensionHandleSet();
      copy.addAll(this);
      return copy;
   }
}
