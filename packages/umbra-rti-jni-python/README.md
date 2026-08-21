# umbra-rti-jni-python

Small provider adapter for the C++ → JNI → Java → JPype route.  It does not
implement RTI services and does not repackage the IEEE Java API.  Install it
alongside `umbra-rti-jpype[jpype]`, then supply three independently built
artifacts:

```powershell
$env:UMBRA_JNI_JAVA_API_JAR = 'C:\vendor\ieee-1516.1-2025-java-api.jar'
$env:UMBRA_JNI_BRIDGE_JAR = 'C:\vendor\umbra-rti-jni.jar'
$env:UMBRA_JNI_NATIVE_LIBRARY = 'C:\vendor\umbra_rti_jni.dll'
```

The adapter places the API and bridge JARs on the JVM class path, sets the
native-library property, and asks the exact Java
`hla.rti1516_2025.RtiFactoryFactory` to discover the bridge's
`NativeRtiFactory`.  Python calls and callbacks then use the existing generic
JPype adapter, while C++ remains the only RTI state/semantics owner.

The same values can be supplied as constructor arguments.  For a build
directory produced by `packages/umbra-rti-jni/build.ps1`, set
`UMBRA_JNI_BRIDGE_ARTIFACT_DIRECTORY`; the adapter finds the bridge JAR and
native library there and still requires `UMBRA_JNI_JAVA_API_JAR`.
