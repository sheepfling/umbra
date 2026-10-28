"""Shared, development-only provider conformance tests."""

from __future__ import annotations

from .cleanup import best_effort_cleanup
from .conformance import (
    ConnectionFoundationConformanceMixin,
    ConnectionOverloadConformanceMixin,
    FederationExecutionDiscoveryConformanceMixin,
    FederationExecutionMemberDiscoveryConformanceMixin,
    ProviderBindingParityConformanceMixin,
)
from .data_element_matrix import (
    DataElementEdition,
    DataElementValueVector,
    iter_data_element_value_matrix,
)
from .hla_names import HLA_FIXTURES, HLA_FOM, HLA_MOM, HLA_TYPES
from .provider_extension_matrix import (
    ExtendableVariantWireVector,
    VendorDataElementEncodeVector,
    VendorDataElementWireVector,
    VendorTimeArithmeticVector,
    iter_extendable_variant_wire_matrix,
    iter_vendor_data_element_encode_matrix,
    iter_vendor_data_element_wire_matrix,
    iter_vendor_time_arithmetic_matrix,
)
from .save_restore_matrix import (
    DEFAULT_RESTORE_OUTCOMES,
    DEFAULT_SAVE_OUTCOMES,
    SaveRestoreMatrixCase,
    iter_save_restore_matrix,
)
from .surface_matrix import (
    CALLBACK_OVERLOAD_COUNTS,
    DEFAULT_ADVANCE_SERVICES,
    DEFAULT_CALLBACK_MODELS,
    DEFAULT_MEMBER_COUNTS,
    DEFAULT_SAVE_KINDS,
    DEFAULT_TIME_IMPLEMENTATIONS,
    CallbackDeliveryObservation,
    CallbackProvenanceVector,
    SurfaceEventTrace,
    SurfaceMatrixCase,
    assert_callback_delivery_parity,
    assert_event_order,
    iter_callback_provenance_matrix,
    iter_surface_matrix,
    normalize_callback_delivery,
)
from .wire_matrix import (
    LogicalTimeArithmeticVector,
    LogicalTimeWireVector,
    iter_logical_time_arithmetic_matrix,
    iter_logical_time_wire_matrix,
)

__all__ = [
    "CALLBACK_OVERLOAD_COUNTS",
    "DEFAULT_ADVANCE_SERVICES",
    "DEFAULT_CALLBACK_MODELS",
    "DEFAULT_MEMBER_COUNTS",
    "DEFAULT_RESTORE_OUTCOMES",
    "DEFAULT_SAVE_KINDS",
    "DEFAULT_SAVE_OUTCOMES",
    "DEFAULT_TIME_IMPLEMENTATIONS",
    "HLA_FIXTURES",
    "HLA_FOM",
    "HLA_MOM",
    "HLA_TYPES",
    "CallbackDeliveryObservation",
    "CallbackProvenanceVector",
    "ConnectionFoundationConformanceMixin",
    "ConnectionOverloadConformanceMixin",
    "DataElementEdition",
    "DataElementValueVector",
    "ExtendableVariantWireVector",
    "FederationExecutionDiscoveryConformanceMixin",
    "FederationExecutionMemberDiscoveryConformanceMixin",
    "LogicalTimeArithmeticVector",
    "LogicalTimeWireVector",
    "ProviderBindingParityConformanceMixin",
    "SaveRestoreMatrixCase",
    "SurfaceEventTrace",
    "SurfaceMatrixCase",
    "VendorDataElementEncodeVector",
    "VendorDataElementWireVector",
    "VendorTimeArithmeticVector",
    "assert_callback_delivery_parity",
    "assert_event_order",
    "best_effort_cleanup",
    "iter_callback_provenance_matrix",
    "iter_data_element_value_matrix",
    "iter_extendable_variant_wire_matrix",
    "iter_logical_time_arithmetic_matrix",
    "iter_logical_time_wire_matrix",
    "iter_save_restore_matrix",
    "iter_surface_matrix",
    "iter_vendor_data_element_encode_matrix",
    "iter_vendor_data_element_wire_matrix",
    "iter_vendor_time_arithmetic_matrix",
    "normalize_callback_delivery",
]
