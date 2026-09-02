from __future__ import annotations

import json
import tempfile
import unittest
from dataclasses import replace
from pathlib import Path

import tomllib
from umbra_rti_test_support import (
    CALLBACK_OVERLOAD_COUNTS,
    CallbackDeliveryObservation,
    SurfaceEventTrace,
    assert_callback_delivery_parity,
    assert_event_order,
    iter_callback_provenance_matrix,
    iter_data_element_value_matrix,
    iter_extendable_variant_wire_matrix,
    iter_logical_time_arithmetic_matrix,
    iter_logical_time_wire_matrix,
    iter_save_restore_matrix,
    iter_surface_matrix,
    iter_vendor_data_element_encode_matrix,
    iter_vendor_data_element_wire_matrix,
    iter_vendor_time_arithmetic_matrix,
    normalize_callback_delivery,
)

from tools.verify_python_surface_report import verify


class PythonSurfaceReportTest(unittest.TestCase):
    def test_package_metadata_keeps_edition_groups_and_wheel_roots_isolated(
        self,
    ) -> None:
        """Distribution metadata must preserve the two public edition routes."""

        package_root = Path(__file__).parents[2]

        def metadata(package: str) -> dict[str, object]:
            with (package_root / package / "pyproject.toml").open("rb") as stream:
                return tomllib.load(stream)

        api = metadata("umbra-rti-api")
        api_project = api["project"]
        self.assertEqual(api_project["name"], "hla-rti-api")  # type: ignore[index]
        self.assertEqual(
            api["tool"]["hatch"]["build"]["targets"]["wheel"]["packages"],  # type: ignore[index]
            ["src/hla"],
        )

        jpype = metadata("umbra-rti-jpype")
        jpype_project = jpype["project"]
        self.assertEqual(jpype_project["name"], "umbra-rti-jpype")  # type: ignore[index]
        self.assertEqual(
            jpype_project["entry-points"]["hla.rti1516e.factories"],  # type: ignore[index]
            {"java-2010": "umbra._java.rti1516e:Java2010RtiFactory"},
        )
        self.assertEqual(
            set(jpype_project["entry-points"]["hla.rti1516_2025.factories"]),  # type: ignore[index]
            {"java", "umbra-jni"},
        )
        self.assertEqual(
            jpype["tool"]["hatch"]["build"]["targets"]["wheel"]["packages"],  # type: ignore[index]
            ["src/umbra"],
        )

        native = metadata("umbra-rti-native")
        native_project = native["project"]
        self.assertEqual(native_project["name"], "umbra-rti-native")  # type: ignore[index]
        self.assertEqual(
            native_project["entry-points"]["hla.rti1516e.factories"],  # type: ignore[index]
            {"UmbraNative2010": "umbra._native.rti1516e:Native2010RtiFactory"},
        )
        self.assertEqual(
            native_project["entry-points"]["hla.rti1516_2025.factories"],  # type: ignore[index]
            {"Umbra": "umbra._native.rti1516_2025:UmbraRtiFactory"},
        )
        self.assertEqual(
            native["tool"]["scikit-build"]["wheel"]["packages"],  # type: ignore[index]
            ["src/umbra"],
        )

    def test_public_namespaces_keep_2010_and_2025_exports_edition_scoped(self) -> None:
        """The Python import surface must not silently merge the editions."""

        import hla.rti1516_2025 as current
        import hla.rti1516e as legacy

        self.assertEqual(legacy.STANDARD_EDITION, "IEEE 1516.1-2010")
        self.assertEqual(legacy.JAVA_PACKAGE, "hla.rti1516e")
        self.assertEqual(legacy.CPP_NAMESPACE, "rti1516e")
        self.assertEqual(current.STANDARD_EDITION, "IEEE 1516.1-2025")
        self.assertEqual(current.JAVA_PACKAGE, "hla.rti1516_2025")
        self.assertEqual(current.CPP_NAMESPACE, "RTI")

        # These are Java 1516.1-2025 additions (not 2010 compatibility
        # aliases).  A provider transplanted behind the 2010 route must not
        # accidentally import them from the newer namespace.
        current_only = {
            "AdditionalSettingsResultCode",
            "ConfigurationResult",
            "RtiConfiguration",
            "TimeQueryResult",
            "HLAunsignedInteger16BE",
            "HLAunsignedInteger16LE",
            "HLAunsignedInteger32BE",
            "HLAunsignedInteger32LE",
            "HLAunsignedInteger64BE",
            "HLAunsignedInteger64LE",
        }
        for name in current_only:
            with self.subTest(current_only=name):
                self.assertIn(name, current.__all__)
                self.assertTrue(hasattr(current, name))
                self.assertNotIn(name, legacy.__all__)
                self.assertFalse(hasattr(legacy, name))

        # These callback/value records are specific to the Java 2010
        # contract.  Keeping them out of the 2025 top-level namespace makes
        # an edition mismatch fail at import time instead of at a callback.
        legacy_only = {
            "MessageRetractionReturn",
            "SupplementalReflectInfo",
            "SupplementalReceiveInfo",
            "SupplementalRemoveInfo",
            "TimeQueryReturn",
        }
        for name in legacy_only:
            with self.subTest(legacy_only=name):
                self.assertIn(name, legacy.__all__)
                self.assertTrue(hasattr(legacy, name))
                self.assertNotIn(name, current.__all__)
                self.assertFalse(hasattr(current, name))

        self.assertNotEqual(
            legacy.RtiFactoryFactory._ENTRY_POINT_GROUP,
            current.RtiFactoryFactory._ENTRY_POINT_GROUP,
        )
        self.assertEqual(
            legacy.RtiFactoryFactory._ENTRY_POINT_GROUP,
            "hla.rti1516e.factories",
        )
        self.assertEqual(
            current.RtiFactoryFactory._ENTRY_POINT_GROUP,
            "hla.rti1516_2025.factories",
        )

    def test_each_edition_links_a_detailed_jni_roundtrip_catalog(self) -> None:
        """Keep the compact report anchored to both carrier inventories."""

        root = Path(__file__).parents[3]
        catalog = json.loads(
            (root / "compliance" / "catalogs" / "python-surface-completeness-catalog.json")
            .read_text(encoding="utf-8")
        )
        expected = {
            "2010": (
                "compliance/catalogs/ieee1516e-2010-jni-type-roundtrip-catalog.json",
                "IEEE 1516.1-2010",
                79,
            ),
            "2025": (
                "compliance/catalogs/ieee1516-2025-jni-type-roundtrip-catalog.json",
                "IEEE 1516.1-2025",
                103,
            ),
        }
        for edition in catalog["editions"]:
            identifier = edition["id"]
            path_text, standard, vector_count = expected[identifier]
            self.assertEqual(edition["roundtrip_catalog"], path_text)
            detail = json.loads((root / path_text).read_text(encoding="utf-8"))
            self.assertEqual(detail["standard"], standard)
            self.assertEqual(detail["basic_value_matrix"]["vector_count"], vector_count)
            self.assertTrue(detail["route"].startswith("Python -> JPype ->"))
            # The 2010 catalog predates the 2025 normalized section names;
            # accept its documented aliases while requiring the same
            # transport inventory on both edition routes.
            handle_matrix = detail.get("handle_and_collection_carrier_matrix")
            if handle_matrix is None:
                handle_matrix = detail["python_carrier_facade_matrix"]
                handles = handle_matrix["handle_families"]
            else:
                handles = handle_matrix["handles"]
            record_matrix = detail.get("record_enum_and_exception_carriers")
            if record_matrix is None:
                records = detail["python_carrier_facade_matrix"]["record_carriers"]
            else:
                records = record_matrix["records"]
            self.assertTrue(handles)
            self.assertTrue(records)

    def test_data_element_value_matrix_is_edition_scoped_and_deterministic(
        self,
    ) -> None:
        legacy = iter_data_element_value_matrix("2010")
        current = iter_data_element_value_matrix("2025")
        self.assertEqual(len(legacy), 79)
        self.assertEqual(len(current), 103)
        self.assertEqual(
            {vector.kind for vector in legacy},
            {
                "HLAASCIIchar",
                "HLAASCIIstring",
                "HLAboolean",
                "HLAbyte",
                "HLAfloat32BE",
                "HLAfloat32LE",
                "HLAfloat64BE",
                "HLAfloat64LE",
                "HLAinteger16BE",
                "HLAinteger16LE",
                "HLAinteger32BE",
                "HLAinteger32LE",
                "HLAinteger64BE",
                "HLAinteger64LE",
                "HLAoctet",
                "HLAoctetPairBE",
                "HLAoctetPairLE",
                "HLAopaqueData",
                "HLAunicodeChar",
                "HLAunicodeString",
            },
        )
        self.assertTrue(
            {vector.kind for vector in current}
            >= {"HLAunsignedInteger16BE", "HLAunsignedInteger64LE"}
        )
        self.assertEqual(len({vector.case_id for vector in current}), len(current))
        self.assertEqual(current[0].case_id, "HLAinteger16BE:minimum")
        with self.assertRaises(ValueError):
            iter_data_element_value_matrix("1999")  # type: ignore[arg-type]

    def test_surface_matrix_is_deterministic_and_partial_order_aware(self) -> None:
        cases = iter_surface_matrix()
        self.assertEqual(len(cases), 120)
        self.assertEqual(
            cases[0].case_id,
            "HLA_IMMEDIATE:HLAinteger64Time:scalar:timeAdvanceRequest:members=1",
        )
        self.assertEqual(
            {case.advance_service for case in cases},
            {
                "timeAdvanceRequest",
                "timeAdvanceRequestAvailable",
                "nextMessageRequest",
                "nextMessageRequestAvailable",
                "flushQueueRequest",
            },
        )
        self.assertEqual(len({case.case_id for case in cases}), len(cases))
        trace = SurfaceEventTrace()
        trace.record("advisory")
        trace.record("timeRegulationEnabled")
        trace.record("timeAdvanceGrant")
        assert_event_order(trace.events, "timeRegulationEnabled", "timeAdvanceGrant")

        with self.assertRaises(ValueError):
            iter_surface_matrix(member_counts=(True,))

    def test_callback_provenance_matrix_is_shared_and_deterministic(self) -> None:
        vectors = iter_callback_provenance_matrix()
        self.assertEqual(len(vectors), 5)
        self.assertEqual(len({vector.case_id for vector in vectors}), len(vectors))
        self.assertEqual(
            [vector.callback for vector in vectors],
            [
                "discoverObjectInstance",
                "reflectAttributeValues",
                "receiveInteraction",
                "reflectAttributeValues",
                "receiveInteraction",
            ],
        )
        self.assertEqual(
            [vector.label for vector in vectors if vector.timed],
            ["reflection-timed-region", "interaction-timed-no-region"],
        )
        self.assertEqual(vectors[0].producing_federate, b"callback-producer")
        self.assertEqual(vectors[-1].received_order, "RECEIVE")

    def test_callback_delivery_normalization_preserves_per_member_order(self) -> None:
        """Differential checks ignore cross-member interleaving, not gaps."""

        vectors = iter_callback_provenance_matrix()
        expected = tuple(
            CallbackDeliveryObservation.from_vector(
                vector,
                recipient=recipient,
                sequence=sequence,
            )
            for recipient in ("member-a", "member-b")
            for sequence, vector in enumerate(vectors)
        )
        # A provider may interleave member streams differently.  Canonical
        # normalization must still compare the same per-member sequences.
        actual = tuple(reversed(expected))
        self.assertEqual(
            normalize_callback_delivery(actual),
            normalize_callback_delivery(expected),
        )
        assert_callback_delivery_parity(expected, actual)

        broken = list(expected)
        broken[4] = replace(broken[4], sequence=5)
        with self.assertRaises(ValueError):
            normalize_callback_delivery(broken)

        changed = list(expected)
        changed[0] = replace(changed[0], tag=b"different-tag")
        with self.assertRaises(AssertionError):
            assert_callback_delivery_parity(expected, changed)

    def test_wire_matrix_keeps_boundary_vectors_stable_for_both_time_families(
        self,
    ) -> None:
        vectors = iter_logical_time_wire_matrix()
        self.assertEqual(
            {vector.implementation for vector in vectors},
            {"HLAinteger64Time", "HLAfloat64Time"},
        )
        self.assertEqual(len(vectors), 16)
        self.assertEqual(len({vector.case_id for vector in vectors}), len(vectors))
        self.assertEqual(
            sum(
                vector.valid
                for vector in vectors
                if vector.implementation == "HLAinteger64Time"
            ),
            4,
        )
        self.assertEqual(
            sum(
                vector.valid
                for vector in vectors
                if vector.implementation == "HLAfloat64Time"
            ),
            4,
        )
        arithmetic = iter_logical_time_arithmetic_matrix()
        self.assertEqual(len(arithmetic), 4)
        self.assertEqual(
            arithmetic[0].expected_sum,
            2**53 + 8,
        )

    def test_provider_extension_wire_matrix_keeps_unknown_and_malformed_cases(
        self,
    ) -> None:
        vectors = iter_extendable_variant_wire_matrix()
        self.assertEqual(len(vectors), 15)
        self.assertEqual(len({vector.case_id for vector in vectors}), len(vectors))
        self.assertEqual(
            [vector.label for vector in vectors if vector.valid],
            ["known-integer", "known-ascii", "unknown-future", "unknown-empty"],
        )
        self.assertIsNone(vectors[2].value)
        self.assertEqual(vectors[2].encoded_length, 8)
        self.assertIsNone(vectors[3].value)
        self.assertEqual(vectors[3].encoded_length, 8)
        self.assertEqual(sum(vector.valid for vector in vectors), 4)
        vendor_arithmetic = iter_vendor_time_arithmetic_matrix()
        self.assertEqual(len(vendor_arithmetic), 7)
        self.assertEqual(
            {vector.operation for vector in vendor_arithmetic},
            {"add", "subtract", "distance", "compareTo"},
        )
        self.assertEqual(len(iter_vendor_data_element_wire_matrix()), 4)
        encode_matrix = iter_vendor_data_element_encode_matrix()
        self.assertEqual(len(encode_matrix), 4)
        self.assertEqual(
            [vector.label for vector in encode_matrix if vector.valid],
            ["exact-offset-trailing", "exact-origin"],
        )
        self.assertEqual(
            [vector.label for vector in encode_matrix if not vector.valid],
            ["truncated-offset", "empty-offset"],
        )

    def test_save_restore_matrix_covers_each_standard_outcome_pair(self) -> None:
        cases = iter_save_restore_matrix()
        self.assertEqual(len(cases), 720)
        self.assertEqual(len({case.case_id for case in cases}), len(cases))
        self.assertEqual(cases[0].save_service, "federateSaveComplete")
        self.assertEqual(cases[0].restore_service, "federateRestoreComplete")
        self.assertEqual(
            {case.advance_service for case in cases},
            {
                "timeAdvanceRequest",
                "timeAdvanceRequestAvailable",
                "nextMessageRequest",
                "nextMessageRequestAvailable",
                "flushQueueRequest",
            },
        )
        self.assertEqual(
            {(case.save_outcome, case.restore_outcome) for case in cases},
            {
                (save_outcome, restore_outcome)
                for save_outcome in ("complete", "not-complete", "abort")
                for restore_outcome in ("complete", "not-complete", "abort")
            },
        )
        with self.assertRaises(ValueError):
            iter_save_restore_matrix(save_outcomes=("unknown",))
        with self.assertRaises(ValueError):
            iter_save_restore_matrix(member_counts=(True,))

    def test_checked_in_report_covers_both_editions_and_routes(self) -> None:
        result = verify()
        self.assertEqual(result["status"], "pass", result["findings"])
        self.assertEqual(
            {edition["id"] for edition in result["editions"]},
            {"2010", "2025"},
        )
        self.assertTrue(
            all(
                adapter["status"] == "pass"
                for edition in result["editions"]
                for adapter in edition["adapters"]
            )
        )
        dimensions = {
            item["id"]: item["status"] for item in result["coverage_dimensions"]
        }
        self.assertEqual(
            dimensions["callback-ordering-and-member-state-space"], "matrix-covered"
        )
        self.assertEqual(dimensions["direct-native-completeness"], "bounded")
        self.assertEqual(
            dimensions["vendor-specific-floating-and-non-time-malformed-matrices"],
            "deferred",
        )
        self.assertEqual(
            dimensions["provider-extension-wire-boundaries"], "matrix-covered"
        )
        matrices = {item["id"]: item for item in result["matrix_evidence"]}
        self.assertEqual(
            matrices["basic-data-element-values"]["editions"]["2010"]["vector_count"],
            79,
        )
        self.assertEqual(
            matrices["basic-data-element-values"]["editions"]["2025"]["vector_count"],
            103,
        )
        self.assertEqual(
            matrices["callback-ordering-member-state-space"]["vector_count"], 120
        )
        self.assertEqual(
            matrices["callback-provenance-payload-parity"]["vector_count"], 5
        )
        self.assertEqual(
            matrices["callback-delivery-provenance-normalization"]["vector_count"],
            10,
        )
        self.assertEqual(
            matrices["callback-overload-carriers"]["editions"]["2010"][
                "vector_count"
            ],
            CALLBACK_OVERLOAD_COUNTS["2010"] * 2,
        )
        self.assertEqual(
            matrices["callback-overload-carriers"]["editions"]["2025"][
                "vector_count"
            ],
            CALLBACK_OVERLOAD_COUNTS["2025"] * 2,
        )
        self.assertEqual(matrices["save-restore-outcomes"]["vector_count"], 720)
        self.assertEqual(
            matrices["logical-time-wire-and-arithmetic"]["wire_vector_count"], 16
        )
        self.assertEqual(
            matrices["logical-time-wire-and-arithmetic"]["arithmetic_vector_count"],
            4,
        )
        self.assertEqual(matrices["provider-extension-wire"]["vector_count"], 15)
        self.assertEqual(
            matrices["vendor-time-arithmetic-surface"]["vector_count"], 7
        )
        self.assertIn(
            "packages/umbra-rti-jpype/tests/test_java_provider.py"
            "#test_java_provider_preserves_custom_2025_time_factory_and_arithmetic",
            matrices["vendor-time-arithmetic-surface"]["routes"],
        )
        self.assertIn(
            "packages/umbra-rti-jpype/tests/test_java_2010_provider.py"
            "#test_ambassador_queries_preserve_unknown_vendor_time_carriers",
            next(
                item["evidence"]
                for item in result["coverage_dimensions"]
                if item["id"] == "vendor-specific-floating-and-non-time-malformed-matrices"
            ),
        )
        self.assertIn(
            "packages/umbra-rti-jpype/tests/test_java_provider.py"
            "#test_java_encoder_preserves_custom_2025_logical_time_data_elements",
            next(
                item["evidence"]
                for item in result["coverage_dimensions"]
                if item["id"] == "provider-extension-wire-boundaries"
            ),
        )
        self.assertIn(
            "packages/umbra-rti-jpype/tests/test_java_provider.py"
            "#test_java_provider_delegates_variable_width_vendor_time_wire",
            next(
                item["evidence"]
                for item in result["coverage_dimensions"]
                if item["id"] == "provider-extension-wire-boundaries"
            ),
        )
        self.assertIn(
            "packages/umbra-rti-jpype/tests/test_java_provider.py"
            "#test_java_callback_does_not_guess_vendor_time_from_class_substrings",
            next(
                item["evidence"]
                for item in result["coverage_dimensions"]
                if item["id"] == "provider-extension-wire-boundaries"
            ),
        )
        self.assertIn(
            "packages/umbra-rti-jpype/tests/test_java_2010_provider.py"
            "#test_2010_data_element_class_name_does_not_guess_vendor_interface",
            next(
                item["evidence"]
                for item in result["coverage_dimensions"]
                if item["id"] == "provider-extension-wire-boundaries"
            ),
        )
        self.assertEqual(
            matrices["vendor-data-element-wire-surface"]["vector_count"], 4
        )
        self.assertEqual(
            matrices["vendor-data-element-encode-surface"]["vector_count"], 4
        )
        self.assertEqual(
            matrices["logical-time-wire-and-arithmetic"]["requirement_refs"],
            [
                "compliance/requirements-lab/logical-time-encoding-requirements-contract.json",
                "compliance/requirements-lab/float-time-requirements-contract.json",
                "compliance/requirements-lab/reference-time-requirements-contract.json",
            ],
        )
        self.assertTrue(
            all(matrix["requirement_refs"] for matrix in result["matrix_evidence"])
        )

    def test_report_rejects_unknown_coverage_status(self) -> None:
        catalog_path = (
            Path(__file__).parents[3]
            / "compliance"
            / "catalogs"
            / "python-surface-completeness-catalog.json"
        )
        catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
        catalog["coverage_dimensions"][0]["status"] = "claimed-complete"
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "catalog.json"
            path.write_text(json.dumps(catalog), encoding="utf-8")
            result = verify(path)
        self.assertEqual(result["status"], "fail")
        self.assertTrue(
            any("unknown status" in finding for finding in result["findings"])
        )

    def test_report_rejects_duplicate_edition_identity_fields(self) -> None:
        catalog_path = (
            Path(__file__).parents[3]
            / "compliance"
            / "catalogs"
            / "python-surface-completeness-catalog.json"
        )
        catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
        catalog["editions"][1]["python_namespace"] = catalog["editions"][0][
            "python_namespace"
        ]
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "catalog.json"
            path.write_text(json.dumps(catalog), encoding="utf-8")
            result = verify(path)
        self.assertEqual(result["status"], "fail")
        self.assertTrue(
            any(
                "edition identity field 'python_namespace' is duplicated" in finding
                for finding in result["findings"]
            )
        )

    def test_report_rejects_missing_coverage_evidence(self) -> None:
        catalog_path = (
            Path(__file__).parents[3]
            / "compliance"
            / "catalogs"
            / "python-surface-completeness-catalog.json"
        )
        catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
        catalog["coverage_dimensions"][0]["evidence"] = ["missing/evidence.py"]
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "catalog.json"
            path.write_text(json.dumps(catalog), encoding="utf-8")
            result = verify(path)
        self.assertEqual(result["status"], "fail")
        self.assertTrue(
            any("evidence path is missing" in finding for finding in result["findings"])
        )

    def test_report_rejects_missing_roundtrip_catalog(self) -> None:
        """Do not allow an edition to lose its detailed carrier inventory."""

        catalog_path = (
            Path(__file__).parents[3]
            / "compliance"
            / "catalogs"
            / "python-surface-completeness-catalog.json"
        )
        catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
        del catalog["editions"][0]["roundtrip_catalog"]
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "catalog.json"
            path.write_text(json.dumps(catalog), encoding="utf-8")
            result = verify(path)
        self.assertEqual(result["status"], "fail")
        self.assertTrue(
            any("needs a roundtrip_catalog path" in finding for finding in result["findings"])
        )

    def test_report_accepts_method_qualified_coverage_evidence(self) -> None:
        catalog_path = (
            Path(__file__).parents[3]
            / "compliance"
            / "catalogs"
            / "python-surface-completeness-catalog.json"
        )
        catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
        catalog["coverage_dimensions"][0]["evidence"] = [
            (
                "packages/umbra-rti-jpype/tests/test_java_2010_provider.py"
                "#test_runtime_preserves_unknown_vendor_time_carriers"
            )
        ]
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "catalog.json"
            path.write_text(json.dumps(catalog), encoding="utf-8")
            result = verify(path)
        self.assertEqual(result["status"], "pass")

    def test_report_rejects_missing_matrix_test_method(self) -> None:
        catalog_path = (
            Path(__file__).parents[3]
            / "compliance"
            / "catalogs"
            / "python-surface-completeness-catalog.json"
        )
        catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
        catalog["matrix_evidence"][0]["editions"]["2010"]["routes"][0] = (
            "packages/umbra-rti-native/tests/test_native_2010.py#test_removed_matrix"
        )
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "catalog.json"
            path.write_text(json.dumps(catalog), encoding="utf-8")
            result = verify(path)
        self.assertEqual(result["status"], "fail")
        self.assertTrue(
            any("route method is missing" in finding for finding in result["findings"])
        )

    def test_report_rejects_stale_matrix_vector_count(self) -> None:
        catalog_path = (
            Path(__file__).parents[3]
            / "compliance"
            / "catalogs"
            / "python-surface-completeness-catalog.json"
        )
        catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
        catalog["matrix_evidence"][1]["vector_count"] = 121
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "catalog.json"
            path.write_text(json.dumps(catalog), encoding="utf-8")
            result = verify(path)
        self.assertEqual(result["status"], "fail")
        self.assertTrue(any("derived 120" in finding for finding in result["findings"]))

    def test_report_rejects_missing_matrix_requirement_reference(self) -> None:
        catalog_path = (
            Path(__file__).parents[3]
            / "compliance"
            / "catalogs"
            / "python-surface-completeness-catalog.json"
        )
        catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
        catalog["matrix_evidence"][0]["requirement_refs"] = [
            "missing/requirements-contract.json"
        ]
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "catalog.json"
            path.write_text(json.dumps(catalog), encoding="utf-8")
            result = verify(path)
        self.assertEqual(result["status"], "fail")
        self.assertTrue(
            any(
                "requirement reference is missing" in finding
                for finding in result["findings"]
            )
        )


if __name__ == "__main__":
    unittest.main()
