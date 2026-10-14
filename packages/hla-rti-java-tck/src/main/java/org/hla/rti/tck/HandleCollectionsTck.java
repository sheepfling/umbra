package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.FederateHandleSaveStatusPair;
import hla.rti1516_2025.FederateRestoreStatus;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RangeBounds;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.RestoreStatus;
import hla.rti1516_2025.SaveStatus;
import java.util.Arrays;

/** Provider-independent Java subset of the standard handle/collection contract. */
final class HandleCollectionsTck {
   private HandleCollectionsTck() {
   }

   static void run(RtiFactory factory) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      attributeHandleSet(ambassador);
      attributeValueMap(ambassador);
      parameterValueMap(ambassador);
      attributeRegionPairs(ambassador);
      statusValues();
      rangeBounds();
   }

   private static void attributeHandleSet(RTIambassador ambassador) throws Exception {
      AttributeHandleSet values = ambassador.getAttributeHandleSetFactory().create();
      AttributeHandle first = new TestAttributeHandle(0x31);
      AttributeHandle equal = new TestAttributeHandle(0x31);
      values.add(first);
      values.add(equal);
      require(values.size() == 1 && values.contains(first) && values.contains(equal),
         "AttributeHandleSet did not preserve set equality semantics");
      AttributeHandleSet copied = values.clone();
      require(copied.size() == 1 && copied.contains(first),
         "AttributeHandleSet.clone did not preserve its entries");
      copied.remove(equal);
      require(values.size() == 1 && values.contains(first) && copied.isEmpty(),
         "AttributeHandleSet.clone shared mutable set structure");
   }

   private static void attributeValueMap(RTIambassador ambassador) throws Exception {
      AttributeHandleValueMap values =
         ambassador.getAttributeHandleValueMapFactory().create(1);
      AttributeHandle handle = new TestAttributeHandle(0x41);
      byte[] data = new byte[] {0x40, 0x50};
      values.put(handle, data);
      require(values.size() == 1 && values.containsKey(handle)
            && Arrays.equals(values.get(handle), data),
         "AttributeHandleValueMap did not preserve its entry");
      AttributeHandleValueMap copied = values.clone();
      require(copied.size() == 1 && Arrays.equals(copied.get(handle), data),
         "AttributeHandleValueMap.clone did not preserve its entry");
      copied.remove(handle);
      require(values.size() == 1 && values.containsKey(handle) && copied.isEmpty(),
         "AttributeHandleValueMap.clone shared mutable map structure");
   }

   private static void parameterValueMap(RTIambassador ambassador) throws Exception {
      ParameterHandleValueMap values =
         ambassador.getParameterHandleValueMapFactory().create(1);
      ParameterHandle handle = new TestParameterHandle(0x51);
      byte[] data = new byte[] {0x10, 0x20};
      values.put(handle, data);
      require(values.size() == 1 && values.containsKey(handle)
            && Arrays.equals(values.get(handle), data),
         "ParameterHandleValueMap did not preserve its entry");
      ParameterHandleValueMap copied = values.clone();
      require(copied.size() == 1 && Arrays.equals(copied.get(handle), data),
         "ParameterHandleValueMap.clone did not preserve its entry");
      copied.remove(handle);
      require(values.size() == 1 && values.containsKey(handle) && copied.isEmpty(),
         "ParameterHandleValueMap.clone shared mutable map structure");
   }

   private static void attributeRegionPairs(RTIambassador ambassador) throws Exception {
      AttributeHandleSet attributes = ambassador.getAttributeHandleSetFactory().create();
      AttributeHandle attribute = new TestAttributeHandle(0x61);
      attributes.add(attribute);
      RegionHandleSet regions = ambassador.getRegionHandleSetFactory().create();
      RegionHandle region = new TestRegionHandle(0x71);
      regions.add(region);
      AttributeRegionAssociation association =
         new AttributeRegionAssociation(attributes, regions);
      AttributeSetRegionSetPairList pairs =
         ambassador.getAttributeSetRegionSetPairListFactory().create(1);
      pairs.add(association);
      require(pairs.size() == 1 && pairs.get(0).ahset.contains(attribute)
            && pairs.get(0).rhset.contains(region),
         "AttributeSetRegionSetPairList did not preserve its association");
      AttributeSetRegionSetPairList copied = pairs.clone();
      copied.clear();
      require(pairs.size() == 1 && pairs.get(0) == association && copied.isEmpty(),
         "AttributeSetRegionSetPairList.clone did not create an independent list");
   }

   private static void statusValues() {
      FederateHandle handle = new TestFederateHandle(0x21);
      FederateHandleSaveStatusPair saveStatus =
         new FederateHandleSaveStatusPair(handle, SaveStatus.NO_SAVE_IN_PROGRESS);
      FederateRestoreStatus restoreStatus =
         new FederateRestoreStatus(handle, new TestFederateHandle(0x22),
            RestoreStatus.FEDERATE_RESTORING);
      require(saveStatus.handle.equals(handle)
            && saveStatus.status == SaveStatus.NO_SAVE_IN_PROGRESS,
         "FederateHandleSaveStatusPair did not preserve its fields");
      require(restoreStatus.preRestoreHandle.equals(handle)
            && restoreStatus.postRestoreHandle.equals(new TestFederateHandle(0x22))
            && restoreStatus.status == RestoreStatus.FEDERATE_RESTORING,
         "FederateRestoreStatus did not preserve its fields");
   }

   private static void rangeBounds() {
      RangeBounds bounds = new RangeBounds(2, 7);
      RangeBounds equalBounds = new RangeBounds(2, 7);
      RangeBounds differentBounds = new RangeBounds(3, 7);
      require(bounds.lower == 2 && bounds.upper == 7
            && bounds.equals(equalBounds)
            && bounds.hashCode() == equalBounds.hashCode()
            && !bounds.equals(differentBounds),
         "RangeBounds value semantics changed");
   }

   private static void require(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }

   private static final class TestAttributeHandle implements AttributeHandle {
      private final int value;

      private TestAttributeHandle(int value) {
         this.value = value;
      }

      @Override
      public boolean equals(Object other) {
         return other instanceof TestAttributeHandle
            && ((TestAttributeHandle) other).value == value;
      }

      @Override
      public int hashCode() {
         return value;
      }

      @Override
      public int encodedLength() {
         return 1;
      }

      @Override
      public void encode(byte[] buffer, int offset) {
         buffer[offset] = (byte) value;
      }

      @Override
      public String toString() {
         return "test-attribute-" + value;
      }
   }

   private static final class TestParameterHandle implements ParameterHandle {
      private final int value;

      private TestParameterHandle(int value) {
         this.value = value;
      }

      @Override
      public boolean equals(Object other) {
         return other instanceof TestParameterHandle
            && ((TestParameterHandle) other).value == value;
      }

      @Override
      public int hashCode() {
         return value;
      }

      @Override
      public int encodedLength() {
         return 1;
      }

      @Override
      public void encode(byte[] buffer, int offset) {
         buffer[offset] = (byte) value;
      }

      @Override
      public String toString() {
         return "test-parameter-" + value;
      }
   }

   private static final class TestRegionHandle implements RegionHandle {
      private final int value;

      private TestRegionHandle(int value) {
         this.value = value;
      }

      @Override
      public boolean equals(Object other) {
         return other instanceof TestRegionHandle
            && ((TestRegionHandle) other).value == value;
      }

      @Override
      public int hashCode() {
         return value;
      }

      @Override
      public int encodedLength() {
         return 1;
      }

      @Override
      public void encode(byte[] buffer, int offset) {
         buffer[offset] = (byte) value;
      }

      @Override
      public String toString() {
         return "test-region-" + value;
      }
   }

   private static final class TestFederateHandle implements FederateHandle {
      private final int value;

      private TestFederateHandle(int value) {
         this.value = value;
      }

      @Override
      public boolean equals(Object other) {
         return other instanceof TestFederateHandle
            && ((TestFederateHandle) other).value == value;
      }

      @Override
      public int hashCode() {
         return value;
      }

      @Override
      public int encodedLength() {
         return 1;
      }

      @Override
      public void encode(byte[] buffer, int offset) {
         buffer[offset] = (byte) value;
      }

      @Override
      public String toString() {
         return "test-federate-" + value;
      }
   }
}
