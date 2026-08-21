package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleFactory;

/** Decoder for C++-validated encoded {@link RegionHandle} values. */
final class NativeRegionHandleFactory implements RegionHandleFactory {
   @Override
   public RegionHandle decode(byte[] buffer, int offset) {
      if (buffer == null || offset < 0 || offset > buffer.length) {
         throw new IndexOutOfBoundsException("Invalid RegionHandle decode buffer or offset");
      }
      byte[] value = new byte[buffer.length - offset];
      System.arraycopy(buffer, offset, value, 0, value.length);
      return new NativeRegionHandle(NativeBridge.nativeDecodeRegionHandle(value));
   }
}
