package org.umbra.jni.rti1516_2025;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.ConfigurationResult;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleSet;
import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.ServiceGroup;
import hla.rti1516_2025.auth.Credentials;
import hla.rti1516_2025.exceptions.NotConnected;
import hla.rti1516_2025.exceptions.RTIinternalError;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.net.URL;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Set;

/**
 * Invocation handler for Umbra's C++-backed Java {@link RTIambassador}.
 *
 * <p>The factory returns a standard Java dynamic proxy rather than extending a
 * mock ambassador. Every routed method crosses JNI; an unbound standard
 * service fails visibly instead of acquiring test-fixture semantics by
 * inheritance. Add a dispatch branch only with its C++ mapping and test.</p>
 */
final class NativeRTIambassador implements InvocationHandler, AutoCloseable {
   private long nativeHandle = NativeBridge.nativeCreate();

   static RTIambassador create() {
      NativeRTIambassador handler = new NativeRTIambassador();
      return (RTIambassador) Proxy.newProxyInstance(
         RTIambassador.class.getClassLoader(),
         new Class<?>[] { RTIambassador.class, AutoCloseable.class },
         handler);
   }

   /**
    * Deterministic embedded-transport fault source for JNI integration tests.
    * This package-private helper is not part of the standard Java API and is
    * intentionally reachable only through reflective test tooling.
    */
   static boolean failEmbeddedTransportConnectionForTesting(
      RTIambassador proxy, String faultDescription) {
      if (proxy == null || !Proxy.isProxyClass(proxy.getClass())) {
         throw new IllegalArgumentException("Expected the Umbra native RTI proxy");
      }
      InvocationHandler handler = Proxy.getInvocationHandler(proxy);
      if (!(handler instanceof NativeRTIambassador)) {
         throw new IllegalArgumentException("Expected the Umbra native RTI proxy");
      }
      return NativeBridge.nativeFailEmbeddedTransportConnectionForTesting(
         ((NativeRTIambassador) handler).requireNativeHandle(), faultDescription);
   }

   /**
    * Deterministic RTI-originated resignation source for JNI integration
    * tests. This package-private helper is not part of the standard Java API
    * and is intentionally reachable only through reflective test tooling.
    */
   static boolean forceEmbeddedFederateResignationForTesting(
      RTIambassador proxy, String reason) {
      if (proxy == null || !Proxy.isProxyClass(proxy.getClass())) {
         throw new IllegalArgumentException("Expected the Umbra native RTI proxy");
      }
      InvocationHandler handler = Proxy.getInvocationHandler(proxy);
      if (!(handler instanceof NativeRTIambassador)) {
         throw new IllegalArgumentException("Expected the Umbra native RTI proxy");
      }
      return NativeBridge.nativeForceEmbeddedFederateResignationForTesting(
         ((NativeRTIambassador) handler).requireNativeHandle(), reason);
   }

   private long requireNativeHandle() {
      if (nativeHandle == 0L) {
         throw new IllegalStateException("The Umbra JNI RTI ambassador is closed");
      }
      return nativeHandle;
   }

