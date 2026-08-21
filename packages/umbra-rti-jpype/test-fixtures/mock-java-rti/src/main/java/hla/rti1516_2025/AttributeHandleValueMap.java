package hla.rti1516_2025;

import java.util.Map;
import hla.rti1516_2025.encoding.ByteWrapper;

/** Fixture subset of the standard attribute-handle/value map contract. */
public interface AttributeHandleValueMap extends Map<AttributeHandle, byte[]>, Cloneable {
   ByteWrapper getValueReference(AttributeHandle handle);
   ByteWrapper getValueReference(AttributeHandle handle, ByteWrapper wrapper);
   AttributeHandleValueMap clone();
}
