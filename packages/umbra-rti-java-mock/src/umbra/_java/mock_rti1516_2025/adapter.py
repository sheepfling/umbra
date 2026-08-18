"""Reference implementation of a named Java RTI provider adapter."""

from __future__ import annotations

import os
from pathlib import Path

from hla.rti1516_2025.exceptions import RTIinternalError
from umbra._java.rti1516_2025 import JavaProviderConfiguration, JavaRtiFactory
from umbra._java.rti1516_2025._runtime import JavaRuntime


class MockJavaRtiFactory(JavaRtiFactory):
    """Registered adapter for the repository's mock Java 2025 RTI.

    This is the intended shape of a real vendor adapter: the entry-point alias
    identifies the Python transport, while ``JAVA_FACTORY_NAME`` identifies the
    actual factory exposed by Java ``ServiceLoader``.
    """

    ENTRY_POINT_ALIAS = "umbra-mock-java"
    JAVA_FACTORY_NAME = "Umbra Mock Java RTI"
    JAR_ENVIRONMENT_VARIABLE = "UMBRA_MOCK_JAVA_RTI_JAR"

    def __init__(
        self,
        jar_path: str | Path | None = None,
        *,
        jvm_path: str | None = None,
        jvm_options: tuple[str, ...] = (),
        runtime: JavaRuntime | None = None,
    ) -> None:
        selected_jar = str(jar_path) if jar_path is not None else os.getenv(self.JAR_ENVIRONMENT_VARIABLE)
        self._jar_path = selected_jar
        super().__init__(
            JavaProviderConfiguration(
                classpath=(selected_jar,) if selected_jar is not None else (),
                rti_factory_name=self.JAVA_FACTORY_NAME,
                jvm_path=jvm_path,
                jvm_options=jvm_options,
            ),
            runtime=runtime,
        )

    def _java_factory(self) -> object:
        if self._jar_path is None:
            raise RTIinternalError(
                f"Set {self.JAR_ENVIRONMENT_VARIABLE} or pass jar_path to {type(self).__name__}"
            )
        if not Path(self._jar_path).is_file():
            raise RTIinternalError(f"Java RTI JAR does not exist: {self._jar_path}")
        return super()._java_factory()
