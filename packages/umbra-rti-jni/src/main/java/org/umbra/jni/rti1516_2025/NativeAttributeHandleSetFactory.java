package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleSetFactory;

/** Factory for standard Java attribute-handle set carriers. */
final class NativeAttributeHandleSetFactory implements AttributeHandleSetFactory {
   @Override
   public AttributeHandleSet create() {
      return new NativeAttributeHandleSet();
   }
}
