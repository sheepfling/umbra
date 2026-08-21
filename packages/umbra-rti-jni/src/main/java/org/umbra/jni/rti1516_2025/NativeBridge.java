package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.auth.AuthorizationResult;
import hla.rti1516_2025.auth.Credentials;
import java.util.Set;
import hla.rti1516_2025.exceptions.AlreadyConnected;
import hla.rti1516_2025.exceptions.NotConnected;

/** Native entry points used only by the JNI integration RTI façade. */
final class NativeBridge {
   private static final String LIBRARY_PROPERTY = "umbra.rti.jni.library";

   static {
      String library = System.getProperty(LIBRARY_PROPERTY);
      if (library == null || library.isEmpty()) {
         System.loadLibrary("umbra_rti_jni");
      } else {
         System.load(library);
      }
   }

   private NativeBridge() {
   }

   static native long nativeCreate();

   static native void nativeDestroy(long handle);

   static native ConfigurationResult nativeConnect(
      long handle,
      FederateAmbassador federateAmbassador,
      String callbackModelName,
      RtiConfiguration configuration,
      Credentials credentials) throws AlreadyConnected;

   static native void nativeDisconnect(long handle) throws NotConnected;

   // Internal deterministic fault source used only by JNI integration tests;
   // it is deliberately absent from the standard RTIambassador interface.
   static native boolean nativeFailEmbeddedTransportConnectionForTesting(
      long handle, String faultDescription);

   // Internal deterministic RTI membership-control source used only by JNI
   // integration tests; it is absent from the standard RTIambassador API.
   static native boolean nativeForceEmbeddedFederateResignationForTesting(
      long handle, String reason);

   static native boolean nativeEvokeCallback(long handle, double minimumTime)
      throws NotConnected;

   static native boolean nativeEvokeMultipleCallbacks(
      long handle,
      double minimumTime,
      double maximumTime) throws NotConnected;

   static native void nativeEnableCallbacks(long handle) throws NotConnected;

   static native void nativeDisableCallbacks(long handle) throws NotConnected;

   static native void nativeListFederationExecutions(long handle) throws NotConnected;

   static native void nativeListFederationExecutionMembers(
      long handle,
      String federationExecutionName) throws NotConnected;

   static native void nativeCreateFederationExecution(
      long handle,
      String federationExecutionName,
      String[] fomModules,
      String logicalTimeImplementationName) throws NotConnected;

   static native void nativeCreateFederationExecutionWithMIM(
      long handle,
      String federationExecutionName,
      String[] fomModules,
      String mimModule,
      String logicalTimeImplementationName) throws NotConnected;

   static native void nativeDestroyFederationExecution(
      long handle,
      String federationExecutionName) throws NotConnected;

   static native byte[] nativeJoinFederationExecution(
      long handle,
      String federateName,
      String federateType,
      String federationExecutionName,
      String[] additionalFomModules) throws NotConnected;

   static native void nativeResignFederationExecution(
      long handle,
      String resignActionName) throws NotConnected;

   static native byte[] nativeGetFederateHandle(long handle, String federateName);

   static native String nativeGetFederateName(long handle, byte[] encodedFederateHandle);

   static native long nativeNormalizeFederateHandle(long handle, byte[] encodedFederateHandle);

   static native byte[] nativeDecodeObjectClassHandle(byte[] encodedValue);

   static native byte[] nativeGetObjectClassHandle(long handle, String objectClassName);

   static native String nativeGetObjectClassName(long handle, byte[] encodedObjectClassHandle);

   static native byte[] nativeGetKnownObjectClassHandle(
      long handle, byte[] encodedObjectInstanceHandle);

   static native long nativeNormalizeObjectClassHandle(long handle, byte[] encodedObjectClassHandle);

   static native byte[] nativeDecodeAttributeHandle(byte[] encodedValue);

   static native byte[] nativeGetAttributeHandle(
      long handle, byte[] encodedObjectClassHandle, String attributeName);

   static native String nativeGetAttributeName(
      long handle, byte[] encodedObjectClassHandle, byte[] encodedAttributeHandle);

   static native byte[] nativeDecodeDimensionHandle(byte[] encodedValue);

   static native byte[] nativeGetDimensionHandle(long handle, String dimensionName);

   static native String nativeGetDimensionName(long handle, byte[] encodedDimensionHandle);

   static native byte[][] nativeGetAvailableDimensionsForObjectClass(
      long handle, byte[] encodedObjectClassHandle);

   static native byte[][] nativeGetAvailableDimensionsForInteractionClass(
      long handle, byte[] encodedInteractionClassHandle);

   static native long nativeGetDimensionUpperBound(long handle, byte[] encodedDimensionHandle);

   static native byte[] nativeDecodeRegionHandle(byte[] encodedValue);

   static native byte[] nativeCreateRegion(long handle, DimensionHandleSet dimensions);

   static native void nativeCommitRegionModifications(long handle, RegionHandleSet regions);

   static native void nativeDeleteRegion(long handle, byte[] encodedRegionHandle);

   static native byte[][] nativeGetDimensionHandleSet(long handle, byte[] encodedRegionHandle);

   static native long[] nativeGetRangeBounds(
      long handle, byte[] encodedRegionHandle, byte[] encodedDimensionHandle);

   static native void nativeSetRangeBounds(
      long handle,
      byte[] encodedRegionHandle,
      byte[] encodedDimensionHandle,
      long lowerBound,
      long upperBound);

