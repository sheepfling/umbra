package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.FederateHandle;
import java.nio.charset.StandardCharsets;

/** Fixture-only deterministic encoded handle. */
public final class MockFederateHandle implements FederateHandle {
   private final byte[] encoded;

   public MockFederateHandle(String value) {
      encoded = value.getBytes(StandardCharsets.UTF_8);
   }

   public MockFederateHandle(byte[] source, int offset) {
      encoded = new byte[source.length - offset];
      System.arraycopy(source, offset, encoded, 0, encoded.length);
   }

   public String value() { return new String(encoded, StandardCharsets.UTF_8); }

   @Override
   public int encodedLength() { return encoded.length; }

   @Override
   public void encode(byte[] buffer, int offset) {
      System.arraycopy(encoded, 0, buffer, offset, encoded.length);
   }
}
