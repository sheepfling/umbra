package org.umbra.jni.rti1516e;

import hla.rti1516e.RTIambassador;
import hla.rti1516e.RtiFactory;
import hla.rti1516e.encoding.EncoderFactory;
import hla.rti1516e.exceptions.RTIinternalError;

/** ServiceLoader factory for the C++-backed IEEE 1516e null provider. */
public final class NativeRtiFactory implements RtiFactory {
   public static final String NAME = "Umbra JNI IEEE 1516e Null RTI";

   @Override
   public RTIambassador getRtiAmbassador() throws RTIinternalError {
      return NativeRTIambassador.create();
   }

   @Override
   public EncoderFactory getEncoderFactory() throws RTIinternalError {
      return NativeTypeRoundTrip.encoderFactoryCarrier();
   }

   @Override
   public String rtiName() {
      return NAME;
   }

   @Override
   public String rtiVersion() {
      return "0.1.0-null";
   }
}
