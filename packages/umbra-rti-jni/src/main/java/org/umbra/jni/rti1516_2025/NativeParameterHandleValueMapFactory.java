package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ParameterHandleValueMapFactory;

/** Factory for the standard Java parameter-value map carrier. */
final class NativeParameterHandleValueMapFactory implements ParameterHandleValueMapFactory {
   @Override
   public ParameterHandleValueMap create(int capacity) {
      return new NativeParameterHandleValueMap(capacity);
   }
}
