package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleSet;
import java.util.HashSet;

/** Fixture implementation used to exercise Java set conversion through JPype. */
public final class MockFederateHandleSet extends HashSet<FederateHandle>
   implements FederateHandleSet {
}
