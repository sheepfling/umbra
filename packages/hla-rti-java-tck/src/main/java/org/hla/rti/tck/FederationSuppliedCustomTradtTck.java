package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.HLAinteger64Time;
import hla.rti1516_2025.time.HLAinteger64TimeFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeFactoryFactory;
import java.util.UUID;

/** Checks federation selection and Java API handling of a custom integer64 TRADT. */
final class FederationSuppliedCustomTradtTck {
   private static final String CUSTOM_FACTORY_NAME = TckTaggedTimeFactory.NAME;
   private static final long WALL_CLOCK_KIND = 1L << 62;

   private FederationSuppliedCustomTradtTck() { }

   static void run(RtiFactory factory, String fom, CallbackModel callbackModel)
         throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      String federation = "java-tck-custom-tradt-" + UUID.randomUUID();
      String federateName = "java-tck-custom-tradt-federate-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         LogicalTimeFactory<?, ?> serviceFactory =
            LogicalTimeFactoryFactory.getLogicalTimeFactory(CUSTOM_FACTORY_NAME);
         JavaTckSupport.check(serviceFactory instanceof HLAinteger64TimeFactory
               && CUSTOM_FACTORY_NAME.equals(serviceFactory.getName()),
            "Java service loading did not expose the custom integer64 time factory");

         ambassador.connect(new NullFederateAmbassador(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(
            federation, new String[] {fom}, CUSTOM_FACTORY_NAME);
         created = true;
         ambassador.joinFederationExecution(federateName, "java-tck", federation);
         joined = true;

         LogicalTimeFactory<?, ?> selectedFactory = ambassador.getTimeFactory();
         JavaTckSupport.check(selectedFactory instanceof HLAinteger64TimeFactory
               && CUSTOM_FACTORY_NAME.equals(selectedFactory.getName()),
            "RTI did not select the federation-supplied custom logical-time factory");
         HLAinteger64TimeFactory integerFactory = (HLAinteger64TimeFactory) selectedFactory;

         HLAinteger64Time initial = integerFactory.makeInitial();
         JavaTckSupport.check(initial.isInitial() && initial.getValue() == 0,
            "custom factory did not create its initial integer64 time");
         HLAinteger64Time logicalTime = integerFactory.makeTime(7);
         JavaTckSupport.check(logicalTime.getValue() == 7
               && (logicalTime.getValue() & WALL_CLOCK_KIND) == 0,
            "custom factory did not produce the logical-time discriminator");

         HLAinteger64Time wallClockWireTime =
            integerFactory.makeTime(WALL_CLOCK_KIND | 7);
         byte[] encodedWallClockTime = new byte[wallClockWireTime.encodedLength()];
         wallClockWireTime.encode(encodedWallClockTime, 0);
         HLAinteger64Time decodedWallClockTime =
            integerFactory.decodeTime(encodedWallClockTime, 0);
         JavaTckSupport.check((decodedWallClockTime.getValue() & WALL_CLOCK_KIND) != 0,
            "custom factory decode lost the wall-clock discriminator");
         JavaTckSupport.check(!logicalTime.equals(decodedWallClockTime),
            "equal payloads with distinct TRADT time kinds compared equal");

         LogicalTime<?, ?> queriedTime = ambassador.queryLogicalTime();
         JavaTckSupport.check(queriedTime instanceof HLAinteger64Time
               && queriedTime.isInitial()
               && ((HLAinteger64Time) queriedTime).getValue() == 0,
            "Query Logical Time did not preserve the selected initial TRADT value");
      } finally {
         if (joined) try {
            ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (created) try {
            ambassador.destroyFederationExecution(federation);
         } catch (Exception ignored) { }
         if (connected) try { ambassador.disconnect(); } catch (Exception ignored) { }
      }
   }
}
