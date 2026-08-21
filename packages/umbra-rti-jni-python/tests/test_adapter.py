from pathlib import Path
from importlib.metadata import entry_points
import os
import tempfile
import unittest

from hla.rti1516_2025 import RtiFactoryFactory
from hla.rti1516_2025.exceptions import RTIinternalError
from umbra._java.jni_rti1516_2025 import JniRtiFactory


class _FakeFactory:
    def rtiName(self):
        return "Umbra JNI C++ RTI"

    def rtiVersion(self):
        return "test"


class _FakeRuntime:
    def __init__(self):
        self.configuration = None

    def get_rti_factory(self, configuration):
        self.configuration = configuration
        return _FakeFactory()


class JniRtiFactoryTest(unittest.TestCase):
    def test_package_declares_the_standard_factory_entry_point(self) -> None:
        metadata = (Path(__file__).parents[1] / "pyproject.toml").read_text(
            encoding="utf-8"
        )
        self.assertIn(
            '[project.entry-points."hla.rti1516_2025.factories"]', metadata
        )
        self.assertRegex(
            metadata,
            r'(?m)^umbra-jni\s*=\s*'
            r'"umbra\.\_java\.jni_rti1516_2025:JniRtiFactory"\s*$',
        )

    def test_factory_is_discoverable_from_the_standard_api_namespace(self) -> None:
        """Keep the JNI transport selectable like every other Python RTI."""
        if not any(
            point.name == JniRtiFactory.ENTRY_POINT_ALIAS
            for point in entry_points(group="hla.rti1516_2025.factories")
        ):
            self.skipTest(
                "JNI entry-point metadata is available only after installing the companion package"
            )
        # The source checkout's API package performs the same alias lookup
        # that an installed application will use.  Artifact validation is
        # deliberately deferred until the returned factory is asked to start
        # Java, so discovery itself remains side-effect free.
        discovered = RtiFactoryFactory.getRtiFactory(JniRtiFactory.ENTRY_POINT_ALIAS)
        self.assertIsInstance(discovered, JniRtiFactory)

    def test_configuration_only_adapter_delegates_to_java_factory(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            api = root / "ieee-api.jar"
            bridge = root / "umbra-rti-jni.jar"
            native = root / "umbra_rti_jni.dll"
            for path in (api, bridge, native):
                path.touch()

            runtime = _FakeRuntime()
            factory = JniRtiFactory(api, bridge, native, runtime=runtime)

            self.assertEqual(factory.rtiName(), "Umbra JNI C++ RTI")
            self.assertEqual(runtime.configuration.classpath, (str(api), str(bridge)))
            self.assertEqual(
                runtime.configuration.rti_factory_name,
                JniRtiFactory.JAVA_FACTORY_NAME,
            )
            self.assertTrue(
                any(
                    option.startswith("-Dumbra.rti.jni.library=")
                    for option in runtime.configuration.jvm_options
                )
            )

    def test_artifact_directory_is_a_lazy_configuration_source(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "umbra-rti-jni.jar").touch()
            (root / "umbra_rti_jni.dll").touch()
            api = root / "ieee-api.jar"
            api.touch()
            old = {
                name: os.environ.get(name)
                for name in (
                    JniRtiFactory.ARTIFACT_DIRECTORY_ENVIRONMENT_VARIABLE,
                    JniRtiFactory.API_JAR_ENVIRONMENT_VARIABLE,
                )
            }
            try:
                os.environ[JniRtiFactory.ARTIFACT_DIRECTORY_ENVIRONMENT_VARIABLE] = str(root)
                os.environ[JniRtiFactory.API_JAR_ENVIRONMENT_VARIABLE] = str(api)
                runtime = _FakeRuntime()
                factory = JniRtiFactory(runtime=runtime)
                self.assertEqual(factory.rtiVersion(), "test")
                self.assertEqual(
                    runtime.configuration.classpath,
                    (str(api), str(root / "umbra-rti-jni.jar")),
                )
            finally:
                for name, value in old.items():
                    if value is None:
                        os.environ.pop(name, None)
                    else:
                        os.environ[name] = value

    def test_artifact_directory_can_be_supplied_without_environment_state(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "umbra-rti-jni.jar").touch()
            (root / "umbra_rti_jni.dll").touch()
            api = root / "ieee-api.jar"
            api.touch()

            runtime = _FakeRuntime()
            factory = JniRtiFactory(
                api_jar=api,
                artifact_directory=root,
                runtime=runtime,
            )

            self.assertEqual(factory.rtiVersion(), "test")
            self.assertEqual(
                runtime.configuration.classpath,
                (str(api), str(root / "umbra-rti-jni.jar")),
            )
            self.assertTrue(
                any(
                    option.startswith("-Dumbra.rti.jni.library=")
                    for option in runtime.configuration.jvm_options
                )
            )

    def test_artifact_directory_discovers_one_adjacent_api_jar(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "umbra-rti-jni.jar").touch()
            (root / "umbra_rti_jni.dll").touch()
            api = root / "hla-4-api-2.1.0.jar"
            api.touch()

            runtime = _FakeRuntime()
            factory = JniRtiFactory(artifact_directory=root, runtime=runtime)

            self.assertEqual(factory.rtiVersion(), "test")
            self.assertEqual(
                runtime.configuration.classpath,
                (str(api), str(root / "umbra-rti-jni.jar")),
            )

    def test_artifact_directory_does_not_guess_between_sibling_api_jars(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "umbra-rti-jni.jar").touch()
            (root / "umbra_rti_jni.dll").touch()
            (root / "hla-4-api.jar").touch()
            (root / "hla-4-api-extra.jar").touch()

            runtime = _FakeRuntime()
            factory = JniRtiFactory(artifact_directory=root, runtime=runtime)

            with self.assertRaises(RTIinternalError):
                factory.rtiVersion()
            self.assertIsNone(runtime.configuration)

    def test_missing_artifacts_fail_before_starting_jvm(self):
        runtime = _FakeRuntime()
        factory = JniRtiFactory(runtime=runtime)
        with self.assertRaises(RTIinternalError):
            factory.rtiName()
        self.assertIsNone(runtime.configuration)


if __name__ == "__main__":
    unittest.main()
