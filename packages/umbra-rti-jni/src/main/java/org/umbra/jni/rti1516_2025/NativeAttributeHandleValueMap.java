package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.encoding.ByteWrapper;
import java.util.HashMap;

/** Standard Java carrier for copied attribute-handle/value pairs. */
final class NativeAttributeHandleValueMap
   extends HashMap<AttributeHandle, byte[]>
   implements AttributeHandleValueMap {
   private static final long serialVersionUID = 1L;

   NativeAttributeHandleValueMap() { }

   NativeAttributeHandleValueMap(int capacity) { super(capacity); }

   @Override public NativeAttributeHandleValueMap clone() {
      NativeAttributeHandleValueMap copy = new NativeAttributeHandleValueMap(size());
      forEach((key, value) -> copy.put(key, value == null ? null : value.clone()));
      return copy;
   }

   @Override public ByteWrapper getValueReference(AttributeHandle handle) {
      byte[] value = get(handle);
      return value == null ? null : new ByteWrapper(value);
   }

   @Override public ByteWrapper getValueReference(AttributeHandle handle, ByteWrapper wrapper) {
      byte[] value = get(handle);
      if (value == null) return null;
      if (wrapper == null) return new ByteWrapper(value);
      wrapper.reassign(value, 0, value.length);
      return wrapper;
   }
}