   static native byte[] nativeDecodeObjectInstanceHandle(byte[] encodedValue);

   static native void nativePublishObjectClassAttributes(
      long handle, byte[] encodedObjectClassHandle, AttributeHandleSet attributes);

   static native void nativeUnpublishObjectClass(long handle, byte[] encodedObjectClassHandle);

   static native void nativeUnpublishObjectClassAttributes(
      long handle, byte[] encodedObjectClassHandle, AttributeHandleSet attributes);

   static native void nativePublishObjectClassDirectedInteractions(
      long handle, byte[] encodedObjectClassHandle, java.util.Set<?> interactionClasses);

   static native void nativeUnpublishObjectClassDirectedInteractions(
      long handle, byte[] encodedObjectClassHandle);

   static native void nativeUnpublishObjectClassDirectedInteractionsWithClasses(
      long handle, byte[] encodedObjectClassHandle, java.util.Set<?> interactionClasses);

   static native void nativeSubscribeObjectClassAttributes(
      long handle,
      byte[] encodedObjectClassHandle,
      AttributeHandleSet attributes,
      boolean active,
      String updateRateDesignator);

   static native void nativeUnsubscribeObjectClass(long handle, byte[] encodedObjectClassHandle);

   static native void nativeUnsubscribeObjectClassAttributes(
      long handle, byte[] encodedObjectClassHandle, AttributeHandleSet attributes);

   static native void nativeSubscribeObjectClassDirectedInteractions(
      long handle,
      byte[] encodedObjectClassHandle,
      java.util.Set<?> interactionClasses,
      boolean universally);

   static native void nativeUnsubscribeObjectClassDirectedInteractions(
      long handle, byte[] encodedObjectClassHandle);

   static native void nativeUnsubscribeObjectClassDirectedInteractionsWithClasses(
      long handle, byte[] encodedObjectClassHandle, java.util.Set<?> interactionClasses);

   static native void nativeSubscribeObjectClassAttributesWithRegions(
      long handle,
      byte[] encodedObjectClassHandle,
      AttributeSetRegionSetPairList attributesAndRegions,
      boolean active,
      String updateRateDesignator);

   static native void nativeUnsubscribeObjectClassAttributesWithRegions(
      long handle,
      byte[] encodedObjectClassHandle,
      AttributeSetRegionSetPairList attributesAndRegions);

   static native void nativeReserveObjectInstanceName(long handle, String objectInstanceName);

   static native void nativeReleaseObjectInstanceName(long handle, String objectInstanceName);

   static native void nativeReserveMultipleObjectInstanceNames(
      long handle, Set<?> objectInstanceNames);

   static native void nativeReleaseMultipleObjectInstanceNames(
      long handle, Set<?> objectInstanceNames);

   static native byte[] nativeRegisterObjectInstance(
      long handle, byte[] encodedObjectClassHandle, String objectInstanceName);

   static native byte[] nativeRegisterObjectInstanceWithRegions(
      long handle,
      byte[] encodedObjectClassHandle,
      AttributeSetRegionSetPairList attributesAndRegions,
      String objectInstanceName);

   static native void nativeAssociateRegionsForUpdates(
      long handle,
      byte[] encodedObjectInstanceHandle,
      AttributeSetRegionSetPairList attributesAndRegions);

   static native void nativeUnassociateRegionsForUpdates(
      long handle,
      byte[] encodedObjectInstanceHandle,
      AttributeSetRegionSetPairList attributesAndRegions);

   static native byte[] nativeGetObjectInstanceHandle(long handle, String objectInstanceName);

   static native String nativeGetObjectInstanceName(
      long handle, byte[] encodedObjectInstanceHandle);

   static native long nativeNormalizeObjectInstanceHandle(
      long handle, byte[] encodedObjectInstanceHandle);

   static native void nativeUpdateAttributeValues(
      long handle,
      byte[] encodedObjectInstanceHandle,
      AttributeHandleValueMap attributeValues,
      byte[] userSuppliedTag);

   static native void nativeDeleteObjectInstance(
      long handle, byte[] encodedObjectInstanceHandle, byte[] userSuppliedTag);

   static native void nativeLocalDeleteObjectInstance(
      long handle, byte[] encodedObjectInstanceHandle);

   static native void nativeRequestAttributeValueUpdateForObjectInstance(
      long handle,
      byte[] encodedObjectInstanceHandle,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag);

   static native void nativeRequestAttributeValueUpdateForObjectClass(
      long handle,
      byte[] encodedObjectClassHandle,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag);

   static native void nativeRequestAttributeValueUpdateWithRegions(
      long handle,
      byte[] encodedObjectClassHandle,
      AttributeSetRegionSetPairList attributesAndRegions,
      byte[] userSuppliedTag);

   static native void nativeQueryAttributeOwnership(
      long handle, byte[] encodedObjectInstanceHandle, AttributeHandleSet attributes);

   static native boolean nativeIsAttributeOwnedByFederate(
      long handle, byte[] encodedObjectInstanceHandle, byte[] encodedAttributeHandle);

   static native void nativeUnconditionalAttributeOwnershipDivestiture(
      long handle,
      byte[] encodedObjectInstanceHandle,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag);

   static native void nativeNegotiatedAttributeOwnershipDivestiture(
      long handle,
      byte[] encodedObjectInstanceHandle,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag);

