"""Public entry points for Umbra's native 2025 Python provider.

Concrete pybind11 adaptation code lives in :mod:`.provider`; this module only
keeps the established import path and the small compatibility surface used by
source-checkout tests.
"""

from __future__ import annotations

from . import _native
from .provider import UmbraRtiFactory
from .provider import _UmbraRTIambassador as _ProviderRTIambassador


class _UmbraRTIambassador(_ProviderRTIambassador):
    """Compatibility name for the concrete native ambassador façade.

    Java surface: ``hla.rti1516_2025.RTIambassador``.
    C++ surface: ``RTI::RTIambassador``.
    """


__all__ = ["UmbraRtiFactory"]
