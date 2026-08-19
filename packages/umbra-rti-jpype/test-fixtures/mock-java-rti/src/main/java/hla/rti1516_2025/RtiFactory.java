package hla.rti1516_2025;

import hla.rti1516_2025.encoding.EncoderFactory;

public interface RtiFactory {
   RTIambassador getRtiAmbassador();

   EncoderFactory getEncoderFactory();

   String rtiName();

   String rtiVersion();
}
