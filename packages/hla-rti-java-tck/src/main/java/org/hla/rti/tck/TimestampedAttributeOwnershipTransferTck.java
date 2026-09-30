package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Checks delivery of an accepted timestamped update after its attribute changes owner. */
final class TimestampedAttributeOwnershipTransferTck {
   private TimestampedAttributeOwnershipTransferTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador acquirer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder acquirerRecorder = new Recorder();
      String federation = "java-tck-timestamped-attribute-owner-transfer-" + UUID.randomUUID();
      boolean ownerConnected = false, acquirerConnected = false, created = false;
      boolean ownerJoined = false, acquirerJoined = false;
      boolean ownerPublished = false, acquirerPublished = false, subscribed = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         acquirer.connect(acquirerRecorder.proxy(), callbackModel);
         acquirerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-timestamped-attribute-owner", federation);
         ownerJoined = true;
         acquirer.joinFederationExecution("java-tck-timestamped-attribute-acquirer", federation);
         acquirerJoined = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle acquirerClass = acquirer.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle acquirerAttribute = acquirer.getAttributeHandle(acquirerClass, attributeName);
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet acquirerAttributes = JavaTckSupport.attributeSet(
            acquirer, acquirerAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         acquirer.publishObjectClassAttributes(acquirerClass, acquirerAttributes);
         acquirerPublished = true;
         acquirer.subscribeObjectClassAttributes(acquirerClass, acquirerAttributes);
         subscribed = true;
         owner.changeDefaultAttributeOrderType(ownerClass, ownerAttributes, OrderType.TIMESTAMP);

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         String objectName = owner.getObjectInstanceName(object);
         await(() -> acquirerRecorder.discovered.contains(object), acquirer,
            "ownership-transfer object discovery");

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTimeInterval lookahead = intervalOffset(epsilon, 5);
         LogicalTime ownerInitial = owner.queryLogicalTime();
         LogicalTime acquirerInitial = acquirer.queryLogicalTime();
         check(ownerInitial.equals(acquirerInitial),
            "ownership-transfer federates began at different logical times");

         acquirer.enableTimeConstrained();
         await(() -> acquirerRecorder.timeConstrainedEnabled, acquirer,
            "timeConstrainedEnabled callback");
         owner.enableTimeRegulation(lookahead);
         await(() -> ownerRecorder.timeRegulationEnabled, owner,
            "timeRegulationEnabled callback");

         LogicalTime updateTime = offset(ownerInitial, epsilon, 5);
         byte[] value = new byte[] {(byte) 0xBE, (byte) 0xEF};
         byte[] updateTag = new byte[] {0x54, 0x53, 0x4F, 0x2D, 0x4F, 0x57, 0x4E};
         byte[] divestitureTag = new byte[] {0x44, 0x49, 0x56};
         byte[] acquisitionTag = new byte[] {0x41, 0x43, 0x51};
         AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
         values.put(ownerAttribute, value);
         MessageRetractionReturn update = owner.updateAttributeValues(
            object, values, updateTag, updateTime);
         check(update != null && update.retractionHandleIsValid && update.handle != null,
            "timestamped update returned no valid retraction designator");
         check(acquirerRecorder.reflections.isEmpty(),
            "timestamped update reflected before the constrained grant");

         owner.unconditionalAttributeOwnershipDivestiture(
            object, ownerAttributes, divestitureTag);
         await(() -> !acquirerRecorder.assumptions.isEmpty(), acquirer,
            "requestAttributeOwnershipAssumption callback");
         OwnershipEvent assumption = acquirerRecorder.assumptions.get(0);
         assertOwnershipEvent(assumption, object, acquirerAttribute, divestitureTag,
            "ownership assumption");
         check(!owner.isAttributeOwnedByFederate(object, ownerAttribute)
               && !acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "unconditional divestiture changed ownership before acquisition");

         acquirer.attributeOwnershipAcquisitionIfAvailable(
            object, acquirerAttributes, acquisitionTag);
         await(() -> !acquirerRecorder.acquisitions.isEmpty(), acquirer,
            "attributeOwnershipAcquisitionNotification callback");
         OwnershipEvent acquisition = acquirerRecorder.acquisitions.get(0);
         assertOwnershipEvent(acquisition, object, acquirerAttribute, acquisitionTag,
            "ownership acquisition");
         check(acquirer.isAttributeOwnedByFederate(object, acquirerAttribute),
            "if-available acquisition did not transfer attribute ownership");

         LogicalTime ownerTarget = offset(ownerInitial, epsilon, 2);
         LogicalTime acquirerTarget = offset(acquirerInitial, epsilon, 5);
         acquirerRecorder.callbackOrder.clear();
         acquirer.timeAdvanceRequest(acquirerTarget);
         owner.timeAdvanceRequest(ownerTarget);
         await(() -> !acquirerRecorder.reflections.isEmpty()
               && acquirerRecorder.grants.size() == 1, acquirer,
            "timestamped reflection at the constrained grant");
         await(() -> ownerRecorder.grants.size() == 1, owner,
            "owner time-advance grant");

         check(acquirerRecorder.reflections.size() == 1,
            "ownership transfer produced duplicate timestamped reflections");
         Reflection reflection = acquirerRecorder.reflections.get(0);
         check(object.equals(reflection.object),
            "post-transfer reflection identified the wrong object");
         check(reflection.values.size() == 1
               && Arrays.equals(value, reflection.values.get(acquirerAttribute)),
            "post-transfer reflection changed the attribute value");
         check(Arrays.equals(updateTag, reflection.tag)
               && producer.equals(reflection.producer)
               && updateTime.equals(reflection.time),
            "post-transfer reflection changed tag, producer, or timestamp");
         check(reflection.sentOrder == OrderType.TIMESTAMP
               && reflection.receivedOrder == OrderType.TIMESTAMP
               && update.handle.equals(reflection.retraction),
            "post-transfer reflection changed order or retraction metadata");
         check(reflection.transportation != null
               && !acquirer.getTransportationTypeName(reflection.transportation).isEmpty(),
            "post-transfer reflection returned an unknown transportation type");
         check(acquirerTarget.equals(acquirerRecorder.grants.get(0))
               && ownerTarget.equals(ownerRecorder.grants.get(0)),
            "ownership transfer returned incorrect time-advance grants");
         check(acquirerRecorder.callbackOrder.size() == 2
               && "reflectAttributeValues".equals(acquirerRecorder.callbackOrder.get(0))
               && "timeAdvanceGrant".equals(acquirerRecorder.callbackOrder.get(1)),
            "timestamped reflection was not delivered before its matching grant");

         acquirer.disableTimeConstrained();
         owner.disableTimeRegulation();
         acquirer.unsubscribeObjectClassAttributes(acquirerClass, acquirerAttributes);
         subscribed = false;
         acquirer.unpublishObjectClassAttributes(acquirerClass, acquirerAttributes);
         acquirerPublished = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = false;
      } finally {
         if (subscribed && acquirerJoined) try {
            acquirer.unsubscribeObjectClassAttributes(
               acquirer.getObjectClassHandle(objectClassName),
               JavaTckSupport.attributeSet(acquirer,
                  acquirer.getAttributeHandle(acquirer.getObjectClassHandle(objectClassName),
                     attributeName)));
         } catch (Exception ignored) { }
         if (acquirerPublished && acquirerJoined) try {
            acquirer.unpublishObjectClassAttributes(
               acquirer.getObjectClassHandle(objectClassName),
               JavaTckSupport.attributeSet(acquirer,
                  acquirer.getAttributeHandle(acquirer.getObjectClassHandle(objectClassName),
                     attributeName)));
         } catch (Exception ignored) { }
         if (ownerPublished && ownerJoined) try {
            owner.unpublishObjectClassAttributes(
               owner.getObjectClassHandle(objectClassName),
               JavaTckSupport.attributeSet(owner,
                  owner.getAttributeHandle(owner.getObjectClassHandle(objectClassName),
                     attributeName)));
         } catch (Exception ignored) { }
         if (acquirerJoined) try {
            acquirer.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (acquirerConnected) try { acquirer.disconnect(); }
            catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static void assertOwnershipEvent(OwnershipEvent event, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] tag, String description) {
      check(object.equals(event.object) && event.attributes.size() == 1
            && event.attributes.contains(attribute) && Arrays.equals(tag, event.tag),
         description + " returned incorrect object, attributes, or tag");
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) {
         result = (LogicalTimeInterval) ((LogicalTimeInterval) result).add(epsilon);
      }
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> offset(LogicalTime<?, ?> start,
         LogicalTimeInterval epsilon, int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static void await(ReadyCheck ready, RTIambassador ambassador, String description)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         JavaTckSupport.drain(ambassador);
         if (ready.isReady()) return;
         Thread.sleep(1L);
      }
      throw new AssertionError(description + " timed out");
   }

