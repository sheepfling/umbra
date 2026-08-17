# Requirements-Lab compliance workflow

Umbra follows the HLA Requirements Lab's portable sidecar model. Test runners
produce behavior observations; the Lab checks their relationship to the pinned
corpus; a separate protected review accepts evidence before a binding can be
called verified.

~~~text
plan -> implementation -> Catch2/JUnit XML -> raw sidecar manifest
     -> protected evidence review -> verified manifest
~~~

## Current state

compliance/catch2-test-plan.json remains planning input for unfinished service
families. compliance/test-catalog.json now contains one real embedded
Disconnect integration test with the Lab's exact selected C++ API surface.
Running umbra_catch2_junit emits one matching JUnit testcase and the sidecar
produces an implemented binding plus unreviewed, real passed evidence. That raw
result is not validated or verified. The plan also records a real Create/
Destroy/Join/Resign Catch2 scenario in Umbra's non-installable development
profile. That scenario is source/test traceability only: it has no test-catalog
entry, JUnit sidecar result, protected review, package support, or conformance
claim.

## 1. Export and prepare

Use the Python environment that can run the local Requirements Lab:

~~~powershell
$python = 'C:\Users\peanu\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $python tools/requirements_lab.py export --python $python
& $python tools/requirements_lab_sidecar.py prepare --python $python
& $python tools/generate_cpp_conformance_worklist.py
~~~

This writes an ignored .compliance/adapter-request.json containing the exact
requirements, transitions, mappings, and selected API surfaces that the Lab
currently expects. The generated .compliance/cpp-conformance-worklist.json then
lists every C++ binding as matched, ambiguous, unmatched, or planned. Both
artifacts make no implementation claim.

## 2. Add a real test catalog with each service slice

Once a native standard symbol and Catch2 behavior test exist, add a schema-valid
compliance/test-catalog.json. Copy the IDs from the freshly exported bundle; do
not invent or abbreviate them. A catalog entry needs the exact mapping ID, one
selected API surface, the implementation symbol, a stable Catch2 selector, and
the exact JUnit testcase selector. Only real RTI behavior tests may use
execution_mode: real.

The Connect family remains outside the catalog because the aggregate mapping
still has four C++ candidates and no selected surface in the Lab sidecar
request. Umbra implements and tests all four overloads, but does not claim
catalog evidence for them. Disconnect has a selected surface and one real
cataloged test. Resign is exercised only by the non-installable development
profile, so it remains outside the catalog until its service and package scope
are stable enough for a reproducible JUnit evidence submission.

## 3. Produce raw evidence

After the existing build runs its mapped Catch2 tests and writes JUnit XML:

~~~powershell
cmake --build .build --target umbra_catch2_junit --config Debug
& $python tools/requirements_lab_sidecar.py raw --python $python --catalog compliance/test-catalog.json --results .build/compliance/catch2-results.xml
~~~

raw invokes the Lab's supplied JUnit adapter. Its output manifest is ignored
under .compliance/ and deliberately contains unreviewed evidence only. The
pinned adapter derives coverage_mode: partial from this intentionally small
catalog, keeping all missing work visible without claiming complete scope.

The JUnit artifact contains only catalog-tagged tests; baseline and private unit
tests remain useful but do not enter the raw compliance manifest.

## 4. Review and verify

A protected CI or reviewer promotes only inspected, reproducible real evidence
to review_status: accepted and only then changes a binding to verified. The
review-owned manifest can be checked with:

~~~powershell
& $python tools/requirements_lab_sidecar.py verify --python $python --manifest path\to\reviewed-manifest.json
~~~

The sidecar's structural passed field and its coverage.fully_verified field
answer different questions. A partial manifest can be structurally valid
without supporting a full conformance claim.
