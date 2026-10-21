# Shared-library identity and ABI policy

The IEEE 1516.1-2025 package exports the stable CMake targets `umbra::rti`,
`umbra::fedtime`, and `umbra::authorizer`. Their stable library basenames are
`umbra_rti`, `umbra_fedtime`, and `umbra_authorizer`; platform prefixes and
suffixes remain the toolchain's responsibility.

For shared builds, CMake records the project version as each library's
`VERSION` and the project major version as its `SOVERSION`. The package config
and each imported target expose that ABI major. CMake's compatible-interface
string check rejects a consumer that combines Umbra targets with different ABI
majors. An ABI-breaking change therefore requires a project-major bump; minor
and patch releases retain the ABI generation.

The vendored IEEE `RTI/SpecificConfig.h` export macros remain authoritative:
shared library compilation defines `BUILDING_RTI`, `BUILDING_FEDTIME`, or
`BUILDING_AUTHORIZER` privately; static consumers receive the corresponding
`STATIC_*` definition through the exported targets. Automatic Windows symbol
export is disabled so the official API annotations, rather than incidental
implementation symbols, define the exported surface.

This metadata and policy do not prove ABI compatibility with another RTI or
conformance to IEEE 1516.1-2025. That still requires an independent compatible
implementation and the same already-built consumer executable to run against
both providers without recompilation or relinking.