   static native void nativeConfirmDivestiture(
      long handle,
      byte[] encodedObjectInstanceHandle,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag);

   static native void nativeCancelNegotiatedAttributeOwnershipDivestiture(
      long handle, byte[] encodedObjectInstanceHandle, AttributeHandleSet attributes);

   static native void nativeAttributeOwnershipAcquisition(
      long handle,
      byte[] encodedObjectInstanceHandle,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag);

   static native void nativeAttributeOwnershipAcquisitionIfAvailable(
      long handle,
      byte[] encodedObjectInstanceHandle,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag);

   static native void nativeCancelAttributeOwnershipAcquisition(
      long handle, byte[] encodedObjectInstanceHandle, AttributeHandleSet attributes);

   static native void nativeAttributeOwnershipReleaseDenied(
      long handle,
      byte[] encodedObjectInstanceHandle,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag);

   static native byte[][] nativeAttributeOwnershipDivestitureIfWanted(
      long handle,
      byte[] encodedObjectInstanceHandle,
      AttributeHandleSet attributes,
      byte[] userSuppliedTag);

   static native void nativeQueryAttributeTransportationType(
      long handle, byte[] encodedObjectInstanceHandle, byte[] encodedAttributeHandle);

   static native void nativeQueryInteractionTransportationType(
      long handle, byte[] encodedFederateHandle, byte[] encodedInteractionClassHandle);

   static native void nativeRequestAttributeTransportationTypeChange(
      long handle,
      byte[] encodedObjectInstanceHandle,
      AttributeHandleSet attributes,
      byte[] encodedTransportationTypeHandle);

   static native void nativeRequestInteractionTransportationTypeChange(
      long handle,
      byte[] encodedInteractionClassHandle,
      byte[] encodedTransportationTypeHandle);

   static native void nativeSetAttributeScopeAdvisorySwitch(long handle, boolean enabled);

   static native boolean nativeGetObjectClassRelevanceAdvisorySwitch(long handle);

   static native void nativeSetObjectClassRelevanceAdvisorySwitch(long handle, boolean enabled);

   static native boolean nativeGetAttributeRelevanceAdvisorySwitch(long handle);

   static native void nativeSetAttributeRelevanceAdvisorySwitch(long handle, boolean enabled);

   static native boolean nativeGetAttributeScopeAdvisorySwitch(long handle);

   static native boolean nativeGetInteractionRelevanceAdvisorySwitch(long handle);

   static native void nativeSetInteractionRelevanceAdvisorySwitch(long handle, boolean enabled);

   static native String nativeGetAutomaticResignDirective(long handle);

   static native void nativeSetAutomaticResignDirective(long handle, String resignActionName);

   static native boolean nativeGetServiceReportingSwitch(long handle);

   static native void nativeSetServiceReportingSwitch(long handle, boolean enabled);

   static native boolean nativeGetExceptionReportingSwitch(long handle);

   static native void nativeSetExceptionReportingSwitch(long handle, boolean enabled);

   static native boolean nativeGetSendServiceReportsToFileSwitch(long handle);

   static native void nativeSetSendServiceReportsToFileSwitch(long handle, boolean enabled);

   static native String nativeGetHLAversion(long handle);

   static native boolean nativeGetAutoProvideSwitch(long handle);

   static native boolean nativeGetDelaySubscriptionEvaluationSwitch(long handle);

   static native boolean nativeGetAdvisoriesUseKnownClassSwitch(long handle);

   static native boolean nativeGetAllowRelaxedDDMSwitch(long handle);

   static native boolean nativeGetNonRegulatedGrantSwitch(long handle);

   static native long nativeNormalizeServiceGroup(long handle, String serviceGroupName);

   static native String nativeGetOrderType(long handle, String orderTypeName);

   static native String nativeGetOrderName(long handle, String orderTypeName);

   static native void nativeChangeAttributeOrderType(
      long handle,
      byte[] encodedObjectInstanceHandle,
      AttributeHandleSet attributes,
      String orderTypeName);

   static native void nativeChangeDefaultAttributeOrderType(
      long handle,
      byte[] encodedObjectClassHandle,
      AttributeHandleSet attributes,
      String orderTypeName);

   static native void nativeChangeInteractionOrderType(
      long handle, byte[] encodedInteractionClassHandle, String orderTypeName);

   static native double nativeGetUpdateRateValue(long handle, String updateRateDesignator);

   static native double nativeGetUpdateRateValueForAttribute(
      long handle, byte[] encodedObjectInstanceHandle, byte[] encodedAttributeHandle);

   static native void nativeChangeDefaultAttributeTransportationType(
      long handle,
      byte[] encodedObjectClassHandle,
      AttributeHandleSet attributes,
      byte[] encodedTransportationTypeHandle);

   static native String nativeGetTimeFactoryName(long handle);

   static native void nativeValidateLogicalTimeImplementation(
      long handle, String implementationName, boolean interval);

   static native byte[] nativeMakeIntegerLogicalTime(long handle, long value);

   static native byte[] nativeMakeIntegerLogicalTimeInterval(long handle, long value);

   static native byte[] nativeMakeFloatLogicalTime(long handle, double value);

   static native byte[] nativeMakeFloatLogicalTimeInterval(long handle, double value);

   static native byte[] nativeDecodeLogicalTime(long handle, byte[] encodedValue);

   static native byte[] nativeDecodeLogicalTimeInterval(long handle, byte[] encodedValue);

