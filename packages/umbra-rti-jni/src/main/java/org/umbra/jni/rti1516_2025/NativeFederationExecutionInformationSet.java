package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionInformationSet;
import java.util.LinkedHashSet;

/** JNI-owned concrete realization for a standard reporting callback value. */
public final class NativeFederationExecutionInformationSet
   extends LinkedHashSet<FederationExecutionInformation>
   implements FederationExecutionInformationSet {

   @Override
   public NativeFederationExecutionInformationSet clone() {
      NativeFederationExecutionInformationSet copy =
         new NativeFederationExecutionInformationSet();
      copy.addAll(this);
      return copy;
   }
}
