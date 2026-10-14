package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Standard object-class lookup boundaries and FOM-module composition. */
final class ObjectClassLookupLifecycleTck {
   private ObjectClassLookupLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String additionalFom,
         String timeImplementation, String baseObjectName, String extensionObjectName,
         CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(baseObjectName != null && !baseObjectName.trim().isEmpty()
            && extensionObjectName != null && !extensionObjectName.trim().isEmpty(),
         "adapter must provide the base and additional-module object-class names");
      JavaTckSupport.check(!baseObjectName.equals(extensionObjectName),
         "additional-module object class must differ from the base object class");
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador extension = factory.getRtiAmbassador();
      String federation = "java-tck-object-class-lookup-" + UUID.randomUUID();
      boolean observerConnected = false;
      boolean ownerConnected = false;
      boolean extensionConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean extensionJoined = false;
      try {
         JavaTckSupport.expectFailure(
            () -> observer.getObjectClassHandle(baseObjectName), "NotConnected",
            "looking up an object class before connecting");
         JavaTckSupport.expectFailure(
            () -> observer.getObjectClassName(null), "NotConnected",
            "looking up an object-class name before connecting");

         observer.connect(callbacks(), callbackModel);
         observerConnected = true;
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         extension.connect(callbacks(), callbackModel);
         extensionConnected = true;

         JavaTckSupport.expectFailure(
            () -> observer.getObjectClassHandle(baseObjectName),
            "FederateNotExecutionMember",
            "looking up an object class after connecting but before joining");
         JavaTckSupport.expectFailure(
            () -> observer.getObjectClassName(null), "FederateNotExecutionMember",
            "looking up an object-class name after connecting but before joining");

         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-object-class-lookup-owner", federation);
         ownerJoined = true;

         ObjectClassHandle base = owner.getObjectClassHandle(baseObjectName);
         JavaTckSupport.check(base != null
               && base.equals(owner.getObjectClassHandle(baseObjectName))
               && baseObjectName.equals(owner.getObjectClassName(base)),
            "ordinary object-class lookup did not round-trip a stable base handle");
         String unknownObjectClassName = "HLAobjectRoot.TckMissingObject"
            + UUID.randomUUID().toString().replace("-", "");
         JavaTckSupport.expectFailure(
            () -> owner.getObjectClassHandle(unknownObjectClassName), "NameNotFound",
            "looking up an unknown object class");
         // Java exposes no public invalid-handle constructor; null is its portable invalid input.
         JavaTckSupport.expectFailure(
            () -> owner.getObjectClassName(null), "InvalidObjectClassHandle",
            "looking up an object-class name from an invalid handle");
         JavaTckSupport.expectFailure(
            () -> owner.getObjectClassHandle(extensionObjectName), "NameNotFound",
            "looking up the extension object class before an additional-module join");

         extension.joinFederationExecution("java-tck-object-class-lookup-extension",
            "java-tck-object-class-lookup-extension", federation,
            new String[] {additionalFom});
         extensionJoined = true;
         ObjectClassHandle ownerExtension = owner.getObjectClassHandle(extensionObjectName);
         ObjectClassHandle memberExtension = extension.getObjectClassHandle(extensionObjectName);
         JavaTckSupport.check(ownerExtension != null && ownerExtension.equals(memberExtension)
               && extensionObjectName.equals(owner.getObjectClassName(ownerExtension))
               && base.equals(owner.getObjectClassHandle(baseObjectName)),
            "additional-module join did not compose a shared extension object class or preserve the base handle");

         extension.resignFederationExecution(ResignAction.NO_ACTION);
         extensionJoined = false;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (extensionJoined) {
            try { extension.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (extensionConnected) { try { extension.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
         if (observerConnected) { try { observer.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK object-class lookup callback sink";
               }
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
