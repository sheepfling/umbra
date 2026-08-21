package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleFactory;

/** Decoder for C++-validated encoded {@link FederateHandle} values. */
final class NativeFederateHandleFactory implements FederateHandleFactory {
   @Override
   public FederateHandle decode(byte[] buffer, int offset) {
      if (buffer == null || offset < 0 || offset > buffer.length) {
         throw new IndexOutOfBoundsException("Invalid FederateHandle decode buffer or offset");
      }
      byte[] value = new byte[buffer.length - offset];
      System.arraycopy(buffer, offset, value, 0, value.length);
      return new NativeFederateHandle(NativeBridge.nativeDecodeFederateHandle(value));
   }
}
