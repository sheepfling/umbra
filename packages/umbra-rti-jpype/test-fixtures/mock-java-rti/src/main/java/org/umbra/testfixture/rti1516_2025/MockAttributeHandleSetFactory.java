package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleSetFactory;

/** Fixture factory used by the adapter's standard Java construction path. */
public final class MockAttributeHandleSetFactory implements AttributeHandleSetFactory {
   @Override
   public AttributeHandleSet create() {
      return new MockAttributeHandleSet();
   }
}