   @Override
   public Object invoke(Object proxy, Method method, Object[] arguments) throws Throwable {
      Object[] values = arguments == null ? new Object[0] : arguments;
      String name = method.getName();
      if (method.getDeclaringClass() == Object.class) {
         return invokeObjectMethod(proxy, method, values);
      }
      if ("close".equals(name) && values.length == 0) {
         close();
         return null;
      }
      if ("connect".equals(name)) return connect(values);
      if ("disconnect".equals(name) && values.length == 0) {
         try {
            NativeBridge.nativeDisconnect(requireNativeHandle());
         } catch (NotConnected ignored) {
            // IEEE 1516.1-2025 does not declare NotConnected for
            // disconnect. Preserve that public contract even though the
            // underlying C++ lifecycle checker detects a redundant close.
         }
         return null;
      }
      if ("evokeCallback".equals(name) && values.length == 1) {
         return NativeBridge.nativeEvokeCallback(
            requireNativeHandle(), ((Double) values[0]).doubleValue());
      }
      if ("evokeMultipleCallbacks".equals(name) && values.length == 2) {
         return NativeBridge.nativeEvokeMultipleCallbacks(
            requireNativeHandle(), ((Double) values[0]).doubleValue(),
            ((Double) values[1]).doubleValue());
      }
      if ("enableCallbacks".equals(name) && values.length == 0) {
         NativeBridge.nativeEnableCallbacks(requireNativeHandle());
         return null;
      }
      if ("disableCallbacks".equals(name) && values.length == 0) {
         NativeBridge.nativeDisableCallbacks(requireNativeHandle());
         return null;
      }
      if ("listFederationExecutions".equals(name) && values.length == 0) {
         NativeBridge.nativeListFederationExecutions(requireNativeHandle());
         return null;
      }
      if ("listFederationExecutionMembers".equals(name) && values.length == 1) {
         NativeBridge.nativeListFederationExecutionMembers(
            requireNativeHandle(), (String) values[0]);
         return null;
      }
      if ("createFederationExecution".equals(name) && values.length == 2) {
         NativeBridge.nativeCreateFederationExecution(
            requireNativeHandle(),
            (String) values[0],
            fomPaths(values[1]),
            "");
         return null;
      }
      if ("createFederationExecution".equals(name) && values.length == 3) {
         NativeBridge.nativeCreateFederationExecution(
            requireNativeHandle(),
            (String) values[0],
            fomPaths(values[1]),
            (String) values[2]);
         return null;
      }
      if ("createFederationExecutionWithMIM".equals(name) && values.length == 3) {
         NativeBridge.nativeCreateFederationExecutionWithMIM(
            requireNativeHandle(),
            (String) values[0],
            fomPaths(values[1]),
            fomPath(values[2]),
            "");
         return null;
      }
      if ("createFederationExecutionWithMIM".equals(name) && values.length == 4) {
         NativeBridge.nativeCreateFederationExecutionWithMIM(
            requireNativeHandle(),
            (String) values[0],
            fomPaths(values[1]),
            fomPath(values[2]),
            (String) values[3]);
         return null;
      }
      if ("destroyFederationExecution".equals(name) && values.length == 1) {
         NativeBridge.nativeDestroyFederationExecution(
            requireNativeHandle(), (String) values[0]);
         return null;
      }
      if ("joinFederationExecution".equals(name)) return joinFederationExecution(values);
      if ("resignFederationExecution".equals(name) && values.length == 1) {
         NativeBridge.nativeResignFederationExecution(
            requireNativeHandle(), ((ResignAction) values[0]).name());
         return null;
      }
      if ("getFederateHandle".equals(name) && values.length == 1) {
         return new NativeFederateHandle(NativeBridge.nativeGetFederateHandle(
            requireNativeHandle(), (String) values[0]));
      }
      if ("getFederateName".equals(name) && values.length == 1) {
         return NativeBridge.nativeGetFederateName(
            requireNativeHandle(), encodedFederateHandle((FederateHandle) values[0]));
      }
      if ("normalizeFederateHandle".equals(name) && values.length == 1) {
         return Long.valueOf((long) NativeBridge.nativeNormalizeFederateHandle(
            requireNativeHandle(), encodedFederateHandle((FederateHandle) values[0])));
      }
      if ("getObjectClassHandle".equals(name) && values.length == 1) {
         return new NativeObjectClassHandle(NativeBridge.nativeGetObjectClassHandle(
            requireNativeHandle(), (String) values[0]));
      }
      if ("getObjectClassName".equals(name) && values.length == 1) {
         return NativeBridge.nativeGetObjectClassName(
            requireNativeHandle(), encodedObjectClassHandle((ObjectClassHandle) values[0]));
      }
      if ("getKnownObjectClassHandle".equals(name) && values.length == 1) {
         return new NativeObjectClassHandle(NativeBridge.nativeGetKnownObjectClassHandle(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0])));
      }
      if ("normalizeObjectClassHandle".equals(name) && values.length == 1) {
         return Long.valueOf((long) NativeBridge.nativeNormalizeObjectClassHandle(
            requireNativeHandle(), encodedObjectClassHandle((ObjectClassHandle) values[0])));
      }
      if ("getAttributeHandle".equals(name) && values.length == 2) {
         return new NativeAttributeHandle(NativeBridge.nativeGetAttributeHandle(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (String) values[1]));
      }
      if ("getAttributeName".equals(name) && values.length == 2) {
         return NativeBridge.nativeGetAttributeName(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            encodedAttributeHandle((AttributeHandle) values[1]));
      }
      if ("getDimensionHandle".equals(name) && values.length == 1) {
         return new NativeDimensionHandle(NativeBridge.nativeGetDimensionHandle(
            requireNativeHandle(), (String) values[0]));
      }
      if ("getDimensionName".equals(name) && values.length == 1) {
         return NativeBridge.nativeGetDimensionName(
            requireNativeHandle(), encodedDimensionHandle((DimensionHandle) values[0]));
      }
      if ("getAvailableDimensionsForObjectClass".equals(name) && values.length == 1) {
         return dimensionHandleSet(NativeBridge.nativeGetAvailableDimensionsForObjectClass(
            requireNativeHandle(), encodedObjectClassHandle((ObjectClassHandle) values[0])));
      }
      if ("getAvailableDimensionsForInteractionClass".equals(name) && values.length == 1) {
         return dimensionHandleSet(NativeBridge.nativeGetAvailableDimensionsForInteractionClass(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0])));
      }
      if ("getDimensionUpperBound".equals(name) && values.length == 1) {
         return NativeBridge.nativeGetDimensionUpperBound(
            requireNativeHandle(), encodedDimensionHandle((DimensionHandle) values[0]));
      }
      if ("getDimensionHandleFactory".equals(name) && values.length == 0) {
         return new NativeDimensionHandleFactory();
      }
      if ("getDimensionHandleSetFactory".equals(name) && values.length == 0) {
         return new NativeDimensionHandleSetFactory();
      }
      if ("getRegionHandleFactory".equals(name) && values.length == 0) {
         return new NativeRegionHandleFactory();
      }
      if ("getMessageRetractionHandleFactory".equals(name) && values.length == 0) {
         return new NativeMessageRetractionHandleFactory(requireNativeHandle());
      }
      if ("getRegionHandleSetFactory".equals(name) && values.length == 0) {
         return new NativeRegionHandleSetFactory();
      }
      if ("createRegion".equals(name) && values.length == 1) {
         return new NativeRegionHandle(NativeBridge.nativeCreateRegion(
            requireNativeHandle(), (DimensionHandleSet) values[0]));
      }
      if ("commitRegionModifications".equals(name) && values.length == 1) {
         NativeBridge.nativeCommitRegionModifications(
            requireNativeHandle(), (RegionHandleSet) values[0]);
         return null;
      }
      if ("deleteRegion".equals(name) && values.length == 1) {
         NativeBridge.nativeDeleteRegion(
            requireNativeHandle(), encodedRegionHandle((RegionHandle) values[0]));
         return null;
      }
      if ("getDimensionHandleSet".equals(name) && values.length == 1) {
         return dimensionHandleSet(NativeBridge.nativeGetDimensionHandleSet(
            requireNativeHandle(), encodedRegionHandle((RegionHandle) values[0])));
      }
      if ("getRangeBounds".equals(name) && values.length == 2) {
         long[] bounds = NativeBridge.nativeGetRangeBounds(
            requireNativeHandle(),
            encodedRegionHandle((RegionHandle) values[0]),
            encodedDimensionHandle((DimensionHandle) values[1]));
         if (bounds == null || bounds.length != 2) {
            throw new IllegalStateException("The native RTI returned malformed region bounds");
         }
         return new RangeBounds(bounds[0], bounds[1]);
      }
      if ("setRangeBounds".equals(name) && values.length == 3) {
         RangeBounds bounds = (RangeBounds) values[2];
         NativeBridge.nativeSetRangeBounds(
            requireNativeHandle(),
            encodedRegionHandle((RegionHandle) values[0]),
            encodedDimensionHandle((DimensionHandle) values[1]),
            bounds.lower,
            bounds.upper);
         return null;
      }
      if ("getFederateHandleFactory".equals(name) && values.length == 0) {
         return new NativeFederateHandleFactory();
      }
      if ("getFederateHandleSetFactory".equals(name) && values.length == 0) {
         return new NativeFederateHandleSetFactory();
      }
      if ("getObjectClassHandleFactory".equals(name) && values.length == 0) {
         return new NativeObjectClassHandleFactory();
      }
      if ("getAttributeHandleFactory".equals(name) && values.length == 0) {
         return new NativeAttributeHandleFactory();
      }
      if ("getAttributeHandleSetFactory".equals(name) && values.length == 0) {
         return new NativeAttributeHandleSetFactory();
      }
      if ("getAttributeSetRegionSetPairListFactory".equals(name) && values.length == 0) {
         return attributeSetRegionSetPairListFactory(method.getReturnType());
      }
      if ("getAttributeHandleValueMapFactory".equals(name) && values.length == 0) {
         return new NativeAttributeHandleValueMapFactory();
      }
      if ("publishObjectClassAttributes".equals(name) && values.length == 2) {
         NativeBridge.nativePublishObjectClassAttributes(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (AttributeHandleSet) values[1]);
         return null;
      }
      if ("unpublishObjectClass".equals(name) && values.length == 1) {
         NativeBridge.nativeUnpublishObjectClass(
            requireNativeHandle(), encodedObjectClassHandle((ObjectClassHandle) values[0]));
         return null;
      }
      if ("unpublishObjectClassAttributes".equals(name) && values.length == 2) {
         NativeBridge.nativeUnpublishObjectClassAttributes(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (AttributeHandleSet) values[1]);
         return null;
      }
      if ("publishObjectClassDirectedInteractions".equals(name) && values.length == 2) {
         NativeBridge.nativePublishObjectClassDirectedInteractions(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (Set<?>) values[1]);
         return null;
      }
      if ("unpublishObjectClassDirectedInteractions".equals(name) && values.length == 1) {
         NativeBridge.nativeUnpublishObjectClassDirectedInteractions(
            requireNativeHandle(), encodedObjectClassHandle((ObjectClassHandle) values[0]));
         return null;
      }
      if ("unpublishObjectClassDirectedInteractions".equals(name) && values.length == 2) {
         NativeBridge.nativeUnpublishObjectClassDirectedInteractionsWithClasses(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (Set<?>) values[1]);
         return null;
      }
      if ("subscribeObjectClassAttributes".equals(name)
         && (values.length == 2 || values.length == 3)) {
         NativeBridge.nativeSubscribeObjectClassAttributes(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (AttributeHandleSet) values[1],
            true,
            values.length == 3 ? (String) values[2] : "");
         return null;
      }
      if ("subscribeObjectClassAttributesPassively".equals(name)
         && (values.length == 2 || values.length == 3)) {
         NativeBridge.nativeSubscribeObjectClassAttributes(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (AttributeHandleSet) values[1],
            false,
            values.length == 3 ? (String) values[2] : "");
         return null;
      }
      if ("unsubscribeObjectClass".equals(name) && values.length == 1) {
         NativeBridge.nativeUnsubscribeObjectClass(
            requireNativeHandle(), encodedObjectClassHandle((ObjectClassHandle) values[0]));
         return null;
      }
      if ("unsubscribeObjectClassAttributes".equals(name) && values.length == 2) {
         NativeBridge.nativeUnsubscribeObjectClassAttributes(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (AttributeHandleSet) values[1]);
         return null;
      }
      if ("subscribeObjectClassDirectedInteractions".equals(name) && values.length == 2) {
         NativeBridge.nativeSubscribeObjectClassDirectedInteractions(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (Set<?>) values[1],
            false);
         return null;
      }
      if ("subscribeObjectClassDirectedInteractionsUniversally".equals(name)
         && values.length == 2) {
         NativeBridge.nativeSubscribeObjectClassDirectedInteractions(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (Set<?>) values[1],
            true);
         return null;
      }
      if ("unsubscribeObjectClassDirectedInteractions".equals(name) && values.length == 1) {
         NativeBridge.nativeUnsubscribeObjectClassDirectedInteractions(
            requireNativeHandle(), encodedObjectClassHandle((ObjectClassHandle) values[0]));
         return null;
      }
      if ("unsubscribeObjectClassDirectedInteractions".equals(name) && values.length == 2) {
         NativeBridge.nativeUnsubscribeObjectClassDirectedInteractionsWithClasses(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (Set<?>) values[1]);
         return null;
      }
      if ("subscribeObjectClassAttributesWithRegions".equals(name)
         && (values.length == 2 || values.length == 3)) {
         NativeBridge.nativeSubscribeObjectClassAttributesWithRegions(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (AttributeSetRegionSetPairList) values[1],
            true,
            values.length == 3 ? (String) values[2] : "");
         return null;
      }
      if ("subscribeObjectClassAttributesPassivelyWithRegions".equals(name)
         && (values.length == 2 || values.length == 3)) {
         NativeBridge.nativeSubscribeObjectClassAttributesWithRegions(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (AttributeSetRegionSetPairList) values[1],
            false,
            values.length == 3 ? (String) values[2] : "");
         return null;
      }
      if ("unsubscribeObjectClassAttributesWithRegions".equals(name) && values.length == 2) {
         NativeBridge.nativeUnsubscribeObjectClassAttributesWithRegions(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (AttributeSetRegionSetPairList) values[1]);
         return null;
      }
      if ("reserveObjectInstanceName".equals(name) && values.length == 1) {
         NativeBridge.nativeReserveObjectInstanceName(
            requireNativeHandle(), (String) values[0]);
         return null;
      }
      if ("releaseObjectInstanceName".equals(name) && values.length == 1) {
         NativeBridge.nativeReleaseObjectInstanceName(
            requireNativeHandle(), (String) values[0]);
         return null;
      }
      if ("reserveMultipleObjectInstanceNames".equals(name) && values.length == 1) {
         NativeBridge.nativeReserveMultipleObjectInstanceNames(
            requireNativeHandle(), (java.util.Set<?>) values[0]);
         return null;
      }
      if ("releaseMultipleObjectInstanceNames".equals(name) && values.length == 1) {
         NativeBridge.nativeReleaseMultipleObjectInstanceNames(
            requireNativeHandle(), (java.util.Set<?>) values[0]);
         return null;
      }
      if ("registerObjectInstance".equals(name) &&
          (values.length == 1 || values.length == 2)) {
         return new NativeObjectInstanceHandle(NativeBridge.nativeRegisterObjectInstance(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            values.length == 2 ? (String) values[1] : null));
      }
      if ("registerObjectInstanceWithRegions".equals(name) &&
          (values.length == 2 || values.length == 3)) {
         return new NativeObjectInstanceHandle(
            NativeBridge.nativeRegisterObjectInstanceWithRegions(
               requireNativeHandle(),
               encodedObjectClassHandle((ObjectClassHandle) values[0]),
               (AttributeSetRegionSetPairList) values[1],
               values.length == 3 ? (String) values[2] : null));
      }
      if ("associateRegionsForUpdates".equals(name) && values.length == 2) {
         NativeBridge.nativeAssociateRegionsForUpdates(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeSetRegionSetPairList) values[1]);
         return null;
      }
      if ("unassociateRegionsForUpdates".equals(name) && values.length == 2) {
         NativeBridge.nativeUnassociateRegionsForUpdates(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeSetRegionSetPairList) values[1]);
         return null;
      }
      if ("getObjectInstanceHandle".equals(name) && values.length == 1) {
         return new NativeObjectInstanceHandle(NativeBridge.nativeGetObjectInstanceHandle(
            requireNativeHandle(), (String) values[0]));
      }
      if ("getObjectInstanceName".equals(name) && values.length == 1) {
         return NativeBridge.nativeGetObjectInstanceName(
            requireNativeHandle(), encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]));
      }
      if ("normalizeObjectInstanceHandle".equals(name) && values.length == 1) {
         return Long.valueOf((long) NativeBridge.nativeNormalizeObjectInstanceHandle(
            requireNativeHandle(), encodedObjectInstanceHandle((ObjectInstanceHandle) values[0])));
      }
      if ("getObjectInstanceHandleFactory".equals(name) && values.length == 0) {
         return new NativeObjectInstanceHandleFactory();
      }
      if ("updateAttributeValues".equals(name) && values.length == 3) {
         NativeBridge.nativeUpdateAttributeValues(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleValueMap) values[1],
            (byte[]) values[2]);
         return null;
      }
      if ("updateAttributeValues".equals(name) && values.length == 4) {
         return messageRetractionResult(method, NativeBridge.nativeUpdateAttributeValuesWithTime(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleValueMap) values[1],
            (byte[]) values[2],
            encodedLogicalTime((LogicalTime) values[3])));
      }
      if ("deleteObjectInstance".equals(name) && values.length == 2) {
         NativeBridge.nativeDeleteObjectInstance(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (byte[]) values[1]);
         return null;
      }
      if ("deleteObjectInstance".equals(name) && values.length == 3) {
         return messageRetractionResult(method, NativeBridge.nativeDeleteObjectInstanceWithTime(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (byte[]) values[1],
            encodedLogicalTime((LogicalTime) values[2])));
      }
      if ("localDeleteObjectInstance".equals(name) && values.length == 1) {
         NativeBridge.nativeLocalDeleteObjectInstance(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]));
         return null;
      }
      if ("requestAttributeValueUpdate".equals(name) && values.length == 3) {
         if (values[0] instanceof ObjectInstanceHandle) {
            NativeBridge.nativeRequestAttributeValueUpdateForObjectInstance(
               requireNativeHandle(),
               encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
               (AttributeHandleSet) values[1],
               (byte[]) values[2]);
            return null;
         }
         if (values[0] instanceof ObjectClassHandle) {
            NativeBridge.nativeRequestAttributeValueUpdateForObjectClass(
               requireNativeHandle(),
               encodedObjectClassHandle((ObjectClassHandle) values[0]),
               (AttributeHandleSet) values[1],
               (byte[]) values[2]);
            return null;
         }
      }
      if ("requestAttributeValueUpdateWithRegions".equals(name) && values.length == 3) {
         NativeBridge.nativeRequestAttributeValueUpdateWithRegions(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (AttributeSetRegionSetPairList) values[1],
            (byte[]) values[2]);
         return null;
      }
      if ("queryAttributeOwnership".equals(name) && values.length == 2) {
         NativeBridge.nativeQueryAttributeOwnership(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleSet) values[1]);
         return null;
      }
      if ("isAttributeOwnedByFederate".equals(name) && values.length == 2) {
         return NativeBridge.nativeIsAttributeOwnedByFederate(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            encodedAttributeHandle((AttributeHandle) values[1]));
      }
      if ("unconditionalAttributeOwnershipDivestiture".equals(name) &&
          values.length == 3) {
         NativeBridge.nativeUnconditionalAttributeOwnershipDivestiture(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleSet) values[1],
            (byte[]) values[2]);
         return null;
      }
      if ("negotiatedAttributeOwnershipDivestiture".equals(name) && values.length == 3) {
         NativeBridge.nativeNegotiatedAttributeOwnershipDivestiture(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleSet) values[1],
            (byte[]) values[2]);
         return null;
      }
      if ("confirmDivestiture".equals(name) && values.length == 3) {
         NativeBridge.nativeConfirmDivestiture(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleSet) values[1],
            (byte[]) values[2]);
         return null;
      }
      if ("cancelNegotiatedAttributeOwnershipDivestiture".equals(name) &&
          values.length == 2) {
         NativeBridge.nativeCancelNegotiatedAttributeOwnershipDivestiture(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleSet) values[1]);
         return null;
      }
      if ("attributeOwnershipAcquisition".equals(name) && values.length == 3) {
         NativeBridge.nativeAttributeOwnershipAcquisition(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleSet) values[1],
            (byte[]) values[2]);
         return null;
      }
      if ("attributeOwnershipAcquisitionIfAvailable".equals(name) &&
          values.length == 3) {
         NativeBridge.nativeAttributeOwnershipAcquisitionIfAvailable(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleSet) values[1],
            (byte[]) values[2]);
         return null;
      }
      if ("cancelAttributeOwnershipAcquisition".equals(name) && values.length == 2) {
         NativeBridge.nativeCancelAttributeOwnershipAcquisition(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleSet) values[1]);
         return null;
      }
      if ("attributeOwnershipReleaseDenied".equals(name) && values.length == 3) {
         NativeBridge.nativeAttributeOwnershipReleaseDenied(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleSet) values[1],
            (byte[]) values[2]);
         return null;
      }
      if ("attributeOwnershipDivestitureIfWanted".equals(name) && values.length == 3) {
         return attributeHandleSet(NativeBridge.nativeAttributeOwnershipDivestitureIfWanted(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleSet) values[1],
            (byte[]) values[2]));
      }
      if ("queryAttributeTransportationType".equals(name) && values.length == 2) {
         NativeBridge.nativeQueryAttributeTransportationType(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            encodedAttributeHandle((AttributeHandle) values[1]));
         return null;
      }
      if ("queryInteractionTransportationType".equals(name) && values.length == 2) {
         NativeBridge.nativeQueryInteractionTransportationType(
            requireNativeHandle(),
            encodedFederateHandle((FederateHandle) values[0]),
            encodedInteractionClassHandle((InteractionClassHandle) values[1]));
         return null;
      }
      if ("requestAttributeTransportationTypeChange".equals(name) && values.length == 3) {
         NativeBridge.nativeRequestAttributeTransportationTypeChange(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleSet) values[1],
            encodedTransportationTypeHandle((TransportationTypeHandle) values[2]));
         return null;
      }
      if ("requestInteractionTransportationTypeChange".equals(name) && values.length == 2) {
         NativeBridge.nativeRequestInteractionTransportationTypeChange(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            encodedTransportationTypeHandle((TransportationTypeHandle) values[1]));
         return null;
      }
      if ("setAttributeScopeAdvisorySwitch".equals(name) && values.length == 1) {
         NativeBridge.nativeSetAttributeScopeAdvisorySwitch(
            requireNativeHandle(), ((Boolean) values[0]).booleanValue());
         return null;
      }
      if ("getObjectClassRelevanceAdvisorySwitch".equals(name) && values.length == 0) {
         return NativeBridge.nativeGetObjectClassRelevanceAdvisorySwitch(requireNativeHandle());
      }
      if ("setObjectClassRelevanceAdvisorySwitch".equals(name) && values.length == 1) {
         NativeBridge.nativeSetObjectClassRelevanceAdvisorySwitch(
            requireNativeHandle(), ((Boolean) values[0]).booleanValue());
         return null;
      }
      if ("getAttributeRelevanceAdvisorySwitch".equals(name) && values.length == 0) {
         return NativeBridge.nativeGetAttributeRelevanceAdvisorySwitch(requireNativeHandle());
      }
      if ("setAttributeRelevanceAdvisorySwitch".equals(name) && values.length == 1) {
         NativeBridge.nativeSetAttributeRelevanceAdvisorySwitch(
            requireNativeHandle(), ((Boolean) values[0]).booleanValue());
         return null;
      }
      if ("getAttributeScopeAdvisorySwitch".equals(name) && values.length == 0) {
         return NativeBridge.nativeGetAttributeScopeAdvisorySwitch(requireNativeHandle());
      }
      if ("getInteractionRelevanceAdvisorySwitch".equals(name) && values.length == 0) {
         return NativeBridge.nativeGetInteractionRelevanceAdvisorySwitch(requireNativeHandle());
      }
      if ("setInteractionRelevanceAdvisorySwitch".equals(name) && values.length == 1) {
         NativeBridge.nativeSetInteractionRelevanceAdvisorySwitch(
            requireNativeHandle(), ((Boolean) values[0]).booleanValue());
         return null;
      }
      if ("getAutomaticResignDirective".equals(name) && values.length == 0) {
         return ResignAction.valueOf(
            NativeBridge.nativeGetAutomaticResignDirective(requireNativeHandle()));
      }
      if ("setAutomaticResignDirective".equals(name) && values.length == 1) {
         NativeBridge.nativeSetAutomaticResignDirective(
            requireNativeHandle(), ((ResignAction) values[0]).name());
         return null;
      }
      if ("getServiceReportingSwitch".equals(name) && values.length == 0) {
         return NativeBridge.nativeGetServiceReportingSwitch(requireNativeHandle());
      }
      if ("setServiceReportingSwitch".equals(name) && values.length == 1) {
         NativeBridge.nativeSetServiceReportingSwitch(
            requireNativeHandle(), ((Boolean) values[0]).booleanValue());
         return null;
      }
      if ("getExceptionReportingSwitch".equals(name) && values.length == 0) {
         return NativeBridge.nativeGetExceptionReportingSwitch(requireNativeHandle());
      }
      if ("setExceptionReportingSwitch".equals(name) && values.length == 1) {
         NativeBridge.nativeSetExceptionReportingSwitch(
            requireNativeHandle(), ((Boolean) values[0]).booleanValue());
         return null;
      }
      if ("getSendServiceReportsToFileSwitch".equals(name) && values.length == 0) {
         return NativeBridge.nativeGetSendServiceReportsToFileSwitch(requireNativeHandle());
      }
      if ("setSendServiceReportsToFileSwitch".equals(name) && values.length == 1) {
         NativeBridge.nativeSetSendServiceReportsToFileSwitch(
            requireNativeHandle(), ((Boolean) values[0]).booleanValue());
         return null;
      }
      if ("getHLAversion".equals(name) && values.length == 0) {
         return NativeBridge.nativeGetHLAversion(requireNativeHandle());
      }
      if ("getAutoProvideSwitch".equals(name) && values.length == 0) {
         return NativeBridge.nativeGetAutoProvideSwitch(requireNativeHandle());
      }
      if ("getDelaySubscriptionEvaluationSwitch".equals(name) && values.length == 0) {
         return NativeBridge.nativeGetDelaySubscriptionEvaluationSwitch(requireNativeHandle());
      }
      if ("getAdvisoriesUseKnownClassSwitch".equals(name) && values.length == 0) {
         return NativeBridge.nativeGetAdvisoriesUseKnownClassSwitch(requireNativeHandle());
      }
      if ("getAllowRelaxedDDMSwitch".equals(name) && values.length == 0) {
         return NativeBridge.nativeGetAllowRelaxedDDMSwitch(requireNativeHandle());
      }
      if ("getNonRegulatedGrantSwitch".equals(name) && values.length == 0) {
         return NativeBridge.nativeGetNonRegulatedGrantSwitch(requireNativeHandle());
      }
      if ("normalizeServiceGroup".equals(name) && values.length == 1) {
         return Long.valueOf((long) NativeBridge.nativeNormalizeServiceGroup(
            requireNativeHandle(), ((ServiceGroup) values[0]).name()));
      }
      if ("getOrderType".equals(name) && values.length == 1) {
         return OrderType.valueOf(NativeBridge.nativeGetOrderType(
            requireNativeHandle(), (String) values[0]));
      }
      if ("getOrderName".equals(name) && values.length == 1) {
         return NativeBridge.nativeGetOrderName(
            requireNativeHandle(), ((OrderType) values[0]).name());
      }
      if ("changeAttributeOrderType".equals(name) && values.length == 3) {
         NativeBridge.nativeChangeAttributeOrderType(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleSet) values[1],
            ((OrderType) values[2]).name());
         return null;
      }
      if ("changeDefaultAttributeOrderType".equals(name) && values.length == 3) {
         NativeBridge.nativeChangeDefaultAttributeOrderType(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (AttributeHandleSet) values[1],
            ((OrderType) values[2]).name());
         return null;
      }
      if ("changeInteractionOrderType".equals(name) && values.length == 2) {
         NativeBridge.nativeChangeInteractionOrderType(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            ((OrderType) values[1]).name());
         return null;
      }
      if ("getUpdateRateValue".equals(name) && values.length == 1) {
         return NativeBridge.nativeGetUpdateRateValue(
            requireNativeHandle(), (String) values[0]);
      }
      if ("getUpdateRateValueForAttribute".equals(name) && values.length == 2) {
         return NativeBridge.nativeGetUpdateRateValueForAttribute(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            encodedAttributeHandle((AttributeHandle) values[1]));
      }
      if ("changeDefaultAttributeTransportationType".equals(name) && values.length == 3) {
         NativeBridge.nativeChangeDefaultAttributeTransportationType(
            requireNativeHandle(),
            encodedObjectClassHandle((ObjectClassHandle) values[0]),
            (AttributeHandleSet) values[1],
            encodedTransportationTypeHandle((TransportationTypeHandle) values[2]));
         return null;
      }
      if ("getTimeFactory".equals(name) && values.length == 0) {
         return selectedLogicalTimeFactory();
      }
      if ("enableTimeRegulation".equals(name) && values.length == 1) {
         NativeBridge.nativeEnableTimeRegulation(
            requireNativeHandle(), encodedLogicalTimeInterval((LogicalTimeInterval) values[0]));
         return null;
      }
      if ("disableTimeRegulation".equals(name) && values.length == 0) {
         NativeBridge.nativeDisableTimeRegulation(requireNativeHandle());
         return null;
      }
      if ("enableTimeConstrained".equals(name) && values.length == 0) {
         NativeBridge.nativeEnableTimeConstrained(requireNativeHandle());
         return null;
      }
      if ("disableTimeConstrained".equals(name) && values.length == 0) {
         NativeBridge.nativeDisableTimeConstrained(requireNativeHandle());
         return null;
      }
      if ("enableAsynchronousDelivery".equals(name) && values.length == 0) {
         NativeBridge.nativeEnableAsynchronousDelivery(requireNativeHandle());
         return null;
      }
      if ("disableAsynchronousDelivery".equals(name) && values.length == 0) {
         NativeBridge.nativeDisableAsynchronousDelivery(requireNativeHandle());
         return null;
      }
      if ("modifyLookahead".equals(name) && values.length == 1) {
         NativeBridge.nativeModifyLookahead(
            requireNativeHandle(), encodedLogicalTimeInterval((LogicalTimeInterval) values[0]));
         return null;
      }
      if ("queryLookahead".equals(name) && values.length == 0) {
         return selectedLogicalTimeFactory().decodeInterval(
            NativeBridge.nativeQueryLookahead(requireNativeHandle()), 0);
      }
      if ("timeAdvanceRequest".equals(name) && values.length == 1) {
         NativeBridge.nativeTimeAdvanceRequest(
            requireNativeHandle(), encodedLogicalTime((LogicalTime) values[0]));
         return null;
      }
      if ("timeAdvanceRequestAvailable".equals(name) && values.length == 1) {
         NativeBridge.nativeTimeAdvanceRequestAvailable(
            requireNativeHandle(), encodedLogicalTime((LogicalTime) values[0]));
         return null;
      }
      if ("nextMessageRequest".equals(name) && values.length == 1) {
         NativeBridge.nativeNextMessageRequest(
            requireNativeHandle(), encodedLogicalTime((LogicalTime) values[0]));
         return null;
      }
      if ("nextMessageRequestAvailable".equals(name) && values.length == 1) {
         NativeBridge.nativeNextMessageRequestAvailable(
            requireNativeHandle(), encodedLogicalTime((LogicalTime) values[0]));
         return null;
      }
      if ("flushQueueRequest".equals(name) && values.length == 1) {
         NativeBridge.nativeFlushQueueRequest(
            requireNativeHandle(), encodedLogicalTime((LogicalTime) values[0]));
         return null;
      }
      if ("queryLogicalTime".equals(name) && values.length == 0) {
         return selectedLogicalTimeFactory().decodeTime(
            NativeBridge.nativeQueryLogicalTime(requireNativeHandle()), 0);
      }
      if ("queryGALT".equals(name) && values.length == 0) {
         return timeQueryReturn(NativeBridge.nativeQueryGALT(requireNativeHandle()));
      }
      if ("queryLITS".equals(name) && values.length == 0) {
         return timeQueryReturn(NativeBridge.nativeQueryLITS(requireNativeHandle()));
      }
      if ("sendInteractionWithTime".equals(name) && values.length == 4) {
         return messageRetractionResult(method, NativeBridge.nativeSendInteractionWithTime(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            (ParameterHandleValueMap) values[1],
            (byte[]) values[2],
            encodedLogicalTime((LogicalTime) values[3])));
      }
      if ("retract".equals(name) && values.length == 1) {
         NativeBridge.nativeRetract(
            requireNativeHandle(),
            encodedMessageRetractionHandle((MessageRetractionHandle) values[0]));
         return null;
      }
      if ("updateAttributeValuesWithTime".equals(name) && values.length == 4) {
         return messageRetractionResult(method, NativeBridge.nativeUpdateAttributeValuesWithTime(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (AttributeHandleValueMap) values[1],
            (byte[]) values[2],
            encodedLogicalTime((LogicalTime) values[3])));
      }
      if ("deleteObjectInstanceWithTime".equals(name) && values.length == 3) {
         return messageRetractionResult(method, NativeBridge.nativeDeleteObjectInstanceWithTime(
            requireNativeHandle(),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[0]),
            (byte[]) values[1],
            encodedLogicalTime((LogicalTime) values[2])));
      }
      if ("getInteractionClassHandle".equals(name) && values.length == 1) {
         return new NativeInteractionClassHandle(NativeBridge.nativeGetInteractionClassHandle(
            requireNativeHandle(), (String) values[0]));
      }
      if ("getInteractionClassName".equals(name) && values.length == 1) {
         return NativeBridge.nativeGetInteractionClassName(
            requireNativeHandle(), encodedInteractionClassHandle((InteractionClassHandle) values[0]));
      }
      if ("normalizeInteractionClassHandle".equals(name) && values.length == 1) {
         return Long.valueOf((long) NativeBridge.nativeNormalizeInteractionClassHandle(
            requireNativeHandle(), encodedInteractionClassHandle((InteractionClassHandle) values[0])));
      }
      if ("getInteractionClassHandleFactory".equals(name) && values.length == 0) {
         return new NativeInteractionClassHandleFactory();
      }
      if ("getInteractionClassHandleSetFactory".equals(name) && values.length == 0) {
         return new NativeInteractionClassHandleSetFactory();
      }
      if ("getParameterHandle".equals(name) && values.length == 2) {
         return new NativeParameterHandle(NativeBridge.nativeGetParameterHandle(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            (String) values[1]));
      }
      if ("getParameterName".equals(name) && values.length == 2) {
         return NativeBridge.nativeGetParameterName(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            encodedParameterHandle((ParameterHandle) values[1]));
      }
      if ("getParameterHandleFactory".equals(name) && values.length == 0) {
         return new NativeParameterHandleFactory();
      }
      if ("getParameterHandleValueMapFactory".equals(name) && values.length == 0) {
         return new NativeParameterHandleValueMapFactory();
      }
      if ("publishInteractionClass".equals(name) && values.length == 1) {
         NativeBridge.nativePublishInteractionClass(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]));
         return null;
      }
      if ("unpublishInteractionClass".equals(name) && values.length == 1) {
         NativeBridge.nativeUnpublishInteractionClass(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]));
         return null;
      }
      if ("subscribeInteractionClass".equals(name) && values.length == 1) {
         NativeBridge.nativeSubscribeInteractionClass(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            true);
         return null;
      }
      if ("subscribeInteractionClassPassively".equals(name) && values.length == 1) {
         NativeBridge.nativeSubscribeInteractionClass(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            false);
         return null;
      }
      if ("unsubscribeInteractionClass".equals(name) && values.length == 1) {
         NativeBridge.nativeUnsubscribeInteractionClass(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]));
         return null;
      }
      if ("subscribeInteractionClassWithRegions".equals(name) && values.length == 2) {
         NativeBridge.nativeSubscribeInteractionClassWithRegions(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            (RegionHandleSet) values[1],
            true);
         return null;
      }
      if ("subscribeInteractionClassPassivelyWithRegions".equals(name) && values.length == 2) {
         NativeBridge.nativeSubscribeInteractionClassWithRegions(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            (RegionHandleSet) values[1],
            false);
         return null;
      }
      if ("unsubscribeInteractionClassWithRegions".equals(name) && values.length == 2) {
         NativeBridge.nativeUnsubscribeInteractionClassWithRegions(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            (RegionHandleSet) values[1]);
         return null;
      }
      if ("getTransportationTypeHandle".equals(name) && values.length == 1) {
         return new NativeTransportationTypeHandle(
            NativeBridge.nativeGetTransportationTypeHandle(
               requireNativeHandle(), (String) values[0]));
      }
      if ("getTransportationTypeName".equals(name) && values.length == 1) {
         return NativeBridge.nativeGetTransportationTypeName(
            requireNativeHandle(),
            encodedTransportationTypeHandle((TransportationTypeHandle) values[0]));
      }
      if ("getTransportationTypeHandleFactory".equals(name) && values.length == 0) {
         return new NativeTransportationTypeHandleFactory(requireNativeHandle());
      }
      if ("sendInteraction".equals(name) && values.length == 3) {
         NativeBridge.nativeSendInteraction(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            (ParameterHandleValueMap) values[1],
            (byte[]) values[2]);
         return null;
      }
      if ("sendInteraction".equals(name) && values.length == 4) {
         return messageRetractionResult(method, NativeBridge.nativeSendInteractionWithTime(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            (ParameterHandleValueMap) values[1],
            (byte[]) values[2],
            encodedLogicalTime((LogicalTime) values[3])));
      }
      if ("sendDirectedInteraction".equals(name) && values.length == 4) {
         NativeBridge.nativeSendDirectedInteraction(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[1]),
            (ParameterHandleValueMap) values[2],
            (byte[]) values[3]);
         return null;
      }
      if ("sendDirectedInteraction".equals(name) && values.length == 5) {
         return messageRetractionResult(method, NativeBridge.nativeSendDirectedInteractionWithTime(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            encodedObjectInstanceHandle((ObjectInstanceHandle) values[1]),
            (ParameterHandleValueMap) values[2],
            (byte[]) values[3],
            encodedLogicalTime((LogicalTime) values[4])));
      }
      if ("sendInteractionWithRegions".equals(name) && values.length == 4) {
         NativeBridge.nativeSendInteractionWithRegions(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            (ParameterHandleValueMap) values[1],
            (RegionHandleSet) values[2],
            (byte[]) values[3]);
         return null;
      }
      if ("sendInteractionWithRegions".equals(name) && values.length == 5) {
         return messageRetractionResult(method, NativeBridge.nativeSendInteractionWithRegionsWithTime(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            (ParameterHandleValueMap) values[1],
            (RegionHandleSet) values[2],
            (byte[]) values[3],
            encodedLogicalTime((LogicalTime) values[4])));
      }
      if ("sendInteractionWithRegionsWithTime".equals(name) && values.length == 5) {
         return messageRetractionResult(method, NativeBridge.nativeSendInteractionWithRegionsWithTime(
            requireNativeHandle(),
            encodedInteractionClassHandle((InteractionClassHandle) values[0]),
            (ParameterHandleValueMap) values[1],
            (RegionHandleSet) values[2],
            (byte[]) values[3],
            encodedLogicalTime((LogicalTime) values[4])));
      }
      if ("getConveyRegionDesignatorSetsSwitch".equals(name) && values.length == 0) {
         return NativeBridge.nativeGetConveyRegionDesignatorSetsSwitch(requireNativeHandle());
      }
      if ("setConveyRegionDesignatorSetsSwitch".equals(name) && values.length == 1) {
         NativeBridge.nativeSetConveyRegionDesignatorSetsSwitch(
            requireNativeHandle(), ((Boolean) values[0]).booleanValue());
         return null;
      }
      if ("registerFederationSynchronizationPoint".equals(name) &&
          (values.length == 2 || values.length == 3)) {
         NativeBridge.nativeRegisterFederationSynchronizationPoint(
            requireNativeHandle(),
            (String) values[0],
            (byte[]) values[1],
            values.length == 3 ? encodedFederateHandles((FederateHandleSet) values[2]) : null);
         return null;
      }
      if ("synchronizationPointAchieved".equals(name) &&
          (values.length == 1 || values.length == 2)) {
         NativeBridge.nativeSynchronizationPointAchieved(
            requireNativeHandle(),
            (String) values[0],
            values.length == 1 || ((Boolean) values[1]).booleanValue());
         return null;
      }
      if ("queryFederationSaveStatus".equals(name) && values.length == 0) {
         NativeBridge.nativeQueryFederationSaveStatus(requireNativeHandle());
         return null;
      }
      if ("requestFederationSave".equals(name) && values.length == 1) {
         NativeBridge.nativeRequestFederationSave(requireNativeHandle(), (String) values[0]);
         return null;
      }
      if ("requestFederationSave".equals(name) && values.length == 2) {
         NativeBridge.nativeRequestFederationSaveWithTime(
            requireNativeHandle(),
            (String) values[0],
            encodedLogicalTime((LogicalTime) values[1]));
         return null;
      }
      if ("federateSaveBegun".equals(name) && values.length == 0) {
         NativeBridge.nativeFederateSaveBegun(requireNativeHandle());
         return null;
      }
      if ("federateSaveComplete".equals(name) && values.length == 0) {
         NativeBridge.nativeFederateSaveComplete(requireNativeHandle());
         return null;
      }
      if ("federateSaveNotComplete".equals(name) && values.length == 0) {
         NativeBridge.nativeFederateSaveNotComplete(requireNativeHandle());
         return null;
      }
      if ("abortFederationSave".equals(name) && values.length == 0) {
         NativeBridge.nativeAbortFederationSave(requireNativeHandle());
         return null;
      }
      if ("queryFederationRestoreStatus".equals(name) && values.length == 0) {
         NativeBridge.nativeQueryFederationRestoreStatus(requireNativeHandle());
         return null;
      }
      if ("requestFederationRestore".equals(name) && values.length == 1) {
         NativeBridge.nativeRequestFederationRestore(requireNativeHandle(), (String) values[0]);
         return null;
      }
      if ("federateRestoreComplete".equals(name) && values.length == 0) {
         NativeBridge.nativeFederateRestoreComplete(requireNativeHandle());
         return null;
      }
      if ("federateRestoreNotComplete".equals(name) && values.length == 0) {
         NativeBridge.nativeFederateRestoreNotComplete(requireNativeHandle());
         return null;
      }
      if ("abortFederationRestore".equals(name) && values.length == 0) {
         NativeBridge.nativeAbortFederationRestore(requireNativeHandle());
         return null;
      }
      throw unavailable(method.toGenericString());
   }

   private ConfigurationResult connect(Object[] values) throws Exception {
      if (values.length < 2 || values.length > 4) throw unavailable("connect");
      FederateAmbassador callbacks = (FederateAmbassador) values[0];
      CallbackModel callbackModel = (CallbackModel) values[1];
      RtiConfiguration configuration = null;
      Credentials credentials = null;
      if (values.length == 3) {
         if (values[2] instanceof RtiConfiguration) configuration = (RtiConfiguration) values[2];
         else credentials = (Credentials) values[2];
      } else if (values.length == 4) {
         configuration = (RtiConfiguration) values[2];
         credentials = (Credentials) values[3];
      }
      return NativeBridge.connect(requireNativeHandle(), callbacks, callbackModel, configuration, credentials);
   }

   private NativeFederateHandle joinFederationExecution(Object[] values) throws Exception {
      String federateName = null;
      String federateType;
      String federationName;
      String[] additionalFomModules = new String[0];
      if (values.length == 2) {
         federateType = (String) values[0];
         federationName = (String) values[1];
      } else if (values.length == 3 && isFomModuleSequence(values[2])) {
         federateType = (String) values[0];
         federationName = (String) values[1];
         additionalFomModules = fomPaths(values[2]);
      } else if (values.length == 3) {
         federateName = (String) values[0];
         federateType = (String) values[1];
         federationName = (String) values[2];
      } else if (values.length == 4) {
         federateName = (String) values[0];
         federateType = (String) values[1];
         federationName = (String) values[2];
         additionalFomModules = fomPaths(values[3]);
      } else {
         throw unavailable("joinFederationExecution");
      }
      return new NativeFederateHandle(NativeBridge.nativeJoinFederationExecution(
         requireNativeHandle(), federateName, federateType, federationName, additionalFomModules));
   }

   private static Object invokeObjectMethod(Object proxy, Method method, Object[] values) {
      switch (method.getName()) {
         case "toString": return "Umbra JNI C++ RTI ambassador";
         case "hashCode": return System.identityHashCode(proxy);
         case "equals": return values.length == 1 && proxy == values[0];
         default: throw new AssertionError("Unexpected Object method " + method);
      }
   }

   /**
    * Construct IEEE collection interfaces without imposing their concrete
    * implementation on a vendor JAR. The local compatibility fixture predates
    * this factory, so reflection keeps its smaller API independently usable.
    */
   private static Object attributeSetRegionSetPairListFactory(Class<?> factoryType) {
      return Proxy.newProxyInstance(
         factoryType.getClassLoader(),
         new Class<?>[] { factoryType },
         (proxy, method, arguments) -> {
            Object[] values = arguments == null ? new Object[0] : arguments;
            if ("create".equals(method.getName()) && values.length == 1) {
               return attributeSetRegionSetPairList(
                  method.getReturnType(), ((Integer) values[0]).intValue());
            }
            if (method.getDeclaringClass() == Object.class) {
               return invokeObjectMethod(proxy, method, values);
            }
            throw new UnsupportedOperationException(
               "Unbound AttributeSetRegionSetPairListFactory method: " + method);
         });
   }

   private static Object attributeSetRegionSetPairList(Class<?> listType, int capacity) {
      List<Object> entries = new ArrayList<>(Math.max(0, capacity));
      return Proxy.newProxyInstance(
         listType.getClassLoader(),
         new Class<?>[] { listType },
         (proxy, method, arguments) -> {
            Object[] values = arguments == null ? new Object[0] : arguments;
            if ("clone".equals(method.getName()) && values.length == 0) {
               Object copy = attributeSetRegionSetPairList(listType, entries.size());
               Method addAll = listType.getMethod("addAll", java.util.Collection.class);
               addAll.invoke(copy, entries);
               return copy;
            }
            try {
               return method.invoke(entries, values);
            } catch (InvocationTargetException error) {
               throw error.getCause();
            }
         });
   }

   private RTIinternalError unavailable(String service) {
      return new RTIinternalError("Umbra JNI RTI does not yet bind standard service " + service);
   }

   private String[] fomPaths(Object value) throws Exception {
      if (value instanceof String) return new String[] { fomPath(value) };
      if (value instanceof String[]) {
         String[] values = (String[]) value;
         String[] result = new String[values.length];
         for (int index = 0; index < values.length; ++index) result[index] = fomPath(values[index]);
         return result;
      }
      if (value instanceof URL[]) {
         URL[] values = (URL[]) value;
         String[] result = new String[values.length];
         for (int index = 0; index < values.length; ++index) result[index] = fomPath(values[index]);
         return result;
      }
      throw unavailable("a standard String[] FOM module sequence");
   }

   private boolean isFomModuleSequence(Object value) {
      return value instanceof String[] || value instanceof URL[];
   }

   private String fomPath(Object value) throws Exception {
      if (value instanceof String) {
         String path = (String) value;
         if (path.regionMatches(true, 0, "file:", 0, "file:".length())) {
            return Path.of(java.net.URI.create(path)).toString();
         }
         return path;
      }
      if (!(value instanceof URL)) {
         throw unavailable("a FOM module designator");
      }
      URL url = (URL) value;
      if (!"file".equalsIgnoreCase(url.getProtocol())) {
         throw unavailable("createFederationExecution with a non-file FOM URL");
      }
      return Path.of(url.toURI()).toString();
   }

   private byte[][] encodedFederateHandles(FederateHandleSet values) {
      byte[][] result = new byte[values.size()][];
      int index = 0;
      for (FederateHandle value : values) {
         result[index++] = encodedFederateHandle(value);
      }
      return result;
   }

   private byte[] encodedFederateHandle(FederateHandle value) {
      byte[] result = new byte[value.encodedLength()];
      value.encode(result, 0);
      return result;
   }

   private byte[] encodedObjectClassHandle(ObjectClassHandle value) {
      byte[] result = new byte[value.encodedLength()];
      value.encode(result, 0);
      return result;
   }

   private byte[] encodedAttributeHandle(AttributeHandle value) {
      byte[] result = new byte[value.encodedLength()];
      value.encode(result, 0);
      return result;
   }

   private NativeAttributeHandleSet attributeHandleSet(byte[][] encodedHandles) {
      if (encodedHandles == null) {
         throw new IllegalStateException("The native RTI returned a null attribute-handle set");
      }
      NativeAttributeHandleSet result = new NativeAttributeHandleSet();
      for (byte[] encoded : encodedHandles) result.add(new NativeAttributeHandle(encoded));
      return result;
   }

   private byte[] encodedDimensionHandle(DimensionHandle value) {
      byte[] result = new byte[value.encodedLength()];
      value.encode(result, 0);
      return result;
   }

   private byte[] encodedRegionHandle(RegionHandle value) {
      byte[] result = new byte[value.encodedLength()];
      value.encode(result, 0);
      return result;
   }

   private NativeDimensionHandleSet dimensionHandleSet(byte[][] encodedHandles) {
      if (encodedHandles == null) {
         throw new IllegalStateException("The native RTI returned a null dimension-handle set");
      }
      NativeDimensionHandleSet result = new NativeDimensionHandleSet();
      for (byte[] encoded : encodedHandles) result.add(new NativeDimensionHandle(encoded));
      return result;
   }

   private byte[] encodedObjectInstanceHandle(ObjectInstanceHandle value) {
      byte[] result = new byte[value.encodedLength()];
      value.encode(result, 0);
      return result;
   }

   private byte[] encodedInteractionClassHandle(InteractionClassHandle value) {
      byte[] result = new byte[value.encodedLength()];
      value.encode(result, 0);
      return result;
   }

   private byte[] encodedParameterHandle(ParameterHandle value) {
      byte[] result = new byte[value.encodedLength()];
      value.encode(result, 0);
      return result;
   }

   private byte[] encodedTransportationTypeHandle(TransportationTypeHandle value) {
      byte[] result = new byte[value.encodedLength()];
      value.encode(result, 0);
      return result;
   }

   private byte[] encodedLogicalTime(LogicalTime<?, ?> value) throws RTIinternalError {
      // Keep typed IEEE validation exceptions from the C++ RTI visible.  Do
      // not include this call in the encoding catch below, which is reserved
      // for malformed Java value-object encodings and would otherwise erase
      // InvalidLogicalTime as RTIinternalError.
      NativeBridge.nativeValidateLogicalTimeImplementation(
         requireNativeHandle(), NativeLogicalTimeFactories.implementationName(value), false);
      try {
         byte[] result = new byte[value.encodedLength()];
         value.encode(result, 0);
         return result;
      } catch (Exception error) {
         throw internalError("Could not encode logical time for the C++ RTI", error);
      }
   }

   private byte[] encodedLogicalTimeInterval(LogicalTimeInterval<?> value) throws RTIinternalError {
      // See encodedLogicalTime: the C++ InvalidLookahead must cross JNI
      // unchanged rather than being wrapped as a generic encoding failure.
      NativeBridge.nativeValidateLogicalTimeImplementation(
         requireNativeHandle(), NativeLogicalTimeFactories.implementationName(value), true);
      try {
         byte[] result = new byte[value.encodedLength()];
         value.encode(result, 0);
         return result;
      } catch (Exception error) {
         throw internalError("Could not encode logical-time interval for the C++ RTI", error);
      }
   }

   private byte[] encodedMessageRetractionHandle(MessageRetractionHandle value) {
      byte[] result = new byte[value.encodedLength()];
      value.encode(result, 0);
      return result;
   }

   /**
    * Preserve both the fixture's historical raw-handle methods and IEEE
    * 1516.1-2025's overloads, whose return is MessageRetractionReturn.
    */
   private Object messageRetractionResult(Method method, byte[] encodedHandle)
      throws RTIinternalError {
      NativeMessageRetractionHandle handle = new NativeMessageRetractionHandle(encodedHandle);
      Class<?> returnType = method.getReturnType();
      if (!"hla.rti1516_2025.MessageRetractionReturn".equals(returnType.getName())) {
         return handle;
      }
      try {
         return returnType
            .getConstructor(boolean.class, MessageRetractionHandle.class)
            .newInstance(true, handle);
      } catch (ReflectiveOperationException error) {
         throw internalError("Could not create the IEEE message-retraction return value", error);
      }
   }

   private LogicalTimeFactory<?, ?> selectedLogicalTimeFactory() throws RTIinternalError {
      String factoryName = NativeBridge.nativeGetTimeFactoryName(requireNativeHandle());
      try {
         return NativeLogicalTimeFactories.forName(factoryName, requireNativeHandle());
      } catch (IllegalArgumentException error) {
         throw unavailable("logical-time factory for C++ implementation " + factoryName);
      }
   }

   private hla.rti1516_2025.TimeQueryReturn timeQueryReturn(byte[] encodedResult)
      throws RTIinternalError {
      if (encodedResult == null || encodedResult.length < 2) {
         throw new IllegalStateException("The native time-query result is malformed");
      }
      try {
         LogicalTime<?, ?> time = selectedLogicalTimeFactory().decodeTime(
            Arrays.copyOfRange(encodedResult, 1, encodedResult.length), 0);
         return new hla.rti1516_2025.TimeQueryReturn(encodedResult[0] != 0, time);
      } catch (Exception error) {
         throw internalError("C++ RTI returned an invalid logical-time encoding", error);
      }
   }

   private static RTIinternalError internalError(String message, Exception cause) {
      RTIinternalError error = new RTIinternalError(message);
      error.initCause(cause);
      return error;
   }

   /** Release the native ambassador deterministically after disconnecting. */
   @Override
   public void close() {
      if (nativeHandle != 0L) {
         NativeBridge.nativeDestroy(nativeHandle);
         nativeHandle = 0L;
      }
   }
}
