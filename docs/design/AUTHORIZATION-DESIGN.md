# Authorization design

## Current bounded foundation

Umbra implements the official C++ `RTI/auth/HLAplainTextPassword.h` value
class.  Its constructor sets the predefined `HLAplainTextPassword` type and
stores the supplied string in the exact `HLAunicodeString` representation:
a four-octet big-endian UTF-16 code-unit count followed by UTF-16BE code
units.  Decode delegates to the same standard helper, including malformed
length, trailing-data, and UTF-16 validation.

Umbra also supplies the standard reference-library shape: an
`HLAauthorizer`, an `AuthorizerFactory` that creates it, and
`HLAauthorizerFactoryFactory::getAuthorizerFactory`.  The separate static
`umbra::authorizer` target implements the library-level
`AuthorizerFactoryFactory::getAuthorizerFactory` entry point and forwards a
standard-name request to the built-in factory.  An unknown name produces no
factory.  This preserves the C++ API/library seam without inventing custom
authorizer discovery before the required runtime configuration exists.

The embedded profile currently has no authorization service configured.  Its
Connect overloads therefore have this deliberately limited behavior:

- The overloads without a `Credentials` argument connect normally.
- An explicit, empty `HLAnoCredentials` envelope is treated as the standard
  no-credentials form and connects normally.
- Any other supplied credential envelope is rejected with `Unauthorized`
  before connection state changes.  It is never silently accepted as though
  authentication had occurred.

This is a client credential-value foundation and a no-authorization safety
boundary; it is not an enabled authorization service.

## Reference authorizer test seam

`ReferenceAuthorizerConfiguration` is an internal construction seam, used by
Catch2 to give a direct `HLAauthorizer` instance its global plaintext password.
With that private test configuration, a valid matching
`HLAplainTextPassword` authorizes, a nonmatching or unknown credential type is
unauthorized, and malformed credential data is invalid.  This proves the
bounded reference behavior without defining a public password syntax or
selecting an authorizer in the embedded RTI.

The public `HLAauthorizerFactoryFactory` is deliberately unconfigured today.
An authorizer made through it reports `AUTHORIZATION_ERROR`, rather than
pretending a password policy exists.  The embedded `UmbraRtiAmbassador` does
not obtain or invoke that authorizer; its disabled-authorization Connect gate
above remains the production behavior.

The separate JNI bridge exposes a raw Java conformance route through the exact
2025 `AuthorizerFactoryFactory` ServiceLoader interface. Its
`NativeAuthorizerFactory` delegates authorization decisions to the same C++
reference implementation and may receive a test password through the
`umbra.rti.jni.authorizer.password` JVM property. This does not configure the
embedded C++ RTI profile, widen the shared Python contract, or claim that the
secure RID-backed production slice is complete.

`umbra::authorizer` is a static CMake artifact boundary, not a claim that the
SDK already satisfies the standard's platform-specific dynamic library
nomenclature, third-party direct-link, or custom-authorizer loading rules.

## Deferred standard-facing work

IEEE 1516.1-2025 requires the selectable authorization service to be
configured through the RTI's RID and defines the reference `HLAauthorizer`
with a global plaintext password.  Umbra intentionally does not yet expose a
plaintext password through `RtiConfiguration::additionalSettings`: that field
is opaque, has no standard secret syntax, and is captured by the current
service-report initial record.  Doing so would create an implementation-defined
secret format and risk exposing a password in an operational report.

Consequently, the following remain a single later design/implementation slice:

1. A secure RID-backed configuration source and redacted report behavior.
2. Runtime selection, ownership, and result-code mapping of the configured
   `Authorizer` for Connect,
   Create Federation Execution, Destroy Federation Execution, and Join
   Federation Execution.
3. A defined policy for custom authorizer libraries, error containment,
   lifecycle, and concurrency, including the SISO library-nomenclature and
   direct-link requirements.

Until that slice exists, no production Umbra RTI profile claims to select,
configure, or execute an `HLAauthorizer`.

## Traceability and tests

`compliance/requirements-lab/authorization-requirements-contract.json` is a source/test
traceability contract only.  It references the 2025 Requirements Lab's
immutable candidates for the predefined password constructor, disabled
authorization behavior, reference authorizer, and authorizer-library
forwarding boundary.  The reconstructed source is §§12.5-12.6 plus the C++
library discussion; the exported candidates retain inaccurate Annex/§12.8
metadata for the authorization material, logged as RL-056.  RL-057 records a
source/header spelling discrepancy for the library factory method.

Catch2 tests prove exact safe vectors, a non-ASCII BMP string, copy/assignment,
malformed credential rejection, the factory forwarding and identity behavior,
the configured internal matching path, and a supplied password's rejected
Connect path.  They do not provide catalog, JUnit, package, interoperability,
security review, runtime-configuration, or conformance evidence.
