package hla.rti1516_2025;

public interface RtiFactory {
   RTIambassador getRtiAmbassador();

   Object getEncoderFactory();

   String rtiName();

   String rtiVersion();
}
