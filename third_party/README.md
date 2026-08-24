# Vendored standards inputs

This directory holds pinned, immutable IEEE standards material used to compile
or validate Umbra. It is not Umbra implementation code.

| Directory | Contents | Verification |
| --- | --- | --- |
| ieee1516.1-2025/ | Official C++ API header baseline, provenance, and header digest manifest. | tools/verify_ieee_headers.py |
| ieee1516.2-2025/ | OMT schemas, standard MIM, supplied examples, provenance, and resource digests. | tools/ieee_1516_2_resources.py --check |

Do not hand-edit files below either vendor directory. A standards update is a
deliberate baseline change: import the authoritative source, update the
provenance and digest manifest through the corresponding tool, then review
downstream compilation and validation changes.

External FOM corpora are not stored here. Their non-redistributing manifests
live under [compliance/fom/](../compliance/fom/).
