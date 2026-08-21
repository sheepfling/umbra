package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleSet;
import java.util.LinkedHashSet;

/** Fixture carrier for standard typed interaction-class handle sets. */
public final class MockInteractionClassHandleSet extends LinkedHashSet<InteractionClassHandle>
   implements InteractionClassHandleSet {
   @Override public MockInteractionClassHandleSet clone() {
      MockInteractionClassHandleSet copy = new MockInteractionClassHandleSet();
      copy.addAll(this);
      return copy;
   }
}
