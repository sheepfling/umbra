package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleSet;
import java.util.LinkedHashSet;

/** Standard typed collection carrier for directed-interaction services. */
final class NativeInteractionClassHandleSet extends LinkedHashSet<InteractionClassHandle>
   implements InteractionClassHandleSet {
   @Override public NativeInteractionClassHandleSet clone() {
      NativeInteractionClassHandleSet result = new NativeInteractionClassHandleSet();
      result.addAll(this);
      return result;
   }
}
