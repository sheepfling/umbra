"""Configuration that must be known before the JPype JVM starts."""

from __future__ import annotations

from dataclasses import dataclass
import os
from typing import Mapping


@dataclass(frozen=True, slots=True)
class JavaProviderConfiguration:
    """Select and configure one Java 1516.1-2025 RTI provider.

    JPype permits only one JVM in a Python process. Consequently, classpath,
    JVM path, and JVM options belong to the process-level configuration rather
    than to an individual ambassador.
    """

    classpath: tuple[str, ...] = ()
    rti_factory_name: str | None = None
    jvm_path: str | None = None
    jvm_options: tuple[str, ...] = ()
    convert_strings: bool = False

    @classmethod
    def from_environment(
        cls,
        environment: Mapping[str, str] | None = None,
    ) -> JavaProviderConfiguration:
        """Build configuration from deliberately Java-specific environment keys.

        `HLA_RTI_FACTORY_NAME` remains owned by the standard Java factory. The
        `UMBRA_JAVA_*` variables configure only this Python-to-Java adapter.
        """

        values = os.environ if environment is None else environment
        classpath = tuple(
            path
            for path in values.get("UMBRA_JAVA_RTI_CLASSPATH", "").split(os.pathsep)
            if path
        )
        return cls(
            classpath=classpath,
            rti_factory_name=values.get("UMBRA_JAVA_RTI_FACTORY_NAME") or None,
            jvm_path=values.get("UMBRA_JAVA_RTI_JVM_PATH") or None,
        )
