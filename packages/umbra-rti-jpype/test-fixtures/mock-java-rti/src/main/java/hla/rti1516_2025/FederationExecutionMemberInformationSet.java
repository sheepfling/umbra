package hla.rti1516_2025;

import java.util.Set;

/** Test-fixture subset of the standard member-information set interface. */
public interface FederationExecutionMemberInformationSet
   extends Set<FederationExecutionMemberInformation>, Cloneable {
   FederationExecutionMemberInformationSet clone();
}
