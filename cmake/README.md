# CMake support files

The root CMakeLists file owns the Umbra build, targets, and test registration.
This directory contains only the installed-package verification path and the
package configuration template.

| Path | Purpose |
| --- | --- |
| umbra_rti-config.cmake.in | Template installed for downstream find_package consumers. |
| run_installed_package_smoke.cmake | Builds the exported runtime set, installs the configured build into a staging prefix, validates the profile/resource manifests, configures a clean consumer, builds it, and runs its test. |
| package-smoke/ | Minimal downstream project used by the smoke script. |

CTest invokes the installed-package smoke path from the ordinary native build;
it is also available as the named `umbra_test_installable_package` target (or
the `installable-package` CTest label). Its staging and consumer builds are
created below the selected out/cmake profile, never in a source directory. The
smoke validates `umbra_rti-profile.json`, the exported LibXml2 dependency when
the embedded federation-management option is enabled, and the installed
1516.2 resource payload used by the runtime fallback.
The active process lane has a separate `umbra_process_boundary_junit` target,
which writes a bounded JUnit artifact under the configured compliance output
directory.

For the embedded federation-management profile, the package smoke also builds
and runs `umbra_rti_package_process_consumer` and its separately labelled
timestamped companion `umbra_rti_package_process_timestamped_consumer`, plus
the parameterized-envelope projection
`umbra_rti_package_process_parameterized_consumer`, plus the
`umbra_rti_package_process_connection_loss_consumer` recovery projection,
the `umbra_rti_package_process_object_registration_consumer` projection,
the `umbra_rti_package_process_named_registration_consumer` projection, and
the `umbra_rti_package_process_attribute_update_consumer` projection.
The ordinary, timestamped, parameterized, object-registration,
named-registration, and attribute-update cases
use only the installed public RTIambassador and official 2025 headers against
the source-tree private `umbra_process_federation_service_probe public-server`
fixture. The ordinary case covers two independent public ambassadors, Join,
server-owned interaction lookup, receive-order Send Interaction, Evoke
delivery, and NoAction Resign; the timestamped case additionally verifies the
official `HLAinteger64Time` callback overload, encoded time preservation,
RECEIVE order classification, and the absence of a fabricated retraction
handle. The parameterized case additionally resolves `TimelinessOk` and
verifies the exact parameter handle/value envelope at the receiver. The
connection-loss case uses the same installed public surface with
the private `public-server-loss` fixture, verifies the official
`connectionLost` callback through `EvokeMultipleCallbacks`, and proves the
surviving sender can continue using the federation after the receiver closes.
The object-registration case resolves the official object and attribute
handles, publishes `HLAprivilegeToDeleteObject`, registers an unnamed object,
and uses `DELETE_OBJECTS` on sender resignation while a second federate remains
joined. The named-registration case reserves a legal name through the official
callback surface, registers the named object, and verifies duplicate
registration raises `ObjectInstanceNameInUse` while an illegal reservation
raises `IllegalName`. The attribute-update case uses the installed public
`RTIambassador::updateAttributeValues` and official
`FederateAmbassador::reflectAttributeValues` callback; its private fixture
provides the server-owned object-class catalog while the process protocol
carries the receiver's subscription and automatic discovery through the
official public surfaces.
The fixture is a test dependency and is not installed as a runtime surface.

For the build commands and named test lanes, start at the root README and
[requirements and testing](../docs/testing/REQUIREMENTS-AND-TESTING.md). CI
providers and local checkouts invoke matching presets through
[tools/ci.py](../tools/ci.py); see the [CI contract](../docs/development/CI.md).
