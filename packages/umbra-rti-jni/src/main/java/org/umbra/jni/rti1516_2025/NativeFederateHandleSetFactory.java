package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.FederateHandleSet;
import hla.rti1516_2025.FederateHandleSetFactory;

/** Factory for standard Java federate-handle set carriers. */
final class NativeFederateHandleSetFactory implements FederateHandleSetFactory {
   @Override
   public FederateHandleSet create() {
      return new NativeFederateHandleSet();
   }
}
