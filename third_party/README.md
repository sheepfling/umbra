# Vendored standards inputs

This directory holds pinned, immutable IEEE standards material used to compile
or validate Umbra. It is not Umbra implementation code.

| Directory | Contents | Verification |
| --- | --- | --- |
| ieee1516.1-2010/ | Official IEEE 1516e C++ API headers and attribution for the bounded reference target. | tools/generate_1516e_cpp_shell.py --check; tools/verify_1516e_artifacts.py |
| ieee1516.1-2025/ | Official C++ API header baseline, provenance, and header digest manifest. | tools/verify_ieee_headers.py |
| ieee1516.2-2025/ | OMT schemas, standard MIM, supplied examples, provenance, and resource digests. | tools/ieee_1516_2_resources.py --check |

The IEEE 1516.1/1516.2-2010 FOM schemas and MIM are intentionally kept as
reviewed external resources for the opt-in `fomEdition=2010` compatibility
lane. Their source URLs and file digests are pinned in
`compliance/fom/external-2010-fom-resources.json`; configure
`UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY` to use them.

Do not hand-edit files below the vendor directories. A standards update is a
deliberate baseline change: import the authoritative source, update the
provenance and digest manifest through the corresponding tool, then review
downstream compilation and validation changes.

External FOM corpora are not stored here. Their non-redistributing manifests
live under [compliance/fom/](../compliance/fom/).
