"""Command-line launcher for the optional observer dashboard."""

from __future__ import annotations

import argparse

from .core import ObserverStore
from .server import create_app
from .showcase import ShowcaseAdapter


def _seed_demo(store: ObserverStore) -> None:
    store.set_capabilities(
        {
            "lifecycle": "available",
            "federates": "available",
            "objects": "available",
            "attributes": "available",
            "interactions": "available",
            "logical_time": {
                "state": "unsupported",
                "reason": "The provider-neutral demo does not simulate logical time.",
            },
            "mom": {
                "state": "unsupported",
                "reason": "The provider-neutral demo has no management object model source.",
            },
        },
        owner="demo",
        source="demo-source",
    )
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
    parser.add_argument("--no-analytics", action="store_true", help="disable optional windowed analytics")
    parser.add_argument("--enable-controls", action="store_true", help="enable synthetic session controls (requires --showcase)")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--demo", action="store_true", help="seed generic events before serving")
    mode.add_argument("--showcase", action="store_true", help="run the live observer showcase")
    parser.add_argument(
        "--profile",
        choices=("umbra", "limited"),
        default="umbra",
        help="showcase capability profile (default: umbra)",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=1.5,
        help="seconds between live showcase updates (default: 1.5)",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.enable_controls and not args.showcase:
        raise SystemExit("--enable-controls requires --showcase; real sessions must be explicitly registered by the application")
    store = ObserverStore(max_events=args.max_events)
    if args.demo:
        _seed_demo(store)
    if args.showcase:
        store.attach_adapter(ShowcaseAdapter(profile=args.profile, interval=args.interval))
    commands = None
    try:
        if args.enable_controls:
            from .commands import CommandDispatcher
            from .session_commands import SessionController
            from .showcase_commands import ShowcaseSessionDriver

            commands = CommandDispatcher(sink=store.publish)
            commands.register(SessionController(
                target_id="showcase-session", label="Showcase control session", adapter="showcase",
                driver_factory=ShowcaseSessionDriver, federations=("ShowcaseFederation",),
                sink=store.publish, synthetic=True,
            ))
        app = create_app(store, enable_analytics=not args.no_analytics, commands=commands)
        try:
            import uvicorn
        except ImportError as error:  # pragma: no cover - depends on optional extra
            raise SystemExit(
                "The dashboard launcher requires optional dependencies; install "
                "umbra-rti-observer[web]."
            ) from error
        uvicorn.run(app, host=args.host, port=args.port)
    finally:
        if args.enable_controls and commands is not None:
            commands.close()
        store.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
