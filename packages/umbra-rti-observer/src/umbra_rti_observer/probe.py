"""Dependency-free compatibility probe for observer HTTP backends.

The probe deliberately speaks only the observer wire contract. It can check a
Python, Java, or vendor-specific backend without importing that backend's RTI
bindings or sharing implementation details with it.
"""

from __future__ import annotations

import argparse
import json
from collections.abc import Callable, Mapping
from dataclasses import dataclass
from typing import cast
from urllib.parse import urlencode
from urllib.request import Request, urlopen

from .capabilities import CAPABILITY_STATES
from .contract import OBSERVER_CONTRACT_VERSION
from .models import JsonObject, JsonValue

PayloadFetcher = Callable[[str], tuple[int, JsonValue]]


@dataclass(frozen=True)
class ProbeCheck:
    """One observable compatibility assertion."""

    name: str
    passed: bool
    detail: str

    def as_dict(self) -> JsonObject:
        return {"name": self.name, "passed": self.passed, "detail": self.detail}


@dataclass(frozen=True)
class ProbeResult:
    """Serializable result returned by :class:`ObserverContractProbe`."""

    base_url: str
    checks: tuple[ProbeCheck, ...]

    @property
    def passed(self) -> bool:
        return all(check.passed for check in self.checks)

    def as_dict(self) -> JsonObject:
        return {
            "base_url": self.base_url,
            "passed": self.passed,
            "checks": [check.as_dict() for check in self.checks],
        }


