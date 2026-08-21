package org.umbra.jni.rti1516_2025;

/**
 * Private bridge marker for dynamic Java data-element proxies that are backed
 * by a native C++ composite.  It is not part of the IEEE Java API; JNI uses it
 * only to clone the C++ element when a standard composite contains the proxy.
 */
public interface NativeCompositionDataElement {
   long nativeHandleForComposition();
}
