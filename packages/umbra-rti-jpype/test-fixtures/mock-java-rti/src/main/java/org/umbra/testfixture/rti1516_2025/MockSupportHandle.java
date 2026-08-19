package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.TransportationTypeHandle;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

/**
 * A deterministic value that models the common byte-oriented behavior of the
 * six support-service handle domains exercised by this fixture.  Production
 * Java RTIs retain their own distinct opaque handle implementations.
 */
public final class MockSupportHandle implements
   ObjectClassHandle,
   ObjectInstanceHandle,
   AttributeHandle,
   InteractionClassHandle,
   ParameterHandle,
   TransportationTypeHandle,
   DimensionHandle,
   RegionHandle,
   MessageRetractionHandle
{
   private final byte[] encoded;

   public MockSupportHandle(String value) {
      encoded = value.getBytes(StandardCharsets.UTF_8);
   }

   public MockSupportHandle(byte[] source, int offset) {
      encoded = new byte[source.length - offset];
      System.arraycopy(source, offset, encoded, 0, encoded.length);
   }

   @Override
   public int encodedLength() { return encoded.length; }

   @Override
   public void encode(byte[] buffer, int offset) {
      System.arraycopy(encoded, 0, buffer, offset, encoded.length);
   }

   @Override
   public boolean equals(Object other) {
      return other instanceof MockSupportHandle
         && Arrays.equals(encoded, ((MockSupportHandle) other).encoded);
   }

   @Override
   public int hashCode() { return Arrays.hashCode(encoded); }
}
