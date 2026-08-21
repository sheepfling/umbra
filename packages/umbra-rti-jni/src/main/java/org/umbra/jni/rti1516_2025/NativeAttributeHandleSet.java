package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import java.util.HashSet;

/** Standard Java collection carrier for copied attribute-handle values. */
final class NativeAttributeHandleSet extends HashSet<AttributeHandle> implements AttributeHandleSet {
   private static final long serialVersionUID = 1L;

   @Override public NativeAttributeHandleSet clone() {
      NativeAttributeHandleSet copy = new NativeAttributeHandleSet();
      copy.addAll(this);
      return copy;
   }
}
