package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Standard attribute lookup boundaries, inheritance, and cross-member identity. */
final class AttributeLookupLifecycleTck {
   private AttributeLookupLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String baseObjectName, String derivedObjectName, String inheritedAttributeName,
         String derivedAttributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String federation = "java-tck-attribute-lookup-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean memberJoined = false;
      try {
         owner.connect(callbacks(), callbackModel);
         ownerConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;

         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-attribute-lookup-owner", federation);
         ownerJoined = true;

         ObjectClassHandle baseObject = owner.getObjectClassHandle(baseObjectName);
         ObjectClassHandle derivedObject = owner.getObjectClassHandle(derivedObjectName);
         AttributeHandle baseInherited =
            owner.getAttributeHandle(baseObject, inheritedAttributeName);
         AttributeHandle derivedInherited =
            owner.getAttributeHandle(derivedObject, inheritedAttributeName);
         AttributeHandle derivedAttribute =
            owner.getAttributeHandle(derivedObject, derivedAttributeName);
         JavaTckSupport.check(baseObject != null && derivedObject != null
               && baseInherited != null && derivedInherited != null && derivedAttribute != null,
            "inherited attribute lookup returned a null class or attribute handle");
         JavaTckSupport.check(baseInherited.equals(derivedInherited)
               && baseInherited.equals(owner.getAttributeHandle(derivedObject,
                  inheritedAttributeName))
               && inheritedAttributeName.equals(
                  owner.getAttributeName(baseObject, baseInherited))
               && inheritedAttributeName.equals(
                  owner.getAttributeName(derivedObject, derivedInherited))
               && derivedAttributeName.equals(
                  owner.getAttributeName(derivedObject, derivedAttribute)),
            "inherited attribute lookup did not preserve stable handles and names");

         JavaTckSupport.expectFailure(
            () -> owner.getAttributeHandle(baseObject, derivedAttributeName), "NameNotFound",
            "looking up a derived attribute on its base object class");
         String unknownAttributeName = "TckMissingAttribute"
            + UUID.randomUUID().toString().replace("-", "");
         JavaTckSupport.expectFailure(
            () -> owner.getAttributeHandle(derivedObject, unknownAttributeName), "NameNotFound",
            "looking up an unknown attribute name");
         JavaTckSupport.expectFailure(
            () -> owner.getAttributeName(baseObject, derivedAttribute), "AttributeNotDefined",
            "looking up a derived attribute handle on its base object class");

         member.joinFederationExecution("java-tck-attribute-lookup-member",
            "java-tck-attribute-lookup-member", federation);
         memberJoined = true;
         ObjectClassHandle memberBase = member.getObjectClassHandle(baseObjectName);
         AttributeHandle memberBaseInherited =
            member.getAttributeHandle(memberBase, inheritedAttributeName);
         ObjectClassHandle memberDerived = member.getObjectClassHandle(derivedObjectName);
         AttributeHandle memberInherited =
            member.getAttributeHandle(memberDerived, inheritedAttributeName);
         AttributeHandle memberDerivedAttribute =
            member.getAttributeHandle(memberDerived, derivedAttributeName);
         JavaTckSupport.check(baseObject.equals(memberBase)
               && derivedObject.equals(memberDerived)
               && baseInherited.equals(memberBaseInherited)
               && baseInherited.equals(memberInherited)
               && derivedAttribute.equals(memberDerivedAttribute)
               && inheritedAttributeName.equals(
                  member.getAttributeName(memberBase, memberBaseInherited))
               && inheritedAttributeName.equals(
                  member.getAttributeName(memberDerived, memberInherited))
               && derivedAttributeName.equals(
                  member.getAttributeName(memberDerived, memberDerivedAttribute)),
            "inherited attribute handles or names did not retain cross-federate identity");

         member.resignFederationExecution(ResignAction.NO_ACTION);
         memberJoined = false;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (memberConnected) { try { member.disconnect(); } catch (Exception ignored) { } }
         if (ownerConnected) { try { owner.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK attribute lookup callback sink";
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
