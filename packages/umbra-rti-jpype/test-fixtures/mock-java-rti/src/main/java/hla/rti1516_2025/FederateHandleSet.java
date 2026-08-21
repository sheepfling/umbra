package hla.rti1516_2025;

import java.util.Set;

/** Test-fixture shape of the standard Java FederateHandleSet. */
public interface FederateHandleSet extends Set<FederateHandle>, Cloneable {
   FederateHandleSet clone();
}
