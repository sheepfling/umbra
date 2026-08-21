package hla.rti1516_2025;

import java.util.Set;

/** Test-fixture subset of the standard mutable Java region-handle set. */
public interface RegionHandleSet extends Set<RegionHandle>, Cloneable {
   RegionHandleSet clone();
}
