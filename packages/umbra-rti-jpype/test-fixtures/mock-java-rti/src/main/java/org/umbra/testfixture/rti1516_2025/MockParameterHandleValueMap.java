package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.encoding.ByteWrapper;
import java.util.LinkedHashMap;

/** Linked map preserving the fixture's submitted parameter order. */
public final class MockParameterHandleValueMap
   extends LinkedHashMap<ParameterHandle, byte[]>
   implements ParameterHandleValueMap {
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
   @Override public MockParameterHandleValueMap clone() {
      MockParameterHandleValueMap copy = new MockParameterHandleValueMap();
      forEach((key, value) -> copy.put(key, value == null ? null : value.clone()));
      return copy;
   }
}
