package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.FederationExecutionMemberInformation;
import hla.rti1516_2025.FederationExecutionMemberInformationSet;
import java.util.LinkedHashSet;

/** JNI-owned concrete realization for a standard member-report callback value. */
public final class NativeFederationExecutionMemberInformationSet
   extends LinkedHashSet<FederationExecutionMemberInformation>
   implements FederationExecutionMemberInformationSet {

   @Override
   public NativeFederationExecutionMemberInformationSet clone() {
      NativeFederationExecutionMemberInformationSet copy =
         new NativeFederationExecutionMemberInformationSet();
      copy.addAll(this);
      return copy;
   }
}
