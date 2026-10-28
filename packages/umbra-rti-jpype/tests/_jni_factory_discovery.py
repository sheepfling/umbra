"""Focused factory discovery and lifecycle checks for the JPype JNI bridge."""

from __future__ import annotations

try:
    import jpype
except ModuleNotFoundError:
    jpype = None


class JniFactoryDiscoveryMixin:
    """Factory discovery tests that share the main JNI integration fixture."""

    def test_jni_factory_is_discovered_by_the_exact_2025_java_serviceloader(
        self,
    ) -> None:
        """Prove Python reaches the bridge through Java's standard factory API."""
        java_factory = self.factory.unwrap_java_factory()
        standard_factory = jpype.JClass("hla.rti1516_2025.RtiFactory").class_
        self.assertTrue(standard_factory.isAssignableFrom(java_factory.getClass()))
        self.assertEqual(str(java_factory.rtiName()), self.factory.JAVA_FACTORY_NAME)
        self.assertTrue(str(java_factory.rtiVersion()))

        # The object returned by the Java ServiceLoader must itself create the
        # standard RTIambassador interface that the Python provider wraps. A
        # bespoke Python/C++ shortcut would bypass this exact Java surface.
        java_ambassador = java_factory.getRtiAmbassador()
        standard_ambassador = jpype.JClass("hla.rti1516_2025.RTIambassador").class_
        self.assertTrue(
            standard_ambassador.isAssignableFrom(java_ambassador.getClass())
        )
        java_ambassador.close()

    def test_jni_factory_is_discovered_by_standard_java_no_arg_serviceloader(
        self,
    ) -> None:
        """The bridge is usable through the normal no-argument factory API."""
        # The configured Python adapter uses the named standard overload to
        # avoid ambiguity when an application class path contains several
        # providers. With only the independent IEEE API and Umbra bridge on
        # this class path, the ordinary Java no-argument ServiceLoader route
        # must discover the same C++ provider too.
        self.factory.getRtiAmbassador()
        factory_factory = jpype.JClass("hla.rti1516_2025.RtiFactoryFactory")
        java_factory = factory_factory.getRtiFactory()
        self.assertEqual(str(java_factory.rtiName()), self.factory.JAVA_FACTORY_NAME)
        standard_factory = jpype.JClass("hla.rti1516_2025.RtiFactory").class_
        self.assertTrue(standard_factory.isAssignableFrom(java_factory.getClass()))
        java_ambassador = java_factory.getRtiAmbassador()
        try:
            standard_ambassador = jpype.JClass("hla.rti1516_2025.RTIambassador").class_
            self.assertTrue(
                standard_ambassador.isAssignableFrom(java_ambassador.getClass())
            )
        finally:
            java_ambassador.close()

    def test_cpp_jni_java_jpype_standard_java_close_releases_native_state(
        self,
    ) -> None:
        """The Java AutoCloseable extension must own the C++ ambassador lifetime."""
        ambassador = self.factory.getRtiAmbassador()
        raw_ambassador = ambassador.unwrap_java_object()
        try:
            # Invoke close on the exact Java proxy rather than on the Python
            # adapter's private implementation handle.
            raw_ambassador.close()
            with self.assertRaises(jpype.JClass("java.lang.IllegalStateException")):
                raw_ambassador.listFederationExecutions()
            # Native destruction is idempotent, so Java try-with-resources and
            # the Python adapter's cleanup path may safely overlap.
            raw_ambassador.close()
        finally:
            ambassador._implementation.close()

    def test_jni_factory_probe_reads_standard_identity_without_ambassador(
        self,
    ) -> None:
        probe = self.factory.probe()

        self.assertIs(probe.factory, self.factory)
        self.assertEqual(probe.rti_name, self.factory.JAVA_FACTORY_NAME)
        self.assertTrue(probe.rti_version)
        self.assertEqual(
            probe.configuration.classpath,
            (str(self.artifacts.api_jar), str(self.artifacts.bridge_jar)),
        )
