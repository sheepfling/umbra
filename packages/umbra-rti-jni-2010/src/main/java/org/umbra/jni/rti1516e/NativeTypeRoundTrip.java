package org.umbra.jni.rti1516e;

import hla.rti1516e.encoding.ByteWrapper;
import hla.rti1516e.encoding.DataElement;
import hla.rti1516e.encoding.DataElementFactory;
import hla.rti1516e.encoding.DecoderException;
import hla.rti1516e.encoding.EncoderFactory;
import hla.rti1516e.exceptions.RTIinternalError;
import hla.rti1516e.AttributeHandleSet;
import hla.rti1516e.AttributeRegionAssociation;
import hla.rti1516e.RegionHandleSet;
import java.lang.reflect.Array;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Public, provider-neutral type transport probe for the IEEE 1516e-2010 JNI
 * route.
 *
 * <p>The probe intentionally does not expose RTI service state.  It accepts
 * the standard Java representation of a carrier (or its encoded byte form),
 * sends that carrier through JNI into the C++ 1516.1 implementation, and
 * returns the standard Java representation again.  This makes it useful from
 * JPype without requiring a Python-specific or vendor-specific type model.</p>
 */
public final class NativeTypeRoundTrip {
   private static final String[] DATA_ELEMENT_KINDS = {
      "HLAASCIIchar", "HLAASCIIstring", "HLAboolean", "HLAbyte",
      "HLAfloat32BE", "HLAfloat32LE", "HLAfloat64BE", "HLAfloat64LE",
      "HLAinteger16BE", "HLAinteger16LE", "HLAinteger32BE", "HLAinteger32LE",
      "HLAinteger64BE", "HLAinteger64LE", "HLAoctet", "HLAoctetPairBE",
      "HLAoctetPairLE", "HLAunicodeChar", "HLAunicodeString", "HLAopaqueData",
      "HLAvariableArray", "HLAfixedArray", "HLAfixedRecord", "HLAvariantRecord"
   };

   private static final String[] HANDLE_KINDS = {
      "FederateHandle", "ObjectClassHandle", "InteractionClassHandle",
      "ObjectInstanceHandle", "AttributeHandle", "ParameterHandle",
      "DimensionHandle", "MessageRetractionHandle", "RegionHandle"
   };

   /* 1516e exposes this handle, while the 1516.1 C++ handle set does not. */
   private static final String[] JAVA_ONLY_HANDLE_KINDS = {
      "TransportationTypeHandle"
   };

   private static final String[] LOGICAL_TIME_KINDS = {
      "HLAinteger64Time", "HLAinteger64Interval", "HLAfloat64Time",
      "HLAfloat64Interval"
   };

   private static final String[] LOGICAL_TIME_FACTORY_KINDS = {
      "HLAinteger64TimeFactory", "HLAfloat64TimeFactory"
   };

   private static final String[] COLLECTION_KINDS = {
      "AttributeHandleSet", "DimensionHandleSet", "FederateHandleSet",
      "RegionHandleSet", "FederationExecutionInformationSet",
      "AttributeHandleValueMap", "ParameterHandleValueMap",
      "AttributeSetRegionSetPairList"
   };

   private static final String[] COLLECTION_FACTORY_KINDS = {
      "AttributeHandleSetFactory", "DimensionHandleSetFactory",
      "FederateHandleSetFactory", "RegionHandleSetFactory",
      "AttributeHandleValueMapFactory", "ParameterHandleValueMapFactory",
      "AttributeSetRegionSetPairListFactory"
   };

   private static final String[] HANDLE_FACTORY_KINDS = {
      "FederateHandleFactory", "ObjectClassHandleFactory",
      "InteractionClassHandleFactory", "ObjectInstanceHandleFactory",
      "AttributeHandleFactory", "ParameterHandleFactory",
      "DimensionHandleFactory", "TransportationTypeHandleFactory"
   };

   private static final String[] DATA_ELEMENT_FACTORY_KINDS = {
      "HLAoctet", "HLAinteger32BE", "HLAASCIIstring", "HLAopaqueData",
      "HLAvariableArray", "HLAfixedArray", "HLAfixedRecord", "HLAvariantRecord"
   };

   private static final String[] ENUM_TYPES = {
      "CallbackModel", "OrderType", "ResignAction", "ServiceGroup",
      "SynchronizationPointFailureReason", "SaveFailureReason", "SaveStatus",
      "RestoreFailureReason", "RestoreStatus"
   };

