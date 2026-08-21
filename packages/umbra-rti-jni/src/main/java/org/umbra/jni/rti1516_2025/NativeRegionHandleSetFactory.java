package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RegionHandleSetFactory;

/** Factory for standard Java region-handle set carriers. */
final class NativeRegionHandleSetFactory implements RegionHandleSetFactory {
   @Override
   public RegionHandleSet create() {
      return new NativeRegionHandleSet();
   }
}
