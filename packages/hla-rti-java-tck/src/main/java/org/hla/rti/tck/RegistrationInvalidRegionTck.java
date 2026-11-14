package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeRegionAssociation;
import hla.rti1516_2025.AttributeSetRegionSetPairList;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.DimensionHandle;
import hla.rti1516_2025.DimensionHandleSet;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.RegionHandle;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks InvalidRegion for a deleted region used during region-qualified registration. */
final class RegistrationInvalidRegionTck {
   private RegistrationInvalidRegionTck() { }

   static void run(RtiFactory factory, String fom, String objectClassName,
         String attributeName, String dimensionName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      String federation = "java-tck-registration-invalid-region-" + UUID.randomUUID();
      boolean connected = false, created = false, joined = false, published = false;
      RegionHandle staleRegion = null;
      boolean regionDeleted = false;
      try {
         owner.connect(noopFederateAmbassador(), callbackModel);
         connected = true;
         owner.createFederationExecution(federation, fom);
         created = true;
         owner.joinFederationExecution("java-tck-registration-invalid-region-owner", federation);
         joined = true;

         ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
         AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
         DimensionHandle dimension = owner.getDimensionHandle(dimensionName);
         JavaTckSupport.check(owner.getAvailableDimensionsForObjectClass(objectClass)
               .contains(dimension),
            "adapter object class does not expose the configured registration dimension");
         AttributeHandleSet attributes = JavaTckSupport.attributeSet(owner, attribute);
         owner.publishObjectClassAttributes(objectClass, attributes);
         published = true;

         DimensionHandleSet dimensions = owner.getDimensionHandleSetFactory().create();
         dimensions.add(dimension);
         staleRegion = owner.createRegion(dimensions);
         JavaTckSupport.check(staleRegion != null && staleRegion.encodedLength() > 0,
            "invalid-region registration could not create its region fixture");
         owner.deleteRegion(staleRegion);
         regionDeleted = true;

         RegionHandleSet staleRegions = owner.getRegionHandleSetFactory().create();
         staleRegions.add(staleRegion);
         AttributeSetRegionSetPairList invalidPair =
            owner.getAttributeSetRegionSetPairListFactory().create(1);
         invalidPair.add(new AttributeRegionAssociation(attributes, staleRegions));
         JavaTckSupport.expectFailure(() -> owner.registerObjectInstanceWithRegions(
               objectClass, invalidPair), "InvalidRegion",
            "registering an object instance with a deleted region designator");

         owner.unpublishObjectClassAttributes(objectClass, attributes);
         published = false;
         owner.resignFederationExecution(ResignAction.DELETE_OBJECTS);
         joined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (published && joined) try {
            ObjectClassHandle objectClass = owner.getObjectClassHandle(objectClassName);
            AttributeHandle attribute = owner.getAttributeHandle(objectClass, attributeName);
            owner.unpublishObjectClassAttributes(objectClass,
               JavaTckSupport.attributeSet(owner, attribute));
         } catch (Exception ignored) { }
         if (staleRegion != null && !regionDeleted && joined) try { owner.deleteRegion(staleRegion); }
            catch (Exception ignored) { }
         if (joined) try { owner.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         if (created) try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (connected) try { owner.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK no-op federate";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) return proxy == (args == null ? null : args[0]);
            }
            return null;
         });
   }
}
