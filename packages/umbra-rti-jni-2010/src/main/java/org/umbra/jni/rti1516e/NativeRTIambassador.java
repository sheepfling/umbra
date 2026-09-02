package org.umbra.jni.rti1516e;

import hla.rti1516e.RTIambassador;
import hla.rti1516e.exceptions.RTIinternalError;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

/**
 * Standard Java ambassador façade for the C++ 2010 null binding.
 *
 * <p>The dynamic proxy keeps the Java surface exactly equal to the supplied
 * IEEE API JAR. Every RTI method enters the native boundary and receives the
 * standard {@code RTIinternalError}; no Java-side RTI state is maintained.</p>
 */
final class NativeRTIambassador implements InvocationHandler, AutoCloseable {
   private long nativeHandle = NativeBridge.nativeCreate();
   private Object federateAmbassador;

   private NativeRTIambassador() throws RTIinternalError {
   }

   static RTIambassador create() throws RTIinternalError {
      NativeRTIambassador handler = new NativeRTIambassador();
      return (RTIambassador) Proxy.newProxyInstance(
         RTIambassador.class.getClassLoader(),
         new Class<?>[] { RTIambassador.class, AutoCloseable.class },
         handler);
   }

   private long requireNativeHandle() throws RTIinternalError {
      if (nativeHandle == 0L) {
         throw new RTIinternalError(
            "The Umbra IEEE 1516e RTI ambassador is closed");
      }
      return nativeHandle;
   }

   @Override
   public Object invoke(Object proxy, Method method, Object[] arguments) throws Throwable {
      if (method.getDeclaringClass() == Object.class) {
         if ("toString".equals(method.getName())) {
            return "Umbra IEEE 1516.1-2010 JNI null RTIambassador";
         }
         if ("hashCode".equals(method.getName())) {
            return System.identityHashCode(proxy);
         }
         if ("equals".equals(method.getName())) {
            return proxy == (arguments == null ? null : arguments[0]);
         }
      }
      if (method.getDeclaringClass() == AutoCloseable.class &&
          "close".equals(method.getName())) {
         close();
         return null;
      }
      if ("connect".equals(method.getName())) {
         Object[] supplied = arguments == null ? new Object[0] : arguments;
         Object model = supplied.length > 1 ? supplied[1] : null;
         String modelName = model instanceof Enum<?> ?
            ((Enum<?>) model).name() : String.valueOf(model);
         String settings = supplied.length > 2 && supplied[2] != null ?
            String.valueOf(supplied[2]) : "";
         NativeBridge.nativeConnect(requireNativeHandle(), modelName, settings);
         // Keep the callback strongly reachable for the lifetime of the
         // standard ambassador, even though this bounded native slice does
         // not yet forward callbacks across JNI.
         federateAmbassador = supplied.length > 0 ? supplied[0] : null;
         return null;
      }
      if ("disconnect".equals(method.getName())) {
         NativeBridge.nativeDisconnect(requireNativeHandle());
         federateAmbassador = null;
         return null;
      }
      NativeBridge.nativeUnavailable(requireNativeHandle());
      throw new AssertionError("nativeUnavailable returned without throwing");
   }

   @Override
   public void close() {
      if (nativeHandle != 0L) {
         NativeBridge.nativeDestroy(nativeHandle);
         nativeHandle = 0L;
         federateAmbassador = null;
      }
   }
}
