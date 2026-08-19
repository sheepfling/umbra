package hla.rti1516_2025;

public final class FederateRestoreStatus {
   public final FederateHandle preRestoreHandle;
   public final FederateHandle postRestoreHandle;
   public final RestoreStatus status;

   public FederateRestoreStatus(
      FederateHandle preRestoreHandle,
      FederateHandle postRestoreHandle,
      RestoreStatus status)
   {
      this.preRestoreHandle = preRestoreHandle;
      this.postRestoreHandle = postRestoreHandle;
      this.status = status;
   }
}
