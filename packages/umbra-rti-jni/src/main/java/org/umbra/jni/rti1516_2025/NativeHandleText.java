package org.umbra.jni.rti1516_2025;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/** Formats Umbra's public handles with the same diagnostic text as the C++ binding. */
final class NativeHandleText {
   private static final int IDENTITY_LENGTH = 8;
   private static final int HANDLE_KIND_LENGTH = 1;
   private static final int ENCODED_PAYLOAD_LENGTH = HANDLE_KIND_LENGTH + IDENTITY_LENGTH;
   private static final int ENCODED_LENGTH = Integer.BYTES + ENCODED_PAYLOAD_LENGTH;

   private NativeHandleText() {
   }

   static String format(String handleType, byte[] encoded) {
      if (encoded == null || encoded.length != ENCODED_LENGTH) {
         throw new IllegalArgumentException("Invalid encoded " + handleType);
      }
      ByteBuffer value = ByteBuffer.wrap(encoded).order(ByteOrder.BIG_ENDIAN);
      if (value.getInt() != ENCODED_PAYLOAD_LENGTH) {
         throw new IllegalArgumentException("Invalid encoded " + handleType);
      }
      // The encoded handle carries a kind discriminator before its common identity.
      int handleKind = Byte.toUnsignedInt(value.get());
      if (handleKind == 0 || value.remaining() != IDENTITY_LENGTH) {
         throw new IllegalArgumentException("Invalid encoded " + handleType);
      }
      long identity = value.getLong();
      return identity == 0L
         ? handleType + "(invalid)"
         : handleType + "(" + Long.toUnsignedString(identity) + ")";
   }
}