   private static final String[] STANDARD_EXCEPTION_NAMES = {
      "AlreadyConnected", "AsynchronousDeliveryAlreadyDisabled",
      "AsynchronousDeliveryAlreadyEnabled", "AttributeAcquisitionWasNotRequested",
      "AttributeAlreadyBeingAcquired", "AttributeAlreadyBeingChanged",
      "AttributeAlreadyBeingDivested", "AttributeAlreadyOwned",
      "AttributeDivestitureWasNotRequested", "AttributeNotDefined",
      "AttributeNotOwned", "AttributeNotPublished", "AttributeNotRecognized",
      "AttributeNotSubscribed", "AttributeRelevanceAdvisorySwitchIsOff",
      "AttributeRelevanceAdvisorySwitchIsOn", "AttributeScopeAdvisorySwitchIsOff",
      "AttributeScopeAdvisorySwitchIsOn", "CallNotAllowedFromWithinCallback",
      "ConnectionFailed", "CouldNotCreateLogicalTimeFactory", "CouldNotDecode",
      "CouldNotEncode", "CouldNotOpenFDD", "CouldNotOpenMIM",
      "DeletePrivilegeNotHeld", "DesignatorIsHLAstandardMIM", "ErrorReadingFDD",
      "ErrorReadingMIM", "FederateAlreadyExecutionMember", "FederateHandleNotKnown",
      "FederateHasNotBegunSave", "FederateInternalError", "FederateIsExecutionMember",
      "FederateNameAlreadyInUse", "FederateNotExecutionMember", "FederateOwnsAttributes",
      "FederateServiceInvocationsAreBeingReportedViaMOM", "FederateUnableToUseTime",
      "FederatesCurrentlyJoined", "FederationExecutionAlreadyExists",
      "FederationExecutionDoesNotExist", "IllegalName", "IllegalTimeArithmetic",
      "InTimeAdvancingState", "InconsistentFDD", "InteractionClassAlreadyBeingChanged",
      "InteractionClassNotDefined", "InteractionClassNotPublished",
      "InteractionParameterNotDefined", "InteractionRelevanceAdvisorySwitchIsOff",
      "InteractionRelevanceAdvisorySwitchIsOn", "InvalidAttributeHandle",
      "InvalidDimensionHandle", "InvalidFederateHandle", "InvalidInteractionClassHandle",
      "InvalidLocalSettingsDesignator", "InvalidLogicalTime", "InvalidLogicalTimeInterval",
      "InvalidLookahead", "InvalidMessageRetractionHandle", "InvalidObjectClassHandle",
      "InvalidOrderName", "InvalidOrderType", "InvalidParameterHandle", "InvalidRangeBound",
      "InvalidRegion", "InvalidRegionContext", "InvalidResignAction", "InvalidServiceGroup",
      "InvalidTransportationName", "InvalidTransportationType", "InvalidUpdateRateDesignator",
      "LogicalTimeAlreadyPassed", "MessageCanNoLongerBeRetracted", "NameNotFound",
      "NameSetWasEmpty", "NoAcquisitionPending",
      "NoRequestToEnableTimeConstrainedWasPending", "NoRequestToEnableTimeRegulationWasPending",
      "NotConnected", "ObjectClassNotDefined", "ObjectClassNotPublished",
      "ObjectClassRelevanceAdvisorySwitchIsOff", "ObjectClassRelevanceAdvisorySwitchIsOn",
      "ObjectInstanceNameInUse", "ObjectInstanceNameNotReserved", "ObjectInstanceNotKnown",
      "OwnershipAcquisitionPending", "RTIexception", "RTIinternalError",
      "RegionDoesNotContainSpecifiedDimension", "RegionInUseForUpdateOrSubscription",
      "RegionNotCreatedByThisFederate",
      "RequestForTimeConstrainedPending", "RequestForTimeRegulationPending",
      "RestoreInProgress", "RestoreNotInProgress", "RestoreNotRequested", "SaveInProgress",
      "SaveNotInProgress", "SaveNotInitiated", "SynchronizationPointLabelNotAnnounced",
      "TimeConstrainedAlreadyEnabled", "TimeConstrainedIsNotEnabled",
      "TimeRegulationAlreadyEnabled", "TimeRegulationIsNotEnabled", "UnableToPerformSave",
      "UnknownName", "UnsupportedCallbackModel"
   };

   private NativeTypeRoundTrip() {
   }

   public static String[] dataElementKinds() {
      return DATA_ELEMENT_KINDS.clone();
   }

   public static String[] handleKinds() {
      return HANDLE_KINDS.clone();
   }

   public static String[] javaOnlyHandleKinds() {
      return JAVA_ONLY_HANDLE_KINDS.clone();
   }

   public static String[] logicalTimeKinds() {
      return LOGICAL_TIME_KINDS.clone();
   }

   public static String[] logicalTimeFactoryKinds() {
      return LOGICAL_TIME_FACTORY_KINDS.clone();
   }

   public static String[] enumTypes() {
      return ENUM_TYPES.clone();
   }

   public static String[] standardExceptionNames() {
      return STANDARD_EXCEPTION_NAMES.clone();
   }

   public static String[] collectionKinds() {
      return COLLECTION_KINDS.clone();
   }

   public static String[] collectionFactoryKinds() {
      return COLLECTION_FACTORY_KINDS.clone();
   }

   public static String[] handleFactoryKinds() {
      return HANDLE_FACTORY_KINDS.clone();
   }

   public static String[] dataElementFactoryKinds() {
      return DATA_ELEMENT_FACTORY_KINDS.clone();
   }

   public static byte[] roundTripBytes(byte[] value) throws RTIinternalError {
      return NativeBridge.nativeRoundTripBytes(value);
   }

   public static void throwStandardException(String name, String message)
      throws RTIinternalError {
      NativeBridge.nativeThrowStandardException(name, message);
   }

   public static String roundTripString(String value) throws RTIinternalError {
      return NativeBridge.nativeRoundTripString(value);
   }

   public static byte roundTripByte(byte value) {
      return NativeBridge.nativeRoundTripByte(value);
   }

   public static short roundTripShort(short value) {
      return NativeBridge.nativeRoundTripShort(value);
   }

   public static int roundTripInt(int value) {
      return NativeBridge.nativeRoundTripInt(value);
   }

   public static long roundTripLong(long value) {
      return NativeBridge.nativeRoundTripLong(value);
   }

   public static float roundTripFloat(float value) {
      return NativeBridge.nativeRoundTripFloat(value);
   }

   public static double roundTripDouble(double value) {
      return NativeBridge.nativeRoundTripDouble(value);
   }

   public static boolean roundTripBoolean(boolean value) {
      return NativeBridge.nativeRoundTripBoolean(value);
   }

   /** Identity round-trip for Java-only carriers with no C++ standard value. */
   public static Object roundTripObject(Object value) {
      return NativeBridge.nativeRoundTripObject(value);
   }

   /** Return an unknown provider-owned DataElement for raw-carrier tests. */
   public static DataElement vendorDataElementCarrier() {
      return new NativeVendorDataElement(new byte[] { 'a', 'b', 'c', 'd' });
   }

   public static <T extends Enum<T>> T roundTripEnum(T value) {
      @SuppressWarnings("unchecked")
      T returned = (T) roundTripObject(value);
      return returned;
   }

   public static byte[] seedDataElement(String kind) throws RTIinternalError {
      return NativeBridge.nativeSeedDataElement(kind);
   }

