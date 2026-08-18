package hla.rti1516_2025;

import hla.rti1516_2025.exceptions.RTIinternalError;
import java.util.ServiceLoader;

/** A small ServiceLoader-based counterpart to the standard helper. */
public final class RtiFactoryFactory {
   private RtiFactoryFactory() {
   }

   public static RtiFactory getRtiFactory(String name) throws RTIinternalError {
      for (RtiFactory factory : ServiceLoader.load(RtiFactory.class)) {
         if (factory.rtiName().equals(name)) {
            return factory;
         }
      }
      throw new RTIinternalError("Cannot find factory matching " + name);
   }

   public static RtiFactory getRtiFactory() throws RTIinternalError {
      String name = System.getenv("HLA_RTI_FACTORY_NAME");
      if (name != null) {
         return getRtiFactory(name);
      }
      for (RtiFactory factory : ServiceLoader.load(RtiFactory.class)) {
         return factory;
      }
      throw new RTIinternalError("Cannot find factory");
   }
}
