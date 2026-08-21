package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.encoding.ByteWrapper;
import java.util.LinkedHashMap;

/** Linked map preserving the fixture's submitted attribute order. */
public final class MockAttributeHandleValueMap
   extends LinkedHashMap<AttributeHandle, byte[]>
   implements AttributeHandleValueMap {
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
   @Override public MockAttributeHandleValueMap clone() {
      MockAttributeHandleValueMap copy = new MockAttributeHandleValueMap();
      forEach((key, value) -> copy.put(key, value == null ? null : value.clone()));
      return copy;
   }
}
