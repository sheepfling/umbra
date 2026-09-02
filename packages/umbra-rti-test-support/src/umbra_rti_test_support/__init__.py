"""Shared, development-only provider conformance tests."""

from __future__ import annotations

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
    CallbackDeliveryObservation,
    CallbackProvenanceVector,
    DEFAULT_ADVANCE_SERVICES,
    DEFAULT_CALLBACK_MODELS,
    DEFAULT_MEMBER_COUNTS,
    DEFAULT_SAVE_KINDS,
    DEFAULT_TIME_IMPLEMENTATIONS,
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
    "ConnectionFoundationConformanceMixin",
    "ConnectionOverloadConformanceMixin",
    "CALLBACK_OVERLOAD_COUNTS",
    "CallbackDeliveryObservation",
    "CallbackProvenanceVector",
    "DataElementEdition",
    "DataElementValueVector",
    "ExtendableVariantWireVector",
    "VendorDataElementEncodeVector",
    "VendorDataElementWireVector",
    "VendorTimeArithmeticVector",
    "FederationExecutionDiscoveryConformanceMixin",
    "FederationExecutionMemberDiscoveryConformanceMixin",
    "LogicalTimeArithmeticVector",
    "LogicalTimeWireVector",
    "ProviderBindingParityConformanceMixin",
    "SaveRestoreMatrixCase",
    "SurfaceEventTrace",
    "SurfaceMatrixCase",
    "assert_callback_delivery_parity",
    "assert_event_order",
    "iter_data_element_value_matrix",
    "iter_callback_provenance_matrix",
    "iter_extendable_variant_wire_matrix",
    "iter_vendor_data_element_encode_matrix",
    "iter_vendor_data_element_wire_matrix",
    "iter_vendor_time_arithmetic_matrix",
    "iter_logical_time_arithmetic_matrix",
    "iter_logical_time_wire_matrix",
    "iter_save_restore_matrix",
    "iter_surface_matrix",
    "normalize_callback_delivery",
]
