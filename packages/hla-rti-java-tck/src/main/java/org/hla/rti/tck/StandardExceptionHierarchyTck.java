package org.hla.rti.tck;

import hla.rti1516_2025.exceptions.RTIexception;

/** Java-standard subset of the provider-independent standard exception contract. */
final class StandardExceptionHierarchyTck {
   private static final String MESSAGE = "portable-exception";
   private static final String[] SHARED_EXCEPTION_TYPES = {
      "AlreadyConnected", "AsynchronousDeliveryAlreadyDisabled",
      "AsynchronousDeliveryAlreadyEnabled", "AttributeAcquisitionWasNotRequested",
      "AttributeAlreadyBeingAcquired", "AttributeAlreadyBeingChanged",
      "AttributeAlreadyBeingDivested", "AttributeAlreadyOwned",
      "AttributeDivestitureWasNotRequested", "AttributeNotDefined", "AttributeNotOwned",
      "AttributeNotPublished", "CallNotAllowedFromWithinCallback", "ConnectionFailed",
      "CouldNotCreateLogicalTimeFactory", "CouldNotDecode", "CouldNotEncode",
      "CouldNotOpenFOM", "CouldNotOpenMIM", "DeletePrivilegeNotHeld",
      "DesignatorIsHLAstandardMIM", "RequestForTimeConstrainedPending",
      "RequestForTimeRegulationPending", "ErrorReadingFOM", "ErrorReadingMIM",
      "FederateAlreadyExecutionMember", "FederateHandleNotKnown", "FederateHasNotBegunSave",
      "FederateInternalError", "FederateIsExecutionMember", "FederateNameAlreadyInUse",
      "FederateNotExecutionMember", "FederateOwnsAttributes",
      "FederateServiceInvocationsAreBeingReportedViaMOM", "FederateUnableToUseTime",
      "FederatesCurrentlyJoined", "FederationExecutionAlreadyExists",
      "FederationExecutionDoesNotExist", "IllegalName", "IllegalTimeArithmetic",
      "InconsistentFOM", "InteractionClassAlreadyBeingChanged", "InteractionClassNotDefined",
      "InteractionClassNotPublished", "InteractionParameterNotDefined", "InTimeAdvancingState",
      "InvalidAttributeHandle", "InvalidCredentials", "InvalidDimensionHandle",
      "InvalidFederateHandle", "InvalidFOM", "InvalidInteractionClassHandle",
      "InvalidLogicalTime", "InvalidLogicalTimeInterval", "InvalidLookahead", "InvalidMIM",
      "InvalidObjectClassHandle", "InvalidObjectInstanceHandle", "InvalidOrderName",
      "InvalidOrderType", "InvalidParameterHandle", "InvalidRangeBound", "InvalidRegion",
      "InvalidResignAction", "InvalidRegionContext", "InvalidMessageRetractionHandle",
      "InvalidServiceGroup", "InvalidTransportationName", "InvalidTransportationTypeHandle",
      "InvalidUpdateRateDesignator", "LogicalTimeAlreadyPassed",
      "MessageCanNoLongerBeRetracted", "NameNotFound", "NameSetWasEmpty",
      "NoAcquisitionPending", "NotConnected", "ObjectClassNotDefined",
      "ObjectClassNotPublished", "ObjectInstanceNameInUse", "ObjectInstanceNameNotReserved",
      "ObjectInstanceNotKnown", "OwnershipAcquisitionPending", "RTIinternalError",
      "RegionDoesNotContainSpecifiedDimension", "RegionInUseForUpdateOrSubscription",
      "RegionNotCreatedByThisFederate", "ReportServiceInvocationsAreSubscribed",
      "RestoreInProgress", "RestoreNotInProgress", "RestoreNotRequested", "SaveInProgress",
      "SaveNotInProgress", "SaveNotInitiated", "SynchronizationPointLabelNotAnnounced",
      "TimeConstrainedAlreadyEnabled", "TimeConstrainedIsNotEnabled",
      "TimeRegulationAlreadyEnabled", "TimeRegulationIsNotEnabled", "Unauthorized",
      "UnsupportedCallbackModel"
   };

   private StandardExceptionHierarchyTck() {
   }

   static void run() throws Exception {
      require(RTIexception.class.getSuperclass() == Exception.class,
         "RTIexception does not extend java.lang.Exception");
      for (String name : SHARED_EXCEPTION_TYPES) {
         Class<?> rawType = Class.forName("hla.rti1516_2025.exceptions." + name);
         Class<? extends RTIexception> type = rawType.asSubclass(RTIexception.class);
         RTIexception instance = type.getConstructor(String.class).newInstance(MESSAGE);
         RTIexception throughBase = instance;
         require(instance.getClass() == type,
            name + " constructor produced a different exception type");
         require(name.equals(throughBase.name()),
            name + " did not preserve its standard exception name through RTIexception");
         require(MESSAGE.equals(throughBase.getMessage()),
            name + " did not preserve its constructor message through RTIexception");
      }
      require(SHARED_EXCEPTION_TYPES.length == 100,
         "the shared standard exception inventory changed unexpectedly");
   }

   private static void require(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }
}
