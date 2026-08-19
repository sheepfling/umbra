package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleValueMap;
import java.util.LinkedHashMap;

/** Linked map preserving the fixture's submitted attribute order. */
public final class MockAttributeHandleValueMap
   extends LinkedHashMap<AttributeHandle, byte[]>
   implements AttributeHandleValueMap {
}
