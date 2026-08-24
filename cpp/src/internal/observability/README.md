# Observability and service-report internals

This domain records what the runtime did. It owns low-level instrumentation,
CSV/report output support, durable service-report storage, and the private
encoding used for MOM service reports. It observes validated operations; it
does not decide their semantics or authorization.

## Key files

- runtime_instrumentation files collect operation timing and outcome data.
- runtime_instrumentation_output files turn collected data into output
  records.
- service_report_store files manage the private durable report directory.
- mom_service_report_encoding files encode report content for the bounded MOM
  reporting path.

## Working here

- Add an observation, metric, report record, or serialization detail here.
- Keep the source of truth in the owning
  [federation](../federation/README.md),
  [time](../time/README.md), or
  [runtime](../runtime/README.md) domain.
- Do not let successful output substitute for behavior evidence. Compliance
  promotion is governed by
  [requirements and testing](../../../../docs/testing/REQUIREMENTS-AND-TESTING.md).

## Tests and references

Use runtime_instrumentation_catch2.cpp, service_report_store_catch2.cpp, and
mom_service_report_encoding_catch2.cpp under
[cpp/tests/](../../../tests/). The intended boundary is recorded in
[internal runtime instrumentation](../../../../docs/design/INTERNAL-RUNTIME-INSTRUMENTATION-DESIGN.md)
and [MOM service reporting](../../../../docs/design/MOM-SERVICE-REPORTING-DESIGN.md).
