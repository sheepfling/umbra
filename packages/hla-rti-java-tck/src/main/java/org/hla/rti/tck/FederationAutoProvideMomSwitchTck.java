package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.encoding.EncoderFactory;
import hla.rti1516_2025.encoding.HLAinteger32BE;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies the standard federation-MOM HLAautoProvide switch controls solicitation. */
final class FederationAutoProvideMomSwitchTck {
   private static final long TIMEOUT_NANOS = 10_000_000_000L;
   private static final String SET_SWITCHES =
      "HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches";
   private FederationAutoProvideMomSwitchTck() { }

   static void run(RtiFactory factory, String fom, String mim, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      EncoderFactory encoder = factory.getEncoderFactory();
      Recorder ownerCallbacks = new Recorder(false);
      Recorder requesterCallbacks = new Recorder(true);
      String federation = "java-tck-federation-auto-provide-mom-" + UUID.randomUUID();
      boolean ownerConnected = false, requesterConnected = false, created = false;
      boolean ownerJoined = false, requesterJoined = false, published = false, subscribed = false;
      ObjectClassHandle ownerClass = null, requesterClass = null;
      AttributeHandle ownerAttribute = null;
      AttributeHandleSet ownerAttributes = null, requesterAttributes = null;
      ObjectInstanceHandle disabledObject = null, enabledObject = null, disabledAgainObject = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel); ownerConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel); requesterConnected = true;
         owner.createFederationExecutionWithMIM(federation, new String[] {fom}, mim,
            timeImplementation); created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "federation-auto-provide-mom-owner", federation); ownerJoined = true;
         requester.joinFederationExecution("federation-auto-provide-mom-requester", federation);
         requesterJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         requesterClass = requester.getObjectClassHandle(objectClassName);
         ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle requesterAttribute = requester.getAttributeHandle(requesterClass,
            attributeName);
         InteractionClassHandle ownerSetSwitches = owner.getInteractionClassHandle(SET_SWITCHES);
         InteractionClassHandle requesterSetSwitches = requester.getInteractionClassHandle(SET_SWITCHES);
         ParameterHandle ownerAutoProvide = owner.getParameterHandle(ownerSetSwitches, "HLAautoProvide");
         ParameterHandle requesterAutoProvide = requester.getParameterHandle(
            requesterSetSwitches, "HLAautoProvide");
         JavaTckSupport.check(ownerClass.equals(requesterClass)
               && ownerAttribute.equals(requesterAttribute)
               && ownerSetSwitches.equals(requesterSetSwitches)
               && ownerAutoProvide.equals(requesterAutoProvide),
            "standard application or federation-MOM lookup returned inconsistent handles");

         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         requesterAttributes = JavaTckSupport.attributeSet(requester, requesterAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes); published = true;
         requester.subscribeObjectClassAttributes(requesterClass, requesterAttributes, "");
         subscribed = true;

         sendSwitch(requester, requesterSetSwitches, requesterAutoProvide, false, encoder);
         disabledObject = registerAndObserveDiscovery(owner, requester, ownerClass,
            requesterClass, producer, requesterCallbacks, callbackModel,
            "federation MOM Auto Provide disabled discovery");
         drainOwnerCallbacks(owner, callbackModel);
         JavaTckSupport.check(ownerCallbacks.provided.isEmpty(),
            "federation MOM Auto Provide false still solicited the provider");

         sendSwitch(requester, requesterSetSwitches, requesterAutoProvide, true, encoder);
         enabledObject = registerAndObserveDiscovery(owner, requester, ownerClass,
            requesterClass, producer, requesterCallbacks, callbackModel,
            "federation MOM Auto Provide enabled discovery");
         ObjectInstanceHandle enabledObjectHandle = enabledObject;
         await(owner, requester, callbackModel,
            () -> requesterCallbacks.discoveries.stream()
               .anyMatch(discovery -> enabledObjectHandle.equals(discovery.object))
               && ownerCallbacks.provided.size() == 1,
            "federation MOM Auto Provide provider request");
         JavaTckSupport.check(ownerCallbacks.provided.size() == 1,
            "federation MOM Auto Provide true did not produce exactly one request");
         verifyRequest(ownerCallbacks.provided.get(0), enabledObject, ownerAttribute);

         sendSwitch(owner, ownerSetSwitches, ownerAutoProvide, false, encoder);
         disabledAgainObject = registerAndObserveDiscovery(owner, requester, ownerClass,
            requesterClass, producer, requesterCallbacks, callbackModel,
            "federation MOM Auto Provide disabled-again discovery");
         drainOwnerCallbacks(owner, callbackModel);
         JavaTckSupport.check(ownerCallbacks.provided.size() == 1,
            "federation MOM Auto Provide false generated an additional provider request");
         JavaTckSupport.check(!disabledObject.equals(enabledObject)
               && !enabledObject.equals(disabledAgainObject)
               && !disabledObject.equals(disabledAgainObject),
            "the federation-wide switch observations reused an object instance");

         requester.unsubscribeObjectClassAttributes(requesterClass, requesterAttributes);
         subscribed = false;
         owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); published = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION); requesterJoined = false;
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;
         owner.destroyFederationExecution(federation); created = false;
      } finally {
         if (requesterJoined && subscribed && requesterClass != null && requesterAttributes != null) try {
            requester.unsubscribeObjectClassAttributes(requesterClass, requesterAttributes);
         } catch (Exception ignored) { }
         if (published && ownerJoined && ownerClass != null && ownerAttributes != null) try {
            owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes);
         } catch (Exception ignored) { }
         if (requesterJoined) try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (ownerJoined) try {
            owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (requesterConnected) try { requester.disconnect(); } catch (Exception ignored) { }
         if (ownerConnected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static void sendSwitch(RTIambassador setter, InteractionClassHandle interaction,
         ParameterHandle parameter, boolean enabled, EncoderFactory encoder) throws Exception {
      HLAinteger32BE value = encoder.createHLAinteger32BE(enabled ? 1 : 0);
      ParameterHandleValueMap parameters = setter.getParameterHandleValueMapFactory().create(1);
      parameters.put(parameter, value.toByteArray());
      setter.sendInteraction(interaction, parameters, new byte[0]);
   }

   private static ObjectInstanceHandle registerAndObserveDiscovery(RTIambassador owner,
         RTIambassador requester, ObjectClassHandle ownerClass, ObjectClassHandle requesterClass,
         FederateHandle producer, Recorder recorder, CallbackModel model, String description)
         throws Exception {
      recorder.discoveries.clear();
      ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
      JavaTckSupport.check(object != null && object.encodedLength() > 0,
         description + " returned an invalid object handle");
      await(owner, requester, model,
         () -> recorder.discoveries.stream().anyMatch(discovery -> object.equals(discovery.object)),
         description);
      Discovery discovery = recorder.discoveries.stream()
         .filter(candidate -> object.equals(candidate.object)).findFirst().orElse(null);
      JavaTckSupport.check(discovery != null && requesterClass.equals(discovery.objectClass)
            && producer.equals(discovery.producer),
         description + " delivered incorrect standard discovery data");
      return object;
   }

   private static void verifyRequest(ProvidedUpdate request, ObjectInstanceHandle object,
         AttributeHandle attribute) {
      JavaTckSupport.check(object.equals(request.object) && request.attributes.size() == 1
            && request.attributes.contains(attribute) && request.tag.length == 0,
         "federation MOM Auto Provide request returned incorrect object, attributes, or tag");
   }

   private static void await(RTIambassador owner, RTIambassador requester, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         pump(owner, model); pump(requester, model); Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void pump(RTIambassador ambassador, CallbackModel model) throws Exception {
      if (model == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
      else Thread.yield();
   }

   private static void drainOwnerCallbacks(RTIambassador owner, CallbackModel model)
         throws Exception {
      for (int i = 0; i < 4; ++i) pump(owner, model);
   }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
      private FederateHandle producer;
   }

   private static final class ProvidedUpdate {
      private ObjectInstanceHandle object;
      private AttributeHandleSet attributes;
      private byte[] tag;
   }

   private static final class Recorder {
      private final boolean requester;
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<ProvidedUpdate> provided = new CopyOnWriteArrayList<>();
      private Recorder(boolean requester) { this.requester = requester; }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && requester && args != null
                     && args.length >= 3) {
                  Discovery discovery = new Discovery();
                  discovery.object = (ObjectInstanceHandle) args[0];
                  discovery.objectClass = (ObjectClassHandle) args[1];
                  discovery.producer = (FederateHandle) args[2];
                  discoveries.add(discovery);
               } else if ("provideAttributeValueUpdate".equals(name) && !requester
                     && args != null && args.length == 3) {
                  ProvidedUpdate update = new ProvidedUpdate();
                  update.object = (ObjectInstanceHandle) args[0];
                  update.attributes = (AttributeHandleSet) ((AttributeHandleSet) args[1]).clone();
                  update.tag = ((byte[]) args[2]).clone();
                  provided.add(update);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK federation MOM Auto Provide callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (args == null ? null : args[0]);
               }
               return null;
            });
      }
   }
}
