package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.DimensionHandleSetFactory;

/** Factory for standard Java dimension-handle set carriers. */
final class NativeDimensionHandleSetFactory implements DimensionHandleSetFactory {
   @Override
   public DimensionHandleSet create() {
      return new NativeDimensionHandleSet();
   }
}
