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
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks passel integrity and suppression of a retracted queued timestamped update. */
final class TimestampedAttributeQueuedPasselRetractionTck {
   private TimestampedAttributeQueuedPasselRetractionTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-timestamped-attribute-queued-passel-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean created = false;
      boolean regulating = false;
      boolean constrained = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeHandleSet receiverAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-queued-passel-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-queued-passel-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherFirst = publisher.getAttributeHandle(
            publisherClass, firstAttributeName);
         AttributeHandle publisherSecond = publisher.getAttributeHandle(
            publisherClass, secondAttributeName);
         AttributeHandle receiverFirst = receiver.getAttributeHandle(
            receiverClass, firstAttributeName);
         AttributeHandle receiverSecond = receiver.getAttributeHandle(
            receiverClass, secondAttributeName);
         JavaTckSupport.check(publisherFirst != null && publisherSecond != null
               && receiverFirst != null && receiverSecond != null
               && !publisherFirst.equals(publisherSecond),
            "adapter multi-attribute FOM did not provide two distinct attribute handles");
         publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherFirst, publisherSecond);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverFirst, receiverSecond);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         awaitBoth(publisher, receiver, callbackModel,
            () -> object.equals(receiverRecorder.discoveredObject),
            "receiver object discovery");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "timestamped queued-passel members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "timestamped queued-passel members began at different logical times");
         LogicalTime lookaheadTarget = offset(publisherInitial, epsilon, 5);
         LogicalTimeInterval lookahead = (LogicalTimeInterval) lookaheadTarget.distance(publisherInitial);

         receiver.enableTimeConstrained();
         constrained = true;
         awaitBoth(publisher, receiver, callbackModel,
            () -> receiverRecorder.timeConstrainedEnabled,
            "receiver time-constrained callback");
         publisher.enableTimeRegulation(lookahead);
         regulating = true;
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherRecorder.timeRegulationEnabled,
            "publisher time-regulation callback");

         LogicalTime invalidUpdateTime = offset(publisherInitial, epsilon, 4);
         LogicalTime firstUpdateTime = offset(publisherInitial, epsilon, 6);
         LogicalTime secondUpdateTime = offset(publisherInitial, epsilon, 12);
         byte[] firstValue = new byte[] {0x11, 0x22};
         byte[] secondValue = new byte[] {0x33, 0x44, 0x55};
         byte[] tag = new byte[] {(byte) 0xa4, 0x15};
         AttributeHandleValueMap passel = publisher.getAttributeHandleValueMapFactory().create(2);
         passel.put(publisherFirst, firstValue);
         passel.put(publisherSecond, secondValue);
         JavaTckSupport.expectFailure(
            () -> publisher.updateAttributeValues(object, passel, tag, invalidUpdateTime),
            "InvalidLogicalTime", "timestamped passel update below producer lookahead");

         MessageRetractionReturn first = publisher.updateAttributeValues(
            object, passel, tag, firstUpdateTime);
         checkRetraction(first, "first queued passel");
         JavaTckSupport.check(receiverRecorder.reflections.isEmpty(),
            "first queued passel reflected before its receiver advanced");
         receiver.timeAdvanceRequest(firstUpdateTime);
         publisher.retract(first.handle);
         publisher.timeAdvanceRequest(firstUpdateTime);
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherRecorder.timeAdvanceGrantTimes.size() == 1
                  && receiverRecorder.timeAdvanceGrantTimes.size() == 1,
            "grant after retracting first passel");
         JavaTckSupport.check(receiverRecorder.reflections.isEmpty()
               && receiverRecorder.retractionCallbacks.isEmpty(),
            "retracted queued passel leaked a reflection or retraction callback");

         MessageRetractionReturn second = publisher.updateAttributeValues(
            object, passel, tag, secondUpdateTime);
         checkRetraction(second, "second queued passel");
         receiver.timeAdvanceRequest(secondUpdateTime);
         publisher.timeAdvanceRequest(secondUpdateTime);
         awaitBoth(publisher, receiver, callbackModel,
            () -> receiverRecorder.reflections.size() >= 1
                  && receiverRecorder.timeAdvanceGrantTimes.size() == 2
                  && publisherRecorder.timeAdvanceGrantTimes.size() == 2,
            "second passel delivery and grants");

         List<Reflection> reflections = receiverRecorder.reflections;
         JavaTckSupport.check(!reflections.isEmpty(),
            "second queued passel returned no timestamped reflections");
         List<String> callbackOrder = receiverRecorder.callbackOrder;
         JavaTckSupport.check(callbackOrder.size() >= 3
               && "timeAdvanceGrant".equals(callbackOrder.get(0))
               && "timeAdvanceGrant".equals(callbackOrder.get(callbackOrder.size() - 1))
               && callbackOrder.contains("reflectAttributeValues"),
            "queued passel returned the wrong callback ordering");

         boolean firstReported = false;
         boolean secondReported = false;
         for (Reflection reflection : reflections) {
            JavaTckSupport.check(object.equals(reflection.object)
                  && reflection.values != null && !reflection.values.isEmpty()
                  && Arrays.equals(tag, reflection.tag)
                  && producer.equals(reflection.producer)
                  && secondUpdateTime.equals(reflection.time)
                  && reflection.sentOrder == OrderType.TIMESTAMP
                  && reflection.receivedOrder == OrderType.TIMESTAMP
                  && second.handle.equals(reflection.retraction)
                  && reflection.transportation != null
                  && (reflection.regions == null || reflection.regions.isEmpty()),
               "queued passel reflection returned incorrect standard metadata");
            JavaTckSupport.check(!receiver.getTransportationTypeName(
                  reflection.transportation).isEmpty(),
               "queued passel reflection transportation handle did not resolve");
            if (reflection.values.containsKey(receiverFirst)) {
               JavaTckSupport.check(!firstReported
                     && Arrays.equals(firstValue, reflection.values.get(receiverFirst)),
                  "queued passel first attribute was duplicated or had the wrong value");
               firstReported = true;
            }
            if (reflection.values.containsKey(receiverSecond)) {
               JavaTckSupport.check(!secondReported
                     && Arrays.equals(secondValue, reflection.values.get(receiverSecond)),
                  "queued passel second attribute was duplicated or had the wrong value");
               secondReported = true;
            }
            JavaTckSupport.check(reflection.values.containsKey(receiverFirst)
                  || reflection.values.containsKey(receiverSecond),
               "queued passel reflection contained an unknown attribute");
         }
         JavaTckSupport.check(firstReported && secondReported,
            "queued passel did not deliver both attribute values");
         JavaTckSupport.expectFailure(() -> publisher.retract(second.handle),
            "MessageCanNoLongerBeRetracted", "retracting the delivered queued passel");
      } finally {
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runMixedOrder(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String timestampedAttributeName,
         String receiveOrderAttributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-mixed-order-passel-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean created = false;
      boolean regulating = false;
      boolean constrained = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeHandleSet receiverAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-mixed-order-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-mixed-order-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherTimestamped = publisher.getAttributeHandle(
            publisherClass, timestampedAttributeName);
         AttributeHandle publisherReceiveOrder = publisher.getAttributeHandle(
            publisherClass, receiveOrderAttributeName);
         AttributeHandle receiverTimestamped = receiver.getAttributeHandle(
            receiverClass, timestampedAttributeName);
         AttributeHandle receiverReceiveOrder = receiver.getAttributeHandle(
            receiverClass, receiveOrderAttributeName);
         JavaTckSupport.check(publisherTimestamped != null && publisherReceiveOrder != null
               && receiverTimestamped != null && receiverReceiveOrder != null
               && !publisherTimestamped.equals(publisherReceiveOrder),
            "adapter multi-attribute FOM did not provide distinct mixed-order attributes");
         publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherTimestamped, publisherReceiveOrder);
         receiverAttributes = JavaTckSupport.attributeSet(
            receiver, receiverTimestamped, receiverReceiveOrder);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;
         publisher.changeDefaultAttributeOrderType(
            publisherClass,
            JavaTckSupport.attributeSet(publisher, publisherTimestamped),
            OrderType.TIMESTAMP);

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         awaitBoth(publisher, receiver, callbackModel,
            () -> object.equals(receiverRecorder.discoveredObject), "receiver discovery");
         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "mixed-order members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "mixed-order members began at different logical times");
         LogicalTime lookaheadTarget = offset(publisherInitial, epsilon, 5);
         LogicalTimeInterval lookahead = (LogicalTimeInterval) lookaheadTarget.distance(
            publisherInitial);

         receiver.enableTimeConstrained();
         constrained = true;
         awaitBoth(publisher, receiver, callbackModel,
            () -> receiverRecorder.timeConstrainedEnabled, "receiver time-constrained callback");
         receiver.enableAsynchronousDelivery();
         publisher.enableTimeRegulation(lookahead);
         regulating = true;
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherRecorder.timeRegulationEnabled, "publisher time-regulation callback");

         LogicalTime updateTime = offset(publisherInitial, epsilon, 6);
         LogicalTime receiverTarget = offset(receiverInitial, epsilon, 6);
         LogicalTime publisherTarget = offset(publisherInitial, epsilon, 2);
         byte[] timestampedValue = new byte[] {0x51, 0x52};
         byte[] receiveOrderValue = new byte[] {0x61, 0x62, 0x63};
         byte[] tag = new byte[] {0x4d, 0x49, 0x58};
         AttributeHandleValueMap passel = publisher.getAttributeHandleValueMapFactory().create(2);
         passel.put(publisherTimestamped, timestampedValue);
         passel.put(publisherReceiveOrder, receiveOrderValue);
         MessageRetractionReturn retraction = publisher.updateAttributeValues(
            object, passel, tag, updateTime);
         checkRetraction(retraction, "mixed-order passel");
         awaitBoth(publisher, receiver, callbackModel,
            () -> receiverRecorder.reflections.size() == 1,
            "receive-order part of mixed-order passel");
         Reflection receiveOrder = receiverRecorder.reflections.get(0);
         JavaTckSupport.check(object.equals(receiveOrder.object)
               && receiveOrder.values != null && receiveOrder.values.size() == 1
               && Arrays.equals(receiveOrderValue, receiveOrder.values.get(receiverReceiveOrder))
               && receiveOrder.receivedOrder == OrderType.RECEIVE
               && receiveOrder.retraction == null,
            "mixed-order passel did not report only its receive-order attribute with receive-order metadata before retraction");

         publisher.retract(retraction.handle);
         receiver.timeAdvanceRequest(receiverTarget);
         publisher.timeAdvanceRequest(publisherTarget);
         awaitBoth(publisher, receiver, callbackModel,
            () -> receiverRecorder.timeAdvanceGrantTimes.size() == 1
                  && publisherRecorder.timeAdvanceGrantTimes.size() == 1,
            "grants after retracting queued timestamped part");
         JavaTckSupport.check(receiverRecorder.reflections.size() == 1
               && receiverRecorder.timeAdvanceGrantTimes.size() == 1,
            "mixed-order retraction changed the delivered receive-order result or released the queued timestamped attribute");
      } finally {
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runSenderRegulationTransition(RtiFactory factory, String fom,
         String timeImplementation, String objectClassName, String attributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-timestamped-sender-role-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean created = false;
      boolean regulating = false;
      boolean constrained = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandle publisherAttribute = null;
      AttributeHandle receiverAttribute = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeHandleSet receiverAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-sender-role-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-sender-role-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         JavaTckSupport.check(publisherAttribute != null && receiverAttribute != null,
            "adapter FOM did not provide the sender-transition attribute");
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         awaitBoth(publisher, receiver, callbackModel,
            () -> object.equals(receiverRecorder.discoveredObject), "receiver discovery");
         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "sender-transition members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "sender-transition members began at different logical times");
         LogicalTime updateTime = offset(publisherInitial, epsilon, 6);
         LogicalTime receiverTarget = offset(receiverInitial, epsilon, 6);
         LogicalTime publisherTarget = offset(publisherInitial, epsilon, 2);
         LogicalTime lookaheadTarget = offset(publisherInitial, epsilon, 5);
         LogicalTimeInterval lookahead = (LogicalTimeInterval) lookaheadTarget.distance(
            publisherInitial);

         receiver.enableTimeConstrained();
         constrained = true;
         awaitBoth(publisher, receiver, callbackModel,
            () -> receiverRecorder.timeConstrainedEnabled, "receiver time-constrained callback");
         receiver.enableAsynchronousDelivery();

         byte[] firstValue = new byte[] {0x41, 0x42};
         byte[] firstTag = new byte[] {0x31};
         AttributeHandleValueMap firstValues =
            publisher.getAttributeHandleValueMapFactory().create(1);
         firstValues.put(publisherAttribute, firstValue);
         MessageRetractionReturn firstRetraction = publisher.updateAttributeValues(
            object, firstValues, firstTag, updateTime);
         JavaTckSupport.check(firstRetraction != null && !firstRetraction.retractionHandleIsValid,
            "timestamped send before Time Regulation returned a valid retraction handle");
         awaitBoth(publisher, receiver, callbackModel,
            () -> receiverRecorder.reflections.size() == 1,
            "pre-regulation receive-order reflection");
         Reflection first = receiverRecorder.reflections.get(0);
         JavaTckSupport.check(object.equals(first.object) && first.values != null
               && first.values.size() == 1
               && Arrays.equals(firstValue, first.values.get(receiverAttribute))
               && producer.equals(first.producer)
               && Arrays.equals(firstTag, first.tag)
               && updateTime.equals(first.time)
               && first.sentOrder == OrderType.RECEIVE
               && first.receivedOrder == OrderType.RECEIVE
               && first.retraction == null
               && receiverRecorder.timeAdvanceGrantTimes.isEmpty(),
            "timestamped send before Time Regulation did not arrive as receive-order");

         publisher.enableTimeRegulation(lookahead);
         regulating = true;
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherRecorder.timeRegulationEnabled,
            "publisher time-regulation callback");
         byte[] secondValue = new byte[] {0x54, 0x53, 0x4f};
         byte[] secondTag = new byte[] {0x32};
         AttributeHandleValueMap secondValues =
            publisher.getAttributeHandleValueMapFactory().create(1);
         secondValues.put(publisherAttribute, secondValue);
         MessageRetractionReturn secondRetraction = publisher.updateAttributeValues(
            object, secondValues, secondTag, updateTime);
         checkRetraction(secondRetraction, "post-regulation timestamped update");
         JavaTckSupport.check(receiverRecorder.reflections.size() == 1
               && receiverRecorder.timeAdvanceGrantTimes.isEmpty(),
            "timestamped send after Time Regulation was delivered before receiver advancement");

         receiver.timeAdvanceRequest(receiverTarget);
         publisher.timeAdvanceRequest(publisherTarget);
         awaitBoth(publisher, receiver, callbackModel,
            () -> receiverRecorder.reflections.size() == 2
                  && receiverRecorder.timeAdvanceGrantTimes.size() == 1
                  && publisherRecorder.timeAdvanceGrantTimes.size() == 1,
            "post-regulation timestamped reflection and grants");
         Reflection second = receiverRecorder.reflections.get(1);
         JavaTckSupport.check(object.equals(second.object) && second.values != null
               && second.values.size() == 1
               && Arrays.equals(secondValue, second.values.get(receiverAttribute))
               && producer.equals(second.producer)
               && Arrays.equals(secondTag, second.tag)
               && updateTime.equals(second.time)
               && second.sentOrder == OrderType.TIMESTAMP
               && second.receivedOrder == OrderType.TIMESTAMP
               && secondRetraction.handle.equals(second.retraction)
               && receiverTarget.equals(receiverRecorder.timeAdvanceGrantTimes.get(0)),
            "timestamped send after Time Regulation returned incorrect TSO metadata");
         JavaTckSupport.check(receiverRecorder.callbackOrder.size() >= 3
               && "reflectAttributeValues".equals(receiverRecorder.callbackOrder.get(0))
               && "reflectAttributeValues".equals(receiverRecorder.callbackOrder.get(1))
               && "timeAdvanceGrant".equals(receiverRecorder.callbackOrder.get(2)),
            "sender-role transition returned the wrong reflection/grant callback order");
      } finally {
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void checkRetraction(MessageRetractionReturn result, String description) {
      JavaTckSupport.check(result != null && result.retractionHandleIsValid
            && result.handle != null, description + " returned no valid retraction handle");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void awaitBoth(RTIambassador publisher, RTIambassador receiver,
         CallbackModel callbackModel, BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(publisher);
            JavaTckSupport.drain(receiver);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private RegionHandleSet regions;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;
      private volatile ObjectInstanceHandle discoveredObject;
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<MessageRetractionHandle> retractionCallbacks = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 10) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) arguments[0];
                  reflection.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  reflection.tag = ((byte[]) arguments[2]).clone();
                  reflection.transportation = (TransportationTypeHandle) arguments[3];
                  reflection.producer = (FederateHandle) arguments[4];
                  reflection.regions = (RegionHandleSet) arguments[5];
                  reflection.time = (LogicalTime<?, ?>) arguments[6];
                  reflection.sentOrder = (OrderType) arguments[7];
                  reflection.receivedOrder = (OrderType) arguments[8];
                  reflection.retraction = (MessageRetractionHandle) arguments[9];
                  reflections.add(reflection);
                  callbackOrder.add(name);
               } else if ("requestRetraction".equals(name)
                     && arguments != null && arguments.length == 1) {
                  retractionCallbacks.add((MessageRetractionHandle) arguments[0]);
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK queued-passel callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
