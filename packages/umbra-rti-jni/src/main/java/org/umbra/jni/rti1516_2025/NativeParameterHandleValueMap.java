package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.encoding.ByteWrapper;
import java.util.HashMap;

/** Standard Java carrier for copied parameter-handle/value pairs. */
final class NativeParameterHandleValueMap
   extends HashMap<ParameterHandle, byte[]>
   implements ParameterHandleValueMap {
   private static final long serialVersionUID = 1L;

   NativeParameterHandleValueMap() { }

   NativeParameterHandleValueMap(int capacity) { super(capacity); }

   @Override public NativeParameterHandleValueMap clone() {
      NativeParameterHandleValueMap copy = new NativeParameterHandleValueMap(size());
      forEach((key, value) -> copy.put(key, value == null ? null : value.clone()));
      return copy;
   }

   @Override public ByteWrapper getValueReference(ParameterHandle handle) {
      byte[] value = get(handle);
      return value == null ? null : new ByteWrapper(value);
   }

   @Override public ByteWrapper getValueReference(ParameterHandle handle, ByteWrapper wrapper) {
      byte[] value = get(handle);
      if (value == null) return null;
      if (wrapper == null) return new ByteWrapper(value);
      wrapper.reassign(value, 0, value.length);
      return wrapper;
   }
}
