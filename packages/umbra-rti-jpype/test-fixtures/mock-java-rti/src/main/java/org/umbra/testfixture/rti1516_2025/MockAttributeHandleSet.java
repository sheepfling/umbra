package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import java.util.LinkedHashSet;

/** Fixture collection whose elements retain their actual Java handle type. */
public final class MockAttributeHandleSet extends LinkedHashSet<AttributeHandle>
   implements AttributeHandleSet
{
   private static final long serialVersionUID = 1L;
}
