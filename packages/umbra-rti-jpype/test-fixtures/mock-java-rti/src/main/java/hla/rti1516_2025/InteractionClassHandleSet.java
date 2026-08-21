package hla.rti1516_2025;

import java.util.Set;

/** Test-fixture shape of the standard typed interaction-class handle set. */
public interface InteractionClassHandleSet extends Set<InteractionClassHandle>, Cloneable {
   InteractionClassHandleSet clone();
}
