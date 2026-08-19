package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.AttributeHandleValueMapFactory;

/** Fixture factory used by the adapter's standard Java map construction path. */
public final class MockAttributeHandleValueMapFactory implements AttributeHandleValueMapFactory {
   @Override
   public AttributeHandleValueMap create() {
      return new MockAttributeHandleValueMap();
   }
}
