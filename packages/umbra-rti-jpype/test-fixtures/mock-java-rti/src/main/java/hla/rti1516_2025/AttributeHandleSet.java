package hla.rti1516_2025;

import java.util.Set;

/** Test-fixture subset of the standard mutable Java attribute-handle set. */
public interface AttributeHandleSet extends Set<AttributeHandle>, Cloneable {
   AttributeHandleSet clone();
}
