package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.FederationExecutionInformation;
import hla.rti1516_2025.FederationExecutionInformationSet;
import java.util.HashSet;

/** Private mutable implementation needed only by the Java fixture. */
final class MockFederationExecutionInformationSet
   extends HashSet<FederationExecutionInformation>
   implements FederationExecutionInformationSet {

   @Override
   public FederationExecutionInformationSet clone() {
      MockFederationExecutionInformationSet copy = new MockFederationExecutionInformationSet();
      copy.addAll(this);
      return copy;
   }
}
