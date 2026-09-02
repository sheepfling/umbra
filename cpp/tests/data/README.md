# Native XML fixture corpus

These files are deliberately small, named test inputs for the native FOM and
region test suites. The test source names and fixture names form the
traceability boundary, so keep a fixture's path stable once a test references
it.

## Naming

- A name beginning with invalid, malformed, unresolved, conflicting, or wrong
  describes a negative validation case.
- A name beginning with valid, strict, or basic describes a positive baseline.
- Names containing provider and consumer are paired composition inputs.
- Names containing region, dimension, directed, or update target a focused
  runtime or model-composition rule.

The corpus remains flat because many tests resolve fixtures directly by
filename and a short descriptive name makes a test case easy to locate. Add a
new fixture only for a behavior that needs a distinct model input; do not use
this directory for unreviewed external FOM corpora. Those are represented by
manifests under [compliance/fom/](../../../compliance/fom/).

The `regional-ownership-fanout-fom.xml` fixture is the focused positive model
for the public regional Auto Provide multi-provider lane: one dimensional
object class with two `DivestAcquire` attributes, used only to make the
ownership split explicit in C++ tests.