   static native byte[] nativeAddLogicalTime(
      long handle, byte[] encodedTime, byte[] encodedInterval);

   static native byte[] nativeSubtractLogicalTime(
      long handle, byte[] encodedTime, byte[] encodedInterval);

   static native byte[] nativeAddLogicalTimeInterval(
      long handle, byte[] encodedInterval, byte[] encodedAddend);

   static native byte[] nativeSubtractLogicalTimeInterval(
      long handle, byte[] encodedInterval, byte[] encodedSubtrahend);

   static native byte[] nativeDifferenceLogicalTime(
      long handle, byte[] encodedMinuend, byte[] encodedSubtrahend);

   static native void nativeEnableTimeRegulation(
      long handle, byte[] encodedLogicalTimeInterval);

   static native void nativeDisableTimeRegulation(long handle);

   static native void nativeEnableTimeConstrained(long handle);

   static native void nativeDisableTimeConstrained(long handle);

   static native void nativeEnableAsynchronousDelivery(long handle);

   static native void nativeDisableAsynchronousDelivery(long handle);

   static native void nativeModifyLookahead(long handle, byte[] encodedLogicalTimeInterval);

   static native byte[] nativeQueryLookahead(long handle);

   static native void nativeTimeAdvanceRequest(long handle, byte[] encodedLogicalTime);

   static native void nativeTimeAdvanceRequestAvailable(long handle, byte[] encodedLogicalTime);

   static native void nativeNextMessageRequest(long handle, byte[] encodedLogicalTime);

   static native void nativeNextMessageRequestAvailable(long handle, byte[] encodedLogicalTime);

   static native void nativeFlushQueueRequest(long handle, byte[] encodedLogicalTime);

   static native byte[] nativeQueryLogicalTime(long handle);

   static native byte[] nativeQueryGALT(long handle);

   static native byte[] nativeQueryLITS(long handle);

   static native byte[] nativeSendInteractionWithTime(
      long handle,
      byte[] encodedInteractionClassHandle,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag,
      byte[] encodedLogicalTime);

   static native void nativeRetract(long handle, byte[] encodedMessageRetractionHandle);

   static native byte[] nativeDecodeMessageRetractionHandle(
      long handle, byte[] encodedMessageRetractionHandle);

   static native byte[] nativeUpdateAttributeValuesWithTime(
      long handle,
      byte[] encodedObjectInstanceHandle,
      AttributeHandleValueMap attributeValues,
      byte[] userSuppliedTag,
      byte[] encodedLogicalTime);

   static native byte[] nativeDeleteObjectInstanceWithTime(
      long handle,
      byte[] encodedObjectInstanceHandle,
      byte[] userSuppliedTag,
      byte[] encodedLogicalTime);

   static native byte[] nativeDecodeInteractionClassHandle(byte[] encodedValue);

   static native byte[] nativeGetInteractionClassHandle(long handle, String interactionClassName);

   static native String nativeGetInteractionClassName(
      long handle, byte[] encodedInteractionClassHandle);

   static native long nativeNormalizeInteractionClassHandle(
      long handle, byte[] encodedInteractionClassHandle);

   static native byte[] nativeDecodeParameterHandle(byte[] encodedValue);

   static native byte[] nativeGetParameterHandle(
      long handle, byte[] encodedInteractionClassHandle, String parameterName);

   static native String nativeGetParameterName(
      long handle, byte[] encodedInteractionClassHandle, byte[] encodedParameterHandle);

   static native void nativePublishInteractionClass(
      long handle, byte[] encodedInteractionClassHandle);

   static native void nativeUnpublishInteractionClass(
      long handle, byte[] encodedInteractionClassHandle);

   static native void nativeSubscribeInteractionClass(
      long handle, byte[] encodedInteractionClassHandle, boolean active);

   static native void nativeUnsubscribeInteractionClass(
      long handle, byte[] encodedInteractionClassHandle);

   static native void nativeSubscribeInteractionClassWithRegions(
      long handle,
      byte[] encodedInteractionClassHandle,
      RegionHandleSet regions,
      boolean active);

   static native void nativeUnsubscribeInteractionClassWithRegions(
      long handle, byte[] encodedInteractionClassHandle, RegionHandleSet regions);

   static native byte[] nativeDecodeTransportationTypeHandle(byte[] encodedValue);

   static native byte[] nativeGetTransportationTypeHandle(
      long handle, String transportationTypeName);

   static native String nativeGetTransportationTypeName(
      long handle, byte[] encodedTransportationTypeHandle);

   static native void nativeSendInteraction(
      long handle,
      byte[] encodedInteractionClassHandle,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag);

   static native void nativeSendDirectedInteraction(
      long handle,
      byte[] encodedInteractionClassHandle,
      byte[] encodedObjectInstanceHandle,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag);

   static native byte[] nativeSendDirectedInteractionWithTime(
      long handle,
      byte[] encodedInteractionClassHandle,
      byte[] encodedObjectInstanceHandle,
      ParameterHandleValueMap parameterValues,
      byte[] userSuppliedTag,
      byte[] encodedLogicalTime);

   static native void nativeSendInteractionWithRegions(
      long handle,
      byte[] encodedInteractionClassHandle,
      ParameterHandleValueMap parameterValues,
      RegionHandleSet regions,
      byte[] userSuppliedTag);

