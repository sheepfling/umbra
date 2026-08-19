package hla.rti1516_2025;

/** Test-fixture subset of the standard mutable Java region-handle set. */
public interface RegionHandleSet extends Iterable<RegionHandle> {
   boolean add(RegionHandle region);
}
