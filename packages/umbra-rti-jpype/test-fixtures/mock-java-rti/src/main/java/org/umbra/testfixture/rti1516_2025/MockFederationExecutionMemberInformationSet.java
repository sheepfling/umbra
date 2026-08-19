package org.umbra.testfixture.rti1516_2025;

import hla.rti1516_2025.FederationExecutionMemberInformation;
import hla.rti1516_2025.FederationExecutionMemberInformationSet;
import java.util.LinkedHashSet;

/** Fixture-only concrete set; it is not a vendor RTI implementation. */
public final class MockFederationExecutionMemberInformationSet
   extends LinkedHashSet<FederationExecutionMemberInformation>
   implements FederationExecutionMemberInformationSet {
   @Override
   public MockFederationExecutionMemberInformationSet clone() {
      MockFederationExecutionMemberInformationSet result = new MockFederationExecutionMemberInformationSet();
      result.addAll(this);
      return result;
   }
}
