package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleSet;
import java.util.HashSet;

/** Standard Java collection carrier for copied federate-handle values. */
final class NativeFederateHandleSet extends HashSet<FederateHandle> implements FederateHandleSet {
   private static final long serialVersionUID = 1L;

   @Override public NativeFederateHandleSet clone() {
      NativeFederateHandleSet copy = new NativeFederateHandleSet();
      copy.addAll(this);
      return copy;
   }
}
