package hla.rti1516_2025;

/** Immutable callback record matching the standard Java value shape. */
public final class FederationExecutionInformation {
   public final String federationExecutionName;
   public final String logicalTimeImplementationName;

   public FederationExecutionInformation(
      String federationExecutionName,
      String logicalTimeImplementationName)
   {
      this.federationExecutionName = federationExecutionName;
      this.logicalTimeImplementationName = logicalTimeImplementationName;
   }
}