   static native byte[] nativeSendInteractionWithRegionsWithTime(
      long handle,
      byte[] encodedInteractionClassHandle,
      ParameterHandleValueMap parameterValues,
      RegionHandleSet regions,
      byte[] userSuppliedTag,
      byte[] encodedLogicalTime);

   static native boolean nativeGetConveyRegionDesignatorSetsSwitch(long handle);

   static native void nativeSetConveyRegionDesignatorSetsSwitch(long handle, boolean enabled);

   static native long nativeCreateHLAinteger32BE(int value);

   static native void nativeDestroyHLAinteger32BE(long handle);

   static native int nativeGetHLAinteger32BE(long handle);

   static native void nativeSetHLAinteger32BE(long handle, int value);

   static native int nativeHLAinteger32BEOctetBoundary(long handle);

   static native int nativeHLAinteger32BEEncodedLength(long handle);

   static native byte[] nativeHLAinteger32BEToByteArray(long handle);

   static native void nativeDecodeHLAinteger32BE(long handle, byte[] bytes);

   static native long nativeCreateHLAinteger32LE(int value);

   static native void nativeDestroyHLAinteger32LE(long handle);

   static native int nativeGetHLAinteger32LE(long handle);

   static native void nativeSetHLAinteger32LE(long handle, int value);

   static native int nativeHLAinteger32LEOctetBoundary(long handle);

   static native int nativeHLAinteger32LEEncodedLength(long handle);

   static native byte[] nativeHLAinteger32LEToByteArray(long handle);

   static native void nativeDecodeHLAinteger32LE(long handle, byte[] bytes);

   static native long nativeCreateHLAinteger64BE(long value);

   static native void nativeDestroyHLAinteger64BE(long handle);

   static native long nativeGetHLAinteger64BE(long handle);

   static native void nativeSetHLAinteger64BE(long handle, long value);

   static native int nativeHLAinteger64BEOctetBoundary(long handle);

   static native int nativeHLAinteger64BEEncodedLength(long handle);

   static native byte[] nativeHLAinteger64BEToByteArray(long handle);

   static native void nativeDecodeHLAinteger64BE(long handle, byte[] bytes);

   static native long nativeCreateHLAinteger64LE(long value);

   static native void nativeDestroyHLAinteger64LE(long handle);

   static native long nativeGetHLAinteger64LE(long handle);

   static native void nativeSetHLAinteger64LE(long handle, long value);

   static native int nativeHLAinteger64LEOctetBoundary(long handle);

   static native int nativeHLAinteger64LEEncodedLength(long handle);

   static native byte[] nativeHLAinteger64LEToByteArray(long handle);

   static native void nativeDecodeHLAinteger64LE(long handle, byte[] bytes);

   static native long nativeCreateHLAinteger16BE(short value);

   static native void nativeDestroyHLAinteger16BE(long handle);

   static native short nativeGetHLAinteger16BE(long handle);

   static native void nativeSetHLAinteger16BE(long handle, short value);

   static native int nativeHLAinteger16BEOctetBoundary(long handle);

   static native int nativeHLAinteger16BEEncodedLength(long handle);

   static native byte[] nativeHLAinteger16BEToByteArray(long handle);

   static native void nativeDecodeHLAinteger16BE(long handle, byte[] bytes);

   static native long nativeCreateHLAinteger16LE(short value);

   static native void nativeDestroyHLAinteger16LE(long handle);

   static native short nativeGetHLAinteger16LE(long handle);

   static native void nativeSetHLAinteger16LE(long handle, short value);

   static native int nativeHLAinteger16LEOctetBoundary(long handle);

   static native int nativeHLAinteger16LEEncodedLength(long handle);

   static native byte[] nativeHLAinteger16LEToByteArray(long handle);

   static native void nativeDecodeHLAinteger16LE(long handle, byte[] bytes);

   static native long nativeCreateHLAfloat32BE(float value);

   static native void nativeDestroyHLAfloat32BE(long handle);

   static native float nativeGetHLAfloat32BE(long handle);

   static native void nativeSetHLAfloat32BE(long handle, float value);

   static native int nativeHLAfloat32BEOctetBoundary(long handle);

   static native int nativeHLAfloat32BEEncodedLength(long handle);

   static native byte[] nativeHLAfloat32BEToByteArray(long handle);

   static native void nativeDecodeHLAfloat32BE(long handle, byte[] bytes);

   static native long nativeCreateHLAfloat32LE(float value);

   static native void nativeDestroyHLAfloat32LE(long handle);

   static native float nativeGetHLAfloat32LE(long handle);

   static native void nativeSetHLAfloat32LE(long handle, float value);

   static native int nativeHLAfloat32LEOctetBoundary(long handle);

   static native int nativeHLAfloat32LEEncodedLength(long handle);

   static native byte[] nativeHLAfloat32LEToByteArray(long handle);

   static native void nativeDecodeHLAfloat32LE(long handle, byte[] bytes);

   static native long nativeCreateHLAfloat64BE(double value);

   static native void nativeDestroyHLAfloat64BE(long handle);

   static native double nativeGetHLAfloat64BE(long handle);

   static native void nativeSetHLAfloat64BE(long handle, double value);

   static native int nativeHLAfloat64BEOctetBoundary(long handle);

   static native int nativeHLAfloat64BEEncodedLength(long handle);

   static native byte[] nativeHLAfloat64BEToByteArray(long handle);

   static native void nativeDecodeHLAfloat64BE(long handle, byte[] bytes);

   static native long nativeCreateHLAfloat64LE(double value);

