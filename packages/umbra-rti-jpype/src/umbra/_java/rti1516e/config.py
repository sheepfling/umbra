"""Process-level configuration for the IEEE 1516.1-2010 JPype route."""

from __future__ import annotations

from dataclasses import dataclass
import os
from typing import Mapping


@dataclass(frozen=True, slots=True)
class Java2010ProviderConfiguration:
    classpath: tuple[str, ...] = ()
    rti_factory_name: str | None = None
    jvm_path: str | None = None
    jvm_options: tuple[str, ...] = ()
    convert_strings: bool = False

    @classmethod
    def from_environment(
        cls,
        environment: Mapping[str, str] | None = None,
    ) -> "Java2010ProviderConfiguration":
        values = os.environ if environment is None else environment
        classpath = tuple(
            path
            for path in values.get("UMBRA_JAVA_1516E_CLASSPATH", "").split(os.pathsep)
            if path
        )
        return cls(
            classpath=classpath,
            rti_factory_name=values.get("UMBRA_JAVA_1516E_FACTORY_NAME") or None,
            jvm_path=values.get("UMBRA_JAVA_1516E_JVM_PATH") or None,
        )
