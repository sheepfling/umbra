# Authorization design

## Current bounded foundation

Umbra implements the official C++ `RTI/auth/HLAplainTextPassword.h` value
class. Its constructor sets the predefined `HLAplainTextPassword` type and
stores the supplied string in the exact `HLAunicodeString` representation:
a four-octet big-endian UTF-16 code-unit count followed by UTF-16BE code
units. Decode delegates to the same standard helper, including malformed
length, trailing-data, and UTF-16 validation.

Umbra also supplies the standard reference-library shape: an
`HLAauthorizer`, an `AuthorizerFactory` that creates it, and
`HLAauthorizerFactoryFactory::getAuthorizerFactory`. The separate static
`umbra::authorizer` target implements the library-level
`AuthorizerFactoryFactory::getAuthorizerFactory` entry point. It asks the
optional provider bridge for the requested service, then falls back to the
built-in `HLAauthorizerFactoryFactory` when the provider is absent or returns
no matching factory. A configured but unavailable provider returns no factory;
with no provider configured, an unknown name also produces no factory. The
embedded RID adapter has its own selection path for non-reference services;
the [authorization flow guide](HLA-2025-AUTHORIZATION-FLOW-GUIDE.md#how-umbra-selects-an-enabled-service-factory)
records its branches and limits.

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

The factory-created embedded RTI now uses this Connect path. It also invokes
the scoped authorization callbacks for Create Federation Execution (including
Create-With-MIM), Destroy Federation Execution, and Join before their ordinary
service paths. See the [2025 authorization and credentials flow
guide](HLA-2025-AUTHORIZATION-FLOW-GUIDE.md) for the callback scopes, exception
mapping, and evidence limits. This does not establish process-endpoint parity
or full standards conformance.

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

### Shared profile parser and RTI protocol timeouts

The same parser also reads Umbra's `[rti.<profile>]` sections from this file.
Its INI-like section syntax is implementation-defined; it is not the standard's
text-file grammar. An authorization section and an RTI section with the same
name contribute to one selected profile, and each section type may appear at
most once:

```ini
[authorization.default]
service=HLAauthorizer
globalPasswordFile=global-password.txt

[rti.default]
RTI_TIMEOUT_HEART=90.25
RTI_TIMEOUT_PURGE=1200
```

The official `RtiConfiguration::configurationName` selects both sections by
profile name. The `[rti.<profile>]` section accepts `RTI_TIMEOUT_HEART` and
`RTI_TIMEOUT_PURGE` as positive decimal seconds with up to three fractional
digits, converted to milliseconds. Duplicate recognized sections or keys,
unknown keys within either recognized section, zero or malformed durations
are rejected. When timeout settings are loaded, Umbra also rejects an
effective purge duration shorter than the effective heartbeat duration. A
missing RID path, or a file with no recognized profiles, leaves the RTI's
timeout defaults in place; if profiles exist but the selected name does not,
profile lookup fails. The private host defaults are 180 seconds for heartbeat
timeout and 600 seconds for purge timeout; the implementation labels these as
Table 31 values in the [2025 host options](../../cpp/src/internal/federation/process_federation_service_host.hpp#L21).

Two [internal timeout-profile cases](../../cpp/tests/ieee1516_2025_authorization_catch2.cpp#L650)
cover default and named profiles, conversion to milliseconds, propagation into
host options, and an empty RTI section using defaults. The adjacent
[invalid-profile case](../../cpp/tests/ieee1516_2025_authorization_catch2.cpp#L695)
rejects zero, more than three fractional digits, purge shorter than heartbeat,
and duplicate keys. Both are tagged `internal-only` and
`no-normative-mapping`; they establish Umbra parser behavior, not HLA
conformance. The [process-host RID case](../../cpp/tests/ieee1516_2025_connection_process_federation_service_host_catch2.cpp#L680)
uses parsed values and exercises retention, resume, and eventual purge, without
asserting exact timeout boundaries. These cases were not rerun during this
documentation review.

Build ownership follows the same edition boundary: the parser is compiled into
the 2025 `umbra_rti` target ([source list](../../CMakeLists.txt#L460),
[2025 API link](../../CMakeLists.txt#L501)); `umbra_rti_2010` is a separate
target ([boundary](../../CMakeLists.txt#L510)). Its focused test source is
included in the 2025 Catch2 target
([test list](../../cmake/catch2_executable_targets.cmake#L7)).

The configured authorizer is retained by the ambassador for the connection
lifetime and released at Disconnect. The embedded operation gates now cover
Create, Create-With-MIM, Destroy, and Join; the selected operation-test
evidence and its setup limitations are recorded in the linked flow guide.

Umbra implements an implementation-specific shared-library provider bridge
for non-reference service names. A non-empty `UMBRA_AUTHORIZER_PROVIDER_LIBRARY`
must name an absolute path to a library exporting
`umbra_authorizer_factory_v1`, which returns a 2025 `AuthorizerFactory` for a
requested service name. The loader caches its result on first lookup and keeps
the library loaded while its provider instance lives. In the RID path, a
configured but unusable provider (relative path, load failure, or missing
entry point) makes the profile invalid. If a usable provider returns no
factory with the requested name, Umbra tries the standard
`HLAauthorizerFactoryFactory` lookup and requires that result to match. The
[provider loader](../../cpp/src/internal/runtime/authorizer_factory_provider.cpp#L82)
and [RID selection branch](../../cpp/src/internal/runtime/rti_initialization_data.cpp#L339)
are source evidence; the current [authorization Catch2 suite](../../cpp/tests/ieee1516_2025_authorization_catch2.cpp#L371)
does not contain focused provider-loading, fallback, or failure cases. This
bridge is Umbra policy, not an IEEE requirement, and does not establish the
standard's platform-specific dynamic library nomenclature, third-party
direct-link, interoperability, or conformance requirements.

## Traceability and tests

`compliance/requirements-lab/authorization-requirements-contract.json` is a source/test
traceability contract only. It references the 2025 Requirements Lab's
immutable candidates for the predefined password constructor, disabled
authorization behavior, reference authorizer, and authorizer-library
forwarding boundary. The reconstructed source is §§12.5-12.6 plus the C++
library discussion; the exported candidates retain inaccurate Annex/§12.8
metadata for the authorization material, logged as RL-056. RL-057 records a
source/header spelling discrepancy for the library factory method.

Catch2 tests prove exact safe vectors, a non-ASCII BMP string, copy/assignment,
malformed credential rejection, factory forwarding and identity behavior,
private authorizer result mapping, and factory-created RTIambassador Connect
using default and named RID profiles. The integration case covers matching,
wrong, omitted, and malformed credentials plus a missing-profile error. These
are private development-profile tests, not a security review, catalog/JUnit or
package evidence, interoperability, validation, or conformance evidence.
