# Private ambassador service implementations exist only in the embedded
# federation-management profile. Keep the default shell library's source set
# aligned with the methods enabled in its internal adapter declaration.
if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  target_sources(umbra_rti PRIVATE
    cpp/src/internal/runtime/umbra_rti_ambassador_service_reporting_switches.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_object_class_lookup.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_interaction_class_lookup.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_attribute_lookup.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_update_rate_lookup.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_object_class_publication.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_object_class_subscription.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_name_reservation.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_multiple_object_instance_name_reservation.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_region_registration.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_region_update_association.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_time_constrained_control.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_time_regulation_control.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_next_message_request.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_interaction_transportation_type_change.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_interaction_transportation_type_query.cpp
    cpp/src/internal/runtime/umbra_rti_ambassador_federation_save_restore.cpp
  )
  include(cmake/umbra_rti_ambassador_service_sources.cmake)
endif()
