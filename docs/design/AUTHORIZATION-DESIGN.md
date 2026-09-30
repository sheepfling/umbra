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

The embedded profile loads authorization configuration during Connect. Its
Connect overloads therefore have this deliberately bounded behavior:

- The overloads without a `Credentials` argument use the standard
  no-credentials form. With authorization disabled, they connect normally;
  with `HLAauthorizer` configured, they are authorized as `HLAnoCredentials`.
- An explicit, empty `HLAnoCredentials` envelope follows that same configured
  or disabled behavior.
- When disabled, any other supplied credential envelope is rejected with
  `Unauthorized` before connection state changes. When configured, its result
  is mapped from the official `AuthorizationResult` code.

The factory-created embedded RTI now uses this Connect path. This does not yet
authorize Create, Destroy, or Join Federation Execution.

## Reference authorizer test seam

`ReferenceAuthorizerConfiguration` is an internal construction seam. The
factory-created RTI's RID adapter uses it to give `HLAauthorizer` its global
plaintext password; Catch2 also uses it to exercise the reference behavior
directly. A valid matching `HLAplainTextPassword` authorizes, a nonmatching or
unknown credential type is unauthorized, and malformed credential data is
invalid.

The public `HLAauthorizerFactoryFactory` remains deliberately unconfigured:
its factory does not invent a password policy. The embedded RTI obtains the
configured factory through its private runtime adapter instead.

The separate JNI bridge exposes a raw Java conformance route through the exact
2025 `AuthorizerFactoryFactory` ServiceLoader interface. Its
`NativeAuthorizerFactory` delegates authorization decisions to the same C++
reference implementation and may receive a test password through the
`umbra.rti.jni.authorizer.password` JVM property. This does not configure the
embedded C++ RTI profile or widen the shared Python contract.

`umbra::authorizer` is a static CMake artifact boundary, not a claim that the
SDK already satisfies the standard's platform-specific dynamic library
nomenclature, third-party direct-link, or custom-authorizer loading rules.

## Embedded RID configuration

IEEE 1516.1-2025 requires the selectable authorization service to be
configured through RTI Runtime Initialization Data (RID), but does not define
an Umbra-style file grammar or pathname environment variable. Umbra's current
embedded-profile adapter is therefore explicitly implementation-defined:
`UMBRA_RTI_RID_FILE` names the RID file, and the official
`RtiConfiguration::configurationName` selects a profile. An empty or omitted
configuration name selects `default`.

The bounded file format is:

```ini
[authorization.default]
service=HLAauthorizer
globalPasswordFile=global-password.txt

[authorization.test]
service=disabled
```

`globalPasswordFile` may be absolute or relative to the RID file's directory;
its UTF-8 plaintext content is the reference authorizer's global password.
Umbra requires the RID and password to be regular files owned by the current
user, in a directory also owned by that user: on POSIX the files have no
group/other permissions and the containing directory is not group/other
writable; on Windows the ACL may grant access only to the current user,
Administrators, and Local System. Keep both files
outside source control and provision them with those protections. RID parsing
or credential-file failures produce a generic `RTIinternalError` from Connect.
The global password is not accepted through `additionalSettings` and is not
copied into the service-report connection snapshot.

The configured authorizer is retained by the ambassador for the connection
lifetime and released at Disconnect. This slice does not implement custom
authorizer-library loading, nor authorization of Create, Destroy, or Join
Federation Execution. The static `umbra::authorizer` artifact still does not
establish the standard's platform-specific dynamic library nomenclature,
third-party direct-link, interoperability, or conformance requirements.

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
malformed credential rejection, factory forwarding and identity behavior,
private authorizer result mapping, and factory-created RTIambassador Connect
using default and named RID profiles. The integration case covers matching,
wrong, omitted, and malformed credentials plus a missing-profile error. These
are private development-profile tests, not a security review, catalog/JUnit or
package evidence, interoperability, validation, or conformance evidence.
