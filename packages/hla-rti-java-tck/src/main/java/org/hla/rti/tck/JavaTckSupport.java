package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.InteractionClassHandleSet;
import hla.rti1516_2025.RTIambassador;
import java.lang.reflect.Proxy;

/** Small provider-neutral helpers shared by the P1/P2 Java contract slices. */
final class JavaTckSupport {
   private JavaTckSupport() {
   }

   interface CheckedOperation {
      void run() throws Exception;
   }

   interface HandleEncoder {
      void encode(byte[] buffer, int offset);
   }

   static void check(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }

   static void expectFailure(CheckedOperation operation, String expected,
         String description) throws Exception {
      try {
         operation.run();
      } catch (Throwable error) {
         for (Throwable current = error; current != null; current = current.getCause()) {
            if (expected.equals(current.getClass().getSimpleName())) {
               return;
            }
         }
         throw new AssertionError(description + " threw " + error, error);
      }
      throw new AssertionError(description + " completed without " + expected);
   }

   static void drain(RTIambassador ambassador) throws Exception {
      for (int i = 0; i < 64; ++i) {
         ambassador.evokeCallback(0.0);
      }
   }

   static AttributeHandleSet attributeSet(RTIambassador ambassador,
         AttributeHandle... handles) throws Exception {
      AttributeHandleSet result = ambassador.getAttributeHandleSetFactory().create();
      for (AttributeHandle handle : handles) {
         result.add(handle);
      }
      return result;
   }

   static InteractionClassHandleSet interactionSet(RTIambassador ambassador,
         InteractionClassHandle... handles) throws Exception {
      InteractionClassHandleSet result =
         ambassador.getInteractionClassHandleSetFactory().create();
      for (InteractionClassHandle handle : handles) {
         result.add(handle);
      }
      return result;
   }

   static byte[] encode(int length, HandleEncoder encoder) {
      check(length > 0, "standard handle encodedLength was not positive");
      byte[] result = new byte[length];
      encoder.encode(result, 0);
      return result;
   }

   /**
    * Create an opaque invalid standard handle for negative service checks.
    * The proxy implements only the public handle interface; no provider type
    * enters the portable source set.
    */
   @SuppressWarnings("unchecked")
   static <T> T invalidHandle(Class<T> handleType) {
      return (T) Proxy.newProxyInstance(
         handleType.getClassLoader(),
         new Class<?>[] {handleType},
         (proxy, method, arguments) -> {
            if ("encodedLength".equals(method.getName())) {
               return 0;
            }
            if ("encode".equals(method.getName())) {
               return null;
            }
            if ("toString".equals(method.getName())) {
               return "invalid standard handle";
            }
            if ("hashCode".equals(method.getName())) {
               return System.identityHashCode(proxy);
            }
            if ("equals".equals(method.getName())) {
               return proxy == (arguments == null ? null : arguments[0]);
            }
            return null;
         });
   }
}