   public static byte[] roundTripDataElement(String kind, byte[] value)
      throws RTIinternalError, DecoderException {
      return NativeBridge.nativeRoundTripDataElement(kind, value);
   }

   public static byte[] seedHandle(String kind, long value) throws RTIinternalError {
      return NativeBridge.nativeSeedHandle(kind, value);
   }

   public static byte[] roundTripHandle(String kind, byte[] value) throws RTIinternalError {
      return NativeBridge.nativeRoundTripHandle(kind, value);
   }

   /** Return a standard 1516e handle interface backed by the native carrier. */
   public static NativeHandleCarrier handleCarrier(String kind, long value)
      throws RTIinternalError {
      return new NativeHandleCarrier(kind, isJavaOnlyHandle(kind)
         ? javaOnlyHandleSeed(value) : seedHandle(kind, value));
   }

   /** Validate a standard-interface handle carrier through the C++ decoder. */
   public static NativeHandleCarrier roundTripHandleCarrier(
      String kind, NativeHandleCarrier value) throws RTIinternalError {
      byte[] encoded = isJavaOnlyHandle(kind)
         ? roundTripBytes(value.encodedValue())
         : roundTripHandle(kind, value.encodedValue());
      return new NativeHandleCarrier(kind, encoded);
   }

   private static boolean isJavaOnlyHandle(String kind) {
      return "TransportationTypeHandle".equals(kind);
   }

   private static byte[] javaOnlyHandleSeed(long value) {
      return ByteBuffer.allocate(Long.BYTES).putLong(value).array();
   }

   public static byte[][] roundTripHandleCollection(String kind, byte[][] values) throws RTIinternalError {
      return NativeBridge.nativeRoundTripHandleCollection(kind, values);
   }

   public static byte[][] roundTripByteMatrix(byte[][] values) throws RTIinternalError {
      return NativeBridge.nativeRoundTripByteMatrix(values);
   }

   public static byte[] seedLogicalTime(String kind) throws RTIinternalError {
      return NativeBridge.nativeSeedLogicalTime(kind);
   }

   public static byte[] roundTripLogicalTime(String kind, byte[] value) throws RTIinternalError {
      return NativeBridge.nativeRoundTripLogicalTime(kind, value);
   }

   private static byte[] logicalTimeEncoding(String kind, Number value)
      throws RTIinternalError {
      ByteBuffer result = ByteBuffer.allocate(Long.BYTES);
      if (kind.startsWith("HLAinteger64")) {
         return result.putLong(value.longValue()).array();
      }
      if (kind.startsWith("HLAfloat64")) {
         return result.putLong(Double.doubleToRawLongBits(value.doubleValue())).array();
      }
      throw new RTIinternalError("Unknown IEEE 1516.1-2010 logical-time kind: " + kind);
   }

   private static byte[] logicalTimeSlice(byte[] source, int offset)
      throws RTIinternalError {
      if (source == null || offset < 0 || offset > source.length - Long.BYTES) {
         throw new RTIinternalError(
            "An IEEE 1516.1-2010 logical-time encoding must contain eight octets.");
      }
      return Arrays.copyOfRange(source, offset, offset + Long.BYTES);
   }

   private static Object logicalTimeCarrier(String kind, Number value)
      throws RTIinternalError {
      byte[] encoded = roundTripLogicalTime(kind, logicalTimeEncoding(kind, value));
      return logicalTimeCarrier(kind, encoded);
   }

   /** Return a standard DataElement interface backed by a native-validated byte carrier. */
   public static DataElement dataElementCarrier(String kind) throws RTIinternalError {
      try {
         Class<?> contract = Class.forName("hla.rti1516e.encoding." + kind);
         return (DataElement) Proxy.newProxyInstance(
            contract.getClassLoader(), new Class<?>[] { contract },
            new NativeDataElementHandler(kind, seedDataElement(kind)));
      } catch (ClassNotFoundException | ClassCastException error) {
         throw new RTIinternalError("Cannot create standard data-element carrier for " + kind, error);
      }
   }