   static native void nativeDestroyHLAfloat64LE(long handle);

   static native double nativeGetHLAfloat64LE(long handle);

   static native void nativeSetHLAfloat64LE(long handle, double value);

   static native int nativeHLAfloat64LEOctetBoundary(long handle);

   static native int nativeHLAfloat64LEEncodedLength(long handle);

   static native byte[] nativeHLAfloat64LEToByteArray(long handle);

   static native void nativeDecodeHLAfloat64LE(long handle, byte[] bytes);

   static native long nativeCreateHLAunsignedInteger16BE(short value);

   static native void nativeDestroyHLAunsignedInteger16BE(long handle);

   static native short nativeGetHLAunsignedInteger16BE(long handle);

   static native void nativeSetHLAunsignedInteger16BE(long handle, short value);

   static native int nativeHLAunsignedInteger16BEOctetBoundary(long handle);

   static native int nativeHLAunsignedInteger16BEEncodedLength(long handle);

   static native byte[] nativeHLAunsignedInteger16BEToByteArray(long handle);

   static native void nativeDecodeHLAunsignedInteger16BE(long handle, byte[] bytes);

   static native long nativeCreateHLAunsignedInteger16LE(short value);

   static native void nativeDestroyHLAunsignedInteger16LE(long handle);

   static native short nativeGetHLAunsignedInteger16LE(long handle);

   static native void nativeSetHLAunsignedInteger16LE(long handle, short value);

   static native int nativeHLAunsignedInteger16LEOctetBoundary(long handle);

   static native int nativeHLAunsignedInteger16LEEncodedLength(long handle);

   static native byte[] nativeHLAunsignedInteger16LEToByteArray(long handle);

   static native void nativeDecodeHLAunsignedInteger16LE(long handle, byte[] bytes);

   static native long nativeCreateHLAunsignedInteger32BE(int value);

   static native void nativeDestroyHLAunsignedInteger32BE(long handle);

   static native int nativeGetHLAunsignedInteger32BE(long handle);

   static native void nativeSetHLAunsignedInteger32BE(long handle, int value);

   static native int nativeHLAunsignedInteger32BEOctetBoundary(long handle);

   static native int nativeHLAunsignedInteger32BEEncodedLength(long handle);

   static native byte[] nativeHLAunsignedInteger32BEToByteArray(long handle);

   static native void nativeDecodeHLAunsignedInteger32BE(long handle, byte[] bytes);

   static native long nativeCreateHLAunsignedInteger32LE(int value);

   static native void nativeDestroyHLAunsignedInteger32LE(long handle);

   static native int nativeGetHLAunsignedInteger32LE(long handle);

   static native void nativeSetHLAunsignedInteger32LE(long handle, int value);

   static native int nativeHLAunsignedInteger32LEOctetBoundary(long handle);

   static native int nativeHLAunsignedInteger32LEEncodedLength(long handle);

   static native byte[] nativeHLAunsignedInteger32LEToByteArray(long handle);

   static native void nativeDecodeHLAunsignedInteger32LE(long handle, byte[] bytes);

   static native long nativeCreateHLAunsignedInteger64BE(long value);

   static native void nativeDestroyHLAunsignedInteger64BE(long handle);

   static native long nativeGetHLAunsignedInteger64BE(long handle);

   static native void nativeSetHLAunsignedInteger64BE(long handle, long value);

   static native int nativeHLAunsignedInteger64BEOctetBoundary(long handle);

   static native int nativeHLAunsignedInteger64BEEncodedLength(long handle);

   static native byte[] nativeHLAunsignedInteger64BEToByteArray(long handle);

   static native void nativeDecodeHLAunsignedInteger64BE(long handle, byte[] bytes);

   static native long nativeCreateHLAunsignedInteger64LE(long value);

   static native void nativeDestroyHLAunsignedInteger64LE(long handle);

   static native long nativeGetHLAunsignedInteger64LE(long handle);

   static native void nativeSetHLAunsignedInteger64LE(long handle, long value);

   static native int nativeHLAunsignedInteger64LEOctetBoundary(long handle);

   static native int nativeHLAunsignedInteger64LEEncodedLength(long handle);

   static native byte[] nativeHLAunsignedInteger64LEToByteArray(long handle);

   static native void nativeDecodeHLAunsignedInteger64LE(long handle, byte[] bytes);

   static native long nativeCreateHLAbyte(byte value);

   static native void nativeDestroyHLAbyte(long handle);

   static native byte nativeGetHLAbyte(long handle);

   static native void nativeSetHLAbyte(long handle, byte value);

   static native int nativeHLAbyteOctetBoundary(long handle);

   static native int nativeHLAbyteEncodedLength(long handle);

   static native byte[] nativeHLAbyteToByteArray(long handle);

   static native void nativeDecodeHLAbyte(long handle, byte[] bytes);

   static native long nativeCreateHLAoctet(byte value);

   static native void nativeDestroyHLAoctet(long handle);

   static native byte nativeGetHLAoctet(long handle);

   static native void nativeSetHLAoctet(long handle, byte value);

   static native int nativeHLAoctetOctetBoundary(long handle);

   static native int nativeHLAoctetEncodedLength(long handle);

   static native byte[] nativeHLAoctetToByteArray(long handle);

   static native void nativeDecodeHLAoctet(long handle, byte[] bytes);

   static native long nativeCreateHLAoctetPairBE(short value);

