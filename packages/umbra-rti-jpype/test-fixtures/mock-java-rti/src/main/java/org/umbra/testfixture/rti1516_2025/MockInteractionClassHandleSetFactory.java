package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.InteractionClassHandleSet;
import hla.rti1516_2025.InteractionClassHandleSetFactory;

/** Fixture factory for typed interaction-class handle sets. */
public final class MockInteractionClassHandleSetFactory implements InteractionClassHandleSetFactory {
   @Override public InteractionClassHandleSet create() { return new MockInteractionClassHandleSet(); }
}
