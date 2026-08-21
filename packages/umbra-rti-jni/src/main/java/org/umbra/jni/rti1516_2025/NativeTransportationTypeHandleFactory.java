package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.TransportationTypeHandleFactory;

/** Decoder for C++-validated encoded {@link TransportationTypeHandle} values. */
final class NativeTransportationTypeHandleFactory implements TransportationTypeHandleFactory {
   private final long ambassadorHandle;

   NativeTransportationTypeHandleFactory(long ambassadorHandle) {
      this.ambassadorHandle = ambassadorHandle;
   }

   @Override
   public TransportationTypeHandle decode(byte[] buffer, int offset) {
      if (buffer == null || offset < 0 || offset > buffer.length) {
         throw new IndexOutOfBoundsException(
            "Invalid TransportationTypeHandle decode buffer or offset");
      }
      byte[] value = new byte[buffer.length - offset];
      System.arraycopy(buffer, offset, value, 0, value.length);
      return new NativeTransportationTypeHandle(
         NativeBridge.nativeDecodeTransportationTypeHandle(value));
   }

   @Override public TransportationTypeHandle getHLAdefaultReliable() {
      return new NativeTransportationTypeHandle(
         NativeBridge.nativeGetTransportationTypeHandle(ambassadorHandle, "HLAreliable"));
   }

   @Override public TransportationTypeHandle getHLAdefaultBestEffort() {
      return new NativeTransportationTypeHandle(
         NativeBridge.nativeGetTransportationTypeHandle(ambassadorHandle, "HLAbestEffort"));
   }
}
