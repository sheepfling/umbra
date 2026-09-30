"""Command-line launcher for the optional observer dashboard."""

from __future__ import annotations

import argparse

from .core import ObserverStore
from .server import create_app


def _seed_demo(store: ObserverStore) -> None:
    store.set_context(
        connection="demo://local",
        federation="ExampleFederation",
        source="demo-source",
    )
    store.set_status("running")
    store.publish(
        {
            "event_type": "federate.joined",
            "source": "demo-source",
            "federate": "Publisher",
            "role": "publisher",
        }
    )
    store.publish(
        {
            "event_type": "federate.joined",
            "source": "demo-source",
            "federate": "Subscriber",
            "role": "subscriber",
        }
    )
    store.publish(
        {
            "event_type": "object.discovered",
            "source": "demo-source",
            "federate": "Subscriber",
            "object_key": "object-1",
            "object_name": "ExampleObject",
            "class_name": "ExampleClass",
            "attributes": {"state": "ready"},
        }
    )
    store.publish(
        {
            "event_type": "interaction.received",
            "source": "demo-source",
            "federate": "Subscriber",
            "interaction_key": "interaction-1",
            "interaction_class": "ExampleInteraction",
            "parameters": {"value": 42},
        }
    )
    store.publish(
        {
            "event_type": "callback.invoked",
            "source": "demo-source",
            "operation": "exampleCallback",
            "details": {"delivery": "receive-order"},
        }
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Run the Umbra provider-neutral runtime observer dashboard.")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--max-events", type=int, default=2_000)
    parser.add_argument("--demo", action="store_true", help="seed generic events before serving")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    store = ObserverStore(max_events=args.max_events)
    if args.demo:
        _seed_demo(store)
    app = create_app(store)
    try:
        import uvicorn
    except ImportError as error:  # pragma: no cover - depends on optional extra
        raise SystemExit(
            "The dashboard launcher requires optional dependencies; install "
            "umbra-rti-observer[web]."
        ) from error
    uvicorn.run(app, host=args.host, port=args.port)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
