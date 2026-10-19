"""Synthetic control driver for a repeatable, RTI-free management walkthrough."""

from .command_models import JoinParameters, ResignParameters


class ShowcaseSessionDriver:
    def connect(self) -> None:
        return None

    def join(self, parameters: JoinParameters) -> str:
        return f"showcase:{parameters.federate_name}"

    def resign(self, parameters: ResignParameters) -> None:
        return None

    def disconnect(self) -> None:
        return None

    def poll(self) -> None:
        return None
