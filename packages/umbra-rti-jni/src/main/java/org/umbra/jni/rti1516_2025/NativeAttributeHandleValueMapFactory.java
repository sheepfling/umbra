package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.AttributeHandleValueMapFactory;

/** Factory for the standard Java attribute-value map carrier. */
final class NativeAttributeHandleValueMapFactory implements AttributeHandleValueMapFactory {
   @Override
   public AttributeHandleValueMap create(int capacity) {
      return new NativeAttributeHandleValueMap(capacity);
   }
}
