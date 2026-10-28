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
Roadmap family/lane rows must retain copyable case/trace and grouped matrix
handles for both their representative test and indexed ``next_test`` pointer,
so a roadmap lookup cannot strand a contributor at a prose-only title.
The separate Requirements Lab issue ledger is also checked so a known
extraction defect remains queryable without becoming a silent mapping rewrite.
Requirement and canonical-section reverse lookups retain a bounded uncovered
corpus fallback when no Catch2 row exists, so a new case can start from an
exact standard anchor.
The compact ``resume`` projection is checked separately from the richer
dashboard: it must retain one bounded next choice and stable execution
handles without leaking the historical family/test inventory into a normal
first-read.
The family lane inventory also has a focused projection: it must use the
roadmap family's explicit ``focused_lane_tags`` aliases so normal lane
selection does not expand across broad cross-cutting taxonomy tags.
The matrix requirement- and subsection-centric crosswalks are also checked so
standards review can pivot the same direct pairs without expanding every case.
The indexed discovery search accepts multiple terms without requiring a broad
repository scan; its default AND behavior and explicit ``--any-term`` escape
remain stable here.
The regional association failure row is checked as a representative direct
mapping across its current §9.6, §9.7.5, §11.5, and §11.5.2 clauses without
pinning mutable Requirements-Lab candidate IDs. The ownership-transfer row is
checked separately for its narrower §9.1.3.2 cancellation boundary; it does
not claim the broader §9.6 non-restoration behavior.
"""

# ruff: noqa: TRY004
# These assertions report broken query-result contracts, not bad caller inputs.
from __future__ import annotations

import json
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

SCRIPT_ROOT = Path(__file__).resolve().parent
REPOSITORY_ROOT = SCRIPT_ROOT.parent
sys.path.insert(0, str(SCRIPT_ROOT))

import query_rti_work
from completion_ledger_source import (
    completion_ledger_source_pointer_findings,
)
from query_rti_work_regression_context import (
    FamilyMatrixInputs,
    LaneCatalogInputs,
    LaneSearchInputs,
    MappingRowInputs,
)
from query_rti_work_regression_family_matrix import run_family_matrix
from query_rti_work_regression_lane_search import run_lane_search
from query_rti_work_regression_lanes import run_lane_catalog
from query_rti_work_regression_mappings import run_mapping_rows
from query_rti_work_regression_support import (
    TARGET_TEST,
    assert_no_duplicate_json_keys,
    expected_family_gap_head,
)

_LIVE_SOURCE_LOCATION_TEXT = query_rti_work.source_location_text


class _SourceLocationText(str):
    """Compare live source pointers by path when an old line moved.

    ``query_rti_work`` derives the line from the current ``TEST_CASE``
    declaration.  The regression fixture intentionally keeps historical
    source strings for orientation, but inserting a case earlier in the same
    translation unit must not make every mapping check fail.  A path change,
    missing location, or malformed line remains a hard failure.
    """

    def __eq__(self, other: object) -> bool:
        if isinstance(other, str) and str.__eq__(self, other):
            return True
        if not isinstance(other, str):
            return False
        actual_path, separator, actual_line = self.rpartition(":")
        expected_path, expected_separator, _ = other.rpartition(":")
        return bool(
            separator
            and expected_separator
            and actual_path
            and actual_path == expected_path
            and actual_line.isdigit()
        )

    def __ne__(self, other: object) -> bool:
        return not self.__eq__(other)


def _live_source_location_text(test: query_rti_work.JsonObject) -> _SourceLocationText:
    return _SourceLocationText(_LIVE_SOURCE_LOCATION_TEXT(test))


# Keep all existing pointer assertions future-proof without weakening the
# query tool itself; the CLI still emits the exact live path:line location.
query_rti_work.source_location_text = _live_source_location_text


MAPPED_CORE_CASES = {
    "Embedded federate lookup services preserve departed designator identities within the joined federation": {
        "mapping_id": "umbra-federate-lookup-designator-identity",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-10.3.3",
            "hla-1516.1-2025:clause-4.11",
        },
        "source_location": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp:16691",
        "assertions": 33,
    },
    "Embedded asynchronous delivery gates receive-order callbacks by temporal state": {
        "mapping_id": "rti.service.asynchronous-delivery",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-8.15.3",
            "hla-1516.1-2025:clause-8.16.5",
        },
        "source_location": "cpp/tests/ieee1516_2025_asynchronous_delivery_temporal_state_catch2.cpp:5",
        "assertions": 35,
    },
    "Embedded Time Advance Request changes logical time only at Time Advance Grant dispatch": {
        "mapping_id": "rti.service.time-advance-request",
        "requirements": 9,
        "sections": {
            "hla-1516.1-2025:clause-8",
            "hla-1516.1-2025:clause-8.1.2",
            "hla-1516.1-2025:clause-8.14",
            "hla-1516.1-2025:clause-8.18.1",
            "hla-1516.1-2025:clause-8.8",
        },
        "source_location": "cpp/tests/ieee1516_2025_embedded_time_management_catch2.cpp:5",
        "assertions": 40,
    },
    "Embedded time-role services keep enable requests callback-gated before TSO support": {
        "mapping_id": "rti.service.time-role-control",
        "requirements": 9,
        "sections": {
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.3.1",
            "hla-1516.1-2025:clause-8.4",
            "hla-1516.1-2025:clause-8.5",
            "hla-1516.1-2025:clause-8.7",
            "hla-1516.1-2025:clause-8.8",
        },
        "source_location": "cpp/tests/ieee1516_2025_time_role_enable_callback_gating_catch2.cpp:4",
        "assertions": 51,
    },
    "Embedded Modify Lookahead applies increases immediately and decreases gradually": {
        "mapping_id": "rti.service.modify-lookahead",
        "requirements": 6,
        "sections": {
            "hla-1516.1-2025:clause-8.14.4",
            "hla-1516.1-2025:clause-8.20.4",
        },
        "source_location": "cpp/tests/ieee1516_2025_modify_lookahead_increase_and_gradual_decrease_catch2.cpp:4",
        "assertions": 38,
    },
    "RTIambassador requests Flush Queue through a configured process endpoint": {
        "mapping_id": "time-management.process.flush-queue-request",
        "requirements": 4,
        "sections": {
            "hla-1516.1-2025:clause-8.12",
            "hla-1516.1-2025:clause-8.12.3",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_flush_queue_catch2.cpp:459",
        "assertions": 16,
    },
    "RTIambassadors preserve actual and optimistic Flush Queue boundaries through a configured process endpoint": {
        "mapping_id": "time-management.process.flush-queue-galt-frontier",
        "requirements": 19,
        "sections": {
            "hla-1516.1-2025:clause-10.59.4",
            "hla-1516.1-2025:clause-4",
            "hla-1516.1-2025:clause-5.1.3",
            "hla-1516.1-2025:clause-6.12",
            "hla-1516.1-2025:clause-6.13",
            "hla-1516.1-2025:clause-8.12",
            "hla-1516.1-2025:clause-8.12.3",
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.5",
            "hla-1516.1-2025:clause-8.6",
            "hla-1516.1-2025:clause-8.8",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_process_flush_queue_galt_frontier_catch2.cpp:459",
        "assertions": 57,
        "primary_lane": "process-flush-queue-galt-frontier",
    },
    "RTIambassadors deliver multiple queued timestamped process messages in order through Flush Queue": {
        "mapping_id": "time-management.process.flush-queue-multiple-records",
        "requirements": 22,
        "sections": {
            "hla-1516.1-2025:clause-10.59.4",
            "hla-1516.1-2025:clause-4",
            "hla-1516.1-2025:clause-5.1.3",
            "hla-1516.1-2025:clause-6.12",
            "hla-1516.1-2025:clause-6.13",
            "hla-1516.1-2025:clause-8.12",
            "hla-1516.1-2025:clause-8.12.3",
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.22.3",
            "hla-1516.1-2025:clause-8.5",
            "hla-1516.1-2025:clause-8.6",
            "hla-1516.1-2025:clause-8.8",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_process_flush_queue_multiple_records_catch2.cpp:459",
        "assertions": 110,
        "api_surfaces": 23,
        "primary_lane": "process-flush-queue-multiple-records",
    },
    "Embedded Flush Queue Request flushes queued TSO and reports optimistic time": {
        "mapping_id": "rti.service.flush-queue-request",
        "requirements": 10,
        "sections": {
            "hla-1516.1-2025:clause-8",
            "hla-1516.1-2025:clause-8.12",
            "hla-1516.1-2025:clause-8.12.3",
            "hla-1516.1-2025:clause-8.13",
        },
        "source_location": "cpp/tests/flush_queue_request_optimistic_time_catch2.cpp:157",
        "assertions": 70,
    },
    "Embedded constrained TAR waits for GALT and is released by a regulator advance": {
        "mapping_id": "m70.embedded-constrained-tar-galt-regulator-advance",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-8",
            "hla-1516.1-2025:clause-8.1.6",
        },
        "source_location": "cpp/tests/ieee1516_2025_embedded_time_management_catch2.cpp:267",
        "assertions": 30,
    },
    "Embedded Query GALT and Query LITS observe other regulator time and pending advances": {
        "mapping_id": "time-management.query-galt-lits",
        "requirements": 5,
        "sections": {
            "hla-1516.1-2025:clause-8",
            "hla-1516.1-2025:clause-8.1.5",
            "hla-1516.1-2025:clause-8.1.6",
            "hla-1516.1-2025:clause-8.17",
            "hla-1516.1-2025:clause-8.19",
        },
        "source_location": "cpp/tests/ieee1516_2025_query_galt_lits_other_regulator_pending_advances_catch2.cpp:4",
        "assertions": 35,
    },
    "RTIambassador queries GALT and LITS through a configured process endpoint": {
        "mapping_id": "time-management.process.query-galt-lits",
        "requirements": 5,
        "sections": {
            "hla-1516.1-2025:clause-8",
            "hla-1516.1-2025:clause-8.1.5",
            "hla-1516.1-2025:clause-8.1.6",
            "hla-1516.1-2025:clause-8.17",
            "hla-1516.1-2025:clause-8.19",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_time_management_catch2.cpp:457",
        "assertions": 12,
    },
    "RTIambassador enables time constrained through a configured process endpoint": {
        "mapping_id": "time-management.process.enable-time-constrained",
        "requirements": 3,
        "sections": {
            "hla-1516.1-2025:clause-8.5",
            "hla-1516.1-2025:clause-8.6",
            "hla-1516.1-2025:clause-8.7",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_time_management_catch2.cpp:549",
        "assertions": 13,
    },
    "RTIambassador requests time advance through a configured process endpoint": {
        "mapping_id": "time-management.process.time-advance-request",
        "requirements": 4,
        "sections": {
            "hla-1516.1-2025:clause-8.14",
            "hla-1516.1-2025:clause-8.18.1",
            "hla-1516.1-2025:clause-8.8",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_time_management_catch2.cpp:641",
        "assertions": 14,
    },
    "RTIambassador rejects a backward time advance through a configured process endpoint": {
        "mapping_id": "time-management.process.time-advance-rejection",
        "requirements": 4,
        "sections": {
            "hla-1516.1-2025:clause-8.14",
            "hla-1516.1-2025:clause-8.18.1",
            "hla-1516.1-2025:clause-8.8",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_time_advance_rejection_catch2.cpp:458",
        "assertions": 12,
    },
    "RTIambassador rejects an incompatible logical time through a configured process endpoint": {
        "mapping_id": "time-management.process.time-advance-invalid-time",
        "requirements": 1,
        "sections": {
            "hla-1516.1-2025:clause-8.8",
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
            "hla-1516.1-2025:clause-8.8",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_time_regulation_pending_catch2.cpp:514",
        "assertions": 15,
    },
    "RTIambassador rejects a process time advance while time constrained enable is pending": {
        "mapping_id": "time-management.process.time-advance-time-constrained-pending",
        "requirements": 3,
        "sections": {
            "hla-1516.1-2025:clause-8.5",
            "hla-1516.1-2025:clause-8.6",
            "hla-1516.1-2025:clause-8.8",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_time_constrained_pending_catch2.cpp:458",
        "assertions": 14,
    },
    "RTIambassador rejects malformed logical-time encoding through a configured process endpoint": {
        "mapping_id": "time-management.process.time-advance-malformed-encoding",
        "requirements": 1,
        "sections": {
            "hla-1516.1-2025:clause-8.8",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_time_advance_malformed_encoding_catch2.cpp:471",
        "assertions": 9,
    },
    "RTIambassadors coordinate deferred process time advances through the federation scheduler": {
        "mapping_id": "time-management.process.time-advance-federation-scheduler",
        "requirements": 4,
        "sections": {
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.5",
            "hla-1516.1-2025:clause-8.6",
            "hla-1516.1-2025:clause-8.8",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_time_advance_federation_scheduler_catch2.cpp:458",
        "assertions": 30,
    },
    "RTIambassadors deliver a deferred timestamped process interaction before the grant": {
        "mapping_id": "time-management.process.tso-interaction-before-grant",
        "requirements": 20,
        "sections": {
            "hla-1516.1-2025:clause-10.59.4",
            "hla-1516.1-2025:clause-4",
            "hla-1516.1-2025:clause-5.1.3",
            "hla-1516.1-2025:clause-6.12",
            "hla-1516.1-2025:clause-6.13",
            "hla-1516.1-2025:clause-8",
            "hla-1516.1-2025:clause-8.1.5",
            "hla-1516.1-2025:clause-8.1.6",
            "hla-1516.1-2025:clause-8.17",
            "hla-1516.1-2025:clause-8.19",
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.5",
            "hla-1516.1-2025:clause-8.6",
            "hla-1516.1-2025:clause-8.8",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_tso_interaction_before_grant_catch2.cpp:458",
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
            "hla-1516.1-2025:clause-8.5",
            "hla-1516.1-2025:clause-8.6",
            "hla-1516.1-2025:clause-8.8",
        },
        "source_location": "cpp/tests/process_federation_service_tso_attribute_before_grant_catch2.cpp:78",
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
            "hla-1516.1-2025:clause-8",
            "hla-1516.1-2025:clause-8.1.5",
            "hla-1516.1-2025:clause-8.1.6",
            "hla-1516.1-2025:clause-8.17",
            "hla-1516.1-2025:clause-8.19",
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.5",
            "hla-1516.1-2025:clause-8.6",
            "hla-1516.1-2025:clause-8.8",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_tso_attribute_before_grant_catch2.cpp:457",
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
            "hla-1516.1-2025:clause-8.1.6",
            "hla-1516.1-2025:clause-8.17",
            "hla-1516.1-2025:clause-8.19",
            "hla-1516.1-2025:clause-8.2",
            "hla-1516.1-2025:clause-8.22.3",
            "hla-1516.1-2025:clause-8.5",
            "hla-1516.1-2025:clause-8.6",
            "hla-1516.1-2025:clause-8.8",
        },
        "source_location": "cpp/tests/ieee1516_2025_connection_process_tso_attribute_retraction_before_callback_catch2.cpp:366",
        "assertions": 51,
        "primary_lane": "process-tso-attribute-retraction-before-callback",
    },
    "Embedded support switches are seeded per federate and retain static FDD policy": {
        "mapping_id": "rti.service.support-switch-state",
        "requirements": 8,
        "sections": {
            "hla-1516.1-2025:clause-10.44",
            "hla-1516.1-2025:clause-10.45.3",
            "hla-1516.1-2025:clause-10.46.6",
            "hla-1516.1-2025:clause-10.48.1",
            "hla-1516.1-2025:clause-10.50.6",
            "hla-1516.1-2025:clause-10.55.1",
            "hla-1516.1-2025:clause-8.1.8",
            "hla-1516.1-2025:clause-9.1.7",
        },
        "source_location": "cpp/tests/support_switch_state_catch2.cpp:4",
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
        "assertions": 36,
    },
    "Embedded Create Federation Execution accepts a validated explicit MIM path": {
        "mapping_id": "rti.service.create-federation-execution-with-mim",
        "requirements": 1,
        "sections": {
            "hla-1516.1-2025:clause-4.5.5",
        },
        "source_location": "cpp/tests/ieee1516_2025_explicit_mim_create_catch2.cpp:4",
        "assertions": 6,
    },
    "Embedded federation shares the static Advisories Use Known Class switch": {
        "mapping_id": "rti.service.get-advisories-use-known-class-switch",
        "requirements": 2,
        "sections": {
            "hla-1516.1-2025:clause-10.55.1",
        },
        "source_location": "cpp/tests/advisories_use_known_class_switch_catch2.cpp:4",
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
        "requirements": 6,
        "sections": {
            "hla-1516.1-2025:clause-9.1.1",
            "hla-1516.1-2025:clause-9.1.2",
        },
        "source_location": "cpp/tests/dimension_lookup_catch2.cpp:53",
        "assertions": 51,
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
    "Embedded order type control captures per-federate defaults, instance overrides, and publisher interaction order": {
        "mapping_id": "rti.service.order-type-control",
        "requirements": 4,
        "sections": {
            "hla-1516.1-2025:clause-8.1.4",
            "hla-1516.1-2025:clause-8.24.4",
            "hla-1516.1-2025:clause-8.26.4",
        },
        "source_location": "cpp/tests/order_type_control_catch2.cpp:170",
        "assertions": 104,
    },
    "Embedded declaration relevance advisories follow ordinary 2025 publication and subscription transitions": {
        "mapping_id": "rti.service.declaration-relevance-advisories",
        "requirements": 15,
        "sections": {
            "hla-1516.1-2025:clause-10.1.3",
            "hla-1516.1-2025:clause-5.8",
            "hla-1516.1-2025:clause-5.10.2",
            "hla-1516.1-2025:clause-5.14.3",
            "hla-1516.1-2025:clause-5.15.3",
            "hla-1516.1-2025:clause-5.16.5",
            "hla-1516.1-2025:clause-5.17.6",
        },
        "source_location": "cpp/tests/declaration_relevance_advisory_catch2.cpp:91",
        "assertions": 91,
    },
    "Embedded regional declaration relevance advisories follow active subscriptions": {
        "mapping_id": "rti.service.regional-declaration-relevance-advisories",
        "requirements": 13,
        "sections": {
            "hla-1516.1-2025:clause-9.8",
            "hla-1516.1-2025:clause-5.8",
            "hla-1516.1-2025:clause-5.10.2",
            "hla-1516.1-2025:clause-5.14.3",
            "hla-1516.1-2025:clause-5.15.3",
            "hla-1516.1-2025:clause-5.16.5",
            "hla-1516.1-2025:clause-5.17.6",
        },
        "source_location": "cpp/tests/regional_declaration_relevance_advisory_catch2.cpp:122",
        "assertions": 61,
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
        "requirements": 5,
        "sections": {
            "hla-1516.1-2025:clause-9.1.1",
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
        "source_location": "cpp/tests/federation_registry_catch2.cpp:559",
        "assertions": 61,
        "requirements": 17,
        "sections": 8,
        "lane": "process-restart-pending-attribute-value-update",
    },
    "Embedded public fresh-registry restore rebinds pending object-instance Request Attribute Value Update through HLA_EVOKED and HLA_IMMEDIATE": {
        "source_location": "cpp/tests/ieee1516_2025_public_pending_attribute_value_update_restore_catch2.cpp:6",
        "assertions": 61,
        "requirements": 17,
        "sections": 8,
        "lane": "public-process-restart-pending-attribute-value-update",
    },
    "Filesystem fresh-registry restore rebinds a pending object-class Request Attribute Value Update": {
        "source_location": "cpp/tests/federation_registry_catch2.cpp:721",
        "assertions": 61,
        "requirements": 12,
        "sections": 6,
        "lane": "process-restart-class-pending-attribute-value-update",
    },
    "Embedded public fresh-registry restore rebinds pending object-class Request Attribute Value Update through HLA_EVOKED and HLA_IMMEDIATE": {
        "source_location": "cpp/tests/ieee1516_2025_public_class_pending_attribute_value_update_restore_catch2.cpp:6",
        "assertions": 61,
        "requirements": 12,
        "sections": 6,
        "lane": "public-process-restart-class-pending-attribute-value-update",
    },
    "Filesystem fresh-registry restore rebinds a pending regional object-class Request Attribute Value Update": {
        "source_location": "cpp/tests/ieee1516_2025_federation_registry_pending_regional_object_class_attribute_value_update_catch2.cpp:3",
        "assertions": 89,
        "requirements": 11,
        "sections": 6,
        "lane": "process-restart-regional-pending-attribute-value-update-negative",
    },
    "Federation state images round-trip canonical control, temporal, object, and interaction declaration state": {
        "source_location": "cpp/tests/ieee1516_2025_federation_state_image_codec_catch2.cpp:39",
        "assertions": 201,
        "requirements": 33,
        "sections": 28,
        "lane": "membership-lifecycle-state",
    },
    "Federation restore rebinds pending time-role callbacks and fences stale work": {
        "source_location": "cpp/tests/ieee1516_2025_federation_registry_pending_time_role_callback_restore_catch2.cpp:3",
        "assertions": 23,
        "requirements": 4,
        "sections": 4,
        "lane": "time-role",
    },
    "Filesystem state image restores ownership assumption search state and continues with a newly eligible federate": {
        "source_location": "cpp/tests/ieee1516_2025_federation_registry_ownership_assumption_search_filesystem_restart_catch2.cpp:3",
        "assertions": 83,
        "requirements": 13,
        "sections": 8,
        "lane": "process-restart-ownership-assumption-search",
    },
    "Embedded service reporting delivers failed timestamped regional Update Attribute Values invocations through MOM interaction": {
        "source_location": "cpp/tests/ieee1516_2025_timestamped_regional_update_failure_mom_interaction_catch2.cpp:199",
        "assertions": 138,
        "requirements": 5,
        "sections": 3,
        "lane": "timestamped-regional-attribute-update-failure",
    },
    "Embedded service reporting delivers failed ordinary regional Update Attribute Values invocations through MOM interaction": {
        "source_location": "cpp/tests/ieee1516_2025_ordinary_regional_update_failure_mom_catch2.cpp:5",
        "assertions": 119,
        "requirements": 4,
        "sections": 2,
        "lane": "ordinary-regional-attribute-update-failure",
    },
    "Embedded service reporting records regional Update Attribute Values before reflection callback": {
        "source_location": "cpp/tests/ieee1516_2025_ordinary_regional_update_service_report_file_catch2.cpp:5",
        "assertions": 139,
        "requirements": 1,
        "sections": 1,
        "lane": "ordinary-regional-attribute-update-service-report",
    },
    "Embedded queued timestamped Delete Object Instance survives time-regulation disable and re-enable with changed lookahead": {
        "source_location": "cpp/tests/ieee1516_2025_timestamped_delete_object_instance_regulation_reenable_changed_lookahead_catch2.cpp:5",
        "assertions": 52,
        "requirements": 15,
        "sections": 9,
        "lane": "timestamped-object-deletion-regulation-reenable-changed-lookahead",
    },
    "Embedded queued timestamped regional interaction survives time-regulation disable and re-enable with changed lookahead": {
        "source_location": "cpp/tests/ieee1516_2025_timestamped_regional_interaction_regulation_reenable_changed_lookahead_catch2.cpp:5",
        "assertions": 63,
        "requirements": 12,
        "sections": 9,
        "lane": "timestamped-regional-interaction-regulation-reenable-changed-lookahead",
    },
}


MAPPED_SHARED_CASE = {
    "test_case": "Embedded federation-list services dispatch standards reports in both callback models",
    "source_locations": {
        "cpp/tests/federation_listing_catch2.cpp",
        "cpp/tests/ieee1516_2025_federation_management_catch2.cpp",
    },
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


def assert_bounded_discovery_output() -> None:
    """Guard normal CLI usage, not only callers who remember summary flags."""

    for relative, line_limit, character_limit in (
        ("docs/planning/QUERY-CARD.md", 60, 6000),
        ("AGENTS.md", 60, 6000),
    ):
        content = (REPOSITORY_ROOT / relative).read_text(encoding="utf-8")
        if len(content.splitlines()) > line_limit or len(content) > character_limit:
            raise AssertionError(f"{relative} exceeded its bounded entry-point budget")
        if "python tools/query_rti_work.py resume" not in content:
            raise AssertionError(f"{relative} lost its resume entry point")
        if relative == "AGENTS.md" and "docs/planning/QUERY-CARD.md" not in content:
            raise AssertionError("AGENTS.md lost the short command-card link")

    def run(*arguments: str) -> str:
        return subprocess.run(
            [sys.executable, str(SCRIPT_ROOT / "query_rti_work.py"), *arguments],
            cwd=REPOSITORY_ROOT,
            capture_output=True,
            text=True,
            check=True,
        ).stdout

    for command, query in (
        ("requirement", "11.5"),
        ("section", "11.5"),
        ("search", "service-report"),
    ):
        output = run(command, query)
        if len(output.splitlines()) > 100 or len(output) > 20000:
            raise AssertionError(
                f"default {command} output exceeded its discovery budget"
            )
        if "showing 5." not in output or "--limit 0" not in output:
            raise AssertionError(f"default {command} lost truncation metadata")
        if (
            "case: python tools/query_rti_work.py case" not in output
            or "--verbose" not in output
        ):
            raise AssertionError(
                f"default {command} lost exact-case/full-detail handles"
            )
        if command != "search" and " -> hla-1516.1-2025:clause-" not in output:
            raise AssertionError(
                f"default {command} lost direct requirement/subsection pairs"
            )
        if command == "search" and "requirement_section_mappings:" not in output:
            raise AssertionError(
                "default search omitted its bounded direct-pair preview"
            )
        if "standard source:" in output or "    contract:" in output:
            raise AssertionError(f"default {command} leaked full provenance")
        detailed = run(command, query, "--verbose", "--limit", "1")
        if "showing 1." not in detailed or "standard source:" not in detailed:
            raise AssertionError(f"{command} lost explicit detail/limit behavior")
        machine = json.loads(run(command, query, "--json"))
        if machine.get("shown_count") != 20 or len(machine.get("tests", [])) != 20:
            raise AssertionError(f"{command} changed the existing JSON default limit")
        if not any(test.get("requirements") for test in machine["tests"]):
            raise AssertionError(f"{command} removed full JSON requirement records")

    family_pointer = json.loads(
        run("next", "--family", "portable-cpp-tck-2025", "--pointer", "--json")
    )
    pointer_items = family_pointer.get("items", [])
    if len(pointer_items) != 1 or pointer_items[0].get("id") != "portable-cpp-tck-2025":
        raise AssertionError(
            "next --family --pointer returned the global pointer instead of the selected family"
        )

    for arguments, line_limit, character_limit in (
        (("roadmap",), 60, 12000),
        (("plan",), 60, 10000),
        (
            (
                "case",
                "Embedded service reporting delivers federate and object-class lookup return arguments through MOM interaction",
            ),
            32,
            6000,
        ),
        (("--help",), 130, 10000),
    ):
        output = run(*arguments)
        if len(output.splitlines()) > line_limit or len(output) > character_limit:
            raise AssertionError(
                f"default {arguments[0]} output exceeded its discovery budget"
            )
    roadmap = json.loads(run("roadmap", "--json"))
    if roadmap.get("shown_count") != min(12, roadmap.get("count", 0)):
        raise AssertionError("roadmap changed the existing JSON default limit")

    roadmap_summary = run(
        "roadmap", "--status", "open", "--limit", "8", "--summary", "--compact"
    )
    if len(roadmap_summary.splitlines()) > 32 or len(roadmap_summary) > 7000:
        raise AssertionError("compact roadmap summary exceeded its discovery budget")
    if "work=python tools/query_rti_work.py work " not in roadmap_summary:
        raise AssertionError("compact roadmap summary lost exact family work handles")
    if any(
        marker in roadmap_summary
        for marker in ("lane_match:", "next_test_pairs:", "gap_statement=")
    ):
        raise AssertionError("compact roadmap summary leaked family/test inventories")

    resume_summary = run("resume", "--summary", "--compact")
    if len(resume_summary.splitlines()) > 20 or len(resume_summary) > 4000:
        raise AssertionError("compact resume exceeded its first-read discovery budget")
    if "Next slice:" in resume_summary:
        if not all(
            marker in resume_summary
            for marker in ("case=", "ctest_target=", "ctest_filter=")
        ):
            raise AssertionError(
                "compact resume omitted the active case or Catch2 build target"
            )
        if "mapping_seed_only=" in resume_summary:
            if "mapping_seed=" not in resume_summary:
                raise AssertionError(
                    "new-case resume omitted its explicit mapped seed handle"
                )
        elif "contract_mapping=" in resume_summary:
            if not all(
                marker in resume_summary
                for marker in (
                    "case=python tools/cpp_tck.py --scenario ",
                    "mapping_pairs=",
                    "contract_companion=",
                )
            ):
                raise AssertionError(
                    "portable TCK resume omitted its catalog handle or direct contract mappings"
                )
        elif not all(
            marker in resume_summary
            for marker in (
                "case=python tools/query_rti_work.py case ",
                "mapping_pairs=",
            )
        ):
            raise AssertionError(
                "mapped-case resume omitted its exact case query or direct requirement-section pairs"
            )
    elif "work_query=python tools/query_rti_work.py work " not in resume_summary:
        raise AssertionError("compact resume omitted its exact family handoff")
    if any(
        marker in resume_summary
        for marker in (
            "gap_inventory_head=",
            "gap_statement=",
            "coverage inventory only",
        )
    ):
        raise AssertionError(
            "compact resume leaked non-actionable requirement inventory"
        )

    ready_run = subprocess.run(
        [
            sys.executable,
            str(SCRIPT_ROOT / "query_rti_work.py"),
            "ready",
            "--family",
            "transport-and-conformance",
            "--summary",
            "--compact",
        ],
        cwd=REPOSITORY_ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    if ready_run.returncode not in (0, 1):
        raise AssertionError("compact ready handoff failed unexpectedly")
    ready_summary = ready_run.stdout
    if len(ready_summary.splitlines()) > 24 or len(ready_summary) > 4000:
        raise AssertionError("compact ready handoff exceeded its discovery budget")
    if "mapping_seed:" in ready_summary:
        if (
            "seed_matrix_requirement_command: python tools/query_rti_work.py matrix "
            not in ready_summary
            or "seed_matrix_section_command: python tools/query_rti_work.py matrix "
            not in ready_summary
        ):
            raise AssertionError(
                "compact new-case handoff lost its seed requirement/section matrices"
            )
    elif (
        "matrix_requirements=python tools/query_rti_work.py matrix "
        not in ready_summary
    ):
        raise AssertionError("compact ready handoff lost its requirement matrix handle")
    if any(
        marker in ready_summary
        for marker in (
            "gap_inventory_head=",
            "gap_statement=",
            "coverage inventory only",
        )
    ):
        raise AssertionError(
            "compact ready handoff leaked non-actionable requirement inventory"
        )

    item_summary = run("item", "transport-and-conformance", "--summary", "--compact")
    if len(item_summary.splitlines()) > 18 or len(item_summary) > 6000:
        raise AssertionError(
            "compact roadmap-item summary exceeded its discovery budget"
        )
    if 'case=python tools/query_rti_work.py case "' not in item_summary:
        raise AssertionError("compact roadmap-item summary lost exact case handles")
    if "requirement -> standard subsection:" in item_summary:
        raise AssertionError(
            "compact roadmap-item summary leaked case mappings; use its exact case handle"
        )

    regional_case_id = (
        "umbra-cpp-process-multi-recipient-regional-interaction-integration"
    )
    case_summary = run("case", regional_case_id, "--summary", "--compact")
    if (
        "requirement_section_mappings:" not in case_summary
        or "requirement-candidate-content-clauses-09-data-distribution-management-page-225-l18-4 -> hla-1516.1-2025:clause-9.1.7"
        not in case_summary
        or "hla-1516.1-2025:clause-9.12" not in case_summary
        or "matrix_section=" not in case_summary
    ):
        raise AssertionError(
            "exact case handoff lost its direct requirement/subsection crosswalk"
        )
    invalid_region_case_summary = run(
        "case",
        "umbra-cpp-register-object-instance-with-regions-invalid-region",
        "--summary",
        "--compact",
    )
    if (
        "traceability: requirements-mapped" not in invalid_region_case_summary
        or "RL-248" not in invalid_region_case_summary
        or "standard_sections: hla-1516.1-2025:clause-9.9.3"
        not in invalid_region_case_summary
        or "ctest=ctest --test-dir" not in invalid_region_case_summary
    ):
        raise AssertionError(
            "RL-248 registration exception case lost its direct mapping or focused CTest handle"
        )
    trace_summary = run("trace", regional_case_id, "--summary", "--compact")
    if len(trace_summary.splitlines()) > 35 or len(trace_summary) > 6000:
        raise AssertionError("compact exact trace exceeded its discovery budget")
    if "statement:" in trace_summary:
        raise AssertionError("compact exact trace leaked normative prose")
    full_requirement_matrix = run(
        "matrix",
        regional_case_id,
        "--group-by",
        "requirement",
        "--summary",
        "--compact",
        "--limit",
        "20",
    )
    relaxed_ddm_requirement = (
        "requirement-candidate-content-clauses-10-support-services-page-282-l152-47"
    )
    if (
        relaxed_ddm_requirement not in full_requirement_matrix
        or "hla-1516.1-2025:clause-10.55.1" not in full_requirement_matrix
    ):
        raise AssertionError(
            "exact requirement matrix lost the new relaxed-DDM requirement/subsection pair"
        )

    compact_search = run(
        "search",
        "process",
        "regional",
        "interaction",
        "--summary",
        "--compact",
        "--limit",
        "5",
    )
    if len(compact_search.splitlines()) > 20 or len(compact_search) > 5000:
        raise AssertionError("compact search output exceeded its discovery budget")
    if compact_search.count("case: python tools/query_rti_work.py case ") != 5:
        raise AssertionError("compact search output omitted an exact case handle")
    if any(
        label in compact_search
        for label in (
            "requirement -> standard subsection:",
            "cpp api surfaces:",
            "Requirements-Lab API surface:",
        )
    ):
        raise AssertionError(
            "compact search output leaked case detail; use the exact case or matrix query"
        )
    relevance_search = run(
        "search", "multi-region", "interactions", "route", "--summary", "--limit", "5"
    )
    if not relevance_search.splitlines()[2].startswith(
        "- umbra-cpp-multi-region-interaction-routing-integration:"
    ):
        raise AssertionError(
            "search did not rank a title/ID match ahead of cross-field matches"
        )

    active_handoff = (
        query_rti_work.load_json(query_rti_work.DEFAULT_INDEX).get("active_handoff")
        or {}
    )
    active_handoff_family = active_handoff.get("family")
    if not active_handoff_family:
        raise AssertionError("active handoff must declare its roadmap family")
    scoped_ready_result = subprocess.run(
        [
            sys.executable,
            str(SCRIPT_ROOT / "query_rti_work.py"),
            "ready",
            "--family",
            active_handoff_family,
            "--summary",
            "--compact",
        ],
        cwd=REPOSITORY_ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    scoped_ready = scoped_ready_result.stdout
    active_handoff_plan = active_handoff.get("plan_id")
    if str(active_handoff.get("state") or "").casefold() in {
        "complete",
        "completed",
        "closed",
        "done",
        "verified",
    }:
        if f"family_selection: {active_handoff_family}" not in scoped_ready:
            raise AssertionError(
                "completed active handoff did not fall back to bounded family selection"
            )
    elif (
        scoped_ready_result.returncode != 0
        or f"family_selection: {active_handoff_family}" not in scoped_ready
        or not active_handoff_plan
        or f"plan_id: {active_handoff_plan}" not in scoped_ready
        or f"lane: {active_handoff.get('lane')}" not in scoped_ready
        or "state: active-handoff" not in scoped_ready
    ):
        raise AssertionError("scoped ready query lost the current active handoff")
    if "mapping_seed:" in scoped_ready or "seed_trace_command:" in scoped_ready:
        raise AssertionError("compact active handoff leaked mapping-seed details")
    if len(scoped_ready.splitlines()) > 28 or len(scoped_ready) > 5000:
        raise AssertionError(
            "compact ready family output exceeded its discovery budget"
        )
    if any(
        line.startswith(("  requirements=", "  standard_sections=", "  direct_pairs="))
        for line in scoped_ready.splitlines()
    ):
        raise AssertionError(
            "compact ready family output leaked mapping previews; use matrix queries"
        )
    if str(active_handoff.get("state") or "").casefold() in {
        "complete",
        "completed",
        "closed",
        "done",
        "verified",
    }:
        if (
            "matrix_requirements=" not in scoped_ready
            or "matrix_sections=" not in scoped_ready
        ):
            raise AssertionError(
                "bounded family selection lost its on-demand matrix handles"
            )
    elif (
        f'matrix_requirements=python tools/query_rti_work.py matrix "{active_handoff_plan}" --group-by requirement --summary --compact'
        not in scoped_ready
        or f'matrix_sections=python tools/query_rti_work.py matrix "{active_handoff_plan}" --group-by section --summary --compact'
        not in scoped_ready
    ):
        raise AssertionError(
            "compact ready family card lost its on-demand matrix handles"
        )
    scoped_work = run("work", "object-ddm-ownership", "--summary", "--compact")
    if len(scoped_work.splitlines()) > 24 or len(scoped_work) > 4000:
        raise AssertionError("compact work family output exceeded its discovery budget")
    if any(
        line.startswith(
            ("work_requirements:", "work_standard_sections:", "work_direct_pairs:")
        )
        for line in scoped_work.splitlines()
    ):
        raise AssertionError(
            "compact work family output leaked mapping previews; use matrix queries"
        )
    if (
        "work_matrix_requirement:" not in scoped_work
        or "work_matrix_section:" not in scoped_work
    ):
        raise AssertionError(
            "compact work family output lost its on-demand mapping matrix handles"
        )


def main() -> int:
    assert_bounded_discovery_output()
    # The resume entry point must stay bounded even as historical slices grow.
    # Examples belong in the linked catalog, not in the first-read quickstart.
    quickstart = (REPOSITORY_ROOT / "docs/planning/ROADMAP-QUICKSTART.md").read_text(
        encoding="utf-8"
    )
    if len(quickstart.splitlines()) > 80:
        raise AssertionError("roadmap quickstart exceeds its bounded first-read budget")
    if "python tools/query_rti_work.py resume" not in "\n".join(
        quickstart.splitlines()[:12]
    ):
        raise AssertionError("roadmap quickstart must lead with the one-command resume")
    if "QUERY-EXAMPLES.md" not in quickstart:
        raise AssertionError("roadmap quickstart lost the historical-example reference")
    resume_run = subprocess.run(
        [sys.executable, str(SCRIPT_ROOT / "query_rti_work.py"), "resume"],
        cwd=REPOSITORY_ROOT,
        capture_output=True,
        text=True,
        check=True,
    )
    if len(resume_run.stdout.splitlines()) > 24 or len(resume_run.stdout) > 5000:
        raise AssertionError(
            "default resume output exceeded its bounded first-read budget"
        )
    if any(
        label in resume_run.stdout
        for label in ("direct_pairs=", "Deferred seam:", "Verified fix:")
    ):
        raise AssertionError(
            "default resume leaked detail/history; use case or dashboard for those"
        )
    assert_no_duplicate_json_keys(query_rti_work.DEFAULT_INDEX)
    assert_no_duplicate_json_keys(query_rti_work.DEFAULT_PLAN)
    assert_no_duplicate_json_keys(query_rti_work.DEFAULT_LAB_ISSUES)
    index = query_rti_work.load_json(query_rti_work.DEFAULT_INDEX)
    # A planned or proposed handoff must point to an existing build target.
    # Otherwise a bounded resume merely hands the next contributor a new search.
    declared_active_handoff = index.get("active_handoff") or {}
    handoff_target = declared_active_handoff.get("ctest_target")
    target_declaration = re.compile(
        r"\b(?:add_executable|umbra_add_[A-Za-z0-9_]*catch2_target)"
        r"\s*\(\s*([A-Za-z_][A-Za-z0-9_-]*)"
    )
    cmake_sources = [
        REPOSITORY_ROOT / "CMakeLists.txt",
        *(REPOSITORY_ROOT / "cmake").rglob("*.cmake"),
        *(REPOSITORY_ROOT / "packages").rglob("CMakeLists.txt"),
    ]
    declared_executables = {
        target
        for cmake_source in cmake_sources
        for target in target_declaration.findall(
            cmake_source.read_text(encoding="utf-8")
        )
    }
    if handoff_target and handoff_target not in declared_executables:
        raise AssertionError(
            f"active handoff names an undeclared CMake target: {handoff_target}"
        )
    if declared_active_handoff.get("family") == "portable-cpp-tck-2025":
        scenario_id = declared_active_handoff.get("plan_id")
        scenario_command = (declared_active_handoff.get("commands") or {}).get(
            "scenario"
        )
        if not scenario_id or scenario_command != scenario_id:
            raise AssertionError(
                "portable C++ TCK handoffs must identify the scenario separately from its CMake target"
            )
    plan = query_rti_work.load_json(query_rti_work.DEFAULT_PLAN)
    bundle = query_rti_work.load_json(query_rti_work.DEFAULT_BUNDLE)
    cpp_tck_catalog = query_rti_work.load_json(query_rti_work.DEFAULT_CPP_TCK_CATALOG)
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
    active_handoff = query_rti_work.indexed_active_handoff_record(index, tests)
    declared_handoff = index.get("active_handoff")
    mapping_seed_plan = next(
        (
            test
            for test in tests
            if isinstance(test, dict)
            and isinstance(declared_handoff, dict)
            and test.get("id") == declared_handoff.get("mapping_seed_plan_id")
        ),
        None,
    )
    handoff_plan_id = (
        declared_handoff.get("plan_id") if isinstance(declared_handoff, dict) else None
    )
    handoff_plan = next(
        (
            test
            for test in tests
            if isinstance(test, dict)
            and (
                test.get("id") == handoff_plan_id
                or test.get("test_case") == (declared_handoff or {}).get("test_case")
            )
        ),
        None,
    )
    resume_handoff = subprocess.run(
        [
            sys.executable,
            str(SCRIPT_ROOT / "query_rti_work.py"),
            "resume",
            "--summary",
            "--compact",
        ],
        cwd=REPOSITORY_ROOT,
        capture_output=True,
        text=True,
        check=True,
    ).stdout
    handoff_is_completed = isinstance(declared_handoff, dict) and str(
        declared_handoff.get("state") or ""
    ).casefold() in {"complete", "completed", "closed", "done", "verified"}
    if isinstance(handoff_plan, dict) and not handoff_is_completed:
        if (
            not isinstance(active_handoff, dict)
            or not active_handoff.get("found")
            or active_handoff.get("handoff_kind") != "existing-case"
            or active_handoff.get("plan_id") != handoff_plan.get("id")
            or active_handoff.get("test_case") != handoff_plan.get("test_case")
        ):
            raise AssertionError(
                "an open handoff to an existing planned case was hidden instead of resolved"
            )
        ready_handoff = query_rti_work.ready_slice(
            index,
            tests,
            source_locations,
            requested_family=(
                declared_handoff.get("family")
                if isinstance(declared_handoff, dict)
                else None
            ),
        )
        if (
            not ready_handoff.get("found")
            or ready_handoff.get("state") != "active-handoff"
            or ready_handoff.get("plan_id") != handoff_plan.get("id")
            or ready_handoff.get("test_case") != handoff_plan.get("test_case")
            or ready_handoff.get("source_lane") != declared_handoff.get("lane")
            or ready_handoff.get("source_lane")
            not in query_rti_work.strings(handoff_plan.get("tags"))
            or ready_handoff.get("source_target")
            != declared_handoff.get("source_target")
            or ready_handoff.get("ctest_target") != declared_handoff.get("ctest_target")
            or ready_handoff.get("ctest_filter") != declared_handoff.get("ctest_filter")
            or ready_handoff.get("requirement_section_pair_count", 0) == 0
        ):
            raise AssertionError(
                "the existing-case roadmap handoff lost its exact test, source, direct mapping, or CTest handles"
            )
        if (
            handoff_plan.get("test_case") not in resume_handoff
            or str(declared_handoff.get("lane")) not in resume_handoff
            or "ctest_target=" not in resume_handoff
            or "ctest_filter=" not in resume_handoff
            or "mapping_pairs=" not in resume_handoff
            or "mapping_seed_only=" in resume_handoff
        ):
            raise AssertionError(
                "resume did not surface the active case, its build target, and direct requirement-section pairs"
            )
        existing_case_card = query_rti_work.case_card_record(
            handoff_plan, index, str(handoff_plan.get("id"))
        )
        existing_case_text = query_rti_work.text_case_card(existing_case_card)
        case_locations = existing_case_card.get("source_locations", [])
        if (
            not any(
                isinstance(location, dict)
                and location.get("path") == declared_handoff.get("source_target")
                and (f"source: {location.get('path')}:{location.get('line')}")
                in existing_case_text
                for location in case_locations
            )
            or not str(handoff_plan.get("status") or "").startswith("implemented-")
            or f"status: {handoff_plan.get('status')}" not in existing_case_text
            or "source: <TEST_CASE not declared yet>" in existing_case_text
            or "not recorded; case is not implemented" in existing_case_text
            or "requirement_section_mappings:" not in existing_case_text
        ):
            raise AssertionError(
                "the exact active-case card must retain its implementation status, real source, file target, and direct mapping"
            )
        exact_case_ctest_handles = {
            "umbra-cpp-federation-save-commit-filesystem-process-restart-interaction-publication-unit": "^umbra\\.federation_registry\\.catch2\\.Filesystem state image restores a published interaction declaration in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-interaction-subscription-unit": "^umbra\\.federation_registry\\.catch2\\.Filesystem state image restores an interaction subscription declaration in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-interaction-publication-subscription-unit": "^umbra\\.federation_registry\\.catch2\\.Filesystem state image restores a same-class interaction publication and subscription in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-interaction-multiple-publications-unit": "^umbra\\.ieee1516_2025\\.catch2\\.Filesystem state image restores multiple interaction declaration entries in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-interaction-multi-federate-unit": "^umbra\\.federation_registry\\.catch2\\.Filesystem state image restores independent interaction declarations for multiple federates$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-interaction-transportation-type-override-unit": "^umbra\\.federation_registry\\.catch2\\.Filesystem state image restores a committed interaction transportation-type override in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-interaction-regional-subscription-unit": "^umbra\\.federation_registry\\.catch2\\.Filesystem state image restores a regional interaction subscription in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-regional-interaction-tso-ddm-unit": "^umbra\\.ieee1516_2025\\.catch2\\.Filesystem fresh-registry restore preserves one timestamped regional interaction payload and source-region snapshot$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-directed-interaction-declaration-unit": "^umbra\\.federation_registry\\.catch2\\.Filesystem state image restores a directed interaction publication and subscription for multiple federates$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-directed-interaction-routing-unit": "^umbra\\.ieee1516_2025\\.catch2\\.Filesystem state image restores a directed interaction target and receive-order route$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-attribute-transportation-type-change-unit": "^umbra\\.ieee1516_2025\\.catch2\\.Filesystem state image restores a pending attribute transportation-type change in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-interaction-transportation-type-change-unit": "^umbra\\.ieee1516_2025\\.catch2\\.Filesystem state image restores a pending interaction transportation-type change in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-pending-ownership-acquisition-unit": "^umbra\\.ieee1516_2025\\.catch2\\.Filesystem state image restores pending regular ownership release work in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-pending-ownership-if-available-unit": "^umbra\\.ieee1516_2025\\.catch2\\.Filesystem state image restores pending If Available ownership callback in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-pending-negotiated-owner-confirmation-unit": "^umbra\\.ieee1516_2025\\.catch2\\.Filesystem state image restores pending negotiated owner confirmation in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-pending-negotiated-if-available-owner-confirmation-unit": "^umbra\\.ieee1516_2025\\.catch2\\.Filesystem state image restores pending negotiated If Available owner confirmation in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-negotiated-mixed-confirmation-asymmetric-unit-recovered-source-mapping": "^umbra\\.ieee1516_2025\\.catch2\\.Filesystem state image restores an asymmetric mixed negotiated confirmation in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-negotiated-mixed-confirmation-delivered-unit": "^umbra\\.ieee1516_2025\\.catch2\\.Filesystem state image preserves mixed delivered negotiated confirmations in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-negotiated-if-available-confirmation-delivered-unit": "^umbra\\.ieee1516_2025\\.catch2\\.Filesystem state image preserves delivered negotiated If Available confirmation in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-negotiated-confirmation-delivered-unit": "^umbra\\.ieee1516_2025\\.catch2\\.Filesystem state image preserves delivered negotiated owner confirmation in a fresh registry$",
            "umbra-cpp-federation-save-commit-filesystem-process-restart-malformed-mixed-confirmation-unit": "^umbra\\.ieee1516_2025\\.catch2\\.Federation restore rejects malformed mixed negotiated confirmation images$",
        }
        for case_id, ctest_filter in exact_case_ctest_handles.items():
            case = next(
                (
                    test
                    for test in tests
                    if isinstance(test, dict) and test.get("id") == case_id
                ),
                None,
            )
            if not isinstance(case, dict):
                raise AssertionError(f"the case {case_id} must remain indexed")
            card = query_rti_work.case_card_record(case, index, case_id)
            expected_ctest = (
                "ctest --test-dir <build-dir> -C Debug -R "
                f'"{ctest_filter}" --output-on-failure'
            )
            if card.get("commands", {}).get("ctest") != expected_ctest:
                raise AssertionError(
                    f"case card {case_id} must use its exact registered CTest selector, not an unrelated lane prefix"
                )
    elif handoff_is_completed:
        ready_handoff = query_rti_work.ready_slice(
            index,
            tests,
            source_locations,
            requested_family=(
                declared_handoff.get("family")
                if isinstance(declared_handoff, dict)
                else None
            ),
        )
        if active_handoff is not None or (
            isinstance(ready_handoff, dict)
            and ready_handoff.get("plan_id")
            == (
                handoff_plan.get("id")
                if isinstance(handoff_plan, dict)
                else handoff_plan_id
            )
        ):
            raise AssertionError("completed active handoff was reoffered as next work")
    elif declared_handoff is None:
        latest_slice = (index.get("mapping") or {}).get("latest_completed_slice")
        latest_plan_id = (
            latest_slice.get("plan_id") if isinstance(latest_slice, dict) else None
        )
        latest_plan = next(
            (
                test
                for test in tests
                if isinstance(test, dict) and test.get("id") == latest_plan_id
            ),
            None,
        )
        latest_summary = (
            query_rti_work.test_summary_data(latest_plan)
            if isinstance(latest_plan, dict)
            else {}
        )
        latest_locations = latest_summary.get("source_locations", [])
        latest_location = (
            latest_locations[0]
            if latest_locations and isinstance(latest_locations[0], dict)
            else None
        )
        latest_source_location = (
            f"{latest_location.get('path')}:{latest_location.get('line')}"
            if latest_location and latest_location.get("path")
            else None
        )
        ready_handoff = query_rti_work.ready_slice(
            index,
            tests,
            source_locations,
            requested_family=(
                latest_slice.get("owner") if isinstance(latest_slice, dict) else None
            ),
        )
        if (
            active_handoff is not None
            or not isinstance(latest_slice, dict)
            or not isinstance(latest_plan, dict)
            or "implemented" not in str(latest_plan.get("status", ""))
            or latest_slice.get("source_location") != latest_source_location
            or latest_summary.get("requirement_section_mapping_count", 0) == 0
            or latest_plan.get("test_case") not in resume_handoff
            or (
                isinstance(ready_handoff, dict)
                and ready_handoff.get("plan_id") == latest_plan_id
            )
        ):
            raise AssertionError(
                "a completed roadmap slice must remain queryable without being reoffered as the active handoff"
            )
    elif (
        isinstance(active_handoff, dict)
        and active_handoff.get("handoff_kind") == "portable-tck-verification"
    ):
        if (
            not isinstance(declared_handoff, dict)
            or active_handoff.get("plan_id") != declared_handoff.get("plan_id")
            or active_handoff.get("plan_id_status") != "indexed"
            or active_handoff.get("source_target")
            != declared_handoff.get("source_target")
            or active_handoff.get("requirement_section_pair_count", 0) == 0
            or not active_handoff.get("api_surfaces")
            or not active_handoff.get("ctest_filter")
            or active_handoff.get("test_case") not in resume_handoff
            or str(active_handoff.get("source_lane")) not in resume_handoff
        ):
            raise AssertionError(
                "portable TCK verification handoff must resolve from its scenario contracts"
            )
    else:
        if (
            not isinstance(active_handoff, dict)
            or not active_handoff.get("found")
            or not isinstance(declared_handoff, dict)
            or active_handoff.get("test_case") != declared_handoff.get("test_case")
            or active_handoff.get("plan_id") != declared_handoff.get("plan_id")
            or active_handoff.get("plan_id_status") != "proposed"
            or active_handoff.get("source_target")
            != declared_handoff.get("source_target")
            or active_handoff.get("source_location")
            != declared_handoff.get("source_location")
            or not isinstance(mapping_seed_plan, dict)
            or active_handoff.get("mapping_seed_test_case")
            != mapping_seed_plan.get("test_case")
            or active_handoff.get("api_surfaces")
            != query_rti_work.strings(
                mapping_seed_plan.get("selected_cpp_api_surface_ids")
                if isinstance(mapping_seed_plan, dict)
                else None
            )
            or bool(declared_handoff.get("api_surfaces"))
            or active_handoff.get("requirement_section_pair_count", 0) == 0
            or not active_handoff.get("ctest_filter")
            or active_handoff.get("test_case") not in resume_handoff
            or f"proposed_plan_id={declared_handoff.get('plan_id')}"
            not in resume_handoff
            or (
                declared_handoff.get("source_location") is not None
                and str(declared_handoff.get("source_location")) not in resume_handoff
            )
            or f'case=python tools/query_rti_work.py case "{declared_handoff.get("plan_id")}"'
            in resume_handoff
            or f'matrix_requirements=python tools/query_rti_work.py matrix "{declared_handoff.get("plan_id")}"'
            in resume_handoff
            or str(active_handoff.get("source_lane")) not in resume_handoff
            or "mapping_seed_only=" not in resume_handoff
        ):
            raise AssertionError(
                "the current new-case handoff must expose its exact source, API, mapping, and seed"
            )
    if (
        isinstance(declared_handoff, dict)
        and declared_handoff.get("family") == "portable-cpp-tck-2025"
        and not handoff_is_completed
    ):
        tck_handoff = query_rti_work.ready_slice(
            index,
            tests,
            source_locations,
            requested_family="portable-cpp-tck-2025",
        )
        expected_catalog_check = (
            f"python tools/cpp_tck.py --scenario {declared_handoff.get('plan_id')}"
        )
        if (
            not isinstance(tck_handoff, dict)
            or (
                tck_handoff.get("check_command")
                or tck_handoff.get("post_mapping_check_command")
            )
            != expected_catalog_check
            or tck_handoff.get("post_mapping_focus_command") is not None
            or "query_rti_work.py check --lane" in str(tck_handoff)
        ):
            raise AssertionError(
                "portable C++ TCK handoffs must validate through the TCK catalog, not Catch2 lanes"
            )
    delay_requirement_ids = {
        "requirement-candidate-content-clauses-08-time-management-page-187-l25-6",
        "requirement-candidate-content-clauses-08-time-management-page-187-l40-11",
        "requirement-candidate-content-clauses-08-time-management-page-187-l46-13",
    }
    for requirement_id in delay_requirement_ids:
        requirement = standard_requirements.get(requirement_id)
        if (
            not isinstance(requirement, dict)
            or requirement.get("clause_id") != "clause-8.1.8"
            or requirement.get("exported_clause_id") is not None
            or requirement.get("canonical_section_override") is not None
        ):
            raise AssertionError(
                f"{requirement_id} lost the corrected Lab §8.1.8 clause attribution"
            )
    delay_case = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-delay-subscription-evaluation-timestamped-interaction-integration"
        ),
        None,
    )
    delay_pairs = (
        query_rti_work.requirement_section_mapping_rows(delay_case)
        if isinstance(delay_case, dict)
        else []
    )
    if (
        not delay_pairs
        or {row.get("standard_section") for row in delay_pairs}
        != {"hla-1516.1-2025:clause-8.1.8"}
        or {row.get("lab_requirement_id") for row in delay_pairs}
        != delay_requirement_ids
    ):
        raise AssertionError(
            "Delay Subscription Evaluation Catch2 mapping must retain all three exact Lab IDs "
            "and resolve each to IEEE 1516.1-2025 §8.1.8"
        )
    delay_section_summary = subprocess.run(
        [
            sys.executable,
            str(SCRIPT_ROOT / "query_rti_work.py"),
            "section",
            "8.1.8",
            "--summary",
            "--compact",
            "--limit",
            "0",
        ],
        check=True,
        capture_output=True,
        text=True,
        cwd=REPOSITORY_ROOT,
    ).stdout
    if (
        "Embedded Delay Subscription Evaluation defers ordinary timestamped interaction eligibility"
        not in delay_section_summary
    ):
        raise AssertionError(
            "§8.1.8 query omitted the ordinary delay-evaluation Catch2 case"
        )
    # The installable process catalog is deliberately a bounded query surface:
    # every package lane must be reachable both through its exact lane handle
    # and through the family work card.  Keep this guard here so adding a new
    # consumer cannot silently make it executable without a copyable query.
    package_fields = (
        ("next_process_package_test", "next_process_package_ctest_filter"),
        (
            "next_process_package_timestamped_test",
            "next_process_package_timestamped_ctest_filter",
        ),
        (
            "next_process_package_parameterized_test",
            "next_process_package_parameterized_ctest_filter",
        ),
        (
            "next_process_package_connection_loss_test",
            "next_process_package_connection_loss_ctest_filter",
        ),
        (
            "next_process_package_connection_loss_delete_objects_test",
            "next_process_package_connection_loss_delete_objects_ctest_filter",
        ),
        (
            "next_process_package_object_registration_test",
            "next_process_package_object_registration_ctest_filter",
        ),
        (
            "next_process_package_named_registration_test",
            "next_process_package_named_registration_ctest_filter",
        ),
        (
            "next_process_package_attribute_update_test",
            "next_process_package_attribute_update_ctest_filter",
        ),
        (
            "next_process_package_directed_retraction_test",
            "next_process_package_directed_retraction_ctest_filter",
        ),
        (
            "next_process_package_federation_save_restore_test",
            "next_process_package_federation_save_restore_ctest_filter",
        ),
        (
            "next_process_package_federation_save_restore_failure_test",
            "next_process_package_federation_save_restore_failure_ctest_filter",
        ),
        (
            "next_process_package_federation_save_restore_abort_test",
            "next_process_package_federation_save_restore_abort_ctest_filter",
        ),
        (
            "next_process_package_federation_save_restore_status_test",
            "next_process_package_federation_save_restore_status_ctest_filter",
        ),
    )
    package_items = [
        item
        for item in index.get("items", [])
        if isinstance(item, dict)
        and any(item.get(test_field) for test_field, _ in package_fields)
    ]
    if len(package_items) != 1:
        raise AssertionError(
            "installable process package handles are not owned by one roadmap item"
        )
    package_item = package_items[0]
    package_handles = (
        index.get("mapping", {}).get("lane_handles", {})
        if isinstance(index.get("mapping"), dict)
        else {}
    )
    if not isinstance(package_handles, dict):
        raise AssertionError("installable process package lane handle map is absent")
    for test_field, label_field in package_fields:
        test_name = package_item.get(test_field)
        label = package_item.get(label_field)
        if not isinstance(test_name, str) or not test_name:
            raise AssertionError(f"missing query handle: {test_field}")
        if not isinstance(label, str) or not label:
            raise AssertionError(f"missing query handle: {label_field}")
        lane = package_handles.get(label)
        if not isinstance(lane, dict) or lane.get("consumer_test") != test_name:
            raise AssertionError(f"package lane {label} is not linked to {test_name}")
    package_work = query_rti_work.indexed_work_slice(
        index,
        tests,
        None,
        source_locations,
    )
    for test_field, _ in package_fields:
        work_field = test_field.removeprefix("next_")
        if not package_work.get(work_field):
            raise AssertionError(f"work card omitted process package lane {work_field}")
    verified_restore = (
        index.get("mapping", {}).get("latest_verified_restore_slice")
        if isinstance(index.get("mapping"), dict)
        else None
    )
    if not isinstance(verified_restore, dict):
        raise AssertionError(
            "latest verified restore slice is absent from the roadmap index"
        )
    verified_plan_ids = verified_restore.get("plan_ids")
    known_plan_ids = {
        test.get("id")
        for test in tests
        if isinstance(test, dict) and isinstance(test.get("id"), str)
    }
    if not isinstance(verified_plan_ids, list) or not verified_plan_ids:
        raise AssertionError("latest verified restore slice has no mapped plan IDs")
    if any(plan_id not in known_plan_ids for plan_id in verified_plan_ids):
        raise AssertionError(
            "latest verified restore slice references an unknown plan ID"
        )
    verification_counts = verified_restore.get("verification")
    if (
        not isinstance(verification_counts, dict)
        or verification_counts.get("focused_runs") != 26
    ):
        raise AssertionError("latest verified restore focused-run count drifted")
    verified_queries = verified_restore.get("queries")
    if not isinstance(verified_queries, dict) or any(
        not isinstance(verified_queries.get(key), str) or not verified_queries.get(key)
        for key in ("representative_case", "public_case", "focus", "matrix", "check")
    ):
        raise AssertionError("latest verified restore query handles are incomplete")
    verified_ctest = verified_restore.get("ctest")
    if not isinstance(verified_ctest, dict) or any(
        not isinstance(verified_ctest.get(key), str) or not verified_ctest.get(key)
        for key in (
            "registry_primary",
            "registry_standalone",
            "public_fresh_registry",
            "standalone_assumption_regression",
        )
    ):
        raise AssertionError("latest verified restore CTest handles are incomplete")
    # The completion ledger records source locations at the time of evidence.
    # Its lines need not track later edits. Both its historical path and the
    # current plan path must remain navigable; a moved file requires review.
    plan_source_locations = {
        row.get("id"): row.get("source_location")
        for row in plan.get("tests", [])
        if isinstance(row, dict)
        and isinstance(row.get("id"), str)
        and isinstance(row.get("source_location"), str)
    }
    reviewed_source_relocations = {
        # The Available Dimensions guard was extracted from the XML composer
        # translation unit into the focused reference-resolution fixture.
        "umbra-cpp-fom-available-dimension-reference-resolution": (
            "cpp/tests/libxml2_fom_composer_catch2.cpp",
            "cpp/tests/fom_composer_reference_resolution_catch2.cpp",
        ),
        "umbra-cpp-process-tso-regional-interaction-callback-gating": (
            "cpp/tests/ieee1516_2025_connection_process_tso_regional_interaction_callback_gating_catch2.cpp",
            "cpp/tests/ieee1516_2025_connection_process_tso_regional_interaction_arrival_catch2.cpp",
        ),
        # 7d3803e0 extracted this same push-receive case into its own fixture.
        "umbra-cpp-confirm-divestiture-process-push-integration": (
            "cpp/tests/attribute_ownership_acquisition_catch2.cpp",
            "cpp/tests/ieee1516_2025_process_confirm_divestiture_push_catch2.cpp",
        ),
    }
    completed_slices = index.get("recent_completed_slices", [])
    historical_source_findings = completion_ledger_source_pointer_findings(
        completed_slices,
        plan_source_locations,
        reviewed_source_relocations,
        REPOSITORY_ROOT,
    )
    if historical_source_findings:
        raise AssertionError(
            "completion-ledger source provenance is not navigable: "
            + "; ".join(historical_source_findings[:4])
        )
    source_unlocated_history = {
        "plan_id": "source-unlocated-regression",
        "source_location": "",
    }
    if completion_ledger_source_pointer_findings(
        [source_unlocated_history],
        {
            "source-unlocated-regression": "cpp/tests/fom_composer_reference_resolution_catch2.cpp:1"
        },
        {},
        REPOSITORY_ROOT,
    ):
        raise AssertionError("source-unlocated completion history was not audit-only")
    regional_history = next(
        row
        for row in completed_slices
        if isinstance(row, dict)
        and row.get("plan_id")
        == "umbra-cpp-process-tso-regional-interaction-callback-gating"
    )
    unreviewed_history = dict(regional_history)
    unreviewed_history["source_location"] = (
        "cpp/tests/ieee1516_2025_connection_timestamped_delete_object_instance_catch2.cpp:457"
    )
    if not any(
        "unreviewed source relocation" in finding
        for finding in completion_ledger_source_pointer_findings(
            [unreviewed_history],
            plan_source_locations,
            {},
            REPOSITORY_ROOT,
        )
    ):
        raise AssertionError(
            "completion-ledger source relocation counterexample was not rejected"
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
        raise AssertionError(f"expected one exact target test, found {len(matches)}")
    target = matches[0]
    matrix = query_rti_work.matrix_summary_data(target, index)
    pairs = matrix.get("requirement_section_mappings")
    if not isinstance(pairs, list) or not pairs:
        raise AssertionError("matrix row has no direct requirement/section mappings")
    if matrix.get("requirement_section_mapping_count") != len(pairs):
        raise AssertionError("matrix mapping count does not match its mapping rows")

    expected_requirement_ids = set(target.get("lab_requirement_ids", []))
    observed_requirement_ids = {
        row.get("lab_requirement_id") for row in pairs if isinstance(row, dict)
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
        row.get("unresolved") or not row.get("standard_section")
        for row in pairs
        if isinstance(row, dict)
    ):
        raise AssertionError(
            "the target's resolved mappings unexpectedly contain an unresolved row"
        )

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
        "8.8",
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
    if mapping_line.count(" -> ") != 2 or "clause-8.8" not in mapping_line:
        raise AssertionError(
            "section reverse-summary did not narrow direct pairs to the requested subsection"
        )
    if len(reverse_summary_text.splitlines()) >= len(summary_text.splitlines()):
        raise AssertionError(
            "section reverse-summary did not stay smaller than the full test summary"
        )

    # Historical aggregate rows must hand a contributor directly to their
    # focused replacements.  A stale source pointer is useful audit history,
    # but it must not force another repository-wide search to find the live
    # cases that superseded it.
    superseded = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-transportation-type-change-integration"
        ),
        None,
    )
    expected_replacements = {
        "umbra-cpp-custom-transportation-type-control-integration",
        "umbra-cpp-focused-attribute-transportation-type-control-integration",
        "umbra-cpp-process-endpoint-transportation-interaction-integration",
        "umbra-cpp-process-endpoint-regional-interaction-transportation-override-integration",
        "umbra-cpp-process-tso-regional-interaction-transportation-override-integration",
    }
    if (
        superseded is None
        or set(superseded.get("replacement_plan_ids", [])) != expected_replacements
    ):
        raise AssertionError(
            "superseded transportation aggregate lost replacement plan ids"
        )
    superseded_summary = query_rti_work.test_summary_data(superseded)
    if set(superseded_summary.get("replacement_plan_ids", [])) != expected_replacements:
        raise AssertionError(
            "test summary omitted superseded transportation replacements"
        )
    superseded_trace = query_rti_work.trace_record(
        superseded, "test", superseded.get("id"), index
    )
    if set(superseded_trace.get("replacement_plan_ids", [])) != expected_replacements:
        raise AssertionError("trace omitted superseded transportation replacements")
    superseded_card = query_rti_work.case_card_record(
        superseded, index, superseded.get("id")
    )
    if set(superseded_card.get("replacement_plan_ids", [])) != expected_replacements:
        raise AssertionError("case card omitted superseded transportation replacements")
    if "replacement_plan_ids:" not in query_rti_work.text_case_card(superseded_card):
        raise AssertionError(
            "case card text omitted superseded transportation replacements"
        )

    # A focused DDM service-report case already exercises both invalid-region
    # paths.  Keep those exact Lab joins visible so the gap inventory does not
    # send implementation work back to a behavior that is already tested.
    regional_association = next(
        (
            test
            for test in tests
            if test.get("test_case")
            == "Embedded service reporting records failed regional object-attribute association invocations"
        ),
        None,
    )
    if regional_association is None:
        raise AssertionError("regional association failure mapping row disappeared")
    regional_pairs = query_rti_work.matrix_summary_data(
        regional_association, index
    ).get("requirement_section_mappings", [])
    regional_requirement_ids = set(
        query_rti_work.strings(regional_association.get("lab_requirement_ids"))
    )
    paired_requirement_ids = {
        row.get("lab_requirement_id")
        for row in regional_pairs
        if isinstance(row, dict) and row.get("lab_requirement_id")
    }
    regional_sections = {
        row.get("standard_section")
        for row in regional_pairs
        if isinstance(row, dict) and row.get("standard_section")
    }
    if (
        regional_requirement_ids != paired_requirement_ids
        or len(regional_pairs) != len(paired_requirement_ids)
        or regional_sections
        != {
            "hla-1516.1-2025:clause-9.6",
            "hla-1516.1-2025:clause-9.7.5",
            "hla-1516.1-2025:clause-11.5",
            "hla-1516.1-2025:clause-11.5.2",
        }
    ):
        raise AssertionError(
            "regional association failure direct subsection pairs drifted"
        )

    ownership_transfer = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-ownership-transfer-update-region-deferred-cancel-integration"
        ),
        None,
    )
    if ownership_transfer is None:
        raise AssertionError("ownership-transfer update-region mapping row disappeared")
    if ownership_transfer.get(
        "ctest_target"
    ) != "umbra_ownership_transfer_update_region_catch2" or ownership_transfer.get(
        "test_case"
    ) not in str(ownership_transfer.get("ctest_filter") or ""):
        raise AssertionError(
            "ownership-transfer update-region row lost its exact focused CTest handle"
        )
    transfer_pairs = query_rti_work.matrix_summary_data(ownership_transfer, index).get(
        "requirement_section_mappings", []
    )
    transfer_requirement_ids = set(
        query_rti_work.strings(ownership_transfer.get("lab_requirement_ids"))
    )
    paired_transfer_requirement_ids = {
        row.get("lab_requirement_id")
        for row in transfer_pairs
        if isinstance(row, dict) and row.get("lab_requirement_id")
    }
    transfer_sections = {
        row.get("standard_section")
        for row in transfer_pairs
        if isinstance(row, dict) and row.get("standard_section")
    }
    if (
        transfer_requirement_ids != paired_transfer_requirement_ids
        or len(transfer_pairs) != len(paired_transfer_requirement_ids)
        or transfer_sections != {"hla-1516.1-2025:clause-9.1.3.2"}
    ):
        raise AssertionError(
            "ownership-transfer update-region direct subsection pairs drifted"
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

    roadmap_entries = query_rti_work.roadmap_checklist(query_rti_work.DEFAULT_ROADMAP)
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
    for lookup_handle in (
        "section_query",
        "requirement_query",
        "matrix_query",
        "matrix_requirement_crosswalk_query",
        "matrix_section_crosswalk_query",
    ):
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
        or not str(deferred.get("next_step") or "").strip()
    ):
        raise AssertionError("dashboard deferred process TSO seam drifted")
    dashboard_health = dashboard.get("source_health", {})
    if not isinstance(dashboard_health, dict) or not isinstance(
        dashboard_health.get("files_with_unbalanced_conditionals"), int
    ):
        raise AssertionError("dashboard omitted preprocessor source-health counters")
    index_snapshot = dashboard.get("index_snapshot", {})
    if not isinstance(index_snapshot, dict) or not isinstance(
        index_snapshot.get("matches_live"), bool
    ):
        raise AssertionError("dashboard omitted traceability snapshot freshness")
    if not index_snapshot.get("matches_live"):
        raise AssertionError(
            "roadmap index snapshot is stale relative to the live requirements/test plan"
        )
    traceability_snapshot = index_snapshot.get("traceability", {})
    if (
        not isinstance(traceability_snapshot, dict)
        or not traceability_snapshot.get("matches_live")
        or traceability_snapshot.get("stale_fields")
    ):
        raise AssertionError("roadmap index omitted current live traceability totals")
    stale_index = json.loads(json.dumps(index))
    stale_index["mapping"]["current_snapshot"]["direct_requirement_section_pairs"] += 1
    stale_dashboard = query_rti_work.dashboard_snapshot(
        stale_index,
        tests,
        roadmap_entries,
        source_locations,
        source_health,
        limit=3,
        standard_requirements=standard_requirements,
    )
    stale_traceability = stale_dashboard.get("index_snapshot", {}).get(
        "traceability", {}
    )
    if (
        stale_dashboard.get("index_snapshot", {}).get("matches_live")
        or not isinstance(stale_traceability, dict)
        or "direct_requirement_section_pairs"
        not in stale_traceability.get("stale_fields", [])
    ):
        raise AssertionError("dashboard failed to detect a stale direct-pair snapshot")
    dashboard_latest = dashboard.get("latest_completed_slice")
    if not isinstance(dashboard_latest, dict):
        raise AssertionError("dashboard omitted the latest bounded slice card")
    if (
        dashboard_latest.get("plan_id")
        != index["mapping"]["latest_completed_slice"]["plan_id"]
    ):
        raise AssertionError("dashboard latest slice pointer drifted")
    for latest_handle in (
        "case_command",
        "focus_command",
        "trace_command",
        "matrix_command",
        "check_command",
        "ctest_command",
    ):
        if not isinstance(dashboard_latest.get(latest_handle), str):
            raise AssertionError(f"dashboard latest slice omitted {latest_handle}")
    active_pointer = dashboard.get("active_pointer")
    if (
        not isinstance(active_pointer, dict)
        or active_pointer.get("work_id") != "transport-and-conformance"
        or not str(active_pointer.get("task") or "").strip()
        or "64 mapped" in str(active_pointer.get("task") or "")
        or "64 mapped" in str(active_pointer.get("work_query") or "")
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
        if dashboard_next.get("state") != "none" or not dashboard_next.get(
            "family_options"
        ):
            raise AssertionError(
                "dashboard omitted the bounded family fallback handoff"
            )
        if dashboard_next.get("selection_required") is not True:
            raise AssertionError("dashboard family fallback omitted selection_required")
        if dashboard_next.get("recommended_family_id") != dashboard_next.get(
            "family_options", [{}]
        )[0].get("id"):
            raise AssertionError("dashboard omitted its bounded family recommendation")
        if "ready --family <family-id>" not in str(
            dashboard_next.get("family_choice_command") or ""
        ):
            raise AssertionError("dashboard omitted its bounded family-choice command")
        dashboard_family = dashboard_next.get("family_options", [])[0]
        if (
            not isinstance(dashboard_family, dict)
            or dashboard_family.get("action_state")
            not in {"backlog-shaping", "new-case-needed"}
            or not isinstance(dashboard_family.get("gap_preview"), dict)
            or "inventory only"
            not in str(
                dashboard_family.get("gap_preview", {}).get("selection_basis", "")
            )
        ):
            raise AssertionError(
                "dashboard family fallback omitted its bounded inventory-only gap preview"
            )
        family_id = str(dashboard_family.get("id") or "")
        expected_family_gap = expected_family_gap_head(
            index,
            tests,
            standard_requirements,
            family_id,
        )
        preview_requirement = dashboard_family["gap_preview"].get("requirement")
        if isinstance(preview_requirement, dict):
            expected_record = expected_family_gap.get("requirement")
            if (
                not isinstance(expected_record, dict)
                or preview_requirement.get("id") != expected_record.get("id")
                or preview_requirement.get("standard_section")
                != expected_family_gap.get("section")
                or preview_requirement.get("document_id") != "hla-1516.1-2025"
                or dashboard_family["gap_preview"].get(
                    "global_uncovered_candidate_count"
                )
                != expected_family_gap.get("candidate_count")
                or dashboard_family["gap_preview"].get("out_of_scope_candidate_count")
                != expected_family_gap.get("out_of_scope_candidate_count")
                or dashboard_family["gap_preview"].get("inventory_only_gap_count")
                != expected_family_gap.get("inventory_only_count")
                or dashboard_family["gap_preview"].get("preview_candidate_count")
                != expected_family_gap.get("preview_candidate_count")
            ):
                raise AssertionError(
                    "family gap preview did not select an uncovered requirement in a represented subsection"
                )
        elif dashboard_family["gap_preview"].get("preview_candidate_count"):
            raise AssertionError(
                "family gap preview hid a previewable globally uncovered row"
            )
        for field in ("matrix_requirement_command", "matrix_section_command"):
            if "matrix" not in str(dashboard_family.get(field) or ""):
                raise AssertionError(f"dashboard family fallback omitted {field}")
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
            raise AssertionError("dashboard source-only card omitted its queue head")
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
        if resume_next.get("selection_required") is not True:
            raise AssertionError("compact resume card omitted selection_required")
        if resume_next.get("recommended_family_id") != options[0].get("id"):
            raise AssertionError("compact resume card lost its recommended family")
        if "ready --family <family-id>" not in str(
            resume_next.get("family_choice_command") or ""
        ):
            raise AssertionError("compact resume card omitted family-choice command")
        expected_remaining_families = max(
            0, int(resume_next.get("family_queue_count", 0)) - 1
        )
        if resume_next.get("family_options_remaining") != expected_remaining_families:
            raise AssertionError("compact resume family-choice count drifted")
        if "gap_preview" in options[0] or "gap_preview" in resume_next:
            raise AssertionError(
                "compact resume card mixed coverage inventory into its work handoff"
            )
        for field in ("matrix_requirement_command", "matrix_section_command"):
            if "matrix" not in str(options[0].get(field) or ""):
                raise AssertionError(f"compact resume card omitted {field}")
    resume_handles = resume.get("handles")
    if not isinstance(resume_handles, dict) or not resume_handles.get("ready"):
        raise AssertionError("compact resume card omitted the ready handle")
    resume_deferred = resume.get("deferred_slices")
    if not isinstance(resume_deferred, list) or not resume_deferred:
        raise AssertionError("compact resume card omitted the deferred-slice seam")

    handoff_index = dict(index)
    handoff_index["active_handoff"] = {
        "id": "resume-regression-active-handoff",
        "state": "source-reconciliation",
        "family": "time-save-restore",
        "lane": "flush-queue-grant-logical-time-gating",
        "objective": "Regression sentinel: resume should surface this exact handoff.",
        "plan_id": "umbra-cpp-resume-regression-active-handoff",
        "test_case": "Resume regression sentinel active handoff case",
        "source_target": "cpp/tests/ieee1516_2025_federation_management_catch2.cpp",
        "mapping_seed_plan_id": "umbra-cpp-flush-queue-request-logical-time-gating-integration",
        "ctest_target": "umbra_ieee1516_2025_catch2",
        "ctest_filter": "^umbra\\.ieee1516_2025\\.catch2\\.Resume regression sentinel active handoff case$",
        "commands": {
            "case": "python tools/query_rti_work.py case resume-regression-active-handoff"
        },
        "acceptance": [
            "The explicit handoff replaces generic family selection when no runnable slice is queued."
        ],
    }
    handoff_resume = query_rti_work.resume_snapshot(
        handoff_index,
        tests,
        roadmap_entries,
        source_locations,
        source_health,
        limit=1,
        standard_requirements=standard_requirements,
    )
    handoff_next = handoff_resume.get("next")
    if (
        not isinstance(handoff_next, dict)
        or handoff_next.get("active_handoff_id") != "resume-regression-active-handoff"
        or handoff_next.get("test_case")
        != "Resume regression sentinel active handoff case"
        or handoff_next.get("mapping_seed_plan_id")
        != "umbra-cpp-flush-queue-request-logical-time-gating-integration"
        or not handoff_next.get("requirement_section_pairs")
    ):
        raise AssertionError(
            "compact resume card hid the explicit handoff behind family selection"
        )

    lab_issues = query_rti_work.lab_issues_snapshot()
    if (
        not isinstance(lab_issues.get("count"), int)
        or lab_issues.get("count", 0) < lab_issues.get("shown_count", 0)
        or lab_issues.get("shown_count") != len(lab_issues.get("issues", []))
        or lab_issues.get("shown_count", 0) == 0
    ):
        raise AssertionError("Requirements Lab issue ledger preview drifted")
    issue = lab_issues.get("issues", [{}])[0]
    if (
        issue.get("id") != "RL-177"
        or issue.get("exported_clause") != "6.18.1"
        or issue.get("normative_clause") != "6.17.4"
    ):
        raise AssertionError("RL-177 clause-boundary issue lost its evidence")
    issue_lookup = query_rti_work.lab_issues_snapshot(query="6.17.4")
    if (
        issue_lookup.get("count") != 1
        or issue_lookup.get("issues", [{}])[0].get("id") != "RL-177"
    ):
        raise AssertionError("Requirements Lab issue clause lookup drifted")
    parent_clause_lookup = query_rti_work.lab_issues_snapshot(query="6.17")
    if (
        parent_clause_lookup.get("count") != 1
        or parent_clause_lookup.get("issues", [{}])[0].get("id") != "RL-177"
    ):
        raise AssertionError("Requirements Lab parent-clause lookup drifted")
    recurrence = query_rti_work.lab_issues_snapshot(query="RL-248")
    recurrence_record = recurrence.get("issues", [{}])[0]
    if (
        recurrence.get("count") != 1
        or recurrence_record.get("id") != "RL-248"
        or recurrence_record.get("exported_clause") != "9.10"
        or recurrence_record.get("normative_clause") != "9.9.3"
        or recurrence_record.get("reported_normative_clause") != "9.5.3"
        or recurrence_record.get("status") != "resolved-in-lab"
        or recurrence_record.get("affected_requirement_ids")
        != [
            "requirement-candidate-content-clauses-09-data-distribution-management-page-236-l32-9"
        ]
    ):
        raise AssertionError("RL-248 source-clause recurrence lost its evidence")
    extraction_issue = query_rti_work.lab_issues_snapshot(query="RL-249")
    extraction_record = extraction_issue.get("issues", [{}])[0]
    if (
        extraction_issue.get("count") != 1
        or extraction_record.get("id") != "RL-249"
        or extraction_record.get("exported_clause") != "6.1.9"
        or extraction_record.get("normative_clause") != "6.1.9"
        or extraction_record.get("affected_requirement_ids")
        != ["requirement-candidate-content-clauses-06-object-management-page-109-l10-1"]
    ):
        raise AssertionError("RL-249 interaction-receipt text issue lost its evidence")
    split_requirement_issue = query_rti_work.lab_issues_snapshot(query="RL-250")
    split_requirement_record = split_requirement_issue.get("issues", [{}])[0]
    if (
        split_requirement_issue.get("count") != 1
        or split_requirement_record.get("id") != "RL-250"
        or split_requirement_record.get("exported_clause") != "9.1.6"
        or split_requirement_record.get("normative_clause") != "9.1.6"
        or split_requirement_record.get("affected_requirement_ids")
        != [
            "requirement-candidate-content-clauses-09-data-distribution-management-page-224-l89-29",
            "requirement-candidate-content-clauses-09-data-distribution-management-page-224-l92-30",
        ]
    ):
        raise AssertionError("RL-250 split-requirement issue lost its evidence")
    incomplete_list_issue = query_rti_work.lab_issues_snapshot(query="RL-251")
    incomplete_list_record = incomplete_list_issue.get("issues", [{}])[0]
    if (
        incomplete_list_issue.get("count") != 1
        or incomplete_list_record.get("id") != "RL-251"
        or incomplete_list_record.get("exported_clause") != "9.1.6"
        or incomplete_list_record.get("normative_clause") != "9.1.6"
        or incomplete_list_record.get("affected_requirement_ids")
        != [
            "requirement-candidate-content-clauses-09-data-distribution-management-page-224-l137-44"
        ]
    ):
        raise AssertionError("RL-251 incomplete DDM service list lost its evidence")
    section_query_recurrence = query_rti_work.lab_issues_snapshot(query="RL-252")
    section_query_record = section_query_recurrence.get("issues", [{}])[0]
    if (
        section_query_recurrence.get("count") != 1
        or section_query_record.get("id") != "RL-252"
        or section_query_record.get("status") != "resolved-in-umbra"
        or section_query_record.get("exported_clause") != "8.1.10"
        or section_query_record.get("normative_clause") != "8.1.8"
        or section_query_record.get("recurrence_of") != "RL-018"
        or len(section_query_record.get("affected_requirement_ids", [])) != 3
    ):
        raise AssertionError(
            "RL-252 section-query recurrence lost its crosswalk evidence"
        )
    prior_plan_recurrence = query_rti_work.lab_issues_snapshot(query="RL-261")
    prior_plan_record = prior_plan_recurrence.get("issues", [{}])[0]
    if (
        prior_plan_recurrence.get("count") != 1
        or prior_plan_record.get("id") != "RL-261"
        or prior_plan_record.get("status") != "resolved-in-umbra"
        or prior_plan_record.get("recurrence_of") != "RL-260"
    ):
        raise AssertionError("RL-261 plan-corruption recurrence history was rewritten")
    plan_repair_recurrence = query_rti_work.lab_issues_snapshot(query="RL-262")
    plan_repair_record = plan_repair_recurrence.get("issues", [{}])[0]
    if (
        plan_repair_recurrence.get("count") != 1
        or plan_repair_record.get("id") != "RL-262"
        or plan_repair_record.get("status") != "resolved-in-umbra"
        or plan_repair_record.get("recurrence_of") != "RL-261"
        or plan_repair_record.get("kind")
        != "requirements-lab-plan-json-duplicate-key-row-identity-corruption"
    ):
        raise AssertionError(
            "RL-262 Catch2 plan row-corruption recurrence lost its repair record"
        )

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
    # strict-relaxed-ddm-boundary slice closes line 71. The passive regional
    # object-attribute slice closes the §9.8.4 empty-region, active/passive,
    # and update-rate requirements. The service-reporting
    # interlock slice now also maps the enabled-subscription prohibition in
    # clause 11.5.1; the exact fallback lookup below stays on a separate
    # uncovered MOM requirement.
    gaps = query_rti_work.requirement_gap_inventory(
        standard_requirements,
        tests,
        limit=8,
    )
    if gaps.get("total_requirement_count") != 2225:
        raise AssertionError("2025 requirement gap total drifted")
    covered_ids = {
        requirement_id
        for test in tests
        for requirement_id in test.get("lab_requirement_ids", [])
        if requirement_id in standard_requirements
    }
    if gaps.get("covered_requirement_count") != len(covered_ids):
        raise AssertionError("2025 requirement mapped total drifted")
    if gaps.get("uncovered_requirement_count") != len(standard_requirements) - len(
        covered_ids
    ):
        raise AssertionError("2025 uncovered requirement total drifted")
    if gaps.get("coverage_percent") != round(
        100 * len(covered_ids) / len(standard_requirements), 2
    ):
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
        family_clause_gaps.get("total_requirement_count") != 6
        or family_clause_gaps.get("covered_requirement_count") != 6
        or family_clause_gaps.get("uncovered_requirement_count") != 0
        or family_clause_gaps.get("groups") != []
    ):
        raise AssertionError("family-scoped clause-9.1.3.3 gap inventory drifted")
    family_clause_9132_gaps = query_rti_work.requirement_gap_inventory(
        standard_requirements,
        tests,
        index=index,
        family="object-ddm-ownership",
        clause="clause-9.1.3.2",
        limit=8,
    )
    if (
        family_clause_9132_gaps.get("total_requirement_count") != 22
        or family_clause_9132_gaps.get("covered_requirement_count") != 22
        or family_clause_9132_gaps.get("uncovered_requirement_count") != 0
    ):
        raise AssertionError("family-scoped clause-9.1.3.2 gap counts drifted")
    family_gap_covered_elsewhere = query_rti_work.requirement_gap_inventory(
        standard_requirements,
        tests,
        index=index,
        family="portable-cpp-tck-2025",
        query="requirement-candidate-content-clauses-04-federation-management-page-038-l21-4",
        limit=2,
    )
    family_gap_records = family_gap_covered_elsewhere.get("requirements", [])
    if (
        family_gap_covered_elsewhere.get("coverage_scope") != "family-local"
        or family_gap_covered_elsewhere.get("uncovered_requirement_count") != 1
        or family_gap_covered_elsewhere.get("globally_covered_requirement_count") != 1
        or family_gap_covered_elsewhere.get("globally_uncovered_requirement_count") != 0
        or len(family_gap_records) != 1
        or family_gap_records[0].get("coverage_scope") != "family-local"
        or family_gap_records[0].get("global_coverage") != "covered"
    ):
        raise AssertionError(
            "family-local gap failed to distinguish coverage by another family"
        )
    gap_summary = query_rti_work.requirement_gap_summary_data(gaps)
    if (
        gap_summary.get("shown_requirement_count") != 0
        or gap_summary.get("requirements") != []
    ):
        raise AssertionError(
            "gap summary expanded full requirement records instead of staying bounded"
        )
    tck_nmra_gaps = query_rti_work.cpp_tck_requirement_gap_inventory(
        standard_requirements,
        cpp_tck_catalog,
        tests,
        document="hla-1516.1-2025",
        clause="clause-8.11.3",
        limit=0,
    )
    if (
        tck_nmra_gaps.get("coverage_scope") != "cpp-tck-direct-requirement-mappings"
        or tck_nmra_gaps.get("total_requirement_count") != 8
        or tck_nmra_gaps.get("mapped_requirement_count") != 6
        or tck_nmra_gaps.get("uncovered_requirement_count") != 2
        or tck_nmra_gaps.get("promoted_requirement_count") != 6
        or tck_nmra_gaps.get("candidate_only_requirement_count") != 0
        or tck_nmra_gaps.get("invalid_mapping_count") != 0
        or tck_nmra_gaps.get("section_disagreement_count") != 0
    ):
        raise AssertionError("portable TCK §8.11.3 direct-mapping inventory drifted")
    tck_galt_requirement = query_rti_work.cpp_tck_requirement_gap_inventory(
        standard_requirements,
        cpp_tck_catalog,
        tests,
        query="requirement-candidate-content-clauses-08-time-management-page-192-l84-24",
        limit=0,
    )
    if (
        tck_galt_requirement.get("total_requirement_count") != 1
        or tck_galt_requirement.get("mapped_requirement_count") != 1
        or tck_galt_requirement.get("uncovered_requirement_count") != 0
        or tck_galt_requirement.get("promoted_requirement_count") != 1
        or tck_galt_requirement.get("section_disagreement_count") != 0
    ):
        raise AssertionError("portable TCK GALT-equality requirement mapping drifted")
    tck_galt_summary = query_rti_work.cpp_tck_requirement_gap_summary_data(
        tck_galt_requirement
    )
    if (
        tck_galt_summary.get("shown_requirement_count") != 0
        or tck_galt_summary.get("requirements") != []
    ):
        raise AssertionError("portable TCK gap summary expanded requirement records")
    crosswalk_target_id = "requirement-candidate-regression-crosswalk-target"
    crosswalk_requirement_ids = [
        *(f"requirement-candidate-regression-crosswalk-{index}" for index in range(20)),
        crosswalk_target_id,
    ]
    crosswalk_standard_requirements = {
        requirement_id: {
            "id": requirement_id,
            "document_id": "hla-1516.1-2025",
            "clause_id": "clause-9.1.3.3",
            "ordinal": index,
        }
        for index, requirement_id in enumerate(crosswalk_requirement_ids)
    }
    crosswalk_catalog = {
        "kind": "portable-cpp-tck-catalog",
        "scenarios": [
            {
                "id": f"cpp-tck.crosswalk-regression-{index}",
                "promotion": "promoted",
                "requirement_mappings": [
                    {
                        "requirement_id": requirement_id,
                        "standard_section": "hla-1516.1-2025:clause-9.1.3.2",
                    }
                ],
            }
            for index, requirement_id in enumerate(crosswalk_requirement_ids)
        ],
    }
    filtered_crosswalk = query_rti_work.cpp_tck_requirement_gap_inventory(
        crosswalk_standard_requirements,
        crosswalk_catalog,
        [],
        query=crosswalk_target_id,
        clause="clause-9.1.3.3",
        limit=1,
    )
    crosswalk_findings = filtered_crosswalk.get("section_disagreements", [])
    if (
        filtered_crosswalk.get("section_disagreement_count") != 1
        or len(crosswalk_findings) != 1
        or crosswalk_findings[0].get("requirement_id") != crosswalk_target_id
    ):
        raise AssertionError(
            "filtered portable TCK crosswalk query hid its matching disagreement"
        )
    now_covered_lookup = query_rti_work.requirement_gap_inventory(
        standard_requirements,
        tests,
        query="requirement-candidate-table-5-p296-r007",
        limit=20,
    )
    if (
        now_covered_lookup.get("uncovered_requirement_count") != 0
        or now_covered_lookup.get("globally_covered_requirement_count") != 1
    ):
        raise AssertionError("exactly covered requirement lookup drifted")
    uncovered_lookup = query_rti_work.requirement_gap_inventory(
        standard_requirements,
        tests,
        query="requirement-candidate-table-5-p296-r011",
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
        or uncovered_record.get("standard_section") != "hla-1516.1-2025:clause-11.5.1"
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
        raise AssertionError(
            "uncovered requirement lookup summary lost its bounded preview"
        )

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
            raise AssertionError(f"roadmap mapping search did not resolve {query}")
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
        raise AssertionError("public negotiated If Available lane mapping card drifted")
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
        raise AssertionError("public If Available ownership lane mapping card drifted")
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
        raise AssertionError(
            "roadmap family row omitted its exhausted source queue state"
        )
    if time_family.get("next_source_test_query") is not None:
        raise AssertionError(
            "roadmap family retained an exhausted source queue pointer"
        )
    if time_family.get("next_source_location") is not None:
        raise AssertionError(
            "roadmap family retained an exhausted source queue location"
        )
    if time_family.get("next_source_lane") != "save-restore":
        raise AssertionError("roadmap family source queue lane drifted")

    cmake_target_text = (
        REPOSITORY_ROOT
        / "cmake/ieee1516_2025_federation_management_catch2_targets.cmake"
    ).read_text(encoding="utf-8")
    cmake_discovery_text = (
        REPOSITORY_ROOT / "cmake/catch2_executable_targets.cmake"
    ).read_text(encoding="utf-8")
    focused_target = "umbra_connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient_catch2"
    focused_prefix = "umbra.connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient.catch2."
    if (
        focused_target not in cmake_target_text
        or focused_target not in cmake_discovery_text
        or focused_prefix not in cmake_discovery_text
    ):
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
            raise AssertionError(
                f"core case lost direct requirements mapping: {test_case!r}"
            )
        if len(core.get("lab_requirement_ids", [])) != expected["requirements"]:
            raise AssertionError(f"requirement count drifted for {test_case!r}")
        if (
            "api_surfaces" in expected
            and len(core.get("selected_cpp_api_surface_ids", []))
            != expected["api_surfaces"]
        ):
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
            # Source locations are derived from the live TEST_CASE declaration
            # by query_rti_work.  A case insertion earlier in a translation
            # unit legitimately changes its line without changing the
            # traceability identity.  Keep the path check strict (a moved or
            # missing declaration is actionable), but do not turn an
            # append-only line shift into a false regression failure.
            actual_path = (
                actual_source.rsplit(":", 1)[0] if ":" in actual_source else ""
            )
            if actual_path != expected_path:
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
        if (
            query_rti_work.source_location_text(source_case)
            != expected["source_location"]
        ):
            raise AssertionError(f"source-backed pointer drifted for {test_case!r}")
        if source_case.get("assertions") != expected["assertions"]:
            raise AssertionError(
                f"source-backed assertion count drifted for {test_case!r}"
            )
        if len(source_case.get("lab_requirement_ids", [])) != expected["requirements"]:
            raise AssertionError(
                f"source-backed requirement count drifted for {test_case!r}"
            )
        if set(source_case.get("standard_sections", [])) != expected["sections"]:
            raise AssertionError(
                f"source-backed section keys drifted for {test_case!r}"
            )

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
        if (
            query_rti_work.source_location_text(recovered)
            != expected["source_location"]
        ):
            raise AssertionError(f"recovered source pointer drifted for {test_case!r}")
        if recovered.get("assertions") != expected["assertions"]:
            raise AssertionError(f"recovered assertion count drifted for {test_case!r}")
        if len(recovered.get("lab_requirement_ids", [])) != expected["requirements"]:
            raise AssertionError(
                f"recovered requirement count drifted for {test_case!r}"
            )
        if len(recovered.get("standard_sections", [])) != expected["sections"]:
            raise AssertionError(f"recovered section count drifted for {test_case!r}")
        if expected["lane"] not in recovered.get("tags", []):
            raise AssertionError(f"recovered lane tag drifted for {test_case!r}")
        recovered_pairs = query_rti_work.matrix_summary_data(recovered, index).get(
            "requirement_section_mappings", []
        )
        if len(recovered_pairs) != expected["requirements"]:
            raise AssertionError(
                f"recovered direct mapping count drifted for {test_case!r}"
            )

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
        {
            location.get("path")
            for location in test.get("source_locations", [])
            if isinstance(location, dict)
        }
        != MAPPED_SHARED_CASE["source_locations"]
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
            raise AssertionError(
                f"federation-listing mapping row missing: {mapping_id}"
            )
        if len(row.get("lab_requirement_ids", [])) != expected["requirements"]:
            raise AssertionError(
                f"federation-listing requirement count drifted: {mapping_id}"
            )
        if set(row.get("standard_sections", [])) != expected["sections"]:
            raise AssertionError(
                f"federation-listing section keys drifted: {mapping_id}"
            )
    federation_listing_lane = query_rti_work.focused_lane_result(
        index, tests, "federation-listing", limit=1
    )
    if federation_listing_lane.get("assertion_count") != 41:
        raise AssertionError("federation-listing lane assertion total drifted")
    federation_listing_handles = federation_listing_lane.get("lane_handles")
    if not isinstance(federation_listing_handles, dict):
        raise AssertionError("federation-listing lane lost its evidence handles")
    if (
        federation_listing_handles.get("catch2_target")
        != "umbra_federation_listing_catch2"
    ):
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
    queue_by_id: dict[str, query_rti_work.JsonObject] = {}
    for row in query_rti_work.json_object_rows(queue.get("items")):
        row_id = row.get("id")
        if isinstance(row_id, str) and row_id:
            queue_by_id[row_id] = row
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
    if (
        not isinstance(review_queue, dict)
        or review_queue.get("action_state") != "external-review"
    ):
        raise AssertionError(
            "protected-review family did not retain its explicit external action"
        )
    action_counts = queue.get("action_counts")
    expected_action_counts = Counter(
        row.get("action_state")
        for row in queue.get("items", [])
        if isinstance(row, dict) and isinstance(row.get("action_state"), str)
    )
    if not isinstance(action_counts, dict) or action_counts != dict(
        expected_action_counts
    ):
        raise AssertionError(
            "queue action-state summary disagrees with its family rows"
        )
    expected_evidence_complete_count = expected_action_counts.get(
        "evidence-complete", 0
    )
    expected_queued_action_count = sum(
        count
        for action, count in expected_action_counts.items()
        if action != "evidence-complete"
    )
    if (
        queue.get("evidence_complete_family_count") != expected_evidence_complete_count
        or queue.get("queued_action_family_count") != expected_queued_action_count
    ):
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
    process_plan_rows = [
        test for test in tests if "process-boundary" in test.get("tags", [])
    ]
    if (
        process_lane.get("mapped_test_count")
        != sum(bool(test.get("lab_requirement_ids")) for test in process_plan_rows)
        or process_lane.get("assertion_count") != 6959
    ):
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
        process_inventory_row.get("assertion_count") != 6959
        or process_inventory_row.get("recorded_assertion_count")
        != sum(
            value
            for test in process_plan_rows
            for value in [test.get("assertions")]
            if isinstance(value, int)
        )
        or process_inventory_row.get("assertion_count_source") != "indexed-lane-total"
    ):
        raise AssertionError(
            "process-boundary lane inventory does not use the verified aggregate total"
        )
    process_service_report_focus = query_rti_work.focused_lane_result(
        index, tests, "process-service-report-file-switch-cycle", limit=0
    )
    if (
        process_service_report_focus.get("mapped_test_count") != 2
        or process_service_report_focus.get("assertion_count") != 125
        or process_service_report_focus.get("standard_section_count") != 15
        or process_service_report_focus.get("requirement_section_pair_count") != 18
    ):
        raise AssertionError("process service-report switch-cycle lane mapping drifted")
    process_service_report_multifederate_focus = query_rti_work.focused_lane_result(
        index, tests, "process-service-report-file-multifederate", limit=0
    )
    if (
        process_service_report_multifederate_focus.get("mapped_test_count") != 1
        or process_service_report_multifederate_focus.get("assertion_count") != 26
        or process_service_report_multifederate_focus.get("standard_section_count") != 2
        or process_service_report_multifederate_focus.get(
            "requirement_section_pair_count"
        )
        != 2
    ):
        raise AssertionError(
            "process service-report multifederate lane mapping drifted"
        )
    process_service_report_multifederate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-service-report-file-multifederate-integration"
        ),
        None,
    )
    if not isinstance(process_service_report_multifederate, dict):
        raise AssertionError("process service-report multifederate plan row is absent")
    if (
        query_rti_work.source_location_text(process_service_report_multifederate)
        != "cpp/tests/ieee1516_2025_connection_service_report_catch2.cpp:142"
        or process_service_report_multifederate.get("traceability_state")
        != "requirements-mapped"
        or process_service_report_multifederate.get("lab_requirement_ids")
        != [
            "requirement-candidate-content-clauses-11-management-object-model-page-293-l128-42",
            "requirement-candidate-content-clauses-11-management-object-model-page-292-l18-5",
        ]
        or process_service_report_multifederate.get("standard_sections")
        != [
            "hla-1516.1-2025:clause-11.5",
            "hla-1516.1-2025:clause-11.5.2",
        ]
    ):
        raise AssertionError(
            "process service-report multifederate case mapping drifted"
        )
    process_service_report_multifederate_case = query_rti_work.case_card_record(
        process_service_report_multifederate,
        index,
        "umbra-cpp-process-endpoint-service-report-file-multifederate-integration",
    )
    if (
        process_service_report_multifederate_case.get("lane")
        != "process-service-report-file-multifederate"
        or not process_service_report_multifederate_case.get("commands", {}).get(
            "matrix_requirement"
        )
        or not process_service_report_multifederate_case.get("commands", {}).get(
            "matrix_section"
        )
        or not process_service_report_multifederate_case.get("commands", {}).get(
            "ctest"
        )
    ):
        raise AssertionError(
            "process service-report multifederate case handles drifted"
        )
    process_mom_set_switches_focus = query_rti_work.focused_lane_result(
        index, tests, "process-mom-set-switches", limit=0
    )
    if (
        process_mom_set_switches_focus.get("mapped_test_count") != 1
        or process_mom_set_switches_focus.get("assertion_count") != 57
        or process_mom_set_switches_focus.get("standard_section_count") != 3
        or process_mom_set_switches_focus.get("requirement_section_pair_count") != 3
    ):
        raise AssertionError("process MOM set-switches lane mapping drifted")
    process_mom_set_switches = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-mom-federate-set-switches-integration"
        ),
        None,
    )
    if not isinstance(process_mom_set_switches, dict):
        raise AssertionError("process MOM set-switches plan row is absent")
    if (
        query_rti_work.source_location_text(process_mom_set_switches)
        != "cpp/tests/ieee1516_2025_connection_process_mom_set_switches_catch2.cpp:51"
        or process_mom_set_switches.get("traceability_state") != "requirements-mapped"
        or process_mom_set_switches.get("lab_requirement_ids")
        != [
            "requirement-candidate-content-clauses-11-management-object-model-page-290-l36-11",
            "requirement-candidate-content-clauses-11-management-object-model-page-291-l146-48",
            "requirement-source-refinement-mom-service-reporting",
        ]
        or process_mom_set_switches.get("standard_sections")
        != [
            "hla-1516.1-2025:clause-11.4.1",
            "hla-1516.1-2025:clause-11.4.2",
            "hla-1516.1-2025:clause-11.6",
        ]
        or len(process_mom_set_switches.get("selected_cpp_api_surface_ids", [])) != 17
    ):
        raise AssertionError("process MOM set-switches case mapping drifted")
    process_mom_set_switches_case = query_rti_work.case_card_record(
        process_mom_set_switches,
        index,
        "umbra-cpp-process-endpoint-mom-federate-set-switches-integration",
    )
    if (
        process_mom_set_switches_case.get("lane") != "process-mom-set-switches"
        or not process_mom_set_switches_case.get("commands", {}).get(
            "matrix_requirement"
        )
        or not process_mom_set_switches_case.get("commands", {}).get("matrix_section")
        or not process_mom_set_switches_case.get("commands", {}).get("ctest")
    ):
        raise AssertionError("process MOM set-switches case handles drifted")
    process_mom_service_report_interlock_focus = query_rti_work.focused_lane_result(
        index, tests, "process-mom-service-report-interlock", limit=0
    )
    if (
        process_mom_service_report_interlock_focus.get("mapped_test_count") != 2
        or process_mom_service_report_interlock_focus.get("assertion_count") != 18
        or process_mom_service_report_interlock_focus.get("standard_section_count") != 5
        or process_mom_service_report_interlock_focus.get(
            "requirement_section_pair_count"
        )
        != 5
    ):
        raise AssertionError(
            "process MOM service-report interlock lane mapping drifted"
        )
    process_mom_service_report_interlock = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-mom-service-report-interlock-integration"
        ),
        None,
    )
    if not isinstance(process_mom_service_report_interlock, dict):
        raise AssertionError("process MOM service-report interlock plan row is absent")
    if (
        query_rti_work.source_location_text(process_mom_service_report_interlock)
        != "cpp/tests/ieee1516_2025_connection_catch2.cpp:1376"
        or process_mom_service_report_interlock.get("traceability_state")
        != "requirements-mapped"
        or process_mom_service_report_interlock.get("lab_requirement_ids")
        != [
            "requirement-candidate-content-clauses-11-management-object-model-page-290-l36-11",
            "requirement-candidate-content-clauses-11-management-object-model-page-291-l146-48",
            "requirement-candidate-content-clauses-11-management-object-model-page-292-l18-5",
            "requirement-candidate-content-clauses-11-management-object-model-page-292-l60-19",
        ]
        or process_mom_service_report_interlock.get("standard_sections")
        != [
            "hla-1516.1-2025:clause-11.4.1",
            "hla-1516.1-2025:clause-11.4.2",
            "hla-1516.1-2025:clause-11.5",
            "hla-1516.1-2025:clause-11.5.1",
        ]
        or len(
            process_mom_service_report_interlock.get("selected_cpp_api_surface_ids", [])
        )
        != 11
    ):
        raise AssertionError(
            "process MOM service-report interlock case mapping drifted"
        )
    process_mom_service_report_interlock_case = query_rti_work.case_card_record(
        process_mom_service_report_interlock,
        index,
        "umbra-cpp-process-endpoint-mom-service-report-interlock-integration",
    )
    if (
        process_mom_service_report_interlock_case.get("lane")
        != "process-mom-service-report-interlock"
        or not process_mom_service_report_interlock_case.get("commands", {}).get(
            "matrix_requirement"
        )
        or not process_mom_service_report_interlock_case.get("commands", {}).get(
            "matrix_section"
        )
        or not process_mom_service_report_interlock_case.get("commands", {}).get(
            "ctest"
        )
    ):
        raise AssertionError(
            "process MOM service-report interlock case handles drifted"
        )
    process_mom_exception_focus = query_rti_work.focused_lane_result(
        index, tests, "process-mom-exception-report", limit=0
    )
    if (
        process_mom_exception_focus.get("mapped_test_count") != 1
        or process_mom_exception_focus.get("assertion_count") != 34
        or process_mom_exception_focus.get("standard_section_count") != 2
        or process_mom_exception_focus.get("requirement_section_pair_count") != 3
    ):
        raise AssertionError("process MOM exception-report lane mapping drifted")
    process_mom_exception = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-mom-exception-report-integration"
        ),
        None,
    )
    if not isinstance(process_mom_exception, dict):
        raise AssertionError("process MOM exception-report plan row is absent")
    if (
        query_rti_work.source_location_text(process_mom_exception)
        != "cpp/tests/ieee1516_2025_connection_catch2.cpp:1458"
        or process_mom_exception.get("traceability_state") != "requirements-mapped"
        or process_mom_exception.get("lab_requirement_ids")
        != [
            "requirement-candidate-content-clauses-11-management-object-model-page-290-l111-36",
            "requirement-candidate-content-clauses-11-management-object-model-page-290-l150-49",
            "requirement-candidate-content-clauses-11-management-object-model-page-292-l18-5",
        ]
        or process_mom_exception.get("standard_sections")
        != [
            "hla-1516.1-2025:clause-11.4.1",
            "hla-1516.1-2025:clause-11.5",
        ]
        or len(process_mom_exception.get("selected_cpp_api_surface_ids", [])) != 13
    ):
        raise AssertionError("process MOM exception-report case mapping drifted")
    process_mom_exception_case = query_rti_work.case_card_record(
        process_mom_exception,
        index,
        "umbra-cpp-process-endpoint-mom-exception-report-integration",
    )
    if (
        process_mom_exception_case.get("lane") != "process-mom-exception-report"
        or not process_mom_exception_case.get("commands", {}).get("matrix_requirement")
        or not process_mom_exception_case.get("commands", {}).get("matrix_section")
        or not process_mom_exception_case.get("commands", {}).get("ctest")
    ):
        raise AssertionError("process MOM exception-report case handles drifted")
    process_mom_extension_focus = query_rti_work.focused_lane_result(
        index, tests, "process-mom-extension-parameter", limit=0
    )
    if (
        process_mom_extension_focus.get("lane_state") != "complete"
        or process_mom_extension_focus.get("mapped_test_count") != 1
        or process_mom_extension_focus.get("assertion_count") != 26
        or process_mom_extension_focus.get("recorded_assertion_count") != 26
        or process_mom_extension_focus.get("requirement_count") != 3
        or process_mom_extension_focus.get("standard_section_count") != 2
        or process_mom_extension_focus.get("requirement_section_pair_count") != 3
    ):
        raise AssertionError("process MOM extension-parameter lane card drifted")
    process_mom_extension = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-mom-extension-parameter-integration"
        ),
        None,
    )
    if not isinstance(process_mom_extension, dict):
        raise AssertionError("process MOM extension-parameter plan row is absent")
    if (
        query_rti_work.source_location_text(process_mom_extension)
        != "cpp/tests/ieee1516_2025_connection_catch2.cpp:38643"
        or process_mom_extension.get("traceability_state") != "requirements-mapped"
        or process_mom_extension.get("lab_requirement_ids")
        != [
            "requirement-candidate-content-clauses-11-management-object-model-page-290-l54-17",
            "requirement-candidate-content-clauses-11-management-object-model-page-290-l66-21",
            "requirement-candidate-content-clauses-11-management-object-model-page-291-l146-48",
        ]
        or process_mom_extension.get("standard_sections")
        != [
            "hla-1516.1-2025:clause-11.4.1",
            "hla-1516.1-2025:clause-11.4.2",
        ]
        or len(process_mom_extension.get("selected_cpp_api_surface_ids", [])) != 10
    ):
        raise AssertionError("process MOM extension-parameter mapping drifted")
    process_mom_extension_case = query_rti_work.case_card_record(
        process_mom_extension,
        index,
        "umbra-cpp-process-endpoint-mom-extension-parameter-integration",
    )
    if (
        process_mom_extension_case.get("lane") != "process-mom-extension-parameter"
        or not process_mom_extension_case.get("commands", {}).get("matrix_requirement")
        or not process_mom_extension_case.get("commands", {}).get("matrix_section")
        or not process_mom_extension_case.get("commands", {}).get("ctest")
    ):
        raise AssertionError("process MOM extension-parameter case handles drifted")
    process_mom_malformed_focus = query_rti_work.focused_lane_result(
        index, tests, "process-mom-malformed-exception", limit=0
    )
    if (
        process_mom_malformed_focus.get("mapped_test_count") != 1
        or process_mom_malformed_focus.get("assertion_count") != 29
        or process_mom_malformed_focus.get("standard_section_count") != 2
        or process_mom_malformed_focus.get("requirement_section_pair_count") != 3
    ):
        raise AssertionError("process MOM malformed-exception lane mapping drifted")
    process_mom_malformed = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-mom-malformed-exception-integration"
        ),
        None,
    )
    if not isinstance(process_mom_malformed, dict):
        raise AssertionError("process MOM malformed-exception plan row is absent")
    if (
        query_rti_work.source_location_text(process_mom_malformed)
        != "cpp/tests/ieee1516_2025_connection_process_mom_malformed_exception_catch2.cpp:459"
        or process_mom_malformed.get("traceability_state") != "requirements-mapped"
        or process_mom_malformed.get("lab_requirement_ids")
        != [
            "requirement-candidate-content-clauses-11-management-object-model-page-290-l111-36",
            "requirement-candidate-content-clauses-11-management-object-model-page-290-l150-49",
            "requirement-candidate-content-clauses-11-management-object-model-page-292-l18-5",
        ]
        or process_mom_malformed.get("standard_sections")
        != [
            "hla-1516.1-2025:clause-11.4.1",
            "hla-1516.1-2025:clause-11.5",
        ]
        or len(process_mom_malformed.get("selected_cpp_api_surface_ids", [])) != 11
    ):
        raise AssertionError("process MOM malformed-exception case mapping drifted")
    process_mom_malformed_case = query_rti_work.case_card_record(
        process_mom_malformed,
        index,
        "umbra-cpp-process-endpoint-mom-malformed-exception-integration",
    )
    if (
        process_mom_malformed_case.get("lane") != "process-mom-malformed-exception"
        or not process_mom_malformed_case.get("commands", {}).get("matrix_requirement")
        or not process_mom_malformed_case.get("commands", {}).get("matrix_section")
        or not process_mom_malformed_case.get("commands", {}).get("ctest")
    ):
        raise AssertionError("process MOM malformed-exception case handles drifted")
    process_mom_malformed_parameter_focus = query_rti_work.focused_lane_result(
        index, tests, "process-mom-malformed-parameter", limit=0
    )
    if (
        process_mom_malformed_parameter_focus.get("mapped_test_count") != 1
        or process_mom_malformed_parameter_focus.get("assertion_count") != 30
        or process_mom_malformed_parameter_focus.get("standard_section_count") != 2
        or process_mom_malformed_parameter_focus.get("requirement_section_pair_count")
        != 3
    ):
        raise AssertionError("process MOM malformed-parameter lane mapping drifted")
    process_mom_malformed_parameter = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-mom-malformed-parameter-integration"
        ),
        None,
    )
    if not isinstance(process_mom_malformed_parameter, dict):
        raise AssertionError("process MOM malformed-parameter plan row is absent")
    if (
        query_rti_work.source_location_text(process_mom_malformed_parameter)
        != "cpp/tests/ieee1516_2025_connection_process_mom_malformed_parameter_catch2.cpp:459"
        or process_mom_malformed_parameter.get("traceability_state")
        != "requirements-mapped"
        or process_mom_malformed_parameter.get("lab_requirement_ids")
        != [
            "requirement-candidate-content-clauses-11-management-object-model-page-290-l111-36",
            "requirement-candidate-content-clauses-11-management-object-model-page-290-l150-49",
            "requirement-candidate-content-clauses-11-management-object-model-page-292-l18-5",
        ]
        or process_mom_malformed_parameter.get("standard_sections")
        != [
            "hla-1516.1-2025:clause-11.4.1",
            "hla-1516.1-2025:clause-11.5",
        ]
        or len(process_mom_malformed_parameter.get("selected_cpp_api_surface_ids", []))
        != 11
    ):
        raise AssertionError("process MOM malformed-parameter case mapping drifted")
    process_mom_malformed_parameter_case = query_rti_work.case_card_record(
        process_mom_malformed_parameter,
        index,
        "umbra-cpp-process-endpoint-mom-malformed-parameter-integration",
    )
    if (
        process_mom_malformed_parameter_case.get("lane")
        != "process-mom-malformed-parameter"
        or not process_mom_malformed_parameter_case.get("commands", {}).get(
            "matrix_requirement"
        )
        or not process_mom_malformed_parameter_case.get("commands", {}).get(
            "matrix_section"
        )
        or not process_mom_malformed_parameter_case.get("commands", {}).get("ctest")
    ):
        raise AssertionError("process MOM malformed-parameter case handles drifted")
    process_service_report_rejoin_focus = query_rti_work.focused_lane_result(
        index, tests, "process-service-report-file-rejoin", limit=0
    )
    if (
        process_service_report_rejoin_focus.get("mapped_test_count") != 1
        or process_service_report_rejoin_focus.get("assertion_count") != 25
        or process_service_report_rejoin_focus.get("standard_section_count") != 2
        or process_service_report_rejoin_focus.get("requirement_section_pair_count")
        != 2
    ):
        raise AssertionError("process service-report rejoin lane mapping drifted")
    process_service_report_failure_focus = query_rti_work.focused_lane_result(
        index, tests, "process-service-report-file-failure", limit=0
    )
    if (
        process_service_report_failure_focus.get("mapped_test_count") != 1
        or process_service_report_failure_focus.get("assertion_count") != 7
        or process_service_report_failure_focus.get("standard_section_count") != 2
        or process_service_report_failure_focus.get("requirement_section_pair_count")
        != 2
    ):
        raise AssertionError("process service-report failure lane mapping drifted")
    process_service_report_loss_focus = query_rti_work.focused_lane_result(
        index, tests, "process-service-report-file-loss", limit=0
    )
    if (
        process_service_report_loss_focus.get("mapped_test_count") != 0
        or process_service_report_loss_focus.get("assertion_count") != 15
        or process_service_report_loss_focus.get("explicit_disposition_count") != 1
        or process_service_report_loss_focus.get("standard_section_count") != 0
        or process_service_report_loss_focus.get("requirement_section_pair_count") != 0
    ):
        raise AssertionError("process service-report file-loss internal lane drifted")
    service_report_multifederate_focus = query_rti_work.focused_lane_result(
        index, tests, "service-report-file-multifederate", limit=0
    )
    if (
        service_report_multifederate_focus.get("mapped_test_count") != 1
        or service_report_multifederate_focus.get("assertion_count") != 18
        or service_report_multifederate_focus.get("recorded_assertion_count") != 26
        or service_report_multifederate_focus.get("requirement_count") != 2
        or service_report_multifederate_focus.get("standard_section_count") != 2
        or service_report_multifederate_focus.get("requirement_section_pair_count") != 2
    ):
        raise AssertionError("service-report multifederate lane mapping drifted")
    process_sync = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-synchronization-point-integration"
        ),
        None,
    )
    if not isinstance(process_sync, dict):
        raise AssertionError("process synchronization-point plan row is absent")
    if (
        query_rti_work.source_location_text(process_sync)
        != "cpp/tests/ieee1516_2025_connection_synchronization_restore_catch2.cpp:458"
        or process_sync.get("assertions") != 27
        or process_sync.get("primary_lane") != "process-synchronization-point"
        or len(process_sync.get("lab_requirement_ids", [])) != 6
        or len(process_sync.get("standard_sections", [])) != 6
        or len(process_sync.get("selected_cpp_api_surface_ids", [])) != 5
    ):
        raise AssertionError("process synchronization-point mapping drifted")
    process_sync_focus = query_rti_work.focused_lane_result(
        index, tests, "process-synchronization-point", limit=0
    )
    if (
        process_sync_focus.get("lane_state") != "complete"
        or process_sync_focus.get("mapped_test_count") != 1
        or process_sync_focus.get("assertion_count") != 27
        or process_sync_focus.get("requirement_count") != 6
        or process_sync_focus.get("standard_section_count") != 6
        or process_sync_focus.get("requirement_section_pair_count") != 6
        or process_sync_focus.get("lane_handles", {}).get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
        or "RTIambassadors carry federation synchronization points"
        not in str(process_sync_focus.get("lane_handles", {}).get("ctest_filter") or "")
    ):
        raise AssertionError("process synchronization-point lane card drifted")
    process_sync_explicit = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-synchronization-point-explicit-failure-integration"
        ),
        None,
    )
    if not isinstance(process_sync_explicit, dict):
        raise AssertionError("explicit synchronization-point plan row is absent")
    if (
        query_rti_work.source_location_text(process_sync_explicit)
        != "cpp/tests/ieee1516_2025_connection_synchronization_failure_catch2.cpp:457"
        or process_sync_explicit.get("assertions") != 23
        or process_sync_explicit.get("primary_lane")
        != "process-synchronization-point-explicit-failure"
        or len(process_sync_explicit.get("lab_requirement_ids", [])) != 9
        or len(process_sync_explicit.get("standard_sections", [])) != 6
        or len(process_sync_explicit.get("selected_cpp_api_surface_ids", [])) != 7
    ):
        raise AssertionError("explicit synchronization-point mapping drifted")
    process_sync_explicit_focus = query_rti_work.focused_lane_result(
        index, tests, "process-synchronization-point-explicit-failure", limit=0
    )
    if (
        process_sync_explicit_focus.get("lane_state") != "complete"
        or process_sync_explicit_focus.get("mapped_test_count") != 1
        or process_sync_explicit_focus.get("assertion_count") != 23
        or process_sync_explicit_focus.get("requirement_count") != 9
        or process_sync_explicit_focus.get("standard_section_count") != 6
        or process_sync_explicit_focus.get("requirement_section_pair_count") != 9
        or process_sync_explicit_focus.get("lane_handles", {}).get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
        or "RTIambassadors honor explicit synchronization sets and failure results"
        not in str(
            process_sync_explicit_focus.get("lane_handles", {}).get("ctest_filter")
            or ""
        )
    ):
        raise AssertionError("explicit synchronization-point lane card drifted")
    process_sync_explicit_immediate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-synchronization-point-explicit-failure-immediate-integration"
        ),
        None,
    )
    if not isinstance(process_sync_explicit_immediate, dict):
        raise AssertionError(
            "immediate explicit synchronization-point plan row is absent"
        )
    if (
        query_rti_work.source_location_text(process_sync_explicit_immediate)
        != "cpp/tests/ieee1516_2025_connection_synchronization_failure_catch2.cpp:678"
        or process_sync_explicit_immediate.get("assertions") != 23
        or process_sync_explicit_immediate.get("primary_lane")
        != "process-synchronization-point-explicit-failure-immediate"
        or process_sync_explicit_immediate.get("callback_models") != ["HLA_IMMEDIATE"]
        or len(process_sync_explicit_immediate.get("lab_requirement_ids", [])) != 9
        or len(process_sync_explicit_immediate.get("standard_sections", [])) != 6
        or len(process_sync_explicit_immediate.get("selected_cpp_api_surface_ids", []))
        != 7
    ):
        raise AssertionError("immediate explicit synchronization-point mapping drifted")
    process_sync_explicit_immediate_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-synchronization-point-explicit-failure-immediate",
        limit=0,
    )
    if (
        process_sync_explicit_immediate_focus.get("lane_state") != "complete"
        or process_sync_explicit_immediate_focus.get("mapped_test_count") != 1
        or process_sync_explicit_immediate_focus.get("assertion_count") != 23
        or process_sync_explicit_immediate_focus.get("requirement_count") != 9
        or process_sync_explicit_immediate_focus.get("standard_section_count") != 6
        or process_sync_explicit_immediate_focus.get("requirement_section_pair_count")
        != 9
        or process_sync_explicit_immediate_focus.get("lane_handles", {}).get(
            "catch2_target"
        )
        != "umbra_ieee1516_2025_catch2"
        or "pushed process endpoint"
        not in str(
            process_sync_explicit_immediate_focus.get("lane_handles", {}).get(
                "ctest_filter"
            )
            or ""
        )
    ):
        raise AssertionError(
            "immediate explicit synchronization-point lane card drifted"
        )
    process_save = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-federation-save-untimed-integration"
        ),
        None,
    )
    if not isinstance(process_save, dict):
        raise AssertionError("process federation-save plan row is absent")
    if (
        query_rti_work.source_location_text(process_save)
        != "cpp/tests/ieee1516_2025_connection_federation_save_catch2.cpp:457"
        or process_save.get("assertions") != 15
        or process_save.get("primary_lane") != "process-federation-save-untimed"
        or process_save.get("callback_models") != ["HLA_EVOKED"]
        or len(process_save.get("lab_requirement_ids", [])) != 5
        or len(process_save.get("standard_sections", [])) != 5
        or len(process_save.get("selected_cpp_api_surface_ids", [])) != 6
    ):
        raise AssertionError("process federation-save mapping drifted")
    process_save_focus = query_rti_work.focused_lane_result(
        index, tests, "process-federation-save-untimed", limit=0
    )
    if (
        process_save_focus.get("lane_state") != "complete"
        or process_save_focus.get("mapped_test_count") != 1
        or process_save_focus.get("assertion_count") != 70
        or process_save_focus.get("recorded_assertion_count") != 15
        or process_save_focus.get("requirement_count") != 5
        or process_save_focus.get("standard_section_count") != 5
        or process_save_focus.get("requirement_section_pair_count") != 5
        or process_save_focus.get("lane_handles", {}).get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or "RTIambassadors carry untimed federation save callbacks through the configured process endpoint"
        not in str(process_save_focus.get("lane_handles", {}).get("ctest_filter") or "")
    ):
        raise AssertionError("process federation-save lane card drifted")
    configuration_fallback = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-connect-configuration-fallback-integration"
        ),
        None,
    )
    if not isinstance(configuration_fallback, dict):
        raise AssertionError("configuration-fallback plan row is absent")
    if (
        _live_source_location_text(configuration_fallback)
        != "cpp/tests/ieee1516_2025_connection_configuration_fallback_catch2.cpp:459"
        or configuration_fallback.get("assertions") != 8
        or configuration_fallback.get("primary_lane") != "configuration-fallback"
        or configuration_fallback.get("callback_models") != ["HLA_EVOKED"]
        or len(configuration_fallback.get("lab_requirement_ids", [])) != 2
        or configuration_fallback.get("lab_requirement_ids", [None])[0]
        != "requirement-candidate-content-clauses-04-federation-management-page-051-l18-3"
        or configuration_fallback.get("lab_requirement_ids", [None, None])[1]
        != "requirement-candidate-content-clauses-04-federation-management-page-051-l23-4"
        or configuration_fallback.get("standard_sections")
        != [
            "hla-1516.1-2025:clause-4.2.4",
        ]
        or len(configuration_fallback.get("selected_cpp_api_surface_ids", [])) != 1
    ):
        raise AssertionError("configuration-fallback mapping drifted")
    configuration_fallback_focus = query_rti_work.focused_lane_result(
        index, tests, "configuration-fallback", limit=0
    )
    if (
        configuration_fallback_focus.get("lane_state") != "complete"
        or configuration_fallback_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or configuration_fallback_focus.get("mapped_test_count") != 1
        or configuration_fallback_focus.get("assertion_count") != 8
        or configuration_fallback_focus.get("requirement_count") != 2
        or configuration_fallback_focus.get("standard_section_count") != 1
        or configuration_fallback_focus.get("requirement_section_pair_count") != 2
        or configuration_fallback_focus.get("lane_handles", {}).get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or "Embedded Connect falls back to its default configuration"
        not in str(
            configuration_fallback_focus.get("lane_handles", {}).get("ctest_filter")
            or ""
        )
    ):
        raise AssertionError("configuration-fallback lane card drifted")
    configuration_additional_settings = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-connect-additional-settings-parse-failure-integration"
        ),
        None,
    )
    if not isinstance(configuration_additional_settings, dict):
        raise AssertionError("configuration-additional-settings plan row is absent")
    if (
        _live_source_location_text(configuration_additional_settings)
        != "cpp/tests/ieee1516_2025_connection_configuration_additional_settings_catch2.cpp:459"
        or configuration_additional_settings.get("assertions") != 5
        or configuration_additional_settings.get("primary_lane")
        != "configuration-additional-settings"
        or configuration_additional_settings.get("callback_models") != ["HLA_EVOKED"]
        or len(configuration_additional_settings.get("lab_requirement_ids", [])) != 1
        or configuration_additional_settings.get("lab_requirement_ids", [None])[0]
        != "requirement-candidate-content-clauses-04-federation-management-page-051-l28-5"
        or configuration_additional_settings.get("standard_sections")
        != [
            "hla-1516.1-2025:clause-4.2.4",
        ]
        or len(
            configuration_additional_settings.get("selected_cpp_api_surface_ids", [])
        )
        != 1
    ):
        raise AssertionError("configuration-additional-settings mapping drifted")
    configuration_additional_settings_focus = query_rti_work.focused_lane_result(
        index, tests, "configuration-additional-settings", limit=0
    )
    if (
        configuration_additional_settings_focus.get("lane_state") != "complete"
        or configuration_additional_settings_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or configuration_additional_settings_focus.get("mapped_test_count") != 1
        or configuration_additional_settings_focus.get("assertion_count") != 5
        or configuration_additional_settings_focus.get("requirement_count") != 1
        or configuration_additional_settings_focus.get("standard_section_count") != 1
        or configuration_additional_settings_focus.get("requirement_section_pair_count")
        != 1
        or configuration_additional_settings_focus.get("lane_handles", {}).get(
            "catch2_target"
        )
        != "umbra_ieee1516_2025_connection_catch2"
        or "optional additional setting cannot be parsed"
        not in str(
            configuration_additional_settings_focus.get("lane_handles", {}).get(
                "ctest_filter"
            )
            or ""
        )
    ):
        raise AssertionError("configuration-additional-settings lane card drifted")
    settings_parse_requirement = query_rti_work.select_tests(
        tests,
        "requirement-candidate-table-5-p295-r002",
        "requirement",
    )
    settings_applied_requirement = query_rti_work.select_tests(
        tests,
        "requirement-candidate-table-5-p295-r003",
        "requirement",
    )
    if [test.get("id") for test in settings_parse_requirement] != [] or [
        test.get("id") for test in settings_applied_requirement
    ] != []:
        raise AssertionError(
            "AdditionalSettingsResultCode Table 5 mapping disposition drifted"
        )
    attribute_handle_encoding = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-handle-value-semantics-sdk-baseline"
        ),
        None,
    )
    if (
        not isinstance(attribute_handle_encoding, dict)
        or attribute_handle_encoding.get("lab_requirement_ids")
        != ["requirement-candidate-table-5-p295-r004"]
        or attribute_handle_encoding.get("standard_sections")
        != ["hla-1516.1-2025:clause-11.5.1"]
        or "handle.toString()"
        not in str(attribute_handle_encoding.get("requirements_lab_api_surface_status"))
    ):
        raise AssertionError("AttributeHandle Table 5 encoding mapping drifted")
    attribute_handle_set_encoding = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-mom-service-report-object-instance-attribute-handles-unit"
        ),
        None,
    )
    if (
        not isinstance(attribute_handle_set_encoding, dict)
        or attribute_handle_set_encoding.get("lab_requirement_ids")
        != [
            "requirement-candidate-content-clauses-11-management-object-model-page-293-l95-31"
        ]
        or attribute_handle_set_encoding.get("standard_sections")
        != ["hla-1516.1-2025:clause-11.5.2"]
        or "not exported as a standalone Requirements-Lab API surface"
        not in str(
            attribute_handle_set_encoding.get("requirements_lab_api_surface_status")
        )
    ):
        raise AssertionError("AttributeHandleSet Table 5 encoding mapping drifted")
    attribute_handle_value_map_encoding = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-mom-service-report-update-attribute-values-unit"
        ),
        None,
    )
    if (
        not isinstance(attribute_handle_value_map_encoding, dict)
        or attribute_handle_value_map_encoding.get("lab_requirement_ids")
        != [
            "requirement-candidate-content-clauses-11-management-object-model-page-292-l117-38",
            "requirement-candidate-content-clauses-11-management-object-model-page-293-l95-31",
        ]
        or attribute_handle_value_map_encoding.get("standard_sections")
        != [
            "hla-1516.1-2025:clause-11.5.1",
            "hla-1516.1-2025:clause-11.5.2",
        ]
        or "not exported as a standalone Requirements-Lab API surface"
        not in str(
            attribute_handle_value_map_encoding.get(
                "requirements_lab_api_surface_status"
            )
        )
    ):
        raise AssertionError("AttributeHandleValueMap Table 5 encoding mapping drifted")
    ddm_attribute_region_encoding = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-mom-service-report-ddm-attribute-region-association-unit"
        ),
        None,
    )
    if (
        not isinstance(ddm_attribute_region_encoding, dict)
        or ddm_attribute_region_encoding.get("lab_requirement_ids")
        != [
            "requirement-candidate-content-clauses-11-management-object-model-page-292-l93-30",
            "requirement-candidate-content-clauses-11-management-object-model-page-292-l117-38",
        ]
        or ddm_attribute_region_encoding.get("standard_sections")
        != ["hla-1516.1-2025:clause-11.5.1"]
        or "not exported as a standalone Requirements-Lab API surface"
        not in str(
            ddm_attribute_region_encoding.get("requirements_lab_api_surface_status")
        )
    ):
        raise AssertionError("internal DDM service-report encoding mapping drifted")
    attribute_region_association_encoding = query_rti_work.select_tests(
        tests,
        "requirement-candidate-table-5-p295-r007",
        "requirement",
    )
    attribute_handle_set_row = query_rti_work.select_tests(
        tests,
        "requirement-candidate-table-5-p295-r005",
        "requirement",
    )
    publish_service_record_row = query_rti_work.select_tests(
        tests,
        "requirement-candidate-table-5-p295-r008",
        "requirement",
    )
    region_handle_set_row = query_rti_work.select_tests(
        tests,
        "requirement-candidate-table-5-p295-r009",
        "requirement",
    )
    attribute_region_association_list_row = query_rti_work.select_tests(
        tests,
        "requirement-candidate-table-5-p295-r010",
        "requirement",
    )
    attribute_handle_value_map_row = query_rti_work.select_tests(
        tests,
        "requirement-candidate-table-5-p295-r006",
        "requirement",
    )
    attribute_value_map_array_row = query_rti_work.select_tests(
        tests,
        "requirement-candidate-table-5-p295-r011",
        "requirement",
    )
    if (
        [test.get("id") for test in attribute_region_association_encoding]
        != [
            "umbra-cpp-associate-regions-for-updates-service-report-file-integration",
            "umbra-cpp-service-report-regional-object-attribute-association-file-integration",
        ]
        or [test.get("id") for test in attribute_handle_set_row]
        != ["umbra-cpp-publish-object-class-attributes-service-report-file-integration"]
        or [test.get("id") for test in publish_service_record_row]
        != ["umbra-cpp-publish-object-class-attributes-service-report-file-integration"]
        or [test.get("id") for test in region_handle_set_row]
        != ["umbra-cpp-commit-region-modifications-service-report-integration"]
        or [test.get("id") for test in attribute_region_association_list_row]
        != [
            "umbra-cpp-associate-regions-for-updates-service-report-file-integration",
            "umbra-cpp-service-report-regional-object-attribute-association-file-integration",
        ]
        or [test.get("id") for test in attribute_handle_value_map_row]
        != ["umbra-cpp-update-attribute-values-service-report-file-integration"]
        or [test.get("id") for test in attribute_value_map_array_row]
        != ["umbra-cpp-update-attribute-values-service-report-file-integration"]
        or attribute_region_association_encoding[0].get("standard_sections")
        != ["hla-1516.1-2025:clause-11.5.1"]
        or "AttributeRegionAssociation"
        not in str(
            attribute_region_association_encoding[0].get(
                "requirements_lab_api_surface_status"
            )
        )
    ):
        raise AssertionError(
            "AttributeRegionAssociation Table 5 encoding mapping drifted"
        )
    boolean_encoding = query_rti_work.select_tests(
        tests,
        "requirement-candidate-table-5-p295-r012",
        "requirement",
    )
    if (
        [test.get("id") for test in boolean_encoding]
        != ["umbra-cpp-mom-service-report-successful-void-record-unit"]
        or boolean_encoding[0].get("standard_sections")
        != ["hla-1516.1-2025:clause-11.5.1"]
        or "not exported as a standalone Requirements-Lab API surface"
        not in str(boolean_encoding[0].get("requirements_lab_api_surface_status"))
    ):
        raise AssertionError("Boolean Table 5 encoding mapping drifted")
    callback_model_encoding = query_rti_work.select_tests(
        tests,
        "requirement-candidate-table-5-p295-r013",
        "requirement",
    )
    initial_record_encoding = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-mom-service-report-initial-record-unit"
        ),
        None,
    )
    callback_model_log_format = next(
        (
            test
            for test in callback_model_encoding
            if test.get("id") == "umbra-cpp-service-report-log-format-concatenation"
        ),
        None,
    )
    if (
        len(callback_model_encoding) != 1
        or not isinstance(callback_model_log_format, dict)
        or "requirement-candidate-table-5-p295-r013"
        not in callback_model_log_format.get("lab_requirement_ids", [])
        or "hla-1516.1-2025:clause-11.5.1"
        not in callback_model_log_format.get("standard_sections", [])
        or "not a public-API conformance test"
        not in str(callback_model_log_format.get("requirements_lab_api_surface_status"))
        or not isinstance(initial_record_encoding, dict)
        or initial_record_encoding.get("lab_requirement_ids")
        != [
            "requirement-candidate-content-clauses-11-management-object-model-page-293-l122-40",
            "requirement-candidate-content-clauses-11-management-object-model-page-294-l20-6",
        ]
        or initial_record_encoding.get("standard_sections")
        != [
            "hla-1516.1-2025:clause-11.5.2",
            "hla-1516.1-2025:clause-11.5.2.1",
        ]
        or "not exported as a standalone Requirements-Lab API surface"
        not in str(initial_record_encoding.get("requirements_lab_api_surface_status"))
    ):
        raise AssertionError("CallbackModel Table 5 encoding mapping drifted")
    process_save_status = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-federation-save-status-integration"
        ),
        None,
    )
    if not isinstance(process_save_status, dict):
        raise AssertionError("process federation-save status plan row is absent")
    if (
        query_rti_work.source_location_text(process_save_status)
        != "cpp/tests/ieee1516_2025_connection_federation_save_catch2.cpp:2182"
        or process_save_status.get("assertions") != 19
        or process_save_status.get("primary_lane") != "process-federation-save-status"
        or process_save_status.get("callback_models") != ["HLA_EVOKED"]
        or len(process_save_status.get("lab_requirement_ids", [])) != 7
        or len(process_save_status.get("standard_sections", [])) != 5
        or len(process_save_status.get("selected_cpp_api_surface_ids", [])) != 8
    ):
        raise AssertionError("process federation-save status mapping drifted")
    process_save_status_focus = query_rti_work.focused_lane_result(
        index, tests, "process-federation-save-status", limit=0
    )
    if (
        process_save_status_focus.get("lane_state") != "complete"
        or process_save_status_focus.get("mapped_test_count") != 1
        or process_save_status_focus.get("assertion_count") != 19
        or process_save_status_focus.get("requirement_count") != 7
        or process_save_status_focus.get("standard_section_count") != 5
        or process_save_status_focus.get("requirement_section_pair_count") != 7
        or process_save_status_focus.get("lane_handles", {}).get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
        or "federation save status responses"
        not in str(
            process_save_status_focus.get("lane_handles", {}).get("ctest_filter") or ""
        )
    ):
        raise AssertionError("process federation-save status lane card drifted")
    process_save_abort = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-federation-save-abort-integration"
        ),
        None,
    )
    if not isinstance(process_save_abort, dict):
        raise AssertionError("process federation-save abort plan row is absent")
    if (
        query_rti_work.source_location_text(process_save_abort)
        != "cpp/tests/ieee1516_2025_connection_federation_save_catch2.cpp:2329"
        or process_save_abort.get("assertions") != 13
        or process_save_abort.get("primary_lane") != "process-federation-save-abort"
        or process_save_abort.get("callback_models") != ["HLA_EVOKED"]
        or len(process_save_abort.get("lab_requirement_ids", [])) != 3
        or len(process_save_abort.get("standard_sections", [])) != 2
        or len(process_save_abort.get("selected_cpp_api_surface_ids", [])) != 5
    ):
        raise AssertionError("process federation-save abort mapping drifted")
    process_save_abort_focus = query_rti_work.focused_lane_result(
        index, tests, "process-federation-save-abort", limit=0
    )
    if (
        process_save_abort_focus.get("lane_state") != "complete"
        or process_save_abort_focus.get("mapped_test_count") != 1
        or process_save_abort_focus.get("assertion_count") != 13
        or process_save_abort_focus.get("requirement_count") != 3
        or process_save_abort_focus.get("standard_section_count") != 2
        or process_save_abort_focus.get("requirement_section_pair_count") != 3
        or process_save_abort_focus.get("lane_handles", {}).get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
        or "federation save abort"
        not in str(
            process_save_abort_focus.get("lane_handles", {}).get("ctest_filter") or ""
        )
    ):
        raise AssertionError("process federation-save abort lane card drifted")
    evoked_sync_restore = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-synchronization-point-save-restore-evoked-integration"
        ),
        None,
    )
    if not isinstance(evoked_sync_restore, dict):
        raise AssertionError("evoked synchronization save/restore plan row is absent")
    if (
        query_rti_work.source_location_text(evoked_sync_restore)
        != "cpp/tests/ieee1516_2025_federation_management_save_restore_catch2.cpp:851"
        or evoked_sync_restore.get("assertions") != 48
        or evoked_sync_restore.get("primary_lane")
        != "evoked-synchronization-save-restore"
        or evoked_sync_restore.get("callback_models") != ["HLA_EVOKED"]
        or len(evoked_sync_restore.get("lab_requirement_ids", [])) != 9
        or len(evoked_sync_restore.get("standard_sections", [])) != 9
        or len(evoked_sync_restore.get("selected_cpp_api_surface_ids", [])) != 17
    ):
        raise AssertionError("evoked synchronization save/restore mapping drifted")
    evoked_sync_restore_focus = query_rti_work.focused_lane_result(
        index, tests, "evoked-synchronization-save-restore", limit=0
    )
    if (
        evoked_sync_restore_focus.get("lane_state") != "complete"
        or evoked_sync_restore_focus.get("mapped_test_count") != 1
        or evoked_sync_restore_focus.get("assertion_count") != 39
        or evoked_sync_restore_focus.get("requirement_count") != 9
        or evoked_sync_restore_focus.get("standard_section_count") != 9
        or evoked_sync_restore_focus.get("requirement_section_pair_count") != 9
        or evoked_sync_restore_focus.get("lane_handles", {}).get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
        or "Embedded evoked federation restore reconstitutes"
        not in str(
            evoked_sync_restore_focus.get("lane_handles", {}).get("ctest_filter") or ""
        )
    ):
        raise AssertionError("evoked synchronization save/restore lane card drifted")
    process_name_lane = query_rti_work.focused_lane_result(
        index, tests, "process-object-instance-name-reservation", limit=0
    )
    if (
        process_name_lane.get("lane_state") != "complete"
        or process_name_lane.get("mapped_test_count") != 1
        or process_name_lane.get("assertion_count") != 77
        or process_name_lane.get("requirement_count") != 13
        or process_name_lane.get("standard_section_count") != 8
        or process_name_lane.get("requirement_section_pair_count") != 13
        or process_name_lane.get("lane_handles", {}).get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
    ):
        raise AssertionError(
            "process object-instance-name reservation lane card drifted"
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
        "cpp/tests/ieee1516_2025_connection_process_query_time_bounds_immediate_catch2.cpp:459"
    ):
        raise AssertionError(
            "focused process immediate GALT/LITS source pointer drifted"
        )
    if process_query_time_bounds_immediate.get("assertions") != 14:
        raise AssertionError(
            "focused process immediate GALT/LITS assertion count drifted"
        )
    if process_query_time_bounds_immediate.get("callback_models") != ["HLA_IMMEDIATE"]:
        raise AssertionError(
            "focused process immediate GALT/LITS callback-model mapping drifted"
        )
    if process_query_time_bounds_immediate.get("primary_lane") != (
        "process-query-time-bounds-immediate"
    ):
        raise AssertionError("focused process immediate GALT/LITS primary lane drifted")
    if (
        process_query_time_bounds_immediate.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("focused process immediate GALT/LITS row is not mapped")
    process_query_time_bounds_immediate_focus = query_rti_work.focused_lane_result(
        index, tests, "process-query-time-bounds-immediate", limit=0
    )
    if (
        process_query_time_bounds_immediate_focus.get("lane_state") != "complete"
        or process_query_time_bounds_immediate_focus.get("mapped_test_count") != 1
        or process_query_time_bounds_immediate_focus.get("assertion_count") != 14
        or process_query_time_bounds_immediate_focus.get("requirement_count") != 5
        or process_query_time_bounds_immediate_focus.get("standard_section_count") != 5
    ):
        raise AssertionError("focused process immediate GALT/LITS lane totals drifted")
    process_query_time_bounds_immediate_handles = (
        process_query_time_bounds_immediate_focus.get("lane_handles")
    )
    if (
        not isinstance(process_query_time_bounds_immediate_handles, dict)
        or process_query_time_bounds_immediate_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError(
            "focused process immediate GALT/LITS target handle drifted"
        )
    if (
        process_query_time_bounds_immediate_handles.get("ctest_label")
        != "process-boundary"
    ):
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
    if query_rti_work.source_location_text(
        process_query_time_bounds_multi_federate
    ) != (
        "cpp/tests/ieee1516_2025_connection_process_query_time_bounds_multi_federate_catch2.cpp:460"
    ):
        raise AssertionError(
            "focused process multi-federate GALT/LITS source pointer drifted"
        )
    if process_query_time_bounds_multi_federate.get("assertions") != 39:
        raise AssertionError(
            "focused process multi-federate GALT/LITS assertion count drifted"
        )
    if process_query_time_bounds_multi_federate.get("callback_models") != [
        "HLA_IMMEDIATE"
    ]:
        raise AssertionError(
            "focused process multi-federate GALT/LITS callback-model mapping drifted"
        )
    if process_query_time_bounds_multi_federate.get("primary_lane") != (
        "process-query-time-bounds-multi-federate"
    ):
        raise AssertionError(
            "focused process multi-federate GALT/LITS primary lane drifted"
        )
    if (
        process_query_time_bounds_multi_federate.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused process multi-federate GALT/LITS row is not mapped"
        )
    process_query_time_bounds_multi_federate_focus = query_rti_work.focused_lane_result(
        index, tests, "process-query-time-bounds-multi-federate", limit=0
    )
    if (
        process_query_time_bounds_multi_federate_focus.get("lane_state") != "complete"
        or process_query_time_bounds_multi_federate_focus.get("mapped_test_count") != 1
        or process_query_time_bounds_multi_federate_focus.get("assertion_count") != 39
        or process_query_time_bounds_multi_federate_focus.get(
            "recorded_assertion_count"
        )
        != 39
        or process_query_time_bounds_multi_federate_focus.get("requirement_count") != 8
        or process_query_time_bounds_multi_federate_focus.get("standard_section_count")
        != 8
        or process_query_time_bounds_multi_federate_focus.get(
            "requirement_section_pair_count"
        )
        != 8
    ):
        raise AssertionError(
            "focused process multi-federate GALT/LITS lane totals drifted"
        )
    process_query_time_bounds_multi_federate_handles = (
        process_query_time_bounds_multi_federate_focus.get("lane_handles")
    )
    if (
        not isinstance(process_query_time_bounds_multi_federate_handles, dict)
        or process_query_time_bounds_multi_federate_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError(
            "focused process multi-federate GALT/LITS target handle drifted"
        )
    if (
        process_query_time_bounds_multi_federate_handles.get("ctest_label")
        != "process-boundary"
    ):
        raise AssertionError(
            "focused process multi-federate GALT/LITS CTest label drifted"
        )
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
        "cpp/tests/ieee1516_2025_connection_process_query_time_bounds_queued_tso_catch2.cpp:459"
    ):
        raise AssertionError(
            "focused process queued-TSO GALT/LITS source pointer drifted"
        )
    if process_query_time_bounds_queued_tso.get("assertions") != 53:
        raise AssertionError(
            "focused process queued-TSO GALT/LITS assertion count drifted"
        )
    if process_query_time_bounds_queued_tso.get("callback_models") != ["HLA_IMMEDIATE"]:
        raise AssertionError(
            "focused process queued-TSO GALT/LITS callback-model mapping drifted"
        )
    if process_query_time_bounds_queued_tso.get("primary_lane") != (
        "process-query-time-bounds-queued-tso"
    ):
        raise AssertionError(
            "focused process queued-TSO GALT/LITS primary lane drifted"
        )
    if (
        process_query_time_bounds_queued_tso.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("focused process queued-TSO GALT/LITS row is not mapped")
    process_query_time_bounds_queued_tso_focus = query_rti_work.focused_lane_result(
        index, tests, "process-query-time-bounds-queued-tso", limit=0
    )
    if (
        process_query_time_bounds_queued_tso_focus.get("lane_state") != "complete"
        or process_query_time_bounds_queued_tso_focus.get("mapped_test_count") != 1
        or process_query_time_bounds_queued_tso_focus.get("assertion_count") != 53
        or process_query_time_bounds_queued_tso_focus.get("requirement_count") != 5
        or process_query_time_bounds_queued_tso_focus.get("standard_section_count") != 5
    ):
        raise AssertionError("focused process queued-TSO GALT/LITS lane totals drifted")
    process_query_time_bounds_queued_tso_handles = (
        process_query_time_bounds_queued_tso_focus.get("lane_handles")
    )
    if (
        not isinstance(process_query_time_bounds_queued_tso_handles, dict)
        or process_query_time_bounds_queued_tso_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError(
            "focused process queued-TSO GALT/LITS target handle drifted"
        )
    if (
        process_query_time_bounds_queued_tso_handles.get("ctest_label")
        != "process-boundary"
    ):
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
    if query_rti_work.source_location_text(
        process_query_time_bounds_zero_lookahead
    ) != (
        "cpp/tests/ieee1516_2025_connection_process_query_time_bounds_zero_lookahead_catch2.cpp:459"
    ):
        raise AssertionError(
            "focused process zero-lookahead GALT/LITS source pointer drifted"
        )
    if process_query_time_bounds_zero_lookahead.get("assertions") != 47:
        raise AssertionError(
            "focused process zero-lookahead GALT/LITS assertion count drifted"
        )
    if process_query_time_bounds_zero_lookahead.get("callback_models") != [
        "HLA_EVOKED"
    ]:
        raise AssertionError(
            "focused process zero-lookahead GALT/LITS callback-model mapping drifted"
        )
    if process_query_time_bounds_zero_lookahead.get("primary_lane") != (
        "process-query-time-bounds-zero-lookahead"
    ):
        raise AssertionError(
            "focused process zero-lookahead GALT/LITS primary lane drifted"
        )
    if (
        process_query_time_bounds_zero_lookahead.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused process zero-lookahead GALT/LITS row is not mapped"
        )
    process_query_time_bounds_zero_lookahead_focus = query_rti_work.focused_lane_result(
        index, tests, "process-query-time-bounds-zero-lookahead", limit=0
    )
    if (
        process_query_time_bounds_zero_lookahead_focus.get("lane_state") != "complete"
        or process_query_time_bounds_zero_lookahead_focus.get("mapped_test_count") != 1
        or process_query_time_bounds_zero_lookahead_focus.get("assertion_count") != 47
        or process_query_time_bounds_zero_lookahead_focus.get("requirement_count") != 6
        or process_query_time_bounds_zero_lookahead_focus.get("standard_section_count")
        != 5
    ):
        raise AssertionError(
            "focused process zero-lookahead GALT/LITS lane totals drifted"
        )
    process_query_time_bounds_zero_lookahead_handles = (
        process_query_time_bounds_zero_lookahead_focus.get("lane_handles")
    )
    if (
        not isinstance(process_query_time_bounds_zero_lookahead_handles, dict)
        or process_query_time_bounds_zero_lookahead_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError(
            "focused process zero-lookahead GALT/LITS target handle drifted"
        )
    if (
        process_query_time_bounds_zero_lookahead_handles.get("ctest_label")
        != "process-boundary"
    ):
        raise AssertionError(
            "focused process zero-lookahead GALT/LITS CTest label drifted"
        )
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
    if (
        not isinstance(process_fqr_galt_handles, dict)
        or process_fqr_galt_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
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
    if (
        not isinstance(process_fqr_multiple_handles, dict)
        or process_fqr_multiple_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
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
    if (
        not isinstance(process_fqr_retraction_handles, dict)
        or process_fqr_retraction_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
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
        "process-service-report-file-switch-cycle",
        "process-mom-set-switches",
        "process-mom-service-report-interlock",
        "process-service-report-file-rejoin",
        "process-service-report-file-failure",
        "process-service-report-file-loss",
        "process-directed-interaction-routing",
        "process-directed-interaction-transportation",
        "process-transportation-timestamped-regional-interaction-control",
        "process-synchronization-point",
        "process-synchronization-point-explicit-failure",
        "process-synchronization-point-explicit-failure-immediate",
        "process-federation-save-untimed",
        "process-federation-save-status",
        "process-federation-save-abort",
        "process-federation-restore-request-failure",
        "process-federation-restore-request-failure-immediate",
        "process-federation-restore-failure-lifecycle-immediate",
        "process-federation-restore-success-lifecycle-immediate",
        "process-federation-restore-abort-immediate",
        "process-federation-restore-success-lifecycle",
        "process-federation-restore-failure-lifecycle",
        "process-federation-restore-abort",
        "process-federation-restore-status",
        "process-federation-restore-status-idle-terminal",
        "process-federation-restore-status-multi-federate-immediate",
        "process-federation-restore-status-multi-federate",
        "process-federation-restore-work-item-ownership-assumption",
        "process-federation-restore-work-item-ownership-assumption-immediate",
        "process-federation-restore-work-item-ownership-assumption-push",
        "process-pushed-ownership-assumption-evoked",
        "process-pushed-ownership-assumption-evoked-save-restore",
    }.issubset(set(process_item.get("focused_lane_tags", []))):
        raise AssertionError("process Flush Queue lanes are not roadmap-indexed")
    status = run_lane_catalog(
        LaneCatalogInputs(
            index=index,
            plan=plan,
            process_item=process_item,
            process_lane=process_lane,
            process_rows=process_rows,
            queue_by_id=queue_by_id,
            source_health=source_health,
            source_locations=source_locations,
            standard_requirements=standard_requirements,
            tests=tests,
        )
    )
    restored_default_region_multi = run_mapping_rows(
        MappingRowInputs(
            contract_links=contract_links,
            declared_executables=declared_executables,
            index=index,
            ownership_transfer=ownership_transfer,
            standard_requirements=standard_requirements,
            tests=tests,
        )
    )
    run_family_matrix(
        FamilyMatrixInputs(
            active_handoff=active_handoff,
            index=index,
            queue=queue,
            restored_default_region_multi=restored_default_region_multi,
            source_locations=source_locations,
            standard_requirements=standard_requirements,
            status=status,
            tests=tests,
        )
    )
    return run_lane_search(
        LaneSearchInputs(
            index=index,
            pairs=pairs,
            source_locations=source_locations,
            tests=tests,
        )
    )


if __name__ == "__main__":
    raise SystemExit(main())
