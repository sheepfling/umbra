package hla.rti1516_2025;

import java.util.Set;

/** Test-fixture subset of the standard mutable Java dimension-handle set. */
public interface DimensionHandleSet extends Set<DimensionHandle>, Cloneable {
   DimensionHandleSet clone();
}
