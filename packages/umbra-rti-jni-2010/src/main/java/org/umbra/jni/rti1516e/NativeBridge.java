package org.umbra.jni.rti1516e;

import hla.rti1516e.encoding.DecoderException;
import hla.rti1516e.exceptions.RTIinternalError;

/** JNI entry points for the bounded IEEE 1516e provider shell. */
final class NativeBridge {
   private static final String LIBRARY_PROPERTY = "umbra.rti.jni.2010.library";

   static {
      String library = System.getProperty(LIBRARY_PROPERTY);
      if (library == null || library.isEmpty()) {
         System.loadLibrary("umbra_rti_jni_2010");
      } else {
         System.load(library);
      }
   }

   private NativeBridge() {
   }

   static native long nativeCreate() throws RTIinternalError;

   static native void nativeDestroy(long handle);

   static native void nativeConnect(
      long handle, String callbackModel, String localSettingsDesignator)
      throws RTIinternalError;

   static native void nativeDisconnect(long handle) throws RTIinternalError;

   static native void nativeUnavailable(long handle) throws RTIinternalError;

   static native void nativeThrowStandardException(String name, String message)
      throws RTIinternalError;

   /*
    * These entry points deliberately carry only standard JVM primitives and
    * byte arrays.  The Java-side probe builds the corresponding standard
    * 1516e encoders/handles/times around them, so the test is about the
    * transport boundary rather than a second, vendor-specific API.
    */
   static native byte[] nativeRoundTripBytes(byte[] value) throws RTIinternalError;

   static native String nativeRoundTripString(String value) throws RTIinternalError;

   static native byte nativeRoundTripByte(byte value);

   static native short nativeRoundTripShort(short value);

   static native int nativeRoundTripInt(int value);

   static native long nativeRoundTripLong(long value);

   static native float nativeRoundTripFloat(float value);

   static native double nativeRoundTripDouble(double value);

   static native boolean nativeRoundTripBoolean(boolean value);

   static native Object nativeRoundTripObject(Object value);

   static native byte[] nativeSeedDataElement(String kind) throws RTIinternalError;

   static native byte[] nativeRoundTripDataElement(String kind, byte[] value)
      throws RTIinternalError, DecoderException;

   static native byte[] nativeSeedHandle(String kind, long value) throws RTIinternalError;

   static native byte[] nativeRoundTripHandle(String kind, byte[] value)
      throws RTIinternalError;

   static native byte[][] nativeRoundTripHandleCollection(String kind, byte[][] values)
      throws RTIinternalError;

   static native byte[][] nativeRoundTripByteMatrix(byte[][] values)
      throws RTIinternalError;

   static native byte[] nativeSeedLogicalTime(String kind) throws RTIinternalError;

   static native byte[] nativeRoundTripLogicalTime(String kind, byte[] value)
      throws RTIinternalError;
}