   /** Return the standard EncoderFactory interface without changing RTI service state. */
   public static EncoderFactory encoderFactoryCarrier() {
      return (EncoderFactory) Proxy.newProxyInstance(
         EncoderFactory.class.getClassLoader(), new Class<?>[] { EncoderFactory.class },
         (proxy, method, arguments) -> {
            String name = method.getName();
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) return "IEEE 1516e-2010 JNI encoder factory carrier";
               if ("hashCode".equals(name)) return System.identityHashCode(proxy);
               if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
            }
            if (name.startsWith("createHLA")) {
               String kind = name.substring("create".length());
               DataElement element = dataElementCarrier(kind);
               NativeDataElementHandler handler =
                  (NativeDataElementHandler) Proxy.getInvocationHandler(element);
               if (arguments != null && arguments.length == 1 &&
                   (isScalar(kind) || "HLAopaqueData".equals(kind))) {
                  handler.setInitialValue(arguments[0]);
               } else if ("HLAfixedArray".equals(kind) && arguments != null) {
                  if (arguments.length == 2 && arguments[0] instanceof DataElementFactory &&
                      arguments[1] instanceof Number) {
                     handler.resizeFromFactory(
                        (DataElementFactory<?>) arguments[0],
                        ((Number) arguments[1]).intValue());
                  } else {
                     handler.replaceChildren(arguments, 0);
                  }
               } else if ("HLAvariableArray".equals(kind) && arguments != null) {
                  // The first argument is the element factory; the remaining
                  // arguments are the Java varargs payload.  An empty varargs
                  // call is a valid zero-length array and must not retain the
                  // probe's seed children.
                  handler.replaceChildren(arguments, 1);
               } else if ("HLAvariantRecord".equals(kind) &&
                          arguments != null && arguments.length == 1 &&
                          arguments[0] instanceof DataElement) {
                  handler.setDiscriminantElement((DataElement) arguments[0]);
               } else if ("HLAfixedRecord".equals(kind) &&
                          (arguments == null || arguments.length == 0)) {
                  // A newly-created fixed record starts empty.  Seed children
                  // are reserved for the standalone carrier probe, where the
                  // C++ encoder supplies a known record payload.
                  handler.replaceChildren(arguments, 0);
               }
               return element;
            }
            return defaultValue(method.getReturnType());
         });
   }

   /** Return a generic standard DataElementFactory for the selected element shape. */
   public static DataElementFactory<?> dataElementFactoryCarrier(String elementKind) {
      return (DataElementFactory<?>) Proxy.newProxyInstance(
         DataElementFactory.class.getClassLoader(), new Class<?>[] { DataElementFactory.class },
         (proxy, method, arguments) -> {
            String name = method.getName();
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) return elementKind + "DataElementFactory[native-carrier]";
               if ("hashCode".equals(name)) return System.identityHashCode(proxy);
               if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
            }
            if ("createElement".equals(name)) return dataElementCarrier(elementKind);
            return defaultValue(method.getReturnType());
         });
   }

   /** Return a standard logical-time interface backed by a native-validated byte carrier. */
   public static Object logicalTimeCarrier(String kind) throws RTIinternalError {
      return logicalTimeCarrier(kind, seedLogicalTime(kind));
   }

   private static Object logicalTimeCarrier(String kind, byte[] encoded) throws RTIinternalError {
      try {
         Class<?> contract = Class.forName("hla.rti1516e.time." + kind);
         return Proxy.newProxyInstance(
            contract.getClassLoader(), new Class<?>[] { contract },
            new NativeLogicalTimeHandler(kind, encoded));
      } catch (ClassNotFoundException error) {
         throw new RTIinternalError("Cannot create standard logical-time carrier for " + kind, error);
      }
   }

   /** Return a standard LogicalTimeFactory backed by the native time codecs. */
   public static Object logicalTimeFactoryCarrier(String factoryKind) throws RTIinternalError {
      try {
         Class<?> contract = Class.forName("hla.rti1516e.time." + factoryKind);
         String timeKind = factoryKind.substring(0, factoryKind.length() - "Factory".length());
         String intervalKind = timeKind.replace("Time", "Interval");
         return Proxy.newProxyInstance(
            contract.getClassLoader(), new Class<?>[] { contract },
            (proxy, method, arguments) -> {
               String name = method.getName();
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return factoryKind + "[native-carrier]";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               if ("getName".equals(name)) return timeKind;
               if ("decodeTime".equals(name)) {
                  return decodeLogicalTimeCarrier(timeKind, arguments);
               }
               if ("decodeInterval".equals(name)) {
                  return decodeLogicalTimeCarrier(intervalKind, arguments);
               }
               if ("makeInitial".equals(name)) {
                  return logicalTimeCarrier(timeKind,
                     timeKind.startsWith("HLAinteger64") ? 0L : 0.0D);
               }
               if ("makeFinal".equals(name)) {
                  return logicalTimeCarrier(timeKind,
                     timeKind.startsWith("HLAinteger64") ? Long.MAX_VALUE : Double.MAX_VALUE);
               }
               if ("makeTime".equals(name)) {
                  if (arguments == null || arguments.length != 1 ||
                      !(arguments[0] instanceof Number)) {
                     throw new RTIinternalError("makeTime requires one numeric value");
                  }
                  return logicalTimeCarrier(timeKind, (Number) arguments[0]);
               }
               if ("makeZero".equals(name)) {
                  return logicalTimeCarrier(intervalKind,
                     intervalKind.startsWith("HLAinteger64") ? 0L : 0.0D);
               }
               if ("makeEpsilon".equals(name)) {
                  return logicalTimeCarrier(intervalKind,
                     intervalKind.startsWith("HLAinteger64")
                        ? 1L : Double.longBitsToDouble(1L));
               }
               if ("makeInterval".equals(name)) {
                  if (arguments == null || arguments.length != 1 ||
                      !(arguments[0] instanceof Number)) {
                     throw new RTIinternalError("makeInterval requires one numeric value");
                  }
                  return logicalTimeCarrier(intervalKind, (Number) arguments[0]);
               }
               return defaultValue(method.getReturnType());
            });
      } catch (ClassNotFoundException error) {
         throw new RTIinternalError("Cannot create standard logical-time factory for " + factoryKind, error);
      }
   }

   /** Return a standard set/map/list carrier backed by a Java collection. */
   public static Object collectionCarrier(String kind) throws RTIinternalError {
      try {
         Class<?> contract = Class.forName("hla.rti1516e." + kind);
         return Proxy.newProxyInstance(
            contract.getClassLoader(), new Class<?>[] { contract },
            new NativeCollectionHandler(kind, collectionBacking(kind)));
      } catch (ClassNotFoundException error) {
         throw new RTIinternalError("Cannot create standard collection carrier for " + kind, error);
      }
   }

   /** Return a standard collection factory whose create method returns a carrier. */
   public static Object collectionFactoryCarrier(String factoryKind) throws RTIinternalError {
      try {
         Class<?> contract = Class.forName("hla.rti1516e." + factoryKind);
         String collectionKind = factoryKind.substring(0, factoryKind.length() - "Factory".length());
         return Proxy.newProxyInstance(
            contract.getClassLoader(), new Class<?>[] { contract },
            (proxy, method, arguments) -> {
               String name = method.getName();
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return factoryKind + "[native-carrier]";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               if ("create".equals(name)) return collectionCarrier(collectionKind);
               return defaultValue(method.getReturnType());
            });
      } catch (ClassNotFoundException error) {
         throw new RTIinternalError("Cannot create standard collection factory for " + factoryKind, error);
      }
   }

   /** Return a standard handle factory whose decode/default methods use JNI codecs. */
   public static Object handleFactoryCarrier(String factoryKind) throws RTIinternalError {
      try {
         Class<?> contract = Class.forName("hla.rti1516e." + factoryKind);
         String handleKind = factoryKind.substring(0, factoryKind.length() - "Factory".length());
         return Proxy.newProxyInstance(
            contract.getClassLoader(), new Class<?>[] { contract },
            (proxy, method, arguments) -> {
               String name = method.getName();
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return factoryKind + "[native-carrier]";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               if ("decode".equals(name)) {
                  byte[] source = arguments != null && arguments.length > 0
                     ? (byte[]) arguments[0] : new byte[0];
                  int offset = arguments != null && arguments.length > 1
                     ? ((Number) arguments[1]).intValue() : 0;
                  byte[] encoded = Arrays.copyOfRange(source, offset, source.length);
                  byte[] returned = isJavaOnlyHandle(handleKind)
                     ? roundTripBytes(encoded) : roundTripHandle(handleKind, encoded);
                  return new NativeHandleCarrier(handleKind, returned);
               }
               if ("getHLAdefaultReliable".equals(name)) {
                  return handleCarrier(handleKind, 1L);
               }
               if ("getHLAdefaultBestEffort".equals(name)) {
                  return handleCarrier(handleKind, 2L);
               }
               return defaultValue(method.getReturnType());
            });
      } catch (ClassNotFoundException error) {
         throw new RTIinternalError("Cannot create standard handle factory for " + factoryKind, error);
      }
   }

   /** Send collection payloads through JNI and reconstruct the standard carrier. */
   public static Object roundTripCollectionCarrier(String kind, Object value)
      throws RTIinternalError {
      Object returned = collectionCarrier(kind);
      if (value instanceof Set && returned instanceof Set) {
         Set<Object> target = castSet(returned);
         for (Object item : (Set<?>) value) {
            if (item instanceof NativeHandleCarrier) {
               NativeHandleCarrier handle = (NativeHandleCarrier) item;
               target.add(roundTripHandleCarrier(handle.kind(), handle));
            } else {
               target.add(roundTripObject(item));
            }
         }
         return returned;
      }
      if (value instanceof Map && returned instanceof Map) {
         Map<Object, Object> target = castMap(returned);
         List<byte[]> payloads = new ArrayList<>();
         List<Object> keys = new ArrayList<>();
         for (Map.Entry<?, ?> entry : ((Map<?, ?>) value).entrySet()) {
            keys.add(entry.getKey());
            payloads.add(entry.getValue() instanceof byte[]
               ? ((byte[]) entry.getValue()).clone() : new byte[0]);
         }
         byte[][] matrix = roundTripByteMatrix(payloads.toArray(new byte[0][]));
         for (int index = 0; index < keys.size(); ++index) {
            Object key = keys.get(index);
            if (key instanceof NativeHandleCarrier) {
               NativeHandleCarrier handle = (NativeHandleCarrier) key;
               key = roundTripHandleCarrier(handle.kind(), handle);
            }
            target.put(key, matrix[index]);
         }
         return returned;
      }
      if (value instanceof List && returned instanceof List) {
         List<Object> target = castList(returned);
         for (Object item : (List<?>) value) {
            if (item instanceof AttributeRegionAssociation) {
               AttributeRegionAssociation association = (AttributeRegionAssociation) item;
               AttributeHandleSet attributes = (AttributeHandleSet) roundTripCollectionCarrier(
                  "AttributeHandleSet", association.ahset);
               RegionHandleSet regions = (RegionHandleSet) roundTripCollectionCarrier(
                  "RegionHandleSet", association.rhset);
               target.add(new AttributeRegionAssociation(attributes, regions));
            } else {
               target.add(roundTripObject(item));
            }
         }
         return returned;
      }
      return roundTripObject(value);
   }

   private static Object decodeLogicalTimeCarrier(String kind, Object[] arguments)
      throws RTIinternalError {
      if (arguments == null || arguments.length == 0 || !(arguments[0] instanceof byte[])) {
         return logicalTimeCarrier(kind);
      }
      byte[] source = (byte[]) arguments[0];
      int offset = arguments.length > 1 ? ((Number) arguments[1]).intValue() : 0;
      byte[] encoded = logicalTimeSlice(source, offset);
      return logicalTimeCarrier(kind, roundTripLogicalTime(kind, encoded));
   }

   /** Build one of the standard nested callback-record interfaces. */
   public static Object callbackCarrier(String kind) throws RTIinternalError {
      try {
         Class<?> contract = Class.forName("hla.rti1516e.FederateAmbassador$" + kind);
         return Proxy.newProxyInstance(
            contract.getClassLoader(), new Class<?>[] { contract },
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("hasProducingFederate".equals(name)) return true;
               if ("hasSentRegions".equals(name)) return false;
               if ("getProducingFederate".equals(name)) {
                  return handleCarrier("FederateHandle", 7L);
               }
               if ("getSentRegions".equals(name)) return null;
               if (method.getDeclaringClass() == Object.class && "toString".equals(name)) {
                  return kind + "[native-carrier]";
               }
               return defaultValue(method.getReturnType());
            });
      } catch (ClassNotFoundException error) {
         throw new RTIinternalError("Cannot create standard callback carrier for " + kind, error);
      }
   }

   /** Exercise the standard Java ByteWrapper cursor around a JNI byte carrier. */
   public static byte[] roundTripByteWrapper(byte[] value) throws RTIinternalError {
      ByteWrapper wrapper = new ByteWrapper(value.clone());
      byte[] returned = roundTripBytes(wrapper.array());
      wrapper.reset();
      wrapper.put(returned);
      return Arrays.copyOf(wrapper.array(), returned.length);
   }

   private static Object collectionBacking(String kind) {
      if (kind.endsWith("Map")) return new LinkedHashMap<Object, Object>();
      if (kind.endsWith("Set")) return new LinkedHashSet<Object>();
      return new ArrayList<Object>();
   }

   @SuppressWarnings("unchecked")
   private static Set<Object> castSet(Object value) {
      return (Set<Object>) value;
   }

   @SuppressWarnings("unchecked")
   private static Map<Object, Object> castMap(Object value) {
      return (Map<Object, Object>) value;
   }

   @SuppressWarnings("unchecked")
   private static List<Object> castList(Object value) {
      return (List<Object>) value;
   }

   private static final class NativeCollectionHandler implements InvocationHandler {
      private final String kind;
      private final Object backing;

      private NativeCollectionHandler(String kind, Object backing) {
         this.kind = kind;
         this.backing = backing;
      }

      @Override
      public Object invoke(Object proxy, Method method, Object[] arguments) throws Throwable {
         String name = method.getName();
         if ("toString".equals(name) && method.getParameterTypes().length == 0) {
            return kind + backing.toString();
         }
         if ("hashCode".equals(name) && method.getParameterTypes().length == 0) {
            return backing.hashCode();
         }
         if ("equals".equals(name) && method.getParameterTypes().length == 1) {
            Object other = arguments == null ? null : arguments[0];
            if (other != null && Proxy.isProxyClass(other.getClass())) {
               InvocationHandler handler = Proxy.getInvocationHandler(other);
               if (handler instanceof NativeCollectionHandler) {
                  other = ((NativeCollectionHandler) handler).backing;
               }
            }
            return backing.equals(other);
         }
         if ("clone".equals(name) && method.getParameterTypes().length == 0) {
            return proxy;
         }
         if (name.startsWith("getValueReference") && backing instanceof Map) {
            Map<?, ?> map = (Map<?, ?>) backing;
            byte[] value = map.get(arguments == null ? null : arguments[0]) instanceof byte[]
               ? ((byte[]) map.get(arguments[0])).clone() : new byte[0];
            if (arguments != null && arguments.length == 2 && arguments[1] instanceof ByteWrapper) {
               ByteWrapper target = (ByteWrapper) arguments[1];
               target.reset();
               target.put(value);
               return target;
            }
            return new ByteWrapper(value);
         }
         if (backing instanceof Map) {
            return method.invoke(backing, arguments);
         }
         if (backing instanceof Collection) {
            return method.invoke(backing, arguments);
         }
         return defaultValue(method.getReturnType());
      }
   }

   private static final class NativeDataElementHandler implements InvocationHandler {
      private final String kind;
      private byte[] encoded;
      private final List<DataElement> children;

      private NativeDataElementHandler(String kind, byte[] encoded) {
         this.kind = kind;
         this.encoded = encoded.clone();
         this.children = initialChildren(kind);
      }

      private void setInitialValue(Object value) throws RTIinternalError, DecoderException {
         if ("HLAopaqueData".equals(kind)) {
            encoded = roundTripDataElement(kind, opaqueEncoding((byte[]) value));
         } else if (isScalar(kind)) {
            encoded = roundTripDataElement(kind, scalarEncoding(kind, value));
         }
      }

      private void replaceChildren(Object[] arguments, int start) {
         children.clear();
         if (arguments == null) return;
         for (int index = start; index < arguments.length; ++index) {
            Object argument = arguments[index];
            if (argument == null) continue;
            if (argument.getClass().isArray()) {
               int length = Array.getLength(argument);
               for (int child = 0; child < length; ++child) {
                  Object value = Array.get(argument, child);
                  if (value instanceof DataElement) children.add((DataElement) value);
               }
            } else if (argument instanceof DataElement) {
               children.add((DataElement) argument);
            }
         }
      }

      private void resizeFromFactory(DataElementFactory<?> factory, int size)
         throws Exception {
         if (size < 0) throw new IllegalArgumentException("array size must be non-negative");
         children.clear();
         for (int index = 0; index < size; ++index) {
            DataElement element = factory.createElement(index);
            if (element == null) {
               throw new IllegalArgumentException("DataElementFactory returned null");
            }
            children.add(element);
         }
      }

      private void setDiscriminantElement(DataElement discriminant) {
         if (children.isEmpty()) children.add(discriminant);
         else children.set(0, discriminant);
      }

      @Override
      public Object invoke(Object proxy, Method method, Object[] arguments) throws Throwable {
         String name = method.getName();
         if (method.getDeclaringClass() == Object.class) {
            if ("toString".equals(name)) return kind + "[native-carrier]";
            if ("hashCode".equals(name)) return Arrays.hashCode(encoded);
            if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
         }
         if ("toByteArray".equals(name)) return encoded.clone();
         if ("getEncodedLength".equals(name)) return encoded.length;
         if ("getOctetBoundary".equals(name)) return octetBoundary(kind);
         if ("getValue".equals(name)) {
            if ("HLAopaqueData".equals(kind)) return opaquePayload(encoded);
            if (isScalar(kind)) return scalarValue(kind, encoded);
            if ("HLAvariantRecord".equals(kind)) {
               return children.size() > 1 ? children.get(1) : null;
            }
         }
         if ("setValue".equals(name) && arguments != null && arguments.length == 1) {
            if ("HLAopaqueData".equals(kind) || isScalar(kind)) {
               setInitialValue(arguments[0]);
               return null;
            }
         }
         if ("size".equals(name)) {
            if ("HLAopaqueData".equals(kind)) return opaquePayload(encoded).length;
            return children.size();
         }
         if ("get".equals(name) && arguments != null && arguments.length == 1 &&
             arguments[0] instanceof Number) {
            int index = ((Number) arguments[0]).intValue();
            if ("HLAopaqueData".equals(kind)) return opaquePayload(encoded)[index];
            return children.get(index);
         }
         if ("iterator".equals(name)) {
            if ("HLAopaqueData".equals(kind)) {
               List<Byte> values = new ArrayList<>();
               for (byte value : opaquePayload(encoded)) values.add(value);
               return values.iterator();
            }
            return children.iterator();
         }
         if ("addElement".equals(name) || "add".equals(name)) {
            if (arguments != null && arguments.length == 1 && arguments[0] instanceof DataElement) {
               children.add((DataElement) arguments[0]);
            }
            return null;
         }
         if ("resize".equals(name) && arguments != null && arguments.length == 1) {
            int size = ((Number) arguments[0]).intValue();
            while (children.size() < size) children.add(dataElementCarrier("HLAoctet"));
            while (children.size() > size) children.remove(children.size() - 1);
            return null;
         }
         if ("setDiscriminant".equals(name) && arguments != null && arguments.length == 1) {
            if (children.isEmpty()) children.add((DataElement) arguments[0]);
            else children.set(0, (DataElement) arguments[0]);
            return null;
         }
         if ("setVariant".equals(name) && arguments != null && arguments.length == 2) {
            if (children.size() < 2) children.add((DataElement) arguments[0]);
            if (children.size() < 2) children.add((DataElement) arguments[1]);
            else children.set(1, (DataElement) arguments[1]);
            return null;
         }
         if ("getDiscriminant".equals(name)) return children.isEmpty() ? null : children.get(0);
         if ("encode".equals(name) && arguments != null && arguments.length == 1 &&
             arguments[0] instanceof ByteWrapper) {
            ((ByteWrapper) arguments[0]).put(encoded);
            return null;
         }
         if ("decode".equals(name) && arguments != null && arguments.length == 1) {
            if (arguments[0] instanceof byte[]) {
               encoded = roundTripDataElement(kind, (byte[]) arguments[0]);
               return null;
            }
            if (arguments[0] instanceof ByteWrapper) {
               ByteWrapper wrapper = (ByteWrapper) arguments[0];
               encoded = decodeWrapperElement(kind, wrapper);
               return null;
            }
         }
         return defaultValue(method.getReturnType());
      }

      /**
       * Decode one element from the current Java ByteWrapper window.
       *
       * <p>The C++ carrier API receives a complete value and deliberately
       * rejects trailing octets.  Java's ByteWrapper overload instead
       * consumes one value and leaves the next value in the wrapper.  Try
       * prefixes in increasing order so the official C++ decoder remains the
       * sole source of wire-shape truth, while preserving that Java cursor
       * contract for fixed, variable, and composite elements alike.</p>
       */
      private static byte[] decodeWrapperElement(String kind, ByteWrapper wrapper)
         throws RTIinternalError, DecoderException {
         int start = wrapper.getPos();
         int available = wrapper.remaining();
         DecoderException last = null;
         for (int length = 0; length <= available; ++length) {
            byte[] candidate = Arrays.copyOfRange(wrapper.array(), start, start + length);
            try {
               byte[] decoded = roundTripDataElement(kind, candidate);
               wrapper.advance(length);
               return decoded;
            } catch (DecoderException error) {
               last = error;
            }
         }
         if (last != null) throw last;
         throw new DecoderException("The ByteWrapper does not contain a decodable " + kind);
      }

      private static byte[] opaquePayload(byte[] value) {
         if (value.length < 4) return new byte[0];
         int size = ByteBuffer.wrap(value).getInt();
         if (size < 0 || value.length < 4 + size) return new byte[0];
         return Arrays.copyOfRange(value, 4, 4 + size);
      }

      private static byte[] opaqueEncoding(byte[] payload) {
         byte[] value = payload == null ? new byte[0] : payload.clone();
         return ByteBuffer.allocate(4 + value.length).putInt(value.length).put(value).array();
      }

      private static List<DataElement> initialChildren(String kind) {
         List<DataElement> result = new ArrayList<>();
         try {
            if ("HLAvariableArray".equals(kind) || "HLAfixedArray".equals(kind)) {
               int count = "HLAfixedArray".equals(kind) ? 3 : 2;
               for (int index = 0; index < count; ++index) {
                  result.add(dataElementCarrier("HLAoctet"));
               }
            } else if ("HLAfixedRecord".equals(kind)) {
               result.add(dataElementCarrier("HLAinteger32BE"));
               result.add(dataElementCarrier("HLAASCIIstring"));
            } else if ("HLAvariantRecord".equals(kind)) {
               result.add(dataElementCarrier("HLAinteger32BE"));
               result.add(dataElementCarrier("HLAASCIIstring"));
            }
            return result;
         } catch (RTIinternalError error) {
            throw new IllegalStateException(error);
         }
      }
   }

   private static final class NativeLogicalTimeHandler implements InvocationHandler {
      private final String kind;
      private final byte[] encoded;
      private final Object value;

      private NativeLogicalTimeHandler(String kind, byte[] encoded) {
         this.kind = kind;
         this.encoded = encoded.clone();
         this.value = logicalTimeValue(kind, this.encoded);
      }

      @Override
      public Object invoke(Object proxy, Method method, Object[] arguments)
         throws RTIinternalError {
         String name = method.getName();
         if (method.getDeclaringClass() == Object.class) {
            if ("toString".equals(name)) return kind + "[" + value + "]";
            if ("hashCode".equals(name)) return Arrays.hashCode(encoded);
            if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
         }
         if ("encodedLength".equals(name)) return encoded.length;
         if ("encode".equals(name) && arguments != null && arguments.length == 2) {
            byte[] destination = (byte[]) arguments[0];
            int offset = ((Number) arguments[1]).intValue();
            System.arraycopy(encoded, 0, destination, offset, encoded.length);
            return null;
         }
         if ("getValue".equals(name) || "getTime".equals(name) ||
             "getTimeValue".equals(name) || "getInterval".equals(name) ||
             "getIntervalValue".equals(name)) return value;
         if ("isInitial".equals(name)) {
            return kind.endsWith("Time") && isZeroValue(value);
         }
         if ("isFinal".equals(name)) {
            return kind.endsWith("Time") && isFinalValue(kind, value);
         }
         if ("isZero".equals(name)) {
            return kind.endsWith("Interval") && isZeroValue(value);
         }
         if ("isEpsilon".equals(name)) {
            return kind.endsWith("Interval") && isEpsilonValue(kind, value);
         }
         if ("compareTo".equals(name)) return 0;
         if ("add".equals(name) || "subtract".equals(name)) return proxy;
         if ("distance".equals(name)) {
            // The Java interface declares distance as an interval return
            // type.  Returning the time proxy here happens to look plausible
            // to a dynamic handler but fails the JVM's covariant cast before
            // JPype can wrap it.  Preserve the provider carrier category at
            // this boundary even though the null provider does not claim
            // arithmetic semantics.
            String intervalKind = kind.replace("Time", "Interval");
            return logicalTimeCarrier(intervalKind, encoded);
         }
         if ("implementationName".equals(name) || "getName".equals(name)) return kind;
         return defaultValue(method.getReturnType());
      }
   }

   private static Object logicalTimeValue(String kind, byte[] encoded) {
      ByteBuffer source = ByteBuffer.wrap(encoded);
      if (kind.startsWith("HLAinteger64")) return source.getLong();
      if (kind.startsWith("HLAfloat64")) return Double.longBitsToDouble(source.getLong());
      throw new IllegalArgumentException("Unknown IEEE 1516.1-2010 logical-time kind: " + kind);
   }

   private static boolean isZeroValue(Object value) {
      if (value instanceof Long) return ((Long) value).longValue() == 0L;
      return Double.compare(((Double) value).doubleValue(), 0.0D) == 0;
   }

   private static boolean isFinalValue(String kind, Object value) {
      if (kind.startsWith("HLAinteger64")) return ((Long) value).longValue() == Long.MAX_VALUE;
      return Double.compare(((Double) value).doubleValue(), Double.MAX_VALUE) == 0;
   }

   private static boolean isEpsilonValue(String kind, Object value) {
      if (kind.startsWith("HLAinteger64")) return ((Long) value).longValue() == 1L;
      return Double.doubleToRawLongBits(((Double) value).doubleValue()) == 1L;
   }

   private static int octetBoundary(String kind) {
      if (kind.contains("boolean") || kind.contains("String") ||
          kind.contains("variableArray") || kind.contains("variantRecord")) return 4;
      if ("HLAfixedRecord".equals(kind)) return 4;
      if (kind.contains("64")) return 8;
      if (kind.contains("32")) return 4;
      if (kind.contains("16") || kind.contains("Pair") || kind.contains("unicodeChar")) return 2;
      return 1;
   }

   private static boolean isScalar(String kind) {
      return kind.startsWith("HLAinteger") || kind.startsWith("HLAfloat") ||
         kind.equals("HLAbyte") || kind.equals("HLAoctet") ||
         kind.equals("HLAASCIIchar") || kind.equals("HLAunicodeChar") ||
         kind.startsWith("HLAoctetPair") || kind.equals("HLAboolean") ||
         kind.equals("HLAASCIIstring") || kind.equals("HLAunicodeString");
   }

   private static Object scalarValue(String kind, byte[] value) {
      ByteBuffer buffer = ByteBuffer.wrap(value);
      if (kind.endsWith("LE")) buffer.order(ByteOrder.LITTLE_ENDIAN);
      if (kind.contains("integer16")) return buffer.getShort();
      if (kind.contains("integer32")) return buffer.getInt();
      if (kind.contains("integer64")) return buffer.getLong();
      if (kind.contains("float32")) return buffer.getFloat();
      if (kind.contains("float64")) return buffer.getDouble();
      if (kind.equals("HLAboolean")) return buffer.getInt() != 0;
      if (kind.equals("HLAbyte") || kind.equals("HLAoctet") || kind.equals("HLAASCIIchar")) {
         return value[0];
      }
      if (kind.equals("HLAunicodeChar")) return buffer.getShort();
      if (kind.startsWith("HLAoctetPair")) return buffer.getShort();
      int count = buffer.getInt();
      if (kind.equals("HLAASCIIstring")) {
         return new String(value, 4, count, StandardCharsets.US_ASCII);
      }
      if (kind.equals("HLAunicodeString")) {
         return StandardCharsets.UTF_16BE.decode(ByteBuffer.wrap(value, 4, count * 2)).toString();
      }
      return null;
   }

   private static byte[] scalarEncoding(String kind, Object value) {
      if (kind.equals("HLAASCIIstring")) {
         byte[] text = String.valueOf(value).getBytes(StandardCharsets.US_ASCII);
         ByteBuffer result = ByteBuffer.allocate(4 + text.length).putInt(text.length).put(text);
         return result.array();
      }
      if (kind.equals("HLAunicodeString")) {
         byte[] text = String.valueOf(value).getBytes(StandardCharsets.UTF_16BE);
         ByteBuffer result = ByteBuffer.allocate(4 + text.length).putInt(text.length / 2).put(text);
         return result.array();
      }
      int width = kind.contains("64") ? 8 : kind.contains("32") || kind.equals("HLAboolean") ? 4 :
         (kind.contains("16") || kind.contains("unicodeChar") || kind.startsWith("HLAoctetPair")) ? 2 : 1;
      ByteBuffer result = ByteBuffer.allocate(width);
      if (kind.endsWith("LE")) result.order(ByteOrder.LITTLE_ENDIAN);
      if (kind.contains("integer16") || kind.contains("unicodeChar") || kind.startsWith("HLAoctetPair")) result.putShort(((Number) value).shortValue());
      else if (kind.contains("integer32")) result.putInt(((Number) value).intValue());
      else if (kind.contains("integer64")) result.putLong(((Number) value).longValue());
      else if (kind.contains("float32")) result.putFloat(((Number) value).floatValue());
      else if (kind.contains("float64")) result.putDouble(((Number) value).doubleValue());
      else if (kind.equals("HLAboolean")) result.putInt(Boolean.TRUE.equals(value) ? 1 : 0);
      else result.put(((Number) value).byteValue());
      return result.array();
   }

   private static Object defaultValue(Class<?> type) {
      if (!type.isPrimitive()) return null;
      if (type == boolean.class) return false;
      if (type == byte.class) return (byte) 0;
      if (type == short.class) return (short) 0;
      if (type == int.class) return 0;
      if (type == long.class) return 0L;
      if (type == float.class) return 0.0F;
      if (type == double.class) return 0.0D;
      if (type == char.class) return (char) 0;
      return null;
   }
}
