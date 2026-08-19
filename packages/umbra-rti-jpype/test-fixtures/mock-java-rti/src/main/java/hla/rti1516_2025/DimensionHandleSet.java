package hla.rti1516_2025;

/** Test-fixture subset of the standard mutable Java dimension-handle set. */
public interface DimensionHandleSet extends Iterable<DimensionHandle> {
   boolean add(DimensionHandle dimension);
}
