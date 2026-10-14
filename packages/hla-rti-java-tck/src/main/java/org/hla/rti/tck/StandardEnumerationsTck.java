package org.hla.rti.tck;

import hla.rti1516_2025.AdditionalSettingsResultCode;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RestoreFailureReason;
import hla.rti1516_2025.RestoreStatus;
import hla.rti1516_2025.SaveFailureReason;
import hla.rti1516_2025.SaveStatus;
import hla.rti1516_2025.ServiceGroup;
import hla.rti1516_2025.SynchronizationPointFailureReason;
import hla.rti1516_2025.auth.AuthorizationResult;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/** Provider-independent translation of the standard enumeration value contract. */
final class StandardEnumerationsTck {
   private StandardEnumerationsTck() {
   }

   static void run() {
      verify("AdditionalSettingsResultCode", AdditionalSettingsResultCode.values(),
         "SETTINGS_IGNORED", "SETTINGS_FAILED_TO_PARSE", "SETTINGS_APPLIED");
      verify("CallbackModel", CallbackModel.values(), "HLA_IMMEDIATE", "HLA_EVOKED");
      verify("OrderType", OrderType.values(), "RECEIVE", "TIMESTAMP");
      verify("ResignAction", ResignAction.values(),
         "UNCONDITIONALLY_DIVEST_ATTRIBUTES", "DELETE_OBJECTS",
         "CANCEL_PENDING_OWNERSHIP_ACQUISITIONS", "DELETE_OBJECTS_THEN_DIVEST",
         "CANCEL_THEN_DELETE_THEN_DIVEST", "NO_ACTION");
      verify("RestoreFailureReason", RestoreFailureReason.values(),
         "RTI_UNABLE_TO_RESTORE", "FEDERATE_REPORTED_FAILURE_DURING_RESTORE",
         "FEDERATE_RESIGNED_DURING_RESTORE", "RTI_DETECTED_FAILURE_DURING_RESTORE",
         "RESTORE_ABORTED");
      verify("RestoreStatus", RestoreStatus.values(), "NO_RESTORE_IN_PROGRESS",
         "FEDERATE_RESTORE_REQUEST_PENDING", "FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN",
         "FEDERATE_PREPARED_TO_RESTORE", "FEDERATE_RESTORING",
         "FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE");
      verify("SaveFailureReason", SaveFailureReason.values(), "RTI_UNABLE_TO_SAVE",
         "FEDERATE_REPORTED_FAILURE_DURING_SAVE", "FEDERATE_RESIGNED_DURING_SAVE",
         "RTI_DETECTED_FAILURE_DURING_SAVE", "SAVE_TIME_CANNOT_BE_HONORED",
         "SAVE_ABORTED");
      verify("SaveStatus", SaveStatus.values(), "NO_SAVE_IN_PROGRESS",
         "FEDERATE_INSTRUCTED_TO_SAVE", "FEDERATE_SAVING",
         "FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE");
      verify("ServiceGroup", ServiceGroup.values(), "FEDERATION_MANAGEMENT",
         "DECLARATION_MANAGEMENT", "OBJECT_MANAGEMENT", "OWNERSHIP_MANAGEMENT",
         "TIME_MANAGEMENT", "DATA_DISTRIBUTION_MANAGEMENT", "SUPPORT_SERVICES");
      verify("SynchronizationPointFailureReason", SynchronizationPointFailureReason.values(),
         "SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE", "SYNCHRONIZATION_SET_MEMBER_NOT_JOINED");
      verify("AuthorizationResult.Code", AuthorizationResult.Code.values(), "AUTHORIZED",
         "UNAUTHORIZED", "INVALID_CREDENTIALS", "AUTHORIZATION_ERROR");
   }

   private static void verify(String name, Enum<?>[] values, String... expectedNames) {
      Set<String> expected = new HashSet<>(Arrays.asList(expectedNames));
      Set<String> actual = new HashSet<>();
      for (Enum<?> value : values) {
         actual.add(value.name());
      }
      if (values.length != expectedNames.length || expected.size() != expectedNames.length
            || !actual.equals(expected)) {
         throw new AssertionError(name + " enumerators differ: expected " + expected
            + ", got " + actual);
      }
   }
}
