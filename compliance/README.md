# Compliance inputs

This directory contains reviewable compliance inputs and development-only
manifests. It is intentionally separate from the runtime implementation. The
generated Requirements Lab bundle and protected evidence remain under ignored
`.compliance/`; ordinary raw JUnit output belongs under ignored `out/`.

## Requirements Lab boundary

[requirements-lab/](requirements-lab/) contains the Umbra-owned contracts,
baselines, test plans, test catalog, and lock file that reference the adjacent
HLA Requirements Lab. These files define traceability and evidence inputs;
they are not imported by the packaged runtime.

## Other compliance inputs

- [fom/](fom/) contains manifests for optional, unvendored external FOM
  corpora. The manifests do not redistribute the source XML.
- [catalogs/](catalogs/) contains catalogs for provider-neutral test plans,
  such as the Java RTI TCK scenario catalog.
- [standards/](standards/) contains review inventories for the vendored IEEE
  API baseline.

When adding a new artifact, place it by provenance: Requirements Lab-derived
traceability under `requirements-lab/`, external FOM metadata under `fom/`,
test scenario catalogs under `catalogs/`, and standard-header inventories under
`standards/`.
