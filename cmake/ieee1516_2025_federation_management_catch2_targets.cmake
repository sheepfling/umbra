# Focused Catch2 target definitions for embedded IEEE 1516.1-2025 federation management.
# Included inside the Catch2 target guard; this profile-specific block contains no 2010 targets.

if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  # These focused executables all use the same embedded 2025 test runtime.
  # Keep per-target source lists and exceptional process/MSVC wiring at their
  # declarations while centralizing the shared build contract here.
  function(umbra_add_embedded_2025_catch2_target target_name)
    cmake_parse_arguments(PARSE_ARGV 1 target_options
      "PROCESS_SERVICE_PROBE;MSVC_BIGOBJ"
      ""
      "SOURCES"
    )
    if(target_options_UNPARSED_ARGUMENTS)
      message(FATAL_ERROR
        "Unexpected arguments for ${target_name}: ${target_options_UNPARSED_ARGUMENTS}"
      )
    endif()
    if(NOT target_options_SOURCES)
      message(FATAL_ERROR "No test sources supplied for ${target_name}")
    endif()

    add_executable("${target_name}" ${target_options_SOURCES})
    target_compile_features("${target_name}" PRIVATE cxx_std_20)
    if(MSVC AND target_options_MSVC_BIGOBJ)
      target_compile_options("${target_name}" PRIVATE /bigobj)
    endif()
    target_include_directories("${target_name}" PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
    )
    target_compile_definitions("${target_name}" PRIVATE
      UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    if(target_options_PROCESS_SERVICE_PROBE)
      target_compile_definitions("${target_name}" PRIVATE
        UMBRA_PROCESS_SERVICE_PROBE_PATH="$<TARGET_FILE:umbra_process_federation_service_probe>"
      )
    endif()
    target_compile_definitions("${target_name}" PRIVATE
      UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
    )
    target_link_libraries("${target_name}" PRIVATE
      umbra_fom_validation_backend
      umbra::rti
      umbra::fedtime
      umbra::authorizer
      Catch2::Catch2WithMain
    )
  endfunction()

  umbra_add_embedded_2025_catch2_target(umbra_ieee1516_2025_connection_catch2
    PROCESS_SERVICE_PROBE
    MSVC_BIGOBJ
    SOURCES
    ${UMBRA_IEEE1516_2025_CONNECTION_TEST_SOURCES}
  )
  add_dependencies(umbra_ieee1516_2025_connection_catch2
    umbra_process_federation_service_probe
  )

  # Keep the private process-boundary tests independently buildable as
  # well.  The aggregate 2025 executable also contains the damaged
  # federation-management translation unit, so its historical private
  # tests cannot be the reproducible source for the process JUnit lane.
  umbra_add_embedded_2025_catch2_target(umbra_process_boundary_private_catch2
    PROCESS_SERVICE_PROBE
    MSVC_BIGOBJ
    SOURCES
    cpp/tests/process_transport_catch2.cpp
    cpp/tests/process_transport_service_catch2.cpp
    cpp/tests/process_federation_callback_bridge_catch2.cpp
    cpp/tests/transport_protocol_catch2.cpp
    cpp/tests/process_federation_service_codec_catch2.cpp
    cpp/tests/process_federation_service_catch2.cpp cpp/tests/process_federation_service_tso_attribute_before_grant_catch2.cpp cpp/tests/process_federation_service_attribute_relevance_advisory_catch2.cpp cpp/tests/process_federation_service_regional_class_request_attribute_value_update_catch2.cpp
    cpp/tests/process_federation_additional_fom_catch2.cpp
    cpp/tests/process_federation_service_process_catch2.cpp
  )
  add_dependencies(
    umbra_process_boundary_private_catch2
    umbra_process_federation_service_probe
  )

  # Keep the federation-wide MOM HLAsetSwitches Auto Provide case
  # independently buildable.  This narrow management slice remains
  # queryable while the large aggregate federation-management source is
  # under repair.
  umbra_add_embedded_2025_catch2_target(umbra_auto_provide_mom_catch2
    SOURCES
    cpp/tests/auto_provide_mom_catch2.cpp
  )

  # Keep the joined-federate MOM HLAsetSwitches subset independently
  # buildable. This focused case covers predefined and extension
  # parameters, sender-only mutation, malformed wire values, and the
  # service-reporting interlock without reopening the aggregate test.
  umbra_add_embedded_2025_catch2_target(umbra_mom_federate_set_switches_catch2
    SOURCES
    cpp/tests/mom_federate_set_switches_catch2.cpp
  )

  # Keep the injected writer-creation failure contract independently
  # buildable. This is an internal/test-only seam that proves a failed
  # joined-federate report setup rolls membership back without exposing
  # a memory-backed fallback through the production profile.
  umbra_add_embedded_2025_catch2_target(umbra_service_report_writer_failure_catch2
    SOURCES
    cpp/tests/service_report_writer_failure_catch2.cpp
  )

  # Keep the disabled Auto Provide discovery-only baseline independently
  # buildable. It is the negative control for the richer provider
  # admission and switch-mutation slices.
  umbra_add_embedded_2025_catch2_target(umbra_auto_provide_disabled_discovery_only_catch2
    SOURCES
    cpp/tests/auto_provide_disabled_discovery_only_catch2.cpp
  )

  # Keep object-instance-name reservation state and callback outcomes
  # independently buildable. The 2025 aggregate reuses the focused
  # translation unit so both CTest identities exercise one implementation.
  umbra_add_embedded_2025_catch2_target(umbra_object_instance_name_reservation_catch2
    SOURCES
    cpp/tests/object_instance_name_reservation_catch2.cpp
  )

  # Keep named object-instance registration independently buildable. It
  # consumes the reservation state above and exercises the regional
  # registration surface without the damaged aggregate translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_object_instance_named_registration_catch2
    SOURCES
    cpp/tests/object_instance_named_registration_catch2.cpp
  )

  # Keep unnamed object-instance registration/discovery independently
  # buildable. This focused lifecycle case covers publication,
  # superclass promotion, known-instance lookup, and callback gating
  # without depending on the damaged aggregate translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_object_instance_registration_discovery_catch2
    SOURCES
    cpp/tests/object_instance_registration_discovery_catch2.cpp
  )

  # Keep the no-time regional object-attribute routing slice independently
  # buildable. It is the focused replacement for the matching aggregate
  # declaration and owns the 2025 §§9.5-9.9 overlap evidence.
  umbra_add_embedded_2025_catch2_target(umbra_regional_object_attribute_routing_catch2
    SOURCES
    cpp/tests/regional_object_attribute_routing_catch2.cpp
  )

  # Keep the zero-dimensional regional-interaction DDM slice separately
  # buildable.  Its explicit lane proves the §9.1.3.2 no-overlap rule
  # without coupling the interaction case to the larger object-routing
  # translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_zero_dimensional_regional_interaction_catch2
    SOURCES
    cpp/tests/zero_dimensional_regional_interaction_catch2.cpp
  )

  # Keep the positive dimensional/default-region interaction matrix
  # separately runnable from the aggregate federation-management target.
  umbra_add_embedded_2025_catch2_target(umbra_default_region_interaction_routing_catch2
    SOURCES
    cpp/tests/default_region_interaction_routing_catch2.cpp
  )

  # Keep the multi-region interaction routing matrix independently
  # runnable. It exercises the official RegionHandleSet union semantics
  # without coupling the next DDM slice to the aggregate translation
  # unit.
  umbra_add_embedded_2025_catch2_target(umbra_multi_region_interaction_routing_catch2
    SOURCES
    cpp/tests/multi_region_interaction_routing_catch2.cpp
  )

  # Keep mixed-dimensional Send Interaction With Regions validation
  # independently runnable. It proves the all-or-nothing region-set
  # check without coupling this boundary to the aggregate DDM target.
  umbra_add_embedded_2025_catch2_target(umbra_mixed_region_interaction_validation_catch2
    SOURCES
    cpp/tests/mixed_region_interaction_validation_catch2.cpp
  )

  # Keep the receive-order object-deletion lifecycle independently
  # buildable. It owns the no-time §6.16/§6.16.4/§6.17.1 evidence rather
  # than depending on the damaged aggregate translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_object_instance_deletion_catch2
    SOURCES
    cpp/tests/object_instance_deletion_catch2.cpp
  )

  # Keep the receive-order attribute-update/passel lifecycle independently
  # buildable. It owns the non-region §§6.1.12, 6.10, and 6.11.1 evidence
  # rather than depending on the damaged aggregate translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_receive_order_attribute_update_catch2
    SOURCES
    cpp/tests/receive_order_attribute_update_catch2.cpp
  )

  # Keep the Delay Subscription Evaluation timestamped directed-
  # interaction case independently buildable. It covers creation-time
  # switch enablement/defaulting under both callback models.
  umbra_add_embedded_2025_catch2_target(umbra_delay_subscription_evaluation_timestamped_directed_interaction_catch2
    SOURCES
    cpp/tests/delay_subscription_evaluation_timestamped_directed_interaction_catch2.cpp
  )

  # Keep the receive-order Delay Subscription Evaluation directed-
  # interaction case independently buildable. It covers switch
  # enablement/defaulting and callback-time selector rechecking.
  umbra_add_embedded_2025_catch2_target(umbra_delay_subscription_evaluation_directed_interaction_catch2
    SOURCES
    cpp/tests/delay_subscription_evaluation_directed_interaction_catch2.cpp
  )

  # Keep the filesystem-backed Confirm Synchronization Point
  # Registration result lane independently buildable. It verifies that
  # both callback-result forms are durable before HLA_EVOKED delivery.
  umbra_add_embedded_2025_catch2_target(umbra_service_report_file_confirm_synchronization_point_registration_catch2
    SOURCES
    cpp/tests/service_report_file_confirm_synchronization_point_registration_catch2.cpp
  )

  # Keep the filesystem-backed Announce Synchronization Point recipient
  # lane independently buildable. It checks late-join routing and both
  # service-report switch gates without rotating the selected file.
  umbra_add_embedded_2025_catch2_target(umbra_service_report_file_announce_synchronization_point_catch2
    SOURCES
    cpp/tests/service_report_file_announce_synchronization_point_catch2.cpp
  )

  # Keep the filesystem-backed Federation Synchronized recipient lane
  # independently buildable. It verifies per-recipient serial ordering,
  # failed-set encoding, and append-before-callback behavior.
  umbra_add_embedded_2025_catch2_target(umbra_service_report_file_federation_synchronized_catch2
    SOURCES
    cpp/tests/service_report_file_federation_synchronized_catch2.cpp
  )

  # Keep the filesystem-backed Synchronization Point Achieved argument
  # lane independently buildable. It isolates §4.17.1 successful-void
  # record forms from the §4.18 completion record.
  umbra_add_embedded_2025_catch2_target(umbra_service_report_file_synchronization_point_achieved_catch2
    SOURCES
    cpp/tests/service_report_file_synchronization_point_achieved_catch2.cpp
  )

  # Keep the resignation-completion service-report lane independently
  # buildable. It isolates removal of an unachieved sync-set member.
  umbra_add_embedded_2025_catch2_target(umbra_service_report_file_federation_synchronized_resignation_catch2
    SOURCES
    cpp/tests/service_report_file_federation_synchronized_resignation_catch2.cpp
  )

  # Keep the local-delete/timestamped-attribute recipient-isolation case
  # independently buildable.  This focused time-management lane proves
  # one recipient can forget queued work without affecting its survivor.
  umbra_add_embedded_2025_catch2_target(umbra_local_delete_timestamped_attribute_catch2
    SOURCES
    cpp/tests/local_delete_timestamped_attribute_catch2.cpp
  )

  # Keep the paired local-delete/timestamped-object-removal case
  # independently buildable.  It isolates a queued TSO removal per
  # recipient and retains the official removal metadata assertions.
  umbra_add_embedded_2025_catch2_target(umbra_local_delete_timestamped_object_removal_catch2
    SOURCES
    cpp/tests/local_delete_timestamped_object_removal_catch2.cpp
  )

  # Keep the timestamped source-resignation/ownership-recovery case
  # independently buildable.  It proves an accepted TSO passel remains
  # deliverable after its source resigns and another federate acquires.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_attribute_source_resignation_catch2
    SOURCES
    cpp/tests/timestamped_attribute_source_resignation_catch2.cpp
  )

  # Keep the four-member timestamped source-resignation fanout case
  # independently buildable.  It releases each recipient queue in turn
  # and verifies the copied TSO metadata remains recipient-local.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_attribute_source_resignation_fanout_catch2
    SOURCES
    cpp/tests/timestamped_attribute_source_resignation_fanout_catch2.cpp
  )

  # Keep the accepted-TSO ownership-transfer case independently
  # buildable. It verifies that an in-flight timestamped update retains
  # its original producer, payload, time, and retraction after
  # unconditional divestiture and If Available acquisition.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_attribute_update_ownership_transfer_catch2
    SOURCES
    cpp/tests/timestamped_attribute_update_ownership_transfer_catch2.cpp
  )

  # Keep the focused attribute-transportation control contract
  # independently buildable. It isolates FOM defaults, per-instance
  # changes, and public query/confirmation callbacks from the aggregate
  # federation-management executable.
  umbra_add_embedded_2025_catch2_target(umbra_custom_transportation_type_control_catch2
    SOURCES
    cpp/tests/custom_transportation_type_control_catch2.cpp
  )

  # Keep the declared custom-transportation ordinary-delivery case
  # independently buildable. It carries the composed FOM transport
  # through receive-order interaction and attribute callbacks.
  umbra_add_embedded_2025_catch2_target(umbra_custom_transportation_delivery_catch2
    SOURCES
    cpp/tests/custom_transportation_delivery_catch2.cpp
  )

  # Keep the declared custom-transportation ordinary-regional case
  # independently buildable. It isolates regional interaction overlap
  # and callback designator propagation from the aggregate executable.
  umbra_add_embedded_2025_catch2_target(umbra_custom_transportation_ordinary_regional_interaction_catch2
    SOURCES
    cpp/tests/custom_transportation_ordinary_regional_interaction_catch2.cpp
  )

  # Keep the declared custom-transportation ordinary-regional-attribute
  # case independently buildable. It isolates regional object routing
  # and conveyed designators from the aggregate executable.
  umbra_add_embedded_2025_catch2_target(umbra_custom_transportation_ordinary_regional_attribute_catch2
    SOURCES
    cpp/tests/custom_transportation_ordinary_regional_attribute_catch2.cpp
  )

  # Keep the declared custom-transportation timestamped-delivery case
  # independently buildable. It composes the official interaction and
  # attribute callback surfaces with a user-declared FOM transportation.
  umbra_add_embedded_2025_catch2_target(umbra_custom_transportation_timestamped_delivery_catch2
    SOURCES
    cpp/tests/custom_transportation_timestamped_delivery_catch2.cpp
  )

  # Keep the declared custom-transportation timestamped-directed case
  # independently buildable. It isolates target routing and retraction
  # metadata from the aggregate federation-management executable.
  umbra_add_embedded_2025_catch2_target(umbra_custom_transportation_timestamped_directed_delivery_catch2
    SOURCES
    cpp/tests/custom_transportation_timestamped_directed_delivery_catch2.cpp
  )

  # Keep the declared custom-transportation timestamped regional
  # attribute case independently buildable. It isolates DDM region
  # designator propagation from the aggregate federation-management
  # executable while retaining the official C++ callback surface.
  umbra_add_embedded_2025_catch2_target(umbra_custom_transportation_timestamped_regional_attribute_delivery_catch2
    SOURCES
    cpp/tests/custom_transportation_timestamped_regional_attribute_delivery_catch2.cpp
  )

  # Keep the MOM transportation-type request interaction case
  # independently buildable. It exercises the official MIM payload
  # encoding, callback-gated public changes, and both callback models.
  umbra_add_embedded_2025_catch2_target(umbra_mom_transportation_type_change_request_catch2
    SOURCES
    cpp/tests/mom_transportation_type_change_request_catch2.cpp
  )

  # Keep the multi-recipient timestamped directed-interaction resignation
  # case independently buildable. It isolates queued TSO delivery and
  # per-recipient grant ordering from the damaged aggregate target.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_directed_interaction_multi_recipient_source_resignation_catch2
    SOURCES
    cpp/tests/timestamped_directed_interaction_multi_recipient_source_resignation_catch2.cpp
  )

  # Keep the direct TAR/NMR timestamped directed-interaction frontier
  # independently buildable. It is the narrow two-recipient companion
  # to the broader alternate-advance and source-resignation cases.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_directed_interaction_tar_nmr_catch2
    SOURCES
    cpp/tests/timestamped_directed_interaction_tar_nmr_catch2.cpp
  )

  # Keep federation-scoped MOM save-conditionals independently
  # buildable. This isolates the pending/admitted/completed conditional
  # attribute contract from the damaged aggregate federation-management
  # translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_federation_mom_save_conditionals_catch2
    SOURCES
    cpp/tests/federation_mom_save_conditionals_catch2.cpp
  )

  # Keep the federation-scoped HLAcurrentFDD projection independently
  # buildable. This isolates the composed-FDD request and conditional
  # refresh contract from the damaged aggregate federation-management
  # translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_federation_mom_current_fdd_catch2
    SOURCES
    cpp/tests/federation_mom_current_fdd_catch2.cpp
  )

  # Keep the joined-federate MOM GALT/LITS projection independently
  # buildable. This isolates Query GALT/Query LITS and periodic
  # HLAsetTiming reflections from the aggregate federation-management
  # translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_galt_lits_periodic_catch2
    SOURCES
    cpp/tests/joined_federate_mom_galt_lits_periodic_catch2.cpp
  )

  # Keep the joined-federate MOM TSO-length projection independently
  # buildable. This isolates the recipient-scoped queued TSO ledger and
  # its HLAsetTiming projection from the aggregate test translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_tso_length_periodic_catch2
    SOURCES
    cpp/tests/joined_federate_mom_tso_length_periodic_catch2.cpp
  )

  # Keep the joined-federate MOM time-state duration projection
  # independently buildable. This isolates the direct and periodic
  # HLAtimeGrantedTime/HLAtimeAdvancingTime contract from the aggregate
  # federation-management translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_time_state_duration_catch2
    SOURCES
    cpp/tests/joined_federate_mom_time_state_duration_catch2.cpp
  )

  # Keep the joined-federate MOM reflection-count projection
  # independently buildable. This isolates the distinct-object versus
  # callback-invocation ledger from the aggregate federation-management
  # translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_reflection_count_catch2
    SOURCES
    cpp/tests/joined_federate_mom_reflection_count_catch2.cpp
  )

  # Keep the joined-federate MOM updates-sent projection independently
  # buildable. This isolates accepted Update Attribute Values ledger
  # accounting, transportation buckets, and nested class-count reports.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_updates_sent_catch2
    SOURCES
    cpp/tests/joined_federate_mom_updates_sent_catch2.cpp
  )

  # Keep the joined-federate MOM reflections-received projection
  # independently buildable. This isolates accepted application
  # reflection accounting, transportation buckets, and nested
  # class-count reports.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_reflections_received_catch2
    SOURCES
    cpp/tests/joined_federate_mom_reflections_received_catch2.cpp
  )

  # Keep the joined-federate MOM interactions-received projection
  # independently buildable. This isolates accepted application receive
  # callbacks, transportation buckets, and nested class-count reports.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_interactions_received_catch2
    SOURCES
    cpp/tests/joined_federate_mom_interactions_received_catch2.cpp
  )

  # Keep the joined-federate MOM directed-interactions-received
  # projection independently buildable. This isolates the directed
  # receive subset, ordinary-receive exclusion, and nested count report.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_directed_interactions_received_catch2
    SOURCES
    cpp/tests/joined_federate_mom_directed_interactions_received_catch2.cpp
  )

  # Keep the joined-federate MOM interactions-sent projection
  # independently buildable. This isolates accepted Send Interaction
  # accounting, regional inclusion, transportation buckets, and nested
  # interaction-count reports.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_interactions_sent_catch2
    SOURCES
    cpp/tests/joined_federate_mom_interactions_sent_catch2.cpp
  )

  # Keep the cross-family MOM NULL-bucket projection independently
  # buildable. This isolates empty sender ledgers for updates, ordinary
  # interactions, and directed interactions.
  umbra_add_embedded_2025_catch2_target(umbra_mom_sender_count_reports_null_buckets_catch2
    SOURCES
    cpp/tests/mom_sender_count_reports_null_buckets_catch2.cpp
  )

  # Keep the joined-federate MOM removed-object history projection
  # independently buildable. This combines the public deletion/retraction
  # boundary with the receiving federate's historical MOM counter.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_removed_object_count_tso_retraction_catch2
    SOURCES
    cpp/tests/joined_federate_mom_removed_object_count_tso_retraction_catch2.cpp
  )

  # Keep the Allow Relaxed DDM object-attribute boundary case
  # independently buildable.  It runs the enabled and strict profiles
  # without depending on the damaged aggregate translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_allow_relaxed_ddm_object_attribute_catch2
    SOURCES
    cpp/tests/allow_relaxed_ddm_object_attribute_catch2.cpp
  )

  # Keep the timestamped directed-interaction re-enable case
  # independently buildable.  It isolates one queued directed TSO passel
  # across a disable/enable cycle and checks callback/grant ordering.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_directed_interaction_reenable_catch2
    SOURCES
    cpp/tests/timestamped_directed_interaction_reenable_catch2.cpp
  )

  # Keep the queued directed-TSO source-resignation case independently
  # buildable.  A surviving target owner and regulator release a passel
  # after the separate interaction producer leaves the federation.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_directed_interaction_source_resignation_catch2
    SOURCES
    cpp/tests/timestamped_directed_interaction_source_resignation_catch2.cpp
  )

  # Keep the directed timestamped queue/retraction case independently
  # buildable. It isolates pre-grant retract, then a later eligible TSO
  # delivery with the official directed-interaction metadata.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_directed_interaction_retraction_catch2
    SOURCES
    cpp/tests/timestamped_directed_interaction_retraction_catch2.cpp
  )

  # Keep the immediate-recipient directed TSO resignation case
  # independently buildable. It verifies that source resignation does
  # not discard a callback already queued on an unconstrained receiver.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_directed_interaction_immediate_source_resignation_catch2
    SOURCES
    cpp/tests/timestamped_directed_interaction_immediate_source_resignation_catch2.cpp
  )

  # Keep the terminal timestamped Send Interaction/retraction case
  # independently buildable.  It exercises pre-grant retraction,
  # terminal post-delivery classification, and Request Retraction.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_send_interaction_retraction_catch2
    SOURCES
    cpp/tests/timestamped_send_interaction_retraction_catch2.cpp
  )

  # Keep the no-recipient timestamped Send Interaction designator case
  # independently buildable.  It exercises the retraction ledger without
  # conflating designator validity with recipient fanout.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_send_interaction_no_fanout_catch2
    SOURCES
    cpp/tests/timestamped_send_interaction_no_fanout_catch2.cpp
  )

  # Keep the no-recipient timestamped Update Attribute Values designator
  # case independently buildable. It verifies that the public retraction
  # contract is not contingent on retaining recipient fanout state.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_attribute_update_no_fanout_catch2
    SOURCES
    cpp/tests/timestamped_attribute_update_no_fanout_catch2.cpp
  )

  # Keep the m53 timestamped attribute-update queue/retraction case
  # independently buildable. It isolates the pre-grant retract boundary
  # and the later mixed-transportation delivery cohort.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_attribute_update_queued_passel_retraction_catch2
    SOURCES
    cpp/tests/timestamped_attribute_update_queued_passel_retraction_catch2.cpp
  )

  # Keep the five-form timestamped designator-terminalization case
  # independently buildable. It isolates the producer-side advance
  # boundary that makes each designator no longer retractable.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_tso_designator_terminalization_catch2
    SOURCES
    cpp/tests/timestamped_tso_designator_terminalization_catch2.cpp
  )

  # Keep the Time Regulation disable/re-enable TSO case independently
  # buildable.  It proves the original recipient queue survives role
  # mutation and retains its timestamped metadata.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_interaction_regulation_reenable_catch2
    SOURCES
    cpp/tests/timestamped_interaction_regulation_reenable_catch2.cpp
  )

  # Keep the mixed regional timestamped attribute-update advance case
  # independently buildable. It exercises one shared TSO frontier while
  # FQR, TARA, and NMRA use their distinct official request surfaces.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_regional_attribute_update_mixed_advance_catch2
    SOURCES
    cpp/tests/timestamped_regional_attribute_update_mixed_advance_catch2.cpp
  )

  # Keep the default-source/default-region mixed advance case
  # independently buildable. It complements the explicit regional
  # source lane while retaining the same FQR/TARA/NMRA frontier.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_default_region_attribute_update_mixed_advance_catch2
    SOURCES
    cpp/tests/timestamped_default_region_attribute_update_mixed_advance_catch2.cpp
  )

  # Keep the default-source/default-region timestamped-retraction case
  # independently buildable. It pairs TARA and NMRA alternate advances
  # and proves the queued default-region passel is removed before either
  # recipient crosses its callback boundary.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_default_region_attribute_update_retract_alternate_advance_catch2
    SOURCES
    cpp/tests/timestamped_default_region_attribute_update_retract_alternate_advance_catch2.cpp
  )

  # Keep the regional timestamped-retraction/mixed-advance case
  # independently buildable. It proves all three alternate requests
  # complete after one explicit-source passel is retracted.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_regional_attribute_update_retract_mixed_advance_catch2
    SOURCES
    cpp/tests/timestamped_regional_attribute_update_retract_mixed_advance_catch2.cpp
  )

  # Keep the ordinary regional TAR/NMR timestamped-update case
  # independently buildable. It is the focused two-recipient counterpart
  # to the broader FQR/TARA/NMRA frontier tests.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_regional_attribute_update_tar_nmr_catch2
    SOURCES
    cpp/tests/timestamped_regional_attribute_update_tar_nmr_catch2.cpp
  )

  # Keep the default-source regional timestamped-update resignation
  # case independently buildable. It isolates the queued passel's
  # lifetime across source resignation and its surviving regulator.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_regional_attribute_update_resignation_catch2
    SOURCES
    cpp/tests/timestamped_regional_attribute_update_resignation_catch2.cpp
  )

  # Keep the mixed-fanout regional TSO/retraction case independently
  # buildable. It pairs an immediate recipient with an evoked,
  # time-constrained recipient and checks region metadata in both modes.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_regional_attribute_update_mixed_fanout_catch2
    SOURCES
    cpp/tests/timestamped_regional_attribute_update_mixed_fanout_catch2.cpp
  )

  # Keep the regional best-effort timestamped-rate case independently
  # buildable. It exercises an FDD Low update-rate gate on a regional
  # subscription while the recipient still crosses each TSO boundary.
  umbra_add_embedded_2025_catch2_target(umbra_regional_best_effort_timestamped_attribute_rate_catch2
    SOURCES
    cpp/tests/regional_best_effort_timestamped_attribute_rate_catch2.cpp
  )

  # Keep the receive-order regional best-effort-rate case independently
  # buildable. It exercises the FDD Low update-rate gate and subscription
  # generation reset without depending on the aggregate test binary.
  umbra_add_embedded_2025_catch2_target(umbra_regional_best_effort_attribute_rate_catch2
    SOURCES
    cpp/tests/regional_best_effort_attribute_rate_catch2.cpp
  )

  # Keep the nonregional TARA/NMRA timestamped update case independently
  # buildable. It is the compact available-advance frontier for the
  # attribute-update service.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_attribute_update_available_advance_catch2
    SOURCES
    cpp/tests/timestamped_attribute_update_available_advance_catch2.cpp
  )

  # Keep the timestamped update-rate reduction case independently
  # buildable. It isolates the FDD-defined Low-rate boundary while
  # retaining reliable attributes and closing suppressed TSO retractions.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_attribute_update_rate_reduction_catch2
    SOURCES
    cpp/tests/timestamped_attribute_update_rate_reduction_catch2.cpp
  )

  # Keep the ordinary timestamped attribute re-enable lifecycle case
  # independently buildable. It verifies that a queued passel survives
  # disable/re-enable of Time Constrained without duplication.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_attribute_update_reenable_catch2
    SOURCES
    cpp/tests/timestamped_attribute_update_reenable_catch2.cpp
  )

  # Keep the alternate-advance timestamped attribute-ordering case
  # independently buildable. It exercises equal-timestamp cohorts for
  # both TARA and NMRA before admitting the later queued update.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_attribute_update_alternate_advance_catch2
    SOURCES
    cpp/tests/timestamped_attribute_update_alternate_advance_catch2.cpp
  )

  # Keep the different-timestamp attribute-order cohort independently
  # buildable. It submits timestamp 7 before the timestamp-5 cohort and
  # checks recipient-local ordering through the ordinary TSO advance.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_attribute_order_cohort_catch2
    SOURCES
    cpp/tests/timestamped_attribute_order_cohort_catch2.cpp
  )

  # Keep the default-region interaction retraction/alternate-advance
  # case independently buildable. It isolates private default-source
  # overlap with regional subscribers and grant completion after retract.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_default_region_interaction_retract_alternate_advance_catch2
    SOURCES
    cpp/tests/timestamped_default_region_interaction_retract_alternate_advance_catch2.cpp
  )

  # Keep the timestamped interaction changed-lookahead lifecycle case
  # independently buildable. It verifies that disabling and re-enabling
  # regulation retains the queued message and retract identity.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_interaction_regulation_reenable_changed_lookahead_catch2
    SOURCES
    cpp/tests/timestamped_interaction_regulation_reenable_changed_lookahead_catch2.cpp
  )

  # Keep the timestamped directed-interaction changed-lookahead
  # lifecycle case independently buildable. It verifies that a target-
  # qualified queued interaction retains its retraction identity when
  # regulation is disabled and re-enabled with a new lookahead.
  umbra_add_embedded_2025_catch2_target(umbra_tso_directed_reenable_changed_lookahead_catch2
    SOURCES
    cpp/tests/timestamped_directed_interaction_regulation_reenable_changed_lookahead_catch2.cpp
  )

  # Keep the timestamped attribute changed-lookahead lifecycle case
  # independently buildable. It verifies that disabling and re-enabling
  # regulation retains the queued update and retraction identity.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_attribute_update_regulation_reenable_changed_lookahead_catch2
    SOURCES
    cpp/tests/timestamped_attribute_update_regulation_reenable_changed_lookahead_catch2.cpp
  )

  # Keep the timed multi-recipient regional save/restore ownership case
  # independently buildable. This focused matrix preserves the saved TSO
  # source while cancelling a retained negotiated confirmation.
  umbra_add_embedded_2025_catch2_target(umbra_timed_multi_recipient_regional_attribute_restore_negotiated_cancel_catch2
    SOURCES
    cpp/tests/timed_multi_recipient_regional_attribute_restore_negotiated_cancel_catch2.cpp
  )

  # Keep the timed multi-recipient regional negotiated-cancellation
  # matrix independently buildable. It restores a queued timestamped
  # update, enters negotiated divestiture, and cancels the requester's
  # pending ownership acquisition before confirmation delivery.
  umbra_add_embedded_2025_catch2_target(umbra_tso_regional_negotiated_cancel_restore_catch2
    SOURCES
    cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_cancel_pending_after_restore_catch2.cpp
  )

  # Keep the timed multi-recipient regional negotiated-continuation
  # matrix independently buildable. It restores a queued timestamped
  # update, retains an If Available candidate after requester
  # cancellation, and completes the surviving negotiated transfer.
  umbra_add_embedded_2025_catch2_target(umbra_tso_regional_negotiated_continuation_restore_catch2
    SOURCES
    cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_continuation_after_restore_catch2.cpp
  )

  # Keep the timed regular-candidate continuation case independently
  # buildable. The wrapper exposes the macro-based fixture as a direct
  # TEST_CASE for source indexing and a focused CTest handle.
  umbra_add_embedded_2025_catch2_target(umbra_tso_regional_regular_continuation_restore_catch2
    SOURCES
    cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_regular_candidate_continuation_after_restore_lane_catch2.cpp
  )

  # Keep the timed multi-recipient mixed-candidate continuation case
  # independently buildable. It reuses the regular-candidate fixture
  # while selecting an If Available candidate for the surviving clock.
  umbra_add_embedded_2025_catch2_target(umbra_tso_regional_mixed_continuation_restore_catch2
    SOURCES
    cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_mixed_candidate_continuation_after_restore_catch2.cpp
  )

  # Keep the timed mixed retained-regular pre-delivery cancellation case
  # independently buildable. It reuses the regional save/restore
  # fixture while cancelling before Request Divestiture Confirmation.
  umbra_add_embedded_2025_catch2_target(umbra_tso_mixed_pre_delivery_cancel_catch2
    SOURCES
    cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_pre_delivery_cancel_after_restore_catch2.cpp
  )

  # Keep the timed mixed retained-regular confirmation cancellation case
  # independently buildable. It reuses the regional save/restore
  # fixture while cancelling after Request Divestiture Confirmation.
  umbra_add_embedded_2025_catch2_target(umbra_tso_mixed_confirmation_cancel_catch2
    SOURCES
    cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_confirmation_cancel_after_restore_catch2.cpp
  )

  # Keep the timed multi-recipient regional negotiated-confirmation
  # cancellation case independently buildable. It restores a queued
  # timestamped update, delivers the retained owner's confirmation, and
  # verifies cancellation rejects stale confirmation without transferring
  # ownership.
  umbra_add_embedded_2025_catch2_target(umbra_tso_regional_negotiated_confirmation_cancel_restore_catch2
    SOURCES
    cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_confirmation_cancel_after_restore_catch2.cpp
  )

  # Keep the nonregional Flush Queue timestamped-update case
  # independently buildable. It checks actual-versus-optimistic grant
  # times and passel ordering without depending on the aggregate source.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_attribute_update_flush_queue_catch2
    SOURCES
    cpp/tests/timestamped_attribute_update_flush_queue_catch2.cpp
  )

  # Keep the future-input Flush Queue interaction case independently
  # buildable. It checks that TSO messages submitted after FQR admission
  # still precede its grant in the evoked callback queue.
  umbra_add_embedded_2025_catch2_target(umbra_flush_queue_future_input_catch2
    SOURCES
    cpp/tests/flush_queue_future_input_catch2.cpp
  )

  # Keep the no-recipient timestamped deletion/retraction case
  # independently buildable. It isolates the execution-owned
  # reconstitution boundary before joined-recipient fanout is added.
  umbra_add_embedded_2025_catch2_target(umbra_request_retraction_timestamped_deletion_no_fanout_catch2
    SOURCES
    cpp/tests/request_retraction_timestamped_deletion_no_fanout_catch2.cpp
  )

  # Keep the delivered-recipient timestamped object-deletion retraction
  # case independently buildable. It isolates object reconstitution,
  # Request Retraction fanout, and suppression of the pending recipient.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_object_deletion_retraction_catch2
    SOURCES
    cpp/tests/timestamped_object_deletion_retraction_catch2.cpp
  )

  # Keep the joined-owner timestamped-deletion retraction case
  # independently buildable. It isolates resignation filtering during
  # object reconstitution and the resulting ownership query boundary.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_object_deletion_joined_owner_retraction_catch2
    SOURCES
    cpp/tests/timestamped_object_deletion_joined_owner_retraction_catch2.cpp
  )

  # Keep the optimistic-time Flush Queue Request slice independently
  # buildable. It isolates queued TSO FIFO delivery and grant timing.
  umbra_add_embedded_2025_catch2_target(umbra_flush_queue_request_optimistic_time_catch2
    SOURCES
    cpp/tests/flush_queue_request_optimistic_time_catch2.cpp
  )

  # Keep the terminal timestamped-deletion tombstone case independently
  # buildable. It verifies strict Retract finalization and object-name
  # reuse after the retained deletion state is released.
  umbra_add_embedded_2025_catch2_target(umbra_terminal_timestamped_deletion_tombstone_catch2
    SOURCES
    cpp/tests/terminal_timestamped_deletion_tombstone_catch2.cpp
  )

  # Keep the source-resignation timestamped-deletion fanout case
  # independently buildable. It verifies that each recipient-local
  # removal queue survives the producing owner's departure.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_object_deletion_source_resignation_fanout_catch2
    SOURCES
    cpp/tests/timestamped_object_deletion_source_resignation_fanout_catch2.cpp
  )

  # Keep the compact queued-TSO Next Message Request case independently
  # buildable. It selects the next message timestamp after GALT opens
  # without pulling in the larger alternate-advance matrix.
  umbra_add_embedded_2025_catch2_target(umbra_next_message_request_queued_tso_catch2
    SOURCES
    cpp/tests/next_message_request_queued_tso_catch2.cpp
  )

  # Keep the paired Available-time TARA/NMRA interaction case
  # independently buildable. It exercises inclusive GALT at two queued
  # timestamp frontiers without introducing a new transport seam.
  umbra_add_embedded_2025_catch2_target(umbra_available_time_advance_inclusive_galt_catch2
    SOURCES
    cpp/tests/available_time_advance_inclusive_galt_catch2.cpp
  )

  # Keep the bounded connection-loss/TSO declaration-recheck case
  # independently buildable.  The aggregate federation-management
  # translation unit currently carries unrelated source drift; this
  # target lets the exact planned case compile and run without reopening
  # that aggregate file.
  umbra_add_embedded_2025_catch2_target(umbra_connection_loss_attribute_update_tso_suppressed_cleanup_catch2
    SOURCES
    cpp/tests/connection_loss_attribute_update_tso_suppressed_cleanup_catch2.cpp
  )

  # Keep the multi-recipient automatic-cleanup connection-loss case
  # independently buildable. It is already a mapped C++ plan row, but
  # the aggregate federation-management translation unit is not a safe
  # execution boundary while its unrelated source artifact is being
  # reconciled.
  umbra_add_embedded_2025_catch2_target(umbra_connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient_catch2
    SOURCES
    cpp/tests/connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient_catch2.cpp
  )

  # Keep the single-survivor automatic-cleanup connection-loss case
  # independently buildable.  This is the exact planned cutoff slice;
  # the multi-recipient variant above remains a separate evidence row.
  umbra_add_embedded_2025_catch2_target(umbra_connection_loss_attribute_update_tso_automatic_cleanup_catch2
    SOURCES
    cpp/tests/connection_loss_attribute_update_tso_automatic_cleanup_catch2.cpp
  )

  # Keep the directed-selector connection-loss variants independently
  # buildable.  This target contains both the ownership-selector and
  # unsubscribe-at-callback-boundary cases without the aggregate test
  # translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_connection_loss_directed_selector_mutation_catch2
    SOURCES
    cpp/tests/connection_loss_directed_selector_mutation_catch2.cpp
  )

  # Keep the HLA_EVOKED asynchronous-delivery cutoff case independently
  # buildable alongside the existing HLA_IMMEDIATE counterpart.
  umbra_add_embedded_2025_catch2_target(umbra_connection_loss_attribute_update_tso_asynchronous_delivery_catch2
    SOURCES
    cpp/tests/connection_loss_attribute_update_tso_immediate_catch2.cpp
  )

  # Keep the single-recipient timestamped-interaction resignation case
  # independently buildable beside the existing fan-out variant.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_interaction_source_resignation_catch2
    SOURCES
    cpp/tests/timestamped_interaction_source_resignation_fanout_catch2.cpp
  )

  # Keep the default-region timestamped attribute callback contract
  # independently buildable.  This is the narrow DDM/time slice behind
  # the next roadmap pointer and does not depend on the damaged
  # federation-management aggregate translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_default_region_attribute_update_catch2
    SOURCES
    cpp/tests/timestamped_default_region_attribute_update_catch2.cpp
  )

  # Keep the default-region timestamped re-enable case independently
  # buildable.  It exercises the callback-gated Time Constrained role
  # transition without relying on the aggregate federation-management
  # translation unit.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_default_region_attribute_reenable_catch2
    SOURCES
    cpp/tests/timestamped_default_region_attribute_reenable_catch2.cpp
  )

  # Keep the default-region mixed-fanout/retraction case independently
  # buildable.  It covers one immediate recipient and one constrained
  # recipient while the aggregate source remains under recovery.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_default_region_attribute_mixed_fanout_catch2
    SOURCES
    cpp/tests/timestamped_default_region_attribute_mixed_fanout_catch2.cpp
  )

  # Keep the mixed regular/If Available Divestiture If Wanted case
  # independently buildable. It isolates serial multi-federate callback
  # ordering and the selected owner's follow-up release request.
  umbra_add_embedded_2025_catch2_target(umbra_attribute_ownership_divestiture_if_wanted_mixed_acquirer_catch2
    SOURCES
    cpp/tests/attribute_ownership_divestiture_if_wanted_mixed_acquirer_catch2.cpp
  )

  # Keep the supplied-set Divestiture If Wanted case independently
  # buildable. It isolates the empty-result path, mixed pending-acquirer
  # transfer, and acquisition-notification tag/ownership boundaries.
  umbra_add_embedded_2025_catch2_target(umbra_attribute_ownership_divestiture_if_wanted_pending_catch2
    SOURCES
    cpp/tests/attribute_ownership_divestiture_if_wanted_pending_catch2.cpp
  )

  # Keep the negotiated-divestiture transition independently buildable.
  # It isolates owner Waiting/confirmation state, cancellation rollback,
  # and the final acquisition-notification tag boundary.
  umbra_add_embedded_2025_catch2_target(umbra_negotiated_attribute_ownership_divestiture_pending_catch2
    SOURCES
    cpp/tests/negotiated_attribute_ownership_divestiture_pending_catch2.cpp
  )

  # Keep the MOM Service Reporting subscription interlock independently
  # buildable. It isolates ordinary/regional active and passive
  # subscription admission from the broader MOM service matrix.
  umbra_add_embedded_2025_catch2_target(umbra_mom_service_reporting_interlock_catch2
    SOURCES
    cpp/tests/mom_service_reporting_interlock_catch2.cpp
  )

  # Keep the joined-federate MOM deletable-object projection independently
  # buildable. It isolates the live implicit-privilege ownership ledger,
  # direct AVU request, and HLAsetTiming periodic reflection paths.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_deletable_object_count_catch2
    SOURCES
    cpp/tests/joined_federate_mom_deletable_object_count_catch2.cpp
  )

  # Keep the joined-federate MOM receive-order length projection
  # independently buildable. It isolates the callback-route queue
  # ledger, direct MOM request, and HLAsetTiming periodic reflection.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_ro_length_periodic_catch2
    SOURCES
    cpp/tests/joined_federate_mom_ro_length_periodic_catch2.cpp
  )

  # Keep the joined-federate MOM object-instances-updated report
  # independently buildable. It isolates the Subscribe-only request,
  # accepted update ledger, nested class-count encoding, and
  # RTI-originated callback route.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_object_instances_updated_report_catch2
    SOURCES
    cpp/tests/joined_federate_mom_object_instances_updated_report_catch2.cpp
  )

  # Keep the joined-federate MOM deletable-object report independently
  # buildable. It isolates the Subscribe-only request, live
  # HLAprivilegeToDeleteObject ownership ledger, nested class-count
  # encoding, and RTI-originated callback route.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_object_instances_that_can_be_deleted_report_catch2
    SOURCES
    cpp/tests/joined_federate_mom_object_instances_that_can_be_deleted_report_catch2.cpp
  )

  # Keep the joined-federate MOM reflected-object report independently
  # buildable. It isolates the accepted application reflection ledger,
  # nested class-count encoding, and RTI-originated callback route.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_object_instances_reflected_report_catch2
    SOURCES
    cpp/tests/joined_federate_mom_object_instances_reflected_report_catch2.cpp
  )

  # Keep the joined-federate MOM object-information report independently
  # buildable. It isolates known/NULL object snapshots, nested owned
  # attribute lists, and the requesting federate's callback route.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_object_instance_information_report_catch2
    SOURCES
    cpp/tests/joined_federate_mom_object_instance_information_report_catch2.cpp
  )

  # Keep the joined-federate MOM successful-update projection independently
  # buildable. It isolates the accepted Update Attribute Values ledger,
  # direct MOM request, and HLAsetTiming periodic reflection paths.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_updates_sent_periodic_catch2
    SOURCES
    cpp/tests/joined_federate_mom_updates_sent_periodic_catch2.cpp
  )

  # Keep the joined-federate MOM updated-object projection independently
  # buildable. It isolates the distinct-object Update Attribute Values
  # ledger, direct MOM request, and HLAsetTiming periodic reflection.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_updated_object_count_periodic_catch2
    SOURCES
    cpp/tests/joined_federate_mom_updated_object_count_periodic_catch2.cpp
  )

  # Keep the joined-federate MOM registered-object projection
  # independently buildable. It isolates the registration counter and
  # its relationship to the accepted update/object ledgers.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_registered_object_count_periodic_catch2
    SOURCES
    cpp/tests/joined_federate_mom_registered_object_count_periodic_catch2.cpp
  )

  # Keep the joined-federate MOM deleted-object projection independently
  # buildable. It isolates accepted deletion history from live object
  # membership and periodic MOM reflection.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_deleted_object_count_periodic_catch2
    SOURCES
    cpp/tests/joined_federate_mom_deleted_object_count_periodic_catch2.cpp
  )

  # Keep the receiving-federate MOM removed-object projection
  # independently buildable. It isolates committed Remove Object
  # Instance callbacks from the sender's deletion history and periodic
  # MOM reflection.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_removed_object_count_periodic_catch2
    SOURCES
    cpp/tests/joined_federate_mom_removed_object_count_periodic_catch2.cpp
  )

  # Keep the receiving-federate MOM discovered-object projection
  # independently buildable. It isolates committed Discover Object
  # Instance callbacks, local-delete rediscovery, and periodic MOM
  # reflection.
  umbra_add_embedded_2025_catch2_target(umbra_joined_federate_mom_discovered_object_count_periodic_catch2
    SOURCES
    cpp/tests/joined_federate_mom_discovered_object_count_periodic_catch2.cpp
  )

  # Keep the official handle-normalization support-services surface
  # independently buildable. It isolates connection/member fences,
  # typed invalid-designator errors, and execution-scoped DDM
  # point-coordinate stability from the broader DDM suites.
  umbra_add_embedded_2025_catch2_target(umbra_handle_normalization_catch2
    SOURCES
    cpp/tests/handle_normalization_catch2.cpp
  )

  # Keep the Willing-to-Acquire candidate selection case independently
  # buildable. It isolates the private If Available reservation and its
  # confirmation/notification handoff.
  umbra_add_embedded_2025_catch2_target(umbra_negotiated_willing_to_acquire_candidate_catch2
    SOURCES
    cpp/tests/negotiated_willing_to_acquire_candidate_catch2.cpp
  )

  # Keep the receive-order interaction matrix independently buildable.
  # It isolates active/passive promotion, sender suppression, parameter
  # validation, and callback-time unsubscribe behavior.
  umbra_add_embedded_2025_catch2_target(umbra_receive_order_interaction_catch2
    SOURCES
    cpp/tests/receive_order_interaction_catch2.cpp
  )

  # Keep the receive-order directed-interaction case independently
  # buildable. It isolates target-known routing, declaration fences,
  # sender exclusion, and target-departure callback suppression.
  umbra_add_embedded_2025_catch2_target(umbra_directed_interaction_known_target_catch2
    SOURCES
    cpp/tests/directed_interaction_known_target_catch2.cpp
  )

  # Keep the directed subscription selector matrix independently
  # buildable. It isolates default by-ownership, universal, empty-set,
  # and supplied-class mode changes.
  umbra_add_embedded_2025_catch2_target(umbra_directed_interaction_subscription_kind_catch2
    SOURCES
    cpp/tests/directed_interaction_subscription_kind_catch2.cpp
  )

  # Keep the strict three-dimensional object-attribute overlap case
  # independently buildable. It isolates complete-overlap routing and
  # one-dimension disjoint suppression under HLA_EVOKED.
  umbra_add_embedded_2025_catch2_target(umbra_three_dimensional_regional_object_attribute_overlap_catch2
    SOURCES
    cpp/tests/three_dimensional_regional_object_attribute_overlap_catch2.cpp
  )

  # Keep the partial multi-attribute negotiated-cancellation case
  # independently buildable. It isolates the retained-versus-cancelled
  # member reservation and the single confirmation/notification path.
  umbra_add_embedded_2025_catch2_target(umbra_negotiated_divestiture_partial_acquisition_cancellation_catch2
    SOURCES
    cpp/tests/negotiated_divestiture_partial_acquisition_cancellation_catch2.cpp
  )

  # Keep the pre-delivery cancellation race independently buildable. It
  # isolates stale confirmation suppression and negotiated-state cleanup.
  umbra_add_embedded_2025_catch2_target(umbra_negotiated_divestiture_pre_delivery_acquisition_cancellation_catch2
    SOURCES
    cpp/tests/negotiated_divestiture_pre_delivery_acquisition_cancellation_catch2.cpp
  )

  # Keep the negotiated Willing-to-Acquire continuation case
  # independently buildable. It isolates superseding regular acquisition
  # plus cancellation before the next candidate's confirmation.
  umbra_add_embedded_2025_catch2_target(umbra_negotiated_willing_to_acquire_continuation_catch2
    SOURCES
    cpp/tests/negotiated_willing_to_acquire_continuation_catch2.cpp
  )

  # Keep the two-dimensional multi-attribute DDM source-isolation case
  # independently buildable. It proves per-attribute source-region
  # overlap, conjunctive range filtering, and restoration.
  umbra_add_embedded_2025_catch2_target(umbra_regional_multi_attribute_ddm_catch2
    SOURCES
    cpp/tests/regional_multi_attribute_ddm_catch2.cpp
  )

  # Keep the default-region routing case independently buildable. It
  # isolates ordinary/default source fallback from explicit regional
  # association and subscriber declarations.
  umbra_add_embedded_2025_catch2_target(umbra_default_region_object_routing_catch2
    SOURCES
    cpp/tests/default_region_object_routing_catch2.cpp
  )

  # Keep the 2025 region-template lifecycle independently buildable.
  # This isolates official create/commit/delete/range-bound semantics
  # from the large federation-management aggregate, whose unrelated
  # source-integrity failures must not hide this focused evidence.
  umbra_add_embedded_2025_catch2_target(umbra_region_lifecycle_catch2
    SOURCES
    cpp/tests/region_lifecycle_catch2.cpp
  )

  # Keep the 2025 Query Attribute Ownership grouping case independently
  # buildable. It isolates owner/unowned result callbacks and queued
  # report nullification from the large federation-management aggregate.
  umbra_add_embedded_2025_catch2_target(umbra_attribute_ownership_query_catch2
    SOURCES
    cpp/tests/attribute_ownership_query_catch2.cpp
  )

  # Keep the ownership-transfer/update-region cleanup case independently
  # buildable. It isolates the former-owner association boundary from
  # the large federation-management aggregate.
  umbra_add_embedded_2025_catch2_target(umbra_ownership_transfer_update_region_catch2
    SOURCES
    cpp/tests/ownership_transfer_update_region_catch2.cpp
  )

  # Keep the 2025 If Available acquisition path independently buildable.
  # It isolates Willing-to-Acquire state and terminal callbacks from the
  # large federation-management aggregate.
  umbra_add_embedded_2025_catch2_target(umbra_attribute_ownership_acquisition_if_available_catch2
    SOURCES
    cpp/tests/attribute_ownership_acquisition_if_available_catch2.cpp
  )

  # Keep the 2025 regular acquisition/release-denial path independently
  # buildable. It isolates the owner-release handshake from the aggregate.
  umbra_add_embedded_2025_catch2_target(umbra_attribute_ownership_acquisition_catch2
    SOURCES
    cpp/tests/attribute_ownership_acquisition_catch2.cpp
    cpp/tests/ieee1516_2025_negotiated_divestiture_cancellation_process_catch2.cpp
    cpp/tests/ieee1516_2025_process_restored_ownership_assumption_catch2.cpp
    cpp/tests/ieee1516_2025_process_restored_ownership_assumption_immediate_catch2.cpp
    cpp/tests/ieee1516_2025_process_pushed_ownership_assumption_save_restore_catch2.cpp
    cpp/tests/ieee1516_2025_process_unconsumed_pushed_ownership_assumption_evoked_catch2.cpp
    cpp/tests/ieee1516_2025_process_pushed_ownership_assumption_evoked_save_restore_catch2.cpp
    cpp/tests/ieee1516_2025_confirm_divestiture_process_assumption_catch2.cpp
  )

  # Keep the 2025 unconditional-divestiture/assumption path independently
  # buildable. It isolates ownership release and candidate filtering from
  # the large federation-management aggregate.
  umbra_add_embedded_2025_catch2_target(umbra_unconditional_attribute_ownership_divestiture_catch2
    SOURCES
    cpp/tests/unconditional_attribute_ownership_divestiture_catch2.cpp
  )

  # Keep the 2025 resign-action unconditional-divestiture path
  # independently buildable from the aggregate federation-management test.
  umbra_add_embedded_2025_catch2_target(umbra_resign_action_unconditional_divestiture_catch2
    SOURCES
    cpp/tests/resign_action_unconditional_divestiture_catch2.cpp
  )

  # Keep the 2025 Connection Lost automatic-unconditional-divestiture
  # path independently buildable from the aggregate test. It isolates
  # the configured forced-resignation disposition and survivor offer.
  umbra_add_embedded_2025_catch2_target(umbra_connection_loss_automatic_unconditional_divestiture_catch2
    SOURCES
    cpp/tests/connection_loss_automatic_unconditional_divestiture_catch2.cpp
  )

  # Keep the 2025 Connection Lost pending-acquisition cancellation path
  # independently buildable from the aggregate test. It isolates stale
  # owner-release suppression and subsequent survivor eligibility.
  umbra_add_embedded_2025_catch2_target(umbra_connection_loss_automatic_cancel_pending_acquisition_catch2
    SOURCES
    cpp/tests/connection_loss_automatic_cancel_pending_acquisition_catch2.cpp
  )

  # Keep the 2025 resign-action pending-acquisition rejection boundary
  # independently buildable from the aggregate test.
  umbra_add_embedded_2025_catch2_target(umbra_resign_action_pending_acquisition_rejection_catch2
    SOURCES
    cpp/tests/resign_action_pending_acquisition_rejection_catch2.cpp
  )

  # Keep the 2025 voluntary resign cancellation path independently
  # buildable. It isolates stale regular-acquisition callbacks from the
  # aggregate federation-management test.
  umbra_add_embedded_2025_catch2_target(umbra_resign_action_cancel_pending_acquisition_catch2
    SOURCES
    cpp/tests/resign_action_cancel_pending_acquisition_catch2.cpp
  )

  # Keep the 2025 negotiated cancellation boundary independently
  # buildable. It covers the stale release and confirmation callbacks
  # selected by a pending negotiated transfer.
  umbra_add_embedded_2025_catch2_target(umbra_resign_action_cancel_negotiated_pending_catch2
    SOURCES
    cpp/tests/resign_action_cancel_negotiated_pending_catch2.cpp
  )

  # Keep the 2025 If Available cancellation boundary independently
  # buildable. It isolates Willing-to-Acquire cleanup and callback
  # suppression from the aggregate federation-management test.
  umbra_add_embedded_2025_catch2_target(umbra_resign_action_cancel_if_available_pending_catch2
    SOURCES
    cpp/tests/resign_action_cancel_if_available_pending_catch2.cpp
  )

  # Keep the 2025 voluntary DELETE_OBJECTS resignation case
  # independently buildable. It isolates the delete-privilege guard and
  # receive-order Remove Object Instance callback.
  umbra_add_embedded_2025_catch2_target(umbra_resign_action_delete_objects_catch2
    SOURCES
    cpp/tests/resign_action_delete_objects_catch2.cpp
  )

  # Keep the 2025 final-federate resignation rule independently
  # buildable. It isolates the §4.12.4 directive-two override and the
  # reusable object-name boundary from the aggregate test.
  umbra_add_embedded_2025_catch2_target(umbra_resign_action_final_federate_catch2
    SOURCES
    cpp/tests/resign_action_final_federate_catch2.cpp
  )

  # Keep the 2025 FDD update-rate metadata lookup independently
  # buildable. It isolates named-rate/default lookup and subscription
  # state projection from the aggregate federation-management source.
  umbra_add_embedded_2025_catch2_target(umbra_update_rate_value_catch2
    SOURCES
    cpp/tests/update_rate_value_catch2.cpp
  )

  # Keep the passive object-attribute subscription case independently
  # buildable. It isolates the 2025 passive-declaration rule from the
  # ordinary and regional discovery/reflection paths.
  umbra_add_embedded_2025_catch2_target(umbra_passive_object_attribute_subscription_catch2
    SOURCES
    cpp/tests/passive_object_attribute_subscription_catch2.cpp
  )

  # Keep the non-timestamped Local Delete Object Instance case
  # independently buildable. It isolates local knowledge removal and
  # the FederateOwnsAttributes/OwnershipAcquisitionPending guards.
  umbra_add_embedded_2025_catch2_target(umbra_local_delete_object_instance_catch2
    SOURCES
    cpp/tests/local_delete_object_instance_catch2.cpp
  )

  # Keep the Local Delete Object Instance service-report regression
  # independently buildable. It verifies the filesystem-backed MOM
  # record shape and switch gating for the accepted transition.
  umbra_add_embedded_2025_catch2_target(umbra_local_delete_object_instance_service_report_catch2
    SOURCES
    cpp/tests/local_delete_object_instance_service_report_catch2.cpp
  )

  # Keep the failed Local Delete Object Instance service-report matrix
  # independently buildable. It checks serial ordering and exact failure
  # vocabulary around the accepted local-forget transition.
  umbra_add_embedded_2025_catch2_target(umbra_local_delete_object_instance_failure_service_report_catch2
    SOURCES
    cpp/tests/local_delete_object_instance_failure_service_report_catch2.cpp
  )

  # Keep the failed Local Delete Object Instance MOM-interaction matrix
  # independently buildable. It decodes the official report contract
  # under HLA_IMMEDIATE without coupling to the aggregate test binary.
  umbra_add_embedded_2025_catch2_target(umbra_local_delete_object_instance_failure_service_report_interaction_catch2
    SOURCES
    cpp/tests/local_delete_object_instance_failure_service_report_interaction_catch2.cpp
  )

  # Keep the regular acquisition-cancellation ownership slice
  # independently buildable. It isolates the confirmation boundary and
  # stale callback suppression from the broader ownership matrix.
  umbra_add_embedded_2025_catch2_target(umbra_attribute_ownership_acquisition_cancellation_catch2
    SOURCES
    cpp/tests/attribute_ownership_acquisition_cancellation_catch2.cpp
  )

  # Keep the owner-denial cancellation race independently buildable. It
  # exercises the callback-boundary arbitration without broad ownership
  # matrix setup.
  umbra_add_embedded_2025_catch2_target(umbra_ownership_acquisition_cancellation_race_catch2
    SOURCES
    cpp/tests/ownership_acquisition_cancellation_race_catch2.cpp
  )

  # Keep the competing transfer cancellation race independently
  # buildable. It proves Divestiture If Wanted wins over a queued
  # cancellation confirmation at the callback boundary.
  umbra_add_embedded_2025_catch2_target(umbra_ownership_acquisition_cancellation_transfer_race_catch2
    SOURCES
    cpp/tests/ownership_acquisition_cancellation_transfer_race_catch2.cpp
  )

  # Keep the ordinary mixed TSO/FQR/TARA/NMRA interaction case
  # independently buildable.  This preserves a focused alternate-time
  # grant lane while the aggregate federation-management source is under
  # recovery.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_interaction_mixed_advance_catch2
    SOURCES
    cpp/tests/timestamped_interaction_mixed_advance_catch2.cpp
  )

  # Keep the cross-producer timestamp-order case independently
  # buildable. It exercises two publisher queues and two constrained
  # recipient queues without depending on the damaged aggregate source.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_interaction_cross_producer_order_catch2
    SOURCES
    cpp/tests/timestamped_interaction_cross_producer_order_catch2.cpp
  )

  # Keep the source-resignation fanout case independently buildable. It
  # proves that already-admitted TSO payloads survive producer departure
  # for each recipient-local queue.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_interaction_source_resignation_fanout_catch2
    SOURCES
    cpp/tests/timestamped_interaction_source_resignation_fanout_catch2.cpp
  )

  # Keep the ordinary interaction re-enable case independently
  # buildable. It checks that a queued TSO passel belongs to the joined
  # federate lifetime rather than one Time Constrained enablement.
  umbra_add_embedded_2025_catch2_target(umbra_timestamped_interaction_reenable_catch2
    SOURCES
    cpp/tests/timestamped_interaction_reenable_catch2.cpp
  )

  # Keep the regional Auto Provide overlap/suppression case independently
  # buildable. It is a recovered source slice from the aggregate test
  # while that translation unit remains under repair.
  umbra_add_embedded_2025_catch2_target(umbra_regional_auto_provide_overlap_catch2
    SOURCES
    cpp/tests/regional_auto_provide_overlap_catch2.cpp
  )

  # Keep the regional Auto Provide provider-response case independently
  # buildable. It covers both callback models and the reflected scoped
  # value produced by the owner callback.
  umbra_add_embedded_2025_catch2_target(umbra_regional_auto_provide_response_catch2
    SOURCES
    cpp/tests/regional_auto_provide_response_catch2.cpp
  )

  # Keep the regional multi-provider Auto Provide transfer case
  # independently buildable. It verifies ownership handoff followed by
  # one request/reflection from each eligible provider in both callback
  # models.
  umbra_add_embedded_2025_catch2_target(umbra_regional_auto_provide_multi_provider_catch2
    SOURCES
    cpp/tests/regional_auto_provide_multi_provider_catch2.cpp
  )

  # Keep the multi-source-region Auto Provide response case independently
  # buildable. It checks source-region separation and one provider request
  # per eligible source attribute in both callback models.
  umbra_add_embedded_2025_catch2_target(umbra_regional_auto_provide_multi_source_catch2
    SOURCES
    cpp/tests/regional_auto_provide_multi_source_catch2.cpp
  )

  # Keep the timestamped regional Auto Provide response case
  # independently buildable. It checks one TSO reflection and retraction
  # lifecycle in both callback models.
  umbra_add_embedded_2025_catch2_target(umbra_regional_auto_provide_timestamped_response_catch2
    SOURCES
    cpp/tests/regional_auto_provide_timestamped_response_catch2.cpp
  )

  # Keep the timestamped Auto Provide admission-fence case independently
  # buildable. It verifies a discovery-time MOM switch mutation suppresses
  # provider admission and that re-enabling permits a fresh TSO discovery.
  umbra_add_embedded_2025_catch2_target(umbra_regional_auto_provide_timestamped_switch_admission_catch2
    SOURCES
    cpp/tests/regional_auto_provide_timestamped_switch_admission_catch2.cpp
  )

  # Keep the timestamped Auto Provide switch-mutation case independently
  # buildable. It verifies an admitted TSO response survives a later
  # switch disable while a re-enabled execution admits fresh discovery.
  umbra_add_embedded_2025_catch2_target(umbra_regional_auto_provide_timestamped_switch_mutation_catch2
    SOURCES
    cpp/tests/regional_auto_provide_timestamped_switch_mutation_catch2.cpp
  )

  # Keep the non-timestamped Auto Provide admission-fence case
  # independently buildable. It verifies a discovery callback can turn
  # the switch off before provider planning, then re-enable fresh work.
  umbra_add_embedded_2025_catch2_target(umbra_regional_auto_provide_switch_admission_catch2
    SOURCES
    cpp/tests/regional_auto_provide_switch_admission_catch2.cpp
  )

  # Keep the regional Auto Provide switch-mutation case independently
  # buildable. It covers MOM switch admission, ownership transfer, and
  # fresh discovery after re-enabling the switch.
  umbra_add_embedded_2025_catch2_target(umbra_regional_auto_provide_switch_mutation_catch2
    SOURCES
    cpp/tests/regional_auto_provide_switch_mutation_catch2.cpp
  )

  # Keep the small FOM declaration-management slice independently
  # buildable. The aggregate target also contains the large federation-
  # management translation unit, so this target gives the bounded
  # §4.5.5 case a focused compile/run path while that source is under
  # development.
  umbra_add_embedded_2025_catch2_target(umbra_fom_declaration_management_catch2
    SOURCES
    cpp/tests/fom_declaration_management_catch2.cpp
  )

  # Keep FOM-composition validation independently buildable. The
  # aggregate 2025 executable also contains the large federation-
  # management translation unit; this target lets Annex C and schema
  # reconciliation lanes run without waiting on unrelated service work.
  umbra_add_embedded_2025_catch2_target(umbra_fom_composer_catch2
    SOURCES
    cpp/tests/libxml2_fom_composer_catch2.cpp
    cpp/tests/federation_registry_composed_fom_catch2.cpp
  )
endif()
