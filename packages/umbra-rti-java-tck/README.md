# Umbra Java TCK integration wrapper

This directory is retained as a compatibility entry point for the Umbra JNI
product. The reusable test sources are in
[`../hla-rti-java-tck`](../hla-rti-java-tck/README.md); this directory contains
only the forwarding launcher and the Umbra capability profile.

`build.ps1` forwards compilation to the vendor-neutral package. `run.ps1`
forwards execution and, when `-NativeLibrary` is supplied, adds the Umbra JNI
library property as an adapter-owned JVM argument. No Umbra type or property
enters the portable Java source set.

For a provider-independent workflow, use `packages/hla-rti-java-tck` directly.
