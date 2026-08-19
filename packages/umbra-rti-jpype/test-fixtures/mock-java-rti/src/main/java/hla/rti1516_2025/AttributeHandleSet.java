package hla.rti1516_2025;

/** Test-fixture subset of the standard mutable Java attribute-handle set. */
public interface AttributeHandleSet extends Iterable<AttributeHandle> {
   boolean add(AttributeHandle attribute);
}
