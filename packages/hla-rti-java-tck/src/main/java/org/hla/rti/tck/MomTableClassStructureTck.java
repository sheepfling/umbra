package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Verifies the selected predefined MOM class paths through the joined-federation FDD. */
final class MomTableClassStructureTck {
   private static final String[] OBJECT_CLASS_NAMES = {
      "HLAobjectRoot",
      "HLAobjectRoot.HLAmanager",
      "HLAobjectRoot.HLAmanager.HLAfederate",
      "HLAobjectRoot.HLAmanager.HLAfederation",
   };
   private static final String[] INTERACTION_CLASS_NAMES = {
      "HLAinteractionRoot",
      "HLAinteractionRoot.HLAmanager",
      "HLAinteractionRoot.HLAmanager.HLAfederate",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAmodifyAttributeState",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesThatCanBeDeleted",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesUpdated",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesReflected",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestUpdatesSent",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsSent",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsSent",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestReflectionsReceived",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsReceived",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsReceived",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstanceInformation",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestFOMmoduleData",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassPublication",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassSubscription",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionPublication",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionSubscription",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionPublication",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionSubscription",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesThatCanBeDeleted",
      "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation",
   };

   private MomTableClassStructureTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador creator = factory.getRtiAmbassador();
      RTIambassador member = factory.getRtiAmbassador();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-mom-table-classes-" + suffix;
      boolean creatorConnected = false;
      boolean memberConnected = false;
      boolean created = false;
      boolean creatorJoined = false;
      boolean memberJoined = false;
      try {
         creator.connect(callbacks(), callbackModel);
         creatorConnected = true;
         member.connect(callbacks(), callbackModel);
         memberConnected = true;
         creator.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         creator.joinFederationExecution("java-tck-mom-table-classes-creator-" + suffix,
            "java-tck", federation);
         creatorJoined = true;
         member.joinFederationExecution("java-tck-mom-table-classes-member-" + suffix,
            "java-tck", federation);
         memberJoined = true;

         verifyObjectClasses(creator, creator);
         verifyObjectClasses(member, creator);
         verifyInteractionClasses(creator, creator);
         verifyInteractionClasses(member, creator);
      } finally {
         if (memberJoined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (creatorJoined) {
            try { creator.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { creator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (memberConnected) {
            try { member.disconnect(); }
            catch (Exception ignored) { }
         }
         if (creatorConnected) {
            try { creator.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void verifyObjectClasses(RTIambassador ambassador, RTIambassador creator)
         throws Exception {
      for (String name : OBJECT_CLASS_NAMES) {
         ObjectClassHandle handle = ambassador.getObjectClassHandle(name);
         JavaTckSupport.check(handle != null, "§11.6 Table 6 MOM object class is unavailable");
         JavaTckSupport.check(name.equals(ambassador.getObjectClassName(handle)),
            "§11.6 Table 6 MOM object class name did not round-trip: " + name);
         JavaTckSupport.check(handle.equals(creator.getObjectClassHandle(name)),
            "MOM object class handle differs across joined federates: " + name);
      }
   }

   private static void verifyInteractionClasses(RTIambassador ambassador, RTIambassador creator)
         throws Exception {
      for (String name : INTERACTION_CLASS_NAMES) {
         InteractionClassHandle handle = ambassador.getInteractionClassHandle(name);
         JavaTckSupport.check(
            handle != null, "§11.6 Table 7 MOM interaction class is unavailable");
         JavaTckSupport.check(name.equals(ambassador.getInteractionClassName(handle)),
            "§11.6 Table 7 MOM interaction class name did not round-trip: " + name);
         JavaTckSupport.check(handle.equals(creator.getInteractionClassHandle(name)),
            "MOM interaction class handle differs across joined federates: " + name);
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) return "Java TCK MOM class callbacks";
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
