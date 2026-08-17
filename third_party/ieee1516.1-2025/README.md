# IEEE 1516.1-2025 C++ API headers

This directory contains the unmodified `RTI/` header tree from the official
HLA-4 C++ API distribution. It is a build dependency for Umbra's native 2025
API adapter, not an Umbra-authored API substitute.

Use [manifest.json](manifest.json) and
[header-digests.json](header-digests.json) to refresh and verify this tree:

```powershell
python tools/verify_ieee_headers.py
```

Do not hand-edit files under `include/RTI/`; update them as one reviewed
upstream import and retain their IEEE notices.

The imported archive states the required attribution, reproduced in
[NOTICE](NOTICE). See the Requirements Lab lock for the semantic-contract
revision separately; the header import and semantic corpus are related but
independently versioned inputs.