   static native void nativeDestroyHLAoctetPairBE(long handle);

   static native short nativeGetHLAoctetPairBE(long handle);

   static native void nativeSetHLAoctetPairBE(long handle, short value);

   static native int nativeHLAoctetPairBEOctetBoundary(long handle);

   static native int nativeHLAoctetPairBEEncodedLength(long handle);

   static native byte[] nativeHLAoctetPairBEToByteArray(long handle);

   static native void nativeDecodeHLAoctetPairBE(long handle, byte[] bytes);

   static native long nativeCreateHLAoctetPairLE(short value);

   static native void nativeDestroyHLAoctetPairLE(long handle);

   static native short nativeGetHLAoctetPairLE(long handle);

   static native void nativeSetHLAoctetPairLE(long handle, short value);

   static native int nativeHLAoctetPairLEOctetBoundary(long handle);

   static native int nativeHLAoctetPairLEEncodedLength(long handle);

   static native byte[] nativeHLAoctetPairLEToByteArray(long handle);

   static native void nativeDecodeHLAoctetPairLE(long handle, byte[] bytes);

   static native long nativeCreateHLAASCIIchar(byte value);

   static native void nativeDestroyHLAASCIIchar(long handle);

   static native byte nativeGetHLAASCIIchar(long handle);

   static native void nativeSetHLAASCIIchar(long handle, byte value);

   static native int nativeHLAASCIIcharOctetBoundary(long handle);

   static native int nativeHLAASCIIcharEncodedLength(long handle);

   static native byte[] nativeHLAASCIIcharToByteArray(long handle);

   static native void nativeDecodeHLAASCIIchar(long handle, byte[] bytes);

   static native long nativeCreateHLAunicodeChar(short value);

   static native void nativeDestroyHLAunicodeChar(long handle);

   static native short nativeGetHLAunicodeChar(long handle);

   static native void nativeSetHLAunicodeChar(long handle, short value);

   static native int nativeHLAunicodeCharOctetBoundary(long handle);

   static native int nativeHLAunicodeCharEncodedLength(long handle);

   static native byte[] nativeHLAunicodeCharToByteArray(long handle);

   static native void nativeDecodeHLAunicodeChar(long handle, byte[] bytes);

   static native long nativeCreateHLAboolean(boolean value);

   static native void nativeDestroyHLAboolean(long handle);

   static native boolean nativeGetHLAboolean(long handle);

   static native void nativeSetHLAboolean(long handle, boolean value);

   static native int nativeHLAbooleanOctetBoundary(long handle);

   static native int nativeHLAbooleanEncodedLength(long handle);

   static native byte[] nativeHLAbooleanToByteArray(long handle);

   static native void nativeDecodeHLAboolean(long handle, byte[] bytes);

   static native long nativeCreateHLAASCIIstring(String value);

   static native void nativeDestroyHLAASCIIstring(long handle);

   static native String nativeGetHLAASCIIstring(long handle);

   static native void nativeSetHLAASCIIstring(long handle, String value);

   static native int nativeHLAASCIIstringOctetBoundary(long handle);

   static native int nativeHLAASCIIstringEncodedLength(long handle);

   static native byte[] nativeHLAASCIIstringToByteArray(long handle);

   static native void nativeDecodeHLAASCIIstring(long handle, byte[] bytes);

   static native long nativeCreateHLAunicodeString(String value);

   static native void nativeDestroyHLAunicodeString(long handle);

   static native String nativeGetHLAunicodeString(long handle);

   static native void nativeSetHLAunicodeString(long handle, String value);

   static native int nativeHLAunicodeStringOctetBoundary(long handle);

   static native int nativeHLAunicodeStringEncodedLength(long handle);

   static native byte[] nativeHLAunicodeStringToByteArray(long handle);

   static native void nativeDecodeHLAunicodeString(long handle, byte[] bytes);

   static native long nativeCreateHLAopaqueData(byte[] value);

   static native void nativeDestroyHLAopaqueData(long handle);

   static native int nativeHLAopaqueDataSize(long handle);

   static native byte nativeGetHLAopaqueData(long handle, int index);

   static native byte[] nativeGetHLAopaqueDataValue(long handle);

   static native void nativeSetHLAopaqueData(long handle, byte[] value);

   static native int nativeHLAopaqueDataOctetBoundary(long handle);

   static native int nativeHLAopaqueDataEncodedLength(long handle);

   static native byte[] nativeHLAopaqueDataToByteArray(long handle);

   static native void nativeDecodeHLAopaqueData(long handle, byte[] bytes);

   static native long nativeCreateHLAvariableArray(
      hla.rti1516_2025.encoding.DataElement prototype);

   static native void nativeDestroyHLAvariableArray(long handle);

   static native void nativeAddHLAvariableArrayElement(
      long handle, hla.rti1516_2025.encoding.DataElement element);

   static native void nativeResizeHLAvariableArray(long handle, int size);

   static native int nativeHLAvariableArraySize(long handle);

   static native byte[] nativeHLAvariableArrayElementEncoding(long handle, int index);

   static native int nativeHLAvariableArrayOctetBoundary(long handle);

   static native int nativeHLAvariableArrayEncodedLength(long handle);

   static native byte[] nativeHLAvariableArrayToByteArray(long handle);

   static native void nativeDecodeHLAvariableArray(long handle, byte[] bytes);