   private static void check(boolean condition, String message) {
      if (!condition) throw new AssertionError(message);
   }

   @FunctionalInterface
   private interface ReadyCheck { boolean isReady(); }

   private static final class OwnershipEvent {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final byte[] tag;
      private OwnershipEvent(ObjectInstanceHandle object, AttributeHandleSet attributes,
            byte[] tag) {
         this.object = object;
         this.attributes = attributes;
         this.tag = tag;
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;
      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values, byte[] tag,
            TransportationTypeHandle transportation, FederateHandle producer,
            LogicalTime<?, ?> time, OrderType sentOrder, OrderType receivedOrder,
            MessageRetractionHandle retraction) {
         this.object = object;
         this.values = values;
         this.tag = tag;
         this.transportation = transportation;
         this.producer = producer;
         this.time = time;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
         this.retraction = retraction;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discovered = new ArrayList<>();
      private final List<OwnershipEvent> assumptions = new ArrayList<>();
      private final List<OwnershipEvent> acquisitions = new ArrayList<>();
      private final List<Reflection> reflections = new ArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discovered.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("requestAttributeOwnershipAssumption".equals(name)
                     && arguments != null && arguments.length == 3) {
                  assumptions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], ((byte[]) arguments[2]).clone()));
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length == 3) {
                  acquisitions.add(new OwnershipEvent((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleSet) arguments[1], ((byte[]) arguments[2]).clone()));
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     ((AttributeHandleValueMap) arguments[1]).clone(),
                     ((byte[]) arguments[2]).clone(), (TransportationTypeHandle) arguments[3],
                     (FederateHandle) arguments[4], (LogicalTime<?, ?>) arguments[6],
                     (OrderType) arguments[7], (OrderType) arguments[8],
                     (MessageRetractionHandle) arguments[9]));
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK attribute-ownership callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
