package hla.rti1516_2025.time;

import java.util.HashSet;
import java.util.ServiceLoader;
import java.util.Set;

/** Small ServiceLoader-based counterpart to the official 2025 helper. */
public final class LogicalTimeFactoryFactory {
   private LogicalTimeFactoryFactory() {
   }

   public static LogicalTimeFactory<?, ?> getLogicalTimeFactory(String name) {
      for (LogicalTimeFactory<?, ?> factory : ServiceLoader.load(LogicalTimeFactory.class)) {
         if (factory.getName().equals(name)) return factory;
      }
      return null;
   }

   public static Set<LogicalTimeFactory<?, ?>> getAvailableLogicalTimeFactories() {
      Set<LogicalTimeFactory<?, ?>> factories = new HashSet<>();
      for (LogicalTimeFactory<?, ?> factory : ServiceLoader.load(LogicalTimeFactory.class)) {
         factories.add(factory);
      }
      return factories;
   }
}
