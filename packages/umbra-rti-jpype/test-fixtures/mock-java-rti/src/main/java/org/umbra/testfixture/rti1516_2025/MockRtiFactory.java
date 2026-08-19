package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.encoding.EncoderFactory;

public final class MockRtiFactory implements RtiFactory {
   public static final String NAME = "Umbra Mock Java RTI";

   @Override
   public RTIambassador getRtiAmbassador() {
      return new MockRTIambassador();
   }

   @Override
   public EncoderFactory getEncoderFactory() {
      return new MockEncoderFactory();
   }

   @Override
   public String rtiName() {
      return NAME;
   }

   @Override
   public String rtiVersion() {
      return "2025.mock";
   }
}
