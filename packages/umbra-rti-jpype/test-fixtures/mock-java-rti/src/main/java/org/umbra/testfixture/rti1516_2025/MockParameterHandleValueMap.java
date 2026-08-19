package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import java.util.LinkedHashMap;

/** Linked map preserving the fixture's submitted parameter order. */
public final class MockParameterHandleValueMap
   extends LinkedHashMap<ParameterHandle, byte[]>
   implements ParameterHandleValueMap {
}
