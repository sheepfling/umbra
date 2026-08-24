# CMake support files

The root CMakeLists file owns the Umbra build, targets, and test registration.
This directory contains only the installed-package verification path and the
package configuration template.

| Path | Purpose |
| --- | --- |
| umbra_rti-config.cmake.in | Template installed for downstream find_package consumers. |
| run_installed_package_smoke.cmake | Installs the configured build into a staging prefix, configures a clean consumer, builds it, and runs its test. |
| package-smoke/ | Minimal downstream project used by the smoke script. |

CTest invokes the installed-package smoke path from the ordinary native build;
it is not normally run by hand. Its staging and consumer builds are created
below the selected out/cmake profile, never in a source directory.

For the build commands and named test lanes, start at the root README and
[requirements and testing](../docs/testing/REQUIREMENTS-AND-TESTING.md). CI
providers and local checkouts invoke matching presets through
[tools/ci.py](../tools/ci.py); see the [CI contract](../docs/development/CI.md).
