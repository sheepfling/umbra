package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Base64;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

/** Checks distinct encoded handles for representative FOM names across executions. */
final class FomHandleUniquenessTck {
   private static final String FOM_PROPERTY = "hla.rti.tck.fomHandleUniquenessFom";
   private static final String DEFAULT_FOM =
      "packages/hla-rti-java-tck/fom/fom-handle-uniqueness-tck.xml";
   private static final List<String> REPEATABILITY_HANDLE_NAMES = List.of(
      "HLAobjectRoot.TckHandleObject",
      "HLAobjectRoot.TckHandleObject.TckHandleObjectChild",
      "TckHandleAttribute",
      "TckHandleChildAttribute",
      "HLAinteractionRoot.TckHandleInteraction",
      "HLAinteractionRoot.TckHandleInteraction.TckHandleInteractionChild",
      "TckHandleParameter",
      "TckHandleChildParameter",
      "TckHandleDimension",
      "TckHandleDimensionSecond",
      "TckHandleTransportation");

   private FomHandleUniquenessTck() {
   }

   static void run() throws Exception {
      String fom = System.getProperty(FOM_PROPERTY, DEFAULT_FOM);
      JavaTckSupport.check(Files.isRegularFile(Path.of(fom)),
         "FOM handle-uniqueness fixture does not exist: " + fom);
      run(RtiTckMain.factory(), fom, RtiTckMain.requireMim(),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   private static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-fom-handle-uniqueness-" + suffix;
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         owner.connect(SupportServicesTck.noOpFederateAmbassador(), callbackModel);
         ownerConnected = true;
         member.connect(SupportServicesTck.noOpFederateAmbassador(), callbackModel);
         memberConnected = true;
         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-fom-handle-owner-" + suffix, federation);
         ownerJoined = true;
         member.joinFederationExecution("java-tck-fom-handle-member-" + suffix, federation);
         memberJoined = true;

         Map<String, String> ownerHandles = collectHandleEncodings(owner);
         Map<String, String> memberHandles = collectHandleEncodings(member);
         checkOrderTypes(owner, member);
         JavaTckSupport.check(ownerHandles.equals(memberHandles),
            "federates in one execution received different encodings for the same FOM names");
         checkDistinctHandles(ownerHandles);
         checkNativeHandleReconstitution(owner, member);

         Map<String, String> firstExecutionFddHandles =
            collectFddNamedHandleEncodings(owner);
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;

         federation += "-repeat";
         owner.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.joinFederationExecution(
            "java-tck-fom-handle-owner-repeat-" + suffix, federation);
         ownerJoined = true;
         member.joinFederationExecution(
            "java-tck-fom-handle-member-repeat-" + suffix, federation);
         memberJoined = true;

         Map<String, String> repeatedExecutionFddHandles =
            collectFddNamedHandleEncodings(owner);
         JavaTckSupport.check(firstExecutionFddHandles.equals(repeatedExecutionFddHandles),
            "FDD-derived handles changed across executions using the same RTI and FOM");
      } finally {
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (memberConnected) {
            try { member.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void checkOrderTypes(RTIambassador owner, RTIambassador member)
         throws Exception {
      // The 2025 API exposes order types as fixed enum values, not handles.
      OrderType ownerReceive = owner.getOrderType("Receive");
      OrderType ownerTimeStamp = owner.getOrderType("TimeStamp");
      OrderType memberReceive = member.getOrderType("Receive");
      OrderType memberTimeStamp = member.getOrderType("TimeStamp");
      JavaTckSupport.check(ownerReceive != ownerTimeStamp,
         "distinct standard order names did not resolve to distinct OrderType enum values");
      JavaTckSupport.check(memberReceive != memberTimeStamp,
         "distinct standard order names did not resolve to distinct member OrderType enum values");
      JavaTckSupport.check(memberReceive == ownerReceive,
         "joined federates resolved Receive to different OrderType enum values");
      JavaTckSupport.check(memberTimeStamp == ownerTimeStamp,
         "joined federates resolved TimeStamp to different OrderType enum values");
   }

   private static void checkNativeHandleReconstitution(
         RTIambassador nativeBackedOwner, RTIambassador javaMember) throws Exception {
      String objectClassName = "HLAobjectRoot.TckHandleObject";
      ObjectClassHandle nativeBackedHandle =
         nativeBackedOwner.getObjectClassHandle(objectClassName);
      byte[] nativeEncodedHandle = new byte[nativeBackedHandle.encodedLength()];
      nativeBackedHandle.encode(nativeEncodedHandle, 0);

      ObjectClassHandle reconstructed =
         javaMember.getObjectClassHandleFactory().decode(nativeEncodedHandle, 0);
      JavaTckSupport.check(reconstructed.equals(
            javaMember.getObjectClassHandle(objectClassName)),
         "Java ObjectClassHandleFactory did not reconstitute the native-backed handle bytes");
      JavaTckSupport.check(nativeBackedOwner.getObjectClassName(reconstructed)
            .equals(objectClassName),
         "RTIambassador did not decode the Java-reconstituted handle bytes");
   }

   private static Map<String, String> collectHandleEncodings(RTIambassador ambassador)
         throws Exception {
      ObjectClassHandle objectClass =
         ambassador.getObjectClassHandle("HLAobjectRoot.TckHandleObject");
      ObjectClassHandle objectChild = ambassador.getObjectClassHandle(
         "HLAobjectRoot.TckHandleObject.TckHandleObjectChild");
      InteractionClassHandle interaction =
         ambassador.getInteractionClassHandle("HLAinteractionRoot.TckHandleInteraction");
      InteractionClassHandle interactionChild = ambassador.getInteractionClassHandle(
         "HLAinteractionRoot.TckHandleInteraction.TckHandleInteractionChild");

      Map<String, String> handles = new LinkedHashMap<>();
      handles.put("HLAobjectRoot", encodeHandle(
         ambassador.getObjectClassHandle("HLAobjectRoot")));
      handles.put("HLAobjectRoot.TckHandleObject", encodeHandle(objectClass));
      handles.put("HLAobjectRoot.TckHandleObject.TckHandleObjectChild",
         encodeHandle(objectChild));
      handles.put("TckHandleAttribute", encodeHandle(
         ambassador.getAttributeHandle(objectClass, "TckHandleAttribute")));
      handles.put("TckHandleChildAttribute", encodeHandle(
         ambassador.getAttributeHandle(objectChild, "TckHandleChildAttribute")));
      handles.put("HLAinteractionRoot", encodeHandle(
         ambassador.getInteractionClassHandle("HLAinteractionRoot")));
      handles.put("HLAinteractionRoot.TckHandleInteraction", encodeHandle(interaction));
      handles.put("HLAinteractionRoot.TckHandleInteraction.TckHandleInteractionChild",
         encodeHandle(interactionChild));
      handles.put("TckHandleParameter", encodeHandle(
         ambassador.getParameterHandle(interaction, "TckHandleParameter")));
      handles.put("TckHandleChildParameter", encodeHandle(
         ambassador.getParameterHandle(interactionChild, "TckHandleChildParameter")));
      handles.put("TckHandleDimension",
         encodeHandle(ambassador.getDimensionHandle("TckHandleDimension")));
      handles.put("TckHandleDimensionSecond",
         encodeHandle(ambassador.getDimensionHandle("TckHandleDimensionSecond")));
      handles.put("HLAreliable",
         encodeHandle(ambassador.getTransportationTypeHandle("HLAreliable")));
      handles.put("HLAbestEffort",
         encodeHandle(ambassador.getTransportationTypeHandle("HLAbestEffort")));
      handles.put("TckHandleTransportation",
         encodeHandle(ambassador.getTransportationTypeHandle("TckHandleTransportation")));
      return handles;
   }

   private static Map<String, String> collectFddNamedHandleEncodings(
         RTIambassador ambassador) throws Exception {
      Map<String, String> allHandles = collectHandleEncodings(ambassador);
      Map<String, String> fddNamedHandles = new LinkedHashMap<>();
      for (String name : REPEATABILITY_HANDLE_NAMES) {
         String encoding = allHandles.get(name);
         JavaTckSupport.check(encoding != null,
            "repeatability handle was not collected for " + name);
         fddNamedHandles.put(name, encoding);
      }
      return fddNamedHandles;
   }

   private static void checkDistinctHandles(Map<String, String> handles) {
      Map<String, String> firstNameByEncoding = new HashMap<>();
      List<String> collisions = new ArrayList<>();
      handles.forEach((name, encoding) -> {
         String firstName = firstNameByEncoding.putIfAbsent(encoding, name);
         if (firstName != null) {
            collisions.add(firstName + " == " + name + " (" + encoding + ")");
         }
      });
      JavaTckSupport.check(collisions.isEmpty(),
         "distinct FOM names in one execution shared encoded handle values: "
            + String.join("; ", collisions));
   }

   private static String encodeHandle(ObjectClassHandle handle) {
      return encodeHandle(handle.encodedLength(), handle::encode);
   }

   private static String encodeHandle(AttributeHandle handle) {
      return encodeHandle(handle.encodedLength(), handle::encode);
   }

   private static String encodeHandle(InteractionClassHandle handle) {
      return encodeHandle(handle.encodedLength(), handle::encode);
   }

   private static String encodeHandle(ParameterHandle handle) {
      return encodeHandle(handle.encodedLength(), handle::encode);
   }

   private static String encodeHandle(DimensionHandle handle) {
      return encodeHandle(handle.encodedLength(), handle::encode);
   }

   private static String encodeHandle(TransportationTypeHandle handle) {
      return encodeHandle(handle.encodedLength(), handle::encode);
   }

   private static String encodeHandle(int length, HandleEncoder encoder) {
      byte[] encoded = new byte[length];
      encoder.encode(encoded, 0);
      return Base64.getEncoder().encodeToString(encoded);
   }

   @FunctionalInterface
   private interface HandleEncoder {
      void encode(byte[] buffer, int offset);
   }
}
