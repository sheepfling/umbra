package hla.rti1516_2025;

import java.util.Map;
import hla.rti1516_2025.encoding.ByteWrapper;

/** Fixture subset of the standard parameter-handle/value map contract. */
public interface ParameterHandleValueMap extends Map<ParameterHandle, byte[]>, Cloneable {
   ByteWrapper getValueReference(ParameterHandle handle);
   ByteWrapper getValueReference(ParameterHandle handle, ByteWrapper wrapper);
   ParameterHandleValueMap clone();
}
