package org.umbra.jni.rti1516e;

import hla.rti1516e.AttributeHandle;
import hla.rti1516e.DimensionHandle;
import hla.rti1516e.FederateHandle;
import hla.rti1516e.InteractionClassHandle;
import hla.rti1516e.MessageRetractionHandle;
import hla.rti1516e.ObjectClassHandle;
import hla.rti1516e.ObjectInstanceHandle;
import hla.rti1516e.ParameterHandle;
import hla.rti1516e.RegionHandle;
import hla.rti1516e.TransportationTypeHandle;
import java.io.Serializable;
import java.util.Arrays;

/** Standard-interface handle carrier used only by the JNI type probe. */
public final class NativeHandleCarrier implements Serializable,
   FederateHandle, ObjectClassHandle, InteractionClassHandle,
   ObjectInstanceHandle, AttributeHandle, ParameterHandle, DimensionHandle,
   MessageRetractionHandle, RegionHandle, TransportationTypeHandle {
   private static final long serialVersionUID = 1L;

   private final String kind;
   private final byte[] encoded;

   NativeHandleCarrier(String kind, byte[] encoded) {
      this.kind = kind;
      this.encoded = encoded.clone();
   }

   public String kind() {
      return kind;
   }

   public byte[] encodedValue() {
      return encoded.clone();
   }

   @Override
   public int encodedLength() {
      return encoded.length;
   }

   @Override
   public void encode(byte[] buffer, int offset) {
      if (buffer == null || offset < 0 || buffer.length - offset < encoded.length) {
         throw new IllegalArgumentException("handle output buffer is too small");
      }
      System.arraycopy(encoded, 0, buffer, offset, encoded.length);
   }

   @Override
   public boolean equals(Object other) {
      if (!(other instanceof NativeHandleCarrier)) {
         return false;
      }
      NativeHandleCarrier value = (NativeHandleCarrier) other;
      return kind.equals(value.kind) && Arrays.equals(encoded, value.encoded);
   }

   @Override
   public int hashCode() {
      return 31 * kind.hashCode() + Arrays.hashCode(encoded);
   }

   @Override
   public String toString() {
      return kind + "(" + toHex(encoded) + ")";
   }

   private static String toHex(byte[] value) {
      StringBuilder result = new StringBuilder(value.length * 2);
      for (byte item : value) {
         result.append(String.format("%02x", item & 0xff));
      }
      return result.toString();
   }
}
