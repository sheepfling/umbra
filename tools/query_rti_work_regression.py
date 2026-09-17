"""Offline regression checks for the bounded roadmap traceability queries.

The query tool is the normal work-selection interface once the Requirements
Lab and Catch2 plan have been indexed.  This guard keeps the most important
relationship explicit: each matrix row must expose a direct
Requirements-Lab-id to canonical IEEE 1516.1-2025 subsection mapping, and the
bounded family queue must expose the same live assertion total as its focused
lane. It also keeps the one-screen dashboard, roadmap-family matrix fallback,
the compact summary pair shape, family work-command fallback, and unmapped
contract-candidate discovery deterministic. The local contract-selector guard
must remain clean while external portable-TCK symbols stay classified as
external. It uses the checked-in sources read-only and never rewrites the
roadmap, plan, or Requirements Lab.
The exact ``case`` handoff is also checked: a unique plan id/title must produce
the direct requirement-to-subsection pairs and focused execution handles in
one bounded card.
The separate Requirements Lab issue ledger is also checked so a known
extraction defect remains queryable without becoming a silent mapping rewrite.
Requirement and canonical-section reverse lookups retain a bounded uncovered
corpus fallback when no Catch2 row exists, so a new case can start from an
exact standard anchor.
The compact ``resume`` projection is checked separately from the richer
dashboard: it must retain one bounded next choice and stable execution
handles without leaking the historical family/test inventory into a normal
first-read.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path


SCRIPT_ROOT = Path(__file__).resolve().parent
REPOSITORY_ROOT = SCRIPT_ROOT.parent
sys.path.insert(0, str(SCRIPT_ROOT))

import query_rti_work  # noqa: E402  (repository tool import after path setup)


TARGET_TEST = (
    "Embedded timestamped Delete Object Instance reconstitutes on retraction "
    "and removes before grant"
)

MAPPED_CORE_CASES = {
    "Embedded federate lookup services preserve departed designator identities within the joined federation": {
        "mapping_id": "umbra-federate-lookup-designator-identity",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-4.11.4",
            "hla-1516.1-2025:clause-10.3.3",
        },
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:23973",
        "assertions": 33,
    },
    "Embedded asynchronous delivery gates receive-order callbacks by temporal state": {
        "mapping_id": "rti.service.asynchronous-delivery",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-8.15.3",
            "hla-1516.1-2025:clause-8.16.5",
        },
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:3160",
        "assertions": 35,
    },
    "Embedded Time Advance Request changes logical time only at Time Advance Grant dispatch": {
        "mapping_id": "rti.service.time-advance-request",
        "requirements": 7,
        "sections": {
            "hla-1516.1-2025:clause-8.1.2",
            "hla-1516.1-2025:clause-8.8.3",
            "hla-1516.1-2025:clause-8.14.3",
            "hla-1516.1-2025:clause-8.18.1",
        },
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:51270",
        "assertions": 40,
    },
    "Embedded time-role services keep enable requests callback-gated before TSO support": {
        "mapping_id": "rti.service.time-role-control",
        "requirements": 8,
        "sections": {
            "hla-1516.1-2025:clause-8",
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.3.1",
            "hla-1516.1-2025:clause-8.5.5",
            "hla-1516.1-2025:clause-8.6.3",
            "hla-1516.1-2025:clause-8.7.5",
            "hla-1516.1-2025:clause-8.21.5",
        },
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:51422",
        "assertions": 51,
    },
    "Embedded Modify Lookahead applies increases immediately and decreases gradually": {
        "mapping_id": "rti.service.modify-lookahead",
        "requirements": 7,
        "sections": {
            "hla-1516.1-2025:clause-8.20.4",
        },
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:51539",
        "assertions": 28,
    },
    "RTIambassador requests Flush Queue through a configured process endpoint": {
        "mapping_id": "time-management.process.flush-queue-request",
        "requirements": 4,
        "sections": {
            "hla-1516.1-2025:clause-8.12",
            "hla-1516.1-2025:clause-8.12.3",
        },
            "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:18207",
        "assertions": 16,
    },
    "RTIambassadors preserve actual and optimistic Flush Queue boundaries through a configured process endpoint": {
        "mapping_id": "time-management.process.flush-queue-galt-frontier",
        "requirements": 19,
        "sections": {
            "hla-1516.1-2025:clause-4",
            "hla-1516.1-2025:clause-5.1.4",
            "hla-1516.1-2025:clause-6.12.4",
            "hla-1516.1-2025:clause-6.13",
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.5.5",
            "hla-1516.1-2025:clause-8.6.3",
            "hla-1516.1-2025:clause-8.8.3",
            "hla-1516.1-2025:clause-8.12",
            "hla-1516.1-2025:clause-8.12.3",
            "hla-1516.1-2025:clause-10.60.6",
        },
            "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:18614",
        "assertions": 57,
        "primary_lane": "process-flush-queue-galt-frontier",
    },
    "RTIambassadors deliver multiple queued timestamped process messages in order through Flush Queue": {
        "mapping_id": "time-management.process.flush-queue-multiple-records",
        "requirements": 22,
        "sections": {
            "hla-1516.1-2025:clause-4",
            "hla-1516.1-2025:clause-5.1.4",
            "hla-1516.1-2025:clause-6.12.4",
            "hla-1516.1-2025:clause-6.13",
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.5.5",
            "hla-1516.1-2025:clause-8.6.3",
            "hla-1516.1-2025:clause-8.8.3",
            "hla-1516.1-2025:clause-8.12",
            "hla-1516.1-2025:clause-8.12.3",
            "hla-1516.1-2025:clause-8.22.3",
            "hla-1516.1-2025:clause-10.60.6",
        },
            "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:19050",
        "assertions": 110,
        "api_surfaces": 22,
        "primary_lane": "process-flush-queue-multiple-records",
    },
    "Embedded Flush Queue Request flushes queued TSO and reports optimistic time": {
        "mapping_id": "rti.service.flush-queue-request",
        "requirements": 5,
        "sections": {
            "hla-1516.1-2025:clause-8.12",
            "hla-1516.1-2025:clause-8.12.3",
        },
        "source_location": "cpp/tests/flush_queue_request_optimistic_time_catch2.cpp:140",
        "assertions": 58,
    },
    "Embedded constrained TAR waits for GALT and is released by a regulator advance": {
        "mapping_id": "m70.embedded-constrained-tar-galt-regulator-advance",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-8",
        },
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:51829",
        "assertions": 30,
    },
    "Embedded Query GALT and Query LITS observe other regulator time and pending advances": {
        "mapping_id": "time-management.query-galt-lits",
        "requirements": 5,
        "sections": {
            "hla-1516.1-2025:clause-8",
            "hla-1516.1-2025:clause-8.1.5",
            "hla-1516.1-2025:clause-8.18.1",
            "hla-1516.1-2025:clause-8.19.3",
        },
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:51597",
        "assertions": 35,
    },
    "RTIambassador queries GALT and LITS through a configured process endpoint": {
        "mapping_id": "time-management.process.query-galt-lits",
        "requirements": 5,
        "sections": {
            "hla-1516.1-2025:clause-8",
            "hla-1516.1-2025:clause-8.1.5",
            "hla-1516.1-2025:clause-8.18.1",
            "hla-1516.1-2025:clause-8.19.3",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:411",
        "assertions": 12,
    },
    "RTIambassador enables time constrained through a configured process endpoint": {
        "mapping_id": "time-management.process.enable-time-constrained",
        "requirements": 3,
        "sections": {
            "hla-1516.1-2025:clause-8.5.5",
            "hla-1516.1-2025:clause-8.6.3",
            "hla-1516.1-2025:clause-8.7.5",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:503",
        "assertions": 13,
    },
    "RTIambassador requests time advance through a configured process endpoint": {
        "mapping_id": "time-management.process.time-advance-request",
        "requirements": 4,
        "sections": {
            "hla-1516.1-2025:clause-8.8.3",
            "hla-1516.1-2025:clause-8.14.3",
            "hla-1516.1-2025:clause-8.18.1",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:595",
        "assertions": 14,
    },
    "RTIambassador rejects a backward time advance through a configured process endpoint": {
        "mapping_id": "time-management.process.time-advance-rejection",
        "requirements": 4,
        "sections": {
            "hla-1516.1-2025:clause-8.8.3",
            "hla-1516.1-2025:clause-8.14.3",
            "hla-1516.1-2025:clause-8.18.1",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:10835",
        "assertions": 12,
    },
    "RTIambassador rejects an incompatible logical time through a configured process endpoint": {
        "mapping_id": "time-management.process.time-advance-invalid-time",
        "requirements": 1,
        "sections": {
            "hla-1516.1-2025:clause-8.8.3",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:10928",
        "assertions": 9,
    },
    "RTIambassador rejects a process time advance while time regulation enable is pending": {
        "mapping_id": "time-management.process.time-advance-time-regulation-pending",
        "requirements": 3,
        "sections": {
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.3.1",
            "hla-1516.1-2025:clause-8.8.3",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:11012",
        "assertions": 14,
    },
    "RTIambassador rejects a process time advance while time constrained enable is pending": {
        "mapping_id": "time-management.process.time-advance-time-constrained-pending",
        "requirements": 3,
        "sections": {
            "hla-1516.1-2025:clause-8.5.5",
            "hla-1516.1-2025:clause-8.6.3",
            "hla-1516.1-2025:clause-8.8.3",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:11105",
        "assertions": 14,
    },
    "RTIambassador rejects malformed logical-time encoding through a configured process endpoint": {
        "mapping_id": "time-management.process.time-advance-malformed-encoding",
        "requirements": 1,
        "sections": {
            "hla-1516.1-2025:clause-8.8.3",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:11210",
        "assertions": 9,
    },
    "RTIambassadors coordinate deferred process time advances through the federation scheduler": {
        "mapping_id": "time-management.process.time-advance-federation-scheduler",
        "requirements": 4,
        "sections": {
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.5.5",
            "hla-1516.1-2025:clause-8.6.3",
            "hla-1516.1-2025:clause-8.8.3",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:11295",
        "assertions": 30,
    },
    "RTIambassadors deliver a deferred timestamped process interaction before the grant": {
        "mapping_id": "time-management.process.tso-interaction-before-grant",
        "requirements": 20,
        "sections": {
            "hla-1516.1-2025:clause-4",
            "hla-1516.1-2025:clause-5.1.4",
            "hla-1516.1-2025:clause-6.12.4",
            "hla-1516.1-2025:clause-6.13",
            "hla-1516.1-2025:clause-8",
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.5.5",
            "hla-1516.1-2025:clause-8.6.3",
            "hla-1516.1-2025:clause-8.8.3",
            "hla-1516.1-2025:clause-8.1.5",
            "hla-1516.1-2025:clause-8.18.1",
            "hla-1516.1-2025:clause-8.19.3",
            "hla-1516.1-2025:clause-10.60.6",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:11510",
        "assertions": 62,
    },
    "Private process service releases timestamped Update Attribute Values before a constrained grant": {
        "mapping_id": "time-management.process.tso-attribute-before-grant.private",
        "requirements": 12,
        "sections": {
            "hla-1516.1-2025:clause-5.8",
            "hla-1516.1-2025:clause-6.10",
            "hla-1516.1-2025:clause-6.8.4",
            "hla-1516.1-2025:clause-6.9.3",
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.5.5",
            "hla-1516.1-2025:clause-8.6.3",
            "hla-1516.1-2025:clause-8.8.3",
        },
        "source_location": "cpp/tests/process_federation_service_catch2.cpp:128",
        "assertions": 60,
    },
    "RTIambassadors deliver a deferred timestamped process attribute update before the grant": {
        "mapping_id": "time-management.process.tso-attribute-before-grant",
        "requirements": 17,
        "sections": {
            "hla-1516.1-2025:clause-5.8",
            "hla-1516.1-2025:clause-6.10",
            "hla-1516.1-2025:clause-6.8.4",
            "hla-1516.1-2025:clause-6.9.3",
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.5.5",
            "hla-1516.1-2025:clause-8.6.3",
            "hla-1516.1-2025:clause-8.8.3",
            "hla-1516.1-2025:clause-8",
            "hla-1516.1-2025:clause-8.1.5",
            "hla-1516.1-2025:clause-8.18.1",
            "hla-1516.1-2025:clause-8.19.3",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:6829",
        "assertions": 54,
    },
    "RTIambassadors suppress a retracted timestamped process attribute before the callback": {
        "mapping_id": "time-management.process.tso-attribute-retraction-before-callback",
        "requirements": 20,
        "sections": {
            "hla-1516.1-2025:clause-5.8",
            "hla-1516.1-2025:clause-6.10",
            "hla-1516.1-2025:clause-6.8.4",
            "hla-1516.1-2025:clause-6.9.3",
            "hla-1516.1-2025:clause-8",
            "hla-1516.1-2025:clause-8.1.5",
            "hla-1516.1-2025:clause-8.18.1",
            "hla-1516.1-2025:clause-8.19.3",
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.22.3",
            "hla-1516.1-2025:clause-8.5.5",
            "hla-1516.1-2025:clause-8.6.3",
            "hla-1516.1-2025:clause-8.8.3",
        },
            "source_location": "cpp/tests/ieee1516_2025_connection_catch2.cpp:20378",
        "assertions": 51,
        "primary_lane": "process-tso-attribute-retraction-before-callback",
    },
    "Embedded support switches are seeded per federate and retain static FDD policy": {
        "mapping_id": "rti.service.support-switch-state",
        "requirements": 8,
        "sections": {
            "hla-1516.1-2025:clause-8.1.10",
            "hla-1516.1-2025:clause-9.1.8",
            "hla-1516.1-2025:clause-10.44",
            "hla-1516.1-2025:clause-10.45.3",
            "hla-1516.1-2025:clause-10.46.6",
            "hla-1516.1-2025:clause-10.48.1",
            "hla-1516.1-2025:clause-10.50.6",
            "hla-1516.1-2025:clause-10.55.1",
        },
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:28513",
        "assertions": 40,
    },
    "Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries": {
        "mapping_id": "rti.service.whole-object-class-declaration",
        "requirements": 8,
        "sections": {
            "hla-1516.1-2025:clause-5.3",
            "hla-1516.1-2025:clause-5.3.3",
            "hla-1516.1-2025:clause-5.9",
        },
        "source_location": "cpp/tests/fom_declaration_management_catch2.cpp:139",
        "assertions": 34,
    },
    "Embedded Create Federation Execution accepts a validated explicit MIM path": {
        "mapping_id": "rti.service.create-federation-execution-with-mim",
        "requirements": 1,
        "sections": {
            "hla-1516.1-2025:clause-4.5.5",
        },
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:19879",
        "assertions": 6,
    },
    "Embedded federation shares the static Advisories Use Known Class switch": {
        "mapping_id": "rti.service.get-advisories-use-known-class-switch",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-10.55.1",
        },
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:28471",
        "assertions": 15,
    },
    "Embedded Next Message Request grants at the next queued TSO timestamp": {
        "mapping_id": "rti.service.next-message-request",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-8.10.2",
        },
        "source_location": "cpp/tests/next_message_request_queued_tso_catch2.cpp:126",
        "assertions": 33,
    },
    "Embedded Available time advances use inclusive GALT and queued TSO delivery": {
        "mapping_id": "rti.service.time-advance-request-available",
        "requirements": 4,
        "sections": {
            "hla-1516.1-2025:clause-8.9",
            "hla-1516.1-2025:clause-8.11",
            "hla-1516.1-2025:clause-8.11.3",
        },
        "source_location": "cpp/tests/available_time_advance_inclusive_galt_catch2.cpp:126",
        "assertions": 37,
    },
    "Embedded object-class lookup services use stable handles from the joined federation FOM": {
        "mapping_id": "rti.service.get-object-class-handle-name",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-10.4.6",
            "hla-1516.1-2025:clause-10.6.2",
        },
        "source_location": "cpp/tests/object_class_lookup_catch2.cpp:51",
        "assertions": 25,
    },
    "Embedded interaction-class lookup services use stable handles from the joined federation FOM": {
        "mapping_id": "rti.service.get-interaction-class-handle-name",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-10.13.2",
            "hla-1516.1-2025:clause-10.14.5",
        },
        "source_location": "cpp/tests/interaction_class_lookup_catch2.cpp:51",
        "assertions": 25,
    },
    "Embedded attribute lookup resolves inherited definitions in the joined federation FOM": {
        "mapping_id": "rti.service.get-attribute-handle-name",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-10.9.1",
            "hla-1516.1-2025:clause-10.10.3",
        },
        "source_location": "cpp/tests/attribute_lookup_catch2.cpp:51",
        "assertions": 30,
    },
    "Embedded parameter lookup resolves inherited definitions in the joined federation FOM": {
        "mapping_id": "rti.service.get-parameter-handle-name",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-10.16.1",
        },
        "source_location": "cpp/tests/parameter_lookup_catch2.cpp:51",
        "assertions": 28,
    },
    "Embedded 2025 dimension lookup follows FOM hierarchy and upper bounds": {
        "mapping_id": "rti.service.dimension-lookup-foundation",
        "requirements": 4,
        "sections": {
            "hla-1516.1-2025:clause-9.1.2",
        },
        "source_location": "cpp/tests/dimension_lookup_catch2.cpp:53",
        "assertions": 41,
    },
    "Embedded order type lookup exposes the mandatory 2025 Receive and TimeStamp pair": {
        "mapping_id": "rti.service.get-order-type-name",
        "requirements": 5,
        "sections": {
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-10.17.4",
            "hla-1516.1-2025:clause-10.19",
        },
        "source_location": "cpp/tests/order_type_lookup_catch2.cpp:49",
        "assertions": 20,
    },
    "Embedded order type control captures defaults, instance overrides, and publisher interaction order": {
        "mapping_id": "rti.service.order-type-control",
        "requirements": 9,
        "sections": {
            "hla-1516.1-2025:clause-8.24.4",
            "hla-1516.1-2025:clause-8.25.3",
            "hla-1516.1-2025:clause-8.26.4",
        },
        "source_location": "cpp/tests/order_type_control_catch2.cpp:168",
        "assertions": 67,
    },
    "Embedded declaration relevance advisories follow ordinary 2025 publication and subscription transitions": {
        "mapping_id": "rti.service.declaration-relevance-advisories",
        "requirements": 13,
        "sections": {
            "hla-1516.1-2025:clause-5.8",
            "hla-1516.1-2025:clause-5.10.2",
            "hla-1516.1-2025:clause-5.14.3",
            "hla-1516.1-2025:clause-5.15.3",
            "hla-1516.1-2025:clause-5.16.5",
            "hla-1516.1-2025:clause-5.17.6",
        },
        "source_location": "cpp/tests/declaration_relevance_advisory_catch2.cpp:91",
        "assertions": 77,
    },
    "Embedded regional declaration relevance advisories follow active subscriptions": {
        "mapping_id": "rti.service.regional-declaration-relevance-advisories",
        "requirements": 10,
        "sections": {
            "hla-1516.1-2025:clause-5.8",
            "hla-1516.1-2025:clause-5.10.2",
            "hla-1516.1-2025:clause-5.14.3",
            "hla-1516.1-2025:clause-5.15.3",
            "hla-1516.1-2025:clause-5.16.5",
            "hla-1516.1-2025:clause-5.17.6",
        },
        "source_location": "cpp/tests/regional_declaration_relevance_advisory_catch2.cpp:93",
        "assertions": 49,
    },
    "Embedded service reporting records declaration relevance advisories before callbacks": {
        "mapping_id": "rti.service.declaration-relevance-advisories-file",
        "requirements": 6,
        "sections": {
            "hla-1516.1-2025:clause-5.14.3",
            "hla-1516.1-2025:clause-5.15.3",
            "hla-1516.1-2025:clause-5.16.5",
            "hla-1516.1-2025:clause-5.17.6",
            "hla-1516.1-2025:clause-11.5",
            "hla-1516.1-2025:clause-11.5.2",
        },
        "source_location": "cpp/tests/declaration_relevance_service_report_catch2.cpp:163",
        "assertions": 143,
    },
    "Embedded transportation type lookup exposes the mandatory 2025 support pair": {
        "mapping_id": "rti.service.get-transportation-type-handle-name",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-10.19",
            "hla-1516.1-2025:clause-10.20.4",
        },
        "source_location": "cpp/tests/transportation_type_lookup_catch2.cpp:50",
        "assertions": 20,
    },
    "Embedded transportation type lookup resolves a declared FOM transportation per execution": {
        "mapping_id": "rti.service.get-declared-transportation-type-handle-name",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-10.19",
            "hla-1516.1-2025:clause-10.20.4",
        },
        "source_location": "cpp/tests/transportation_type_lookup_catch2.cpp:105",
        "assertions": 17,
    },
}

# This source-backed stress lane does not use a synthetic mapping id, but it
# still needs a stable source pointer and observed Catch2 assertion total so
# the focused DDM query cannot silently regress to an empty evidence card.
SOURCE_ASSERTION_BASELINES = {
    "Embedded two-dimensional regional object updates filter independent attribute sources": {
        "source_location": "cpp/tests/regional_multi_attribute_ddm_catch2.cpp:116",
        "assertions": 78,
        "requirements": 4,
        "sections": {
            "hla-1516.1-2025:clause-9.5",
            "hla-1516.1-2025:clause-9.6",
            "hla-1516.1-2025:clause-9.8",
            "hla-1516.1-2025:clause-9.9.3",
        },
    },
}

# Recovered save/restore rows are deliberately kept as exact query handles.
# This guard prevents an accidental plan merge from dropping their source,
# assertion, or requirement/section mapping while the surrounding roadmap is
# edited incrementally.
RECOVERED_TRACE_CASES = {
    "Filesystem fresh-registry restore rebinds a pending object-instance Request Attribute Value Update": {
        "source_location": "cpp/tests/federation_registry_catch2.cpp:511",
        "assertions": 61,
        "requirements": 17,
        "sections": 8,
        "lane": "process-restart-pending-attribute-value-update",
    },
    "Embedded public fresh-registry restore rebinds pending object-instance Request Attribute Value Update through HLA_EVOKED and HLA_IMMEDIATE": {
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:65634",
        "assertions": 61,
        "requirements": 17,
        "sections": 8,
        "lane": "public-process-restart-pending-attribute-value-update",
    },
    "Filesystem fresh-registry restore rebinds a pending object-class Request Attribute Value Update": {
        "source_location": "cpp/tests/federation_registry_catch2.cpp:673",
        "assertions": 61,
        "requirements": 12,
        "sections": 6,
        "lane": "process-restart-class-pending-attribute-value-update",
    },
    "Embedded public fresh-registry restore rebinds pending object-class Request Attribute Value Update through HLA_EVOKED and HLA_IMMEDIATE": {
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:65891",
        "assertions": 61,
        "requirements": 12,
        "sections": 6,
        "lane": "public-process-restart-class-pending-attribute-value-update",
    },
    "Filesystem fresh-registry restore rebinds a pending regional object-class Request Attribute Value Update": {
        "source_location": "cpp/tests/federation_registry_catch2.cpp:834",
        "assertions": 89,
        "requirements": 11,
        "sections": 6,
        "lane": "process-restart-regional-pending-attribute-value-update-negative",
    },
    "Federation state images round-trip canonical control, temporal, object, and interaction declaration state": {
        "source_location": "cpp/tests/federation_registry_catch2.cpp:2692",
        "assertions": 201,
        "requirements": 33,
        "sections": 28,
        "lane": "membership-lifecycle-state",
    },
    "Federation restore rebinds pending time-role callbacks and fences stale work": {
        "source_location": "cpp/tests/federation_registry_catch2.cpp:3330",
        "assertions": 23,
        "requirements": 4,
        "sections": 4,
        "lane": "time-role",
    },
    "Filesystem state image restores ownership assumption search state and continues with a newly eligible federate": {
        "source_location": "cpp/tests/federation_registry_catch2.cpp:6449",
        "assertions": 83,
        "requirements": 13,
        "sections": 8,
        "lane": "process-restart-ownership-assumption-search",
    },
    "Embedded service reporting delivers failed timestamped regional Update Attribute Values invocations through MOM interaction": {
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:5667",
        "assertions": 138,
        "requirements": 5,
        "sections": 3,
        "lane": "timestamped-regional-attribute-update-failure",
    },
    "Embedded service reporting delivers failed ordinary regional Update Attribute Values invocations through MOM interaction": {
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:7443",
        "assertions": 119,
        "requirements": 4,
        "sections": 2,
        "lane": "ordinary-regional-attribute-update-failure",
    },
    "Embedded service reporting records regional Update Attribute Values before reflection callback": {
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:7931",
        "assertions": 139,
        "requirements": 1,
        "sections": 1,
        "lane": "ordinary-regional-attribute-update-service-report",
    },
    "Embedded queued timestamped Delete Object Instance survives time-regulation disable and re-enable with changed lookahead": {
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:10433",
        "assertions": 52,
        "requirements": 15,
        "sections": 10,
        "lane": "timestamped-object-deletion-regulation-reenable-changed-lookahead",
    },
    "Embedded queued timestamped regional interaction survives time-regulation disable and re-enable with changed lookahead": {
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:14251",
        "assertions": 63,
        "requirements": 12,
        "sections": 9,
        "lane": "timestamped-regional-interaction-regulation-reenable-changed-lookahead",
    },
}


def assert_no_duplicate_json_keys(path: Path) -> None:
    """Reject ambiguous plan/index objects before query assertions run."""

    duplicates: list[str] = []

    def object_pairs_hook(pairs: list[tuple[str, object]]) -> dict[str, object]:
        seen: set[str] = set()
        result: dict[str, object] = {}
        for key, value in pairs:
            if key in seen:
                duplicates.append(key)
            seen.add(key)
            result[key] = value
        return result

    json.loads(
        path.read_text(encoding="utf-8"),
        object_pairs_hook=object_pairs_hook,
    )
    if duplicates:
        names = ", ".join(sorted(set(duplicates)))
        raise AssertionError(f"{path} contains duplicate JSON keys: {names}")


MAPPED_SHARED_CASE = {
    "test_case": "Embedded federation-list services dispatch standards reports in both callback models",
    "source_location": "cpp/tests/federation_listing_catch2.cpp:77",
    "assertions": 41,
    "callback_models": {"HLA_EVOKED", "HLA_IMMEDIATE"},
    "rows": {
        "rti.service.list-federation-executions": {
            "requirements": 2,
            "sections": {"hla-1516.1-2025:clause-4.8.4"},
        },
        "rti.service.list-federation-execution-members": {
            "requirements": 2,
            "sections": {"hla-1516.1-2025:clause-4.10.3"},
        },
    },
}


def main() -> int:
    assert_no_duplicate_json_keys(query_rti_work.DEFAULT_INDEX)
    assert_no_duplicate_json_keys(query_rti_work.DEFAULT_PLAN)
    assert_no_duplicate_json_keys(query_rti_work.DEFAULT_LAB_ISSUES)
    index = query_rti_work.load_json(query_rti_work.DEFAULT_INDEX)
    plan = query_rti_work.load_json(query_rti_work.DEFAULT_PLAN)
    bundle = query_rti_work.load_json(query_rti_work.DEFAULT_BUNDLE)
    standard_requirements = query_rti_work.load_standard_requirements(bundle)
    contract_links = query_rti_work.load_contract_links(
        query_rti_work.DEFAULT_CONTRACT_DIRECTORY
    )
    contract_test_candidates = query_rti_work.load_contract_test_candidates(
        query_rti_work.DEFAULT_CONTRACT_DIRECTORY
    )
    source_locations, source_health = query_rti_work.load_test_source_locations(
        query_rti_work.DEFAULT_TEST_ROOT,
        include_health=True,
    )
    unbalanced_source_paths = {
        sample.get("path")
        for sample in source_health.get("unbalanced_conditional_samples", [])
        if isinstance(sample, dict) and isinstance(sample.get("path"), str)
    }
    tests = query_rti_work.all_mapped_tests(
        plan,
        standard_requirements,
        contract_links,
        source_locations,
    )
    # The completion ledger is intentionally historical for assertion/status
    # metrics, but its source pointer must remain navigable after a fixture is
    # edited or split.  Reconcile only rows that still have a live plan id and
    # source location; retired historical rows remain audit-only.
    plan_source_locations = {
        row.get("id"): row.get("source_location")
        for row in plan.get("tests", [])
        if isinstance(row, dict)
        and isinstance(row.get("id"), str)
        and isinstance(row.get("source_location"), str)
    }
    historical_source_drift = [
        (
            row.get("plan_id"),
            row.get("source_location"),
            plan_source_locations.get(row.get("plan_id")),
        )
        for row in index.get("recent_completed_slices", [])
        if isinstance(row, dict)
        and row.get("plan_id") in plan_source_locations
        and isinstance(row.get("source_location"), str)
        and row.get("source_location") != plan_source_locations.get(row.get("plan_id"))
    ]
    if historical_source_drift:
        raise AssertionError(
            "completion-ledger source pointers drifted from the live Catch2 plan: "
            + "; ".join(
                f"{plan_id}: {old} != {new}"
                for plan_id, old, new in historical_source_drift[:4]
            )
        )
    contract_drift = query_rti_work.contract_drift_report(
        query_rti_work.DEFAULT_CONTRACT_DIRECTORY,
        source_locations,
        limit=8,
    )
    if (
        contract_drift.get("status") != "clean"
        or contract_drift.get("finding_count") != 0
        or contract_drift.get("native_reference_count")
        != contract_drift.get("clean_reference_count")
        or contract_drift.get("external_reference_count", 0) <= 0
    ):
        raise AssertionError(
            "local contract-selector drift guard or external classification is not clean"
        )
    filtered_contract_drift = query_rti_work.contract_drift_report(
        query_rti_work.DEFAULT_CONTRACT_DIRECTORY,
        source_locations,
        query="timestamped-directed-interaction",
        limit=0,
    )
    if (
        filtered_contract_drift.get("status") != "clean"
        or not filtered_contract_drift.get("contract_count")
        or filtered_contract_drift.get("external_reference_count", 0) <= 0
    ):
        raise AssertionError(
            "contract-selector drift query filter lost its bounded scope or external classification"
        )

    matches = [test for test in tests if test.get("test_case") == TARGET_TEST]
    if len(matches) != 1:
        raise AssertionError(
            f"expected one exact target test, found {len(matches)}"
        )
    target = matches[0]
    matrix = query_rti_work.matrix_summary_data(target, index)
    pairs = matrix.get("requirement_section_mappings")
    if not isinstance(pairs, list) or not pairs:
        raise AssertionError("matrix row has no direct requirement/section mappings")
    if matrix.get("requirement_section_mapping_count") != len(pairs):
        raise AssertionError("matrix mapping count does not match its mapping rows")

    expected_requirement_ids = set(target.get("lab_requirement_ids", []))
    observed_requirement_ids = {
        row.get("lab_requirement_id")
        for row in pairs
        if isinstance(row, dict)
    }
    if observed_requirement_ids != expected_requirement_ids:
        raise AssertionError(
            "matrix requirement IDs do not match the mapped test: "
            f"expected {len(expected_requirement_ids)}, observed {len(observed_requirement_ids)}"
        )

    expected_sections = set(target.get("standard_sections", []))
    observed_sections = {
        row.get("standard_section")
        for row in pairs
        if isinstance(row, dict) and row.get("standard_section")
    }
    if not observed_sections.issubset(expected_sections):
        raise AssertionError(
            "matrix contains a section absent from the test's canonical section set"
        )
    if any(
        row.get("unresolved")
        or not row.get("standard_section")
        for row in pairs
        if isinstance(row, dict)
    ):
        raise AssertionError("the target's resolved mappings unexpectedly contain an unresolved row")

    text = query_rti_work.text_matrix_row(target, index, compact=True)
    if "requirement_section_mappings:" not in text:
        raise AssertionError("bounded matrix text omitted direct mapping pairs")
    if "roadmap:" not in text:
        raise AssertionError("bounded matrix text omitted roadmap ownership/context")

    summary = query_rti_work.test_summary_data(target)
    summary_pairs = summary.get("requirement_section_mappings")
    if summary.get("requirement_section_mapping_count") != len(pairs):
        raise AssertionError(
            "summary mapping count diverges from the direct matrix mapping"
        )
    if summary_pairs != pairs:
        raise AssertionError(
            "summary rows do not expose the same direct requirement/section pairs"
        )
    summary_text = query_rti_work.text_test_summary(target)
    if "requirement -> standard subsection:" not in summary_text:
        raise AssertionError(
            "bounded test summary omitted the direct requirement/subsection handle"
        )
    reverse_summary_text = query_rti_work.text_query_summary_row(
        "section",
        target,
        index,
        "8.8.3",
    )
    if "requirement_section_mappings:" not in reverse_summary_text:
        raise AssertionError(
            "section reverse-summary omitted direct requirement/subsection pairs"
        )
    mapping_line = next(
        (
            line
            for line in reverse_summary_text.splitlines()
            if line.startswith("  requirement_section_mappings:")
        ),
        "",
    )
    if mapping_line.count(" -> ") != 2 or "clause-8.8.3" not in mapping_line:
        raise AssertionError(
            "section reverse-summary did not narrow direct pairs to the requested subsection"
        )
    if len(reverse_summary_text.splitlines()) >= len(summary_text.splitlines()):
        raise AssertionError(
            "section reverse-summary did not stay smaller than the full test summary"
        )

    # Protect the focused ownership-management reverse lookups that are used
    # as the next bounded handoff.  These exact IDs must continue to resolve
    # to the source-backed Catch2 cases without a Requirements-Lab rescan.
    for requirement_id, expected_test_id in (
        (
            "requirement-candidate-content-clauses-07-ownership-management-page-154-l99-31",
            "umbra-cpp-unconditional-attribute-ownership-divestiture-standalone",
        ),
        (
            "requirement-candidate-content-clauses-07-ownership-management-page-154-l102-32",
            "umbra-cpp-unconditional-attribute-ownership-divestiture-standalone",
        ),
        (
            "requirement-candidate-content-clauses-07-ownership-management-page-154-l105-33",
            "umbra-cpp-unconditional-attribute-ownership-divestiture-standalone",
        ),
        (
            "requirement-candidate-content-clauses-07-ownership-management-page-154-l51-16",
            "umbra-cpp-negotiated-willing-to-acquire-integration",
        ),
        (
            "requirement-candidate-content-clauses-07-ownership-management-page-154-l9-2",
            "umbra-cpp-negotiated-assumption-integration",
        ),
        (
            "requirement-candidate-content-clauses-07-ownership-management-page-154-l117-36",
            "umbra-cpp-unconditional-attribute-ownership-divestiture-standalone",
        ),
        (
            "requirement-candidate-content-clauses-07-ownership-management-page-154-l135-42",
            "umbra-cpp-unconditional-attribute-ownership-divestiture-standalone",
        ),
        (
            "requirement-candidate-content-clauses-07-ownership-management-page-154-l144-45",
            "umbra-cpp-unconditional-attribute-ownership-divestiture-standalone",
        ),
    ):
        requirement_matches = query_rti_work.select_tests(
            tests,
            requirement_id,
            "requirement",
        )
        if not any(
            match.get("id") == expected_test_id for match in requirement_matches
        ):
            raise AssertionError(
                f"exact ownership requirement lookup lost {requirement_id}"
            )

    roadmap_entries = query_rti_work.roadmap_checklist(
        query_rti_work.DEFAULT_ROADMAP
    )
    dashboard = query_rti_work.dashboard_snapshot(
        index,
        tests,
        roadmap_entries,
        source_locations,
        source_health,
        limit=3,
        standard_requirements=standard_requirements,
    )
    if dashboard.get("plan_counts", {}).get("catch2_plan_cases") != len(tests):
        raise AssertionError("dashboard plan count diverged from the mapped plan")
    resume_card = dashboard.get("resume_card")
    if not isinstance(resume_card, dict) or not resume_card.get("first_query"):
        raise AssertionError("dashboard omitted the persisted resume card")
    if "lab-issues" not in str(resume_card.get("issue_query") or ""):
        raise AssertionError("resume card omitted the Requirements Lab issue query")
    if "check" not in str(resume_card.get("coverage_query") or "") or "gaps" not in str(
        resume_card.get("gap_query") or ""
    ):
        raise AssertionError("resume card omitted coverage/gap query handles")
    for lookup_handle in ("section_query", "requirement_query", "matrix_query"):
        if "--summary --compact" not in str(resume_card.get(lookup_handle) or ""):
            raise AssertionError(
                f"resume card omitted bounded {lookup_handle.replace('_', ' ')} handle"
            )
    deferred_slices = dashboard.get("deferred_slices")
    if not isinstance(deferred_slices, list) or not deferred_slices:
        raise AssertionError("dashboard omitted the indexed deferred-slice seam")
    deferred = deferred_slices[0]
    if (
        not isinstance(deferred, dict)
        or deferred.get("id") != "process-tso-in-transit"
        or deferred.get("lane") != "process-tso-in-transit"
        or deferred.get("state") != "evidence-complete"
        or "stress" not in str(deferred.get("next_step") or "").casefold()
        or "retraction" not in str(deferred.get("next_step") or "").casefold()
    ):
        raise AssertionError("dashboard deferred process TSO seam drifted")
    dashboard_health = dashboard.get("source_health", {})
    if not isinstance(dashboard_health, dict) or not isinstance(
        dashboard_health.get("files_with_unbalanced_conditionals"), int
    ):
        raise AssertionError("dashboard omitted preprocessor source-health counters")
    if not isinstance(
        dashboard.get("index_snapshot", {}).get("matches_live"), bool
    ):
        raise AssertionError("dashboard omitted traceability snapshot freshness")
    dashboard_latest = dashboard.get("latest_completed_slice")
    if not isinstance(dashboard_latest, dict):
        raise AssertionError("dashboard omitted the latest bounded slice card")
    if dashboard_latest.get("lane") != "process-boundary":
        raise AssertionError("dashboard latest slice lane drifted")
    for latest_handle in (
        "case_command",
        "focus_command",
        "trace_command",
        "matrix_command",
        "check_command",
        "ctest_command",
    ):
        if not isinstance(dashboard_latest.get(latest_handle), str):
            raise AssertionError(
                f"dashboard latest slice omitted {latest_handle}"
            )
    active_pointer = dashboard.get("active_pointer")
    if (
        not isinstance(active_pointer, dict)
        or active_pointer.get("work_id") != "transport-and-conformance"
        or "public process transport baseline"
        not in str(active_pointer.get("task") or "")
        or "63 mapped" in str(active_pointer.get("task") or "")
        or "63 mapped" in str(active_pointer.get("work_query") or "")
    ):
        raise AssertionError(
            "dashboard active pointer did not prefer the current target-family action"
        )
    dashboard_queue = dashboard.get("queue", {})
    if dashboard_queue.get("shown_count") != min(
        3, dashboard_queue.get("open_count", 0)
    ):
        raise AssertionError("dashboard queue preview exceeded its bounded limit")
    for queue_row in dashboard_queue.get("items", []):
        if not isinstance(queue_row, dict):
            continue
        work_query = queue_row.get("work_query")
        if isinstance(work_query, str) and len(work_query) > 240:
            raise AssertionError(
                "dashboard queue preview leaked unbounded family prose"
            )
    dashboard_handles = dashboard.get("handles", {})
    if (
        not dashboard_handles.get("ready")
        or not dashboard_handles.get("check")
        or "lab-issues" not in str(dashboard_handles.get("lab_issues") or "")
    ):
        raise AssertionError("dashboard omitted its copyable resume/check handles")
    if "case" not in str(resume_card.get("case_query") or ""):
        raise AssertionError("resume card omitted the exact case handoff")
    dashboard_next = dashboard.get("next")
    if not isinstance(dashboard_next, dict):
        raise AssertionError("dashboard omitted the active indexed work handoff")
    if not dashboard_next.get("work_id"):
        if dashboard_next.get("state") != "none" or not dashboard_next.get("family_options"):
            raise AssertionError("dashboard omitted the bounded family fallback handoff")
        if dashboard_next.get("recommended_family_id") != dashboard_next.get(
            "family_options", [{}]
        )[0].get("id"):
            raise AssertionError("dashboard omitted its bounded family recommendation")
        dashboard_family = dashboard_next.get("family_options", [])[0]
        if (
            not isinstance(dashboard_family, dict)
            or dashboard_family.get("action_state") != "new-case-needed"
            or not isinstance(dashboard_family.get("gap_preview"), dict)
        ):
            raise AssertionError(
                "dashboard family fallback omitted its bounded gap preview"
            )
    if dashboard_next.get("lane") and not dashboard_handles.get("focus"):
        raise AssertionError("dashboard omitted the active lane focus handle")
    if dashboard_next.get("state") == "unplanned-source":
        if not dashboard_next.get("test_case") or not dashboard_next.get(
            "source_location"
        ):
            raise AssertionError(
                "dashboard omitted the exact source declaration handoff"
            )
        if "Add an explicit Catch2 plan row" not in str(
            dashboard_next.get("next_action") or ""
        ):
            raise AssertionError(
                "dashboard unplanned-source handoff omitted its plan-row action"
            )
    source_reconciliation = dashboard.get("source_only_reconciliation")
    if not isinstance(source_reconciliation, dict):
        raise AssertionError("dashboard omitted the bounded source-only card")
    if source_reconciliation.get("count", 0):
        if not source_reconciliation.get("test_query") or not source_reconciliation.get(
            "source_location"
        ):
            raise AssertionError(
                "dashboard source-only card omitted its queue head"
            )
        if "--include-source-only" not in str(
            source_reconciliation.get("ready_command") or ""
        ):
            raise AssertionError(
                "dashboard source-only card omitted its opt-in command"
            )

    resume = query_rti_work.resume_snapshot(
        index,
        tests,
        roadmap_entries,
        source_locations,
        source_health,
        limit=1,
        standard_requirements=standard_requirements,
    )
    if resume.get("schema_version") != 1 or resume.get("bounded") is not True:
        raise AssertionError("compact resume card lost its bounded schema marker")
    resume_queue = resume.get("queue", {})
    if not isinstance(resume_queue, dict) or resume_queue.get("shown_count") != 1:
        raise AssertionError("compact resume card leaked more than one queue row")
    resume_next = resume.get("next")
    if not isinstance(resume_next, dict):
        raise AssertionError("compact resume card omitted its next handoff")
    if resume_next.get("state") == "none":
        options = resume_next.get("family_options")
        if not isinstance(options, list) or len(options) != 1:
            raise AssertionError("compact resume card did not bound family choices")
        if resume_next.get("recommended_family_id") != options[0].get("id"):
            raise AssertionError("compact resume card lost its recommended family")
        if resume_next.get("family_options_remaining") != 1:
            raise AssertionError("compact resume family-choice count drifted")
        if not isinstance(options[0].get("gap_preview"), dict):
            raise AssertionError("compact resume card omitted its bounded gap preview")
    resume_handles = resume.get("handles")
    if not isinstance(resume_handles, dict) or not resume_handles.get("ready"):
        raise AssertionError("compact resume card omitted the ready handle")
    resume_deferred = resume.get("deferred_slices")
    if not isinstance(resume_deferred, list) or not resume_deferred:
        raise AssertionError("compact resume card omitted the deferred-slice seam")

    lab_issues = query_rti_work.lab_issues_snapshot()
    if lab_issues.get("count") != 1 or lab_issues.get("shown_count") != 1:
        raise AssertionError("Requirements Lab issue ledger count drifted")
    issue = lab_issues.get("issues", [{}])[0]
    if (
        issue.get("id") != "RL-177"
        or issue.get("exported_clause") != "6.18.1"
        or issue.get("normative_clause") != "6.17.4"
    ):
        raise AssertionError("RL-177 clause-boundary issue lost its evidence")
    issue_lookup = query_rti_work.lab_issues_snapshot(query="6.17.4")
    if issue_lookup.get("count") != 1:
        raise AssertionError("Requirements Lab issue clause lookup drifted")

    # The uncovered-requirement card is the bounded forward bridge when all
    # existing source rows are complete. Protect its global totals and one
    # clause filter so selecting a new C++ case does not require a full Lab
    # export/search. The negotiated-assumption slice closes the former
    # clause-7.2 gap and the regional interaction invalid-context slice closes
    # the selected clause-9.1.3.3 gap; the regional routing row and focused
    # zero-dimensional object and interaction cases carry the bounded
    # clause-9.1.3.2 evidence. The empty regional subscription-set slice
    # closes two existing clause-9.1.4 requirements and the region-based
    # receiving rule, the unavailable-dimension subscription slice closes one
    # more, the multi-region row carries the bounded update/subscription-set
    # and positive-dimensional-overlap rules, the whole-class unsubscribe
    # slice closes the default-region removal requirement, and the latest
    # no-common-dimension object-attribute slice closes line 68, the
    # time-axis-independence TAR/NMR slice closes line 89, and the current
    # strict-relaxed-ddm-boundary slice closes line 71. The service-reporting
    # interlock slice now also maps the enabled-subscription prohibition in
    # clause 11.5.1; the exact fallback lookup below stays on a separate
    # uncovered MOM requirement.
    gaps = query_rti_work.requirement_gap_inventory(
        standard_requirements,
        tests,
        limit=8,
    )
    if gaps.get("total_requirement_count") != 2220:
        raise AssertionError("2025 requirement gap total drifted")
    if gaps.get("covered_requirement_count") != 855:
        raise AssertionError("2025 requirement mapped total drifted")
    if gaps.get("uncovered_requirement_count") != 1365:
        raise AssertionError("2025 uncovered requirement total drifted")
    if gaps.get("coverage_percent") != 38.51:
        raise AssertionError("2025 requirement coverage percentage drifted")
    clause_gaps = query_rti_work.requirement_gap_inventory(
        standard_requirements,
        tests,
        query="hla-1516.1-2025:clause-7.2",
        limit=8,
    )
    if clause_gaps.get("uncovered_requirement_count") != 0:
        raise AssertionError("clause-7.2 uncovered requirement count drifted")
    family_clause_gaps = query_rti_work.requirement_gap_inventory(
        standard_requirements,
        tests,
        index=index,
        family="object-ddm-ownership",
        clause="clause-9.1.3.3",
        limit=8,
    )
    if (
        family_clause_gaps.get("total_requirement_count") != 23
        or family_clause_gaps.get("covered_requirement_count") != 23
        or family_clause_gaps.get("uncovered_requirement_count") != 0
    ):
        raise AssertionError(
            "family-scoped clause-9.1.3.3 gap counts drifted"
        )
    family_clause_9132_gaps = query_rti_work.requirement_gap_inventory(
        standard_requirements,
        tests,
        index=index,
        family="object-ddm-ownership",
        clause="clause-9.1.3.2",
        limit=8,
    )
    if (
        family_clause_9132_gaps.get("total_requirement_count") != 12
        or family_clause_9132_gaps.get("covered_requirement_count") != 12
        or family_clause_9132_gaps.get("uncovered_requirement_count") != 0
    ):
        raise AssertionError(
            "family-scoped clause-9.1.3.2 gap counts drifted"
        )
    gap_summary = query_rti_work.requirement_gap_summary_data(gaps)
    if gap_summary.get("shown_requirement_count") != 0 or gap_summary.get(
        "requirements"
    ) != []:
        raise AssertionError(
            "gap summary expanded full requirement records instead of staying bounded"
        )
    uncovered_lookup = query_rti_work.requirement_gap_inventory(
        standard_requirements,
        tests,
        query="requirement-candidate-content-clauses-11-management-object-model-page-293-l89-29",
        limit=20,
    )
    if uncovered_lookup.get("uncovered_requirement_count") != 1:
        raise AssertionError("exact uncovered requirement lookup drifted")
    uncovered_records = uncovered_lookup.get("requirements", [])
    if len(uncovered_records) != 1:
        raise AssertionError("exact uncovered requirement lookup omitted its record")
    uncovered_record = uncovered_records[0]
    uncovered_source = uncovered_record.get("source")
    if (
        uncovered_record.get("coverage") != "uncovered"
        or uncovered_record.get("standard_section")
        != "hla-1516.1-2025:clause-11.5.2"
        or not uncovered_record.get("statement")
        or not isinstance(uncovered_source, dict)
        or not uncovered_source.get("path")
    ):
        raise AssertionError(
            "exact uncovered requirement lookup omitted normative trace fields"
        )
    lookup_summary = query_rti_work.requirement_gap_lookup_summary_data(
        uncovered_lookup,
        preview_limit=8,
    )
    if lookup_summary.get("shown_requirement_count") != 1 or lookup_summary.get(
        "requirement_preview_truncated"
    ):
        raise AssertionError("uncovered requirement lookup summary lost its bounded preview")

    # Roadmap family search must also be a reverse mapping entry point.  An
    # exact Lab requirement, canonical 2025 subsection, or official C++ API
    # surface should resolve to the owning family without a broad plan dump.
    for query, expected_family in (
        (
            "requirement-candidate-content-clauses-06-object-management-page-112-l75-22",
            "object-ddm-ownership",
        ),
        ("hla-1516.1-2025:clause-6.1.13", "object-ddm-ownership"),
        ("api.2025.cpp.rtiambassador.updateattributevalues", "object-ddm-ownership"),
    ):
        roadmap_matches = query_rti_work.roadmap_inventory(
            index,
            tests,
            query=query,
            status="open",
            limit=0,
        )
        if not any(
            family.get("id") == expected_family
            for family in roadmap_matches.get("families", [])
            if isinstance(family, dict)
        ):
            raise AssertionError(
                f"roadmap mapping search did not resolve {query}"
            )
    regional_lane_families = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process-multi-recipient-regional-interaction",
        status="open",
        limit=0,
    ).get("families", [])
    if [
        family.get("id")
        for family in regional_lane_families
        if isinstance(family, dict)
    ] != ["transport-and-conformance"]:
        raise AssertionError(
            "regional process lane is not owned by exactly one roadmap family"
        )
    negotiated_if_available_lane_families = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="public-process-restart-attribute-ownership-negotiated-if-available-owner-confirmation",
        status="open",
        limit=0,
    ).get("families", [])
    if [
        family.get("id")
        for family in negotiated_if_available_lane_families
        if isinstance(family, dict)
    ] != ["time-save-restore"]:
        raise AssertionError(
            "public negotiated If Available lane is not owned by exactly one roadmap family"
        )
    negotiated_if_available_lane_match = (
        negotiated_if_available_lane_families[0].get("lane_matches", [{}])[0]
        if negotiated_if_available_lane_families
        and isinstance(negotiated_if_available_lane_families[0], dict)
        and negotiated_if_available_lane_families[0].get("lane_matches")
        else {}
    )
    if (
        negotiated_if_available_lane_match.get("test_count") != 1
        or negotiated_if_available_lane_match.get("mapped_count") != 1
        or negotiated_if_available_lane_match.get("requirement_count") != 19
        or negotiated_if_available_lane_match.get("standard_section_count") != 12
    ):
        raise AssertionError(
            "public negotiated If Available lane mapping card drifted"
        )
    if_available_lane_families = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="public-process-restart-attribute-ownership-acquisition-if-available",
        status="open",
        limit=0,
    ).get("families", [])
    if [
        family.get("id")
        for family in if_available_lane_families
        if isinstance(family, dict)
    ] != ["time-save-restore"]:
        raise AssertionError(
            "public If Available ownership lane is not owned by exactly one roadmap family"
        )
    if_available_lane_match = (
        if_available_lane_families[0].get("lane_matches", [{}])[0]
        if if_available_lane_families
        and isinstance(if_available_lane_families[0], dict)
        and if_available_lane_families[0].get("lane_matches")
        else {}
    )
    if (
        if_available_lane_match.get("test_count") != 1
        or if_available_lane_match.get("mapped_count") != 1
        or if_available_lane_match.get("requirement_count") != 11
        or if_available_lane_match.get("standard_section_count") != 7
    ):
        raise AssertionError(
            "public If Available ownership lane mapping card drifted"
        )
    post_confirmation_lane_families = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="public-process-restart-confirm-divestiture-post-confirmation-resignation",
        status="open",
        limit=0,
    ).get("families", [])
    if [
        family.get("id")
        for family in post_confirmation_lane_families
        if isinstance(family, dict)
    ] != ["time-save-restore"]:
        raise AssertionError(
            "public post-confirmation resignation lane is not owned by exactly one roadmap family"
        )
    post_confirmation_lane_match = (
        post_confirmation_lane_families[0].get("lane_matches", [{}])[0]
        if post_confirmation_lane_families
        and isinstance(post_confirmation_lane_families[0], dict)
        and post_confirmation_lane_families[0].get("lane_matches")
        else {}
    )
    if (
        post_confirmation_lane_match.get("test_count") != 3
        or post_confirmation_lane_match.get("mapped_count") != 3
        or post_confirmation_lane_match.get("requirement_count") != 26
        or post_confirmation_lane_match.get("standard_section_count") != 16
    ):
        raise AssertionError(
            "public post-confirmation resignation lane mapping card drifted"
        )
    resignation_lane_families = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="public-process-restart-confirm-divestiture-resignation",
        status="open",
        limit=0,
    ).get("families", [])
    if [
        family.get("id")
        for family in resignation_lane_families
        if isinstance(family, dict)
    ] != ["time-save-restore"]:
        raise AssertionError(
            "public Confirm Divestiture resignation lane is not owned by exactly one roadmap family"
        )
    resignation_lane_match = (
        resignation_lane_families[0].get("lane_matches", [{}])[0]
        if resignation_lane_families
        and isinstance(resignation_lane_families[0], dict)
        and resignation_lane_families[0].get("lane_matches")
        else {}
    )
    if (
        resignation_lane_match.get("test_count") != 4
        or resignation_lane_match.get("mapped_count") != 4
        or resignation_lane_match.get("requirement_count") != 26
        or resignation_lane_match.get("standard_section_count") != 16
    ):
        raise AssertionError(
            "public Confirm Divestiture resignation lane mapping card drifted"
        )
    time_family = next(
        family
        for family in query_rti_work.roadmap_inventory(
            index, tests, query="time-save-restore", status="open", limit=0
        ).get("families", [])
        if isinstance(family, dict) and family.get("id") == "time-save-restore"
    )
    if time_family.get("next_source_state") != "exhausted":
        raise AssertionError("roadmap family row omitted its exhausted source queue state")
    if time_family.get("next_source_test_query") is not None:
        raise AssertionError("roadmap family retained an exhausted source queue pointer")
    if time_family.get("next_source_location") is not None:
        raise AssertionError("roadmap family retained an exhausted source queue location")
    if time_family.get("next_source_lane") != "save-restore":
        raise AssertionError("roadmap family source queue lane drifted")

    cmake_text = (REPOSITORY_ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    focused_target = (
        "umbra_connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient_catch2"
    )
    focused_prefix = (
        "umbra.connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient.catch2."
    )
    if focused_target not in cmake_text or focused_prefix not in cmake_text:
        raise AssertionError(
            "connection-loss automatic-cleanup slice lost its focused CMake target"
        )

    # Keep a small, explicit guard around the recently mapped core service
    # slices. These are the first handles a contributor is likely to resume;
    # if a plan edit drops their direct mapping, source pointer, or assertion
    # count, the bounded query regression should fail immediately.
    for test_case, expected in MAPPED_CORE_CASES.items():
        core_matches = [
            test
            for test in tests
            if test.get("test_case") == test_case
            and test.get("requirements_lab_mapping_id") == expected["mapping_id"]
        ]
        if len(core_matches) != 1:
            raise AssertionError(
                f"expected one mapped core row for {test_case!r} with mapping "
                f"{expected['mapping_id']!r}, found {len(core_matches)}"
            )
        core = core_matches[0]
        if core.get("requirements_lab_mapping_id") != expected["mapping_id"]:
            raise AssertionError(f"mapping id drifted for {test_case!r}")
        if query_rti_work.traceability_state(core) != "requirements-mapped":
            raise AssertionError(f"core case lost direct requirements mapping: {test_case!r}")
        if len(core.get("lab_requirement_ids", [])) != expected["requirements"]:
            raise AssertionError(f"requirement count drifted for {test_case!r}")
        if "api_surfaces" in expected and len(core.get("selected_cpp_api_surface_ids", [])) != expected["api_surfaces"]:
            raise AssertionError(f"C++ API-surface count drifted for {test_case!r}")
        if set(core.get("standard_sections", [])) != expected["sections"]:
            raise AssertionError(f"standard-section keys drifted for {test_case!r}")
        expected_source = expected["source_location"]
        expected_path = expected_source.rsplit(":", 1)[0]
        actual_source = query_rti_work.source_location_text(core)
        if expected_path in unbalanced_source_paths:
            # The aggregate federation-management translation unit is a
            # deliberately retained diagnostic artifact with an unmatched
            # preprocessor conditional. Its declaration line is not a stable
            # executable pointer, but its path must still remain visible so
            # source health cannot be mistaken for Lab drift.
            actual_paths = {
                str(location.get("path") or "")
                for location in core.get("source_locations", [])
                if isinstance(location, dict)
            }
            if expected_path not in actual_paths:
                raise AssertionError(
                    f"unbalanced source path disappeared for {test_case!r}: {actual_source}"
                )
        elif actual_source != expected_source:
            raise AssertionError(f"source pointer drifted for {test_case!r}")
        if core.get("assertions") != expected["assertions"]:
            raise AssertionError(f"assertion count drifted for {test_case!r}")
        expected_primary_lane = expected.get("primary_lane")
        if (
            expected_primary_lane is not None
            and core.get("primary_lane") != expected_primary_lane
        ):
            raise AssertionError(f"primary lane drifted for {test_case!r}")

    for test_case, expected in SOURCE_ASSERTION_BASELINES.items():
        source_matches = [test for test in tests if test.get("test_case") == test_case]
        if len(source_matches) != 1:
            raise AssertionError(
                f"expected one source-backed baseline for {test_case!r}, "
                f"found {len(source_matches)}"
            )
        source_case = source_matches[0]
        if query_rti_work.source_location_text(source_case) != expected["source_location"]:
            raise AssertionError(f"source-backed pointer drifted for {test_case!r}")
        if source_case.get("assertions") != expected["assertions"]:
            raise AssertionError(f"source-backed assertion count drifted for {test_case!r}")
        if len(source_case.get("lab_requirement_ids", [])) != expected["requirements"]:
            raise AssertionError(f"source-backed requirement count drifted for {test_case!r}")
        if set(source_case.get("standard_sections", [])) != expected["sections"]:
            raise AssertionError(f"source-backed section keys drifted for {test_case!r}")

    for test_case, expected in RECOVERED_TRACE_CASES.items():
        recovered_matches = [
            test for test in tests if test.get("test_case") == test_case
        ]
        if len(recovered_matches) != 1:
            raise AssertionError(
                f"expected one recovered trace row for {test_case!r}, "
                f"found {len(recovered_matches)}"
            )
        recovered = recovered_matches[0]
        if query_rti_work.source_location_text(recovered) != expected["source_location"]:
            raise AssertionError(f"recovered source pointer drifted for {test_case!r}")
        if recovered.get("assertions") != expected["assertions"]:
            raise AssertionError(f"recovered assertion count drifted for {test_case!r}")
        if len(recovered.get("lab_requirement_ids", [])) != expected["requirements"]:
            raise AssertionError(f"recovered requirement count drifted for {test_case!r}")
        if len(recovered.get("standard_sections", [])) != expected["sections"]:
            raise AssertionError(f"recovered section count drifted for {test_case!r}")
        if expected["lane"] not in recovered.get("tags", []):
            raise AssertionError(f"recovered lane tag drifted for {test_case!r}")
        recovered_pairs = query_rti_work.matrix_summary_data(recovered, index).get(
            "requirement_section_mappings", []
        )
        if len(recovered_pairs) != expected["requirements"]:
            raise AssertionError(f"recovered direct mapping count drifted for {test_case!r}")

    shared_matches = [
        test
        for test in tests
        if test.get("test_case") == MAPPED_SHARED_CASE["test_case"]
    ]
    if len(shared_matches) != len(MAPPED_SHARED_CASE["rows"]):
        raise AssertionError(
            "federation-listing shared test rows drifted: "
            f"expected {len(MAPPED_SHARED_CASE['rows'])}, found {len(shared_matches)}"
        )
    if any(
        query_rti_work.source_location_text(test) != MAPPED_SHARED_CASE["source_location"]
        or test.get("assertions") != MAPPED_SHARED_CASE["assertions"]
        or set(test.get("callback_models", [])) != MAPPED_SHARED_CASE["callback_models"]
        for test in shared_matches
    ):
        raise AssertionError("federation-listing shared source evidence drifted")
    for mapping_id, expected in MAPPED_SHARED_CASE["rows"].items():
        row = next(
            (
                test
                for test in shared_matches
                if test.get("requirements_lab_mapping_id") == mapping_id
            ),
            None,
        )
        if row is None:
            raise AssertionError(f"federation-listing mapping row missing: {mapping_id}")
        if len(row.get("lab_requirement_ids", [])) != expected["requirements"]:
            raise AssertionError(f"federation-listing requirement count drifted: {mapping_id}")
        if set(row.get("standard_sections", [])) != expected["sections"]:
            raise AssertionError(f"federation-listing section keys drifted: {mapping_id}")
    federation_listing_lane = query_rti_work.focused_lane_result(
        index, tests, "federation-listing", limit=1
    )
    if federation_listing_lane.get("assertion_count") != 41:
        raise AssertionError("federation-listing lane assertion total drifted")
    federation_listing_handles = federation_listing_lane.get("lane_handles")
    if not isinstance(federation_listing_handles, dict):
        raise AssertionError("federation-listing lane lost its evidence handles")
    if federation_listing_handles.get("catch2_target") != "umbra_federation_listing_catch2":
        raise AssertionError("federation-listing Catch2 target handle drifted")
    if federation_listing_handles.get("ctest_label") != "federation-listing":
        raise AssertionError("federation-listing CTest label handle drifted")

    # Unmapped work must be discoverable without another repository-wide Lab
    # scan.  Keep one positive candidate lookup and one negative lookup
    # explicit: the first has an exact local contract reference that can be
    # reviewed before assigning a plan mapping, while the second must remain a
    # deliberate no-candidate gap instead of being guessed from API names.
    explicit_mim = next(
        test
        for test in tests
        if test.get("test_case")
        == "Embedded Create Federation Execution accepts a validated explicit MIM path"
    )
    explicit_mim_candidates = query_rti_work.contract_candidates_for_test(
        explicit_mim, contract_test_candidates
    )
    if len(explicit_mim_candidates) != 2:
        raise AssertionError(
            "explicit-MIM candidate discovery lost one of its exact contract references"
        )
    if {
        candidate.get("lab_requirement_id")
        for candidate in explicit_mim_candidates
        if isinstance(candidate, dict)
    } != {
        "requirement-candidate-content-clauses-04-federation-management-page-053-l43-7"
    }:
        raise AssertionError("explicit-MIM candidate Lab id drifted")
    if {
        candidate.get("clause_id")
        for candidate in explicit_mim_candidates
        if isinstance(candidate, dict)
    } != {"clause-4.5.5"}:
        raise AssertionError("explicit-MIM candidate clause drifted")
    object_lookup = next(
        test
        for test in tests
        if test.get("test_case")
        == "Embedded object-class lookup services use stable handles from the joined federation FOM"
    )
    if query_rti_work.contract_candidates_for_test(
        object_lookup, contract_test_candidates
    ):
        raise AssertionError(
            "object-class lookup unexpectedly acquired a guessed contract candidate"
        )
    next_message = next(
        test
        for test in tests
        if test.get("test_case")
        == "Embedded Next Message Request grants at the next queued TSO timestamp"
    )
    next_message_candidates = query_rti_work.contract_candidates_for_test(
        next_message, contract_test_candidates
    )
    if len(next_message_candidates) != 2 or not all(
        candidate.get("source_path_match") is True
        for candidate in next_message_candidates
        if isinstance(candidate, dict)
    ):
        raise AssertionError(
            "relocated Next Message Request contract candidates are not exact source matches"
        )
    next_message_candidate_summary = query_rti_work.text_test_summary(
        {**next_message, "contract_candidates": next_message_candidates}
    )
    if (
        "contract candidates:" not in next_message_candidate_summary
        or "[source-mismatch]" in next_message_candidate_summary
    ):
        raise AssertionError(
            "bounded unmapped summary did not retain exact relocated-contract source matches"
        )

    queue = query_rti_work.indexed_work_queue(index, tests, limit=0)
    queue_by_id = {
        row.get("id"): row
        for row in queue.get("items", [])
        if isinstance(row, dict) and row.get("id")
    }
    callback_queue = queue_by_id.get("multi-federate-callback-ordering")
    if not isinstance(callback_queue, dict):
        raise AssertionError("queue lost the multi-federate callback family")
    if callback_queue.get("action_state") != "evidence-complete":
        raise AssertionError(
            "completed callback family was exposed as a runnable queue action"
        )
    if callback_queue.get("actionable_source_drift_count") != 0:
        raise AssertionError(
            "callback family diagnostic source drift was marked actionable"
        )
    review_queue = queue_by_id.get("disconnect-protected-review")
    if not isinstance(review_queue, dict) or review_queue.get("action_state") != "external-review":
        raise AssertionError(
            "protected-review family did not retain its explicit external action"
        )
    action_counts = queue.get("action_counts")
    if not isinstance(action_counts, dict) or action_counts.get("evidence-complete") != 6:
        raise AssertionError("queue action-state aggregate drifted")
    if queue.get("evidence_complete_family_count") != 6 or queue.get(
        "queued_action_family_count"
    ) != 2:
        raise AssertionError("queue action-state family totals drifted")
    process_rows = [
        row
        for row in queue.get("items", [])
        if isinstance(row, dict) and row.get("next_lane") == "process-boundary"
    ]
    if len(process_rows) != 1:
        raise AssertionError(
            "bounded queue did not expose exactly one process-boundary owner"
        )
    process_lane = query_rti_work.focused_lane_result(
        index, tests, "process-boundary", limit=1
    )
    if process_rows[0].get("assertion_count") != process_lane.get("assertion_count"):
        raise AssertionError(
            "queue assertion total diverges from the focused lane result"
        )
    if process_lane.get("mapped_test_count") != 109 or process_lane.get("assertion_count") != 5239:
        raise AssertionError("process-boundary lane baseline counts drifted")
    process_lane_inventory = query_rti_work.lane_inventory(
        index,
        tests,
        family="transport-and-conformance",
        limit=0,
    )
    process_inventory_row = next(
        (
            row
            for row in process_lane_inventory.get("lanes", [])
            if isinstance(row, dict) and row.get("tag") == "process-boundary"
        ),
        None,
    )
    if not isinstance(process_inventory_row, dict):
        raise AssertionError("process-boundary lane inventory row is absent")
    if (
        process_inventory_row.get("assertion_count") != 5239
        or process_inventory_row.get("recorded_assertion_count") != 4603
        or process_inventory_row.get("assertion_count_source") != "indexed-lane-total"
    ):
        raise AssertionError(
            "process-boundary lane inventory does not use the verified aggregate total"
        )
    process_query_time_bounds_immediate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-query-time-bounds-immediate-integration"
        ),
        None,
    )
    if not isinstance(process_query_time_bounds_immediate, dict):
        raise AssertionError("focused process immediate GALT/LITS row is absent")
    if query_rti_work.source_location_text(process_query_time_bounds_immediate) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:19564"
    ):
        raise AssertionError("focused process immediate GALT/LITS source pointer drifted")
    if process_query_time_bounds_immediate.get("assertions") != 14:
        raise AssertionError("focused process immediate GALT/LITS assertion count drifted")
    if process_query_time_bounds_immediate.get("callback_models") != ["HLA_IMMEDIATE"]:
        raise AssertionError("focused process immediate GALT/LITS callback-model mapping drifted")
    if process_query_time_bounds_immediate.get("primary_lane") != (
        "process-query-time-bounds-immediate"
    ):
        raise AssertionError("focused process immediate GALT/LITS primary lane drifted")
    if process_query_time_bounds_immediate.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process immediate GALT/LITS row is not mapped")
    process_query_time_bounds_immediate_focus = query_rti_work.focused_lane_result(
        index, tests, "process-query-time-bounds-immediate", limit=0
    )
    if (
        process_query_time_bounds_immediate_focus.get("lane_state") != "complete"
        or process_query_time_bounds_immediate_focus.get("mapped_test_count") != 1
        or process_query_time_bounds_immediate_focus.get("assertion_count") != 14
        or process_query_time_bounds_immediate_focus.get("requirement_count") != 5
        or process_query_time_bounds_immediate_focus.get("standard_section_count") != 4
    ):
        raise AssertionError("focused process immediate GALT/LITS lane totals drifted")
    process_query_time_bounds_immediate_handles = (
        process_query_time_bounds_immediate_focus.get("lane_handles")
    )
    if not isinstance(process_query_time_bounds_immediate_handles, dict) or process_query_time_bounds_immediate_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused process immediate GALT/LITS target handle drifted")
    if process_query_time_bounds_immediate_handles.get("ctest_label") != "process-boundary":
        raise AssertionError("focused process immediate GALT/LITS CTest label drifted")
    process_query_time_bounds_multi_federate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-query-time-bounds-multi-federate-integration"
        ),
        None,
    )
    if not isinstance(process_query_time_bounds_multi_federate, dict):
        raise AssertionError("focused process multi-federate GALT/LITS row is absent")
    if query_rti_work.source_location_text(process_query_time_bounds_multi_federate) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:19674"
    ):
        raise AssertionError("focused process multi-federate GALT/LITS source pointer drifted")
    if process_query_time_bounds_multi_federate.get("assertions") != 24:
        raise AssertionError("focused process multi-federate GALT/LITS assertion count drifted")
    if process_query_time_bounds_multi_federate.get("callback_models") != ["HLA_IMMEDIATE"]:
        raise AssertionError(
            "focused process multi-federate GALT/LITS callback-model mapping drifted"
        )
    if process_query_time_bounds_multi_federate.get("primary_lane") != (
        "process-query-time-bounds-multi-federate"
    ):
        raise AssertionError("focused process multi-federate GALT/LITS primary lane drifted")
    if process_query_time_bounds_multi_federate.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process multi-federate GALT/LITS row is not mapped")
    process_query_time_bounds_multi_federate_focus = query_rti_work.focused_lane_result(
        index, tests, "process-query-time-bounds-multi-federate", limit=0
    )
    if (
        process_query_time_bounds_multi_federate_focus.get("lane_state") != "complete"
        or process_query_time_bounds_multi_federate_focus.get("mapped_test_count") != 1
        or process_query_time_bounds_multi_federate_focus.get("assertion_count") != 24
        or process_query_time_bounds_multi_federate_focus.get("requirement_count") != 5
        or process_query_time_bounds_multi_federate_focus.get("standard_section_count") != 4
    ):
        raise AssertionError("focused process multi-federate GALT/LITS lane totals drifted")
    process_query_time_bounds_multi_federate_handles = (
        process_query_time_bounds_multi_federate_focus.get("lane_handles")
    )
    if not isinstance(process_query_time_bounds_multi_federate_handles, dict) or process_query_time_bounds_multi_federate_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused process multi-federate GALT/LITS target handle drifted")
    if process_query_time_bounds_multi_federate_handles.get("ctest_label") != "process-boundary":
        raise AssertionError("focused process multi-federate GALT/LITS CTest label drifted")
    process_query_time_bounds_queued_tso = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-query-time-bounds-queued-tso-integration"
        ),
        None,
    )
    if not isinstance(process_query_time_bounds_queued_tso, dict):
        raise AssertionError("focused process queued-TSO GALT/LITS row is absent")
    if query_rti_work.source_location_text(process_query_time_bounds_queued_tso) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:19881"
    ):
        raise AssertionError("focused process queued-TSO GALT/LITS source pointer drifted")
    if process_query_time_bounds_queued_tso.get("assertions") != 31:
        raise AssertionError("focused process queued-TSO GALT/LITS assertion count drifted")
    if process_query_time_bounds_queued_tso.get("callback_models") != ["HLA_IMMEDIATE"]:
        raise AssertionError(
            "focused process queued-TSO GALT/LITS callback-model mapping drifted"
        )
    if process_query_time_bounds_queued_tso.get("primary_lane") != (
        "process-query-time-bounds-queued-tso"
    ):
        raise AssertionError("focused process queued-TSO GALT/LITS primary lane drifted")
    if process_query_time_bounds_queued_tso.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process queued-TSO GALT/LITS row is not mapped")
    process_query_time_bounds_queued_tso_focus = query_rti_work.focused_lane_result(
        index, tests, "process-query-time-bounds-queued-tso", limit=0
    )
    if (
        process_query_time_bounds_queued_tso_focus.get("lane_state") != "complete"
        or process_query_time_bounds_queued_tso_focus.get("mapped_test_count") != 1
        or process_query_time_bounds_queued_tso_focus.get("assertion_count") != 31
        or process_query_time_bounds_queued_tso_focus.get("requirement_count") != 5
        or process_query_time_bounds_queued_tso_focus.get("standard_section_count") != 4
    ):
        raise AssertionError("focused process queued-TSO GALT/LITS lane totals drifted")
    process_query_time_bounds_queued_tso_handles = (
        process_query_time_bounds_queued_tso_focus.get("lane_handles")
    )
    if not isinstance(process_query_time_bounds_queued_tso_handles, dict) or process_query_time_bounds_queued_tso_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused process queued-TSO GALT/LITS target handle drifted")
    if process_query_time_bounds_queued_tso_handles.get("ctest_label") != "process-boundary":
        raise AssertionError("focused process queued-TSO GALT/LITS CTest label drifted")
    process_query_time_bounds_zero_lookahead = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-query-time-bounds-zero-lookahead-integration"
        ),
        None,
    )
    if not isinstance(process_query_time_bounds_zero_lookahead, dict):
        raise AssertionError("focused process zero-lookahead GALT/LITS row is absent")
    if query_rti_work.source_location_text(process_query_time_bounds_zero_lookahead) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:20173"
    ):
        raise AssertionError("focused process zero-lookahead GALT/LITS source pointer drifted")
    if process_query_time_bounds_zero_lookahead.get("assertions") != 28:
        raise AssertionError("focused process zero-lookahead GALT/LITS assertion count drifted")
    if process_query_time_bounds_zero_lookahead.get("callback_models") != ["HLA_EVOKED"]:
        raise AssertionError(
            "focused process zero-lookahead GALT/LITS callback-model mapping drifted"
        )
    if process_query_time_bounds_zero_lookahead.get("primary_lane") != (
        "process-query-time-bounds-zero-lookahead"
    ):
        raise AssertionError("focused process zero-lookahead GALT/LITS primary lane drifted")
    if process_query_time_bounds_zero_lookahead.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process zero-lookahead GALT/LITS row is not mapped")
    process_query_time_bounds_zero_lookahead_focus = query_rti_work.focused_lane_result(
        index, tests, "process-query-time-bounds-zero-lookahead", limit=0
    )
    if (
        process_query_time_bounds_zero_lookahead_focus.get("lane_state") != "complete"
        or process_query_time_bounds_zero_lookahead_focus.get("mapped_test_count") != 1
        or process_query_time_bounds_zero_lookahead_focus.get("assertion_count") != 28
        or process_query_time_bounds_zero_lookahead_focus.get("requirement_count") != 6
        or process_query_time_bounds_zero_lookahead_focus.get("standard_section_count") != 4
    ):
        raise AssertionError("focused process zero-lookahead GALT/LITS lane totals drifted")
    process_query_time_bounds_zero_lookahead_handles = (
        process_query_time_bounds_zero_lookahead_focus.get("lane_handles")
    )
    if not isinstance(process_query_time_bounds_zero_lookahead_handles, dict) or process_query_time_bounds_zero_lookahead_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused process zero-lookahead GALT/LITS target handle drifted")
    if process_query_time_bounds_zero_lookahead_handles.get("ctest_label") != "process-boundary":
        raise AssertionError("focused process zero-lookahead GALT/LITS CTest label drifted")
    process_fqr_galt_focus = query_rti_work.focused_lane_result(
        index, tests, "process-flush-queue-galt-frontier", limit=0
    )
    if (
        process_fqr_galt_focus.get("lane_state") != "complete"
        or process_fqr_galt_focus.get("mapped_test_count") != 1
        or process_fqr_galt_focus.get("assertion_count") != 57
        or process_fqr_galt_focus.get("requirement_count") != 19
        or process_fqr_galt_focus.get("standard_section_count") != 11
    ):
        raise AssertionError("process FQR/GALT frontier focus totals drifted")
    process_fqr_galt_handles = process_fqr_galt_focus.get("lane_handles")
    if not isinstance(process_fqr_galt_handles, dict) or process_fqr_galt_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("process FQR/GALT frontier lost its C++ target handle")
    if process_fqr_galt_handles.get("ctest_label") != "process-boundary":
        raise AssertionError("process FQR/GALT frontier CTest label drifted")
    process_fqr_multiple_focus = query_rti_work.focused_lane_result(
        index, tests, "process-flush-queue-multiple-records", limit=0
    )
    if (
        process_fqr_multiple_focus.get("lane_state") != "complete"
        or process_fqr_multiple_focus.get("mapped_test_count") != 1
        or process_fqr_multiple_focus.get("assertion_count") != 110
        or process_fqr_multiple_focus.get("requirement_count") != 22
        or process_fqr_multiple_focus.get("standard_section_count") != 12
    ):
        raise AssertionError("process multiple-record FQR focus totals drifted")
    process_fqr_multiple_handles = process_fqr_multiple_focus.get("lane_handles")
    if not isinstance(process_fqr_multiple_handles, dict) or process_fqr_multiple_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("process multiple-record FQR lost its C++ target handle")
    if process_fqr_multiple_handles.get("ctest_label") != "process-boundary":
        raise AssertionError("process multiple-record FQR CTest label drifted")
    process_fqr_retraction_focus = query_rti_work.focused_lane_result(
        index, tests, "process-flush-queue-retraction", limit=0
    )
    if (
        process_fqr_retraction_focus.get("lane_state") != "complete"
        or process_fqr_retraction_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or process_fqr_retraction_focus.get("mapped_test_count") != 1
        or process_fqr_retraction_focus.get("assertion_count") != 110
        or process_fqr_retraction_focus.get("requirement_count") != 22
        or process_fqr_retraction_focus.get("standard_section_count") != 12
    ):
        raise AssertionError("process FQR retraction focus totals drifted")
    process_fqr_retraction_handles = process_fqr_retraction_focus.get("lane_handles")
    if not isinstance(process_fqr_retraction_handles, dict) or process_fqr_retraction_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("process FQR retraction lost its C++ target handle")
    if process_fqr_retraction_handles.get("ctest_label") != "process-boundary":
        raise AssertionError("process FQR retraction CTest label drifted")
    process_item = next(
        (
            item
            for item in index.get("items", [])
            if isinstance(item, dict) and item.get("id") == "transport-and-conformance"
        ),
        None,
    )
    if not isinstance(process_item, dict) or not {
        "process-flush-queue",
        "process-flush-queue-galt-frontier",
        "process-flush-queue-multiple-records",
        "process-flush-queue-retraction",
    }.issubset(set(process_item.get("focused_lane_tags", []))):
        raise AssertionError("process Flush Queue lanes are not roadmap-indexed")
    process_handles = process_lane.get("lane_handles")
    if not isinstance(process_handles, dict):
        raise AssertionError("process-boundary lane lost its evidence handles")
    if process_handles.get("ctest_execution_count") != 108:
        raise AssertionError("process-boundary CTest execution baseline drifted")
    if process_handles.get("junit_testcase_count") != 4132:
        raise AssertionError("process-boundary JUnit testcase baseline drifted")
    if process_handles.get("junit_failures") != 0:
        raise AssertionError("process-boundary JUnit report is not green")
    if process_handles.get("junit_input_targets") != [
        "umbra_process_boundary_private_catch2",
        "umbra_ieee1516_2025_connection_catch2",
    ]:
        raise AssertionError("process-boundary JUnit inputs are not independently buildable")
    if process_handles.get("junit_merge_script") != "tools/merge_junit_reports.py":
        raise AssertionError("process-boundary JUnit merge script handle drifted")
    object_queue_row = queue_by_id.get("object-ddm-ownership")
    if not isinstance(object_queue_row, dict):
        raise AssertionError("queue lost the object/ownership handoff")
    if (
        object_queue_row.get("state") != "complete-pointer"
        or object_queue_row.get("action_state") != "evidence-complete"
        or object_queue_row.get("planned_count") != 0
        or object_queue_row.get("next_test_query")
        != "RTIambassadors continue Confirm Divestiture with a pushed assumption candidate"
        or object_queue_row.get("next_test_state") != "complete"
    ):
        raise AssertionError("object/ownership queue handoff drifted")
    push_confirm = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-confirm-divestiture-process-push-integration"
        ),
        None,
    )
    if not isinstance(push_confirm, dict):
        raise AssertionError("push Confirm Divestiture row is absent")
    if (
        push_confirm.get("status") != "implemented-public-foundation-not-conformance-evidence"
        or query_rti_work.source_location_text(push_confirm)
        != "cpp/tests/attribute_ownership_acquisition_catch2.cpp:2118"
        or push_confirm.get("assertions") != 41
        or len(push_confirm.get("lab_requirement_ids", [])) != 4
        or len(push_confirm.get("standard_sections", [])) != 4
        or len(push_confirm.get("selected_cpp_api_surface_ids", [])) != 15
    ):
        raise AssertionError("push Confirm Divestiture mapping drifted")
    push_confirm_focus = query_rti_work.focused_lane_result(
        index, tests, "process-confirm-divestiture-push", limit=0
    )
    if (
        push_confirm_focus.get("lane_state") != "complete"
        or push_confirm_focus.get("action_state") != "evidence-complete"
        or push_confirm_focus.get("planned_count") != 0
        or push_confirm_focus.get("mapped_test_count") != 1
        or push_confirm_focus.get("requirement_count") != 4
        or push_confirm_focus.get("standard_section_count") != 4
        or push_confirm_focus.get("requirement_section_pair_count") != 4
        or push_confirm_focus.get("assertion_count") != 41
    ):
        raise AssertionError("push Confirm Divestiture focus card drifted")
    assumption_case = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-confirm-divestiture-process-assumption-integration"
        ),
        None,
    )
    if not isinstance(assumption_case, dict):
        raise AssertionError("Confirm Divestiture assumption row is absent")
    if (
        assumption_case.get("status")
        != "implemented-public-foundation-not-conformance-evidence"
        or query_rti_work.source_location_text(assumption_case)
        != "cpp/tests/attribute_ownership_acquisition_catch2.cpp:2480"
        or assumption_case.get("assertions") != 60
        or len(assumption_case.get("lab_requirement_ids", [])) != 1
        or len(assumption_case.get("standard_sections", [])) != 1
        or len(assumption_case.get("selected_cpp_api_surface_ids", [])) != 16
    ):
        raise AssertionError("Confirm Divestiture assumption mapping drifted")
    assumption_focus = query_rti_work.focused_lane_result(
        index, tests, "process-confirm-divestiture-assumption", limit=0
    )
    if (
        assumption_focus.get("lane_state") != "complete"
        or assumption_focus.get("action_state") != "evidence-complete"
        or assumption_focus.get("planned_count") != 0
        or assumption_focus.get("mapped_test_count") != 1
        or assumption_focus.get("requirement_count") != 1
        or assumption_focus.get("standard_section_count") != 1
        or assumption_focus.get("requirement_section_pair_count") != 1
        or assumption_focus.get("assertion_count") != 60
    ):
        raise AssertionError("Confirm Divestiture assumption focus card drifted")
    ownership_lane = query_rti_work.focused_lane_result(
        index, tests, "process-ownership-check", limit=1
    )
    if (
        ownership_lane.get("mapped_test_count") != 1
        or ownership_lane.get("assertion_count") != 18
        or ownership_lane.get("requirement_section_pair_count") != 3
        or ownership_lane.get("roadmap_owner") != "object-ddm-ownership"
    ):
        raise AssertionError("process-ownership-check lane baseline drifted")
    ownership_test = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-check-process-integration"
        ),
        None,
    )
    if not isinstance(ownership_test, dict):
        raise AssertionError("process ownership-check plan row is absent")
    if ownership_test.get("primary_lane") != "process-ownership-check":
        raise AssertionError("process ownership-check primary lane drifted")
    ownership_links = query_rti_work.roadmap_links_for_test(index, ownership_test)
    ownership_owner = query_rti_work.primary_roadmap_link(ownership_links)
    if not isinstance(ownership_owner, dict) or ownership_owner.get("id") != "object-ddm-ownership":
        raise AssertionError("process ownership-check roadmap owner drifted")
    ownership_query_lane = query_rti_work.focused_lane_result(
        index, tests, "process-ownership-query", limit=1
    )
    if (
        ownership_query_lane.get("mapped_test_count") != 1
        or ownership_query_lane.get("assertion_count") != 26
        or ownership_query_lane.get("requirement_section_pair_count") != 6
        or ownership_query_lane.get("roadmap_owner") != "object-ddm-ownership"
    ):
        raise AssertionError("process-ownership-query lane baseline drifted")
    ownership_query_test = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-query-process-integration"
        ),
        None,
    )
    if not isinstance(ownership_query_test, dict):
        raise AssertionError("process ownership-query plan row is absent")
    if ownership_query_test.get("primary_lane") != "process-ownership-query":
        raise AssertionError("process ownership-query primary lane drifted")
    ownership_query_links = query_rti_work.roadmap_links_for_test(
        index, ownership_query_test
    )
    ownership_query_owner = query_rti_work.primary_roadmap_link(ownership_query_links)
    if (
        not isinstance(ownership_query_owner, dict)
        or ownership_query_owner.get("id") != "object-ddm-ownership"
    ):
        raise AssertionError("process ownership-query roadmap owner drifted")
    process_family_item, process_family_tests = query_rti_work.indexed_item(
        index, tests, "time-save-restore"
    )
    if process_family_item is None or not process_family_tests:
        raise AssertionError("time-save-restore family did not resolve for queue checks")
    process_family_counts = query_rti_work.live_item_mapping_counts(
        process_family_item, tests
    )
    if process_rows[0].get("requirement_section_pair_count") != process_family_counts.get(
        "requirement_section_pair_count"
    ):
        raise AssertionError("queue direct pair count diverges from family mapping")
    scoped_process_check_errors = query_rti_work.validate_index(
        index,
        query_rti_work.DEFAULT_ROADMAP,
        plan,
        standard_requirements,
        tests,
        source_locations=source_locations,
        focus_lane="process-boundary",
    )
    if scoped_process_check_errors:
        raise AssertionError(
            "process-boundary scoped roadmap check is not clean: "
            + "; ".join(scoped_process_check_errors[:3])
        )

    # The normal integrity gate is live-plan only.  Keep the append-only
    # completion ledger behind an explicit strict-audit switch so replaced
    # historical plan/source handles cannot consume the implementation turn.
    live_check_errors = query_rti_work.validate_index(
        index,
        query_rti_work.DEFAULT_ROADMAP,
        plan,
        standard_requirements,
        tests,
        source_locations=source_locations,
    )
    if live_check_errors:
        raise AssertionError(
            "live roadmap check is not clean: "
            + "; ".join(live_check_errors[:3])
        )
    historical_check_errors = query_rti_work.validate_index(
        index,
        query_rti_work.DEFAULT_ROADMAP,
        plan,
        standard_requirements,
        tests,
        source_locations=source_locations,
        include_historical=True,
    )
    if historical_check_errors:
        raise AssertionError(
            "historical roadmap completion ledger is not clean: "
            + "; ".join(historical_check_errors[:3])
        )
    historical_probe = dict(index)
    historical_probe["recent_completed_slices"] = [
        {"plan_id": "query-rti-work-regression-history-probe"}
    ]
    live_probe_errors = query_rti_work.validate_index(
        historical_probe,
        query_rti_work.DEFAULT_ROADMAP,
        plan,
        standard_requirements,
        tests,
        source_locations=source_locations,
    )
    if any("recent_completed_slices" in error for error in live_probe_errors):
        raise AssertionError(
            "live roadmap check unexpectedly validated historical completion rows"
        )
    historical_probe_errors = query_rti_work.validate_index(
        historical_probe,
        query_rti_work.DEFAULT_ROADMAP,
        plan,
        standard_requirements,
        tests,
        source_locations=source_locations,
        include_historical=True,
    )
    if not any(
        "recent_completed_slices[0]" in error
        for error in historical_probe_errors
    ):
        raise AssertionError(
            "strict historical check did not validate the completion ledger"
        )
    fom_check_errors = query_rti_work.validate_index(
        index,
        query_rti_work.DEFAULT_ROADMAP,
        plan,
        standard_requirements,
        tests,
        source_locations=source_locations,
        focus_family="fom-module-declaration-management",
    )
    if fom_check_errors:
        raise AssertionError(
            "FOM module-management family check is not clean: "
            + "; ".join(fom_check_errors[:3])
        )

    # Baseline pointers are part of the one-screen work handoff.  Keep their
    # mapping counts synchronized with the exact Catch2 row so status/queue
    # can answer the common "what does this test cover?" question without a
    # second broad lookup.
    status = query_rti_work.index_status(
        index,
        tests,
        query_rti_work.roadmap_checklist(query_rti_work.DEFAULT_ROADMAP),
        source_locations,
    )
    if not isinstance(source_health, dict) or source_health.get("files_scanned", 0) <= 0:
        raise AssertionError("source-index health did not report scanned C++ files")
    if source_health.get("status") not in {"ok", "attention"}:
        raise AssertionError("source-index health returned an unknown status")
    latest = status.get("latest_completed_slice")
    if not isinstance(latest, dict):
        raise AssertionError("status omitted the latest completed slice handoff")
    if latest.get("lane") != "process-boundary":
        raise AssertionError("latest completed slice lane drifted")
    if latest.get("plan_id") != "umbra-cpp-process-tso-directed-interaction-callback-gating":
        raise AssertionError("latest completed slice plan id drifted")
    if latest.get("test_case") != "RTIambassadors retain a timestamped directed interaction while callbacks are disabled":
        raise AssertionError("latest completed slice test title drifted")
    if latest.get("focus_lane") != "process-tso-directed-interaction-callback-gating":
        raise AssertionError("latest completed slice focus lane drifted")
    if latest.get("primary_lane") != "process-tso-directed-interaction-callback-gating":
        raise AssertionError("latest completed slice primary lane drifted")
    if latest.get("source_location") != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:15757"
    ):
        raise AssertionError("latest completed slice source pointer drifted")
    if latest.get("assertions") != 62 or latest.get("requirements") != 21:
        raise AssertionError("latest completed slice evidence counts drifted")
    if latest.get("standard_sections") != 16 or latest.get("api_surfaces") != 18:
        raise AssertionError("latest completed slice section/API counts drifted")
    regional_callback_gating = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-regional-interaction-callback-gating"
        ),
        None,
    )
    if not isinstance(regional_callback_gating, dict):
        raise AssertionError("regional callback-gating plan row is absent")
    if (
        query_rti_work.source_location_text(regional_callback_gating)
        != "cpp/tests/ieee1516_2025_connection_catch2.cpp:25321"
        or regional_callback_gating.get("assertions") != 69
        or regional_callback_gating.get("status")
        != "implemented-private-foundation-not-conformance-evidence"
        or len(regional_callback_gating.get("lab_requirement_ids", [])) != 28
        or len(regional_callback_gating.get("standard_sections", [])) != 16
        or len(regional_callback_gating.get("selected_cpp_api_surface_ids", [])) != 20
    ):
        raise AssertionError("regional callback-gating source/evidence mapping drifted")
    regional_callback_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-regional-interaction-callback-gating", limit=0
    )
    if (
        regional_callback_focus.get("lane_state") != "complete"
        or regional_callback_focus.get("mapped_test_count") != 1
        or regional_callback_focus.get("assertion_count") != 69
        or regional_callback_focus.get("recorded_assertion_count") != 69
        or regional_callback_focus.get("requirement_count") != 28
        or regional_callback_focus.get("standard_section_count") != 16
        or regional_callback_focus.get("requirement_section_pair_count") != 28
    ):
        raise AssertionError("regional callback-gating lane card drifted")
    regional_callback_handles = regional_callback_focus.get("lane_handles")
    if not isinstance(regional_callback_handles, dict) or regional_callback_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2" or "callbacks are disabled" not in str(
        regional_callback_handles.get("ctest_filter") or ""
    ):
        raise AssertionError("regional callback-gating CTest handle drifted")
    recent_views = query_rti_work.recent_completed_views(index, tests)
    if not recent_views or recent_views[0].get("plan_id") != "umbra-cpp-process-tso-directed-interaction-callback-gating":
        raise AssertionError("recent completed-slice ordering drifted")
    mixed_region = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-mixed-region-interaction-validation-integration"
        ),
        None,
    )
    if not isinstance(mixed_region, dict):
        raise AssertionError("mixed-dimensional regional interaction plan row is absent")
    if query_rti_work.source_location_text(mixed_region) != (
        "cpp/tests/mixed_region_interaction_validation_catch2.cpp:119"
    ) or mixed_region.get("assertions") != 37:
        raise AssertionError("mixed-dimensional regional interaction source/evidence drifted")
    if mixed_region.get("traceability_state") != "requirements-mapped":
        raise AssertionError("mixed-dimensional regional interaction row is not mapped")
    mixed_region_focus = query_rti_work.focused_lane_result(
        index, tests, "mixed-region-interaction-validation", limit=0
    )
    if (
        mixed_region_focus.get("lane_state") != "complete"
        or mixed_region_focus.get("mapped_test_count") != 1
        or mixed_region_focus.get("assertion_count") != 37
        or mixed_region_focus.get("requirement_count") != 4
        or mixed_region_focus.get("standard_section_count") != 1
        or mixed_region_focus.get("requirement_section_pair_count") != 4
    ):
        raise AssertionError("mixed-dimensional regional interaction lane card drifted")
    mixed_region_handles = mixed_region_focus.get("lane_handles")
    if not isinstance(mixed_region_handles, dict) or mixed_region_handles.get(
        "catch2_target"
    ) != "umbra_mixed_region_interaction_validation_catch2":
        raise AssertionError("mixed-dimensional regional interaction target handle drifted")
    mixed_region_case = query_rti_work.case_card_record(
        mixed_region, index, "umbra-cpp-mixed-region-interaction-validation-integration"
    )
    if mixed_region_case.get("lane") != "mixed-region-interaction-validation":
        raise AssertionError("exact case card did not resolve the mixed-region lane")
    if mixed_region_case.get("commands", {}).get("ctest") is None:
        raise AssertionError("exact case card omitted the mixed-region CTest handle")

    whole_class_unsubscribe = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-whole-class-interaction-unsubscribe-default-region-integration"
        ),
        None,
    )
    if not isinstance(whole_class_unsubscribe, dict):
        raise AssertionError("whole-class default-region unsubscription plan row is absent")
    if query_rti_work.source_location_text(whole_class_unsubscribe) != (
        "cpp/tests/default_region_interaction_routing_catch2.cpp:241"
    ) or whole_class_unsubscribe.get("assertions") != 20:
        raise AssertionError("whole-class default-region source/evidence drifted")
    if whole_class_unsubscribe.get("traceability_state") != "requirements-mapped":
        raise AssertionError("whole-class default-region row is not mapped")
    if whole_class_unsubscribe.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.4",
    ]:
        raise AssertionError("whole-class default-region section mapping drifted")
    if whole_class_unsubscribe.get("lab_requirement_ids") != [
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l35-11",
    ]:
        raise AssertionError("whole-class default-region requirement mapping drifted")
    whole_class_unsubscribe_focus = query_rti_work.focused_lane_result(
        index, tests, "whole-class-unsubscribe", limit=0
    )
    if (
        whole_class_unsubscribe_focus.get("lane_state") != "complete"
        or whole_class_unsubscribe_focus.get("mapped_test_count") != 1
        or whole_class_unsubscribe_focus.get("assertion_count") != 20
        or whole_class_unsubscribe_focus.get("requirement_count") != 1
        or whole_class_unsubscribe_focus.get("standard_section_count") != 1
        or whole_class_unsubscribe_focus.get("requirement_section_pair_count") != 1
    ):
        raise AssertionError("whole-class default-region lane card drifted")
    whole_class_unsubscribe_handles = whole_class_unsubscribe_focus.get("lane_handles")
    if (
        not isinstance(whole_class_unsubscribe_handles, dict)
        or whole_class_unsubscribe_handles.get("catch2_target")
        != "umbra_default_region_interaction_routing_catch2"
        or whole_class_unsubscribe_handles.get("ctest_label")
        != "whole-class-unsubscribe"
    ):
        raise AssertionError("whole-class default-region lane handles drifted")
    whole_class_unsubscribe_case = query_rti_work.case_card_record(
        whole_class_unsubscribe,
        index,
        "umbra-cpp-whole-class-interaction-unsubscribe-default-region-integration",
    )
    if (
        whole_class_unsubscribe_case.get("traceability_state")
        != "requirements-mapped"
        or whole_class_unsubscribe_case.get("lane")
        != "whole-class-unsubscribe"
        or "ctest" not in whole_class_unsubscribe_case.get("commands", {})
    ):
        raise AssertionError("whole-class default-region card lost its handles")

    no_common_dimension = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-regional-object-attribute-no-common-dimension-integration"
        ),
        None,
    )
    if not isinstance(no_common_dimension, dict):
        raise AssertionError("no-common-dimension regional object plan row is absent")
    if query_rti_work.source_location_text(no_common_dimension) != (
        "cpp/tests/regional_object_attribute_routing_catch2.cpp:343"
    ) or no_common_dimension.get("assertions") != 45:
        raise AssertionError("no-common-dimension source/evidence drifted")
    if no_common_dimension.get("traceability_state") != "requirements-mapped":
        raise AssertionError("no-common-dimension row is not mapped")
    if no_common_dimension.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.4",
    ]:
        raise AssertionError("no-common-dimension section mapping drifted")
    if no_common_dimension.get("lab_requirement_ids") != [
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l68-21",
    ]:
        raise AssertionError("no-common-dimension requirement mapping drifted")
    no_common_dimension_focus = query_rti_work.focused_lane_result(
        index, tests, "no-common-dimension", limit=0
    )
    if (
        no_common_dimension_focus.get("lane_state") != "complete"
        or no_common_dimension_focus.get("mapped_test_count") != 1
        or no_common_dimension_focus.get("assertion_count") != 45
        or no_common_dimension_focus.get("requirement_count") != 1
        or no_common_dimension_focus.get("standard_section_count") != 1
        or no_common_dimension_focus.get("requirement_section_pair_count") != 1
    ):
        raise AssertionError("no-common-dimension lane card drifted")
    no_common_dimension_handles = no_common_dimension_focus.get("lane_handles")
    if (
        not isinstance(no_common_dimension_handles, dict)
        or no_common_dimension_handles.get("catch2_target")
        != "umbra_regional_object_attribute_routing_catch2"
        or no_common_dimension_handles.get("ctest_label")
        != "no-common-dimension"
    ):
        raise AssertionError("no-common-dimension lane handles drifted")
    no_common_dimension_case = query_rti_work.case_card_record(
        no_common_dimension,
        index,
        "umbra-cpp-regional-object-attribute-no-common-dimension-integration",
    )
    if (
        no_common_dimension_case.get("traceability_state") != "requirements-mapped"
        or no_common_dimension_case.get("lane") != "no-common-dimension"
        or "ctest" not in no_common_dimension_case.get("commands", {})
    ):
        raise AssertionError("no-common-dimension card lost its handles")

    time_axis_independence = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-timestamped-regional-attribute-update-tar-nmr-integration"
        ),
        None,
    )
    if not isinstance(time_axis_independence, dict):
        raise AssertionError("time-axis-independence plan row is absent")
    if query_rti_work.source_location_text(time_axis_independence) != (
        "cpp/tests/timestamped_regional_attribute_update_tar_nmr_catch2.cpp:151"
    ) or time_axis_independence.get("assertions") != 94:
        raise AssertionError("time-axis-independence source/evidence drifted")
    if time_axis_independence.get("traceability_state") != "requirements-mapped":
        raise AssertionError("time-axis-independence row is not mapped")
    if set(time_axis_independence.get("lab_requirement_ids", [])) != {
        "requirement-candidate-content-clauses-06-object-management-page-120-l83-22",
        "requirement-candidate-content-clauses-06-object-management-page-121-l36-10",
        "requirement-candidate-content-clauses-06-object-management-page-122-l85-25",
        "requirement-candidate-content-clauses-06-object-management-page-122-l103-31",
        "requirement-candidate-content-clauses-08-time-management-page-185-l17-5",
        "requirement-candidate-content-clauses-08-time-management-page-194-l21-3",
        "requirement-candidate-content-clauses-08-time-management-page-194-l69-19",
        "requirement-candidate-content-clauses-08-time-management-page-197-l20-3",
        "requirement-candidate-content-clauses-08-time-management-page-197-l50-13",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l89-28",
    }:
        raise AssertionError("time-axis-independence requirement mapping drifted")
    if set(time_axis_independence.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-6.10",
        "hla-1516.1-2025:clause-6.10.5",
        "hla-1516.1-2025:clause-6.11.1",
        "hla-1516.1-2025:clause-8.1.6",
        "hla-1516.1-2025:clause-8.8.3",
        "hla-1516.1-2025:clause-8.10.2",
        "hla-1516.1-2025:clause-9.1.4",
    }:
        raise AssertionError("time-axis-independence section mapping drifted")
    time_axis_focus = query_rti_work.focused_lane_result(
        index, tests, "time-axis-independence", limit=0
    )
    if (
        time_axis_focus.get("lane_state") != "complete"
        or time_axis_focus.get("mapped_test_count") != 1
        or time_axis_focus.get("assertion_count") != 94
        or time_axis_focus.get("requirement_count") != 10
        or time_axis_focus.get("standard_section_count") != 7
        or time_axis_focus.get("requirement_section_pair_count") != 10
    ):
        raise AssertionError("time-axis-independence lane card drifted")
    time_axis_handles = time_axis_focus.get("lane_handles")
    if (
        not isinstance(time_axis_handles, dict)
        or time_axis_handles.get("catch2_target")
        != "umbra_timestamped_regional_attribute_update_tar_nmr_catch2"
        or time_axis_handles.get("ctest_label") != "time-axis-independence"
    ):
        raise AssertionError("time-axis-independence lane handles drifted")
    time_axis_case = query_rti_work.case_card_record(
        time_axis_independence,
        index,
        "umbra-cpp-timestamped-regional-attribute-update-tar-nmr-integration",
    )
    if (
        time_axis_case.get("traceability_state") != "requirements-mapped"
        or time_axis_case.get("lane") != "time-axis-independence"
        or "ctest" not in time_axis_case.get("commands", {})
    ):
        raise AssertionError("time-axis-independence card lost its handles")

    object_name_reservation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-object-instance-name-reservation-integration"
        ),
        None,
    )
    if not isinstance(object_name_reservation, dict):
        raise AssertionError("object-instance-name-reservation plan row is absent")
    if query_rti_work.source_location_text(object_name_reservation) != (
        "cpp/tests/object_instance_name_reservation_catch2.cpp:95"
    ) or object_name_reservation.get("assertions") != 68:
        raise AssertionError("object-instance-name-reservation source/evidence drifted")
    if object_name_reservation.get("traceability_state") != "requirements-mapped":
        raise AssertionError("object-instance-name-reservation row is not mapped")
    if set(object_name_reservation.get("lab_requirement_ids", [])) != {
        "requirement-candidate-content-clauses-06-object-management-page-113-l153-40",
        "requirement-candidate-content-clauses-06-object-management-page-113-l21-3",
        "requirement-candidate-content-clauses-06-object-management-page-113-l24-4",
        "requirement-candidate-content-clauses-06-object-management-page-114-l110-26",
        "requirement-candidate-content-clauses-06-object-management-page-114-l119-29",
        "requirement-candidate-content-clauses-06-object-management-page-115-l101-27",
        "requirement-candidate-content-clauses-06-object-management-page-115-l104-28",
        "requirement-candidate-content-clauses-06-object-management-page-115-l107-29",
        "requirement-candidate-content-clauses-06-object-management-page-115-l110-30",
        "requirement-candidate-content-clauses-06-object-management-page-117-l36-6",
        "requirement-candidate-content-clauses-06-object-management-page-117-l45-9",
        "requirement-candidate-content-clauses-06-object-management-page-117-l48-10",
    }:
        raise AssertionError("object-instance-name-reservation requirement mapping drifted")
    if set(object_name_reservation.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-6.2",
        "hla-1516.1-2025:clause-6.3",
        "hla-1516.1-2025:clause-6.4.3",
        "hla-1516.1-2025:clause-6.5.3",
        "hla-1516.1-2025:clause-6.7.5",
    }:
        raise AssertionError("object-instance-name-reservation section mapping drifted")
    object_name_focus = query_rti_work.focused_lane_result(
        index, tests, "object-instance-name-reservation", limit=0
    )
    if (
        object_name_focus.get("lane_state") != "complete"
        or object_name_focus.get("mapped_test_count") != 1
        or object_name_focus.get("assertion_count") != 68
        or object_name_focus.get("requirement_count") != 12
        or object_name_focus.get("standard_section_count") != 5
        or object_name_focus.get("requirement_section_pair_count") != 12
    ):
        raise AssertionError("object-instance-name-reservation lane card drifted")
    object_name_handles = object_name_focus.get("lane_handles")
    if (
        not isinstance(object_name_handles, dict)
        or object_name_handles.get("catch2_target")
        != "umbra_object_instance_name_reservation_catch2"
        or object_name_handles.get("ctest_label") != "object-instance-name-reservation"
    ):
        raise AssertionError("object-instance-name-reservation lane handles drifted")
    object_name_case = query_rti_work.case_card_record(
        object_name_reservation,
        index,
        "umbra-cpp-object-instance-name-reservation-integration",
    )
    if (
        object_name_case.get("traceability_state") != "requirements-mapped"
        or object_name_case.get("lane") != "object-instance-name-reservation"
        or "ctest" not in object_name_case.get("commands", {})
    ):
        raise AssertionError("object-instance-name-reservation card lost its handles")

    strict_relaxed = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-public-regional-automatic-provision-relaxed-ddm-integration"
        ),
        None,
    )
    if not isinstance(strict_relaxed, dict):
        raise AssertionError("strict-relaxed-ddm-boundary plan row is absent")
    if query_rti_work.source_location_text(strict_relaxed) != (
        "cpp/tests/regional_auto_provide_response_catch2.cpp:148"
    ) or strict_relaxed.get("assertions") != 240:
        raise AssertionError("strict-relaxed-ddm-boundary source/evidence drifted")
    if strict_relaxed.get("traceability_state") != "requirements-mapped":
        raise AssertionError("strict-relaxed-ddm-boundary row is not mapped")
    if set(strict_relaxed.get("lab_requirement_ids", [])) != {
        "requirement-candidate-content-clauses-01-overview-page-019-l62-7",
        "requirement-candidate-content-clauses-04-federation-management-page-076-l18-2",
        "requirement-candidate-content-clauses-06-object-management-page-110-l16-4",
        "requirement-candidate-content-clauses-06-object-management-page-110-l34-10",
        "requirement-candidate-content-clauses-06-object-management-page-120-l86-23",
        "requirement-candidate-content-clauses-06-object-management-page-120-l110-31",
        "requirement-candidate-content-clauses-06-object-management-page-121-l36-10",
        "requirement-candidate-content-clauses-06-object-management-page-122-l103-31",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l71-22",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-241-l88-26",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-241-l100-30",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-241-l145-45",
        "requirement-source-refinement-relaxed-ddm-overlap",
        "requirement-source-refinement-relaxed-ddm-delivery",
        "requirement-candidate-content-clauses-10-support-services-page-282-l152-47",
    }:
        raise AssertionError("strict-relaxed-ddm-boundary requirement mapping drifted")
    if set(strict_relaxed.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-1",
        "hla-1516.1-2025:clause-4.32",
        "hla-1516.1-2025:clause-6.1.10",
        "hla-1516.1-2025:clause-6.10",
        "hla-1516.1-2025:clause-6.10.5",
        "hla-1516.1-2025:clause-6.11.1",
        "hla-1516.1-2025:clause-9.1.4",
        "hla-1516.1-2025:clause-9.1.8",
        "hla-1516.1-2025:clause-9.13.1",
        "hla-1516.1-2025:clause-10.55.1",
    }:
        raise AssertionError("strict-relaxed-ddm-boundary section mapping drifted")
    strict_relaxed_focus = query_rti_work.focused_lane_result(
        index, tests, "strict-relaxed-ddm-boundary", limit=0
    )
    if (
        strict_relaxed_focus.get("lane_state") != "complete"
        or strict_relaxed_focus.get("mapped_test_count") != 1
        or strict_relaxed_focus.get("assertion_count") != 240
        or strict_relaxed_focus.get("requirement_count") != 15
        or strict_relaxed_focus.get("standard_section_count") != 10
        or strict_relaxed_focus.get("requirement_section_pair_count") != 15
    ):
        raise AssertionError("strict-relaxed-ddm-boundary lane card drifted")
    strict_relaxed_handles = strict_relaxed_focus.get("lane_handles")
    if (
        not isinstance(strict_relaxed_handles, dict)
        or strict_relaxed_handles.get("catch2_target")
        != "umbra_regional_auto_provide_response_catch2"
        or strict_relaxed_handles.get("ctest_label")
        != "strict-relaxed-ddm-boundary"
    ):
        raise AssertionError("strict-relaxed-ddm-boundary lane handles drifted")
    strict_relaxed_case = query_rti_work.case_card_record(
        strict_relaxed,
        index,
        "umbra-cpp-public-regional-automatic-provision-relaxed-ddm-integration",
    )
    if (
        strict_relaxed_case.get("traceability_state") != "requirements-mapped"
        or strict_relaxed_case.get("lane") != "strict-relaxed-ddm-boundary"
        or "ctest" not in strict_relaxed_case.get("commands", {})
    ):
        raise AssertionError("strict-relaxed-ddm-boundary card lost its handles")

    subscription_dimension = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-regional-interaction-subscription-dimension-validation-integration"
        ),
        None,
    )
    if not isinstance(subscription_dimension, dict):
        raise AssertionError("regional subscription-dimension plan row is absent")
    if query_rti_work.source_location_text(subscription_dimension) != (
        "cpp/tests/mixed_region_interaction_validation_catch2.cpp:228"
    ) or subscription_dimension.get("assertions") != 36:
        raise AssertionError("regional subscription-dimension source/evidence drifted")
    if subscription_dimension.get("traceability_state") != "requirements-mapped":
        raise AssertionError("regional subscription-dimension row is not mapped")
    if subscription_dimension.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.4",
    ]:
        raise AssertionError("regional subscription-dimension section mapping drifted")
    if subscription_dimension.get("lab_requirement_ids") != [
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l20-6",
    ]:
        raise AssertionError("regional subscription-dimension requirement mapping drifted")
    subscription_dimension_focus = query_rti_work.focused_lane_result(
        index, tests, "subscription-dimension-validation", limit=0
    )
    if (
        subscription_dimension_focus.get("lane_state") != "complete"
        or subscription_dimension_focus.get("mapped_test_count") != 1
        or subscription_dimension_focus.get("assertion_count") != 36
        or subscription_dimension_focus.get("requirement_count") != 1
        or subscription_dimension_focus.get("standard_section_count") != 1
        or subscription_dimension_focus.get("requirement_section_pair_count") != 1
    ):
        raise AssertionError("regional subscription-dimension lane card drifted")
    subscription_dimension_handles = subscription_dimension_focus.get("lane_handles")
    if (
        not isinstance(subscription_dimension_handles, dict)
        or subscription_dimension_handles.get("catch2_target")
        != "umbra_mixed_region_interaction_validation_catch2"
        or subscription_dimension_handles.get("ctest_label")
        != "subscription-dimension-validation"
    ):
        raise AssertionError("regional subscription-dimension lane handles drifted")
    subscription_dimension_case = query_rti_work.case_card_record(
        subscription_dimension,
        index,
        "umbra-cpp-regional-interaction-subscription-dimension-validation-integration",
    )
    if (
        subscription_dimension_case.get("traceability_state")
        != "requirements-mapped"
        or subscription_dimension_case.get("lane")
        != "subscription-dimension-validation"
        or "ctest" not in subscription_dimension_case.get("commands", {})
    ):
        raise AssertionError("regional subscription-dimension card lost its handles")

    subscription_empty = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-regional-interaction-subscription-empty-set-integration"
        ),
        None,
    )
    if not isinstance(subscription_empty, dict):
        raise AssertionError("empty regional subscription-set plan row is absent")
    if query_rti_work.source_location_text(subscription_empty) != (
        "cpp/tests/multi_region_interaction_routing_catch2.cpp:307"
    ) or subscription_empty.get("assertions") != 23:
        raise AssertionError("empty regional subscription-set source/evidence drifted")
    if subscription_empty.get("traceability_state") != "requirements-mapped":
        raise AssertionError("empty regional subscription-set row is not mapped")
    if subscription_empty.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.4",
    ]:
        raise AssertionError("empty regional subscription-set section mapping drifted")
    if set(subscription_empty.get("lab_requirement_ids", [])) != {
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l14-4",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l8-2",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l41-13",
    }:
        raise AssertionError("empty regional subscription-set requirement mapping drifted")
    subscription_empty_focus = query_rti_work.focused_lane_result(
        index, tests, "subscription-empty-set", limit=0
    )
    if (
        subscription_empty_focus.get("lane_state") != "complete"
        or subscription_empty_focus.get("mapped_test_count") != 1
        or subscription_empty_focus.get("assertion_count") != 23
        or subscription_empty_focus.get("requirement_count") != 3
        or subscription_empty_focus.get("standard_section_count") != 1
        or subscription_empty_focus.get("requirement_section_pair_count") != 3
    ):
        raise AssertionError("empty regional subscription-set lane card drifted")
    subscription_empty_handles = subscription_empty_focus.get("lane_handles")
    if (
        not isinstance(subscription_empty_handles, dict)
        or subscription_empty_handles.get("catch2_target")
        != "umbra_multi_region_interaction_routing_catch2"
        or subscription_empty_handles.get("ctest_label")
        != "subscription-empty-set"
    ):
        raise AssertionError("empty regional subscription-set lane handles drifted")
    subscription_empty_case = query_rti_work.case_card_record(
        subscription_empty,
        index,
        "umbra-cpp-regional-interaction-subscription-empty-set-integration",
    )
    if (
        subscription_empty_case.get("traceability_state") != "requirements-mapped"
        or subscription_empty_case.get("lane") != "subscription-empty-set"
        or "ctest" not in subscription_empty_case.get("commands", {})
    ):
        raise AssertionError("exact empty regional subscription-set card lost its handles")

    # The service-report encoding reconciliation is deliberately represented
    # by exact plan rows rather than a source-only queue.  Keep one of those
    # rows protected as the canonical example for the one-case handoff: its
    # mapping state, source pointer, standard subsection, and focused lane
    # must all be visible without reopening the full plan.
    service_report_case = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-mom-service-report-query-federation-restore-status-unit"
        ),
        None,
    )
    if not isinstance(service_report_case, dict):
        raise AssertionError("service-report encoding reconciliation row is absent")
    if (
        service_report_case.get("traceability_state") != "requirements-mapped"
        or query_rti_work.source_location_text(service_report_case)
        != "cpp/tests/mom_service_report_encoding_catch2.cpp:1490"
        or "hla-1516.1-2025:clause-11.5.1"
        not in service_report_case.get("standard_sections", [])
    ):
        raise AssertionError("service-report encoding row lost its bounded mapping")
    service_report_case_card = query_rti_work.case_card_record(
        service_report_case,
        index,
        "umbra-cpp-mom-service-report-query-federation-restore-status-unit",
    )
    if (
        service_report_case_card.get("traceability_state") != "requirements-mapped"
        or service_report_case_card.get("lane") != "service-report-encoding"
        or "ctest" not in service_report_case_card.get("commands", {})
    ):
        raise AssertionError("exact service-report case card lost its mapping handles")
    if "traceability: requirements-mapped" not in query_rti_work.text_case_card(
        service_report_case_card,
        compact=True,
    ):
        raise AssertionError("case-card text omitted the mapping state")
    object_ddm_item = next(
        (item for item in index.get("items", []) if item.get("id") == "object-ddm-ownership"),
        None,
    )
    if not isinstance(object_ddm_item, dict):
        raise AssertionError("object-ddm roadmap family is absent")
    if "passive-regional-interaction-transition" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("passive regional interaction lane is not roadmap-indexed")
    if "focused-positive-dimensional-regional-interaction" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError(
            "focused positive-dimensional regional interaction lane is not roadmap-indexed"
        )
    if "multi-region-regional-interaction" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("multi-region interaction lane is not roadmap-indexed")
    if "subscription-dimension-validation" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("regional subscription-dimension lane is not roadmap-indexed")
    if "subscription-empty-set" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("empty regional subscription-set lane is not roadmap-indexed")
    if "whole-class-unsubscribe" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("whole-class default-region lane is not roadmap-indexed")
    if "no-common-dimension" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("no-common-dimension lane is not roadmap-indexed")
    if "mixed-region-interaction-validation" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("mixed-region interaction lane is not roadmap-indexed")
    passive_regional_transition_focus = query_rti_work.focused_lane_result(
        index, tests, "passive-regional-interaction-transition", limit=0
    )
    if passive_regional_transition_focus.get("lane_state") != "complete":
        raise AssertionError("focused passive regional interaction lane is not complete")
    if passive_regional_transition_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused passive regional interaction mapped count drifted")
    if passive_regional_transition_focus.get("requirement_count") != 4:
        raise AssertionError("focused passive regional interaction requirement count drifted")
    if passive_regional_transition_focus.get("requirement_section_pair_count") != 4:
        raise AssertionError("focused passive regional interaction direct-pair count drifted")
    passive_regional_transition_handles = passive_regional_transition_focus.get("lane_handles")
    if not isinstance(passive_regional_transition_handles, dict) or passive_regional_transition_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_catch2":
        raise AssertionError("focused passive regional interaction target handle drifted")
    negotiated_confirmation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-negotiated-attribute-ownership-divestiture-integration"
        ),
        None,
    )
    if not isinstance(negotiated_confirmation, dict):
        raise AssertionError("negotiated ownership confirmation plan row is absent")
    if query_rti_work.source_location_text(negotiated_confirmation) != (
        "cpp/tests/negotiated_attribute_ownership_divestiture_pending_catch2.cpp:204"
    ) or negotiated_confirmation.get("assertions") != 103:
        raise AssertionError("negotiated ownership confirmation source/evidence drifted")
    if negotiated_confirmation.get("traceability_state") != "requirements-mapped":
        raise AssertionError("negotiated ownership confirmation row is not mapped")
    negotiated_confirmation_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "negotiated-divestiture-confirmation-pending",
        limit=0,
    )
    if (
        negotiated_confirmation_focus.get("lane_state") != "complete"
        or negotiated_confirmation_focus.get("mapped_test_count") != 1
        or negotiated_confirmation_focus.get("assertion_count") != 103
        or negotiated_confirmation_focus.get("requirement_count") != 14
        or negotiated_confirmation_focus.get("standard_section_count") != 7
        or negotiated_confirmation_focus.get("requirement_section_pair_count") != 14
    ):
        raise AssertionError("negotiated ownership confirmation lane card drifted")
    negotiated_confirmation_handles = negotiated_confirmation_focus.get("lane_handles")
    if not isinstance(negotiated_confirmation_handles, dict) or negotiated_confirmation_handles.get(
        "catch2_target"
    ) != "umbra_negotiated_attribute_ownership_divestiture_pending_catch2":
        raise AssertionError("negotiated ownership confirmation target handle drifted")
    if "negotiated-divestiture-confirmation-pending" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("negotiated ownership confirmation lane is not roadmap-indexed")
    cancellation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-acquisition-cancellation-integration"
        ),
        None,
    )
    if not isinstance(cancellation, dict):
        raise AssertionError("attribute-ownership acquisition cancellation plan row is absent")
    if query_rti_work.source_location_text(cancellation) != (
        "cpp/tests/attribute_ownership_acquisition_cancellation_catch2.cpp:199"
    ) or cancellation.get("assertions") != 51:
        raise AssertionError("attribute-ownership acquisition cancellation source/evidence drifted")
    if cancellation.get("traceability_state") != "requirements-mapped":
        raise AssertionError("attribute-ownership acquisition cancellation row is not mapped")
    cancellation_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "cancel-attribute-ownership-acquisition-confirmation-boundary",
        limit=0,
    )
    if (
        cancellation_focus.get("lane_state") != "complete"
        or cancellation_focus.get("mapped_test_count") != 1
        or cancellation_focus.get("assertion_count") != 51
        or cancellation_focus.get("requirement_count") != 18
        or cancellation_focus.get("standard_section_count") != 4
        or cancellation_focus.get("requirement_section_pair_count") != 18
    ):
        raise AssertionError("attribute-ownership acquisition cancellation lane card drifted")
    cancellation_handles = cancellation_focus.get("lane_handles")
    if not isinstance(cancellation_handles, dict) or cancellation_handles.get(
        "catch2_target"
    ) != "umbra_attribute_ownership_acquisition_cancellation_catch2":
        raise AssertionError("attribute-ownership acquisition cancellation target handle drifted")
    if cancellation_handles.get("ctest_filter") != (
        "^umbra\\.attribute_ownership_acquisition_cancellation\\.catch2\\.Embedded Cancel "
        "Attribute Ownership Acquisition honors the 2025 confirmation boundary$"
    ):
        raise AssertionError("attribute-ownership acquisition cancellation CTest filter drifted")
    if "cancel-attribute-ownership-acquisition-confirmation-boundary" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("attribute-ownership acquisition cancellation lane is not roadmap-indexed")
    acquisition_release = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-attribute-ownership-acquisition-integration"
        ),
        None,
    )
    if not isinstance(acquisition_release, dict):
        raise AssertionError("attribute-ownership acquisition release plan row is absent")
    if query_rti_work.source_location_text(acquisition_release) != (
        "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:54194"
    ) or acquisition_release.get("assertions") != 73:
        raise AssertionError("attribute-ownership acquisition release source/evidence drifted")
    if acquisition_release.get("traceability_state") != "requirements-mapped":
        raise AssertionError("attribute-ownership acquisition release row is not mapped")
    acquisition_release_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "attribute-ownership-acquisition-release-callback",
        limit=0,
    )
    if (
        acquisition_release_focus.get("lane_state") != "complete"
        or acquisition_release_focus.get("mapped_test_count") != 1
        or acquisition_release_focus.get("assertion_count") != 73
        or acquisition_release_focus.get("requirement_count") != 9
        or acquisition_release_focus.get("standard_section_count") != 4
        or acquisition_release_focus.get("requirement_section_pair_count") != 9
    ):
        raise AssertionError("attribute-ownership acquisition release lane card drifted")
    acquisition_release_handles = acquisition_release_focus.get("lane_handles")
    if not isinstance(acquisition_release_handles, dict) or acquisition_release_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_catch2":
        raise AssertionError("attribute-ownership acquisition release target handle drifted")
    if acquisition_release_handles.get("ctest_filter") != (
        "^umbra\\.ieee1516_2025\\.catch2\\.Embedded Attribute Ownership Acquisition "
        "honors 2025 release and denial callbacks$"
    ):
        raise AssertionError("attribute-ownership acquisition release CTest filter drifted")
    if "attribute-ownership-acquisition-release-callback" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("attribute-ownership acquisition release lane is not roadmap-indexed")
    divestiture_if_wanted = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-attribute-ownership-divestiture-if-wanted-integration"
        ),
        None,
    )
    if not isinstance(divestiture_if_wanted, dict):
        raise AssertionError("Divestiture If Wanted plan row is absent")
    if query_rti_work.source_location_text(divestiture_if_wanted) != (
        "cpp/tests/attribute_ownership_divestiture_if_wanted_pending_catch2.cpp:136"
    ) or divestiture_if_wanted.get("assertions") != 66:
        raise AssertionError("Divestiture If Wanted source/evidence drifted")
    if divestiture_if_wanted.get("traceability_state") != "requirements-mapped":
        raise AssertionError("Divestiture If Wanted row is not mapped")
    divestiture_if_wanted_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "attribute-ownership-divestiture-if-wanted-pending",
        limit=0,
    )
    if (
        divestiture_if_wanted_focus.get("lane_state") != "complete"
        or divestiture_if_wanted_focus.get("mapped_test_count") != 1
        or divestiture_if_wanted_focus.get("assertion_count") != 66
        or divestiture_if_wanted_focus.get("requirement_count") != 10
        or divestiture_if_wanted_focus.get("standard_section_count") != 5
        or divestiture_if_wanted_focus.get("requirement_section_pair_count") != 10
    ):
        raise AssertionError("Divestiture If Wanted lane card drifted")
    divestiture_if_wanted_handles = divestiture_if_wanted_focus.get("lane_handles")
    if not isinstance(divestiture_if_wanted_handles, dict) or divestiture_if_wanted_handles.get(
        "catch2_target"
    ) != "umbra_attribute_ownership_divestiture_if_wanted_pending_catch2":
        raise AssertionError("Divestiture If Wanted target handle drifted")
    if divestiture_if_wanted_handles.get("ctest_filter") != (
        "^umbra\\.attribute_ownership_divestiture_if_wanted_pending\\.catch2\\.Embedded "
        "Attribute Ownership Divestiture If Wanted transfers only to 2025 pending acquirers$"
    ):
        raise AssertionError("Divestiture If Wanted CTest filter drifted")
    if "attribute-ownership-divestiture-if-wanted-pending" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("Divestiture If Wanted lane is not roadmap-indexed")
    ownership_query = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-attribute-ownership-query-integration"
        ),
        None,
    )
    if not isinstance(ownership_query, dict):
        raise AssertionError("attribute-ownership query plan row is absent")
    if query_rti_work.source_location_text(ownership_query) != (
        "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:52526"
    ) or ownership_query.get("assertions") != 49:
        raise AssertionError("attribute-ownership query source/evidence drifted")
    if ownership_query.get("traceability_state") != "requirements-mapped":
        raise AssertionError("attribute-ownership query row is not mapped")
    ownership_query_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "attribute-ownership-query-results-removal",
        limit=0,
    )
    if (
        ownership_query_focus.get("lane_state") != "complete"
        or ownership_query_focus.get("mapped_test_count") != 1
        or ownership_query_focus.get("assertion_count") != 49
        or ownership_query_focus.get("requirement_count") != 9
        or ownership_query_focus.get("standard_section_count") != 2
        or ownership_query_focus.get("requirement_section_pair_count") != 9
    ):
        raise AssertionError("attribute-ownership query lane card drifted")
    ownership_query_handles = ownership_query_focus.get("lane_handles")
    if not isinstance(ownership_query_handles, dict) or ownership_query_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_catch2":
        raise AssertionError("attribute-ownership query target handle drifted")
    if ownership_query_handles.get("ctest_filter") != (
        "^umbra\\.ieee1516_2025\\.catch2\\.Embedded Query Attribute Ownership reports 2025 "
        "federate and unowned attributes$"
    ):
        raise AssertionError("attribute-ownership query CTest filter drifted")
    if "attribute-ownership-query-results-removal" not in object_ddm_item.get(
        "focused_lane_tags", []
    ):
        raise AssertionError("attribute-ownership query lane is not roadmap-indexed")
    evoked_regional_snapshot = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-evoked-regional-interaction-source-region-snapshot-integration"
        ),
        None,
    )
    if not isinstance(evoked_regional_snapshot, dict):
        raise AssertionError("evoked regional source-region snapshot plan row is absent")
    if query_rti_work.source_location_text(evoked_regional_snapshot) != (
        "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:44352"
    ) or evoked_regional_snapshot.get("assertions") != 42:
        raise AssertionError("evoked regional source-region snapshot source/evidence drifted")
    if evoked_regional_snapshot.get("traceability_state") != "requirements-mapped":
        raise AssertionError("evoked regional source-region snapshot row is not mapped")
    evoked_regional_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "evoked-regional-interaction-source-region-snapshot",
        limit=0,
    )
    if (
        evoked_regional_focus.get("lane_state") != "complete"
        or evoked_regional_focus.get("mapped_test_count") != 1
        or evoked_regional_focus.get("assertion_count") != 42
        or evoked_regional_focus.get("requirement_count") != 9
        or evoked_regional_focus.get("standard_section_count") != 9
        or evoked_regional_focus.get("requirement_section_pair_count") != 9
    ):
        raise AssertionError("evoked regional source-region snapshot lane card drifted")
    evoked_regional_handles = evoked_regional_focus.get("lane_handles")
    if not isinstance(evoked_regional_handles, dict):
        raise AssertionError("evoked regional source-region snapshot lane lost its handles")
    if evoked_regional_handles.get("catch2_target") != (
        "umbra_ieee1516_2025_catch2"
    ) or evoked_regional_handles.get("ctest_filter") != (
        "^umbra\\.ieee1516_2025\\.catch2\\.Embedded evoked regional interaction retains its send-time source region$"
    ):
        raise AssertionError("evoked regional source-region snapshot CTest handles drifted")
    register_default_region = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-register-object-instance-with-regions-default-region-generated-name-unit"
        ),
        None,
    )
    if not isinstance(register_default_region, dict):
        raise AssertionError("default-region generated-name plan row is absent")
    if query_rti_work.source_location_text(register_default_region) != (
        "cpp/tests/federation_registry_catch2.cpp:12264"
    ):
        raise AssertionError("default-region generated-name source pointer drifted")
    if register_default_region.get("assertions") != 48:
        raise AssertionError("default-region generated-name assertion count drifted")
    if register_default_region.get("traceability_state") != "requirements-mapped":
        raise AssertionError("default-region generated-name row is not mapped")
    register_default_region_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "register-object-instance-with-regions-default-region-generated-name",
        limit=0,
    )
    if (
        register_default_region_focus.get("lane_state") != "complete"
        or register_default_region_focus.get("mapped_test_count") != 2
        or register_default_region_focus.get("assertion_count") != 96
        or register_default_region_focus.get("requirement_count") != 4
        or register_default_region_focus.get("standard_section_count") != 2
        or register_default_region_focus.get("requirement_section_pair_count") != 4
    ):
        raise AssertionError("default-region generated-name lane card drifted")
    register_default_region_handles = register_default_region_focus.get("lane_handles")
    if not isinstance(register_default_region_handles, dict):
        raise AssertionError("default-region generated-name lane lost its handles")
    if register_default_region_handles.get("catch2_target") != (
        "umbra_federation_registry_catch2"
    ) or register_default_region_handles.get("ctest_filter") != (
        "^umbra\\.federation_registry\\.catch2\\.Register Object Instance With Regions uses the default region and execution-wide generated names$"
    ):
        raise AssertionError("default-region generated-name CTest handles drifted")
    register_default_region_trace = query_rti_work.roadmap_links_for_test(
        index, register_default_region, limit=0
    )
    if not register_default_region_trace or register_default_region_trace[0].get("id") != "object-ddm-ownership":
        raise AssertionError("default-region generated-name trace lost its owning family")
    register_default_region_family = next(
        (
            family
            for family in query_rti_work.roadmap_inventory(
                index, tests, query="object-ddm-ownership", status="open", limit=0
            ).get("families", [])
            if isinstance(family, dict) and family.get("id") == "object-ddm-ownership"
        ),
        None,
    )
    if not isinstance(register_default_region_family, dict) or "register-object-instance-with-regions-default-region-generated-name" not in (
        register_default_region_family.get("query_tags") or []
    ):
        raise AssertionError("object/DDM family lost the default-region generated-name focus alias")
    public_register_default_region = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-register-object-instance-with-regions-default-region-generated-name-integration"
        ),
        None,
    )
    if not isinstance(public_register_default_region, dict):
        raise AssertionError("public process default-region generated-name plan row is absent")
    if query_rti_work.source_location_text(public_register_default_region) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:17095"
    ):
        raise AssertionError("public process default-region generated-name source pointer drifted")
    if public_register_default_region.get("assertions") != 48:
        raise AssertionError("public process default-region generated-name assertion count drifted")
    if public_register_default_region.get("traceability_state") != "requirements-mapped":
        raise AssertionError("public process default-region generated-name row is not mapped")
    public_register_default_region_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "public-process-register-object-instance-with-regions-default-region-generated-name",
        limit=0,
    )
    if (
        public_register_default_region_focus.get("lane_state") != "complete"
        or public_register_default_region_focus.get("mapped_test_count") != 1
        or public_register_default_region_focus.get("assertion_count") != 48
        or public_register_default_region_focus.get("requirement_count") != 4
        or public_register_default_region_focus.get("standard_section_count") != 2
        or public_register_default_region_focus.get("requirement_section_pair_count") != 4
    ):
        raise AssertionError("public process default-region generated-name lane card drifted")
    public_register_default_region_handles = public_register_default_region_focus.get("lane_handles")
    if not isinstance(public_register_default_region_handles, dict):
        raise AssertionError("public process default-region generated-name lane lost its handles")
    if public_register_default_region_handles.get("catch2_target") != (
        "umbra_ieee1516_2025_connection_catch2"
    ) or public_register_default_region_handles.get("ctest_filter") != (
        "^umbra\\.ieee1516_2025\\.connection_catch2\\.RTIambassador registers available-dimensional attributes on the default region and exposes generated names through a configured process endpoint$"
    ):
        raise AssertionError("public process default-region generated-name CTest handles drifted")
    deferred_state_image_restore = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-federation-save-commit-filesystem-process-restart-deferred-update-region-association-integration"
        ),
        None,
    )
    if not isinstance(deferred_state_image_restore, dict):
        raise AssertionError("deferred state-image restore plan row is absent")
    if query_rti_work.source_location_text(deferred_state_image_restore) != (
        "cpp/tests/federation_registry_catch2.cpp:11991"
    ):
        raise AssertionError("deferred state-image restore source pointer drifted")
    if deferred_state_image_restore.get("assertions") != 79:
        raise AssertionError("deferred state-image restore assertion count drifted")
    if deferred_state_image_restore.get("traceability_state") != "requirements-mapped":
        raise AssertionError("deferred state-image restore row is not mapped")
    deferred_state_image_restore_focus = query_rti_work.focused_lane_result(
        index, tests, "deferred-update-region-association-state-image-restore", limit=0
    )
    if (
        deferred_state_image_restore_focus.get("lane_state") != "complete"
        or deferred_state_image_restore_focus.get("mapped_test_count") != 1
        or deferred_state_image_restore_focus.get("assertion_count") != 79
        or deferred_state_image_restore_focus.get("requirement_count") != 7
        or deferred_state_image_restore_focus.get("standard_section_count") != 5
        or deferred_state_image_restore_focus.get("requirement_section_pair_count") != 7
    ):
        raise AssertionError("deferred state-image restore lane card drifted")
    deferred_state_image_restore_handles = deferred_state_image_restore_focus.get("lane_handles")
    if not isinstance(deferred_state_image_restore_handles, dict):
        raise AssertionError("deferred state-image restore lane lost its handles")
    if deferred_state_image_restore_handles.get("catch2_target") != (
        "umbra_federation_registry_catch2"
    ) or deferred_state_image_restore_handles.get("ctest_filter") != (
        "^umbra\\.federation_registry\\.catch2\\.Filesystem fresh-registry restore promotes a deferred update-region association after ownership acquisition$"
    ):
        raise AssertionError("deferred state-image restore CTest handles drifted")
    deferred_state_image_restore_trace = query_rti_work.roadmap_links_for_test(
        index, deferred_state_image_restore, limit=0
    )
    if not deferred_state_image_restore_trace or deferred_state_image_restore_trace[0].get("id") != "time-save-restore":
        raise AssertionError("deferred state-image restore trace lost its owning family")
    deferred_state_image_restore_family = next(
        (
            family
            for family in query_rti_work.roadmap_inventory(
                index, tests, query="time-save-restore", status="open", limit=0
            ).get("families", [])
            if isinstance(family, dict) and family.get("id") == "time-save-restore"
        ),
        None,
    )
    if not isinstance(deferred_state_image_restore_family, dict) or "deferred-update-region-association-state-image-restore" not in (
        deferred_state_image_restore_family.get("query_tags") or []
    ):
        raise AssertionError("time/save/restore family lost the deferred state-image restore focus alias")
    deferred_state_image = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-federation-state-image-deferred-update-region-association-unit"
        ),
        None,
    )
    if not isinstance(deferred_state_image, dict):
        raise AssertionError("deferred state-image plan row is absent")
    if query_rti_work.source_location_text(deferred_state_image) != (
        "cpp/tests/federation_registry_catch2.cpp:11943"
    ):
        raise AssertionError("deferred state-image source pointer drifted")
    if deferred_state_image.get("assertions") != 9:
        raise AssertionError("deferred state-image assertion count drifted")
    if deferred_state_image.get("traceability_state") != "requirements-mapped":
        raise AssertionError("deferred state-image row is not mapped")
    deferred_state_image_focus = query_rti_work.focused_lane_result(
        index, tests, "deferred-update-region-association-state-image", limit=0
    )
    if (
        deferred_state_image_focus.get("lane_state") != "complete"
        or deferred_state_image_focus.get("mapped_test_count") != 1
        or deferred_state_image_focus.get("assertion_count") != 9
        or deferred_state_image_focus.get("requirement_count") != 2
        or deferred_state_image_focus.get("standard_section_count") != 2
        or deferred_state_image_focus.get("requirement_section_pair_count") != 2
    ):
        raise AssertionError("deferred state-image lane card drifted")
    deferred_state_image_handles = deferred_state_image_focus.get("lane_handles")
    if not isinstance(deferred_state_image_handles, dict):
        raise AssertionError("deferred state-image lane lost its handles")
    if deferred_state_image_handles.get("catch2_target") != (
        "umbra_federation_registry_catch2"
    ) or deferred_state_image_handles.get("ctest_filter") != (
        "^umbra\\.federation_registry\\.catch2\\.Federation state images preserve deferred update-region associations across a v1 codec round trip$"
    ):
        raise AssertionError("deferred state-image CTest handles drifted")
    deferred_state_image_trace = query_rti_work.roadmap_links_for_test(
        index, deferred_state_image, limit=0
    )
    if not deferred_state_image_trace or deferred_state_image_trace[0].get("id") != "time-save-restore":
        raise AssertionError("deferred state-image trace lost its owning family")
    deferred_state_image_family = next(
        (
            family
            for family in query_rti_work.roadmap_inventory(
                index, tests, query="time-save-restore", status="open", limit=0
            ).get("families", [])
            if isinstance(family, dict) and family.get("id") == "time-save-restore"
        ),
        None,
    )
    if not isinstance(deferred_state_image_family, dict) or "deferred-update-region-association-state-image" not in (
        deferred_state_image_family.get("query_tags") or []
    ):
        raise AssertionError("time/save/restore family lost the deferred state-image focus alias")
    deferred_cancel = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-ownership-transfer-update-region-deferred-cancel-integration"
        ),
        None,
    )
    if not isinstance(deferred_cancel, dict):
        raise AssertionError("cancelled deferred ownership-transfer plan row is absent")
    if query_rti_work.source_location_text(deferred_cancel) != (
        "cpp/tests/ownership_transfer_update_region_catch2.cpp:620"
    ):
        raise AssertionError("cancelled deferred ownership-transfer source pointer drifted")
    if deferred_cancel.get("assertions") != 66:
        raise AssertionError("cancelled deferred ownership-transfer assertion count drifted")
    if deferred_cancel.get("traceability_state") != "requirements-mapped":
        raise AssertionError("cancelled deferred ownership-transfer row is not mapped")
    deferred_cancel_focus = query_rti_work.focused_lane_result(
        index, tests, "ownership-transfer-update-region-deferred-cancel", limit=0
    )
    if (
        deferred_cancel_focus.get("lane_state") != "complete"
        or deferred_cancel_focus.get("mapped_test_count") != 1
        or deferred_cancel_focus.get("assertion_count") != 66
        or deferred_cancel_focus.get("requirement_count") != 1
        or deferred_cancel_focus.get("standard_section_count") != 1
        or deferred_cancel_focus.get("requirement_section_pair_count") != 1
    ):
        raise AssertionError("cancelled deferred ownership-transfer lane card drifted")
    deferred_cancel_handles = deferred_cancel_focus.get("lane_handles")
    if not isinstance(deferred_cancel_handles, dict):
        raise AssertionError("cancelled deferred ownership-transfer lane lost its handles")
    if deferred_cancel_handles.get("catch2_target") != (
        "umbra_ownership_transfer_update_region_catch2"
    ) or deferred_cancel_handles.get("ctest_filter") != (
        "^umbra\\.ownership_transfer_update_region\\.catch2\\.Embedded ownership transfer drops a non-owner's cancelled 2025 update-region association$"
    ):
        raise AssertionError("cancelled deferred ownership-transfer CTest handles drifted")
    deferred_cancel_trace = query_rti_work.roadmap_links_for_test(
        index, deferred_cancel, limit=0
    )
    if not deferred_cancel_trace or deferred_cancel_trace[0].get("id") != "object-ddm-ownership":
        raise AssertionError("cancelled deferred ownership-transfer trace lost its owning family")
    deferred_transfer = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-ownership-transfer-update-region-deferred-integration"
        ),
        None,
    )
    if not isinstance(deferred_transfer, dict):
        raise AssertionError("deferred ownership-transfer plan row is absent")
    if query_rti_work.source_location_text(deferred_transfer) != (
        "cpp/tests/ownership_transfer_update_region_catch2.cpp:406"
    ):
        raise AssertionError("deferred ownership-transfer source pointer drifted")
    if deferred_transfer.get("assertions") != 51:
        raise AssertionError("deferred ownership-transfer assertion count drifted")
    if deferred_transfer.get("traceability_state") != "requirements-mapped":
        raise AssertionError("deferred ownership-transfer row is not mapped")
    deferred_transfer_focus = query_rti_work.focused_lane_result(
        index, tests, "ownership-transfer-update-region-deferred", limit=0
    )
    if (
        deferred_transfer_focus.get("lane_state") != "complete"
        or deferred_transfer_focus.get("mapped_test_count") != 1
        or deferred_transfer_focus.get("assertion_count") != 51
        or deferred_transfer_focus.get("requirement_count") != 2
        or deferred_transfer_focus.get("standard_section_count") != 1
        or deferred_transfer_focus.get("requirement_section_pair_count") != 2
    ):
        raise AssertionError("deferred ownership-transfer lane card drifted")
    deferred_handles = deferred_transfer_focus.get("lane_handles")
    if not isinstance(deferred_handles, dict):
        raise AssertionError("deferred ownership-transfer lane lost its handles")
    if deferred_handles.get("catch2_target") != (
        "umbra_ownership_transfer_update_region_catch2"
    ) or deferred_handles.get("ctest_filter") != (
        "^umbra\\.ownership_transfer_update_region\\.catch2\\.Embedded ownership transfer activates a non-owner's deferred 2025 update-region association$"
    ):
        raise AssertionError("deferred ownership-transfer CTest handles drifted")
    deferred_trace = query_rti_work.roadmap_links_for_test(
        index, deferred_transfer, limit=0
    )
    if not deferred_trace or deferred_trace[0].get("id") != "object-ddm-ownership":
        raise AssertionError("deferred ownership-transfer trace lost its owning family")
    deferred_family = next(
        (
            family
            for family in query_rti_work.roadmap_inventory(
                index, tests, query="object-ddm-ownership", status="open", limit=0
            ).get("families", [])
            if isinstance(family, dict) and family.get("id") == "object-ddm-ownership"
        ),
        None,
    )
    if not isinstance(deferred_family, dict) or "ownership-transfer-update-region-deferred" not in (
        deferred_family.get("query_tags") or []
    ):
        raise AssertionError("object-DDM family lost the deferred ownership focus alias")
    cmake_text = (REPOSITORY_ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    if "umbra_ownership_transfer_update_region_catch2" not in cmake_text:
        raise AssertionError("deferred ownership-transfer focused target disappeared")
    timestamped_regional = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-regional-interaction-integration"
        ),
        None,
    )
    if not isinstance(timestamped_regional, dict):
        raise AssertionError("focused timestamped regional process row is absent")
    if query_rti_work.source_location_text(timestamped_regional) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:15195"
    ):
        raise AssertionError("focused timestamped regional process source pointer drifted")
    if timestamped_regional.get("assertions") != 71:
        raise AssertionError("focused timestamped regional process assertion count drifted")
    if timestamped_regional.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused timestamped regional process row is not mapped")
    timestamped_regional_focus = query_rti_work.focused_lane_result(
        index, tests, "timestamped-process-regional-interaction", limit=0
    )
    if timestamped_regional_focus.get("lane_state") != "complete":
        raise AssertionError("focused timestamped regional process lane is not complete")
    if timestamped_regional_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused timestamped regional process mapped count drifted")
    if timestamped_regional_focus.get("assertion_count") != 71:
        raise AssertionError("focused timestamped regional process lane assertion total drifted")
    if timestamped_regional_focus.get("requirement_count") != 28:
        raise AssertionError("focused timestamped regional process requirement total drifted")
    if timestamped_regional_focus.get("standard_section_count") != 16:
        raise AssertionError("focused timestamped regional process section total drifted")
    if timestamped_regional_focus.get("requirement_section_pair_count") != 28:
        raise AssertionError("focused timestamped regional process direct pair drifted")
    timestamped_regional_handles = timestamped_regional_focus.get("lane_handles")
    if not isinstance(timestamped_regional_handles, dict) or timestamped_regional_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused timestamped regional process target handle drifted")
    timestamped_regional_links = query_rti_work.roadmap_links_for_test(
        index, timestamped_regional, limit=0
    )
    if not timestamped_regional_links or timestamped_regional_links[0].get("id") != (
        "transport-and-conformance"
    ) or timestamped_regional_links[0].get("match") != "lane_owner":
        raise AssertionError(
            "timestamped regional process trace did not prioritize its owning roadmap family"
        )
    multi_recipient_regional = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-multi-recipient-regional-interaction-integration"
        ),
        None,
    )
    if not isinstance(multi_recipient_regional, dict):
        raise AssertionError("focused process multi-recipient regional row is absent")
    if query_rti_work.source_location_text(multi_recipient_regional) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:14552"
    ):
        raise AssertionError("focused process multi-recipient regional source pointer drifted")
    if multi_recipient_regional.get("assertions") != 126:
        raise AssertionError("focused process multi-recipient regional assertion count drifted")
    if multi_recipient_regional.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process multi-recipient regional row is not mapped")
    multi_recipient_regional_focus = query_rti_work.focused_lane_result(
        index, tests, "process-multi-recipient-regional-interaction", limit=0
    )
    if multi_recipient_regional_focus.get("lane_state") != "complete":
        raise AssertionError("focused process multi-recipient regional lane is not complete")
    if multi_recipient_regional_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused process multi-recipient regional mapped count drifted")
    if multi_recipient_regional_focus.get("assertion_count") != 126:
        raise AssertionError("focused process multi-recipient regional lane assertion total drifted")
    if multi_recipient_regional_focus.get("requirement_count") != 13:
        raise AssertionError("focused process multi-recipient regional requirement total drifted")
    if multi_recipient_regional_focus.get("standard_section_count") != 7:
        raise AssertionError("focused process multi-recipient regional section total drifted")
    if multi_recipient_regional_focus.get("requirement_section_pair_count") != 13:
        raise AssertionError("focused process multi-recipient regional direct pair drifted")
    multi_recipient_regional_handles = multi_recipient_regional_focus.get("lane_handles")
    if not isinstance(multi_recipient_regional_handles, dict) or multi_recipient_regional_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused process multi-recipient regional target handle drifted")
    regional_trace_links = query_rti_work.roadmap_links_for_test(
        index, multi_recipient_regional, limit=0
    )
    if not regional_trace_links or regional_trace_links[0].get("id") != (
        "transport-and-conformance"
    ) or regional_trace_links[0].get("match") != "lane_owner":
        raise AssertionError(
            "regional process trace did not prioritize its owning roadmap family"
        )
    multi_recipient_ordering = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-multi-recipient-interaction-ordering-integration"
        ),
        None,
    )
    if not isinstance(multi_recipient_ordering, dict):
        raise AssertionError("focused process multi-recipient ordering row is absent")
    if query_rti_work.source_location_text(multi_recipient_ordering) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:14106"
    ):
        raise AssertionError("focused process multi-recipient ordering source pointer drifted")
    if multi_recipient_ordering.get("assertions") != 79:
        raise AssertionError("focused process multi-recipient ordering assertion count drifted")
    if multi_recipient_ordering.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process multi-recipient ordering row is not mapped")
    multi_recipient_ordering_focus = query_rti_work.focused_lane_result(
        index, tests, "process-multi-recipient-callback-ordering", limit=0
    )
    if multi_recipient_ordering_focus.get("lane_state") != "complete":
        raise AssertionError("focused process multi-recipient ordering lane is not complete")
    if multi_recipient_ordering_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused process multi-recipient ordering mapped count drifted")
    if multi_recipient_ordering_focus.get("assertion_count") != 79:
        raise AssertionError("focused process multi-recipient ordering lane assertion total drifted")
    if multi_recipient_ordering_focus.get("requirement_count") != 10:
        raise AssertionError("focused process multi-recipient ordering requirement total drifted")
    if multi_recipient_ordering_focus.get("standard_section_count") != 6:
        raise AssertionError("focused process multi-recipient ordering section total drifted")
    if multi_recipient_ordering_focus.get("requirement_section_pair_count") != 10:
        raise AssertionError("focused process multi-recipient ordering direct pair drifted")
    multi_recipient_ordering_handles = multi_recipient_ordering_focus.get("lane_handles")
    if not isinstance(multi_recipient_ordering_handles, dict) or multi_recipient_ordering_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused process multi-recipient ordering target handle drifted")
    available_dimensions_hierarchy = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-available-dimensions-hierarchy-integration"
        ),
        None,
    )
    if not isinstance(available_dimensions_hierarchy, dict):
        raise AssertionError("focused process available-dimensions hierarchy row is absent")
    if query_rti_work.source_location_text(available_dimensions_hierarchy) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:13933"
    ):
        raise AssertionError(
            "focused process available-dimensions hierarchy source pointer drifted"
        )
    if available_dimensions_hierarchy.get("assertions") != 28:
        raise AssertionError(
            "focused process available-dimensions hierarchy assertion count drifted"
        )
    if available_dimensions_hierarchy.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "focused process available-dimensions hierarchy row is not mapped"
        )
    available_dimensions_hierarchy_focus = query_rti_work.focused_lane_result(
        index, tests, "process-available-dimensions-hierarchy", limit=0
    )
    if available_dimensions_hierarchy_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused process available-dimensions hierarchy lane is not complete"
        )
    if available_dimensions_hierarchy_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused process available-dimensions hierarchy mapped count drifted"
        )
    if available_dimensions_hierarchy_focus.get("assertion_count") != 28:
        raise AssertionError(
            "focused process available-dimensions hierarchy lane assertion total drifted"
        )
    if available_dimensions_hierarchy_focus.get("requirement_count") != 4:
        raise AssertionError(
            "focused process available-dimensions hierarchy requirement total drifted"
        )
    if available_dimensions_hierarchy_focus.get("standard_section_count") != 1:
        raise AssertionError(
            "focused process available-dimensions hierarchy section total drifted"
        )
    if available_dimensions_hierarchy_focus.get("requirement_section_pair_count") != 4:
        raise AssertionError(
            "focused process available-dimensions hierarchy direct pair drifted"
        )
    reverse_fom_lookup = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-fom-name-lookup-focused-integration"
        ),
        None,
    )
    if not isinstance(reverse_fom_lookup, dict):
        raise AssertionError("focused process reverse-FOM lookup row is absent")
    if query_rti_work.source_location_text(reverse_fom_lookup) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:13224"
    ):
        raise AssertionError("focused process reverse-FOM lookup source pointer drifted")
    if reverse_fom_lookup.get("assertions") != 20:
        raise AssertionError("focused process reverse-FOM lookup assertion count drifted")
    if reverse_fom_lookup.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process reverse-FOM lookup row is not mapped")
    reverse_attribute_parameter_lookup = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-attribute-parameter-name-lookup-focused-integration"
        ),
        None,
    )
    if not isinstance(reverse_attribute_parameter_lookup, dict):
        raise AssertionError("focused process attribute/parameter reverse lookup row is absent")
    if query_rti_work.source_location_text(reverse_attribute_parameter_lookup) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:13411"
    ):
        raise AssertionError(
            "focused process attribute/parameter reverse lookup source pointer drifted"
        )
    if reverse_attribute_parameter_lookup.get("assertions") != 24:
        raise AssertionError(
            "focused process attribute/parameter reverse lookup assertion count drifted"
        )
    if reverse_attribute_parameter_lookup.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "focused process attribute/parameter reverse lookup row is not mapped"
        )
    if set(reverse_attribute_parameter_lookup.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-10.9.1",
        "hla-1516.1-2025:clause-10.10.3",
        "hla-1516.1-2025:clause-10.16.1",
    }:
        raise AssertionError(
            "focused process attribute/parameter reverse lookup sections drifted"
        )
    reverse_dimension_transportation_lookup = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-dimension-transportation-name-lookup-focused-integration"
        ),
        None,
    )
    if not isinstance(reverse_dimension_transportation_lookup, dict):
        raise AssertionError(
            "focused process dimension/transportation reverse lookup row is absent"
        )
    if query_rti_work.source_location_text(reverse_dimension_transportation_lookup) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:13628"
    ):
        raise AssertionError(
            "focused process dimension/transportation reverse lookup source pointer drifted"
        )
    if reverse_dimension_transportation_lookup.get("assertions") != 16:
        raise AssertionError(
            "focused process dimension/transportation reverse lookup assertion count drifted"
        )
    if reverse_dimension_transportation_lookup.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "focused process dimension/transportation reverse lookup row is not mapped"
        )
    if set(reverse_dimension_transportation_lookup.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-9.1.2",
        "hla-1516.1-2025:clause-10.19",
        "hla-1516.1-2025:clause-10.20.4",
    }:
        raise AssertionError(
            "focused process dimension/transportation reverse lookup sections drifted"
        )
    reverse_fom_error_matrix = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-reverse-fom-error-matrix-integration"
        ),
        None,
    )
    if not isinstance(reverse_fom_error_matrix, dict):
        raise AssertionError("focused process reverse-FOM error matrix row is absent")
    if query_rti_work.source_location_text(reverse_fom_error_matrix) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:13785"
    ):
        raise AssertionError("focused process reverse-FOM error matrix source pointer drifted")
    if reverse_fom_error_matrix.get("assertions") != 16:
        raise AssertionError("focused process reverse-FOM error matrix assertion count drifted")
    if reverse_fom_error_matrix.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process reverse-FOM error matrix row is not mapped")
    if set(reverse_fom_error_matrix.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-9.1.2",
        "hla-1516.1-2025:clause-10.19",
        "hla-1516.1-2025:clause-10.20.4",
    }:
        raise AssertionError("focused process reverse-FOM error matrix sections drifted")
    reverse_fom_lookup_focus = query_rti_work.focused_lane_result(
        index, tests, "reverse-fom-lookup", limit=0
    )
    if reverse_fom_lookup_focus.get("lane_state") != "complete":
        raise AssertionError("focused process reverse-FOM lookup lane is not complete")
    if reverse_fom_lookup_focus.get("mapped_test_count") != 4:
        raise AssertionError("focused process reverse-FOM lookup mapped count drifted")
    if reverse_fom_lookup_focus.get("assertion_count") != 76:
        raise AssertionError("focused process reverse-FOM lookup lane assertion total drifted")
    if reverse_fom_lookup_focus.get("requirement_count") != 14:
        raise AssertionError("focused process reverse-FOM lookup requirement total drifted")
    if reverse_fom_lookup_focus.get("standard_section_count") != 10:
        raise AssertionError("focused process reverse-FOM lookup section total drifted")
    if reverse_fom_lookup_focus.get("requirement_section_pair_count") != 14:
        raise AssertionError("focused process reverse-FOM lookup direct pair drifted")
    reverse_fom_lookup_handles = reverse_fom_lookup_focus.get("lane_handles")
    if not isinstance(reverse_fom_lookup_handles, dict) or reverse_fom_lookup_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused process reverse-FOM lookup target handle drifted")
    shorthand_reverse_fom_kind, shorthand_reverse_fom_matches = (
        query_rti_work.resolve_matrix_query(index, tests, "getObjectClassName")
    )
    if shorthand_reverse_fom_kind != "api" or not any(
        test.get("id") == reverse_fom_lookup.get("id")
        for test in shorthand_reverse_fom_matches
        if isinstance(test, dict)
    ):
        raise AssertionError(
            "matrix API method shorthand did not resolve the focused reverse-FOM lookup row"
        )
    object_instance_lookup = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-object-instance-lookup-focused-integration"
        ),
        None,
    )
    if not isinstance(object_instance_lookup, dict):
        raise AssertionError("focused process object-instance lookup row is absent")
    if query_rti_work.source_location_text(object_instance_lookup) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:13040"
    ):
        raise AssertionError("focused process object-instance lookup source pointer drifted")
    if object_instance_lookup.get("assertions") != 18:
        raise AssertionError("focused process object-instance lookup assertion count drifted")
    if object_instance_lookup.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process object-instance lookup row is not mapped")
    object_instance_lookup_focus = query_rti_work.focused_lane_result(
        index, tests, "object-instance-lookup", limit=0
    )
    if object_instance_lookup_focus.get("lane_state") != "complete":
        raise AssertionError("focused process object-instance lookup lane is not complete")
    if object_instance_lookup_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused process object-instance lookup mapped count drifted")
    if object_instance_lookup_focus.get("assertion_count") != 18:
        raise AssertionError("focused process object-instance lookup lane assertion total drifted")
    if object_instance_lookup_focus.get("requirement_count") != 5:
        raise AssertionError("focused process object-instance lookup requirement total drifted")
    if object_instance_lookup_focus.get("standard_section_count") != 2:
        raise AssertionError("focused process object-instance lookup section total drifted")
    object_instance_lookup_handles = object_instance_lookup_focus.get("lane_handles")
    if not isinstance(object_instance_lookup_handles, dict) or object_instance_lookup_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused process object-instance lookup target handle drifted")
    shorthand_object_instance_kind, shorthand_object_instance_matches = (
        query_rti_work.resolve_matrix_query(index, tests, "getObjectInstanceHandle")
    )
    if shorthand_object_instance_kind != "api" or not any(
        test.get("id") == object_instance_lookup.get("id")
        for test in shorthand_object_instance_matches
        if isinstance(test, dict)
    ):
        raise AssertionError(
            "matrix API method shorthand did not resolve the focused object-instance lookup row"
        )
    process_next_message = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-next-message-advance-focused-integration"
        ),
        None,
    )
    if not isinstance(process_next_message, dict):
        raise AssertionError("focused process NMR/NMRA row is absent")
    if query_rti_work.source_location_text(process_next_message) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:12492"
    ):
        raise AssertionError("focused process NMR/NMRA source pointer drifted")
    if process_next_message.get("assertions") != 22:
        raise AssertionError("focused process NMR/NMRA assertion count drifted")
    if process_next_message.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process NMR/NMRA row is not mapped")
    process_next_message_focus = query_rti_work.focused_lane_result(
        index, tests, "process-time-advance-next-message-focused", limit=0
    )
    if process_next_message_focus.get("lane_state") != "complete":
        raise AssertionError("focused process NMR/NMRA lane is not complete")
    if process_next_message_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused process NMR/NMRA mapped count drifted")
    if process_next_message_focus.get("assertion_count") != 22:
        raise AssertionError("focused process NMR/NMRA lane assertion total drifted")
    if process_next_message_focus.get("requirement_count") != 4:
        raise AssertionError("focused process NMR/NMRA requirement total drifted")
    if process_next_message_focus.get("standard_section_count") != 3:
        raise AssertionError("focused process NMR/NMRA section total drifted")
    process_next_message_handles = process_next_message_focus.get("lane_handles")
    if not isinstance(process_next_message_handles, dict) or process_next_message_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused process NMR/NMRA target handle drifted")
    shorthand_api_kind, shorthand_api_matches = query_rti_work.resolve_matrix_query(
        index, tests, "next_message_request"
    )
    if shorthand_api_kind != "api" or not any(
        test.get("id") == process_next_message.get("id")
        for test in shorthand_api_matches
        if isinstance(test, dict)
    ):
        raise AssertionError(
            "matrix API method shorthand did not resolve the focused process NMR/NMRA row"
        )
    process_next_message_queued = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-next-message-queued-tso-focused-integration"
        ),
        None,
    )
    if not isinstance(process_next_message_queued, dict):
        raise AssertionError("focused process queued-TSO NMR/NMRA row is absent")
    if query_rti_work.source_location_text(process_next_message_queued) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:12600"
    ):
        raise AssertionError("focused process queued-TSO source pointer drifted")
    if process_next_message_queued.get("assertions") != 101:
        raise AssertionError("focused process queued-TSO assertion count drifted")
    if process_next_message_queued.get("callback_models") != [
        "HLA_EVOKED",
        "HLA_IMMEDIATE",
    ]:
        raise AssertionError("focused process queued-TSO callback-model mapping drifted")
    if process_next_message_queued.get("primary_lane") != (
        "process-time-advance-next-message-queued-focused"
    ):
        raise AssertionError("focused process queued-TSO primary lane drifted")
    if process_next_message_queued.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process queued-TSO row is not mapped")
    process_next_message_queued_focus = query_rti_work.focused_lane_result(
        index, tests, "process-time-advance-next-message-queued-focused", limit=0
    )
    if process_next_message_queued_focus.get("lane_state") != "complete":
        raise AssertionError("focused process queued-TSO lane is not complete")
    if process_next_message_queued_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused process queued-TSO mapped count drifted")
    if process_next_message_queued_focus.get("assertion_count") != 101:
        raise AssertionError("focused process queued-TSO lane assertion total drifted")
    if process_next_message_queued_focus.get("requirement_count") != 19:
        raise AssertionError("focused process queued-TSO requirement total drifted")
    if process_next_message_queued_focus.get("standard_section_count") != 12:
        raise AssertionError("focused process queued-TSO section total drifted")
    process_next_message_queued_handles = process_next_message_queued_focus.get(
        "lane_handles"
    )
    if not isinstance(process_next_message_queued_handles, dict) or process_next_message_queued_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused process queued-TSO target handle drifted")
    process_time_advance_available = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-time-advance-available-focused-integration"
        ),
        None,
    )
    if not isinstance(process_time_advance_available, dict):
        raise AssertionError("focused process Available-form row is absent")
    if query_rti_work.source_location_text(process_time_advance_available) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:12396"
    ):
        raise AssertionError("focused process Available-form source pointer drifted")
    if process_time_advance_available.get("assertions") != 15:
        raise AssertionError("focused process Available-form assertion count drifted")
    if process_time_advance_available.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process Available-form row is not mapped")
    process_time_advance_available_focus = query_rti_work.focused_lane_result(
        index, tests, "process-time-advance-available-focused", limit=0
    )
    if process_time_advance_available_focus.get("lane_state") != "complete":
        raise AssertionError("focused process Available-form lane is not complete")
    if process_time_advance_available_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused process Available-form mapped count drifted")
    if process_time_advance_available_focus.get("assertion_count") != 15:
        raise AssertionError("focused process Available-form lane assertion total drifted")
    if process_time_advance_available_focus.get("requirement_count") != 4:
        raise AssertionError("focused process Available-form requirement total drifted")
    if process_time_advance_available_focus.get("standard_section_count") != 3:
        raise AssertionError("focused process Available-form section total drifted")
    process_time_advance_available_handles = process_time_advance_available_focus.get(
        "lane_handles"
    )
    if not isinstance(process_time_advance_available_handles, dict) or process_time_advance_available_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused process Available-form target handle drifted")
    process_modify_lookahead_grant = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-modify-lookahead-grant-focused-integration"
        ),
        None,
    )
    if not isinstance(process_modify_lookahead_grant, dict):
        raise AssertionError("focused process grant-time lookahead row is absent")
    if query_rti_work.source_location_text(process_modify_lookahead_grant) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:12164"
    ):
        raise AssertionError("focused process grant-time lookahead source pointer drifted")
    if process_modify_lookahead_grant.get("assertions") != 34:
        raise AssertionError("focused process grant-time lookahead assertion count drifted")
    if process_modify_lookahead_grant.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process grant-time lookahead row is not mapped")
    process_modify_lookahead_grant_focus = query_rti_work.focused_lane_result(
        index, tests, "process-modify-lookahead-grant-focused", limit=0
    )
    if process_modify_lookahead_grant_focus.get("lane_state") != "complete":
        raise AssertionError("focused process grant-time lookahead lane is not complete")
    if process_modify_lookahead_grant_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused process grant-time lookahead mapped count drifted")
    if process_modify_lookahead_grant_focus.get("assertion_count") != 34:
        raise AssertionError("focused process grant-time lookahead lane assertion total drifted")
    if process_modify_lookahead_grant_focus.get("requirement_count") != 6:
        raise AssertionError("focused process grant-time lookahead requirement total drifted")
    if process_modify_lookahead_grant_focus.get("standard_section_count") != 4:
        raise AssertionError("focused process grant-time lookahead section total drifted")
    process_modify_lookahead_grant_handles = process_modify_lookahead_grant_focus.get(
        "lane_handles"
    )
    if not isinstance(process_modify_lookahead_grant_handles, dict) or process_modify_lookahead_grant_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused process grant-time lookahead target handle drifted")
    process_modify_lookahead = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-modify-lookahead-focused-integration"
        ),
        None,
    )
    if not isinstance(process_modify_lookahead, dict):
        raise AssertionError("focused process modify-lookahead row is absent")
    if query_rti_work.source_location_text(process_modify_lookahead) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:12058"
    ):
        raise AssertionError("focused process modify-lookahead source pointer drifted")
    if process_modify_lookahead.get("assertions") != 17:
        raise AssertionError("focused process modify-lookahead assertion count drifted")
    if process_modify_lookahead.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process modify-lookahead row is not mapped")
    if "process-time-role" in process_modify_lookahead.get("tags", []):
        raise AssertionError(
            "focused process modify-lookahead row contaminated the time-role lane"
        )
    process_modify_lookahead_focus = query_rti_work.focused_lane_result(
        index, tests, "process-modify-lookahead-focused", limit=0
    )
    if process_modify_lookahead_focus.get("lane_state") != "complete":
        raise AssertionError("focused process modify-lookahead lane is not complete")
    if process_modify_lookahead_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused process modify-lookahead mapped count drifted")
    if process_modify_lookahead_focus.get("assertion_count") != 17:
        raise AssertionError("focused process modify-lookahead lane assertion total drifted")
    if process_modify_lookahead_focus.get("requirement_count") != 3:
        raise AssertionError("focused process modify-lookahead requirement total drifted")
    if process_modify_lookahead_focus.get("standard_section_count") != 1:
        raise AssertionError("focused process modify-lookahead section total drifted")
    process_modify_lookahead_handles = process_modify_lookahead_focus.get("lane_handles")
    if not isinstance(process_modify_lookahead_handles, dict) or process_modify_lookahead_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused process modify-lookahead target handle drifted")
    process_query_lookahead = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-query-lookahead-focused-integration"
        ),
        None,
    )
    if not isinstance(process_query_lookahead, dict):
        raise AssertionError("focused process query-lookahead row is absent")
    if query_rti_work.source_location_text(process_query_lookahead) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:11962"
    ):
        raise AssertionError("focused process query-lookahead source pointer drifted")
    if process_query_lookahead.get("assertions") != 13:
        raise AssertionError("focused process query-lookahead assertion count drifted")
    if process_query_lookahead.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process query-lookahead row is not mapped")
    process_query_lookahead_focus = query_rti_work.focused_lane_result(
        index, tests, "process-query-lookahead-focused", limit=0
    )
    if process_query_lookahead_focus.get("lane_state") != "complete":
        raise AssertionError("focused process query-lookahead lane is not complete")
    if process_query_lookahead_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused process query-lookahead mapped count drifted")
    if process_query_lookahead_focus.get("assertion_count") != 13:
        raise AssertionError("focused process query-lookahead lane assertion total drifted")
    if process_query_lookahead_focus.get("requirement_count") != 2:
        raise AssertionError("focused process query-lookahead requirement total drifted")
    if process_query_lookahead_focus.get("standard_section_count") != 2:
        raise AssertionError("focused process query-lookahead section total drifted")
    process_query_lookahead_handles = process_query_lookahead_focus.get("lane_handles")
    if not isinstance(process_query_lookahead_handles, dict) or process_query_lookahead_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("focused process query-lookahead target handle drifted")
    public_process_restart = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-public-process-restart-application-value-focused-integration"
        ),
        None,
    )
    if not isinstance(public_process_restart, dict):
        raise AssertionError("focused public process-restart application-value row is absent")
    if query_rti_work.source_location_text(public_process_restart) != (
        "cpp/tests/public_process_restart_application_value_catch2.cpp:189"
    ):
        raise AssertionError("focused public process-restart application-value source pointer drifted")
    if public_process_restart.get("assertions") != 82:
        raise AssertionError("focused public process-restart application-value assertion count drifted")
    if public_process_restart.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused public process-restart application-value row is not mapped")
    public_process_restart_focus = query_rti_work.focused_lane_result(
        index, tests, "public-process-restart-application-value-focused", limit=0
    )
    if public_process_restart_focus.get("lane_state") != "complete":
        raise AssertionError("focused public process-restart application-value lane is not complete")
    if public_process_restart_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused public process-restart application-value mapped count drifted")
    if public_process_restart_focus.get("assertion_count") != 82:
        raise AssertionError("focused public process-restart application-value lane assertion total drifted")
    if public_process_restart_focus.get("requirement_count") != 9:
        raise AssertionError("focused public process-restart application-value requirement total drifted")
    if public_process_restart_focus.get("standard_section_count") != 5:
        raise AssertionError("focused public process-restart application-value section total drifted")
    public_process_restart_handles = public_process_restart_focus.get("lane_handles")
    if not isinstance(public_process_restart_handles, dict) or public_process_restart_handles.get(
        "catch2_target"
    ) != "umbra_public_process_restart_application_value_catch2":
        raise AssertionError("focused public process-restart application-value target handle drifted")
    register_sync = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-service-report-file-register-federation-synchronization-point-integration"
        ),
        None,
    )
    if not isinstance(register_sync, dict):
        raise AssertionError("focused synchronization-point report row is absent")
    if query_rti_work.source_location_text(register_sync) != (
        "cpp/tests/service_report_file_confirm_synchronization_point_registration_catch2.cpp:220"
    ):
        raise AssertionError("focused synchronization-point report source pointer drifted")
    if register_sync.get("assertions") != 18:
        raise AssertionError("focused synchronization-point report assertion count drifted")
    if register_sync.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused synchronization-point report row is not mapped")
    register_sync_focus = query_rti_work.focused_lane_result(
        index, tests, "register-federation-synchronization-point-service-report", limit=0
    )
    if register_sync_focus.get("lane_state") != "complete":
        raise AssertionError("focused synchronization-point report lane is not complete")
    if register_sync_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused synchronization-point report mapped count drifted")
    if register_sync_focus.get("assertion_count") != 18:
        raise AssertionError("focused synchronization-point report lane assertion total drifted")
    save_history = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-federation-save-commit-filesystem-process-restart-save-history-unit"
        ),
        None,
    )
    if not isinstance(save_history, dict):
        raise AssertionError("focused federation save-history row is absent")
    if query_rti_work.source_location_text(save_history) != (
        "cpp/tests/federation_registry_catch2.cpp:6667"
    ):
        raise AssertionError("focused federation save-history source pointer drifted")
    if save_history.get("assertions") != 42:
        raise AssertionError("focused federation save-history assertion count drifted")
    if save_history.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused federation save-history row is not mapped")
    if save_history.get("requirements_lab_mapping_id") != (
        "m93.federation-save-history-process-restart"
    ):
        raise AssertionError("focused federation save-history mapping handle drifted")
    if len(save_history.get("lab_requirement_ids", [])) != 10:
        raise AssertionError("focused federation save-history requirement mapping drifted")
    if len(save_history.get("standard_sections", [])) != 7:
        raise AssertionError("focused federation save-history section mapping drifted")
    if len(save_history.get("selected_cpp_api_surface_ids", [])) != 5:
        raise AssertionError("focused federation save-history API mapping drifted")
    save_history_focus = query_rti_work.focused_lane_result(
        index, tests, "federation-save-history-process-restart", limit=0
    )
    if save_history_focus.get("lane_state") != "complete":
        raise AssertionError("focused federation save-history lane is not complete")
    if save_history_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused federation save-history mapped count drifted")
    if save_history_focus.get("assertion_count") != 42:
        raise AssertionError("focused federation save-history lane assertion total drifted")
    if save_history_focus.get("standard_section_count") != 7:
        raise AssertionError("focused federation save-history lane section total drifted")
    save_history_handles = save_history_focus.get("lane_handles")
    if not isinstance(save_history_handles, dict) or save_history_handles.get(
        "catch2_target"
    ) != "umbra_federation_registry_catch2":
        raise AssertionError("focused federation save-history target handle drifted")
    subscription_generation = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-restore-subscription-generation-allocation-unit"
        ),
        None,
    )
    if not isinstance(subscription_generation, dict):
        raise AssertionError("focused subscription-generation row is absent")
    if query_rti_work.source_location_text(subscription_generation) != (
        "cpp/tests/libxml2_fom_composer_catch2.cpp:1742"
    ):
        raise AssertionError("focused subscription-generation source pointer drifted")
    if subscription_generation.get("assertions") != 31:
        raise AssertionError("focused subscription-generation assertion count drifted")
    if subscription_generation.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused subscription-generation row is not mapped")
    if subscription_generation.get("requirements_lab_mapping_id") != (
        "m92.federation-registry-subscription-generation-restore"
    ):
        raise AssertionError("focused subscription-generation mapping handle drifted")
    if len(subscription_generation.get("lab_requirement_ids", [])) != 1:
        raise AssertionError("focused subscription-generation requirement mapping drifted")
    if len(subscription_generation.get("standard_sections", [])) != 1:
        raise AssertionError("focused subscription-generation section mapping drifted")
    if len(subscription_generation.get("selected_cpp_api_surface_ids", [])) != 2:
        raise AssertionError("focused subscription-generation API mapping drifted")
    subscription_generation_focus = query_rti_work.focused_lane_result(
        index, tests, "subscription-generation", limit=0
    )
    if subscription_generation_focus.get("lane_state") != "complete":
        raise AssertionError("focused subscription-generation lane is not complete")
    if subscription_generation_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused subscription-generation mapped count drifted")
    if subscription_generation_focus.get("assertion_count") != 31:
        raise AssertionError("focused subscription-generation lane assertion total drifted")
    if subscription_generation_focus.get("standard_section_count") != 1:
        raise AssertionError("focused subscription-generation lane section total drifted")
    subscription_generation_handles = subscription_generation_focus.get("lane_handles")
    if not isinstance(subscription_generation_handles, dict) or subscription_generation_handles.get(
        "catch2_target"
    ) != "umbra_fom_composer_catch2":
        raise AssertionError("focused subscription-generation target handle drifted")
    writer_failure = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-service-report-writer-failure-integration"
        ),
        None,
    )
    if not isinstance(writer_failure, dict):
        raise AssertionError("focused service-report writer-failure row is absent")
    if query_rti_work.source_location_text(writer_failure) != (
        "cpp/tests/service_report_writer_failure_catch2.cpp:92"
    ):
        raise AssertionError("focused service-report writer-failure source pointer drifted")
    if writer_failure.get("assertions") != 10:
        raise AssertionError("focused service-report writer-failure assertion count drifted")
    if writer_failure.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused service-report writer-failure row is not mapped")
    if writer_failure.get("requirements_lab_mapping_id") != (
        "m91.embedded-service-report-writer-creation-failure"
    ):
        raise AssertionError("focused service-report writer-failure mapping handle drifted")
    if len(writer_failure.get("lab_requirement_ids", [])) != 1:
        raise AssertionError("focused service-report writer-failure requirement mapping drifted")
    if len(writer_failure.get("standard_sections", [])) != 1:
        raise AssertionError("focused service-report writer-failure section mapping drifted")
    if len(writer_failure.get("selected_cpp_api_surface_ids", [])) != 6:
        raise AssertionError("focused service-report writer-failure API mapping drifted")
    writer_failure_focus = query_rti_work.focused_lane_result(
        index, tests, "service-report-writer-failure", limit=0
    )
    if writer_failure_focus.get("lane_state") != "complete":
        raise AssertionError("focused service-report writer-failure lane is not complete")
    if writer_failure_focus.get("mapped_test_count") != 2:
        raise AssertionError("focused service-report writer-failure mapped count drifted")
    if writer_failure_focus.get("assertion_count") != 27:
        raise AssertionError("focused service-report writer-failure lane assertion total drifted")
    if writer_failure_focus.get("standard_section_count") != 2:
        raise AssertionError("focused service-report writer-failure lane section total drifted")
    writer_failure_handles = writer_failure_focus.get("lane_handles")
    if not isinstance(writer_failure_handles, dict) or writer_failure_handles.get(
        "catch2_target"
    ) != "umbra_service_report_writer_failure_catch2":
        raise AssertionError("focused service-report writer-failure target handle drifted")
    mom_federate_set_switches = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-mom-federate-set-switches-integration"
        ),
        None,
    )
    if not isinstance(mom_federate_set_switches, dict):
        raise AssertionError("focused joined-federate HLAsetSwitches row is absent")
    if query_rti_work.source_location_text(mom_federate_set_switches) != (
        "cpp/tests/mom_federate_set_switches_catch2.cpp:67"
    ):
        raise AssertionError("focused joined-federate HLAsetSwitches source pointer drifted")
    if mom_federate_set_switches.get("assertions") != 45:
        raise AssertionError("focused joined-federate HLAsetSwitches assertion count drifted")
    if mom_federate_set_switches.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate HLAsetSwitches row is not mapped")
    if len(mom_federate_set_switches.get("lab_requirement_ids", [])) != 5:
        raise AssertionError("focused joined-federate HLAsetSwitches requirement mapping drifted")
    if len(mom_federate_set_switches.get("standard_sections", [])) != 3:
        raise AssertionError("focused joined-federate HLAsetSwitches section mapping drifted")
    if len(mom_federate_set_switches.get("selected_cpp_api_surface_ids", [])) != 12:
        raise AssertionError("focused joined-federate HLAsetSwitches API mapping drifted")
    mom_federate_set_switches_focus = query_rti_work.focused_lane_result(
        index, tests, "mom-federate-set-switches", limit=0
    )
    if mom_federate_set_switches_focus.get("lane_state") != "complete":
        raise AssertionError("focused joined-federate HLAsetSwitches lane is not complete")
    if mom_federate_set_switches_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused joined-federate HLAsetSwitches mapped count drifted")
    if mom_federate_set_switches_focus.get("assertion_count") != 45:
        raise AssertionError("focused joined-federate HLAsetSwitches lane assertion total drifted")
    if mom_federate_set_switches_focus.get("requirement_count") != 5:
        raise AssertionError("focused joined-federate HLAsetSwitches lane requirement total drifted")
    if mom_federate_set_switches_focus.get("standard_section_count") != 3:
        raise AssertionError("focused joined-federate HLAsetSwitches lane section total drifted")
    mom_federate_set_switches_handles = mom_federate_set_switches_focus.get("lane_handles")
    if not isinstance(mom_federate_set_switches_handles, dict) or mom_federate_set_switches_handles.get(
        "catch2_target"
    ) != "umbra_mom_federate_set_switches_catch2":
        raise AssertionError("focused joined-federate HLAsetSwitches target handle drifted")
    focused_attribute_transportation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-focused-attribute-transportation-type-control-integration"
        ),
        None,
    )
    if not isinstance(focused_attribute_transportation, dict):
        raise AssertionError("focused attribute transportation row is absent")
    if query_rti_work.source_location_text(focused_attribute_transportation) != (
        "cpp/tests/custom_transportation_type_control_catch2.cpp:102"
    ):
        raise AssertionError("focused attribute transportation source pointer drifted")
    if focused_attribute_transportation.get("assertions") != 43:
        raise AssertionError("focused attribute transportation assertion count drifted")
    if focused_attribute_transportation.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused attribute transportation row is not mapped")
    if len(focused_attribute_transportation.get("lab_requirement_ids", [])) != 8:
        raise AssertionError("focused attribute transportation requirement mapping drifted")
    if len(focused_attribute_transportation.get("standard_sections", [])) != 4:
        raise AssertionError("focused attribute transportation section mapping drifted")
    if len(focused_attribute_transportation.get("selected_cpp_api_surface_ids", [])) != 12:
        raise AssertionError("focused attribute transportation API mapping drifted")
    focused_attribute_transportation_lane = query_rti_work.focused_lane_result(
        index, tests, "custom-transportation-type-control", limit=0
    )
    if focused_attribute_transportation_lane.get("lane_state") != "complete":
        raise AssertionError("focused attribute transportation lane is not complete")
    if focused_attribute_transportation_lane.get("mapped_test_count") != 1:
        raise AssertionError("focused attribute transportation mapped count drifted")
    if focused_attribute_transportation_lane.get("assertion_count") != 43:
        raise AssertionError("focused attribute transportation lane assertion total drifted")
    if focused_attribute_transportation_lane.get("requirement_count") != 8:
        raise AssertionError("focused attribute transportation lane requirement total drifted")
    if focused_attribute_transportation_lane.get("standard_section_count") != 4:
        raise AssertionError("focused attribute transportation lane section total drifted")
    focused_attribute_transportation_handles = focused_attribute_transportation_lane.get(
        "lane_handles"
    )
    if not isinstance(focused_attribute_transportation_handles, dict) or focused_attribute_transportation_handles.get(
        "catch2_target"
    ) != "umbra_custom_transportation_type_control_catch2":
        raise AssertionError("focused attribute transportation target handle drifted")
    focused_timestamped_slices = [
        {
            "id": "umbra-cpp-delay-subscription-evaluation-timestamped-directed-interaction-integration",
            "mapping": "m59.embedded-delay-subscription-evaluation-timestamped-directed-interaction",
            "source": "cpp/tests/delay_subscription_evaluation_timestamped_directed_interaction_catch2.cpp:129",
            "assertions": 166,
            "lane": "delay-subscription-evaluation-timestamped-directed-interaction",
            "requirements": 3,
            "sections": 1,
        },
        {
            "id": "umbra-cpp-timestamped-directed-interaction-tso-retraction-integration",
            "mapping": "m60.embedded-timestamped-directed-interaction-tso-retraction",
            "source": "cpp/tests/timestamped_directed_interaction_retraction_catch2.cpp:150",
            "assertions": 56,
            "lane": "timestamped-directed-interaction-tso-retraction",
            "requirements": 15,
            "sections": 13,
        },
    ]
    for expectation in focused_timestamped_slices:
        focused_slice = next(
            (test for test in tests if test.get("id") == expectation["id"]),
            None,
        )
        if not isinstance(focused_slice, dict):
            raise AssertionError(f"focused {expectation['mapping']} row is absent")
        if query_rti_work.source_location_text(focused_slice) != expectation["source"]:
            raise AssertionError(f"focused {expectation['mapping']} source pointer drifted")
        if focused_slice.get("assertions") != expectation["assertions"]:
            raise AssertionError(f"focused {expectation['mapping']} assertion count drifted")
        if focused_slice.get("requirements_lab_mapping_id") != expectation["mapping"]:
            raise AssertionError(f"focused {expectation['mapping']} mapping handle drifted")
        if len(focused_slice.get("lab_requirement_ids", [])) != expectation["requirements"]:
            raise AssertionError(f"focused {expectation['mapping']} requirement mapping drifted")
        if len(focused_slice.get("standard_sections", [])) != expectation["sections"]:
            raise AssertionError(f"focused {expectation['mapping']} section mapping drifted")
        timestamped_focus = query_rti_work.focused_lane_result(
            index, tests, expectation["lane"], limit=0
        )
        if timestamped_focus.get("lane_state") != "complete":
            raise AssertionError(f"focused {expectation['lane']} lane is not complete")
        if timestamped_focus.get("mapped_test_count") != 1:
            raise AssertionError(f"focused {expectation['lane']} mapped count drifted")
        if timestamped_focus.get("assertion_count") != expectation["assertions"]:
            raise AssertionError(f"focused {expectation['lane']} assertion total drifted")
        if timestamped_focus.get("standard_section_count") != expectation["sections"]:
            raise AssertionError(f"focused {expectation['lane']} section total drifted")
    mom_auto_provide = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-auto-provide-mom-integration"
        ),
        None,
    )
    if not isinstance(mom_auto_provide, dict):
        raise AssertionError("focused MOM Auto Provide row is absent")
    if query_rti_work.source_location_text(mom_auto_provide) != (
        "cpp/tests/auto_provide_mom_catch2.cpp:120"
    ):
        raise AssertionError("focused MOM Auto Provide source pointer drifted")
    if mom_auto_provide.get("assertions") != 41:
        raise AssertionError("focused MOM Auto Provide assertion count drifted")
    if mom_auto_provide.get("traceability_state") != "explicit-disposition":
        raise AssertionError("focused MOM Auto Provide disposition drifted")
    if len(mom_auto_provide.get("lab_requirement_ids", [])) != 0:
        raise AssertionError("focused MOM Auto Provide Lab requirement mapping drifted")
    if len(mom_auto_provide.get("standard_sections", [])) != 3:
        raise AssertionError("focused MOM Auto Provide section mapping drifted")
    if len(mom_auto_provide.get("selected_cpp_api_surface_ids", [])) != 18:
        raise AssertionError("focused MOM Auto Provide API mapping drifted")
    mom_auto_provide_focus = query_rti_work.focused_lane_result(
        index, tests, "mom-auto-provide-switch-mutation", limit=0
    )
    if mom_auto_provide_focus.get("lane_state") != "complete":
        raise AssertionError("focused MOM Auto Provide lane is not complete")
    if mom_auto_provide_focus.get("mapped_test_count") != 0:
        raise AssertionError("focused MOM Auto Provide mapped count drifted")
    if mom_auto_provide_focus.get("assertion_count") != 41:
        raise AssertionError("focused MOM Auto Provide lane assertion total drifted")
    if mom_auto_provide_focus.get("standard_section_count") != 3:
        raise AssertionError("focused MOM Auto Provide lane section total drifted")
    mom_auto_provide_handles = mom_auto_provide_focus.get("lane_handles")
    if not isinstance(mom_auto_provide_handles, dict) or mom_auto_provide_handles.get(
        "catch2_target"
    ) != "umbra_auto_provide_mom_catch2":
        raise AssertionError("focused MOM Auto Provide target handle drifted")
    handle_normalization = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-handle-normalization-integration"
        ),
        None,
    )
    if not isinstance(handle_normalization, dict):
        raise AssertionError("focused handle-normalization row is absent")
    if query_rti_work.source_location_text(handle_normalization) != (
        "cpp/tests/handle_normalization_catch2.cpp:60"
    ):
        raise AssertionError("focused handle-normalization source pointer drifted")
    if handle_normalization.get("assertions") != 52:
        raise AssertionError("focused handle-normalization assertion count drifted")
    if handle_normalization.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused handle-normalization row is not mapped")
    if len(handle_normalization.get("lab_requirement_ids", [])) != 6:
        raise AssertionError("focused handle-normalization requirement mapping drifted")
    if len(handle_normalization.get("standard_sections", [])) != 6:
        raise AssertionError("focused handle-normalization section mapping drifted")
    if len(handle_normalization.get("selected_cpp_api_surface_ids", [])) != 5:
        raise AssertionError("focused handle-normalization API mapping drifted")
    handle_normalization_focus = query_rti_work.focused_lane_result(
        index, tests, "handle-normalization", limit=0
    )
    if handle_normalization_focus.get("lane_state") != "complete":
        raise AssertionError("focused handle-normalization lane is not complete")
    if handle_normalization_focus.get("mapped_test_count") != 2:
        raise AssertionError("focused handle-normalization mapped count drifted")
    if handle_normalization_focus.get("assertion_count") != 75:
        raise AssertionError("focused handle-normalization assertion total drifted")
    if handle_normalization_focus.get("requirement_count") != 6:
        raise AssertionError("focused handle-normalization requirement total drifted")
    if handle_normalization_focus.get("standard_section_count") != 6:
        raise AssertionError("focused handle-normalization section total drifted")
    if handle_normalization_focus.get("requirement_section_pair_count") != 6:
        raise AssertionError("focused handle-normalization direct pair drifted")
    handle_normalization_handles = handle_normalization_focus.get("lane_handles")
    if not isinstance(handle_normalization_handles, dict) or handle_normalization_handles.get(
        "catch2_target"
    ) != "umbra_handle_normalization_catch2":
        raise AssertionError("focused handle-normalization target handle drifted")
    object_instances_updated_report = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-object-instances-updated-report-integration"
        ),
        None,
    )
    if not isinstance(object_instances_updated_report, dict):
        raise AssertionError("focused joined-federate HLAreportObjectInstancesUpdated row is absent")
    if query_rti_work.source_location_text(object_instances_updated_report) != (
        "cpp/tests/joined_federate_mom_object_instances_updated_report_catch2.cpp:137"
    ):
        raise AssertionError("focused joined-federate HLAreportObjectInstancesUpdated source pointer drifted")
    if object_instances_updated_report.get("assertions") != 42:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesUpdated assertion count drifted")
    if object_instances_updated_report.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate HLAreportObjectInstancesUpdated row is not mapped")
    if len(object_instances_updated_report.get("lab_requirement_ids", [])) != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesUpdated requirement mapping drifted")
    if len(object_instances_updated_report.get("standard_sections", [])) != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesUpdated section mapping drifted")
    if len(object_instances_updated_report.get("selected_cpp_api_surface_ids", [])) != 12:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesUpdated API mapping drifted")
    object_instances_updated_report_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-object-instances-updated-report", limit=0
    )
    if object_instances_updated_report_focus.get("lane_state") != "complete":
        raise AssertionError("focused joined-federate HLAreportObjectInstancesUpdated lane is not complete")
    if object_instances_updated_report_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesUpdated mapped count drifted")
    if object_instances_updated_report_focus.get("assertion_count") != 42:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesUpdated assertion total drifted")
    if object_instances_updated_report_focus.get("requirement_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesUpdated requirement total drifted")
    if object_instances_updated_report_focus.get("standard_section_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesUpdated section total drifted")
    if object_instances_updated_report_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesUpdated direct pair drifted")
    object_instances_updated_report_handles = object_instances_updated_report_focus.get("lane_handles")
    if not isinstance(object_instances_updated_report_handles, dict) or object_instances_updated_report_handles.get(
        "catch2_target"
    ) != "umbra_joined_federate_mom_object_instances_updated_report_catch2":
        raise AssertionError("focused joined-federate HLAreportObjectInstancesUpdated target handle drifted")
    object_instances_that_can_be_deleted_report = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-object-instances-that-can-be-deleted-report-integration"
        ),
        None,
    )
    if not isinstance(object_instances_that_can_be_deleted_report, dict):
        raise AssertionError("focused joined-federate HLAreportObjectInstancesThatCanBeDeleted row is absent")
    if query_rti_work.source_location_text(object_instances_that_can_be_deleted_report) != (
        "cpp/tests/joined_federate_mom_object_instances_that_can_be_deleted_report_catch2.cpp:137"
    ):
        raise AssertionError("focused joined-federate HLAreportObjectInstancesThatCanBeDeleted source pointer drifted")
    if object_instances_that_can_be_deleted_report.get("assertions") != 53:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesThatCanBeDeleted assertion count drifted")
    if object_instances_that_can_be_deleted_report.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate HLAreportObjectInstancesThatCanBeDeleted row is not mapped")
    if len(object_instances_that_can_be_deleted_report.get("lab_requirement_ids", [])) != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesThatCanBeDeleted requirement mapping drifted")
    if len(object_instances_that_can_be_deleted_report.get("standard_sections", [])) != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesThatCanBeDeleted section mapping drifted")
    if len(object_instances_that_can_be_deleted_report.get("selected_cpp_api_surface_ids", [])) != 12:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesThatCanBeDeleted API mapping drifted")
    object_instances_that_can_be_deleted_report_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-object-instances-that-can-be-deleted-report", limit=0
    )
    if object_instances_that_can_be_deleted_report_focus.get("lane_state") != "complete":
        raise AssertionError("focused joined-federate HLAreportObjectInstancesThatCanBeDeleted lane is not complete")
    if object_instances_that_can_be_deleted_report_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesThatCanBeDeleted mapped count drifted")
    if object_instances_that_can_be_deleted_report_focus.get("assertion_count") != 53:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesThatCanBeDeleted assertion total drifted")
    if object_instances_that_can_be_deleted_report_focus.get("requirement_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesThatCanBeDeleted requirement total drifted")
    if object_instances_that_can_be_deleted_report_focus.get("standard_section_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesThatCanBeDeleted section total drifted")
    if object_instances_that_can_be_deleted_report_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesThatCanBeDeleted direct pair drifted")
    object_instances_that_can_be_deleted_report_handles = object_instances_that_can_be_deleted_report_focus.get(
        "lane_handles"
    )
    if not isinstance(object_instances_that_can_be_deleted_report_handles, dict) or object_instances_that_can_be_deleted_report_handles.get(
        "catch2_target"
    ) != "umbra_joined_federate_mom_object_instances_that_can_be_deleted_report_catch2":
        raise AssertionError("focused joined-federate HLAreportObjectInstancesThatCanBeDeleted target handle drifted")
    object_instances_reflected_report = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-object-instances-reflected-report-integration"
        ),
        None,
    )
    if not isinstance(object_instances_reflected_report, dict):
        raise AssertionError("focused joined-federate HLAreportObjectInstancesReflected row is absent")
    if query_rti_work.source_location_text(object_instances_reflected_report) != (
        "cpp/tests/joined_federate_mom_object_instances_reflected_report_catch2.cpp:160"
    ):
        raise AssertionError("focused joined-federate HLAreportObjectInstancesReflected source pointer drifted")
    if object_instances_reflected_report.get("assertions") != 51:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesReflected assertion count drifted")
    if object_instances_reflected_report.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate HLAreportObjectInstancesReflected row is not mapped")
    if len(object_instances_reflected_report.get("lab_requirement_ids", [])) != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesReflected requirement mapping drifted")
    if len(object_instances_reflected_report.get("standard_sections", [])) != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesReflected section mapping drifted")
    if len(object_instances_reflected_report.get("selected_cpp_api_surface_ids", [])) != 14:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesReflected API mapping drifted")
    object_instances_reflected_report_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-object-instances-reflected-report", limit=0
    )
    if object_instances_reflected_report_focus.get("lane_state") != "complete":
        raise AssertionError("focused joined-federate HLAreportObjectInstancesReflected lane is not complete")
    if object_instances_reflected_report_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesReflected mapped count drifted")
    if object_instances_reflected_report_focus.get("assertion_count") != 51:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesReflected assertion total drifted")
    if object_instances_reflected_report_focus.get("requirement_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesReflected requirement total drifted")
    if object_instances_reflected_report_focus.get("standard_section_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesReflected section total drifted")
    if object_instances_reflected_report_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstancesReflected direct pair drifted")
    object_instances_reflected_report_handles = object_instances_reflected_report_focus.get(
        "lane_handles"
    )
    if not isinstance(object_instances_reflected_report_handles, dict) or object_instances_reflected_report_handles.get(
        "catch2_target"
    ) != "umbra_joined_federate_mom_object_instances_reflected_report_catch2":
        raise AssertionError("focused joined-federate HLAreportObjectInstancesReflected target handle drifted")
    object_instance_information_report = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-object-instance-information-report-integration"
        ),
        None,
    )
    if not isinstance(object_instance_information_report, dict):
        raise AssertionError("focused joined-federate HLAreportObjectInstanceInformation row is absent")
    if query_rti_work.source_location_text(object_instance_information_report) != (
        "cpp/tests/joined_federate_mom_object_instance_information_report_catch2.cpp:146"
    ):
        raise AssertionError("focused joined-federate HLAreportObjectInstanceInformation source pointer drifted")
    if object_instance_information_report.get("assertions") != 71:
        raise AssertionError("focused joined-federate HLAreportObjectInstanceInformation assertion count drifted")
    if object_instance_information_report.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate HLAreportObjectInstanceInformation row is not mapped")
    if len(object_instance_information_report.get("lab_requirement_ids", [])) != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstanceInformation requirement mapping drifted")
    if len(object_instance_information_report.get("standard_sections", [])) != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstanceInformation section mapping drifted")
    if len(object_instance_information_report.get("selected_cpp_api_surface_ids", [])) != 13:
        raise AssertionError("focused joined-federate HLAreportObjectInstanceInformation API mapping drifted")
    object_instance_information_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-object-instance-information-report", limit=0
    )
    if object_instance_information_focus.get("lane_state") != "complete":
        raise AssertionError("focused joined-federate HLAreportObjectInstanceInformation lane is not complete")
    if object_instance_information_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstanceInformation mapped count drifted")
    if object_instance_information_focus.get("assertion_count") != 71:
        raise AssertionError("focused joined-federate HLAreportObjectInstanceInformation assertion total drifted")
    if object_instance_information_focus.get("requirement_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstanceInformation requirement total drifted")
    if object_instance_information_focus.get("standard_section_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstanceInformation section total drifted")
    if object_instance_information_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused joined-federate HLAreportObjectInstanceInformation direct pair drifted")
    object_instance_information_handles = object_instance_information_focus.get("lane_handles")
    if not isinstance(object_instance_information_handles, dict) or object_instance_information_handles.get(
        "catch2_target"
    ) != "umbra_joined_federate_mom_object_instance_information_report_catch2":
        raise AssertionError("focused joined-federate HLAreportObjectInstanceInformation target handle drifted")
    discovered_object_count_periodic_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-discovered-object-count-periodic-integration"
        ),
        None,
    )
    if not isinstance(discovered_object_count_periodic_mom, dict):
        raise AssertionError("focused joined-federate HLAobjectInstancesDiscovered row is absent")
    if query_rti_work.source_location_text(discovered_object_count_periodic_mom) != (
        "cpp/tests/joined_federate_mom_discovered_object_count_periodic_catch2.cpp:127"
    ):
        raise AssertionError("focused joined-federate HLAobjectInstancesDiscovered source pointer drifted")
    if discovered_object_count_periodic_mom.get("assertions") != 66:
        raise AssertionError("focused joined-federate HLAobjectInstancesDiscovered assertion count drifted")
    if discovered_object_count_periodic_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate HLAobjectInstancesDiscovered row is not mapped")
    if len(discovered_object_count_periodic_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesDiscovered requirement mapping drifted")
    if len(discovered_object_count_periodic_mom.get("standard_sections", [])) != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesDiscovered section mapping drifted")
    if len(discovered_object_count_periodic_mom.get("selected_cpp_api_surface_ids", [])) != 14:
        raise AssertionError("focused joined-federate HLAobjectInstancesDiscovered API mapping drifted")
    discovered_object_count_periodic_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-discovered-object-count-periodic", limit=0
    )
    if discovered_object_count_periodic_focus.get("lane_state") != "complete":
        raise AssertionError("focused joined-federate HLAobjectInstancesDiscovered lane is not complete")
    if discovered_object_count_periodic_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesDiscovered mapped count drifted")
    if discovered_object_count_periodic_focus.get("assertion_count") != 66:
        raise AssertionError("focused joined-federate HLAobjectInstancesDiscovered assertion total drifted")
    if discovered_object_count_periodic_focus.get("requirement_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesDiscovered requirement total drifted")
    if discovered_object_count_periodic_focus.get("standard_section_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesDiscovered section total drifted")
    if discovered_object_count_periodic_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesDiscovered direct pair drifted")
    discovered_object_count_periodic_handles = discovered_object_count_periodic_focus.get("lane_handles")
    if not isinstance(discovered_object_count_periodic_handles, dict) or discovered_object_count_periodic_handles.get(
        "catch2_target"
    ) != "umbra_joined_federate_mom_discovered_object_count_periodic_catch2":
        raise AssertionError("focused joined-federate HLAobjectInstancesDiscovered target handle drifted")
    removed_object_count_periodic_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-removed-object-count-periodic-integration"
        ),
        None,
    )
    if not isinstance(removed_object_count_periodic_mom, dict):
        raise AssertionError("focused joined-federate HLAobjectInstancesRemoved row is absent")
    if query_rti_work.source_location_text(removed_object_count_periodic_mom) != (
        "cpp/tests/joined_federate_mom_removed_object_count_periodic_catch2.cpp:145"
    ):
        raise AssertionError("focused joined-federate HLAobjectInstancesRemoved source pointer drifted")
    if removed_object_count_periodic_mom.get("assertions") != 78:
        raise AssertionError("focused joined-federate HLAobjectInstancesRemoved assertion count drifted")
    if removed_object_count_periodic_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate HLAobjectInstancesRemoved row is not mapped")
    if len(removed_object_count_periodic_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesRemoved requirement mapping drifted")
    if len(removed_object_count_periodic_mom.get("standard_sections", [])) != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesRemoved section mapping drifted")
    if len(removed_object_count_periodic_mom.get("selected_cpp_api_surface_ids", [])) != 14:
        raise AssertionError("focused joined-federate HLAobjectInstancesRemoved API mapping drifted")
    removed_object_count_periodic_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-removed-object-count-periodic", limit=0
    )
    if removed_object_count_periodic_focus.get("lane_state") != "complete":
        raise AssertionError("focused joined-federate HLAobjectInstancesRemoved lane is not complete")
    if removed_object_count_periodic_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesRemoved mapped count drifted")
    if removed_object_count_periodic_focus.get("assertion_count") != 78:
        raise AssertionError("focused joined-federate HLAobjectInstancesRemoved assertion total drifted")
    if removed_object_count_periodic_focus.get("requirement_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesRemoved requirement total drifted")
    if removed_object_count_periodic_focus.get("standard_section_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesRemoved section total drifted")
    if removed_object_count_periodic_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesRemoved direct pair drifted")
    removed_object_count_periodic_handles = removed_object_count_periodic_focus.get("lane_handles")
    if not isinstance(removed_object_count_periodic_handles, dict) or removed_object_count_periodic_handles.get(
        "catch2_target"
    ) != "umbra_joined_federate_mom_removed_object_count_periodic_catch2":
        raise AssertionError("focused joined-federate HLAobjectInstancesRemoved target handle drifted")
    deleted_object_count_periodic_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-deleted-object-count-periodic-integration"
        ),
        None,
    )
    if not isinstance(deleted_object_count_periodic_mom, dict):
        raise AssertionError("focused joined-federate HLAobjectInstancesDeleted row is absent")
    if query_rti_work.source_location_text(deleted_object_count_periodic_mom) != (
        "cpp/tests/joined_federate_mom_deleted_object_count_periodic_catch2.cpp:108"
    ):
        raise AssertionError("focused joined-federate HLAobjectInstancesDeleted source pointer drifted")
    if deleted_object_count_periodic_mom.get("assertions") != 66:
        raise AssertionError("focused joined-federate HLAobjectInstancesDeleted assertion count drifted")
    if deleted_object_count_periodic_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate HLAobjectInstancesDeleted row is not mapped")
    if len(deleted_object_count_periodic_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesDeleted requirement mapping drifted")
    if len(deleted_object_count_periodic_mom.get("standard_sections", [])) != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesDeleted section mapping drifted")
    if len(deleted_object_count_periodic_mom.get("selected_cpp_api_surface_ids", [])) != 12:
        raise AssertionError("focused joined-federate HLAobjectInstancesDeleted API mapping drifted")
    deleted_object_count_periodic_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-deleted-object-count-periodic", limit=0
    )
    if deleted_object_count_periodic_focus.get("lane_state") != "complete":
        raise AssertionError("focused joined-federate HLAobjectInstancesDeleted lane is not complete")
    if deleted_object_count_periodic_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesDeleted mapped count drifted")
    if deleted_object_count_periodic_focus.get("assertion_count") != 66:
        raise AssertionError("focused joined-federate HLAobjectInstancesDeleted assertion total drifted")
    if deleted_object_count_periodic_focus.get("requirement_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesDeleted requirement total drifted")
    if deleted_object_count_periodic_focus.get("standard_section_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesDeleted section total drifted")
    if deleted_object_count_periodic_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesDeleted direct pair drifted")
    deleted_object_count_periodic_handles = deleted_object_count_periodic_focus.get("lane_handles")
    if not isinstance(deleted_object_count_periodic_handles, dict) or deleted_object_count_periodic_handles.get(
        "catch2_target"
    ) != "umbra_joined_federate_mom_deleted_object_count_periodic_catch2":
        raise AssertionError("focused joined-federate HLAobjectInstancesDeleted target handle drifted")
    registered_object_count_periodic_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-registered-object-count-periodic-integration"
        ),
        None,
    )
    if not isinstance(registered_object_count_periodic_mom, dict):
        raise AssertionError("focused joined-federate HLAobjectInstancesRegistered row is absent")
    if query_rti_work.source_location_text(registered_object_count_periodic_mom) != (
        "cpp/tests/joined_federate_mom_registered_object_count_periodic_catch2.cpp:115"
    ):
        raise AssertionError("focused joined-federate HLAobjectInstancesRegistered source pointer drifted")
    if registered_object_count_periodic_mom.get("assertions") != 112:
        raise AssertionError("focused joined-federate HLAobjectInstancesRegistered assertion count drifted")
    if registered_object_count_periodic_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate HLAobjectInstancesRegistered row is not mapped")
    if len(registered_object_count_periodic_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesRegistered requirement mapping drifted")
    if len(registered_object_count_periodic_mom.get("standard_sections", [])) != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesRegistered section mapping drifted")
    if len(registered_object_count_periodic_mom.get("selected_cpp_api_surface_ids", [])) != 12:
        raise AssertionError("focused joined-federate HLAobjectInstancesRegistered API mapping drifted")
    registered_object_count_periodic_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-registered-object-count-periodic", limit=0
    )
    if registered_object_count_periodic_focus.get("lane_state") != "complete":
        raise AssertionError("focused joined-federate HLAobjectInstancesRegistered lane is not complete")
    if registered_object_count_periodic_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesRegistered mapped count drifted")
    if registered_object_count_periodic_focus.get("assertion_count") != 112:
        raise AssertionError("focused joined-federate HLAobjectInstancesRegistered assertion total drifted")
    if registered_object_count_periodic_focus.get("requirement_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesRegistered requirement total drifted")
    if registered_object_count_periodic_focus.get("standard_section_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesRegistered section total drifted")
    if registered_object_count_periodic_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesRegistered direct pair drifted")
    registered_object_count_periodic_handles = registered_object_count_periodic_focus.get("lane_handles")
    if not isinstance(registered_object_count_periodic_handles, dict) or registered_object_count_periodic_handles.get(
        "catch2_target"
    ) != "umbra_joined_federate_mom_registered_object_count_periodic_catch2":
        raise AssertionError("focused joined-federate HLAobjectInstancesRegistered target handle drifted")
    updated_object_count_periodic_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-updated-object-count-periodic-integration"
        ),
        None,
    )
    if not isinstance(updated_object_count_periodic_mom, dict):
        raise AssertionError("focused joined-federate HLAobjectInstancesUpdated row is absent")
    if query_rti_work.source_location_text(updated_object_count_periodic_mom) != (
        "cpp/tests/joined_federate_mom_updated_object_count_periodic_catch2.cpp:109"
    ):
        raise AssertionError("focused joined-federate HLAobjectInstancesUpdated source pointer drifted")
    if updated_object_count_periodic_mom.get("assertions") != 77:
        raise AssertionError("focused joined-federate HLAobjectInstancesUpdated assertion count drifted")
    if updated_object_count_periodic_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate HLAobjectInstancesUpdated row is not mapped")
    if len(updated_object_count_periodic_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesUpdated requirement mapping drifted")
    if len(updated_object_count_periodic_mom.get("standard_sections", [])) != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesUpdated section mapping drifted")
    if len(updated_object_count_periodic_mom.get("selected_cpp_api_surface_ids", [])) != 12:
        raise AssertionError("focused joined-federate HLAobjectInstancesUpdated API mapping drifted")
    updated_object_count_periodic_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-updated-object-count-periodic", limit=0
    )
    if updated_object_count_periodic_focus.get("lane_state") != "complete":
        raise AssertionError("focused joined-federate HLAobjectInstancesUpdated lane is not complete")
    if updated_object_count_periodic_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesUpdated mapped count drifted")
    if updated_object_count_periodic_focus.get("assertion_count") != 77:
        raise AssertionError("focused joined-federate HLAobjectInstancesUpdated assertion total drifted")
    if updated_object_count_periodic_focus.get("requirement_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesUpdated requirement total drifted")
    if updated_object_count_periodic_focus.get("standard_section_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesUpdated section total drifted")
    if updated_object_count_periodic_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused joined-federate HLAobjectInstancesUpdated direct pair drifted")
    updated_object_count_periodic_handles = updated_object_count_periodic_focus.get("lane_handles")
    if not isinstance(updated_object_count_periodic_handles, dict) or updated_object_count_periodic_handles.get(
        "catch2_target"
    ) != "umbra_joined_federate_mom_updated_object_count_periodic_catch2":
        raise AssertionError("focused joined-federate HLAobjectInstancesUpdated target handle drifted")
    updates_sent_periodic_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-updates-sent-periodic-integration"
        ),
        None,
    )
    if not isinstance(updates_sent_periodic_mom, dict):
        raise AssertionError("focused joined-federate HLAupdatesSent row is absent")
    if query_rti_work.source_location_text(updates_sent_periodic_mom) != (
        "cpp/tests/joined_federate_mom_updates_sent_periodic_catch2.cpp:108"
    ):
        raise AssertionError("focused joined-federate HLAupdatesSent source pointer drifted")
    if updates_sent_periodic_mom.get("assertions") != 56:
        raise AssertionError("focused joined-federate HLAupdatesSent assertion count drifted")
    if updates_sent_periodic_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate HLAupdatesSent row is not mapped")
    if len(updates_sent_periodic_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError("focused joined-federate HLAupdatesSent requirement mapping drifted")
    if len(updates_sent_periodic_mom.get("standard_sections", [])) != 1:
        raise AssertionError("focused joined-federate HLAupdatesSent section mapping drifted")
    if len(updates_sent_periodic_mom.get("selected_cpp_api_surface_ids", [])) != 12:
        raise AssertionError("focused joined-federate HLAupdatesSent API mapping drifted")
    updates_sent_periodic_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-updates-sent-periodic", limit=0
    )
    if updates_sent_periodic_focus.get("lane_state") != "complete":
        raise AssertionError("focused joined-federate HLAupdatesSent lane is not complete")
    if updates_sent_periodic_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused joined-federate HLAupdatesSent mapped count drifted")
    if updates_sent_periodic_focus.get("assertion_count") != 56:
        raise AssertionError("focused joined-federate HLAupdatesSent assertion total drifted")
    if updates_sent_periodic_focus.get("requirement_count") != 1:
        raise AssertionError("focused joined-federate HLAupdatesSent requirement total drifted")
    if updates_sent_periodic_focus.get("standard_section_count") != 1:
        raise AssertionError("focused joined-federate HLAupdatesSent section total drifted")
    if updates_sent_periodic_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused joined-federate HLAupdatesSent direct pair drifted")
    updates_sent_periodic_handles = updates_sent_periodic_focus.get("lane_handles")
    if not isinstance(updates_sent_periodic_handles, dict) or updates_sent_periodic_handles.get(
        "catch2_target"
    ) != "umbra_joined_federate_mom_updates_sent_periodic_catch2":
        raise AssertionError("focused joined-federate HLAupdatesSent target handle drifted")
    ro_length_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-ro-length-periodic-integration"
        ),
        None,
    )
    if not isinstance(ro_length_mom, dict):
        raise AssertionError("focused joined-federate RO-length MOM row is absent")
    if query_rti_work.source_location_text(ro_length_mom) != (
        "cpp/tests/joined_federate_mom_ro_length_periodic_catch2.cpp:134"
    ):
        raise AssertionError("focused joined-federate RO-length source pointer drifted")
    if ro_length_mom.get("assertions") != 63:
        raise AssertionError("focused joined-federate RO-length assertion count drifted")
    if ro_length_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate RO-length row is not mapped")
    if len(ro_length_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError("focused joined-federate RO-length requirement mapping drifted")
    if len(ro_length_mom.get("standard_sections", [])) != 1:
        raise AssertionError("focused joined-federate RO-length section mapping drifted")
    if len(ro_length_mom.get("selected_cpp_api_surface_ids", [])) != 12:
        raise AssertionError("focused joined-federate RO-length API mapping drifted")
    ro_length_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-ro-length-periodic", limit=0
    )
    if ro_length_focus.get("lane_state") != "complete":
        raise AssertionError("focused joined-federate RO-length lane is not complete")
    if ro_length_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused joined-federate RO-length mapped count drifted")
    if ro_length_focus.get("assertion_count") != 63:
        raise AssertionError("focused joined-federate RO-length assertion total drifted")
    if ro_length_focus.get("requirement_count") != 1:
        raise AssertionError("focused joined-federate RO-length requirement total drifted")
    if ro_length_focus.get("standard_section_count") != 1:
        raise AssertionError("focused joined-federate RO-length section total drifted")
    if ro_length_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused joined-federate RO-length direct pair drifted")
    ro_length_handles = ro_length_focus.get("lane_handles")
    if not isinstance(ro_length_handles, dict) or ro_length_handles.get(
        "catch2_target"
    ) != "umbra_joined_federate_mom_ro_length_periodic_catch2":
        raise AssertionError("focused joined-federate RO-length target handle drifted")
    deletable_count_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-deletable-object-count-periodic-integration"
        ),
        None,
    )
    if not isinstance(deletable_count_mom, dict):
        raise AssertionError("focused joined-federate deletable-count MOM row is absent")
    if query_rti_work.source_location_text(deletable_count_mom) != (
        "cpp/tests/joined_federate_mom_deletable_object_count_catch2.cpp:108"
    ):
        raise AssertionError("focused joined-federate deletable-count source pointer drifted")
    if deletable_count_mom.get("assertions") != 55:
        raise AssertionError("focused joined-federate deletable-count assertion count drifted")
    if deletable_count_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate deletable-count row is not mapped")
    if len(deletable_count_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError("focused joined-federate deletable-count requirement mapping drifted")
    if len(deletable_count_mom.get("standard_sections", [])) != 1:
        raise AssertionError("focused joined-federate deletable-count section mapping drifted")
    if len(deletable_count_mom.get("selected_cpp_api_surface_ids", [])) != 12:
        raise AssertionError("focused joined-federate deletable-count API mapping drifted")
    deletable_count_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-deletable-object-count", limit=0
    )
    if deletable_count_focus.get("lane_state") != "complete":
        raise AssertionError("focused joined-federate deletable-count lane is not complete")
    if deletable_count_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused joined-federate deletable-count mapped count drifted")
    if deletable_count_focus.get("assertion_count") != 55:
        raise AssertionError("focused joined-federate deletable-count assertion total drifted")
    if deletable_count_focus.get("requirement_count") != 1:
        raise AssertionError("focused joined-federate deletable-count requirement total drifted")
    if deletable_count_focus.get("standard_section_count") != 1:
        raise AssertionError("focused joined-federate deletable-count section total drifted")
    if deletable_count_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused joined-federate deletable-count direct pair drifted")
    deletable_count_handles = deletable_count_focus.get("lane_handles")
    if not isinstance(deletable_count_handles, dict) or deletable_count_handles.get(
        "catch2_target"
    ) != "umbra_joined_federate_mom_deletable_object_count_catch2":
        raise AssertionError("focused joined-federate deletable-count target handle drifted")
    service_reporting_interlock = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-mom-service-reporting-interlock-integration"
        ),
        None,
    )
    if not isinstance(service_reporting_interlock, dict):
        raise AssertionError("focused service-reporting interlock row is absent")
    if query_rti_work.source_location_text(service_reporting_interlock) != (
        "cpp/tests/mom_service_reporting_interlock_catch2.cpp:50"
    ):
        raise AssertionError("focused service-reporting interlock source pointer drifted")
    if service_reporting_interlock.get("assertions") != 41:
        raise AssertionError("focused service-reporting interlock assertion count drifted")
    if service_reporting_interlock.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused service-reporting interlock row is not mapped")
    service_reporting_interlock_focus = query_rti_work.focused_lane_result(
        index, tests, "service-reporting-interlock", limit=0
    )
    if service_reporting_interlock_focus.get("lane_state") != "complete":
        raise AssertionError("focused service-reporting interlock lane is not complete")
    if service_reporting_interlock_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused service-reporting interlock mapped count drifted")
    if service_reporting_interlock_focus.get("assertion_count") != 41:
        raise AssertionError("focused service-reporting interlock assertion total drifted")
    service_reporting_interlock_handles = service_reporting_interlock_focus.get("lane_handles")
    if not isinstance(service_reporting_interlock_handles, dict) or service_reporting_interlock_handles.get(
        "catch2_target"
    ) != "umbra_mom_service_reporting_interlock_catch2":
        raise AssertionError("focused service-reporting interlock target handle drifted")
    negotiated_cancellation_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-service-report-interaction-cancel-negotiated-attribute-ownership-divestiture-integration"
        ),
        None,
    )
    if not isinstance(negotiated_cancellation_mom, dict):
        raise AssertionError("focused negotiated cancellation MOM row is absent")
    if query_rti_work.source_location_text(negotiated_cancellation_mom) != (
        "cpp/tests/negotiated_attribute_ownership_divestiture_pending_catch2.cpp:547"
    ):
        raise AssertionError("focused negotiated cancellation source pointer drifted")
    if negotiated_cancellation_mom.get("assertions") != 87:
        raise AssertionError("focused negotiated cancellation assertion count drifted")
    if negotiated_cancellation_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused negotiated cancellation row is not mapped")
    negotiated_cancellation_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "cancel-negotiated-attribute-ownership-divestiture-service-report-interaction",
        limit=0,
    )
    if negotiated_cancellation_focus.get("lane_state") != "complete":
        raise AssertionError("focused negotiated cancellation lane is not complete")
    if negotiated_cancellation_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused negotiated cancellation mapped count drifted")
    if negotiated_cancellation_focus.get("assertion_count") != 87:
        raise AssertionError("focused negotiated cancellation assertion total drifted")
    negotiated_cancellation_handles = negotiated_cancellation_focus.get("lane_handles")
    if not isinstance(negotiated_cancellation_handles, dict) or negotiated_cancellation_handles.get(
        "catch2_target"
    ) != "umbra_negotiated_attribute_ownership_divestiture_pending_catch2":
        raise AssertionError("focused negotiated cancellation target handle drifted")
    cancellation_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-service-report-interaction-cancel-attribute-ownership-acquisition-integration"
        ),
        None,
    )
    if not isinstance(cancellation_mom, dict):
        raise AssertionError("focused Cancel Attribute Ownership Acquisition MOM row is absent")
    if query_rti_work.source_location_text(cancellation_mom) != (
        "cpp/tests/attribute_ownership_acquisition_cancellation_catch2.cpp:387"
    ):
        raise AssertionError("focused cancellation MOM source pointer drifted")
    if cancellation_mom.get("assertions") != 77:
        raise AssertionError("focused cancellation MOM assertion count drifted")
    if cancellation_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused cancellation MOM row is not mapped")
    cancellation_mom_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "cancel-attribute-ownership-acquisition-service-report-interaction",
        limit=0,
    )
    if cancellation_mom_focus.get("lane_state") != "complete":
        raise AssertionError("focused cancellation MOM lane is not complete")
    if cancellation_mom_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused cancellation MOM mapped count drifted")
    if cancellation_mom_focus.get("assertion_count") != 77:
        raise AssertionError("focused cancellation MOM assertion total drifted")
    cancellation_mom_handles = cancellation_mom_focus.get("lane_handles")
    if not isinstance(cancellation_mom_handles, dict) or cancellation_mom_handles.get(
        "catch2_target"
    ) != "umbra_attribute_ownership_acquisition_cancellation_catch2":
        raise AssertionError("focused cancellation MOM target handle drifted")
    ownership_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-mom-query-attribute-ownership-interaction-integration"
        ),
        None,
    )
    if not isinstance(ownership_mom, dict):
        raise AssertionError("focused Query Attribute Ownership MOM row is absent")
    if query_rti_work.source_location_text(ownership_mom) != (
        "cpp/tests/attribute_ownership_query_catch2.cpp:379"
    ):
        raise AssertionError("focused Query Attribute Ownership MOM source pointer drifted")
    if ownership_mom.get("assertions") != 67:
        raise AssertionError("focused Query Attribute Ownership MOM assertion count drifted")
    ownership_mom_focus = query_rti_work.focused_lane_result(
        index, tests, "query-attribute-ownership-service-report-interaction", limit=0
    )
    if ownership_mom_focus.get("lane_state") != "complete":
        raise AssertionError("focused Query Attribute Ownership MOM lane is not complete")
    if ownership_mom_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused Query Attribute Ownership MOM mapped count drifted")
    if ownership_mom_focus.get("assertion_count") != 67:
        raise AssertionError("focused Query Attribute Ownership MOM assertion total drifted")
    ownership_mom_handles = ownership_mom_focus.get("lane_handles")
    if not isinstance(ownership_mom_handles, dict) or ownership_mom_handles.get(
        "catch2_target"
    ) != "umbra_attribute_ownership_query_catch2":
        raise AssertionError("focused Query Attribute Ownership MOM target handle drifted")
    attribute_relevance = next(
        (
            test
            for test in tests
            if test.get("test_case")
            == "Embedded attribute relevance advisories follow scope transitions"
        ),
        None,
    )
    if not isinstance(attribute_relevance, dict):
        raise AssertionError("focused attribute-relevance plan row is absent")
    if query_rti_work.source_location_text(attribute_relevance) != (
        "cpp/tests/attribute_relevance_advisory_catch2.cpp:265"
    ):
        raise AssertionError("focused attribute-relevance source pointer drifted")
    if attribute_relevance.get("assertions") != 106:
        raise AssertionError("focused attribute-relevance assertion count drifted")
    attribute_relevance_focus = query_rti_work.focused_lane_result(
        index, tests, "attribute-relevance-scope-transition", limit=0
    )
    if attribute_relevance_focus.get("lane_state") != "complete":
        raise AssertionError("focused attribute-relevance lane is not complete")
    if attribute_relevance_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused attribute-relevance mapped count drifted")
    if attribute_relevance_focus.get("assertion_count") != 106:
        raise AssertionError("focused attribute-relevance lane assertion total drifted")
    attribute_relevance_handles = attribute_relevance_focus.get("lane_handles")
    if not isinstance(attribute_relevance_handles, dict) or attribute_relevance_handles.get(
        "catch2_target"
    ) != "umbra_attribute_relevance_advisory_catch2":
        raise AssertionError("focused attribute-relevance target handle drifted")
    attribute_relevance_rate_reissue = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-relevance-advisory-rate-reissue-integration"
        ),
        None,
    )
    if not isinstance(attribute_relevance_rate_reissue, dict):
        raise AssertionError("focused attribute-relevance rate-reissue row is absent")
    if query_rti_work.source_location_text(attribute_relevance_rate_reissue) != (
        "cpp/tests/attribute_relevance_rate_reissue_catch2.cpp:108"
    ):
        raise AssertionError("focused attribute-relevance rate-reissue source pointer drifted")
    if attribute_relevance_rate_reissue.get("assertions") != 108:
        raise AssertionError("focused attribute-relevance rate-reissue assertion count drifted")
    if attribute_relevance_rate_reissue.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused attribute-relevance rate-reissue row is not mapped")
    if len(attribute_relevance_rate_reissue.get("lab_requirement_ids", [])) != 7:
        raise AssertionError("focused attribute-relevance rate-reissue requirement mapping drifted")
    if len(attribute_relevance_rate_reissue.get("standard_sections", [])) != 2:
        raise AssertionError("focused attribute-relevance rate-reissue section mapping drifted")
    if len(attribute_relevance_rate_reissue.get("selected_cpp_api_surface_ids", [])) != 3:
        raise AssertionError("focused attribute-relevance rate-reissue API mapping drifted")
    attribute_relevance_rate_reissue_focus = query_rti_work.focused_lane_result(
        index, tests, "attribute-relevance-rate-reissue", limit=0
    )
    if attribute_relevance_rate_reissue_focus.get("lane_state") != "complete":
        raise AssertionError("focused attribute-relevance rate-reissue lane is not complete")
    if attribute_relevance_rate_reissue_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused attribute-relevance rate-reissue mapped count drifted")
    if attribute_relevance_rate_reissue_focus.get("assertion_count") != 108:
        raise AssertionError("focused attribute-relevance rate-reissue lane assertion total drifted")
    attribute_relevance_rate_reissue_handles = attribute_relevance_rate_reissue_focus.get(
        "lane_handles"
    )
    if not isinstance(attribute_relevance_rate_reissue_handles, dict) or attribute_relevance_rate_reissue_handles.get(
        "catch2_target"
    ) != "umbra_attribute_relevance_rate_reissue_catch2":
        raise AssertionError("focused attribute-relevance rate-reissue target handle drifted")
    regional_attribute_relevance_rate_designator = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-regional-attribute-relevance-advisory-rate-designator-integration"
        ),
        None,
    )
    if not isinstance(regional_attribute_relevance_rate_designator, dict):
        raise AssertionError("focused regional attribute-relevance rate-designator row is absent")
    if query_rti_work.source_location_text(regional_attribute_relevance_rate_designator) != (
        "cpp/tests/regional_attribute_relevance_rate_designator_catch2.cpp:105"
    ):
        raise AssertionError("focused regional rate-designator source pointer drifted")
    if regional_attribute_relevance_rate_designator.get("assertions") != 100:
        raise AssertionError("focused regional rate-designator assertion count drifted")
    if regional_attribute_relevance_rate_designator.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused regional rate-designator row is not mapped")
    if len(regional_attribute_relevance_rate_designator.get("lab_requirement_ids", [])) != 8:
        raise AssertionError("focused regional rate-designator requirement mapping drifted")
    if len(regional_attribute_relevance_rate_designator.get("standard_sections", [])) != 3:
        raise AssertionError("focused regional rate-designator section mapping drifted")
    if len(regional_attribute_relevance_rate_designator.get("selected_cpp_api_surface_ids", [])) != 7:
        raise AssertionError("focused regional rate-designator API mapping drifted")
    regional_attribute_relevance_rate_designator_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "regional-attribute-relevance-rate-designator",
        limit=0,
    )
    if regional_attribute_relevance_rate_designator_focus.get("lane_state") != "complete":
        raise AssertionError("focused regional rate-designator lane is not complete")
    if regional_attribute_relevance_rate_designator_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused regional rate-designator mapped count drifted")
    if regional_attribute_relevance_rate_designator_focus.get("assertion_count") != 100:
        raise AssertionError("focused regional rate-designator lane assertion total drifted")
    regional_attribute_relevance_rate_designator_handles = (
        regional_attribute_relevance_rate_designator_focus.get("lane_handles")
    )
    if not isinstance(regional_attribute_relevance_rate_designator_handles, dict) or regional_attribute_relevance_rate_designator_handles.get(
        "catch2_target"
    ) != "umbra_regional_attribute_relevance_rate_designator_catch2":
        raise AssertionError("focused regional rate-designator target handle drifted")
    known_class_disabled = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-relevance-known-class-disabled-subscription-integration"
        ),
        None,
    )
    if not isinstance(known_class_disabled, dict):
        raise AssertionError("focused known-class-disabled advisory row is absent")
    if query_rti_work.source_location_text(known_class_disabled) != (
        "cpp/tests/attribute_relevance_known_class_disabled_subscription_catch2.cpp:97"
    ):
        raise AssertionError("focused known-class-disabled advisory source pointer drifted")
    if known_class_disabled.get("assertions") != 35:
        raise AssertionError("focused known-class-disabled advisory assertion count drifted")
    if known_class_disabled.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused known-class-disabled advisory row is not mapped")
    if len(known_class_disabled.get("lab_requirement_ids", [])) != 18:
        raise AssertionError("focused known-class-disabled advisory requirement mapping drifted")
    if len(known_class_disabled.get("standard_sections", [])) != 13:
        raise AssertionError("focused known-class-disabled advisory section mapping drifted")
    if len(known_class_disabled.get("selected_cpp_api_surface_ids", [])) != 18:
        raise AssertionError("focused known-class-disabled advisory API mapping drifted")
    known_class_disabled_focus = query_rti_work.focused_lane_result(
        index, tests, "known-class-disabled", limit=0
    )
    if known_class_disabled_focus.get("lane_state") != "complete":
        raise AssertionError("focused known-class-disabled advisory lane is not complete")
    if known_class_disabled_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused known-class-disabled advisory mapped count drifted")
    if known_class_disabled_focus.get("assertion_count") != 35:
        raise AssertionError("focused known-class-disabled advisory assertion total drifted")
    known_class_disabled_handles = known_class_disabled_focus.get("lane_handles")
    if not isinstance(known_class_disabled_handles, dict) or known_class_disabled_handles.get(
        "catch2_target"
    ) != "umbra_attribute_relevance_known_class_disabled_subscription_catch2":
        raise AssertionError("focused known-class-disabled advisory target handle drifted")
    known_class_enabled = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-relevance-known-class-enabled-subscription-integration"
        ),
        None,
    )
    if not isinstance(known_class_enabled, dict):
        raise AssertionError("focused known-class-enabled advisory row is absent")
    if query_rti_work.source_location_text(known_class_enabled) != (
        "cpp/tests/attribute_relevance_known_class_enabled_subscription_catch2.cpp:97"
    ):
        raise AssertionError("focused known-class-enabled advisory source pointer drifted")
    if known_class_enabled.get("assertions") != 30:
        raise AssertionError("focused known-class-enabled advisory assertion count drifted")
    if known_class_enabled.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused known-class-enabled advisory row is not mapped")
    if len(known_class_enabled.get("lab_requirement_ids", [])) != 15:
        raise AssertionError("focused known-class-enabled advisory requirement mapping drifted")
    if len(known_class_enabled.get("standard_sections", [])) != 13:
        raise AssertionError("focused known-class-enabled advisory section mapping drifted")
    if len(known_class_enabled.get("selected_cpp_api_surface_ids", [])) != 17:
        raise AssertionError("focused known-class-enabled advisory API mapping drifted")
    known_class_enabled_focus = query_rti_work.focused_lane_result(
        index, tests, "known-class-enabled", limit=0
    )
    if known_class_enabled_focus.get("lane_state") != "complete":
        raise AssertionError("focused known-class-enabled advisory lane is not complete")
    if known_class_enabled_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused known-class-enabled advisory mapped count drifted")
    if known_class_enabled_focus.get("assertion_count") != 30:
        raise AssertionError("focused known-class-enabled advisory assertion total drifted")
    known_class_enabled_handles = known_class_enabled_focus.get("lane_handles")
    if not isinstance(known_class_enabled_handles, dict) or known_class_enabled_handles.get(
        "catch2_target"
    ) != "umbra_attribute_relevance_known_class_enabled_subscription_catch2":
        raise AssertionError("focused known-class-enabled advisory target handle drifted")
    passive_regional_update_rate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-update-rate-lookup-passive-regional-subscription-integration"
        ),
        None,
    )
    if not isinstance(passive_regional_update_rate, dict):
        raise AssertionError("focused passive regional update-rate row is absent")
    if query_rti_work.source_location_text(passive_regional_update_rate) != (
        "cpp/tests/update_rate_passive_regional_subscription_catch2.cpp:73"
    ):
        raise AssertionError("focused passive regional update-rate source pointer drifted")
    if passive_regional_update_rate.get("assertions") != 43:
        raise AssertionError("focused passive regional update-rate assertion count drifted")
    if passive_regional_update_rate.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused passive regional update-rate row is not mapped")
    if len(passive_regional_update_rate.get("lab_requirement_ids", [])) != 15:
        raise AssertionError("focused passive regional update-rate requirement mapping drifted")
    if len(passive_regional_update_rate.get("standard_sections", [])) != 12:
        raise AssertionError("focused passive regional update-rate section mapping drifted")
    if len(passive_regional_update_rate.get("selected_cpp_api_surface_ids", [])) != 21:
        raise AssertionError("focused passive regional update-rate API mapping drifted")
    passive_regional_update_rate_focus = query_rti_work.focused_lane_result(
        index, tests, "update-rate-passive-regional-subscription", limit=0
    )
    if passive_regional_update_rate_focus.get("lane_state") != "complete":
        raise AssertionError("focused passive regional update-rate lane is not complete")
    if passive_regional_update_rate_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused passive regional update-rate mapped count drifted")
    if passive_regional_update_rate_focus.get("assertion_count") != 43:
        raise AssertionError("focused passive regional update-rate assertion total drifted")
    passive_regional_update_rate_handles = passive_regional_update_rate_focus.get("lane_handles")
    if not isinstance(passive_regional_update_rate_handles, dict) or passive_regional_update_rate_handles.get(
        "catch2_target"
    ) != "umbra_update_rate_passive_regional_subscription_catch2":
        raise AssertionError("focused passive regional update-rate target handle drifted")
    mixed_update_rate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-mixed-update-rate-subscriptions-independent-attribute-gating-integration"
        ),
        None,
    )
    if not isinstance(mixed_update_rate, dict):
        raise AssertionError("focused mixed update-rate row is absent")
    if query_rti_work.source_location_text(mixed_update_rate) != (
        "cpp/tests/mixed_update_rate_subscriptions_catch2.cpp:199"
    ):
        raise AssertionError("focused mixed update-rate source pointer drifted")
    if mixed_update_rate.get("assertions") != 37:
        raise AssertionError("focused mixed update-rate assertion count drifted")
    if mixed_update_rate.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused mixed update-rate row is not mapped")
    if len(mixed_update_rate.get("lab_requirement_ids", [])) != 8:
        raise AssertionError("focused mixed update-rate requirement mapping drifted")
    if len(mixed_update_rate.get("standard_sections", [])) != 2:
        raise AssertionError("focused mixed update-rate section mapping drifted")
    if len(mixed_update_rate.get("selected_cpp_api_surface_ids", [])) != 15:
        raise AssertionError("focused mixed update-rate API mapping drifted")
    mixed_update_rate_focus = query_rti_work.focused_lane_result(
        index, tests, "update-rate-mixed-attribute-gating", limit=0
    )
    if mixed_update_rate_focus.get("lane_state") != "complete":
        raise AssertionError("focused mixed update-rate lane is not complete")
    if mixed_update_rate_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused mixed update-rate mapped count drifted")
    if mixed_update_rate_focus.get("assertion_count") != 37:
        raise AssertionError("focused mixed update-rate assertion total drifted")
    mixed_update_rate_handles = mixed_update_rate_focus.get("lane_handles")
    if not isinstance(mixed_update_rate_handles, dict) or mixed_update_rate_handles.get(
        "catch2_target"
    ) != "umbra_mixed_update_rate_subscriptions_catch2":
        raise AssertionError("focused mixed update-rate target handle drifted")
    teardown_update_rate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-federation-teardown-preserves-update-rate-history-integration"
        ),
        None,
    )
    if not isinstance(teardown_update_rate, dict):
        raise AssertionError("focused federation-teardown update-rate row is absent")
    if query_rti_work.source_location_text(teardown_update_rate) != (
        "cpp/tests/federation_teardown_update_rate_history_catch2.cpp:158"
    ):
        raise AssertionError("focused federation-teardown update-rate source pointer drifted")
    if teardown_update_rate.get("assertions") != 46:
        raise AssertionError("focused federation-teardown update-rate assertion count drifted")
    if teardown_update_rate.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused federation-teardown update-rate row is not mapped")
    if len(teardown_update_rate.get("lab_requirement_ids", [])) != 9:
        raise AssertionError("focused federation-teardown update-rate requirement mapping drifted")
    if len(teardown_update_rate.get("standard_sections", [])) != 5:
        raise AssertionError("focused federation-teardown update-rate section mapping drifted")
    if len(teardown_update_rate.get("selected_cpp_api_surface_ids", [])) != 15:
        raise AssertionError("focused federation-teardown update-rate API mapping drifted")
    teardown_update_rate_focus = query_rti_work.focused_lane_result(
        index, tests, "update-rate-federation-teardown-isolation", limit=0
    )
    if teardown_update_rate_focus.get("lane_state") != "complete":
        raise AssertionError("focused federation-teardown update-rate lane is not complete")
    if teardown_update_rate_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused federation-teardown update-rate mapped count drifted")
    if teardown_update_rate_focus.get("assertion_count") != 46:
        raise AssertionError("focused federation-teardown update-rate assertion total drifted")
    teardown_update_rate_handles = teardown_update_rate_focus.get("lane_handles")
    if not isinstance(teardown_update_rate_handles, dict) or teardown_update_rate_handles.get(
        "catch2_target"
    ) != "umbra_federation_teardown_update_rate_history_catch2":
        raise AssertionError("focused federation-teardown update-rate target handle drifted")
    restored_default_region_multi = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-restore-live-tso-default-region-attribute-update-multi-recipient-integration"
        ),
        None,
    )
    if not isinstance(restored_default_region_multi, dict):
        raise AssertionError("focused multi-recipient default-region restore row is absent")
    if query_rti_work.source_location_text(restored_default_region_multi) != (
        "cpp/tests/restore_live_tso_default_region_attribute_update_multi_recipient_catch2.cpp:187"
    ):
        raise AssertionError("focused multi-recipient default-region restore source pointer drifted")
    if restored_default_region_multi.get("assertions") != 123:
        raise AssertionError("focused multi-recipient default-region restore assertion count drifted")
    if restored_default_region_multi.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused multi-recipient default-region restore row is not mapped")
    if len(restored_default_region_multi.get("lab_requirement_ids", [])) != 18:
        raise AssertionError("focused multi-recipient default-region restore requirement mapping drifted")
    if len(restored_default_region_multi.get("standard_sections", [])) != 14:
        raise AssertionError("focused multi-recipient default-region restore section mapping drifted")
    if len(restored_default_region_multi.get("selected_cpp_api_surface_ids", [])) != 24:
        raise AssertionError("focused multi-recipient default-region restore API mapping drifted")
    restored_default_region_multi_focus = query_rti_work.focused_lane_result(
        index, tests, "timestamped-default-region-attribute-restore-multi-recipient", limit=0
    )
    if restored_default_region_multi_focus.get("lane_state") != "complete":
        raise AssertionError("focused multi-recipient default-region restore lane is not complete")
    if restored_default_region_multi_focus.get("mapped_test_count") != 2:
        raise AssertionError("focused multi-recipient default-region restore mapped count drifted")
    if restored_default_region_multi_focus.get("assertion_count") != 123:
        raise AssertionError("focused multi-recipient default-region restore lane assertion total drifted")
    restored_default_region_multi_handles = restored_default_region_multi_focus.get("lane_handles")
    if not isinstance(restored_default_region_multi_handles, dict) or restored_default_region_multi_handles.get(
        "catch2_target"
    ) != "umbra_restore_live_tso_default_region_attribute_update_multi_recipient_catch2":
        raise AssertionError("focused multi-recipient default-region restore target handle drifted")
    timed_regional_interaction = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-timed-restore-live-tso-regional-interaction-integration"
        ),
        None,
    )
    if not isinstance(timed_regional_interaction, dict):
        raise AssertionError("focused timed regional-interaction restore row is absent")
    if query_rti_work.source_location_text(timed_regional_interaction) != (
        "cpp/tests/timed_restore_live_tso_regional_interaction_catch2.cpp:5"
    ):
        raise AssertionError("focused timed regional-interaction restore source pointer drifted")
    if timed_regional_interaction.get("assertions") != 55:
        raise AssertionError("focused timed regional-interaction restore assertion count drifted")
    if timed_regional_interaction.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused timed regional-interaction restore row is not mapped")
    if len(timed_regional_interaction.get("lab_requirement_ids", [])) != 18:
        raise AssertionError("focused timed regional-interaction restore requirement mapping drifted")
    if len(timed_regional_interaction.get("standard_sections", [])) != 15:
        raise AssertionError("focused timed regional-interaction restore section mapping drifted")
    if len(timed_regional_interaction.get("selected_cpp_api_surface_ids", [])) != 24:
        raise AssertionError("focused timed regional-interaction restore API mapping drifted")
    timed_regional_interaction_focus = query_rti_work.focused_lane_result(
        index, tests, "timestamped-regional-interaction-timed-restore", limit=0
    )
    if timed_regional_interaction_focus.get("lane_state") != "complete":
        raise AssertionError("focused timed regional-interaction restore lane is not complete")
    if timed_regional_interaction_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused timed regional-interaction restore mapped count drifted")
    if timed_regional_interaction_focus.get("assertion_count") != 55:
        raise AssertionError("focused timed regional-interaction restore lane assertion total drifted")
    timed_regional_interaction_handles = timed_regional_interaction_focus.get("lane_handles")
    if not isinstance(timed_regional_interaction_handles, dict) or timed_regional_interaction_handles.get(
        "catch2_target"
    ) != "umbra_timed_restore_live_tso_regional_interaction_catch2":
        raise AssertionError("focused timed regional-interaction restore target handle drifted")
    regional_interaction_resignation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-timed-live-tso-regional-interaction-source-resignation-integration"
        ),
        None,
    )
    if not isinstance(regional_interaction_resignation, dict):
        raise AssertionError("focused regional-interaction source-resignation row is absent")
    if query_rti_work.source_location_text(regional_interaction_resignation) != (
        "cpp/tests/timed_live_tso_regional_interaction_source_resignation_catch2.cpp:14"
    ):
        raise AssertionError("focused regional-interaction source-resignation source pointer drifted")
    if regional_interaction_resignation.get("assertions") != 60:
        raise AssertionError("focused regional-interaction source-resignation assertion count drifted")
    if regional_interaction_resignation.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused regional-interaction source-resignation row is not mapped")
    if len(regional_interaction_resignation.get("lab_requirement_ids", [])) != 14:
        raise AssertionError("focused regional-interaction source-resignation requirement mapping drifted")
    if len(regional_interaction_resignation.get("standard_sections", [])) != 14:
        raise AssertionError("focused regional-interaction source-resignation section mapping drifted")
    if len(regional_interaction_resignation.get("selected_cpp_api_surface_ids", [])) != 27:
        raise AssertionError("focused regional-interaction source-resignation API mapping drifted")
    regional_interaction_resignation_focus = query_rti_work.focused_lane_result(
        index, tests, "timestamped-regional-interaction-source-resignation", limit=0
    )
    if regional_interaction_resignation_focus.get("lane_state") != "complete":
        raise AssertionError("focused regional-interaction source-resignation lane is not complete")
    if regional_interaction_resignation_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused regional-interaction source-resignation mapped count drifted")
    if regional_interaction_resignation_focus.get("assertion_count") != 60:
        raise AssertionError("focused regional-interaction source-resignation lane assertion total drifted")
    regional_interaction_resignation_handles = regional_interaction_resignation_focus.get("lane_handles")
    if not isinstance(regional_interaction_resignation_handles, dict) or regional_interaction_resignation_handles.get(
        "catch2_target"
    ) != "umbra_timed_live_tso_regional_interaction_source_resignation_catch2":
        raise AssertionError("focused regional-interaction source-resignation target handle drifted")
    regional_interaction_restore_multi = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-restore-live-tso-regional-interaction-multi-recipient-integration"
        ),
        None,
    )
    if not isinstance(regional_interaction_restore_multi, dict):
        raise AssertionError("focused regional-interaction restore multi-recipient row is absent")
    if query_rti_work.source_location_text(regional_interaction_restore_multi) != (
        "cpp/tests/restore_live_tso_regional_interaction_multi_recipient_catch2.cpp:169"
    ):
        raise AssertionError("focused regional-interaction restore multi-recipient source pointer drifted")
    if regional_interaction_restore_multi.get("assertions") != 128:
        raise AssertionError("focused regional-interaction restore multi-recipient assertion count drifted")
    if regional_interaction_restore_multi.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused regional-interaction restore multi-recipient row is not mapped")
    if len(regional_interaction_restore_multi.get("lab_requirement_ids", [])) != 11:
        raise AssertionError("focused regional-interaction restore multi-recipient requirement mapping drifted")
    if len(regional_interaction_restore_multi.get("standard_sections", [])) != 11:
        raise AssertionError("focused regional-interaction restore multi-recipient section mapping drifted")
    if len(regional_interaction_restore_multi.get("selected_cpp_api_surface_ids", [])) != 35:
        raise AssertionError("focused regional-interaction restore multi-recipient API mapping drifted")
    regional_interaction_restore_multi_focus = query_rti_work.focused_lane_result(
        index, tests, "timestamped-regional-interaction-restore-multi-recipient", limit=0
    )
    if regional_interaction_restore_multi_focus.get("lane_state") != "complete":
        raise AssertionError("focused regional-interaction restore multi-recipient lane is not complete")
    if regional_interaction_restore_multi_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused regional-interaction restore multi-recipient mapped count drifted")
    if regional_interaction_restore_multi_focus.get("assertion_count") != 128:
        raise AssertionError("focused regional-interaction restore multi-recipient lane assertion total drifted")
    regional_interaction_restore_multi_handles = regional_interaction_restore_multi_focus.get("lane_handles")
    if not isinstance(regional_interaction_restore_multi_handles, dict) or regional_interaction_restore_multi_handles.get(
        "catch2_target"
    ) != "umbra_restore_live_tso_regional_interaction_multi_recipient_catch2":
        raise AssertionError("focused regional-interaction restore multi-recipient target handle drifted")
    timed_regional_attribute_resignation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-timed-live-tso-regional-attribute-update-source-resignation-after-restore-integration"
        ),
        None,
    )
    if not isinstance(timed_regional_attribute_resignation, dict):
        raise AssertionError("focused timed regional-attribute source-resignation row is absent")
    if query_rti_work.source_location_text(timed_regional_attribute_resignation) != (
        "cpp/tests/timed_live_tso_regional_attribute_update_source_resignation_after_restore_catch2.cpp:180"
    ):
        raise AssertionError("focused timed regional-attribute source-resignation source pointer drifted")
    if timed_regional_attribute_resignation.get("assertions") != 106:
        raise AssertionError("focused timed regional-attribute source-resignation assertion count drifted")
    if timed_regional_attribute_resignation.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused timed regional-attribute source-resignation row is not mapped")
    if len(timed_regional_attribute_resignation.get("lab_requirement_ids", [])) != 27:
        raise AssertionError("focused timed regional-attribute source-resignation requirement mapping drifted")
    if len(timed_regional_attribute_resignation.get("standard_sections", [])) != 18:
        raise AssertionError("focused timed regional-attribute source-resignation section mapping drifted")
    if len(timed_regional_attribute_resignation.get("selected_cpp_api_surface_ids", [])) != 23:
        raise AssertionError("focused timed regional-attribute source-resignation API mapping drifted")
    timed_regional_attribute_resignation_focus = query_rti_work.focused_lane_result(
        index, tests, "tso-regional-attribute-update-timed-live-resignation-state", limit=0
    )
    if timed_regional_attribute_resignation_focus.get("lane_state") != "complete":
        raise AssertionError("focused timed regional-attribute source-resignation lane is not complete")
    if timed_regional_attribute_resignation_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused timed regional-attribute source-resignation mapped count drifted")
    if timed_regional_attribute_resignation_focus.get("assertion_count") != 106:
        raise AssertionError("focused timed regional-attribute source-resignation lane assertion total drifted")
    timed_regional_attribute_resignation_handles = timed_regional_attribute_resignation_focus.get("lane_handles")
    if not isinstance(timed_regional_attribute_resignation_handles, dict) or timed_regional_attribute_resignation_handles.get(
        "catch2_target"
    ) != "umbra_timed_regional_attr_source_resign_restore_catch2":
        raise AssertionError("focused timed regional-attribute source-resignation target handle drifted")
    timed_regional_attribute_restore = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-timed-restore-live-tso-explicit-regional-attribute-update-integration"
        ),
        None,
    )
    if not isinstance(timed_regional_attribute_restore, dict):
        raise AssertionError("focused timed regional-attribute restore row is absent")
    if query_rti_work.source_location_text(timed_regional_attribute_restore) != (
        "cpp/tests/timed_restore_live_tso_regional_attribute_update_catch2.cpp:190"
    ):
        raise AssertionError("focused timed regional-attribute restore source pointer drifted")
    if timed_regional_attribute_restore.get("assertions") != 84:
        raise AssertionError("focused timed regional-attribute restore assertion count drifted")
    if timed_regional_attribute_restore.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused timed regional-attribute restore row is not mapped")
    if len(timed_regional_attribute_restore.get("lab_requirement_ids", [])) != 23:
        raise AssertionError("focused timed regional-attribute restore requirement mapping drifted")
    if len(timed_regional_attribute_restore.get("standard_sections", [])) != 15:
        raise AssertionError("focused timed regional-attribute restore section mapping drifted")
    if len(timed_regional_attribute_restore.get("selected_cpp_api_surface_ids", [])) != 23:
        raise AssertionError("focused timed regional-attribute restore API mapping drifted")
    timed_regional_attribute_restore_focus = query_rti_work.focused_lane_result(
        index, tests, "tso-regional-attribute-update-timed-live-restore-state", limit=0
    )
    if timed_regional_attribute_restore_focus.get("lane_state") != "complete":
        raise AssertionError("focused timed regional-attribute restore lane is not complete")
    if timed_regional_attribute_restore_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused timed regional-attribute restore mapped count drifted")
    if timed_regional_attribute_restore_focus.get("assertion_count") != 84:
        raise AssertionError("focused timed regional-attribute restore lane assertion total drifted")
    timed_regional_attribute_restore_handles = timed_regional_attribute_restore_focus.get("lane_handles")
    if not isinstance(timed_regional_attribute_restore_handles, dict) or timed_regional_attribute_restore_handles.get(
        "catch2_target"
    ) != "umbra_timed_restore_regional_attr_catch2":
        raise AssertionError("focused timed regional-attribute restore target handle drifted")
    timestamped_regional_attribute_resignation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-timestamped-regional-attribute-update-resignation-integration"
        ),
        None,
    )
    if not isinstance(timestamped_regional_attribute_resignation, dict):
        raise AssertionError("focused timestamped regional-attribute resignation row is absent")
    if query_rti_work.source_location_text(timestamped_regional_attribute_resignation) != (
        "cpp/tests/timestamped_regional_attribute_update_resignation_catch2.cpp:146"
    ):
        raise AssertionError("focused timestamped regional-attribute resignation source pointer drifted")
    if timestamped_regional_attribute_resignation.get("assertions") != 58:
        raise AssertionError("focused timestamped regional-attribute resignation assertion count drifted")
    if timestamped_regional_attribute_resignation.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused timestamped regional-attribute resignation row is not mapped")
    if len(timestamped_regional_attribute_resignation.get("lab_requirement_ids", [])) != 8:
        raise AssertionError("focused timestamped regional-attribute resignation requirement mapping drifted")
    if len(timestamped_regional_attribute_resignation.get("standard_sections", [])) != 6:
        raise AssertionError("focused timestamped regional-attribute resignation section mapping drifted")
    if len(timestamped_regional_attribute_resignation.get("selected_cpp_api_surface_ids", [])) != 14:
        raise AssertionError("focused timestamped regional-attribute resignation API mapping drifted")
    timestamped_regional_attribute_resignation_focus = query_rti_work.focused_lane_result(
        index, tests, "timestamped-regional-attribute-update-resignation", limit=0
    )
    if timestamped_regional_attribute_resignation_focus.get("lane_state") != "complete":
        raise AssertionError("focused timestamped regional-attribute resignation lane is not complete")
    if timestamped_regional_attribute_resignation_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused timestamped regional-attribute resignation mapped count drifted")
    if timestamped_regional_attribute_resignation_focus.get("assertion_count") != 58:
        raise AssertionError("focused timestamped regional-attribute resignation lane assertion total drifted")
    timestamped_regional_attribute_resignation_handles = timestamped_regional_attribute_resignation_focus.get(
        "lane_handles"
    )
    if not isinstance(timestamped_regional_attribute_resignation_handles, dict) or timestamped_regional_attribute_resignation_handles.get(
        "catch2_target"
    ) != "umbra_timestamped_regional_attribute_update_resignation_catch2":
        raise AssertionError("focused timestamped regional-attribute resignation target handle drifted")
    registration_discovery = next(
        (
            test
            for test in tests
            if test.get("test_case")
            == "Embedded object-instance registration/discovery honors 2025 publication, promotion, and callback lifecycle"
        ),
        None,
    )
    if not isinstance(registration_discovery, dict):
        raise AssertionError("focused object-instance registration/discovery plan row is absent")
    if query_rti_work.source_location_text(registration_discovery) != (
        "cpp/tests/object_instance_registration_discovery_catch2.cpp:78"
    ):
        raise AssertionError("focused object-instance registration/discovery source pointer drifted")
    if registration_discovery.get("assertions") != 81:
        raise AssertionError("focused object-instance registration/discovery assertion count drifted")
    registration_discovery_focus = query_rti_work.focused_lane_result(
        index, tests, "object-instance-registration-discovery", limit=0
    )
    if registration_discovery_focus.get("lane_state") != "complete":
        raise AssertionError("focused object-instance registration/discovery lane is not complete")
    if registration_discovery_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused object-instance registration/discovery mapped count drifted")
    if registration_discovery_focus.get("assertion_count") != 81:
        raise AssertionError("focused object-instance registration/discovery lane assertion total drifted")
    registration_discovery_handles = registration_discovery_focus.get("lane_handles")
    if not isinstance(registration_discovery_handles, dict) or registration_discovery_handles.get(
        "catch2_target"
    ) != "umbra_object_instance_registration_discovery_catch2":
        raise AssertionError("focused object-instance registration/discovery target handle drifted")
    transportation_stability = next(
        (
            test
            for test in tests
            if test.get("test_case")
            == "Embedded custom transportation handles remain stable across an additional FOM join"
        ),
        None,
    )
    if not isinstance(transportation_stability, dict):
        raise AssertionError("focused transportation-handle stability plan row is absent")
    if query_rti_work.source_location_text(transportation_stability) != (
        "cpp/tests/custom_transportation_handle_stability_catch2.cpp:44"
    ):
        raise AssertionError("focused transportation-handle stability source pointer drifted")
    if transportation_stability.get("assertions") != 21:
        raise AssertionError("focused transportation-handle stability assertion count drifted")
    transportation_stability_focus = query_rti_work.focused_lane_result(
        index, tests, "transportation-handle-stability", limit=0
    )
    if transportation_stability_focus.get("lane_state") != "complete":
        raise AssertionError("focused transportation-handle stability lane is not complete")
    if transportation_stability_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused transportation-handle stability mapped count drifted")
    if transportation_stability_focus.get("assertion_count") != 21:
        raise AssertionError("focused transportation-handle stability lane assertion total drifted")
    transportation_stability_handles = transportation_stability_focus.get("lane_handles")
    if not isinstance(transportation_stability_handles, dict) or transportation_stability_handles.get(
        "catch2_target"
    ) != "umbra_custom_transportation_handle_stability_catch2":
        raise AssertionError("focused transportation-handle stability target handle drifted")
    current_fdd = next(
        (
            test
            for test in tests
            if test.get("test_case")
            == "Embedded federation MOM exposes and refreshes HLAcurrentFDD"
        ),
        None,
    )
    if not isinstance(current_fdd, dict):
        raise AssertionError("focused HLAcurrentFDD plan row is absent")
    current_fdd_source = query_rti_work.source_location_text(current_fdd)
    if not current_fdd_source.startswith(
        "cpp/tests/federation_mom_current_fdd_catch2.cpp:"
    ):
        raise AssertionError("focused HLAcurrentFDD source file pointer drifted")
    if current_fdd.get("assertions") != 60:
        raise AssertionError("focused HLAcurrentFDD assertion count drifted")
    current_fdd_focus = query_rti_work.focused_lane_result(
        index, tests, "federation-mom-current-fdd", limit=0
    )
    if current_fdd_focus.get("lane_state") != "complete":
        raise AssertionError("focused HLAcurrentFDD lane is not complete")
    if current_fdd_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused HLAcurrentFDD mapped count drifted")
    if current_fdd_focus.get("assertion_count") != 60:
        raise AssertionError("focused HLAcurrentFDD lane assertion total drifted")
    if current_fdd_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused HLAcurrentFDD direct pair count drifted")
    current_fdd_handles = current_fdd_focus.get("lane_handles")
    if not isinstance(current_fdd_handles, dict) or current_fdd_handles.get(
        "catch2_target"
    ) != "umbra_federation_mom_current_fdd_catch2":
        raise AssertionError("focused HLAcurrentFDD Catch2 handle drifted")
    content_reports = next(
        (
            test
            for test in tests
            if test.get("test_case")
            == "Embedded federation MOM content reports FOM module and MIM data through a focused lane"
        ),
        None,
    )
    if not isinstance(content_reports, dict):
        raise AssertionError("focused federation MOM content-report plan row is absent")
    if content_reports.get("assertions") != 55:
        raise AssertionError("focused federation MOM content-report assertion count drifted")
    content_reports_source = query_rti_work.source_location_text(content_reports)
    if not content_reports_source.startswith(
        "cpp/tests/federation_mom_current_fdd_catch2.cpp:"
    ):
        raise AssertionError("focused federation MOM content-report source pointer drifted")
    content_reports_focus = query_rti_work.focused_lane_result(
        index, tests, "federation-mom-content-reports", limit=0
    )
    if content_reports_focus.get("lane_state") != "complete":
        raise AssertionError("focused federation MOM content-report lane is not complete")
    if content_reports_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused federation MOM content-report mapped count drifted")
    if content_reports_focus.get("assertion_count") != 55:
        raise AssertionError("focused federation MOM content-report lane assertion total drifted")
    if content_reports_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused federation MOM content-report direct pair count drifted")
    content_reports_handles = content_reports_focus.get("lane_handles")
    if not isinstance(content_reports_handles, dict) or content_reports_handles.get(
        "catch2_target"
    ) != "umbra_federation_mom_current_fdd_catch2":
        raise AssertionError("focused federation MOM content-report Catch2 handle drifted")
    time_item = next(
        item
        for item in status["roadmap_items"]
        if item.get("id") == "time-save-restore"
    )
    pointer = time_item.get("next_test_pointer")
    if not isinstance(pointer, dict) or pointer.get("state") != "complete":
        raise AssertionError("active baseline pointer is not classified as complete")
    expected = next(
        test
        for test in tests
        if test.get("test_case") == pointer.get("query")
    )
    if pointer.get("requirement_count") != len(expected.get("lab_requirement_ids", [])):
        raise AssertionError("baseline pointer requirement count diverges from its test")
    if pointer.get("standard_section_count") != len(expected.get("standard_sections", [])):
        raise AssertionError("baseline pointer section count diverges from its test")
    if pointer.get("cpp_api_surface_count") != len(
        expected.get("selected_cpp_api_surface_ids", [])
    ):
        raise AssertionError("baseline pointer API count diverges from its test")
    queue_time_item = next(
        row
        for row in queue["items"]
        if row.get("id") == "time-save-restore"
    )
    queue_pointer = queue_time_item.get("next_test_pointer")
    if queue_pointer != query_rti_work.next_test_pointer(
        time_item, tests
    ):
        raise AssertionError("queue baseline pointer diverges from status pointer")

    # Family status must keep intentional dispositions separate from rows that
    # still need a Lab mapping decision.  This is the compact, plan-derived
    # handoff consumed by ``status --summary``; it must not rely on stale
    # narrative counters in the roadmap index.
    object_item = next(
        item
        for item in status["roadmap_items"]
        if item.get("id") == "object-ddm-ownership"
    )
    object_tests = query_rti_work.item_tests(object_item, tests)
    object_counts = object_item.get("live_mapping_counts")
    if not isinstance(object_counts, dict):
        raise AssertionError("status family row omitted live mapping counts")
    expected_explicit = sum(
        not test.get("requirements")
        and query_rti_work.traceability_state(test) == "explicit-disposition"
        for test in object_tests
    )
    expected_unclassified = sum(
        not test.get("requirements")
        and query_rti_work.traceability_state(test) == "unclassified"
        for test in object_tests
    )
    if object_counts.get("explicit_disposition_count") != expected_explicit:
        raise AssertionError("status family explicit-disposition count diverges from plan")
    if object_counts.get("unclassified_count") != expected_unclassified:
        raise AssertionError("status family unclassified count diverges from plan")
    object_pairs = {
        (row.get("lab_requirement_id"), row.get("standard_section"))
        for test in object_tests
        for row in query_rti_work.requirement_section_mapping_rows(test)
        if isinstance(row, dict) and row.get("lab_requirement_id")
    }
    if object_counts.get("requirement_section_pair_count") != len(object_pairs):
        raise AssertionError("status family direct pair count diverges from plan")
    if object_counts.get("resolved_requirement_section_pair_count") != sum(
        pair[1] is not None for pair in object_pairs
    ):
        raise AssertionError("status family resolved pair count diverges from plan")
    focus_text = object_item.get("live_current_focus") or ""
    if "explicit dispositions" not in focus_text or "unclassified" not in focus_text:
        raise AssertionError("status family focus text collapsed mapping dispositions")

    ready = query_rti_work.ready_slice(index, tests, source_locations)
    if ready.get("state") == "unplanned-source":
        raise AssertionError(
            "default ready handoff promoted an unrelated source-only declaration"
        )
    if not ready.get("found"):
        options = ready.get("family_options")
        if not isinstance(options, list) or not options:
            raise AssertionError(
                "an exhausted bounded queue must expose at least one open family option"
            )
        if not all(
            isinstance(option, dict) and option.get("work_query")
            for option in options
        ):
            raise AssertionError(
                "bounded family options must carry copyable work queries"
            )
        if not all(
            isinstance(option, dict) and option.get("next_action")
            for option in options
        ):
            raise AssertionError(
                "bounded family options must carry an actionable next step"
            )
        if not all(
            isinstance(option, dict) and option.get("lane_discovery_command")
            for option in options
        ):
            raise AssertionError(
                "bounded family options must carry a lane-discovery command"
            )
        if not all(
            isinstance(option, dict)
            and option.get("mapping_lane_discovery_command")
            and "--disposition unclassified" in option["mapping_lane_discovery_command"]
            for option in options
        ):
            raise AssertionError(
                "bounded family options must carry an unclassified mapping command"
            )
        if not all(
            isinstance(option, dict)
            and option.get("gap_query")
            and "query_rti_work.py gaps --family" in option["gap_query"]
            for option in options
        ):
            raise AssertionError(
                "bounded family options must carry an exact family gap query"
            )

    # Once the selected new case is indexed, the explicit handoff disappears
    # and the bounded family queue becomes the source of truth. This keeps the
    # completed slice queryable without leaving a stale proposal in ``ready``.
    transport_ready = query_rti_work.ready_slice(
        index,
        tests,
        source_locations,
        requested_family="transport-and-conformance",
        standard_requirements=standard_requirements,
    )
    if (
        transport_ready.get("found") is not False
        or transport_ready.get("state") != "none"
        or transport_ready.get("requested_family") != "transport-and-conformance"
        or transport_ready.get("reason")
        != "the indexed source and planned-row queues are exhausted; only explicitly queued family actions remain"
    ):
        raise AssertionError("completed process TSO handoff was not retired")

    active_roadmap = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process-tso-attribute-retraction-reenable",
        status="open",
        limit=4,
    )
    active_families = active_roadmap.get("families")
    if not isinstance(active_families, list) or len(active_families) != 1:
        raise AssertionError("active handoff reverse roadmap lookup drifted")
    lane_matches = active_families[0].get("lane_matches")
    if not isinstance(lane_matches, list) or not any(
        isinstance(match, dict)
        and match.get("tag") == "process-tso-attribute-retraction-reenable"
        and match.get("representative_test_id")
        == "umbra-cpp-process-tso-attribute-retraction-reenable-integration"
        and match.get("requirement_section_pair_count") == 5
        for match in lane_matches
    ):
        raise AssertionError("completed process TSO lane reverse lookup omitted its mapping")

    transport_option = {
        "gap_preview": query_rti_work.family_gap_preview(
            index,
            tests,
            standard_requirements,
            "transport-and-conformance",
        )
    }
    transport_gap = transport_option.get("gap_preview")
    transport_requirement = (
        transport_gap.get("requirement")
        if isinstance(transport_gap, dict)
        else None
    )
    if not isinstance(transport_requirement, dict):
        raise AssertionError("new-case family handoff omitted its gap preview")
    if (
        transport_requirement.get("id") != "summary-rule-1"
        or transport_requirement.get("standard_section")
        != "hla-1516-2025:clause-4"
        or "--summary --compact"
        not in str(transport_requirement.get("requirement_query") or "")
        or "--summary --compact"
        not in str(transport_requirement.get("section_query") or "")
    ):
        raise AssertionError(
            "new-case family gap preview omitted exact requirement/section handles"
        )
    transport_gap_text = "\n".join(
        query_rti_work.text_gap_preview(transport_option)
        if isinstance(transport_option, dict)
        else []
    )
    if (
        "gap_scope=604/2220 covered (27.21%); 1616 uncovered" not in transport_gap_text
        or "gap_head=summary-rule-1 -> hla-1516-2025:clause-4" not in transport_gap_text
        or "gap_requirement=python tools/query_rti_work.py requirement summary-rule-1" not in transport_gap_text
        or "gap_section=python tools/query_rti_work.py section hla-1516-2025:clause-4" not in transport_gap_text
    ):
        raise AssertionError("new-case family text omitted its bounded gap handles")

    source_only_ready = query_rti_work.ready_slice(
        index,
        tests,
        source_locations,
        include_source_only=True,
    )
    if source_only_ready.get("state") == "unplanned-source":
        if not source_only_ready.get("source_location"):
            raise AssertionError(
                "explicit source-only handoff omitted its source location"
            )
    elif source_only_ready.get("state") not in {"none", "planned", "new-case-needed"}:
        raise AssertionError(
            "explicit source-only opt-in returned an unexpected queue state"
        )

    scoped_ready = query_rti_work.ready_slice(
        index,
        tests,
        source_locations,
        "object-ddm-ownership",
    )
    if scoped_ready.get("found"):
        if scoped_ready.get("requested_family") != "object-ddm-ownership":
            raise AssertionError(
                "ready --family returned a handoff for a different family"
            )
        owner = scoped_ready.get("owner")
        roadmap_owner = scoped_ready.get("roadmap_owner")
        if not isinstance(owner, dict) or owner.get("id") != "object-ddm-ownership":
            raise AssertionError(
                "ready --family did not retain the selected family as owner"
            )
        if not isinstance(roadmap_owner, dict) or roadmap_owner.get("id") != "object-ddm-ownership":
            raise AssertionError(
                "ready --family returned a different roadmap owner"
            )
        if not scoped_ready.get("next_action"):
            raise AssertionError("selected family handoff omitted its next action")
        if scoped_ready.get("state") == "mapping":
            for field in (
                "plan_id",
                "source_location",
                "trace_command",
                "focus_command",
                "check_command",
            ):
                if not scoped_ready.get(field):
                    raise AssertionError(
                        f"mapping handoff omitted its {field} handle"
                    )
        if scoped_ready.get("state") == "planned":
            if (
                scoped_ready.get("plan_id")
                != "umbra-cpp-confirm-divestiture-process-assumption-integration"
                or scoped_ready.get("source_lane")
                != "process-confirm-divestiture-assumption"
                or not scoped_ready.get("ctest_filter")
                or len(scoped_ready.get("requirement_ids", [])) != 1
                or len(scoped_ready.get("standard_sections", [])) != 1
            ):
                raise AssertionError("planned family handoff omitted exact mapping handles")
    else:
        scoped_options = scoped_ready.get("family_options")
        if not isinstance(scoped_options, list) or [
            option.get("id")
            for option in scoped_options
            if isinstance(option, dict)
        ] != ["object-ddm-ownership"]:
            raise AssertionError(
                "ready --family did not keep an exhausted-family response scoped"
            )
        if not scoped_options[0].get("next_action"):
            raise AssertionError("selected family response omitted its next action")
        if not scoped_options[0].get("gap_query"):
            raise AssertionError("selected family response omitted its gap query")

    object_work = query_rti_work.indexed_work_slice(
        index, tests, "object-ddm-ownership", source_locations
    )
    object_work_commands = object_work.get("commands", [])
    if not any(
        isinstance(command, str)
        and "lanes --family object-ddm-ownership" in command
        for command in object_work_commands
    ):
        raise AssertionError(
            "family work query omitted its bounded lane-discovery command"
        )
    if not any(
        isinstance(command, str)
        and "ready --family object-ddm-ownership" in command
        for command in object_work_commands
    ):
        raise AssertionError(
            "family work query omitted its scoped ready command"
        )

    # An explicitly requested family with its own current action must not
    # inherit the stale pointer text/counts of a broader parent that happens
    # to reference it through ``next_work_id``.
    transport_work = query_rti_work.indexed_work_slice(
        index, tests, "transport-and-conformance", source_locations
    )
    transport_parent = transport_work.get("parent_item")
    transport_target = transport_work.get("work_item")
    if (
        not isinstance(transport_parent, dict)
        or transport_parent.get("id") != "transport-and-conformance"
        or not isinstance(transport_target, dict)
        or transport_target.get("id") != "transport-and-conformance"
        or "public process transport baseline" not in str(transport_work.get("task") or "")
    ):
        raise AssertionError(
            "explicit transport family work query inherited a stale parent pointer"
        )

    api_kind, api_matches = query_rti_work.resolve_trace_query(
        tests, "api.2025.cpp.rtiambassador.querylogicaltime.cb29c063787c"
    )
    if api_kind != "api" or len(api_matches) < 2:
        raise AssertionError(
            "exact C++ API surface lookup did not resolve the Query Logical Time rows"
        )

    family_item, family_tests = query_rti_work.indexed_item(
        index, tests, "object-ddm-ownership"
    )
    if family_item is None or not family_tests:
        raise AssertionError("indexed roadmap family did not resolve to tagged tests")
    family_matrix_kind, family_matrix_matches = query_rti_work.resolve_matrix_query(
        index, tests, "OBJECT-DDM-OWNERSHIP"
    )
    if family_matrix_kind != "family":
        raise AssertionError("matrix did not resolve an indexed roadmap family id")
    if family_matrix_matches != family_tests:
        raise AssertionError("family matrix resolution diverged from indexed item tests")
    if not any(test.get("test_case") == TARGET_TEST for test in family_matrix_matches):
        raise AssertionError("target test is not reachable through its roadmap family")
    scoped_family, scoped_rows = query_rti_work.scoped_plan_tests(
        index, tests, family="object-ddm-ownership"
    )
    if scoped_family != family_item or scoped_rows != family_tests:
        raise AssertionError("family scope resolution diverged from item/matrix rows")

    # Roadmap discovery is intentionally a separate bounded index.  Keep one
    # thematic query protected so a contributor can find the process family
    # and its exact work/focus/matrix handles without enumerating all lanes or
    # reopening the unchanged Requirements Lab.
    roadmap_process = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process",
        status="open",
        limit=0,
    )
    if roadmap_process.get("count") != len(roadmap_process.get("families", [])):
        raise AssertionError("roadmap search count diverges from its unbounded rows")
    process_family_row = next(
        (
            row
            for row in roadmap_process.get("families", [])
            if isinstance(row, dict) and row.get("id") == "transport-and-conformance"
        ),
        None,
    )
    if not isinstance(process_family_row, dict):
        raise AssertionError("roadmap process search lost the transport family")
    if process_family_row.get("next_lane") != "process-boundary":
        raise AssertionError("roadmap process row lost its exact next lane")
    if process_family_row.get("action_state") != "new-case-needed":
        raise AssertionError(
            "roadmap process row lost its deliberate-new-case action state"
        )
    process_commands = process_family_row.get("commands")
    if not isinstance(process_commands, dict) or not all(
        process_commands.get(name) for name in ("work", "matrix")
    ):
        raise AssertionError("roadmap process row lost work/matrix handles")
    process_counts = process_family_row.get("live_mapping_counts")
    if not isinstance(process_counts, dict) or not process_counts.get(
        "requirement_count"
    ):
        raise AssertionError("roadmap process row lost live requirement counts")

    callback_roadmap = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="multi-federate-callback-ordering",
        status="open",
        limit=0,
    )
    callback_rows = callback_roadmap.get("families", [])
    if callback_roadmap.get("count") != 1 or len(callback_rows) != 1:
        raise AssertionError(
            "roadmap callback family query lost its exact family handle"
        )
    if callback_rows[0].get("action_state") != "evidence-complete":
        raise AssertionError(
            "roadmap callback family was exposed as a runnable action"
        )

    constrained_roadmap = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process-time-advance-time-constrained-pending",
        status="open",
        limit=0,
    )
    constrained_rows = constrained_roadmap.get("families", [])
    if constrained_roadmap.get("count") != 1 or len(constrained_rows) != 1:
        raise AssertionError(
            "roadmap exact constrained-pending lane query lost its family handle"
        )
    if constrained_rows[0].get("id") != "transport-and-conformance":
        raise AssertionError(
            "roadmap constrained-pending lane resolved to the wrong owner"
        )

    malformed_roadmap = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process-time-advance-malformed-encoding",
        status="open",
        limit=0,
    )
    malformed_rows = malformed_roadmap.get("families", [])
    if malformed_roadmap.get("count") != 1 or len(malformed_rows) != 1:
        raise AssertionError(
            "roadmap exact malformed-encoding lane query lost its family handle"
        )
    if malformed_rows[0].get("id") != "transport-and-conformance":
        raise AssertionError(
            "roadmap malformed-encoding lane resolved to the wrong owner"
        )

    scheduler_roadmap = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process-time-advance-federation-scheduler",
        status="open",
        limit=0,
    )
    scheduler_rows = scheduler_roadmap.get("families", [])
    if scheduler_roadmap.get("count") != 1 or len(scheduler_rows) != 1:
        raise AssertionError(
            "roadmap exact federation-scheduler query lost its family handle"
        )
    if scheduler_rows[0].get("id") != "transport-and-conformance":
        raise AssertionError(
            "roadmap federation-scheduler query resolved to the wrong owner"
        )

    scheduler_focus = query_rti_work.focused_lane_result(
        index, tests, "process-time-advance-federation-scheduler", limit=1
    )
    if scheduler_focus.get("lane_state") != "complete":
        raise AssertionError("process federation-scheduler lane is not complete")
    if scheduler_focus.get("mapped_test_count") != 1:
        raise AssertionError("process federation-scheduler mapped count drifted")
    if scheduler_focus.get("assertion_count") != 30:
        raise AssertionError("process federation-scheduler assertion total drifted")
    if scheduler_focus.get("requirement_section_pair_count") != 4:
        raise AssertionError("process federation-scheduler mapping pair count drifted")
    if set(scheduler_focus.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-8.2",
        "hla-1516.1-2025:clause-8.5.5",
        "hla-1516.1-2025:clause-8.6.3",
        "hla-1516.1-2025:clause-8.8.3",
    }:
        raise AssertionError("process federation-scheduler section preview drifted")
    scheduler_handles = scheduler_focus.get("lane_handles")
    if not isinstance(scheduler_handles, dict) or scheduler_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("process federation-scheduler Catch2 handle drifted")

    tso_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-interaction-before-grant", limit=1
    )
    if tso_focus.get("lane_state") != "complete":
        raise AssertionError("process TSO interaction lane is not complete")
    if tso_focus.get("mapped_test_count") != 1:
        raise AssertionError("process TSO interaction mapped count drifted")
    if tso_focus.get("assertion_count") != 62:
        raise AssertionError("process TSO interaction assertion total drifted")
    if tso_focus.get("requirement_section_pair_count") != 20:
        raise AssertionError("process TSO interaction mapping pair count drifted")
    if set(tso_focus.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-4",
        "hla-1516.1-2025:clause-5.1.4",
        "hla-1516.1-2025:clause-6.12.4",
        "hla-1516.1-2025:clause-6.13",
        "hla-1516.1-2025:clause-8",
        "hla-1516.1-2025:clause-8.2",
        "hla-1516.1-2025:clause-8.5.5",
        "hla-1516.1-2025:clause-8.6.3",
        "hla-1516.1-2025:clause-8.8.3",
        "hla-1516.1-2025:clause-8.1.5",
        "hla-1516.1-2025:clause-8.18.1",
        "hla-1516.1-2025:clause-8.19.3",
        "hla-1516.1-2025:clause-10.60.6",
    }:
        raise AssertionError("process TSO interaction section preview drifted")
    tso_handles = tso_focus.get("lane_handles")
    if not isinstance(tso_handles, dict) or tso_handles.get(
        "catch2_target"
    ) != "umbra_ieee1516_2025_connection_catch2":
        raise AssertionError("process TSO interaction Catch2 handle drifted")

    tso_in_transit_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-in-transit", limit=1
    )
    if (
        tso_in_transit_focus.get("lane_state") != "complete"
        or tso_in_transit_focus.get("roadmap_owner") != "transport-and-conformance"
        or tso_in_transit_focus.get("mapped_test_count") != 1
        or tso_in_transit_focus.get("assertion_count") != 62
        or tso_in_transit_focus.get("requirement_section_pair_count") != 20
        or tso_in_transit_focus.get("standard_section_count") != 13
    ):
        raise AssertionError("process TSO in-transit lane card drifted")
    tso_in_transit_handles = tso_in_transit_focus.get("lane_handles")
    if (
        not isinstance(tso_in_transit_handles, dict)
        or tso_in_transit_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_in_transit_handles.get("ctest_label") != "process-boundary"
    ):
        raise AssertionError("process TSO in-transit Catch2 handle drifted")

    tso_attribute_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-attribute-before-grant", limit=1
    )
    if tso_attribute_focus.get("lane_state") != "complete":
        raise AssertionError("process TSO attribute lane is not complete")
    if tso_attribute_focus.get("roadmap_owner") != "transport-and-conformance":
        raise AssertionError("process TSO attribute lane owner drifted")
    if tso_attribute_focus.get("mapped_test_count") != 2:
        raise AssertionError("process TSO attribute mapped count drifted")
    if tso_attribute_focus.get("assertion_count") != 114:
        raise AssertionError("process TSO attribute assertion total drifted")
    if tso_attribute_focus.get("requirement_section_pair_count") != 17:
        raise AssertionError("process TSO attribute mapping pair count drifted")
    if set(tso_attribute_focus.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-5.8",
        "hla-1516.1-2025:clause-6.10",
        "hla-1516.1-2025:clause-6.8.4",
        "hla-1516.1-2025:clause-6.9.3",
        "hla-1516.1-2025:clause-8.2",
        "hla-1516.1-2025:clause-8.5.5",
        "hla-1516.1-2025:clause-8.6.3",
        "hla-1516.1-2025:clause-8.8.3",
        "hla-1516.1-2025:clause-8",
        "hla-1516.1-2025:clause-8.1.5",
        "hla-1516.1-2025:clause-8.18.1",
        "hla-1516.1-2025:clause-8.19.3",
    }:
        raise AssertionError("process TSO attribute section preview drifted")
    tso_attribute_handles = tso_attribute_focus.get("lane_handles")
    if not isinstance(tso_attribute_handles, dict) or tso_attribute_handles.get(
        "catch2_targets"
    ) != [
        "umbra_process_boundary_private_catch2",
        "umbra_ieee1516_2025_connection_catch2",
    ]:
        raise AssertionError("process TSO attribute Catch2 handles drifted")

    tso_attribute_retraction_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-attribute-retraction-before-callback", limit=1
    )
    if (
        tso_attribute_retraction_focus.get("lane_state") != "complete"
        or tso_attribute_retraction_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_attribute_retraction_focus.get("mapped_test_count") != 1
        or tso_attribute_retraction_focus.get("assertion_count") != 51
        or tso_attribute_retraction_focus.get("recorded_assertion_count") != 51
        or tso_attribute_retraction_focus.get("requirement_section_pair_count") != 20
        or tso_attribute_retraction_focus.get("standard_section_count") != 13
    ):
        raise AssertionError("process TSO attribute retraction lane card drifted")
    if set(tso_attribute_retraction_focus.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-5.8",
        "hla-1516.1-2025:clause-6.10",
        "hla-1516.1-2025:clause-6.8.4",
        "hla-1516.1-2025:clause-6.9.3",
        "hla-1516.1-2025:clause-8",
        "hla-1516.1-2025:clause-8.1.5",
        "hla-1516.1-2025:clause-8.18.1",
        "hla-1516.1-2025:clause-8.19.3",
        "hla-1516.1-2025:clause-8.2",
        "hla-1516.1-2025:clause-8.22.3",
        "hla-1516.1-2025:clause-8.5.5",
        "hla-1516.1-2025:clause-8.6.3",
        "hla-1516.1-2025:clause-8.8.3",
    }:
        raise AssertionError("process TSO attribute retraction section preview drifted")
    tso_attribute_retraction_handles = tso_attribute_retraction_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(tso_attribute_retraction_handles, dict)
        or tso_attribute_retraction_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_attribute_retraction_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors suppress a retracted timestamped process attribute before the callback"
        not in tso_attribute_retraction_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO attribute retraction handle drifted")

    tso_attribute_fanout_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-attribute-retraction-fanout", limit=1
    )
    if (
        tso_attribute_fanout_focus.get("lane_state") != "complete"
        or tso_attribute_fanout_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_attribute_fanout_focus.get("mapped_test_count") != 1
        or tso_attribute_fanout_focus.get("assertion_count") != 100
        or tso_attribute_fanout_focus.get("recorded_assertion_count") != 100
        or tso_attribute_fanout_focus.get("requirement_section_pair_count") != 20
        or tso_attribute_fanout_focus.get("standard_section_count") != 13
    ):
        raise AssertionError("process TSO attribute fanout lane card drifted")
    tso_attribute_fanout_handles = tso_attribute_fanout_focus.get("lane_handles")
    if (
        not isinstance(tso_attribute_fanout_handles, dict)
        or tso_attribute_fanout_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_attribute_fanout_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors fan out retained timestamped process attributes around a retracted middle update"
        not in tso_attribute_fanout_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO attribute fanout handle drifted")

    tso_attribute_regional = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-attribute-retraction-regional-integration"
        ),
        None,
    )
    if not isinstance(tso_attribute_regional, dict):
        raise AssertionError("process TSO attribute regional plan row is absent")
    if query_rti_work.source_location_text(tso_attribute_regional) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:21443"
    ):
        raise AssertionError("process TSO attribute regional source pointer drifted")
    if (
        tso_attribute_regional.get("assertions") != 122
        or tso_attribute_regional.get("primary_lane")
        != "process-tso-attribute-retraction-regional"
        or tso_attribute_regional.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("process TSO attribute regional plan evidence drifted")
    tso_attribute_regional_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-attribute-retraction-regional", limit=0
    )
    if (
        tso_attribute_regional_focus.get("lane_state") != "complete"
        or tso_attribute_regional_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_attribute_regional_focus.get("mapped_test_count") != 1
        or tso_attribute_regional_focus.get("assertion_count") != 122
        or tso_attribute_regional_focus.get("recorded_assertion_count") != 122
        or tso_attribute_regional_focus.get("requirement_count") != 25
        or tso_attribute_regional_focus.get("standard_section_count") != 15
        or tso_attribute_regional_focus.get("requirement_section_pair_count") != 25
    ):
        raise AssertionError("process TSO attribute regional lane card drifted")
    tso_attribute_regional_handles = tso_attribute_regional_focus.get("lane_handles")
    if (
        not isinstance(tso_attribute_regional_handles, dict)
        or tso_attribute_regional_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_attribute_regional_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors preserve regional scope while retracting a middle timestamped process attribute"
        not in tso_attribute_regional_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO attribute regional execution handle drifted")
    tso_attribute_regional_case = query_rti_work.case_card_record(
        tso_attribute_regional,
        index,
        "umbra-cpp-process-tso-attribute-retraction-regional-integration",
    )
    if (
        tso_attribute_regional_case.get("lane")
        != "process-tso-attribute-retraction-regional"
        or tso_attribute_regional_case.get("commands", {}).get("ctest") is None
    ):
        raise AssertionError("exact process TSO regional case card lost its CTest handle")

    tso_attribute_reenable = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-attribute-retraction-reenable-integration"
        ),
        None,
    )
    if not isinstance(tso_attribute_reenable, dict):
        raise AssertionError("process TSO attribute re-enable plan row is absent")
    if query_rti_work.source_location_text(tso_attribute_reenable) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:22808"
    ):
        raise AssertionError("process TSO attribute re-enable source pointer drifted")
    if (
        tso_attribute_reenable.get("assertions") != 45
        or tso_attribute_reenable.get("primary_lane")
        != "process-tso-attribute-retraction-reenable"
        or tso_attribute_reenable.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("process TSO attribute re-enable plan evidence drifted")
    tso_attribute_reenable_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-attribute-retraction-reenable", limit=0
    )
    if (
        tso_attribute_reenable_focus.get("lane_state") != "complete"
        or tso_attribute_reenable_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_attribute_reenable_focus.get("mapped_test_count") != 1
        or tso_attribute_reenable_focus.get("assertion_count") != 45
        or tso_attribute_reenable_focus.get("recorded_assertion_count") != 45
        or tso_attribute_reenable_focus.get("requirement_count") != 5
        or tso_attribute_reenable_focus.get("standard_section_count") != 4
        or tso_attribute_reenable_focus.get("requirement_section_pair_count") != 5
    ):
        raise AssertionError("process TSO attribute re-enable lane card drifted")
    tso_attribute_reenable_handles = tso_attribute_reenable_focus.get("lane_handles")
    if (
        not isinstance(tso_attribute_reenable_handles, dict)
        or tso_attribute_reenable_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_attribute_reenable_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors preserve a timestamped process attribute retraction designator across time-regulation re-enable"
        not in tso_attribute_reenable_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO attribute re-enable execution handle drifted")
    tso_attribute_reenable_case = query_rti_work.case_card_record(
        tso_attribute_reenable,
        index,
        "umbra-cpp-process-tso-attribute-retraction-reenable-integration",
    )
    if (
        tso_attribute_reenable_case.get("lane")
        != "process-tso-attribute-retraction-reenable"
        or tso_attribute_reenable_case.get("commands", {}).get("ctest") is None
    ):
        raise AssertionError(
            "exact process TSO attribute re-enable case card lost its CTest handle"
        )

    tso_attribute_changed = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-attribute-update-regulation-reenable-changed-lookahead-integration"
        ),
        None,
    )
    if not isinstance(tso_attribute_changed, dict):
        raise AssertionError("process TSO changed-lookahead plan row is absent")
    if query_rti_work.source_location_text(tso_attribute_changed) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:23195"
    ):
        raise AssertionError("process TSO changed-lookahead source pointer drifted")
    if (
        tso_attribute_changed.get("assertions") != 70
        or tso_attribute_changed.get("primary_lane")
        != "process-tso-attribute-update-regulation-reenable-changed-lookahead"
        or tso_attribute_changed.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("process TSO changed-lookahead plan evidence drifted")
    tso_attribute_changed_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-tso-attribute-update-regulation-reenable-changed-lookahead",
        limit=0,
    )
    if (
        tso_attribute_changed_focus.get("lane_state") != "complete"
        or tso_attribute_changed_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_attribute_changed_focus.get("mapped_test_count") != 1
        or tso_attribute_changed_focus.get("assertion_count") != 70
        or tso_attribute_changed_focus.get("recorded_assertion_count") != 70
        or tso_attribute_changed_focus.get("requirement_count") != 11
        or tso_attribute_changed_focus.get("standard_section_count") != 8
        or tso_attribute_changed_focus.get("requirement_section_pair_count") != 11
    ):
        raise AssertionError("process TSO changed-lookahead lane card drifted")
    tso_attribute_changed_handles = tso_attribute_changed_focus.get("lane_handles")
    if (
        not isinstance(tso_attribute_changed_handles, dict)
        or tso_attribute_changed_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_attribute_changed_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors preserve a timestamped process attribute update across time-regulation re-enable with changed lookahead"
        not in tso_attribute_changed_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO changed-lookahead execution handle drifted")
    tso_attribute_changed_case = query_rti_work.case_card_record(
        tso_attribute_changed,
        index,
        "umbra-cpp-process-tso-attribute-update-regulation-reenable-changed-lookahead-integration",
    )
    if (
        tso_attribute_changed_case.get("lane")
        != "process-tso-attribute-update-regulation-reenable-changed-lookahead"
        or tso_attribute_changed_case.get("commands", {}).get("ctest") is None
    ):
        raise AssertionError(
            "exact process TSO changed-lookahead case card lost its CTest handle"
        )

    tso_interaction_changed = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-interaction-regulation-reenable-changed-lookahead-integration"
        ),
        None,
    )
    if not isinstance(tso_interaction_changed, dict):
        raise AssertionError("process TSO interaction changed-lookahead plan row is absent")
    if query_rti_work.source_location_text(tso_interaction_changed) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:23702"
    ):
        raise AssertionError("process TSO interaction changed-lookahead source pointer drifted")
    if (
        tso_interaction_changed.get("assertions") != 67
        or tso_interaction_changed.get("primary_lane")
        != "process-tso-interaction-regulation-reenable-changed-lookahead"
        or tso_interaction_changed.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("process TSO interaction changed-lookahead plan evidence drifted")
    tso_interaction_changed_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-tso-interaction-regulation-reenable-changed-lookahead",
        limit=0,
    )
    if (
        tso_interaction_changed_focus.get("lane_state") != "complete"
        or tso_interaction_changed_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_interaction_changed_focus.get("mapped_test_count") != 1
        or tso_interaction_changed_focus.get("assertion_count") != 67
        or tso_interaction_changed_focus.get("recorded_assertion_count") != 67
        or tso_interaction_changed_focus.get("requirement_count") != 28
        or tso_interaction_changed_focus.get("standard_section_count") != 15
        or tso_interaction_changed_focus.get("requirement_section_pair_count") != 28
    ):
        raise AssertionError("process TSO interaction changed-lookahead lane card drifted")
    tso_interaction_changed_handles = tso_interaction_changed_focus.get("lane_handles")
    if (
        not isinstance(tso_interaction_changed_handles, dict)
        or tso_interaction_changed_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_interaction_changed_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors preserve a timestamped process interaction across time-regulation re-enable with changed lookahead"
        not in tso_interaction_changed_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO interaction changed-lookahead execution handle drifted")
    tso_interaction_changed_case = query_rti_work.case_card_record(
        tso_interaction_changed,
        index,
        "umbra-cpp-process-tso-interaction-regulation-reenable-changed-lookahead-integration",
    )
    if (
        tso_interaction_changed_case.get("lane")
        != "process-tso-interaction-regulation-reenable-changed-lookahead"
        or tso_interaction_changed_case.get("commands", {}).get("ctest") is None
    ):
        raise AssertionError(
            "exact process TSO interaction changed-lookahead case card lost its CTest handle"
        )

    tso_interaction_multiple = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-interaction-multiple-message-ordering-integration"
        ),
        None,
    )
    if not isinstance(tso_interaction_multiple, dict):
        raise AssertionError("process TSO multiple-message plan row is absent")
    if query_rti_work.source_location_text(tso_interaction_multiple) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:24182"
    ):
        raise AssertionError("process TSO multiple-message source pointer drifted")
    if (
        tso_interaction_multiple.get("assertions") != 51
        or tso_interaction_multiple.get("primary_lane")
        != "process-tso-interaction-multiple-message-ordering"
        or tso_interaction_multiple.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("process TSO multiple-message plan evidence drifted")
    tso_interaction_multiple_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-tso-interaction-multiple-message-ordering",
        limit=0,
    )
    if (
        tso_interaction_multiple_focus.get("lane_state") != "complete"
        or tso_interaction_multiple_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_interaction_multiple_focus.get("mapped_test_count") != 1
        or tso_interaction_multiple_focus.get("assertion_count") != 51
        or tso_interaction_multiple_focus.get("recorded_assertion_count") != 51
        or tso_interaction_multiple_focus.get("requirement_count") != 20
        or tso_interaction_multiple_focus.get("standard_section_count") != 13
        or tso_interaction_multiple_focus.get("requirement_section_pair_count") != 20
    ):
        raise AssertionError("process TSO multiple-message lane card drifted")
    tso_interaction_multiple_handles = tso_interaction_multiple_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(tso_interaction_multiple_handles, dict)
        or tso_interaction_multiple_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_interaction_multiple_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors deliver multiple timestamped process interactions in FIFO order before one grant"
        not in tso_interaction_multiple_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO multiple-message execution handle drifted")
    tso_interaction_multiple_case = query_rti_work.case_card_record(
        tso_interaction_multiple,
        index,
        "umbra-cpp-process-tso-interaction-multiple-message-ordering-integration",
    )
    if (
        tso_interaction_multiple_case.get("lane")
        != "process-tso-interaction-multiple-message-ordering"
        or tso_interaction_multiple_case.get("commands", {}).get("ctest") is None
    ):
        raise AssertionError(
            "exact process TSO multiple-message case card lost its CTest handle"
        )

    tso_interaction_fanout = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-interaction-fanout-integration"
        ),
        None,
    )
    if not isinstance(tso_interaction_fanout, dict):
        raise AssertionError("process TSO fan-out plan row is absent")
    if query_rti_work.source_location_text(tso_interaction_fanout) != (
        "cpp/tests/ieee1516_2025_connection_catch2.cpp:24629"
    ):
        raise AssertionError("process TSO fan-out source pointer drifted")
    if (
        tso_interaction_fanout.get("assertions") != 133
        or tso_interaction_fanout.get("primary_lane")
        != "process-tso-interaction-fanout"
        or tso_interaction_fanout.get("callback_gate_modes")
        != ["disabled-queued-reenabled"]
        or tso_interaction_fanout.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("process TSO fan-out plan evidence drifted")
    tso_interaction_fanout_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-tso-interaction-fanout",
        limit=0,
    )
    if (
        tso_interaction_fanout_focus.get("lane_state") != "complete"
        or tso_interaction_fanout_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_interaction_fanout_focus.get("mapped_test_count") != 1
        or tso_interaction_fanout_focus.get("assertion_count") != 133
        or tso_interaction_fanout_focus.get("recorded_assertion_count") != 133
        or tso_interaction_fanout_focus.get("requirement_count") != 20
        or tso_interaction_fanout_focus.get("standard_section_count") != 13
        or tso_interaction_fanout_focus.get("requirement_section_pair_count") != 20
    ):
        raise AssertionError("process TSO fan-out lane card drifted")
    tso_interaction_fanout_handles = tso_interaction_fanout_focus.get("lane_handles")
    if (
        not isinstance(tso_interaction_fanout_handles, dict)
        or tso_interaction_fanout_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_interaction_fanout_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors fan out multiple timestamped process interactions FIFO to every subscribed receiver before one grant"
        not in tso_interaction_fanout_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO fan-out execution handle drifted")
    tso_interaction_fanout_case = query_rti_work.case_card_record(
        tso_interaction_fanout,
        index,
        "umbra-cpp-process-tso-interaction-fanout-integration",
    )
    if (
        tso_interaction_fanout_case.get("lane")
        != "process-tso-interaction-fanout"
        or tso_interaction_fanout_case.get("commands", {}).get("ctest") is None
    ):
        raise AssertionError(
            "exact process TSO fan-out case card lost its CTest handle"
        )

    process_time_focus = query_rti_work.focused_lane_result(
        index, tests, "process-time-role", limit=1
    )
    if process_time_focus.get("requirement_section_pair_count") != 3:
        raise AssertionError("process-time-role direct mapping pair count drifted")
    if set(process_time_focus.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-8.5.5",
        "hla-1516.1-2025:clause-8.6.3",
        "hla-1516.1-2025:clause-8.7.5",
    }:
        raise AssertionError("process-time-role section preview drifted")

    # Lane discovery is the bounded bridge between a family card and one exact
    # test.  It must expose mapping state and a copyable next-test handle
    # without requiring a broad tag/source search.
    family_lanes = query_rti_work.lane_inventory(
        index,
        tests,
        family="object-ddm-ownership",
        limit=0,
    )
    if not family_lanes.get("found") or not family_lanes.get("lanes"):
        raise AssertionError("family lane inventory did not resolve any exact tags")
    deletion_lane = next(
        (
            row
            for row in family_lanes["lanes"]
            if isinstance(row, dict) and row.get("tag") == "object-instance-deletion"
        ),
        None,
    )
    if not isinstance(deletion_lane, dict):
        raise AssertionError("object-instance-deletion lane was absent from family inventory")
    if deletion_lane.get("mapped_count") != 1 or deletion_lane.get("unclassified_count"):
        raise AssertionError("object-instance-deletion lane mapping counts drifted")
    if deletion_lane.get("resolved_requirement_section_pair_count") != 7:
        raise AssertionError("object-instance-deletion direct mapping count drifted")
    if not deletion_lane.get("focus_command") or not deletion_lane.get("ctest_command"):
        raise AssertionError("object-instance-deletion lane lost execution handles")
    unmapped_lanes = query_rti_work.lane_inventory(
        index,
        tests,
        family="object-ddm-ownership",
        unmapped_only=True,
        limit=0,
    )
    if not unmapped_lanes.get("found") or not unmapped_lanes.get("lanes"):
        raise AssertionError("unmapped family lane inventory was unexpectedly empty")
    object_lane = next(
        (
            row
            for row in unmapped_lanes["lanes"]
            if isinstance(row, dict)
            and row.get("tag") == "object-management"
        ),
        None,
    )
    if not isinstance(object_lane, dict):
        raise AssertionError("unmapped lane inventory lost the object-management queue")
    if object_lane.get("unclassified_count"):
        raise AssertionError(
            "object-management lane still reports an unclassified catalog row"
        )
    if object_lane.get("explicit_disposition_count", 0) < 9:
        raise AssertionError(
            "object-management lane lost its explicit catalog dispositions"
        )
    if object_lane.get("next_test") == (
        "Embedded receive-order Update Attribute Values honors 2025 passel and callback lifecycle"
    ):
        raise AssertionError("mapped receive-order update still appears in the unmapped queue")
    if object_lane.get("next_test") is not None:
        raise AssertionError(
            "explicit no-standalone-surface rows leaked into the implementation head"
        )
    if not object_lane.get("review_test") or not object_lane.get("review_trace_command"):
        raise AssertionError(
            "unmapped lane inventory omitted its explicit review handle"
        )

    # Keep intentional no-standalone-surface dispositions separate from rows
    # that still need a Lab mapping decision.  The CLI exposes this as
    # ``lanes --disposition`` so a family inventory can be queried without
    # treating an explicit disposition as missing work.
    explicit_lanes = query_rti_work.lane_inventory(
        index,
        tests,
        family="object-ddm-ownership",
        unmapped_only=True,
        disposition="explicit",
        limit=0,
    )
    if explicit_lanes.get("disposition") != "explicit":
        raise AssertionError("explicit lane inventory lost its disposition filter")
    if not explicit_lanes.get("lanes") or any(
        row.get("explicit_disposition_count", 0) <= 0
        for row in explicit_lanes.get("lanes", [])
        if isinstance(row, dict)
    ):
        raise AssertionError("explicit lane inventory returned a non-explicit lane")
    unclassified_lanes = query_rti_work.lane_inventory(
        index,
        tests,
        family="object-ddm-ownership",
        unmapped_only=True,
        disposition="unclassified",
        limit=0,
    )
    if unclassified_lanes.get("disposition") != "unclassified":
        raise AssertionError(
            "unclassified lane inventory lost its disposition filter"
        )
    if unclassified_lanes.get("lanes"):
        raise AssertionError(
            "object-ddm-ownership unexpectedly exposes an unclassified lane"
        )

    # An exact lane query through ``roadmap`` should be a one-command bridge
    # to the owning family and the same lane/focus/trace handles.  This keeps
    # resume work bounded when a contributor remembers the taxonomy tag but
    # not its roadmap owner.
    roadmap_lane = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="joined-federate-mom-deleted-object-count-periodic",
        status="open",
        limit=0,
    )
    if roadmap_lane.get("count") != 1 or not roadmap_lane.get("families"):
        raise AssertionError("exact lane roadmap search did not resolve one family")
    roadmap_lane_row = roadmap_lane["families"][0]
    if roadmap_lane_row.get("lane_match_count") != 1:
        raise AssertionError("exact lane roadmap search omitted its lane match")
    roadmap_lane_match = roadmap_lane_row.get("lane_matches", [None])[0]
    if not isinstance(roadmap_lane_match, dict):
        raise AssertionError("exact lane roadmap search returned no lane mapping")
    if roadmap_lane_match.get("tag") != "joined-federate-mom-deleted-object-count-periodic":
        raise AssertionError("exact lane roadmap search returned the wrong lane")
    if roadmap_lane_match.get("mapped_count") != 1:
        raise AssertionError("exact lane roadmap search lost mapped case count")
    if not roadmap_lane_match.get("focus_command") or not roadmap_lane_match.get(
        "representative_trace_command"
    ):
        raise AssertionError("exact lane roadmap search omitted focus/trace handles")
    if roadmap_lane_match.get("representative_test") != (
        "Embedded joined-federate MOM exposes HLAobjectInstancesDeleted count"
    ):
        raise AssertionError("exact lane roadmap search returned the wrong representative test")

    # Custom transportation delivery is a deliberate traceability boundary:
    # Eight implemented delivery forms have an explicit no-standalone-Lab
    # disposition, while the separately extracted transportation-control row
    # is now a mapped focused case. Keep that distinction queryable by one
    # exact taxonomy tag and keep the aggregate execution result separate from
    # the focused custom-transportation-type-control mapping.
    custom_rows = [
        test
        for test in tests
        if "custom-transportation" in test.get("tags", [])
    ]
    if len(custom_rows) != 10:
        raise AssertionError(
            "custom-transportation taxonomy row count drifted: "
            f"expected 10, observed {len(custom_rows)}"
        )
    explicit_custom = [
        test
        for test in custom_rows
        if test.get("traceability_state") == "explicit-disposition"
    ]
    unclassified_custom = [
        test
        for test in custom_rows
        if test.get("traceability_state") == "unclassified"
    ]
    mapped_custom = [
        test for test in custom_rows if test.get("traceability_state") == "requirements-mapped"
    ]
    if len(explicit_custom) != 8 or len(unclassified_custom) != 0 or len(mapped_custom) != 2:
        raise AssertionError(
            "custom-transportation disposition split drifted: "
            f"mapped={len(mapped_custom)}, explicit={len(explicit_custom)}, "
            f"unclassified={len(unclassified_custom)}"
        )
    custom_focus = query_rti_work.focused_lane_result(
        index, tests, "custom-transportation", limit=0
    )
    if custom_focus.get("lane_state") != "complete":
        raise AssertionError("custom-transportation lane should be complete after aggregate repair")
    if custom_focus.get("mapped_test_count") != 2:
        raise AssertionError("focus lost custom-transportation mapped count")
    if custom_focus.get("explicit_disposition_count") != 8:
        raise AssertionError("focus lost custom-transportation explicit disposition count")
    if custom_focus.get("unclassified_count") != 0:
        raise AssertionError("focus retained an unexpected custom-transportation gap")

    # The region lifecycle is the first newly isolated DDM target.  Keep its
    # exact taxonomy, requirement mapping, and standalone execution handle
    # protected so future aggregate edits cannot make this slice undiscoverable.
    region_rows = [
        test
        for test in tests
        if "region-lifecycle" in test.get("tags", [])
    ]
    if len(region_rows) != 8:
        raise AssertionError(
            "region-lifecycle taxonomy row count drifted: "
            f"expected 8, observed {len(region_rows)}"
        )
    if any(
        test.get("traceability_state") != "requirements-mapped"
        for test in region_rows
    ):
        raise AssertionError("region-lifecycle retained an unmapped plan row")
    region_focus = query_rti_work.focused_lane_result(
        index, tests, "region-lifecycle", limit=0
    )
    if region_focus.get("lane_state") != "complete":
        raise AssertionError("region-lifecycle lane should be complete")
    if region_focus.get("mapped_test_count") != 8:
        raise AssertionError("focus lost region-lifecycle mapped count")
    if region_focus.get("unclassified_count") != 0:
        raise AssertionError("region-lifecycle retained an unexpected mapping gap")
    region_inventory = query_rti_work.lane_inventory(
        index,
        tests,
        family="object-ddm-ownership",
        limit=0,
    )
    region_lane = next(
        (
            row
            for row in region_inventory.get("lanes", [])
            if isinstance(row, dict) and row.get("tag") == "region-lifecycle"
        ),
        None,
    )
    if not isinstance(region_lane, dict):
        raise AssertionError("region-lifecycle lane was absent from family inventory")
    if region_lane.get("resolved_requirement_section_pair_count") != 41:
        raise AssertionError("region-lifecycle direct mapping count drifted")
    region_handles = region_focus.get("lane_handles")
    if not isinstance(region_handles, dict) or region_handles.get(
        "catch2_target"
    ) != "umbra_region_lifecycle_catch2":
        raise AssertionError("region-lifecycle lost its standalone C++ target handle")

    # Query Attribute Ownership now has standalone owner/unowned and MOM
    # interaction slices. Protect their exact titles, assertion counts,
    # mappings, and executable handles so navigation remains one bounded
    # command.
    ownership_query = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-attribute-ownership-query-standalone"
        ),
        None,
    )
    if not isinstance(ownership_query, dict):
        raise AssertionError("standalone Query Attribute Ownership row is absent")
    if ownership_query.get("assertions") != 48:
        raise AssertionError("standalone Query Attribute Ownership assertion count drifted")
    if ownership_query.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone Query Attribute Ownership row is not mapped")
    ownership_query_focus = query_rti_work.focused_lane_result(
        index, tests, "query-attribute-ownership", limit=0
    )
    if ownership_query_focus.get("lane_state") != "complete":
        raise AssertionError("query-attribute-ownership lane state drifted")
    if ownership_query_focus.get("mapped_test_count") != 10:
        raise AssertionError("query-attribute-ownership mapped count drifted")
    if ownership_query_focus.get("assertion_count") != 190:
        raise AssertionError("query-attribute-ownership assertion total drifted")
    if ownership_query_focus.get("unclassified_count") != 0:
        raise AssertionError("query-attribute-ownership retained an unexpected mapping gap")
    ownership_query_handles = ownership_query_focus.get("lane_handles")
    if not isinstance(ownership_query_handles, dict) or ownership_query_handles.get(
        "catch2_target"
    ) != "umbra_attribute_ownership_query_catch2":
        raise AssertionError("query-attribute-ownership lost its standalone C++ target handle")

    if_available = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-acquisition-if-available-standalone"
        ),
        None,
    )
    if not isinstance(if_available, dict):
        raise AssertionError("standalone If Available acquisition row is absent")
    if if_available.get("assertions") != 59:
        raise AssertionError("standalone If Available acquisition assertion count drifted")
    if if_available.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone If Available acquisition row is not mapped")
    if_available_focus = query_rti_work.focused_lane_result(
        index, tests, "attribute-ownership-acquisition-if-available", limit=0
    )
    if if_available_focus.get("lane_state") != "complete":
        raise AssertionError("If Available acquisition lane should be complete")
    if if_available_focus.get("unclassified_count") != 0:
        raise AssertionError("If Available acquisition retained an unexpected mapping gap")
    if_available_handles = if_available_focus.get("lane_handles")
    if not isinstance(if_available_handles, dict) or if_available_handles.get(
        "catch2_target"
    ) != "umbra_attribute_ownership_acquisition_if_available_catch2":
        raise AssertionError("If Available acquisition lost its standalone C++ target handle")

    process_if_available = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-acquisition-if-available-process-integration"
        ),
        None,
    )
    if not isinstance(process_if_available, dict):
        raise AssertionError("process If Available acquisition row is absent")
    if process_if_available.get("assertions") != 22:
        raise AssertionError("process If Available acquisition assertion count drifted")
    if process_if_available.get("traceability_state") != "requirements-mapped":
        raise AssertionError("process If Available acquisition row is not mapped")
    process_if_available_focus = query_rti_work.focused_lane_result(
        index, tests, "process-ownership-acquisition-if-available", limit=0
    )
    if (
        process_if_available_focus.get("lane_state") != "complete"
        or process_if_available_focus.get("mapped_test_count") != 1
        or process_if_available_focus.get("assertion_count") != 22
        or process_if_available_focus.get("requirement_count") != 4
        or process_if_available_focus.get("standard_section_count") != 3
        or process_if_available_focus.get("requirement_section_pair_count") != 4
    ):
        raise AssertionError("process If Available acquisition lane card drifted")
    process_if_available_handles = process_if_available_focus.get("lane_handles")
    if not isinstance(process_if_available_handles, dict) or process_if_available_handles.get(
        "catch2_target"
    ) != "umbra_attribute_ownership_acquisition_if_available_catch2":
        raise AssertionError("process If Available acquisition lost its C++ target handle")

    process_acquisition = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-acquisition-process-integration"
        ),
        None,
    )
    if not isinstance(process_acquisition, dict):
        raise AssertionError("process regular acquisition row is absent")
    if process_acquisition.get("assertions") != 22:
        raise AssertionError("process regular acquisition assertion count drifted")
    if process_acquisition.get("traceability_state") != "requirements-mapped":
        raise AssertionError("process regular acquisition row is not mapped")
    process_acquisition_focus = query_rti_work.focused_lane_result(
        index, tests, "process-ownership-acquisition", limit=0
    )
    if (
        process_acquisition_focus.get("lane_state") != "complete"
        or process_acquisition_focus.get("mapped_test_count") != 1
        or process_acquisition_focus.get("assertion_count") != 22
        or process_acquisition_focus.get("requirement_count") != 2
        or process_acquisition_focus.get("standard_section_count") != 2
        or process_acquisition_focus.get("requirement_section_pair_count") != 2
    ):
        raise AssertionError("process regular acquisition lane card drifted")
    process_acquisition_handles = process_acquisition_focus.get("lane_handles")
    if not isinstance(process_acquisition_handles, dict) or process_acquisition_handles.get(
        "catch2_target"
    ) != "umbra_attribute_ownership_acquisition_catch2":
        raise AssertionError("process regular acquisition lost its C++ target handle")

    process_acquisition_release = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-acquisition-release-process-integration"
        ),
        None,
    )
    if not isinstance(process_acquisition_release, dict):
        raise AssertionError("process owner-release acquisition row is absent")
    if process_acquisition_release.get("assertions") != 30:
        raise AssertionError("process owner-release acquisition assertion count drifted")
    if process_acquisition_release.get("traceability_state") != "requirements-mapped":
        raise AssertionError("process owner-release acquisition row is not mapped")
    process_acquisition_release_focus = query_rti_work.focused_lane_result(
        index, tests, "process-ownership-acquisition-release", limit=0
    )
    if (
        process_acquisition_release_focus.get("lane_state") != "complete"
        or process_acquisition_release_focus.get("mapped_test_count") != 1
        or process_acquisition_release_focus.get("assertion_count") != 30
        or process_acquisition_release_focus.get("requirement_count") != 3
        or process_acquisition_release_focus.get("standard_section_count") != 2
        or process_acquisition_release_focus.get("requirement_section_pair_count") != 3
    ):
        raise AssertionError("process owner-release acquisition lane card drifted")
    process_acquisition_release_handles = process_acquisition_release_focus.get(
        "lane_handles"
    )
    if not isinstance(process_acquisition_release_handles, dict) or process_acquisition_release_handles.get(
        "catch2_target"
    ) != "umbra_attribute_ownership_acquisition_catch2":
        raise AssertionError("process owner-release acquisition lost its C++ target handle")

    process_acquisition_release_denied = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-acquisition-release-denied-process-integration"
        ),
        None,
    )
    if not isinstance(process_acquisition_release_denied, dict):
        raise AssertionError("process owner-release-denied row is absent")
    if process_acquisition_release_denied.get("assertions") != 36:
        raise AssertionError(
            "process owner-release-denied assertion count drifted"
        )
    if process_acquisition_release_denied.get("traceability_state") != "requirements-mapped":
        raise AssertionError("process owner-release-denied row is not mapped")
    process_acquisition_release_denied_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-ownership-acquisition-release-denied",
        limit=0,
    )
    if (
        process_acquisition_release_denied_focus.get("lane_state") != "complete"
        or process_acquisition_release_denied_focus.get("mapped_test_count") != 1
        or process_acquisition_release_denied_focus.get("assertion_count") != 36
        or process_acquisition_release_denied_focus.get("requirement_count") != 3
        or process_acquisition_release_denied_focus.get("standard_section_count") != 2
        or process_acquisition_release_denied_focus.get("requirement_section_pair_count") != 3
    ):
        raise AssertionError(
            "process owner-release-denied lane card drifted"
        )
    process_acquisition_release_denied_handles = (
        process_acquisition_release_denied_focus.get("lane_handles")
    )
    if not isinstance(process_acquisition_release_denied_handles, dict) or process_acquisition_release_denied_handles.get(
        "catch2_target"
    ) != "umbra_attribute_ownership_acquisition_catch2":
        raise AssertionError(
            "process owner-release-denied lost its C++ target handle"
        )

    process_acquisition_cancellation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-acquisition-cancellation-process-integration"
        ),
        None,
    )
    if not isinstance(process_acquisition_cancellation, dict):
        raise AssertionError("process ownership-cancellation row is absent")
    if query_rti_work.source_location_text(process_acquisition_cancellation) != (
        "cpp/tests/attribute_ownership_acquisition_catch2.cpp:1142"
    ) or process_acquisition_cancellation.get("assertions") != 34:
        raise AssertionError("process ownership-cancellation source/evidence drifted")
    if process_acquisition_cancellation.get("traceability_state") != "requirements-mapped":
        raise AssertionError("process ownership-cancellation row is not mapped")
    process_acquisition_cancellation_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-ownership-acquisition-cancellation",
        limit=0,
    )
    if (
        process_acquisition_cancellation_focus.get("lane_state") != "complete"
        or process_acquisition_cancellation_focus.get("mapped_test_count") != 1
        or process_acquisition_cancellation_focus.get("assertion_count") != 34
        or process_acquisition_cancellation_focus.get("requirement_count") != 3
        or process_acquisition_cancellation_focus.get("standard_section_count") != 3
        or process_acquisition_cancellation_focus.get("requirement_section_pair_count") != 3
    ):
        raise AssertionError("process ownership-cancellation lane card drifted")
    process_acquisition_cancellation_handles = (
        process_acquisition_cancellation_focus.get("lane_handles")
    )
    if not isinstance(process_acquisition_cancellation_handles, dict) or process_acquisition_cancellation_handles.get(
        "catch2_target"
    ) != "umbra_attribute_ownership_acquisition_catch2":
        raise AssertionError("process ownership-cancellation lost its C++ target handle")
    if process_acquisition_cancellation_handles.get("ctest_label") != "process-boundary":
        raise AssertionError("process ownership-cancellation CTest label drifted")

    process_negotiated_cancellation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-negotiated-divestiture-cancellation-process-integration"
        ),
        None,
    )
    if not isinstance(process_negotiated_cancellation, dict):
        raise AssertionError("process negotiated-divestiture cancellation row is absent")
    if query_rti_work.source_location_text(process_negotiated_cancellation) != (
        "cpp/tests/attribute_ownership_acquisition_catch2.cpp:1434"
    ) or process_negotiated_cancellation.get("assertions") != 34:
        raise AssertionError(
            "process negotiated-divestiture cancellation source/evidence drifted"
        )
    if process_negotiated_cancellation.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "process negotiated-divestiture cancellation row is not mapped"
        )
    process_negotiated_cancellation_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-negotiated-divestiture-cancellation",
        limit=0,
    )
    if (
        process_negotiated_cancellation_focus.get("lane_state") != "complete"
        or process_negotiated_cancellation_focus.get("mapped_test_count") != 1
        or process_negotiated_cancellation_focus.get("assertion_count") != 34
        or process_negotiated_cancellation_focus.get("requirement_count") != 3
        or process_negotiated_cancellation_focus.get("standard_section_count") != 3
        or process_negotiated_cancellation_focus.get("requirement_section_pair_count") != 3
    ):
        raise AssertionError(
            "process negotiated-divestiture cancellation lane card drifted"
        )
    process_negotiated_cancellation_handles = (
        process_negotiated_cancellation_focus.get("lane_handles")
    )
    if not isinstance(process_negotiated_cancellation_handles, dict) or process_negotiated_cancellation_handles.get(
        "catch2_target"
    ) != "umbra_attribute_ownership_acquisition_catch2":
        raise AssertionError(
            "process negotiated-divestiture cancellation lost its C++ target handle"
        )
    if process_negotiated_cancellation_handles.get("ctest_label") != "process-boundary":
        raise AssertionError(
            "process negotiated-divestiture cancellation CTest label drifted"
        )

    regular_acquisition = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-attribute-ownership-acquisition-standalone"
        ),
        None,
    )
    if not isinstance(regular_acquisition, dict):
        raise AssertionError("standalone regular acquisition row is absent")
    if regular_acquisition.get("assertions") != 74:
        raise AssertionError("standalone regular acquisition assertion count drifted")
    if regular_acquisition.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone regular acquisition row is not mapped")
    regular_acquisition_focus = query_rti_work.focused_lane_result(
        index, tests, "attribute-ownership-acquisition", limit=0
    )
    if regular_acquisition_focus.get("lane_state") != "complete":
        raise AssertionError("regular acquisition lane should be complete")
    if regular_acquisition_focus.get("unclassified_count") != 0:
        raise AssertionError("regular acquisition retained an unexpected mapping gap")
    regular_acquisition_handles = regular_acquisition_focus.get("lane_handles")
    if not isinstance(regular_acquisition_handles, dict) or regular_acquisition_handles.get(
        "catch2_target"
    ) != "umbra_attribute_ownership_acquisition_catch2":
        raise AssertionError("regular acquisition lost its standalone C++ target handle")

    # Negotiated divestiture now has an explicit §7.2 assumption-callback
    # lane. Protect the one-test reverse map, owning family, assertion total,
    # and focused CTest handle so this narrow slice remains directly
    # queryable as the aggregate ownership tests evolve.
    negotiated_assumption = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-negotiated-assumption-integration"
        ),
        None,
    )
    if not isinstance(negotiated_assumption, dict):
        raise AssertionError("negotiated-assumption row is absent")
    if negotiated_assumption.get("assertions") != 30:
        raise AssertionError("negotiated-assumption assertion count drifted")
    if negotiated_assumption.get("traceability_state") != "requirements-mapped":
        raise AssertionError("negotiated-assumption row is not mapped")
    if negotiated_assumption.get("standard_sections") != [
        "hla-1516.1-2025:clause-7.2"
    ]:
        raise AssertionError("negotiated-assumption section mapping drifted")
    negotiated_assumption_focus = query_rti_work.focused_lane_result(
        index, tests, "negotiated-assumption", limit=0
    )
    if negotiated_assumption_focus.get("lane_state") != "complete":
        raise AssertionError("negotiated-assumption lane should be complete")
    if negotiated_assumption_focus.get("roadmap_owner") != "object-ddm-ownership":
        raise AssertionError("negotiated-assumption lane lost its roadmap owner")
    if negotiated_assumption_focus.get("mapped_test_count") != 1:
        raise AssertionError("negotiated-assumption mapped count drifted")
    if negotiated_assumption_focus.get("assertion_count") != 30:
        raise AssertionError("negotiated-assumption lane assertion total drifted")
    negotiated_assumption_handles = negotiated_assumption_focus.get("lane_handles")
    if not isinstance(negotiated_assumption_handles, dict) or negotiated_assumption_handles.get(
        "catch2_target"
    ) != "umbra_negotiated_willing_to_acquire_candidate_catch2":
        raise AssertionError("negotiated-assumption lost its focused C++ target handle")
    if query_rti_work.lane_ctest_command(negotiated_assumption_handles) != (
        'ctest --test-dir <build-dir> -C Debug -R "^umbra\\.negotiated_willing_to_acquire_candidate\\.catch2\\.Embedded Negotiated Attribute Ownership Divestiture forwards its tag to an assumption candidate$" '
        "--output-on-failure"
    ):
        raise AssertionError("negotiated-assumption lost its focused CTest command")

    regional_region_context = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-regional-object-attribute-region-context-integration"
        ),
        None,
    )
    if not isinstance(regional_region_context, dict):
        raise AssertionError("regional object region-context row is absent")
    if regional_region_context.get("assertions") != 34:
        raise AssertionError("regional object region-context assertion count drifted")
    if len(regional_region_context.get("lab_requirement_ids", [])) != 5:
        raise AssertionError("regional object region-context requirement count drifted")
    if regional_region_context.get("traceability_state") != "requirements-mapped":
        raise AssertionError("regional object region-context row is not mapped")
    if regional_region_context.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.3.3"
    ]:
        raise AssertionError("regional object region-context section mapping drifted")
    regional_region_context_focus = query_rti_work.focused_lane_result(
        index, tests, "regional-object-attribute-region-context", limit=0
    )
    if regional_region_context_focus.get("lane_state") != "complete":
        raise AssertionError("regional object region-context lane should be complete")
    if regional_region_context_focus.get("roadmap_owner") != "object-ddm-ownership":
        raise AssertionError("regional object region-context lane lost its roadmap owner")
    if regional_region_context_focus.get("mapped_test_count") != 1:
        raise AssertionError("regional object region-context mapped count drifted")
    if regional_region_context_focus.get("assertion_count") != 34:
        raise AssertionError("regional object region-context assertion total drifted")
    regional_region_context_handles = regional_region_context_focus.get("lane_handles")
    if not isinstance(regional_region_context_handles, dict) or regional_region_context_handles.get(
        "catch2_target"
    ) != "umbra_regional_object_attribute_routing_catch2":
        raise AssertionError("regional object region-context lost its focused C++ target handle")
    if query_rti_work.lane_ctest_command(regional_region_context_handles) != (
        'ctest --test-dir <build-dir> -C Debug -R "^umbra\\.regional_object_attribute_routing\\.catch2\\.Embedded regional object services reject region dimensions outside available object dimensions$" '
        "--output-on-failure"
    ):
        raise AssertionError("regional object region-context lost its focused CTest command")

    unconditional_divestiture = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-unconditional-attribute-ownership-divestiture-standalone"
        ),
        None,
    )
    if not isinstance(unconditional_divestiture, dict):
        raise AssertionError("standalone unconditional-divestiture row is absent")
    if unconditional_divestiture.get("assertions") != 116:
        raise AssertionError("standalone unconditional-divestiture assertion count drifted")
    if unconditional_divestiture.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone unconditional-divestiture row is not mapped")
    unconditional_focus = query_rti_work.focused_lane_result(
        index, tests, "unconditional-attribute-ownership-divestiture", limit=0
    )
    if unconditional_focus.get("lane_state") != "complete":
        raise AssertionError("unconditional-divestiture lane should be complete")
    if unconditional_focus.get("unclassified_count") != 0:
        raise AssertionError("unconditional-divestiture retained an unexpected mapping gap")
    unconditional_handles = unconditional_focus.get("lane_handles")
    if not isinstance(unconditional_handles, dict) or unconditional_handles.get(
        "catch2_target"
    ) != "umbra_unconditional_attribute_ownership_divestiture_catch2":
        raise AssertionError("unconditional-divestiture lost its standalone C++ target handle")

    resign_action = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-resign-action-divestiture-standalone"
        ),
        None,
    )
    if not isinstance(resign_action, dict):
        raise AssertionError("standalone resign-action divestiture row is absent")
    if resign_action.get("assertions") != 32:
        raise AssertionError("standalone resign-action divestiture assertion count drifted")
    if resign_action.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone resign-action divestiture row is not mapped")
    resign_action_focus = query_rti_work.focused_lane_result(
        index, tests, "resign-action-unconditional-divestiture", limit=0
    )
    if resign_action_focus.get("lane_state") != "complete":
        raise AssertionError("resign-action divestiture lane should be complete")
    if resign_action_focus.get("unclassified_count") != 0:
        raise AssertionError("resign-action divestiture retained an unexpected mapping gap")
    resign_action_handles = resign_action_focus.get("lane_handles")
    if not isinstance(resign_action_handles, dict) or resign_action_handles.get(
        "catch2_target"
    ) != "umbra_resign_action_unconditional_divestiture_catch2":
        raise AssertionError("resign-action divestiture lost its standalone C++ target handle")

    connection_loss_unconditional = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-connection-lost-automatic-unconditional-divest-standalone"
        ),
        None,
    )
    if not isinstance(connection_loss_unconditional, dict):
        raise AssertionError("standalone Connection Lost automatic-divestiture row is absent")
    if connection_loss_unconditional.get("assertions") != 38:
        raise AssertionError("standalone Connection Lost automatic-divestiture assertion count drifted")
    if connection_loss_unconditional.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone Connection Lost automatic-divestiture row is not mapped")
    connection_loss_focus = query_rti_work.focused_lane_result(
        index, tests, "connection-lost-automatic-unconditional-divestiture", limit=0
    )
    if connection_loss_focus.get("lane_state") != "complete":
        raise AssertionError("Connection Lost automatic-divestiture lane should be complete")
    if connection_loss_focus.get("unclassified_count") != 0:
        raise AssertionError(
            "Connection Lost automatic-divestiture retained an unexpected mapping gap"
        )
    connection_loss_handles = connection_loss_focus.get("lane_handles")
    if not isinstance(connection_loss_handles, dict) or connection_loss_handles.get(
        "catch2_target"
    ) != "umbra_connection_loss_automatic_unconditional_divestiture_catch2":
        raise AssertionError(
            "Connection Lost automatic-divestiture lost its standalone C++ target handle"
        )

    connection_loss_cancel = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-connection-lost-automatic-cancel-pending-acquisition-standalone"
        ),
        None,
    )
    if not isinstance(connection_loss_cancel, dict):
        raise AssertionError(
            "standalone Connection Lost cancellation row is absent"
        )
    if connection_loss_cancel.get("assertions") != 62:
        raise AssertionError(
            "standalone Connection Lost cancellation assertion count drifted"
        )
    if connection_loss_cancel.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "standalone Connection Lost cancellation row is not mapped"
        )
    connection_loss_cancel_focus = query_rti_work.focused_lane_result(
        index, tests, "connection-lost-automatic-cancel-pending-acquisition", limit=0
    )
    if connection_loss_cancel_focus.get("lane_state") != "complete":
        raise AssertionError(
            "Connection Lost cancellation lane should be complete"
        )
    if connection_loss_cancel_focus.get("unclassified_count") != 0:
        raise AssertionError(
            "Connection Lost cancellation retained an unexpected mapping gap"
        )
    connection_loss_cancel_handles = connection_loss_cancel_focus.get("lane_handles")
    if not isinstance(connection_loss_cancel_handles, dict) or connection_loss_cancel_handles.get(
        "catch2_target"
    ) != "umbra_connection_loss_automatic_cancel_pending_acquisition_catch2":
        raise AssertionError(
            "Connection Lost cancellation lost its standalone C++ target handle"
        )

    resign_pending = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-resign-action-pending-acquisition-standalone"
        ),
        None,
    )
    if not isinstance(resign_pending, dict):
        raise AssertionError("standalone resign pending-acquisition row is absent")
    if resign_pending.get("assertions") != 23:
        raise AssertionError("standalone resign pending-acquisition assertion count drifted")
    if resign_pending.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone resign pending-acquisition row is not mapped")
    resign_pending_focus = query_rti_work.focused_lane_result(
        index, tests, "resign-action-pending-acquisition-rejection", limit=0
    )
    if resign_pending_focus.get("lane_state") != "complete":
        raise AssertionError("resign pending-acquisition lane should be complete")
    if resign_pending_focus.get("unclassified_count") != 0:
        raise AssertionError(
            "resign pending-acquisition retained an unexpected mapping gap"
        )
    resign_pending_handles = resign_pending_focus.get("lane_handles")
    if not isinstance(resign_pending_handles, dict) or resign_pending_handles.get(
        "catch2_target"
    ) != "umbra_resign_action_pending_acquisition_rejection_catch2":
        raise AssertionError(
            "resign pending-acquisition lost its standalone C++ target handle"
        )

    # The three directive-3 resignation forms share the same public action but
    # deliberately remain separate lanes. Keep each exact source row,
    # assertion total, mapping state, and executable handle protected so the
    # next ownership task is discoverable without reopening the aggregate file.
    for row_id, lane, assertion_count, target_name in (
        (
            "umbra-cpp-resign-action-cancel-pending-standalone",
            "resign-action-cancel-pending-acquisition",
            24,
            "umbra_resign_action_cancel_pending_acquisition_catch2",
        ),
        (
            "umbra-cpp-resign-action-cancel-negotiated-standalone",
            "resign-action-cancel-negotiated-pending",
            25,
            "umbra_resign_action_cancel_negotiated_pending_catch2",
        ),
        (
            "umbra-cpp-resign-action-cancel-if-available-standalone",
            "resign-action-cancel-if-available-pending",
            22,
            "umbra_resign_action_cancel_if_available_pending_catch2",
        ),
    ):
        cancellation_row = next(
            (test for test in tests if test.get("id") == row_id),
            None,
        )
        if not isinstance(cancellation_row, dict):
            raise AssertionError(f"standalone resignation row is absent: {row_id}")
        if cancellation_row.get("assertions") != assertion_count:
            raise AssertionError(f"resignation assertion count drifted: {lane}")
        if cancellation_row.get("traceability_state") != "requirements-mapped":
            raise AssertionError(f"resignation row is not mapped: {lane}")
        cancellation_focus = query_rti_work.focused_lane_result(
            index, tests, lane, limit=0
        )
        if cancellation_focus.get("lane_state") != "complete":
            raise AssertionError(f"resignation lane should be complete: {lane}")
        if cancellation_focus.get("unclassified_count") != 0:
            raise AssertionError(f"resignation lane retained an unmapped case: {lane}")
        cancellation_handles = cancellation_focus.get("lane_handles")
        if not isinstance(cancellation_handles, dict) or cancellation_handles.get(
            "catch2_target"
        ) != target_name:
            raise AssertionError(f"resignation lane lost its C++ target handle: {lane}")

    # Directive 2 is a separate resignation lane from the directive-3
    # cancellation forms above. Keep its delete-privileged object behavior
    # independently indexed so queue selection never has to rediscover the
    # aggregate source case.
    delete_objects_row = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-resign-action-delete-objects-standalone"
        ),
        None,
    )
    if not isinstance(delete_objects_row, dict):
        raise AssertionError("standalone resign delete-objects row is absent")
    if delete_objects_row.get("assertions") != 25:
        raise AssertionError("standalone resign delete-objects assertion count drifted")
    if delete_objects_row.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone resign delete-objects row is not mapped")
    delete_objects_focus = query_rti_work.focused_lane_result(
        index, tests, "resign-action-delete-objects", limit=0
    )
    if delete_objects_focus.get("lane_state") != "complete":
        raise AssertionError("resign delete-objects lane should be complete")
    if delete_objects_focus.get("unclassified_count") != 0:
        raise AssertionError("resign delete-objects retained an unmapped case")
    delete_objects_handles = delete_objects_focus.get("lane_handles")
    if not isinstance(delete_objects_handles, dict) or delete_objects_handles.get(
        "catch2_target"
    ) != "umbra_resign_action_delete_objects_catch2":
        raise AssertionError("resign delete-objects lane lost its C++ target handle")

    final_federate_row = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-resign-action-final-federate-standalone"
        ),
        None,
    )
    if not isinstance(final_federate_row, dict):
        raise AssertionError("standalone final-federate resignation row is absent")
    if final_federate_row.get("assertions") != 22:
        raise AssertionError("standalone final-federate assertion count drifted")
    if final_federate_row.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone final-federate row is not mapped")
    final_federate_focus = query_rti_work.focused_lane_result(
        index, tests, "resign-action-final-federate", limit=0
    )
    if final_federate_focus.get("lane_state") != "complete":
        raise AssertionError("resign final-federate lane should be complete")
    if final_federate_focus.get("unclassified_count") != 0:
        raise AssertionError("resign final-federate retained an unmapped case")
    final_federate_handles = final_federate_focus.get("lane_handles")
    if not isinstance(final_federate_handles, dict) or final_federate_handles.get(
        "catch2_target"
    ) != "umbra_resign_action_final_federate_catch2":
        raise AssertionError("resign final-federate lane lost its C++ target handle")

    update_rate_row = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-update-rate-value-standalone"
        ),
        None,
    )
    if not isinstance(update_rate_row, dict):
        raise AssertionError("standalone update-rate value row is absent")
    if update_rate_row.get("assertions") != 40:
        raise AssertionError("standalone update-rate value assertion count drifted")
    if update_rate_row.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone update-rate value row is not mapped")
    update_rate_focus = query_rti_work.focused_lane_result(
        index, tests, "update-rate-value", limit=0
    )
    if update_rate_focus.get("lane_state") != "complete":
        raise AssertionError("update-rate value lane should be complete")
    if update_rate_focus.get("unclassified_count") != 0:
        raise AssertionError("update-rate value retained an unmapped case")
    update_rate_handles = update_rate_focus.get("lane_handles")
    if not isinstance(update_rate_handles, dict) or update_rate_handles.get(
        "catch2_target"
    ) != "umbra_update_rate_value_catch2":
        raise AssertionError("update-rate value lane lost its C++ target handle")

    ddm_focus = query_rti_work.focused_lane_result(index, tests, "ddm", limit=0)
    # ``ddm`` is an intentionally broad taxonomy view.  Its mapped rows are
    # complete; any historical rows without a standalone source declaration
    # remain explicit dispositions rather than executable work.
    if ddm_focus.get("lane_state") != "complete":
        raise AssertionError("ddm lane state drifted")
    if ddm_focus.get("source_drift_count") != 0:
        raise AssertionError("ddm source-drift count drifted")
    if ddm_focus.get("unclassified_count") != 0:
        raise AssertionError("ddm retained an unexpected unclassified case")

    if not query_rti_work.select_tests(
        tests, "PROCESS-BOUNDARY", "lane"
    ):
        raise AssertionError("lane lookup should accept case-insensitive exact tags")
    uppercase_focus = query_rti_work.focused_lane_result(
        index, tests, "PROCESS-BOUNDARY", limit=1
    )
    if uppercase_focus.get("lane_state") != "complete":
        raise AssertionError("focused lane lookup should accept case-insensitive tags")
    if not uppercase_focus.get("lane_handles"):
        raise AssertionError("case-insensitive focused lane lost its configured handles")

    directed_focus = query_rti_work.focused_lane_result(
        index, tests, "directed", limit=0
    )
    if directed_focus.get("test_count") != 41:
        raise AssertionError("directed lane plan count drifted")
    if directed_focus.get("mapped_test_count") != 39:
        raise AssertionError("directed lane mapped count drifted")
    directed_handles = directed_focus.get("lane_handles")
    if not isinstance(directed_handles, dict) or directed_handles.get(
        "ctest_label"
    ) != "directed":
        raise AssertionError("directed lane lost its cross-target CTest label")
    if query_rti_work.lane_ctest_command(directed_handles) != (
        'ctest --test-dir <build-dir> -C Debug -L "^directed$" '
        "--output-on-failure"
    ):
        raise AssertionError("label-backed directed lane lost its CTest command")

    regional_focus = query_rti_work.focused_lane_result(
        index, tests, "regional-object-attribute-routing", limit=1
    )
    if regional_focus.get("roadmap_owner") != "object-ddm-ownership":
        raise AssertionError(
            "regional object-attribute lane lost its roadmap owner"
        )
    regional_routing = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-regional-object-attribute-routing-integration"
        ),
        None,
    )
    if not isinstance(regional_routing, dict):
        raise AssertionError("regional object-attribute routing row is absent")
    if regional_routing.get("assertions") != 61:
        raise AssertionError("regional object-attribute routing assertion count drifted")
    if len(regional_routing.get("lab_requirement_ids", [])) != 22:
        raise AssertionError("regional object-attribute routing requirement count drifted")
    if (
        "requirement-candidate-content-clauses-09-data-distribution-management-page-219-l113-36"
        not in regional_routing.get("lab_requirement_ids", [])
    ):
        raise AssertionError(
            "regional object-attribute routing lost the §9.1.3.2 relationship anchor"
        )
    if regional_routing.get("traceability_state") != "requirements-mapped":
        raise AssertionError("regional object-attribute routing row is not mapped")
    if regional_routing.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.3.2",
        "hla-1516.1-2025:clause-9.1.3.3",
        "hla-1516.1-2025:clause-9.5",
        "hla-1516.1-2025:clause-9.6",
        "hla-1516.1-2025:clause-9.7.5",
        "hla-1516.1-2025:clause-9.8",
        "hla-1516.1-2025:clause-9.9.3",
    ]:
        raise AssertionError("regional object-attribute routing section mapping drifted")
    if regional_focus.get("lane_state") != "complete":
        raise AssertionError("regional object-attribute lane should be complete")
    if regional_focus.get("mapped_test_count") != 1:
        raise AssertionError("regional object-attribute mapped count drifted")
    if regional_focus.get("assertion_count") != 61:
        raise AssertionError("regional object-attribute lane assertion total drifted")
    regional_handles = regional_focus.get("lane_handles")
    if not isinstance(regional_handles, dict) or regional_handles.get(
        "catch2_target"
    ) != "umbra_regional_object_attribute_routing_catch2":
        raise AssertionError(
            "regional object-attribute lane lost its focused C++ target handle"
        )

    zero_dim = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-zero-dimensional-region-nonoverlap-integration"
        ),
        None,
    )
    if not isinstance(zero_dim, dict):
        raise AssertionError("zero-dimensional region row is absent")
    if query_rti_work.source_location_text(zero_dim) != (
        "cpp/tests/regional_object_attribute_routing_catch2.cpp:593"
    ):
        raise AssertionError("zero-dimensional region source pointer drifted")
    if zero_dim.get("assertions") != 26:
        raise AssertionError("zero-dimensional region assertion count drifted")
    if zero_dim.get("lab_requirement_ids") != [
        "requirement-candidate-content-clauses-09-data-distribution-management-page-219-l23-7"
    ]:
        raise AssertionError("zero-dimensional region requirement mapping drifted")
    if zero_dim.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.3.2"
    ]:
        raise AssertionError("zero-dimensional region section mapping drifted")
    zero_dim_focus = query_rti_work.focused_lane_result(
        index, tests, "zero-dimensional-region", limit=0
    )
    if zero_dim_focus.get("lane_state") != "complete":
        raise AssertionError("zero-dimensional region lane should be complete")
    if zero_dim_focus.get("mapped_test_count") != 1:
        raise AssertionError("zero-dimensional region mapped count drifted")
    if zero_dim_focus.get("assertion_count") != 26:
        raise AssertionError("zero-dimensional region lane assertion total drifted")
    zero_dim_handles = zero_dim_focus.get("lane_handles")
    if (
        not isinstance(zero_dim_handles, dict)
        or zero_dim_handles.get("catch2_target")
        != "umbra_regional_object_attribute_routing_catch2"
        or zero_dim_handles.get("ctest_label") != "zero-dimensional-region"
    ):
        raise AssertionError("zero-dimensional region lane handles drifted")

    zero_dim_interaction = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-zero-dimensional-regional-interaction-nonoverlap-integration"
        ),
        None,
    )
    if not isinstance(zero_dim_interaction, dict):
        raise AssertionError("zero-dimensional regional-interaction row is absent")
    if query_rti_work.source_location_text(zero_dim_interaction) != (
        "cpp/tests/zero_dimensional_regional_interaction_catch2.cpp:91"
    ):
        raise AssertionError("zero-dimensional regional-interaction source pointer drifted")
    if zero_dim_interaction.get("assertions") != 28:
        raise AssertionError("zero-dimensional regional-interaction assertion count drifted")
    if zero_dim_interaction.get("lab_requirement_ids") != [
        "requirement-candidate-content-clauses-09-data-distribution-management-page-219-l23-7"
    ]:
        raise AssertionError("zero-dimensional regional-interaction requirement mapping drifted")
    if zero_dim_interaction.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.3.2"
    ]:
        raise AssertionError("zero-dimensional regional-interaction section mapping drifted")
    zero_dim_interaction_focus = query_rti_work.focused_lane_result(
        index, tests, "zero-dimensional-regional-interaction", limit=0
    )
    if zero_dim_interaction_focus.get("lane_state") != "complete":
        raise AssertionError("zero-dimensional regional-interaction lane should be complete")
    if zero_dim_interaction_focus.get("mapped_test_count") != 1:
        raise AssertionError("zero-dimensional regional-interaction mapped count drifted")
    if zero_dim_interaction_focus.get("assertion_count") != 28:
        raise AssertionError("zero-dimensional regional-interaction lane assertion total drifted")
    zero_dim_interaction_handles = zero_dim_interaction_focus.get("lane_handles")
    if (
        not isinstance(zero_dim_interaction_handles, dict)
        or zero_dim_interaction_handles.get("catch2_target")
        != "umbra_zero_dimensional_regional_interaction_catch2"
        or zero_dim_interaction_handles.get("ctest_label")
        != "zero-dimensional-regional-interaction"
    ):
        raise AssertionError("zero-dimensional regional-interaction lane handles drifted")

    focused_positive_interaction = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-default-region-interaction-routing-focused-integration"
        ),
        None,
    )
    if not isinstance(focused_positive_interaction, dict):
        raise AssertionError("focused positive-dimensional regional-interaction row is absent")
    if query_rti_work.source_location_text(focused_positive_interaction) != (
        "cpp/tests/default_region_interaction_routing_catch2.cpp:89"
    ):
        raise AssertionError("focused positive-dimensional interaction source pointer drifted")
    if focused_positive_interaction.get("assertions") != 50:
        raise AssertionError("focused positive-dimensional interaction assertion count drifted")
    if len(focused_positive_interaction.get("lab_requirement_ids", [])) != 8:
        raise AssertionError("focused positive-dimensional interaction requirement mapping drifted")
    if focused_positive_interaction.get("standard_sections") != [
        "hla-1516.1-2025:clause-9",
        "hla-1516.1-2025:clause-9.1.3.3",
        "hla-1516.1-2025:clause-9.1.4",
        "hla-1516.1-2025:clause-9.1.8",
    ]:
        raise AssertionError("focused positive-dimensional interaction section mapping drifted")
    focused_positive_interaction_focus = query_rti_work.focused_lane_result(
        index, tests, "focused-positive-dimensional-regional-interaction", limit=0
    )
    if focused_positive_interaction_focus.get("lane_state") != "complete":
        raise AssertionError("focused positive-dimensional interaction lane should be complete")
    if focused_positive_interaction_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused positive-dimensional interaction mapped count drifted")
    if focused_positive_interaction_focus.get("assertion_count") != 50:
        raise AssertionError("focused positive-dimensional interaction lane assertion total drifted")
    if focused_positive_interaction_focus.get("requirement_count") != 8:
        raise AssertionError("focused positive-dimensional interaction lane requirement total drifted")
    if focused_positive_interaction_focus.get("standard_section_count") != 4:
        raise AssertionError("focused positive-dimensional interaction lane section total drifted")
    focused_positive_interaction_handles = focused_positive_interaction_focus.get("lane_handles")
    if (
        not isinstance(focused_positive_interaction_handles, dict)
        or focused_positive_interaction_handles.get("catch2_target")
        != "umbra_default_region_interaction_routing_catch2"
        or focused_positive_interaction_handles.get("ctest_label")
        != "focused-positive-dimensional-regional-interaction"
    ):
        raise AssertionError("focused positive-dimensional interaction lane handles drifted")

    multi_region_interaction = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-multi-region-interaction-routing-integration"
        ),
        None,
    )
    if not isinstance(multi_region_interaction, dict):
        raise AssertionError("multi-region interaction row is absent")
    if query_rti_work.source_location_text(multi_region_interaction) != (
        "cpp/tests/multi_region_interaction_routing_catch2.cpp:106"
    ):
        raise AssertionError("multi-region interaction source pointer drifted")
    if multi_region_interaction.get("assertions") != 107:
        raise AssertionError("multi-region interaction assertion count drifted")
    if len(multi_region_interaction.get("lab_requirement_ids", [])) != 7:
        raise AssertionError("multi-region interaction requirement mapping drifted")
    if multi_region_interaction.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.3.3",
        "hla-1516.1-2025:clause-9.1.4",
    ]:
        raise AssertionError("multi-region interaction section mapping drifted")
    multi_region_interaction_focus = query_rti_work.focused_lane_result(
        index, tests, "multi-region-regional-interaction", limit=0
    )
    if multi_region_interaction_focus.get("lane_state") != "complete":
        raise AssertionError("multi-region interaction lane should be complete")
    if multi_region_interaction_focus.get("mapped_test_count") != 1:
        raise AssertionError("multi-region interaction mapped count drifted")
    if multi_region_interaction_focus.get("assertion_count") != 107:
        raise AssertionError("multi-region interaction lane assertion total drifted")
    if multi_region_interaction_focus.get("requirement_count") != 7:
        raise AssertionError("multi-region interaction lane requirement total drifted")
    if multi_region_interaction_focus.get("standard_section_count") != 2:
        raise AssertionError("multi-region interaction lane section total drifted")
    multi_region_interaction_handles = multi_region_interaction_focus.get("lane_handles")
    if (
        not isinstance(multi_region_interaction_handles, dict)
        or multi_region_interaction_handles.get("catch2_target")
        != "umbra_multi_region_interaction_routing_catch2"
        or multi_region_interaction_handles.get("ctest_label")
        != "multi-region-regional-interaction"
    ):
        raise AssertionError("multi-region interaction lane handles drifted")

    match_kind, trace_matches = query_rti_work.resolve_trace_query(
        tests, TARGET_TEST
    )
    if match_kind != "test" or len(trace_matches) != 1:
        raise AssertionError(
            f"exact trace handle did not resolve deterministically: {match_kind}, "
            f"{len(trace_matches)} matches"
        )

    # Developers normally have the printed subsection number at hand, not the
    # longer corpus key.  Keep the three supported spellings equivalent so the
    # section, trace, and reverse-matrix entry points remain easy to query.
    for section_query in (
        "6.8.4",
        "clause-6.8.4",
        "hla-1516.1-2025:clause-6.8.4",
    ):
        section_matches = query_rti_work.select_tests(
            tests, section_query, "section"
        )
        if not section_matches:
            raise AssertionError(
                f"section lookup did not resolve {section_query!r}"
            )
        section_kind, section_trace_matches = query_rti_work.resolve_trace_query(
            tests, section_query
        )
        if section_kind != "section" or not section_trace_matches:
            raise AssertionError(
                f"trace section lookup did not resolve {section_query!r}"
            )
        matrix_kind, matrix_matches = query_rti_work.resolve_matrix_query(
            index, tests, section_query
        )
        if matrix_kind != "section" or not matrix_matches:
            raise AssertionError(
                f"matrix section lookup did not resolve {section_query!r}"
            )

    # The implementation plan is a second source of navigation context, not
    # another prose search surface.  Protect the heading-only filter so a
    # resume can select a section and line without loading the full document.
    plan_sections = query_rti_work.implementation_plan_sections(
        query_rti_work.DEFAULT_IMPLEMENTATION_PLAN
    )
    if not plan_sections:
        raise AssertionError("implementation-plan heading index is empty")
    if any(not section.get("path") for section in plan_sections):
        raise AssertionError("implementation-plan headings lost breadcrumb paths")
    plan_matches, plan_shown = query_rti_work.filtered_plan_sections(
        plan_sections,
        "current indexed",
        2,
    )
    if not plan_matches or len(plan_shown) != min(2, len(plan_matches)):
        raise AssertionError("filtered implementation-plan heading query is not bounded")
    if not all("current indexed" in section.get("path", "").casefold() for section in plan_shown):
        raise AssertionError("filtered plan headings do not carry the requested breadcrumb")
    _, all_plan_rows = query_rti_work.filtered_plan_sections(plan_sections, None, 0)
    if len(all_plan_rows) != len(plan_sections):
        raise AssertionError("unbounded implementation-plan outline dropped headings")

    print(
        "query-rti-work: direct requirement-to-2025-section matrix regression "
        f"passed ({len(pairs)} mapped requirements)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
