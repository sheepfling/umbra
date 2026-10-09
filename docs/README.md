# Documentation

This directory is organized by the question a document answers rather than by
the order in which it was written. Start with the architecture and planning
pages for the project boundary, then use the focused area that matches the
work you are doing.

## Start here

- [Architecture](architecture/ARCHITECTURE.md) — public API boundary, runtime
  layers, and requirement traceability.
- [Roadmap](planning/ROADMAP.md) — completed work, current gates, and future
  capability phases.
- [Implementation plan](planning/IMPLEMENTATION-PLAN.md) — the native C++
  implementation sequence and engineering controls.
- [Requirements and testing](testing/REQUIREMENTS-AND-TESTING.md) — test
  authority, promotion rules, and development lanes.

## Areas

### Development

- [Contributor guide](../CONTRIBUTING.md) — first setup, safe changes, test
  profiles, and review expectations.
- [Repository guide](development/REPOSITORY-GUIDE.md) — top-level ownership,
  source locations, and local-output conventions.
- [Style and review](development/STYLE-AND-REVIEW.md) — source style,
  review expectations, and automated baseline checks.
- [CI contract](development/CI.md) — provider-neutral local and pipeline
  entry point.

### Architecture

- [Architecture](architecture/ARCHITECTURE.md) — public API and runtime
  layering.

### Planning

[planning/](planning/) contains the project roadmap and the more detailed
native implementation plan.

### Focused designs

[design/](design/) contains bounded design records for individual runtime
capabilities. These pages explain the implemented boundary, deliberate limits,
and the evidence needed before expansion.

- [Authorization](design/AUTHORIZATION-DESIGN.md)
- [Embedded time coordination](design/EMBEDDED-TIME-COORDINATION-DESIGN.md)
- [HLA behavior-flow guide backlog](design/HLA-BEHAVIOR-FLOW-GUIDES.md) — the
  edition-separated plan for contributor state and information-flow diagrams.
- [IEEE 1516.1-2010 reference RTI implemented flow](design/HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 attribute ownership](design/HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 callback and service ordering](design/HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 data distribution and regions](design/HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 federation and federate lifecycle](design/HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 federation save and restore](design/HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 federation synchronization points](design/HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 exception reporting](design/HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 joined-federate MOM object lifecycle](design/HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 FOM module admission and composition](design/HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 service invocation reporting](design/HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 object and interaction information flow](design/HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 object-instance name reservation](design/HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 order and transportation](design/HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 declaration and attribute relevance advisories](design/HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 Attribute Scope Advisory](design/HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 Auto Provide](design/HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md)
- [IEEE 1516.1-2025 time-management contributor guide](design/HLA-2025-TIME-MANAGEMENT-GUIDE.md)
- [IEEE 1516.1-2025 TSO retraction](design/HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md)
- [Internal runtime instrumentation](design/INTERNAL-RUNTIME-INSTRUMENTATION-DESIGN.md)
- [Logical time](design/LOGICAL-TIME-DESIGN.md)
- [MOM service reporting](design/MOM-SERVICE-REPORTING-DESIGN.md)
- [Relaxed DDM policy](design/RELAXED-DDM-POLICY.md)

### FOM

[fom/](fom/) contains FOM validation/composition design and the curated stress
corpus intake backlog.

- [FOM validation](fom/FOM-VALIDATION-DESIGN.md)
- [FOM stress-corpus intake](fom/FOM-STRESS-CORPUS-BACKLOG.md)

### Python bindings

[python/](python/) contains the Python public-contract, native-provider,
encoding, factory, coverage, conformance, and Java-provider adapter documents.

- [Bindings design](python/PYTHON-BINDINGS-DESIGN.md)
- [API coverage plan](python/PYTHON-API-COVERAGE-PLAN.md)
- [Conformance testing](python/PYTHON-CONFORMANCE-TESTING.md)
- [Encoding binding design](python/PYTHON-ENCODING-BINDING-DESIGN.md)
- [Factory inventory](python/PYTHON-FACTORY-INVENTORY.md)
- [Java adapter](python/PYTHON-JAVA-ADAPTER.md)
- [Package map](../packages/README.md) — the reason for each Python and Java
  distribution.

### Testing and compliance

[testing/](testing/) contains the Requirements Lab workflow and observations,
test-lane guidance, the Java conformance TCK boundary, and legacy Python test
resource/backlog records.

- [Compliance workflow](testing/COMPLIANCE-WORKFLOW.md)
- [Java RTI conformance TCK](testing/JAVA-RTI-CONFORMANCE-TCK.md)
- [Legacy Python scenario backlog](testing/LEGACY-PYTHON-RTI-TEST-BACKLOG.md)
- [Legacy Python test resources](testing/LEGACY-PYTHON-RTI-TEST-RESOURCES.md)
- [Requirements Lab observations](testing/REQUIREMENTS-LAB-OBSERVATIONS.md)
- [Requirements and testing](testing/REQUIREMENTS-AND-TESTING.md)

## Documentation conventions

Design and planning pages describe bounded implementation work; they are not
claims of complete IEEE conformance unless they say so explicitly. Requirements
Lab observations and test evidence remain separate from implementation plans so
that a proposed capability cannot be mistaken for verified behavior.
