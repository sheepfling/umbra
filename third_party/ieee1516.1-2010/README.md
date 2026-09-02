# IEEE 1516.1-2010 C++ API headers

This directory contains the unmodified C++ API headers from the locally
staged IEEE 1516.1-2010 API archive. They define the public `rti1516e`
namespace used by the bounded null provider.

Umbra does not redistribute the IEEE download archives. The archive digests,
source URL, and local-verification procedure are recorded in
`compliance/standards/ieee1516e-2010-artifact-manifest.json`.

The direct C++ implementation linked to these headers is intentionally
bounded: it provides the encoder/time families, connection lifecycle, and the
reference federation/declaration/object/interaction/ownership/synchronization
proof slices used by the 2010 surface tests. Remaining RTI/MOM services are
explicit capability gaps. This is not a claim of complete IEEE conformance.
The separate `umbra-rti-jni-2010` artifact is the null connect/disconnect ABI
bridge; it is not the implementation described by this directory.
