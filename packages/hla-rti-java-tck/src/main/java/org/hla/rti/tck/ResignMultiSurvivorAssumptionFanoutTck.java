package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Each eligible survivor retains its own queued ownership-assumption offer after resignation. */
final class ResignMultiSurvivorAssumptionFanoutTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private ResignMultiSurvivorAssumptionFanoutTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "independent callback-queue observation requires evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder firstRecorder = new Recorder();
      Recorder secondRecorder = new Recorder();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-resign-assumption-fanout-" + suffix;
      boolean[] connected = new boolean[3], joined = new boolean[3];
      boolean created = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         connected[0] = true;
         first.connect(firstRecorder.proxy(), callbackModel);
         connected[1] = true;
         second.connect(secondRecorder.proxy(), callbackModel);
         connected[2] = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-fanout-owner-" + suffix, federation);
         joined[0] = true;
         first.joinFederationExecution("java-tck-fanout-first-" + suffix, federation);
         joined[1] = true;
         second.joinFederationExecution("java-tck-fanout-second-" + suffix, federation);
         joined[2] = true;

         ObjectClassHandle ownerClass = owner.getObjectClassHandle(objectClassName);
         ObjectClassHandle firstClass = first.getObjectClassHandle(objectClassName);
         ObjectClassHandle secondClass = second.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle firstAttribute = first.getAttributeHandle(firstClass, attributeName);
         AttributeHandle secondAttribute = second.getAttributeHandle(secondClass, attributeName);
         AttributeHandle firstPrivilege = first.getAttributeHandle(
            firstClass, "HLAprivilegeToDeleteObject");
         AttributeHandle secondPrivilege = second.getAttributeHandle(
            secondClass, "HLAprivilegeToDeleteObject");
         JavaTckSupport.check(ownerClass != null && firstClass != null && secondClass != null
               && ownerAttribute != null && firstAttribute != null
               && secondAttribute != null && firstPrivilege != null
               && secondPrivilege != null
               && objectClassName.equals(owner.getObjectClassName(ownerClass))
               && objectClassName.equals(first.getObjectClassName(firstClass))
               && objectClassName.equals(second.getObjectClassName(secondClass))
               && attributeName.equals(owner.getAttributeName(ownerClass, ownerAttribute))
               && attributeName.equals(first.getAttributeName(firstClass, firstAttribute))
               && attributeName.equals(second.getAttributeName(secondClass, secondAttribute)),
            "adapter FOM did not resolve the object attribute and standard delete privilege");
         AttributeHandleSet ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         AttributeHandleSet firstAttributes = JavaTckSupport.attributeSet(first, firstAttribute);
         AttributeHandleSet secondAttributes = JavaTckSupport.attributeSet(
            second, secondAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         first.subscribeObjectClassAttributes(firstClass, firstAttributes);
         second.subscribeObjectClassAttributes(secondClass, secondAttributes);
         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null, "registration returned no object");
         String objectName = owner.getObjectInstanceName(object);
         await(first, () -> firstRecorder.discoveries.size() == 1,
            "first survivor discovery");
         ObjectInstanceHandle firstObject = first.getObjectInstanceHandle(objectName);
         JavaTckSupport.check(firstObject.equals(firstRecorder.discoveries.get(0)),
            "first survivor discovery did not resolve to its local object handle");
         await(second, () -> secondRecorder.discoveries.size() == 1,
            "second survivor discovery");
         ObjectInstanceHandle secondObject = second.getObjectInstanceHandle(objectName);
         JavaTckSupport.check(secondObject.equals(secondRecorder.discoveries.get(0)),
            "second survivor discovery did not resolve to its local object handle");
         first.publishObjectClassAttributes(firstClass, firstAttributes);
         second.publishObjectClassAttributes(secondClass, secondAttributes);

         owner.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         joined[0] = false;
         JavaTckSupport.check(firstRecorder.assumptions.isEmpty()
               && secondRecorder.assumptions.isEmpty(),
            "evoked assumption arrived before either survivor was serviced");
         await(first, () -> firstRecorder.assumptions.size() == 1,
            "first resignation assumption");
         Set<AttributeHandle> firstExpected = new HashSet<>(
            JavaTckSupport.attributeSet(first, firstAttribute, firstPrivilege));
         JavaTckSupport.check(firstRecorder.assumptions.size() == 1
               && firstRecorder.assumptions.get(0).matches(firstObject, firstExpected)
               && secondRecorder.assumptions.isEmpty(),
            "first survivor did not receive its independent assumption offer");
         await(second, () -> secondRecorder.assumptions.size() == 1,
            "second resignation assumption");
         Set<AttributeHandle> secondExpected = new HashSet<>(
            JavaTckSupport.attributeSet(second, secondAttribute, secondPrivilege));
         JavaTckSupport.check(secondRecorder.assumptions.size() == 1
               && secondRecorder.assumptions.get(0).matches(secondObject, secondExpected)
               && firstRecorder.assumptions.size() == 1,
            "second survivor did not receive its independent assumption offer");

         first.resignFederationExecution(ResignAction.NO_ACTION);
         joined[1] = false;
         second.resignFederationExecution(ResignAction.NO_ACTION);
         joined[2] = false;
         second.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined[2]) resign(second);
         if (joined[1]) resign(first);
         if (joined[0]) resign(owner);
         if (created) {
            try { second.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected[2]) { try { second.disconnect(); } catch (Exception ignored) { } }
         if (connected[1]) { try { first.disconnect(); } catch (Exception ignored) { } }
         if (connected[0]) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
      catch (Exception ignored) { }
   }

   private static void await(RTIambassador ambassador, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         ambassador.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Assumption {
      private final ObjectInstanceHandle object;
      private final Set<AttributeHandle> attributes;
      private final byte[] tag;

      private Assumption(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         attributes = new HashSet<>((AttributeHandleSet) arguments[1]);
         tag = ((byte[]) arguments[2]).clone();
      }

      private boolean matches(ObjectInstanceHandle expectedObject,
            Set<AttributeHandle> expectedAttributes) {
         return expectedObject.equals(object) && expectedAttributes.equals(attributes)
            && tag.length == 0;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Assumption> assumptions = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("requestAttributeOwnershipAssumption".equals(name)
                     && arguments != null) {
                  assumptions.add(new Assumption(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK resignation assumption callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
