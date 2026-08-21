package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.encoding.EncoderFactory;

/** ServiceLoader factory for the C++-backed Umbra Java RTI façade. */
public final class NativeRtiFactory implements RtiFactory {
   public static final String NAME = "Umbra JNI C++ RTI";

   @Override
   public RTIambassador getRtiAmbassador() {
      return NativeRTIambassador.create();
   }

   @Override
   public EncoderFactory getEncoderFactory() {
      return NativeEncoderFactory.create();
   }

   @Override
   public String rtiName() {
      return NAME;
   }

   @Override
   public String rtiVersion() {
      return "2025.jni-smoke";
   }
}
