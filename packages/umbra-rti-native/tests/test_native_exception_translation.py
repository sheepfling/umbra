"""Internal Python edge checks, not IEEE RTI behavioral/conformance evidence."""

import unittest
from collections.abc import Callable
from functools import partial
from unittest.mock import patch

from hla.rti1516_2025.encoding import EncoderException
from hla.rti1516_2025.exceptions import (
    FederationExecutionDoesNotExist,
    RTIexception,
)
from umbra._native.rti1516_2025 import provider


class NativeExceptionTranslationTest(unittest.TestCase):
    def test_2025_routes_use_two_argument_api_and_preserve_native_cause(self) -> None:
        cases: tuple[tuple[str, type[RTIexception], str], ...] = (
            (
                "FederationExecutionDoesNotExist: provider text: a:b α",
                FederationExecutionDoesNotExist,
                "provider text: a:b α",
            ),
            ("VendorFailure: unknown provider error", RTIexception, "unknown provider error"),
            (
                "FederationExecutionDoesNotExist",
                FederationExecutionDoesNotExist,
                "FederationExecutionDoesNotExist",
            ),
        )
        for route in ("helper", "ambassador", "factory"):
            for text, expected_type, message in cases:
                with self.subTest(route=route, text=text):
                    error = provider._native.NativeRtiError(text)

                    def reject(exception: BaseException = error) -> object:
                        raise exception

                    operation: Callable[[], object]
                    if route == "helper":
                        operation = partial(provider._call_native, reject)
                    elif route == "ambassador":
                        operation = partial(provider._UmbraRTIambassador._call, reject)
                    else:
                        operation = provider.UmbraRtiFactory().getRtiAmbassador
                    with (
                        patch.object(provider._native, "NativeAmbassador", side_effect=error),
                        self.assertRaises(expected_type) as raised,
                    ):
                        operation()
                    self.assertIs(type(raised.exception), expected_type)
                    self.assertEqual(str(raised.exception), message)
                    self.assertIs(raised.exception.__cause__, error)

    def test_encoder_specific_translation_retains_original_native_cause(self) -> None:
        error = provider._native.NativeRtiError("EncoderException: invalid encoded value")

        def reject() -> object:
            raise error

        with self.assertRaisesRegex(EncoderException, "^invalid encoded value$") as raised:
            provider._call_native(reject, encoding_error=EncoderException)
        self.assertIs(raised.exception.__cause__, error)

    def test_non_native_errors_are_not_relabelled_as_rti_errors(self) -> None:
        error = ValueError("Python boundary validation failed")

        def reject() -> object:
            raise error

        with self.assertRaises(ValueError) as raised:
            provider._call_native(reject)
        self.assertIs(raised.exception, error)

    def test_successful_native_results_are_returned_unchanged(self) -> None:
        result = object()
        self.assertIs(provider._call_native(lambda: result), result)
