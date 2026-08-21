package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.InteractionClassHandleSet;
import hla.rti1516_2025.InteractionClassHandleSetFactory;

/** Creates the bridge's standard typed interaction-class handle sets. */
final class NativeInteractionClassHandleSetFactory implements InteractionClassHandleSetFactory {
   @Override public InteractionClassHandleSet create() {
      return new NativeInteractionClassHandleSet();
   }
}