class ObserverContractProbe:
    """Validate the minimum HTTP surface required by the dashboard."""

    def __init__(
        self,
        base_url: str,
        *,
        timeout: float = 5.0,
        fetch: PayloadFetcher | None = None,
    ) -> None:
        self.base_url = base_url.rstrip("/")
        self.timeout = max(float(timeout), 0.1)
        self._fetcher = fetch or self._fetch_http

    def run(self) -> ProbeResult:
        checks: list[ProbeCheck] = []
        contract = self._get_json("contract", "/api/contract", checks)
        if contract is None:
            return ProbeResult(self.base_url, tuple(checks))

        self._check_contract(contract, checks)
        expected_version = contract.get("contract_version")
        for name, path in (
            ("health", "/api/health"),
            ("state", "/api/state?include_events=false"),
            ("metrics", "/api/metrics"),
            ("events", "/api/events?" + urlencode({"after": 0, "limit": 20})),
        ):
            payload = self._get_json(name, path, checks)
            if payload is None:
                continue
            self._check_version(name, payload, expected_version, checks)
            if name == "health":
                self._check_fields(name, payload, ("status", "event_count", "capabilities"), checks)
            elif name == "state":
                self._check_fields(
                    name,
                    payload,
                    (
                        "contract_version",
                        "schema_version",
                        "status",
                        "context",
                        "live_metrics",
                        "retention",
                        "adapters",
                        "capabilities",
                        "federate_roster",
                        "inspectors",
                    ),
                    checks,
                )
                self._check_capabilities(name, payload.get("capabilities"), checks)
            elif name == "metrics":
                self._check_fields(name, payload, ("schema_version", "status", "live_metrics", "retention", "adapters"), checks)
            elif name == "events":
                self._check_event_page(payload, checks)

        return ProbeResult(self.base_url, tuple(checks))

    def _fetch_http(self, path: str) -> tuple[int, JsonValue]:
        request = Request(self.base_url + path, headers={"Accept": "application/json"})
        with urlopen(request, timeout=self.timeout) as response:
            body = response.read().decode("utf-8")
            return int(response.status), cast(JsonValue, json.loads(body))

    def _get_json(self, name: str, path: str, checks: list[ProbeCheck]) -> Mapping[str, JsonValue] | None:
        try:
            status, payload = self._fetcher(path)
        except Exception as error:  # noqa: BLE001 - transport failures become failed checks
            checks.append(ProbeCheck(name, False, f"request failed: {error}"))
            return None
        if status != 200:
            checks.append(ProbeCheck(name, False, f"expected HTTP 200, received {status}"))
            return None
        if not isinstance(payload, Mapping):
            checks.append(ProbeCheck(name, False, "response body must be a JSON object"))
            return None
        checks.append(ProbeCheck(name, True, "HTTP 200 JSON object"))
        return payload

    @staticmethod
    def _check_contract(contract: Mapping[str, JsonValue], checks: list[ProbeCheck]) -> None:
        version = contract.get("contract_version")
        checks.append(
            ProbeCheck(
                "contract_version",
                version == OBSERVER_CONTRACT_VERSION,
                f"advertised {version!r}; expected {OBSERVER_CONTRACT_VERSION!r}",
            )
        )
        schema = contract.get("capability_schema")
        states = schema.get("states") if isinstance(schema, Mapping) else None
        checks.append(
            ProbeCheck(
                "capability_states",
                isinstance(states, list) and set(CAPABILITY_STATES).issubset(states),
                f"advertised states: {states!r}",
            )
        )
        api_paths = contract.get("api_paths")
        checks.append(
            ProbeCheck(
                "api_paths",
                isinstance(api_paths, Mapping)
                and api_paths.get("contract") == "/api/contract"
                and api_paths.get("state") == "/api/state"
                and api_paths.get("events") == "/api/events",
                "contract, state, and events paths are present",
            )
        )

    @staticmethod
    def _check_version(name: str, payload: Mapping[str, JsonValue], expected: JsonValue, checks: list[ProbeCheck]) -> None:
        if expected is None:
            return
        actual = payload.get("contract_version")
        checks.append(ProbeCheck(f"{name}_version", actual == expected, f"advertised {actual!r}"))

    @staticmethod
    def _check_fields(name: str, payload: Mapping[str, JsonValue], fields: tuple[str, ...], checks: list[ProbeCheck]) -> None:
        missing = [field for field in fields if field not in payload]
        checks.append(ProbeCheck(f"{name}_fields", not missing, "missing: " + ", ".join(missing) if missing else "all required fields present"))

    @staticmethod
    def _check_capabilities(name: str, capabilities: JsonValue, checks: list[ProbeCheck]) -> None:
        valid = isinstance(capabilities, Mapping) and all(
            isinstance(value, Mapping) and value.get("state") in CAPABILITY_STATES
            for value in capabilities.values()
        )
        checks.append(ProbeCheck(f"{name}_capabilities", valid, "capability entries use declared states"))

    @staticmethod
    def _check_event_page(payload: Mapping[str, JsonValue], checks: list[ProbeCheck]) -> None:
        fields = (
            "contract_version",
            "events",
            "next_after",
            "has_more",
            "gap",
            "first_retained_sequence",
            "last_sequence",
            "retained_events",
            "dropped_events",
        )
        missing = [field for field in fields if field not in payload]
        checks.append(ProbeCheck("events_fields", not missing, "missing: " + ", ".join(missing) if missing else "all required fields present"))
        events = payload.get("events")
        envelope_fields = ("sequence", "event_type", "family", "source", "observed_at")
        valid_envelopes = isinstance(events, list) and all(
            isinstance(event, Mapping)
            and all(field in event for field in envelope_fields)
            and isinstance(event.get("sequence"), int)
            for event in events
        )
        event_rows = [event for event in events if isinstance(event, Mapping)] if isinstance(events, list) else []
        sequences = cast(list[int], [event["sequence"] for event in event_rows]) if valid_envelopes else []
        ordered = valid_envelopes and sequences == sorted(sequences)
        checks.append(ProbeCheck("event_envelopes", ordered, f"{len(sequences)} ordered event envelope(s)"))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Check an observer backend against the language-neutral HTTP contract.")
    parser.add_argument("base_url", help="observer backend URL, for example http://127.0.0.1:8765")
    parser.add_argument("--timeout", type=float, default=5.0)
    args = parser.parse_args(argv)
    result = ObserverContractProbe(args.base_url, timeout=args.timeout).run()
    print(json.dumps(result.as_dict(), indent=2, sort_keys=True))
    return 0 if result.passed else 1


if __name__ == "__main__":
    raise SystemExit(main())


__all__ = ["ObserverContractProbe", "ProbeCheck", "ProbeResult", "main"]
