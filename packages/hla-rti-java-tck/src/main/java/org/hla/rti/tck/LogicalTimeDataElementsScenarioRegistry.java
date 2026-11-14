package org.hla.rti.tck;

/** Registers the Java-visible logical-time DataElement wrapper contract. */
final class LogicalTimeDataElementsScenarioRegistry {
   private LogicalTimeDataElementsScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.logical-time-data-elements-contract",
            "Exercise Java logical-time and interval DataElement encodings and cursors",
            "time",
            "RTIambassador.getTimeFactory;EncoderFactory.createHLAlogicalTime;HLAlogicalTime.getValue;HLAlogicalTime.setValue;EncoderFactory.createHLAlogicalTimeInterval;HLAlogicalTimeInterval.getValue;HLAlogicalTimeInterval.setValue;DataElement.getEncodedLength;DataElement.getOctetBoundary;DataElement.toByteArray;DataElement.encode;DataElement.decode(byte[]);DataElement.decode(ByteWrapper);ByteWrapper.put;ByteWrapper.advance;ByteWrapper.getPos;ByteWrapper.array",
            new String[0], true), RtiTckMain::logicalTimeDataElements);
   }
}
