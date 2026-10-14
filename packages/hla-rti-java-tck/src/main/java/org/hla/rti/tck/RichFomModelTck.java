package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAASCIIstring;
import hla.rti1516_2025.encoding.HLAboolean;
import hla.rti1516_2025.encoding.HLAbyte;
import hla.rti1516_2025.encoding.HLAfixedArray;
import hla.rti1516_2025.encoding.HLAfixedRecord;
import hla.rti1516_2025.encoding.HLAfloat64BE;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import hla.rti1516_2025.encoding.HLAopaqueData;
import hla.rti1516_2025.encoding.HLAunicodeString;
import hla.rti1516_2025.encoding.HLAvariantRecord;
import hla.rti1516_2025.encoding.HLAvariableArray;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Runs the adapter's rich standard FOM through Java lookup and delivery services. */
final class RichFomModelTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;
   private static final String CUSTOM_TRANSPORTATION_PROPERTY =
      "hla.rti.tck.customTransportationType";

   private RichFomModelTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder memberRecorder = new Recorder();
      String federation = "java-tck-rich-fom-model-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         member.connect(memberRecorder.proxy(), callbackModel);
         memberConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-rich-fom-owner", federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-rich-fom-member", federation);
         memberJoined = true;

         verifyModelLookups(owner, member);

         EncoderFactory encoder = factory.getEncoderFactory();
         Map<String, byte[]> values = typedAttributeValues(encoder);
         String baseObjectName = setting("typedObjectClass", "HLAobjectRoot.TckTypedObject");
         String derivedObjectName = setting("typedDerivedObjectClass",
            "HLAobjectRoot.TckTypedObject.TckTypedDerivedObject");
         String baseInteractionName = setting("typedInteractionClass",
            "HLAinteractionRoot.TckTypedInteraction");
         String derivedInteractionName = setting("typedDerivedInteractionClass",
            "HLAinteractionRoot.TckTypedInteraction.TckTypedDerivedInteraction");
         String integerAttributeName = setting("typedIntegerAttribute", "IntegerValue");
         String integerParameterName = setting("typedIntegerParameter", "IntegerParameter");
         String derivedParameterName = setting("typedDerivedParameter", "DerivedParameter");

         ObjectClassHandle ownerBaseObject = owner.getObjectClassHandle(baseObjectName);
         ObjectClassHandle ownerDerivedObject = owner.getObjectClassHandle(derivedObjectName);
         ObjectClassHandle memberDerivedObject = member.getObjectClassHandle(derivedObjectName);
         check(derivedObjectName.equals(owner.getObjectClassName(ownerDerivedObject))
               && ownerDerivedObject.equals(memberDerivedObject),
            "FOM model object-class names or cross-federate handles did not round-trip");
         AttributeHandle inheritedOwnerInteger = owner.getAttributeHandle(
            ownerBaseObject, integerAttributeName);
         AttributeHandle ownerInteger = owner.getAttributeHandle(
            ownerDerivedObject, integerAttributeName);
         check(inheritedOwnerInteger.equals(ownerInteger),
            "inherited FOM attribute lookup did not retain the base handle");

         Map<String, AttributeHandle> ownerAttributes = new LinkedHashMap<>();
         Map<String, AttributeHandle> memberAttributes = new LinkedHashMap<>();
         for (String name : values.keySet()) {
            AttributeHandle ownerHandle = owner.getAttributeHandle(ownerDerivedObject, name);
            AttributeHandle memberHandle = member.getAttributeHandle(memberDerivedObject, name);
            check(name.equals(member.getAttributeName(memberDerivedObject, memberHandle)),
               "FOM model attribute name did not round-trip: " + name);
            ownerAttributes.put(name, ownerHandle);
            memberAttributes.put(name, memberHandle);
         }
         check(ownerAttributes.get(integerAttributeName).equals(
               memberAttributes.get(integerAttributeName)),
            "FOM model inherited attribute handle differed between members");

         AttributeHandleSet ownerAttributeSet = owner.getAttributeHandleSetFactory().create();
         AttributeHandleSet memberAttributeSet = member.getAttributeHandleSetFactory().create();
         for (String name : values.keySet()) {
            ownerAttributeSet.add(ownerAttributes.get(name));
            memberAttributeSet.add(memberAttributes.get(name));
         }
         owner.publishObjectClassAttributes(ownerDerivedObject, ownerAttributeSet);
         member.subscribeObjectClassAttributes(memberDerivedObject, memberAttributeSet);

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerDerivedObject);
         await(member, callbackModel, () -> memberRecorder.discoveries.contains(object),
            "rich-FOM object discovery");
         JavaTckSupport.check(memberRecorder.discoveries.contains(object),
            "rich-FOM discovery returned the wrong object");

         AttributeHandleValueMap attributeValues =
            owner.getAttributeHandleValueMapFactory().create(values.size());
         for (Map.Entry<String, byte[]> entry : values.entrySet()) {
            attributeValues.put(ownerAttributes.get(entry.getKey()), entry.getValue());
         }
         byte[] updateTag = new byte[] {(byte) 0xf0, 0x0d};
         owner.updateAttributeValues(object, attributeValues, updateTag);
         await(member, callbackModel, () -> !memberRecorder.reflections.isEmpty(),
            "rich-FOM typed attribute reflection");
         Reflection reflection = memberRecorder.reflections.get(0);
         check(object.equals(reflection.object) && Arrays.equals(updateTag, reflection.tag),
            "rich-FOM reflection returned the wrong object or tag");
         check(reflection.values.size() == values.size(),
            "rich-FOM reflection returned the wrong attribute count");
         for (Map.Entry<String, byte[]> entry : values.entrySet()) {
            byte[] reflected = reflection.values.get(memberAttributes.get(entry.getKey()));
            check(Arrays.equals(entry.getValue(), reflected),
               "rich-FOM reflection changed encoded data for " + entry.getKey());
         }

         InteractionClassHandle ownerBaseInteraction =
            owner.getInteractionClassHandle(baseInteractionName);
         InteractionClassHandle ownerDerivedInteraction =
            owner.getInteractionClassHandle(derivedInteractionName);
         InteractionClassHandle memberDerivedInteraction =
            member.getInteractionClassHandle(derivedInteractionName);
         check(derivedInteractionName.equals(
               owner.getInteractionClassName(ownerDerivedInteraction))
               && ownerDerivedInteraction.equals(memberDerivedInteraction),
            "FOM model interaction-class names or handles did not round-trip");
         ParameterHandle inheritedInteger = owner.getParameterHandle(
            ownerBaseInteraction, integerParameterName);
         ParameterHandle ownerIntegerParameter = owner.getParameterHandle(
            ownerDerivedInteraction, integerParameterName);
         check(inheritedInteger.equals(ownerIntegerParameter)
               && ownerIntegerParameter.equals(member.getParameterHandle(
                  memberDerivedInteraction, integerParameterName)),
            "inherited FOM parameter handle did not retain identity");

         Map<String, String> parameterToAttribute = new LinkedHashMap<>();
         parameterToAttribute.put(integerParameterName, integerAttributeName);
         parameterToAttribute.put(setting("typedAsciiParameter", "AsciiParameter"),
            setting("typedAsciiAttribute", "AsciiValue"));
         parameterToAttribute.put(setting("typedFixedArrayParameter", "FixedArrayParameter"),
            setting("typedFixedArrayAttribute", "FixedArrayValue"));
         parameterToAttribute.put(setting("typedRecordParameter", "RecordParameter"),
            setting("typedRecordAttribute", "RecordValue"));
         parameterToAttribute.put(setting("typedChoiceParameter", "ChoiceParameter"),
            setting("typedChoiceAttribute", "ChoiceValue"));
         parameterToAttribute.put(derivedParameterName, "");

         ParameterHandleValueMap parameterValues =
            owner.getParameterHandleValueMapFactory().create(parameterToAttribute.size());
         Map<ParameterHandle, byte[]> expectedParameters = new LinkedHashMap<>();
         for (Map.Entry<String, String> entry : parameterToAttribute.entrySet()) {
            ParameterHandle handle = owner.getParameterHandle(
               ownerDerivedInteraction, entry.getKey());
            byte[] bytes = entry.getValue().isEmpty()
               ? encoder.createHLAunicodeString("derived").toByteArray()
               : values.get(entry.getValue());
            check(handle != null && entry.getKey().equals(
                  owner.getParameterName(ownerDerivedInteraction, handle)),
               "FOM model parameter name did not round-trip: " + entry.getKey());
            parameterValues.put(handle, bytes);
            expectedParameters.put(handle, bytes);
         }
         owner.publishInteractionClass(ownerDerivedInteraction);
         member.subscribeInteractionClass(memberDerivedInteraction);
         byte[] interactionTag = new byte[] {(byte) 0xc0, (byte) 0xde};
         owner.sendInteraction(ownerDerivedInteraction, parameterValues, interactionTag);
         await(member, callbackModel, () -> !memberRecorder.interactions.isEmpty(),
            "rich-FOM typed interaction delivery");
         Interaction interaction = memberRecorder.interactions.get(0);
         check(memberDerivedInteraction.equals(interaction.interaction)
               && Arrays.equals(interactionTag, interaction.tag)
               && interaction.parameters.size() == expectedParameters.size(),
            "rich-FOM interaction returned the wrong class, tag, or parameter count");
         for (Map.Entry<ParameterHandle, byte[]> entry : expectedParameters.entrySet()) {
            check(Arrays.equals(entry.getValue(), interaction.parameters.get(entry.getKey())),
               "rich-FOM interaction changed encoded parameter data");
         }
      } finally {
         if (memberJoined) resign(member, ResignAction.NO_ACTION);
         if (ownerJoined) resign(owner, ResignAction.DELETE_OBJECTS);
         if (created) destroy(owner, federation);
         if (memberConnected) disconnect(member);
         if (ownerConnected) disconnect(owner);
      }
   }

   private static void verifyModelLookups(RTIambassador ambassador, RTIambassador peer)
         throws Exception {
      String typedObjectName = setting("typedObjectClass", "HLAobjectRoot.TckTypedObject");
      String typedDerivedObjectName = setting("typedDerivedObjectClass",
         "HLAobjectRoot.TckTypedObject.TckTypedDerivedObject");
      String typedInteractionName = setting("typedInteractionClass",
         "HLAinteractionRoot.TckTypedInteraction");
      String typedDerivedInteractionName = setting("typedDerivedInteractionClass",
         "HLAinteractionRoot.TckTypedInteraction.TckTypedDerivedInteraction");
      ObjectClassHandle typedObject = ambassador.getObjectClassHandle(typedObjectName);
      ObjectClassHandle typedDerivedObject = ambassador.getObjectClassHandle(
         typedDerivedObjectName);
      InteractionClassHandle typedInteraction =
         ambassador.getInteractionClassHandle(typedInteractionName);
      InteractionClassHandle typedDerivedInteraction =
         ambassador.getInteractionClassHandle(typedDerivedInteractionName);
      check(typedObjectName.equals(ambassador.getObjectClassName(typedObject))
            && typedDerivedObjectName.equals(ambassador.getObjectClassName(typedDerivedObject))
            && typedInteractionName.equals(ambassador.getInteractionClassName(typedInteraction))
            && typedDerivedInteractionName.equals(
               ambassador.getInteractionClassName(typedDerivedInteraction)),
         "FOM model class names did not round-trip");

      String dimensionName = System.getProperty("hla.rti.tck.dimension", "TckDimension");
      DimensionHandle dimension = ambassador.getDimensionHandle(dimensionName);
      check(dimensionName.equals(ambassador.getDimensionName(dimension))
            && ambassador.getDimensionUpperBound(dimension) == 64L,
         "FOM model dimension lookup did not round-trip its name and upper bound");
      DimensionHandleSet objectDimensions =
         ambassador.getAvailableDimensionsForObjectClass(typedDerivedObject);
      DimensionHandleSet interactionDimensions =
         ambassador.getAvailableDimensionsForInteractionClass(typedInteraction);
      check(objectDimensions.contains(dimension) && interactionDimensions.contains(dimension),
         "FOM model class-dimension association was not composed");

      check(Double.compare(ambassador.getUpdateRateValue(
               System.getProperty("hla.rti.tck.fomUpdateRate", "TckFast")), 25.0) == 0
            && Double.compare(ambassador.getUpdateRateValue(
               System.getProperty("hla.rti.tck.fomSlowUpdateRate", "TckSlow")), 0.5) == 0,
         "FOM model update-rate values did not round-trip");
      String transportationName = System.getProperty(
         CUSTOM_TRANSPORTATION_PROPERTY, "TckBestEffort");
      TransportationTypeHandle transportation =
         ambassador.getTransportationTypeHandle(transportationName);
      check(transportationName.equals(ambassador.getTransportationTypeName(transportation)),
         "FOM model custom transportation name did not round-trip");

      boolean autoProvide = ambassador.getAutoProvideSwitch();
      boolean objectRelevance = ambassador.getObjectClassRelevanceAdvisorySwitch();
      boolean attributeRelevance = ambassador.getAttributeRelevanceAdvisorySwitch();
      boolean interactionRelevance = ambassador.getInteractionRelevanceAdvisorySwitch();
      ObjectClassHandle publishOnly = ambassador.getObjectClassHandle(
         "HLAobjectRoot.TckPublishOnlyObject");
      AttributeHandle publishOnlyAttribute = ambassador.getAttributeHandle(
         publishOnly, "PublishedValue");
      check(publishOnly != null && publishOnlyAttribute != null,
         "FOM model publication-only declarations were not loaded");

      check(typedObject.equals(peer.getObjectClassHandle(typedObjectName))
            && typedInteraction.equals(peer.getInteractionClassHandle(typedInteractionName)),
         "FOM model base class handles differed across federates");
   }

   private static Map<String, byte[]> typedAttributeValues(EncoderFactory encoder)
         throws Exception {
      Map<String, byte[]> values = new LinkedHashMap<>();
      values.put(setting("typedIdentityAttribute", "Identity"),
         encoder.createHLAASCIIstring("typed-object").toByteArray());
      values.put(setting("typedIntegerAttribute", "IntegerValue"),
         encoder.createHLAinteger32BE(42).toByteArray());
      values.put(setting("typedFloatAttribute", "FloatValue"),
         encoder.createHLAfloat64BE(3.5).toByteArray());
      values.put(setting("typedAsciiAttribute", "AsciiValue"),
         encoder.createHLAASCIIstring("ascii").toByteArray());
      values.put(setting("typedUnicodeAttribute", "UnicodeValue"),
         encoder.createHLAunicodeString("unicode").toByteArray());
      values.put(setting("typedEnumAttribute", "EnumValue"),
         encoder.createHLAinteger32BE(1).toByteArray());

      HLAfixedArray<HLAbyte> fixedArray = encoder.createHLAfixedArray(
         encoder.createHLAbyte((byte) 0x11), encoder.createHLAbyte((byte) 0x22),
         encoder.createHLAbyte((byte) 0x33));
      values.put(setting("typedFixedArrayAttribute", "FixedArrayValue"),
         fixedArray.toByteArray());
      HLAvariableArray<HLAbyte> variableArray = encoder.createHLAvariableArray(
         index -> encoder.createHLAbyte());
      variableArray.addElement(encoder.createHLAbyte((byte) 0x44));
      variableArray.addElement(encoder.createHLAbyte((byte) 0x55));
      values.put(setting("typedVariableArrayAttribute", "VariableArrayValue"),
         variableArray.toByteArray());

      HLAfixedRecord record = encoder.createHLAfixedRecord();
      record.add(encoder.createHLAinteger32BE(7));
      record.add(encoder.createHLAboolean(true));
      values.put(setting("typedRecordAttribute", "RecordValue"), record.toByteArray());

      HLAinteger32BE ready = encoder.createHLAinteger32BE(0);
      HLAinteger32BE busy = encoder.createHLAinteger32BE(1);
      HLAvariantRecord<HLAinteger32BE> choice =
         encoder.createHLAvariantRecord(encoder.createHLAinteger32BE());
      choice.setVariant(ready, encoder.createHLAinteger32BE());
      choice.setVariant(busy, encoder.createHLAASCIIstring());
      choice.setVariant(ready, encoder.createHLAinteger32BE(41));
      choice.setDiscriminant(ready);
      values.put(setting("typedChoiceAttribute", "ChoiceValue"), choice.toByteArray());
      values.put(setting("typedReferenceAttribute", "ReferenceValue"),
         encoder.createHLAASCIIstring("typed-object").toByteArray());
      values.put(setting("typedOpaqueAttribute", "OpaqueValue"),
         encoder.createHLAopaqueData(new byte[] {(byte) 0xa5, 0, 0x5a}).toByteArray());
      values.put(setting("typedDerivedAttribute", "DerivedValue"),
         encoder.createHLAinteger32BE(99).toByteArray());
      return values;
   }

   private static String setting(String suffix, String defaultValue) {
      return System.getProperty("hla.rti.tck." + suffix, defaultValue);
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values, byte[] tag) {
         this.object = object;
         this.values = values;
         this.tag = tag == null ? null : tag.clone();
      }
   }

   private static final class Interaction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;

      private Interaction(InteractionClassHandle interaction,
            ParameterHandleValueMap parameters, byte[] tag) {
         this.interaction = interaction;
         this.parameters = parameters;
         this.tag = tag == null ? null : tag.clone();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     ((AttributeHandleValueMap) arguments[1]).clone(), (byte[]) arguments[2]));
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  interactions.add(new Interaction((InteractionClassHandle) arguments[0],
                     ((ParameterHandleValueMap) arguments[1]).clone(), (byte[]) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK rich-FOM callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }

   private static void check(boolean condition, String message) {
      JavaTckSupport.check(condition, message);
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static void destroy(RTIambassador ambassador, String federation) {
      try { ambassador.destroyFederationExecution(federation); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }
}