   static native long nativeCreateHLAfixedArray(
      hla.rti1516_2025.encoding.DataElement prototype, int size);
   static native void nativeDestroyHLAfixedArray(long handle);
   static native void nativeSetHLAfixedArrayElement(
      long handle, int index, hla.rti1516_2025.encoding.DataElement element);
   static native int nativeHLAfixedArraySize(long handle);
   static native byte[] nativeHLAfixedArrayElementEncoding(long handle, int index);
   static native int nativeHLAfixedArrayOctetBoundary(long handle);
   static native int nativeHLAfixedArrayEncodedLength(long handle);
   static native byte[] nativeHLAfixedArrayToByteArray(long handle);
   static native void nativeDecodeHLAfixedArray(long handle, byte[] bytes);

   static native long nativeCreateHLAfixedRecord();
   static native void nativeDestroyHLAfixedRecord(long handle);
   static native void nativeAppendHLAfixedRecordElement(
      long handle, hla.rti1516_2025.encoding.DataElement element);
   static native void nativeSetHLAfixedRecordElement(
      long handle, int index, hla.rti1516_2025.encoding.DataElement element);
   static native int nativeHLAfixedRecordSize(long handle);
   static native byte[] nativeHLAfixedRecordElementEncoding(long handle, int index);
   static native int nativeHLAfixedRecordOctetBoundary(long handle);
   static native int nativeHLAfixedRecordEncodedLength(long handle);
   static native byte[] nativeHLAfixedRecordToByteArray(long handle);
   static native void nativeDecodeHLAfixedRecord(long handle, byte[] bytes);

   static native long nativeCreateHLAvariantRecord(
      hla.rti1516_2025.encoding.DataElement discriminantPrototype);
   static native void nativeDestroyHLAvariantRecord(long handle);
   static native void nativeSetHLAvariantRecordVariant(
      long handle,
      hla.rti1516_2025.encoding.DataElement discriminant,
      hla.rti1516_2025.encoding.DataElement value);
   static native void nativeSetHLAvariantRecordDiscriminant(
      long handle, hla.rti1516_2025.encoding.DataElement discriminant);
   static native byte[] nativeHLAvariantRecordDiscriminantEncoding(long handle);
   static native byte[] nativeHLAvariantRecordValueEncoding(long handle);
   static native int nativeHLAvariantRecordOctetBoundary(long handle);
   static native int nativeHLAvariantRecordEncodedLength(long handle);
   static native byte[] nativeHLAvariantRecordToByteArray(long handle);
   static native void nativeDecodeHLAvariantRecord(long handle, byte[] bytes);

   static native long nativeCreateHLAextendableVariantRecord(
      hla.rti1516_2025.encoding.DataElement discriminantPrototype);
   static native void nativeDestroyHLAextendableVariantRecord(long handle);
   static native void nativeAddHLAextendableVariantRecordVariant(
      long handle,
      hla.rti1516_2025.encoding.DataElement discriminant,
      hla.rti1516_2025.encoding.DataElement value);
   static native void nativeSetHLAextendableVariantRecordVariant(
      long handle,
      hla.rti1516_2025.encoding.DataElement discriminant,
      hla.rti1516_2025.encoding.DataElement value);
   static native void nativeSetHLAextendableVariantRecordDiscriminant(
      long handle, hla.rti1516_2025.encoding.DataElement discriminant);
   static native byte[] nativeHLAextendableVariantRecordDiscriminantEncoding(long handle);
   static native byte[] nativeHLAextendableVariantRecordValueEncoding(long handle);
   static native int nativeHLAextendableVariantRecordOctetBoundary(long handle);
   static native int nativeHLAextendableVariantRecordEncodedLength(long handle);
   static native byte[] nativeHLAextendableVariantRecordToByteArray(long handle);
   static native void nativeDecodeHLAextendableVariantRecord(long handle, byte[] bytes);

   static native byte[] nativeDecodeFederateHandle(byte[] encodedValue);

   static native void nativeRegisterFederationSynchronizationPoint(
      long handle,
      String synchronizationPointLabel,
      byte[] userSuppliedTag,
      byte[][] synchronizationSet);

   static native void nativeSynchronizationPointAchieved(
      long handle,
      String synchronizationPointLabel,
      boolean successfully);

   static native void nativeQueryFederationSaveStatus(long handle);

   static native void nativeRequestFederationSave(long handle, String label);

   static native void nativeRequestFederationSaveWithTime(
      long handle, String label, byte[] encodedLogicalTime);

   static native void nativeFederateSaveBegun(long handle);

   static native void nativeFederateSaveComplete(long handle);

   static native void nativeFederateSaveNotComplete(long handle);

   static native void nativeAbortFederationSave(long handle);

   static native void nativeQueryFederationRestoreStatus(long handle);

   static native void nativeRequestFederationRestore(long handle, String label);

   static native void nativeFederateRestoreComplete(long handle);

   static native void nativeFederateRestoreNotComplete(long handle);

   static native void nativeAbortFederationRestore(long handle);

   static native AuthorizationResult nativeAuthorize(
      String password,
      Credentials credentials,
      String federationName,
      String federateName,
      String federateType);

   static ConfigurationResult connect(
      long handle,
      FederateAmbassador federateAmbassador,
      CallbackModel callbackModel,
      RtiConfiguration configuration,
      Credentials credentials) throws AlreadyConnected
   {
      return nativeConnect(
         handle,
         federateAmbassador,
         callbackModel.name(),
         configuration,
         credentials);
   }
}
