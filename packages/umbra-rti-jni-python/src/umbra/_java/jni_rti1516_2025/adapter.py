"""Configuration-only adapter for the C++ -> JNI -> Java provider route.

The adapter deliberately contains no RTI service implementation.  It selects
the independently supplied IEEE Java API JAR and Umbra bridge JAR, loads the
bridge's standard ``RtiFactory`` through Java ``ServiceLoader``, and delegates
the public Python contract to :class:`JavaRtiFactory`.
"""

from __future__ import annotations

import os
from pathlib import Path

from hla.rti1516_2025.exceptions import RTIinternalError
from umbra._java.rti1516_2025 import JavaProviderConfiguration, JavaRtiFactory
from umbra._java.rti1516_2025._runtime import JavaRuntime


class JniRtiFactory(JavaRtiFactory):
    """Discover Umbra's C++-backed Java provider through standard Java APIs.

    The three artifacts are intentionally external to this Python package:
    the IEEE API JAR supplies ``hla.rti1516_2025``; the bridge JAR supplies
    only ``org.umbra.jni`` and its ``ServiceLoader`` descriptors; and the
    native library contains the C++ RTI.  The JVM is still started lazily by
    the inherited JPype runtime.
    """

    ENTRY_POINT_ALIAS = "umbra-jni"
    JAVA_FACTORY_NAME = "Umbra JNI C++ RTI"
    API_JAR_ENVIRONMENT_VARIABLE = "UMBRA_JNI_JAVA_API_JAR"
    BRIDGE_JAR_ENVIRONMENT_VARIABLE = "UMBRA_JNI_BRIDGE_JAR"
    NATIVE_LIBRARY_ENVIRONMENT_VARIABLE = "UMBRA_JNI_NATIVE_LIBRARY"
    ARTIFACT_DIRECTORY_ENVIRONMENT_VARIABLE = "UMBRA_JNI_BRIDGE_ARTIFACT_DIRECTORY"

    def __init__(
        self,
        api_jar: str | Path | None = None,
        bridge_jar: str | Path | None = None,
        native_library: str | Path | None = None,
        *,
        artifact_directory: str | Path | None = None,
        jvm_path: str | None = None,
        jvm_options: tuple[str, ...] = (),
        runtime: JavaRuntime | None = None,
    ) -> None:
        configured_directory = artifact_directory
        if configured_directory is None:
            configured_directory = os.getenv(self.ARTIFACT_DIRECTORY_ENVIRONMENT_VARIABLE)
        directory = Path(configured_directory) if configured_directory else None

        selected_api = self._selected_path(
            api_jar,
            self.API_JAR_ENVIRONMENT_VARIABLE,
            None,
        )
        selected_bridge = self._selected_path(
            bridge_jar,
            self.BRIDGE_JAR_ENVIRONMENT_VARIABLE,
            directory / "umbra-rti-jni.jar" if directory else None,
        )
        selected_native = self._selected_path(
            native_library,
            self.NATIVE_LIBRARY_ENVIRONMENT_VARIABLE,
            self._native_library_from_directory(directory),
        )

        self._api_jar = selected_api
        self._bridge_jar = selected_bridge
        self._native_library = selected_native
        options = tuple(jvm_options)
        native_option_prefix = "-Dumbra.rti.jni.library="
        if not any(option.startswith(native_option_prefix) for option in options):
            if selected_native is not None:
                options += (native_option_prefix + Path(selected_native).resolve().as_posix(),)

        super().__init__(
            JavaProviderConfiguration(
                classpath=tuple(
                    path for path in (selected_api, selected_bridge) if path is not None
                ),
                # Use the standard named factory overload.  A supplied API
                # JAR may legitimately carry another provider descriptor
                # (as the conformance fixture does); the JNI bridge must not
                # silently select that provider through the no-argument
                # overload.
                rti_factory_name=self.JAVA_FACTORY_NAME,
                jvm_path=jvm_path,
                jvm_options=options,
            ),
            runtime=runtime,
        )

    @staticmethod
    def _selected_path(
        explicit: str | Path | None,
        environment_variable: str,
        directory_default: Path | None,
    ) -> str | None:
        if explicit is not None:
            return str(explicit)
        environment_value = os.getenv(environment_variable)
        if environment_value:
            return environment_value
        return str(directory_default) if directory_default is not None else None

    @classmethod
    def _native_library_from_directory(cls, directory: Path | None) -> Path | None:
        if directory is None:
            return None
        for name in ("umbra_rti_jni.dll", "libumbra_rti_jni.so", "libumbra_rti_jni.dylib"):
            candidate = directory / name
            if candidate.is_file():
                return candidate
        # Preserve a useful path in the eventual missing-artifact diagnostic.
        return directory / "umbra_rti_jni.dll"

    def _java_factory(self) -> object:
        missing = [
            path
            for path in (self._api_jar, self._bridge_jar, self._native_library)
            if path is None or not Path(path).is_file()
        ]
        if missing:
            rendered = ", ".join(str(path) for path in missing)
            raise RTIinternalError(
                "Umbra JNI provider requires an IEEE API JAR, bridge JAR, and native library; "
                f"missing: {rendered}"
            )
        return super()._java_factory()
