# File size and splitting policy

This policy is for human reviewability and GitHub rendering, not a compiler
restriction. It applies to new code and to any file touched during a change.
It does not require a risky whole-file rewrite of legacy work.

## Limits

The preferred target is the first number. Crossing the review threshold requires
an explicit reason in the change description. The hard limit applies to new
files; existing exceptions are recorded in
[`source-size-policy.json`](source-size-policy.json) and may not grow.

| File category | Preferred | Review at | Hard limit |
| --- | ---: | ---: | ---: |
| Production C++ source | 2,000 | 2,500 | 4,000 |
| C++ header | 1,200 | 2,000 | 3,000 |
| Catch2 test source | 2,500 | 3,500 | 5,000 |
| Markdown documentation | 3,000 | 4,500 | 6,000 |
| CMake entry point | 3,000 | 5,000 | 8,000 |

These are maintainability limits. A file above the preferred target is not
automatically wrong; a cohesive public boundary or generated artifact can be a
valid exception. The guard is run by `python -m tools.ci lint` and prevents
both new hard-limit violations and growth of a recorded legacy exception:

```text
python tools/check_source_file_sizes.py --report
python tools/check_source_file_sizes.py
```

Canonical compliance exports, catalogs, generated sources, vendored headers,
and machine-maintained ledgers are not split solely to satisfy a line count.
They must remain valid as one input to their consumer. Use bounded query tools,
small summaries, or generated views for GitHub navigation instead.

## How to split an iceberg

Split by ownership and behavior, never at an arbitrary line number.

1. Identify the semantic seams with a bounded symbol/test-case inventory.
   Name the proposed files after the owning domain or focused lane.
2. Keep the public API and ABI boundary stable. Public translation units should
   become thin entry points; detailed state machines belong in the owning
   `cpp/src/internal/<domain>` area.
3. Keep IEEE 1516.1-2010 and 2025 implementations separate. A shared helper
   is allowed only when it is public-type-free and version-neutral; each stream
   keeps its own adapter, diagnostics, tests, and include boundary.
4. For Catch2 suites, move complete test cases and their private fixtures as a
   unit. Preserve titles, tags, callback models, focused lanes, and target
   ownership. Update every affected `source_location` and CTest mapping in the
   same change.
5. Build each new translation unit independently, run the exact affected CTest
   selectors, run the relevant lane mapping check, and then expand regression
   coverage when a shared runtime seam changed.
6. Do not mix a file split with behavioral changes. A pure split should have a
   no-behavior-change statement and identical focused-test results.

## Current iceberg queue

The first wave should be staged rather than attempted as one rewrite:

- The 2025 federation-management restore TSO/retraction cases are now isolated
  in `ieee1516_2025_restore_tso_catch2.cpp` (2,694 lines; 17 active tests).
  Their common prelude and federation-management fixtures live in focused
  2025 support headers (1,184 and 489 lines); the 570-line restore helper is
  compiled once in `ieee1516_2025_federation_management_restore_tso_support.cpp`.
  The suite then yielded four adjacent logical-time/lookahead/pending-advance
  restore cases into `ieee1516_2025_restore_time_window_state_catch2.cpp`
  (356 lines, four focused CTests). Four adjacent public callback-rebinding
  cases now live in `ieee1516_2025_restore_pending_time_role_callbacks_catch2.cpp`
  (477 lines, four focused CTests). Four pending time-advance and Flush Queue
  restore cases now live in `ieee1516_2025_restore_pending_request_rescheduling_catch2.cpp`
  (592 lines, four focused CTests); their direct mappings and prior lane
  ownership are preserved. The custom transportation-type control case now
  lives in `ieee1516_2025_custom_transportation_type_controls_catch2.cpp`
  (165 lines, one focused CTest); its exact source pointer is now explicit and
  its eight direct requirement-section pairs are preserved. The original suite
  is now 63,800 lines, 6,487 fewer than its 70,287-line starting point. The
  adjacent directed-interaction Request Retraction mixed-fanout case now lives
  in `ieee1516_2025_request_retraction_directed_interaction_mixed_fanout_catch2.cpp`
  (156 lines, one focused CTest), preserving its 3 direct Section 8.23.3 pairs.
  The regional interaction subscription receive-order case now lives in
  `ieee1516_2025_regional_interaction_subscription_receive_order_catch2.cpp`
  (268 lines, one focused CTest), with both mapped plan rows kept separate.
  The evoked regional interaction send-time source-region snapshot case now
  lives in
  `ieee1516_2025_evoked_regional_interaction_source_region_snapshot_catch2.cpp`
  (119 lines, one focused CTest); its 42 assertions and 9 direct
  requirement-section pairs remain intact. The timestamped regional
  interaction TSO/retraction case now lives in
  `ieee1516_2025_timestamped_regional_interaction_tso_retraction_catch2.cpp`
  (147 lines, one focused CTest); its 68 assertions and 14 direct
  requirement-section pairs remain intact. The regional available/next-message
  advance case now lives in
  `ieee1516_2025_timestamped_regional_interaction_alternate_advances_catch2.cpp`
  (158 lines, one focused CTest); its 8 direct requirement-section pairs are
  preserved. The ordinary TAR/NMR regional TSO-delivery case now lives in
  ieee1516_2025_timestamped_regional_interaction_tar_nmr_catch2.cpp (142
  lines, one focused CTest); its 8 direct requirement-section pairs are
  preserved. The mixed regional TSO case spanning FQR, TARA, and NMRA grants
  now lives in ieee1516_2025_timestamped_regional_interaction_mixed_advances_catch2.cpp
  (168 lines, one focused CTest); its 13 direct requirement-section pairs are
  preserved. The timestamped default-region interaction delivered by Flush
  Queue Request now lives in
  ieee1516_2025_timestamped_default_region_interaction_flush_queue_catch2.cpp
  (115 lines, one focused CTest); its 11 direct requirement-section pairs are
  preserved. The pending regular ownership-acquisition restore case now lives
  in ieee1516_2025_public_ownership_acquisition_restore_catch2.cpp (314 lines,
  177 assertions, one focused CTest); its 13 direct requirement-section pairs
  and both callback models remain intact. Its collision-resistant temporary
  directories, registry scope, and report-file helpers are shared through a
  focused 2025 test-support header. The mixed negotiated-ownership restore
  companion is now isolated in
  ieee1516_2025_public_mixed_negotiated_ownership_restore_catch2.cpp (428 lines,
  273 assertions, one focused CTest); its 19 direct requirement-section pairs
  and both callback models remain intact. The delivered negotiated-confirmation
  restore companion is now isolated in
  ieee1516_2025_public_mixed_delivered_negotiated_ownership_restore_catch2.cpp
  (449 lines, 273 assertions, one focused CTest); its 19 direct pairs and both
  callback models remain intact. The pending negotiated If Available owner-
  confirmation restore case is now isolated in
  ieee1516_2025_public_pending_negotiated_if_available_owner_confirmation_restore_catch2.cpp
  (338 lines, 207 assertions, one focused CTest); its 19 direct pairs and both
  callback models remain intact. The pending regular negotiated owner-
  confirmation restore case is now isolated in
  ieee1516_2025_public_pending_negotiated_owner_confirmation_restore_catch2.cpp
  (346 lines, 223 assertions, one focused CTest); its 18 direct pairs and both
  callback models remain intact. The delivered negotiated owner-confirmation
  restore case is now isolated in
  ieee1516_2025_public_delivered_negotiated_owner_confirmation_restore_catch2.cpp
  (352 lines, 228 assertions, one focused CTest); its 19 direct pairs and both
  callback models remain intact. The delivered negotiated If Available
  confirmation restore case is now isolated in
  ieee1516_2025_public_delivered_negotiated_if_available_owner_confirmation_restore_catch2.cpp
  (350 lines, 223 assertions, one focused CTest); its 19 direct pairs and both
  callback models remain intact. The asymmetric mixed negotiated confirmation
  restore case is now isolated in
  ieee1516_2025_public_asymmetric_mixed_negotiated_confirmation_restore_catch2.cpp
  (449 lines, 274 assertions, one focused CTest); its 19 direct pairs and both
  callback models remain intact. The reverse asymmetric mixed negotiated
  confirmation restore case is now isolated in
  ieee1516_2025_public_reverse_asymmetric_mixed_negotiated_confirmation_restore_catch2.cpp
  (451 lines, 274 assertions, one focused CTest); its 19 direct pairs and both
  callback models remain intact. The pending attribute transportation-type
  restore case is now isolated in
  ieee1516_2025_public_pending_attribute_transportation_type_change_restore_catch2.cpp
  (346 lines, 220 assertions, one focused CTest); its 16 direct pairs and both
  callback models remain intact. The pending interaction transportation-type
  restore case is now isolated in
  ieee1516_2025_public_pending_interaction_transportation_type_change_restore_catch2.cpp
  (311 lines, 196 assertions, one focused CTest); its 16 direct pairs and both
  callback models remain intact. The committed interaction transportation-type
  override restore case is now isolated in
  ieee1516_2025_public_committed_interaction_transportation_type_override_restore_catch2.cpp
  (285 lines, 186 assertions, one focused CTest); its 16 direct pairs and both
  callback models remain intact. The mixed interaction override restore case is
  isolated in ieee1516_2025_public_mixed_interaction_transportation_type_override_restore_catch2.cpp
  (280 lines, 176 assertions, one focused CTest); its 16 direct pairs and both
  callback models remain intact. The public directed by-ownership handoff
  restore case is now isolated in
  ieee1516_2025_public_directed_interaction_ownership_handoff_restore_catch2.cpp
  (403 lines, 264 assertions, one focused CTest); all 21 direct pairs across
  14 clauses, 30 API surfaces, and both callback models remain intact. The
  directed TSO ownership-callback restore case is now isolated in
  ieee1516_2025_public_directed_tso_ownership_callback_restore_catch2.cpp
  (428 lines, 282 assertions, one focused CTest); its 25 direct pairs across
  17 clauses, 39 API surfaces, and both callback models remain intact. The
  public directed TSO post-delivery-resignation case is now isolated in
  ieee1516_2025_public_directed_tso_post_delivery_resignation_restore_catch2.cpp
  (461 lines, 300 assertions, one focused CTest); its 21 direct pairs across
  14 clauses, 36 API surfaces, and both callback models remain intact. The
  directed TSO delivery and Request Retraction case is now isolated in
  ieee1516_2025_public_directed_tso_delivery_retraction_restore_catch2.cpp
  (330 lines, one focused CTest); its 20 direct pairs across 13 clauses and
  36 API surfaces remain intact. The mixed interaction-declaration restore
  case is now isolated in
  ieee1516_2025_public_mixed_interaction_declaration_restore_catch2.cpp
  (237 lines, 146 assertions, one focused CTest); its 8 direct pairs across 7
  clauses, 17 API surfaces, and both callback models remain intact. The
  pending If Available ownership-callback restore case is now isolated in
  ieee1516_2025_public_pending_ownership_if_available_restore_catch2.cpp
  (320 lines, 193 assertions, one focused CTest); its 11 direct pairs across 7
  clauses, 21 API surfaces, and both callback models remain intact. The
  ownership-acquisition cancellation restore case is now isolated in
  ieee1516_2025_public_ownership_acquisition_cancellation_restore_catch2.cpp
  (316 lines, 171 assertions, one focused CTest); its 17 direct pairs across
  10 clauses, 23 API surfaces, and both callback models remain intact. The
  Confirm Divestiture notification restore case is now isolated in
  ieee1516_2025_public_confirm_divestiture_notification_restore_catch2.cpp
  (336 lines, 191 assertions, one focused CTest); its 18 direct pairs across
  11 clauses, 17 API surfaces, and both callback models remain intact. The
  three-recipient Confirm Divestiture fan-out restore case is now isolated in
  ieee1516_2025_public_confirm_divestiture_fanout_restore_catch2.cpp
  (506 lines, one focused CTest); its 18 direct pairs across 11 clauses and 17
  API surfaces remain intact. The 2025 target build, exact CTest, and ownership
  lane mapping check pass. The public post-confirmation resignation
  multi-survivor fan-out case is now isolated in
  ieee1516_2025_public_confirm_divestiture_post_resignation_fanout_restore_catch2.cpp
  (524 lines, one focused CTest); its 23 direct pairs across 14 clauses and 27
  API surfaces remain intact. The plan does not specify its assertion count or
  callback models. The 2025 target build, exact CTest, and ownership lane
  mapping check pass. The public mixed regular and If Available Confirm
  Divestiture fan-out case is now isolated in
  ieee1516_2025_public_confirm_divestiture_mixed_fanout_restore_catch2.cpp
  (460 lines, one focused CTest); its 19 direct pairs across 12 clauses and 18
  API surfaces remain intact. The plan does not specify its assertion count or
  callback models. The 2025 target build, exact CTest, and ownership lane
  mapping check pass. The public resigned-candidate Confirm Divestiture restore
  case is now isolated in
  ieee1516_2025_public_confirm_divestiture_resignation_restore_catch2.cpp
  (425 lines, one focused CTest); its 20 direct pairs across 13 clauses, 26 API
  surfaces, 222 assertions, and both HLA_EVOKED and HLA_IMMEDIATE callback
  models remain intact. The public post-confirmation resignation Confirm
  Divestiture restore case is now isolated in
  ieee1516_2025_public_confirm_divestiture_post_confirmation_resignation_restore_catch2.cpp
  (433 lines, one focused CTest); its 23 direct pairs across 14 clauses, 27 API
  surfaces, 235 assertions, and both HLA_EVOKED and HLA_IMMEDIATE callback
  models remain intact. The 2025 target build, exact CTest, and ownership lane
  mapping check pass. The Divestiture If Wanted notification restore case is
  now isolated in
  ieee1516_2025_public_divestiture_if_wanted_notification_restore_catch2.cpp
  (301 lines, one focused CTest); its 15 direct pairs across 10 clauses and 15
  API surfaces remain intact. The plan does not specify its assertion count or
  callback-model metadata. The 2025 target build, exact CTest, and ownership
  lane mapping check pass. The Request Attribute Ownership Assumption restore
  case is now isolated in
  ieee1516_2025_public_ownership_assumption_search_restore_catch2.cpp (291
  lines, one focused CTest); its 13 direct pairs across 8 clauses and 22 API
  surfaces remain intact. The plan does not specify its assertion count or
  callback-model metadata. The 2025 target build, exact CTest, and ownership
  lane mapping check pass. The public pending object-instance Request
  Attribute Value Update restore case is now isolated in
  ieee1516_2025_public_pending_attribute_value_update_restore_catch2.cpp (263
  lines, one focused CTest); its 17 direct pairs across 8 clauses, 18 API
  surfaces, and 61 assertions remain intact. Callback-model metadata remains
  unspecified. The 2025 target build, exact CTest, and lane mapping check pass.
  The original suite is now 51,993 lines, 18,294 fewer than its 70,287-line
  starting point. The public pending object-class Request Attribute Value
  Update restore case is now isolated in
  ieee1516_2025_public_class_pending_attribute_value_update_restore_catch2.cpp
  (258 lines, one focused CTest); its 12 direct pairs across 6 clauses, 8 API
  surfaces, and 61 assertions remain intact. Callback-model metadata remains
  unspecified. The 2025 target build, exact CTest, and lane mapping check pass.
  The public regional object-class Request Attribute Value Update restore case
  is now isolated in
  ieee1516_2025_public_regional_class_pending_attribute_value_update_restore_catch2.cpp
  (420 lines, one focused CTest); its 11 direct pairs across 6 clauses, 21 API
  surfaces, 392 assertions, and both HLA_EVOKED and HLA_IMMEDIATE callback
  models remain intact. The 2025 target build, exact CTest, and regional lane
  mapping check pass. The delivered receive-order regional provider-response
  no-replay case is now isolated in
  ieee1516_2025_public_regional_provider_response_no_replay_restore_catch2.cpp
  (381 lines, one focused CTest); its 13 direct pairs across 7 clauses and 24
  API surfaces, 187 assertions, and both callback models are recorded. The
  no-replay-after-delivery invariant is explicitly internal development
  evidence. The 2025 target build, exact CTest, and lane mapping check pass.
  The public fresh-registry application-value/report-file-lifetime restore
  case is isolated in ieee1516_2025_public_application_value_restore_catch2.cpp
  (287 lines, one focused CTest); its 9 direct pairs across 5 clauses and 18
  API surfaces remain intact. Assertion-count and callback-model metadata stay
  unspecified. The public fresh-registry timestamped regional attribute-update
  restore case is now isolated in
  ieee1516_2025_public_regional_attribute_update_tso_restore_catch2.cpp (517
  lines, one focused CTest); its 20 direct pairs across 16 clauses and 33 API
  surfaces remain intact, with assertion-count and callback-model metadata
  unspecified. Both focused cases pass the 2025 target build, exact CTest, and
  lane checks. The joined-federate MOM conditional-reflections case is now
  isolated in
  ieee1516_2025_joined_federate_mom_conditional_reflections_catch2.cpp (518
  lines, one focused CTest); its one direct pair to clause 11.4.1 and 23 API
  surfaces remain intact. The plan records 622 assertions and both callback
   models. The public fresh-registry directed TSO restore case is now isolated
   in ieee1516_2025_public_directed_tso_alternate_advances_restore_catch2.cpp
   (517 lines, one focused CTest); its 33 direct pairs across 19 clauses and
   37 API surfaces remain intact, while assertion-count and callback-model
   metadata remain unspecified. The public fresh-registry ownership-assumption
   restore case beside two Confirm Divestiture notifications is now isolated in
   ieee1516_2025_public_confirm_divestiture_assumption_fanout_restore_catch2.cpp
   (514 lines, one focused CTest); its 26 direct pairs across 16 clauses and 27
   API surfaces, 319 assertions, and both callback models remain intact. The
   2025 target build, exact CTest, and ownership-acquisition lane check pass.
   The timestamped regional provider-response case is isolated in
   ieee1516_2025_timestamped_regional_request_provider_response_catch2.cpp
   (194 lines); its 9 direct pairs across 6 clauses and 17 API surfaces,
   60 assertions, and HLA_EVOKED callback model remain intact. The timestamped
   object-class provider-response case is isolated in
   ieee1516_2025_timestamped_object_class_request_provider_response_catch2.cpp
   (183 lines); its 10 direct pairs across 6 clauses and 27 API surfaces,
   97 assertions, and both callback models remain intact. The timed regional
   interaction save/restore case is isolated in
   ieee1516_2025_timed_restore_live_tso_regional_interaction_catch2.cpp
   (123 lines), preserving 18 direct pairs, 24 API surfaces, 55 assertions,
   and HLA_EVOKED. Its focused CTest fails at `report.sentRegionsSupplied`;
   the same 39/40 result was reproduced with the unchanged test in its original
   monolith location, so this is a pre-existing behavior failure, not a split
   regression. No runtime behavior was changed. The timestamped Update
  Attribute Values failure/MOM interaction restored-baseline case was first
  isolated in its own 165-line unit, preserving 11 direct pairs across 9
  clauses, 13 API surfaces, 125 assertions, and HLA_EVOKED/HLA_IMMEDIATE. It
  now shares a 177-line unit and private runner with the original case, while
  retaining separate identities, callback behavior, and mappings. Its exact CTest and focused
   lane mapping check pass. After that split the monolith stood at 47,994
   lines, 22,293 fewer than
   its 70,287-line starting point. The regional Provide Attribute Value Update
   restored-baseline and non-restored cases now share
   `ieee1516_2025_regional_provide_attribute_value_update_service_report_catch2.cpp`
   (174 lines) through one 141-line private scenario runner. Their Catch2 test
   identities, lanes, and mapped plan rows remain separate. Both exact CTests
   and both lane checks pass; the monolith stood at 47,694 lines.
   The original and restored-baseline regional Request Attribute Value Update
   filtering tests now share
   `ieee1516_2025_regional_request_attribute_value_update_filtering_catch2.cpp`
   (197 lines) through one private scenario runner. The runner retains their
   distinct requester-publication and callback-absence checks while both test
   names and mapping rows remain separate. Their mappings preserve 10 direct
   pairs across 6 clauses/2 APIs and 10 pairs across 7 clauses/20 APIs,
   respectively; both exact CTests and the shared lane check pass. The
   original and restored-baseline regional provider-response cases now share
   one 172-line focused 2025 unit and private runner while preserving their
   separate test identities and mappings (6 direct pairs across 3 clauses and
   9 APIs per row); the 2025 target build, both exact CTests, and lane check
   pass. The original and restored-baseline timestamped regional
   Update Attribute Values failure/MOM cases now share one 221-line focused
   2025 unit and private scenario runner, preserving both 138-assertion test
   identities and plan rows. Their callback models are HLA_EVOKED and
   HLA_IMMEDIATE; the original mapping retains 5 direct pairs across 3 clauses
   and 7 APIs, while the restored-baseline mapping retains 11 pairs across 9
   clauses and 30 APIs. The 2025 target build, both exact CTests, and the lane
   check pass (3 mapped cases, none unmapped). The federation-management
   monolith is now 45,843 lines, 24,444 fewer than its 70,287-line starting
   point, with its ceiling ratcheted to 45,843. The original and
   restored-baseline timestamped Update Attribute Values file-report failure
   cases now share a private runner in
   `ieee1516_2025_timestamped_update_attribute_values_failure_file_catch2.cpp`
   (190 lines), while retaining separate test identities, 218/236 assertions,
   and independent 11-pair/9-clause/15-API mappings. The original case alone
   checks that disabled file output does not append and re-enabling preserves
   the same file. The original and restored-baseline timestamped regional
   Update Attribute Values file-report cases now share a private runner in
   `ieee1516_2025_timestamped_regional_update_file_report_catch2.cpp` (203
   lines). Both exact CTests pass; the baseline retains 206 assertions, and
   the original-only receiver-disconnect difference remains explicit. Their
   independent mappings (1 pair/1 clause/9 APIs and 11 pairs/9 clauses/30
   APIs) and Catch2 identities are preserved. The accepted timestamped
   regional Update Attribute Values MOM-interaction case is now isolated in
   `ieee1516_2025_timestamped_regional_update_mom_interaction_catch2.cpp`
   (247 lines), preserving its 5 direct pairs across 3 clauses/10 APIs, exact
   Catch2 identity, and native HLA_IMMEDIATE observer/callback ordering. The
   accepted ordinary regional Update Attribute Values MOM-interaction case
   is now isolated in
   `ieee1516_2025_ordinary_regional_update_mom_interaction_catch2.cpp` (205
   lines), preserving 96 assertions and 12 direct pairs across 10 sections/31
   APIs. Its HLA_IMMEDIATE observer and HLA_EVOKED receiver retain the
    report-before-callback ordering. The federation-management monolith is now
    44,617 lines, 25,670 below its 70,287-line start, with its ceiling
    ratcheted to 44,617. The failed ordinary regional Update Attribute Values
    file-report case is isolated in
    `ieee1516_2025_ordinary_regional_update_failure_file_catch2.cpp` (149
    lines), preserving its one direct requirement pair in clause 11.5 and 7 API
    surfaces. Its paired failed ordinary regional Update Attribute Values
    MOM-interaction case is isolated in
    `ieee1516_2025_ordinary_regional_update_failure_mom_catch2.cpp` (199 lines),
    preserving 119 assertions and 4 direct pairs across clauses 6.10/11.5 with
    6 API surfaces. The timestamped Send Interaction failure MOM-interaction
    case is isolated in
    `ieee1516_2025_timestamped_send_interaction_failure_mom_catch2.cpp` (166
    lines), preserving 5 direct pairs across 4 clauses and 3 API surfaces; its
    focused 2025 build, exact CTest, and mapping check pass. The accepted
    timestamped Update Attribute Values file-report case is now isolated in
    `ieee1516_2025_timestamped_update_attribute_values_service_report_catch2.cpp`
    (132 lines), preserving its 4 direct pairs across 4 clauses and 4 API
    surfaces and report-before-reflection callback ordering. The accepted
    ordinary regional Update Attribute Values file-report case is isolated in
    `ieee1516_2025_ordinary_regional_update_service_report_file_catch2.cpp`
    (145 lines), preserving 139 assertions, its direct clause 11.5 mapping,
    and 8 API surfaces. The failed timestamped Send Directed Interaction
    file-report case is isolated in
    `ieee1516_2025_timestamped_directed_interaction_failure_file_catch2.cpp`
    (179 lines), preserving 313 assertions, 5 direct pairs across clauses
    6.13/8.22.3/11.5/11.5.2, and 3 API surfaces, including the file-switch
    append gate and serial continuity. The failed timestamped Send Directed
    Interaction MOM-interaction case is now isolated in
    `ieee1516_2025_timestamped_directed_interaction_failure_mom_route_catch2.cpp`
    (217 lines), preserving 215 assertions, 6 direct pairs across clauses
    6.13/8.22.3/11.5/11.5.1, and 8 API surfaces. Its observer-subscription
    and no-recipient behavior remains distinct from the file-switch-cycle
    case. The timestamped Send Interaction failure/file-switch case is now
    isolated in
    `ieee1516_2025_timestamped_send_interaction_failure_file_catch2.cpp`
    (147 lines), preserving 242 assertions, 5 direct pairs across clauses
    6.13/8.22.3/11.5/11.5.2, and 3 API surfaces. The accepted Publish Object
    Class Attributes argument case is now isolated in
    `ieee1516_2025_publish_object_class_attributes_service_report_file_catch2.cpp`
    (130 lines), preserving 149 assertions, 2 direct pairs in clause 11.5.1,
    and 1 API surface. The restored-baseline Unpublish Object Class Attributes
    case is now isolated in
    `ieee1516_2025_unpublish_object_class_attributes_restored_baseline_catch2.cpp`
    (94 lines), preserving 102 assertions, 7 direct pairs across clauses 5.3,
    11.5, 11.5.1, 11.5.2, and 11.5.2.1, and 1 API surface. The 2025 target
    rebuilt; its exact CTest and the distinct whole-class Unpublish CTest
    passed, and both plan traces resolve to their intended tests. The
    distinct whole-class Unpublish case is now isolated in
    `ieee1516_2025_unpublish_object_class_attributes_whole_class_catch2.cpp`
    (121 lines), preserving 7 direct pairs across clauses 5.3, 11.5, 11.5.1,
    11.5.2, and 11.5.2.1 with both API surfaces. Its exact CTest and the
    restored-baseline Unpublish CTest pass; both plan traces point to their
    separate focused tests. The stale regional-subscription source pointer was
    corrected to its actual test declaration. The source-size policy, JSON
    parsing, and diff checks pass. The Subscribe Object Class Attributes
    argument case is now isolated in
    `ieee1516_2025_subscribe_object_class_attributes_service_report_file_catch2.cpp`
    (100 lines), preserving 9 direct pairs across clauses 5.8, 11.5, 11.5.1,
    11.5.2, and 11.5.2.1, and 1 API surface. The 2025 target rebuilt; exact
    CTests passed for Subscribe, adjacent Unsubscribe, and whole-class
    Unpublish; the Subscribe trace resolves to its focused plan row. The
    source-size policy, JSON parsing, and diff checks pass. The Unsubscribe
    Object Class Attributes argument case is now isolated in
    `ieee1516_2025_unsubscribe_object_class_attributes_service_report_file_catch2.cpp`
    (100 lines), preserving 7 direct pairs across clauses 5.9.4, 11.5,
    11.5.1, 11.5.2, and 11.5.2.1 with both API surfaces. Its exact CTest,
    adjacent Publish Directed Interactions CTest, and mapped trace pass.
    The Publish Object Class Directed Interactions argument case is now
    isolated in
    `ieee1516_2025_publish_object_class_directed_interactions_service_report_file_catch2.cpp`
    (118 lines), preserving 8 direct pairs across clauses 5.6.3, 11.5,
    11.5.1, 11.5.2, and 11.5.2.1 with 1 API surface. Its exact CTest,
    adjacent Unpublish Directed Interactions CTest, and mapped trace pass.
    The Unpublish Object Class Directed Interactions argument case is now
    isolated in
    `ieee1516_2025_unpublish_object_class_directed_interactions_service_report_file_catch2.cpp`
    (131 lines), preserving 10 direct pairs across clauses 5.7.4, 11.5,
    11.5.1, 11.5.2, and 11.5.2.1 with both API surfaces. Its exact CTest,
    adjacent Subscribe Directed Interactions CTest, and mapped trace pass.
    The Subscribe Object Class Directed Interactions argument case is now
    isolated in
    `ieee1516_2025_subscribe_object_class_directed_interactions_service_report_file_catch2.cpp`
    (123 lines), preserving 10 direct pairs across clauses 5.12, 11.5,
    11.5.1, 11.5.2, and 11.5.2.1 with 1 API surface. Its exact CTest,
    adjacent Unsubscribe Directed Interactions CTest, and mapped trace pass.
    The Unsubscribe Object Class Directed Interactions argument case is now
    isolated in
    `ieee1516_2025_unsubscribe_object_class_directed_interactions_service_report_file_catch2.cpp`
    (131 lines), preserving 11 direct pairs across clauses 5.13, 11.5,
    11.5.1, 11.5.2, and 11.5.2.1 with both API surfaces. Its exact CTest,
    adjacent Unpublish Interaction Class CTest, and mapped trace pass.
    The Unpublish Interaction Class service-report case is now isolated in
    `ieee1516_2025_unpublish_interaction_class_service_report_file_catch2.cpp`
    (80 lines), preserving its 6 direct pairs across clauses 5.5, 11.5,
    11.5.1, 11.5.2, and 11.5.2.1 with 1 API surface. The Unsubscribe
    Interaction Class service-report case is now isolated in
    `ieee1516_2025_unsubscribe_interaction_class_service_report_file_catch2.cpp`
    (73 lines), preserving 6 direct pairs across clauses 5.11, 11.5, 11.5.1,
    11.5.2, and 11.5.2.1 with 1 API surface. The 2025 target built; the exact
    Unsubscribe CTest and adjacent regional-advisory CTest passed, and its
    requirement trace resolves to the focused test. The federation-management
    monolith was then 42,000 lines, 28,287 below its 70,287-line start, with its
    ceiling ratcheted to 42,000. Three duplicate declaration-advisory 2025
    definitions were removed in separate, test-mapped lanes. The regional
    focused case retains 61
    assertions, 13 direct pairs across 7 sections, and 9 API surfaces. The
    ordinary focused case retains 91 assertions, 15 direct pairs across 7
    sections, and 8 API surfaces, including explicit suppression while the
    advisory switch is disabled. Both 2025 test targets built; the retained
    CTests passed and each now has one registration. Their traces resolve to
    the focused source files, the ordinary lane gate passes, and the two plan
    source pointers now name those focused files. The declaration-advisory
    service-report case retains its 143 assertions, 6 direct pairs across 6
    sections, and 4 API surfaces in the focused unit; its retained CTest passes
    and the trace points only to that source. The mapped Discover Object
    Instance service-report case is now isolated in
    `discover_object_instance_service_report_catch2.cpp` (139 lines, 33
    assertions, 3 direct pairs across clauses 6.9.3, 11.5, and 11.5.2, and 4
    API surfaces). The receive-order Remove Object Instance case is isolated
    in `remove_object_instance_service_report_catch2.cpp` (126 lines, 29
    assertions, 9 direct pairs across clauses 6.17.1, 11.5, and 11.5.2, and 5
    API surfaces). The timestamped Remove Object Instance case is now isolated
    in `timestamped_remove_object_instance_service_report_catch2.cpp` (234
    lines, 73 assertions, 10 direct pairs across clauses 6.17.1, 11.5, and
    11.5.2, and 8 API surfaces). The three focused removal/discovery CTests
    passed, as did the adjacent static Advisories Use Known Class regression;
    both focused and aggregate 2025 targets built. The timestamped service-
    report case has one CTest registration and its lane trace/check resolve to
    the focused source. The composed-FDD advisory seed case is now isolated in
    `join_advisory_switch_seed_catch2.cpp` (45 lines, 16 assertions, 13 direct
    pairs across 8 standard sections, and 8 API surfaces); its focused CTest
    passes with exactly one registration, and its source trace and lane check
    resolve. The static Advisories Use Known Class test is now isolated in
    `advisories_use_known_class_switch_catch2.cpp` (45 lines, 15 assertions,
    2 direct pairs in IEEE 1516.1-2025 clause 10.55.1, and 1 API surface).
    Its focused CTest passes exactly once; the focused and aggregate targets
    build, and the adjacent support-switch regression passes in the aggregate.
    Its lane trace resolves to the focused source. The stale monolith source
    pointer was corrected, and the previous advisory-seed row's duplicate
    source pointer was removed. The support-switch regression is now isolated
    in `support_switch_state_catch2.cpp` (86 lines, 40 assertions, 8 direct
    pairs across 8 IEEE 1516.1-2025 sections, and 12 API surfaces); its
    callback-model classification remains unspecified. The plan narrative now
    distinguishes mapped public switch behavior from fixture setup and removes
    stale HLAreportServiceFile catalog-only claims. The focused CTest passes
    exactly once, the lane check resolves the focused source, both focused and
    aggregate targets build, and the adjacent HLAreportServiceInvocation
    observer regression passes in the aggregate. The Join-time explicit FOM
    NoAction automatic-resign case is isolated in
    `join_automatic_resign_directive_catch2.cpp` (27 lines, 7 HLA_EVOKED
    assertions, 9 direct pairs across four IEEE 1516.1-2025 and two
    IEEE 1516.2-2025 clauses, and 7 API surfaces). Its focused target builds,
    its CTest passes exactly once, and the lane check resolves one roadmap item
    and one mapped case. The nine direct pairs remain separated by standard.
    The failed regional object-attribute association file-report case is now
    isolated in `ieee1516_2025_ddm_regional_association_failure_file_catch2.cpp`
    (230 lines, 302 assertions, 8 direct IEEE 1516.1-2025 pairs across clauses
    9.6, 9.7.5, 11.5, and 11.5.2, and 7 API surfaces). Its file-destination
    behavior remains distinct from the mapped MOM-interaction sibling. The
    focused and aggregate 2025 targets build, the lane check resolves one
    roadmap item and two mapped cases, and all post-extraction source pointers
    are adjusted. The shared CMake helper serves thirteen focused targets;
    `CMakeLists.txt` remains 12,392 lines (ceiling 12,392). The receive-order
    regional asynchronous delivery case is isolated in
    `ieee1516_2025_ddm_regional_async_delivery_catch2.cpp` (131 lines, 51
    assertions, 9 direct requirement-section pairs across 7 IEEE 1516.1-2025
    clauses, and 11 official API surfaces). Its focused and aggregate tests
    plus two neighboring async/regional tests pass (4 exact CTests). The
    timestamped regional interaction Time Constrained re-enable case is now
    isolated in
    `ieee1516_2025_timestamped_regional_interaction_reenable_catch2.cpp` (136
    lines, 60 assertions, 11 direct requirement-section pairs across 9 IEEE
    1516.1-2025 clauses, and 29 official API surfaces). The focused and
    aggregate targets build; its focused and aggregate tests plus the mapped
    TSO seed, time-role, and default-region neighbors pass (5 exact CTests).
    The Flush Queue Request case is isolated in
    `ieee1516_2025_timestamped_regional_interaction_flush_queue_context_catch2.cpp`
    (120 lines, 51 assertions, 12 direct requirement-section pairs across 7
    IEEE 1516.1-2025 clauses, and 24 official API surfaces). The focused and
    aggregate targets build; its focused and aggregate CTests plus the TSO
    seed, Time Constrained re-enable, and default-region FQR neighbors pass
    (5 exact CTests). All three mapping lanes pass. The timestamped regional
    callback-suppression case is isolated in
    `ieee1516_2025_regional_tso_suppressed_callback_catch2.cpp` (102 lines,
    31 assertions, 3 direct requirement-section pairs across clauses 8.22.3,
    8.23.3, and 9.12.5, and 23 official API surfaces). Its focused and
    aggregate targets build; its focused and aggregate CTests plus the TSO
    seed, Flush Queue, and Time Constrained re-enable neighbors pass (5 exact
    CTests), and its mapping lane passes. The federation-management monolith
    is 41,096 lines, 29,191 below its 70,287-line start, with its ceiling
    ratcheted down to 41,096. No runtime
    behavior changed, no Requirements Lab resync occurred, and no IEEE
    1516.1-2010 path or mapping changed.
    Next handoff: independently map `Embedded timestamped Send Interaction
    With Regions returns a retraction designator without overlap-qualified
    recipients` at `ieee1516_2025_federation_management_catch2.cpp:33468`; keep
    no-overlap-at-send behavior distinct from callback-time suppression and
    delivered-recipient retraction.
- The first 2025 federation-management service-report lane is isolated in
  `ieee1516_2025_service_report_catch2.cpp` (2,452 lines), and the process
  service-report lifecycle block is isolated in
  `ieee1516_2025_connection_service_report_catch2.cpp` (2,346 lines).
  The synchronization/restore lifecycle block is now isolated in
  `ieee1516_2025_connection_synchronization_restore_catch2.cpp` (2,123 lines).
  The federation-save lifecycle block is now isolated in
  `ieee1516_2025_connection_federation_save_catch2.cpp` (2,448 lines).
  The paired explicit synchronization/failure callback cases are now isolated
  in `ieee1516_2025_connection_synchronization_failure_catch2.cpp` (896
  lines).
  The initial GALT/LITS and time-management cases are now isolated in
  `ieee1516_2025_connection_time_management_catch2.cpp` (738 lines).
  The object-registration and attribute-declaration cases are now isolated in
  `ieee1516_2025_connection_object_registration_catch2.cpp` (826 lines).
  The default and named update-rate query cases are now isolated in
  `ieee1516_2025_connection_update_rate_catch2.cpp` (731 lines).
  The malformed-address connection case is now isolated in
  `ieee1516_2025_connection_address_validation_catch2.cpp` (475 lines).
  The local-delete object-management case is now isolated in
  `ieee1516_2025_connection_local_delete_catch2.cpp` (799 lines).
  The named object-registration case is now isolated in
  `ieee1516_2025_connection_named_object_registration_catch2.cpp` (873 lines).
  The public Send Interaction case is now isolated in
  `ieee1516_2025_connection_send_interaction_catch2.cpp` (703 lines).
  The interaction/parameter handle-lookup case is now isolated in
  `ieee1516_2025_connection_handle_lookup_catch2.cpp` (600 lines).
  The single-recipient directed-interaction case is now isolated in
  `ieee1516_2025_connection_directed_interaction_catch2.cpp` (819 lines).
  The multi-recipient directed-interaction case is now isolated in
  `ieee1516_2025_connection_directed_interaction_multi_recipient_catch2.cpp` (990 lines).
  The ordinary interaction-declaration case is now isolated in
  `ieee1516_2025_connection_interaction_declaration_catch2.cpp` (623 lines).
  The untimestamped Evoke callback interaction case is now isolated in
  `ieee1516_2025_connection_receive_evoke_catch2.cpp` (787 lines).
  The timestamped Evoke interaction case is now isolated in
  `ieee1516_2025_connection_timestamped_receive_evoke_catch2.cpp` (785 lines).
  The public Update Attribute Values case is now isolated in
  `ieee1516_2025_connection_update_attribute_values_catch2.cpp` (639 lines).
  The overlapping-subscription reflection case is now isolated in
  `ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp` (889 lines).
  The region-lifecycle and regional-registration case is now isolated in
  `ieee1516_2025_connection_region_lifecycle_catch2.cpp` (614 lines).
  The remote regional-subscription and scoped-update case is now isolated in
  `ieee1516_2025_connection_regional_subscription_update_catch2.cpp` (894 lines).
  The timestamped regional-update case is now isolated in
  `ieee1516_2025_connection_timestamped_regional_update_catch2.cpp` (971 lines).
  The regional-unsubscription and scope-transition case is now isolated in
  `ieee1516_2025_connection_regional_unsubscribe_catch2.cpp` (1005 lines).
  The disjoint regional-update suppression case is now isolated in
  `ieee1516_2025_connection_disjoint_regional_update_catch2.cpp` (811 lines).
  The public object-class subscription case is now isolated in
  `ieee1516_2025_connection_public_object_subscription_catch2.cpp` (653 lines).
  The regional Attribute Relevance Advisory case is now isolated in
  `ieee1516_2025_connection_regional_attribute_relevance_catch2.cpp` (894 lines).
  The directed-interaction callback case is now isolated in
  `ieee1516_2025_connection_directed_interaction_callback_catch2.cpp` (1077 lines).
  The timestamped process-attribute-before-grant case is now isolated in
  `ieee1516_2025_connection_tso_attribute_before_grant_catch2.cpp` (828 lines).
  The configured-endpoint Attribute Relevance Advisory case is now isolated in
  `ieee1516_2025_connection_attribute_relevance_advisory_catch2.cpp` (916 lines).
  The receive-order Delete Object Instance case is now isolated in
  `ieee1516_2025_connection_delete_object_instance_catch2.cpp` (844 lines).
  The timestamped Delete Object Instance case is now isolated in
  `ieee1516_2025_connection_timestamped_delete_object_instance_catch2.cpp` (1043 lines).
  The regional Attribute Relevance Advisory subscription-transition case is now
  isolated in `ieee1516_2025_connection_regional_attribute_relevance_subscription_transition_catch2.cpp` (864 lines).
  The initial regional Attribute Relevance Advisory-after-discovery case is now
  isolated in `ieee1516_2025_connection_regional_attribute_relevance_initial_discovery_catch2.cpp` (799 lines).
  The Attribute Relevance Advisory rate-reissue case is now isolated in
  `ieee1516_2025_connection_attribute_relevance_rate_reissue_catch2.cpp` (1039 lines).
  The process-boundary object-discovery case is now isolated in
  `ieee1516_2025_connection_object_discovery_catch2.cpp` (842 lines).
  The process-time-advance rejection case is now isolated in
  `ieee1516_2025_connection_time_advance_rejection_catch2.cpp` (550 lines).
  The time-regulation-pending rejection case is now isolated in
  `ieee1516_2025_connection_time_regulation_pending_catch2.cpp` (552 lines).
  The time-constrained-pending rejection case is now isolated in
  `ieee1516_2025_connection_time_constrained_pending_catch2.cpp` (551 lines).
  The malformed logical-time encoding case and its fixture are now isolated in
  `ieee1516_2025_connection_time_advance_malformed_encoding_catch2.cpp` (557 lines).
  The deferred federation-scheduler case is now isolated in
  `ieee1516_2025_connection_time_advance_federation_scheduler_catch2.cpp` (674 lines).
  The deferred timestamped process-interaction case is now isolated in
  `ieee1516_2025_connection_tso_interaction_before_grant_catch2.cpp` (911 lines).
  The Query Lookahead process-boundary case is now isolated in
  `ieee1516_2025_connection_query_lookahead_catch2.cpp` (555 lines).
  The focused Modify Lookahead process-boundary case is now isolated in
  `ieee1516_2025_connection_modify_lookahead_catch2.cpp` (565 lines).
  The two-federate deferred lower-lookahead grant case is now isolated in
  `ieee1516_2025_connection_modify_lookahead_grant_catch2.cpp` (691 lines).
  The focused Time Advance Request Available case is now isolated in
  `ieee1516_2025_connection_time_advance_available_catch2.cpp` (555 lines).
  The no-queued-message Next Message Request/Available case is now isolated in
  `ieee1516_2025_connection_time_advance_next_message_catch2.cpp` (567 lines),
  separate from queued timestamped-message behavior.
  The queued timestamped-message Next Message case is now isolated in
  `ieee1516_2025_connection_time_advance_next_message_queued_tso_catch2.cpp`
  (900 lines), preserving its HLA_IMMEDIATE and HLA_EVOKED sections.
  The object-instance name/handle lookup case is now isolated in
  `ieee1516_2025_connection_object_instance_lookup_catch2.cpp` (643 lines),
  preserving both callback-model sections.
  The reverse-FOM object/interaction class name lookup case is now isolated in
  `ieee1516_2025_connection_reverse_fom_lookup_catch2.cpp` (646 lines),
  preserving the federation-owned FOM catalog boundary and both callback-model
  sections.
  The attribute/parameter reverse-lookup case is now isolated in
  `ieee1516_2025_connection_attribute_parameter_lookup_catch2.cpp` (677 lines),
  preserving the federation-owned FOM catalog boundary and both callback-model
  sections.
  The dimension/transportation reverse-lookup case is now isolated in
  `ieee1516_2025_connection_dimension_transportation_lookup_catch2.cpp`
  (617 lines), preserving the federation-owned FOM catalog boundary and both
  callback-model sections.
  The reverse-FOM dimension/transportation error matrix is now isolated in
  `ieee1516_2025_connection_reverse_fom_error_matrix_catch2.cpp` (624 lines),
  separate from the successful lookup baseline and preserving both
  callback-model sections.
  The available-FOM-dimensions hierarchy case is now isolated in
  `ieee1516_2025_connection_available_dimensions_hierarchy_catch2.cpp`
  (633 lines), preserving inherited dimensions, empty dimension sets, invalid
  class-handle checks, and both callback-model sections.
  The ordinary multi-recipient interaction FIFO case is now isolated in
  `ieee1516_2025_connection_multi_recipient_interaction_ordering_catch2.cpp`
  (906 lines), preserving the sender/two-receiver process fixture, per-recipient
  ordering assertions, and both callback-model sections.
  The multi-recipient regional interaction/Relaxed-DDM case is now isolated in
  `ieee1516_2025_connection_multi_recipient_regional_interaction_catch2.cpp`
  (1,185 lines), preserving regional scope, touching-range filtering, source
  metadata, and both callback-model sections.
  The timestamped regional interaction case is now isolated in
  `ieee1516_2025_connection_tso_regional_interaction_catch2.cpp` (1,019 lines),
  preserving timestamp/retraction reconstruction, time-advance gating,
  source-region metadata, and its HLA_EVOKED section.
  The timestamped directed-interaction callback-gating case is now isolated in
  `ieee1516_2025_connection_tso_directed_interaction_callback_gating_catch2.cpp`
  (1,007 lines), preserving callback-disabled delivery, directed routing,
  time-advance gating, and its HLA_EVOKED section.
  The object-instance Request Attribute Value Update process-boundary case is
  now isolated in `ieee1516_2025_connection_request_attribute_value_update_catch2.cpp`
  (983 lines), preserving Provide callback delivery, discovery/reflection,
  request tags, and both callback-model sections.
  The regional class Request Attribute Value Update process-boundary case is now
  isolated in
  `ieee1516_2025_connection_regional_class_request_attribute_value_update_catch2.cpp`
  (1,011 lines), preserving region selection, discovery/reflection,
  sent-region metadata, and both callback-model sections.
  The public invalid-regional-selector Request Attribute Value Update matrix
  is now isolated in
  `ieee1516_2025_connection_regional_class_request_attribute_value_update_invalid_region_catch2.cpp`
  (765 lines), preserving typed exception translation and its invalid-region
  process-boundary scenarios.
  The configured process-endpoint default-region registration/generated-name
  case is now isolated in
  `ieee1516_2025_connection_default_region_registration_catch2.cpp`
  (715 lines), preserving empty-region registration, generated-name uniqueness,
  and both callback-model sections.
  The remote getTimeFactory process-boundary case is now isolated in
  `ieee1516_2025_connection_get_time_factory_catch2.cpp` (565 lines),
  preserving selected logical-time factory reconstruction, remote join setup,
  and its HLA_EVOKED section.
  The process-owned handle-normalization case is now isolated in
  `ieee1516_2025_connection_handle_normalization_catch2.cpp` (663 lines),
  preserving execution-scoped normalization, typed invalid-handle gates, and
  its HLA_EVOKED section.
  The single-endpoint Flush Queue Request/grant case is now isolated in
  `ieee1516_2025_connection_flush_queue_catch2.cpp` (557 lines), preserving
  actual/optimistic grant values, Query Logical Time, and its HLA_EVOKED
  callback path.
  The federate identity lookup case is now isolated in
  `ieee1516_2025_connection_federate_identity_lookup_catch2.cpp` (655 lines),
  preserving active-name resolution, retained federate-name lookup after
  resignation, and the inactive-name exception gate.
  The HLA_IMMEDIATE missing-save Federation Restore failure callback case is
  now isolated in
  `ieee1516_2025_connection_restore_request_failure_immediate_catch2.cpp`
  (566 lines), preserving the request/result path, callback count,
  no-success assertion, and label/order checks.
  The HLA_IMMEDIATE Federation Restore failure lifecycle case is now isolated
  in `ieee1516_2025_connection_restore_failure_lifecycle_immediate_catch2.cpp`
  (606 lines), preserving save/restore callbacks, ordered failure delivery,
  failure reason, and callback-order checks.
  The HLA_IMMEDIATE Federation Restore success lifecycle case is now isolated
  in `ieee1516_2025_connection_restore_success_lifecycle_immediate_catch2.cpp`
  (604 lines), preserving save/restore callbacks, ordered success delivery,
  and callback-order checks.
  The HLA_IMMEDIATE Federation Restore abort case is now isolated in
  `ieee1516_2025_connection_restore_abort_immediate_catch2.cpp` (604 lines),
  preserving save/restore callbacks, ordered abort delivery, the
  `RESTORE_ABORTED` reason, and callback-order checks.
  The embedded configuration-fallback case is now isolated in
  `ieee1516_2025_connection_configuration_fallback_catch2.cpp` (484 lines),
  preserving absent/unknown configuration fallback, `SETTINGS_IGNORED`
  results, and no-RtiAddress coverage.
  The embedded additional-settings parse-failure case is now isolated in
  `ieee1516_2025_connection_configuration_additional_settings_catch2.cpp`
  (481 lines), preserving `SETTINGS_FAILED_TO_PARSE`, the result message, and
  no-RtiAddress/configuration coverage.
  The process-boundary order-type lookup case is now isolated in
  `ieee1516_2025_connection_process_order_type_lookup_catch2.cpp` (580 lines),
  preserving fixed Receive/TimeStamp support, invalid-name/type guards, and
  process lifecycle coverage.
  The process Flush Queue/GALT frontier case is now isolated in
  `ieee1516_2025_connection_process_flush_queue_galt_frontier_catch2.cpp`
  (895 lines), preserving queued timestamp-7 delivery, actual/optimistic
  frontier reporting, and two-federate lifecycle coverage.
  The process multiple-record Flush Queue case is now isolated in
  `ieee1516_2025_connection_process_flush_queue_multiple_records_catch2.cpp`
  (971 lines), preserving FIFO timestamps, future-frontier suppression,
  pre-frontier retraction, and both callback models.
  The immediate-callback process Query GALT/LITS undefined-bound case is now
  isolated in
  `ieee1516_2025_connection_process_query_time_bounds_immediate_catch2.cpp`
  (569 lines), preserving the time-regulation callback and unchanged initial
  logical-time values.
  The two-federate process Query GALT/LITS available-bound case is now
  isolated in
  `ieee1516_2025_connection_process_query_time_bounds_multi_federate_catch2.cpp`
  (666 lines), preserving self-exclusion, observer lookahead bounds, and
  available-result encoding.
  The queued-TSO process Query GALT/LITS case is now isolated in
  `ieee1516_2025_connection_process_query_time_bounds_queued_tso_catch2.cpp`
  (752 lines), preserving queued timestamp LITS, undefined GALT after
  regulator disable, and role callbacks.
  The zero-lookahead process Query GALT/LITS case is now isolated in
  `ieee1516_2025_connection_process_query_time_bounds_zero_lookahead_catch2.cpp`
  (665 lines), preserving exclusive lower-bound observations and time-advance
  grant callbacks.
  The timestamped process attribute retraction-before-callback case is now
  isolated in
  `ieee1516_2025_connection_process_tso_attribute_retraction_before_callback_catch2.cpp`
  (896 lines), preserving queued timestamp suppression and HLA_EVOKED callback
  behavior.
  The two-recipient fanout process TSO retraction case is now isolated in
  `ieee1516_2025_connection_process_tso_attribute_retraction_fanout_catch2.cpp`
  (1113 lines), preserving FIFO timestamps 5 and 7, suppression of the
  retracted middle timestamp 6, and HLA_EVOKED callbacks.
  The regional two-recipient process TSO retraction case is now isolated in
  `ieee1516_2025_connection_process_tso_attribute_retraction_regional_catch2.cpp`
  (1276 lines), preserving committed region scope, FIFO timestamps 5 and 7,
  suppression of the retracted middle timestamp 6, and HLA_EVOKED callbacks.
  The time-regulation re-enable process TSO retraction case is now isolated in
  `ieee1516_2025_connection_process_tso_attribute_retraction_reenable_catch2.cpp`
  (847 lines), preserving the producer-owned retraction designator across
  disable/re-enable and HLA_EVOKED callbacks.
  The changed-lookahead process timestamped attribute update case is now
  isolated in
  `ieee1516_2025_connection_process_tso_attribute_update_regulation_reenable_changed_lookahead_catch2.cpp`
  (968 lines), preserving the update across time-regulation disable/re-enable,
  lookahead-three bounds, and HLA_EVOKED callbacks.
  The changed-lookahead process timestamped interaction case is now isolated in
  `ieee1516_2025_connection_process_tso_interaction_regulation_reenable_changed_lookahead_catch2.cpp`
  (940 lines), preserving interaction delivery across time-regulation
  disable/re-enable, lookahead bounds, and HLA_EVOKED callbacks.
  The multiple-message process interaction ordering case is now isolated in
  `ieee1516_2025_connection_process_tso_interaction_multiple_message_ordering_catch2.cpp`
  (907 lines), preserving same-timestamp FIFO delivery before one grant and
  HLA_EVOKED callbacks.
  The public multi-recipient process interaction fanout case is now isolated in
  `ieee1516_2025_connection_process_tso_interaction_fanout_catch2.cpp`
  (1151 lines), preserving FIFO delivery before one grant, the
  disabled-queued-reenabled recipient gate, and HLA_EVOKED callbacks.
  The timestamped regional interaction callback-gating case is now isolated in
  `ieee1516_2025_connection_process_tso_regional_interaction_callback_gating_catch2.cpp`
  (1036 lines), preserving regional routing, disabled-queued-reenabled
  delivery, timestamp/retraction preservation, and HLA_EVOKED callbacks.
  The timestamped regional interaction transportation case is now isolated in
  `ieee1516_2025_connection_process_tso_regional_interaction_transportation_catch2.cpp`
  (1075 lines), preserving the transportation override confirmation, regional
  delivery, timestamp/order metadata, and HLA_EVOKED/HLA_IMMEDIATE coverage.
  The directed interaction transportation case is now isolated in
  `ieee1516_2025_connection_process_directed_interaction_transportation_catch2.cpp`
  (864 lines), preserving the configured process endpoint, callback-immediate
  behavior, and transportation override confirmation.
  The timestamped federation-save replacement case is now isolated in
  `ieee1516_2025_connection_process_federation_save_timed_replacement_catch2.cpp`
  (757 lines), preserving replacement timing, multi-federate callback ordering,
  and completion callbacks.
  The single-federate federation-save not-complete case is now isolated in
  `ieee1516_2025_connection_process_federation_save_not_complete_catch2.cpp`
  (591 lines), preserving HLA_EVOKED failure-reason delivery and no-early-
  not-saved behavior.
  The adjacent multi-federate federation-save not-complete case is now
  isolated in
  `ieee1516_2025_connection_process_federation_save_not_complete_multi_federate_catch2.cpp`
  (692 lines), preserving per-recipient HLA_EVOKED failure delivery and the
  configured process-endpoint choreography.
  The terminal federation-save status case is now isolated in
  `ieee1516_2025_connection_process_federation_save_not_complete_status_terminal_catch2.cpp`
  (606 lines), preserving HLA_EVOKED callback gating, failure delivery, and
  terminal NO_SAVE_IN_PROGRESS reporting.
  The two-federate terminal federation-save status case is now isolated in
  `ieee1516_2025_connection_process_federation_save_not_complete_status_terminal_multi_federate_catch2.cpp`
  (738 lines), preserving per-participant HLA_EVOKED callback gating and
  terminal status reporting.
  The owner-reported terminal federation-save status case is now isolated in
  `ieee1516_2025_connection_process_federation_save_not_complete_status_terminal_owner_reporter_catch2.cpp`
  (738 lines), preserving owner-reported failure semantics and terminal status
  reporting.
  The receiver-reported federation-save not-complete case is now isolated in
  `ieee1516_2025_connection_process_federation_save_not_complete_receiver_reporter_catch2.cpp`
  (692 lines), preserving receiver-reported failure semantics and per-recipient
  HLA_EVOKED delivery.
  The HLA_IMMEDIATE terminal federation-save status case is now isolated in
  `ieee1516_2025_connection_process_federation_save_not_complete_status_terminal_immediate_catch2.cpp`
  (598 lines), preserving push-mode callback delivery and terminal
  NO_SAVE_IN_PROGRESS reporting.
  The HLA_IMMEDIATE federation-restore status case is now isolated in
  `ieee1516_2025_connection_process_federation_restore_status_immediate_catch2.cpp`
  (618 lines), preserving push-mode restore callbacks, valid handle transitions,
  and restored status reporting.
  The HLA_IMMEDIATE idle, in-progress, and terminal federation-restore status
  case is now isolated in
  `ieee1516_2025_connection_process_federation_restore_status_idle_terminal_immediate_catch2.cpp`
  (643 lines), preserving status transitions, valid handle projections, and
  callback delivery.
  The two-federate HLA_IMMEDIATE federation-restore status projection case is
  now isolated in
  `ieee1516_2025_connection_process_federation_restore_status_multi_federate_immediate_catch2.cpp`
  (878 lines), preserving requester-only and per-recipient callbacks,
  dual-client status queries, and terminal projections.
  The malformed process HLAresignAction HLAsetSwitches case is now isolated in
  `ieee1516_2025_connection_process_mom_malformed_resign_action_catch2.cpp`
  (685 lines), preserving malformed-parameter reporting, public callback
  delivery, and MOM exception projection.
  The empty-map malformed process HLAsetSwitches interaction case is now
  isolated in
  `ieee1516_2025_connection_process_mom_malformed_exception_catch2.cpp`
  (670 lines), preserving RTI-owned MOM exception reporting, parameter-error
  projection, and switch-state preservation.
  The unknown-parameter malformed process HLAsetSwitches case is now isolated
  in
  `ieee1516_2025_connection_process_mom_malformed_parameter_catch2.cpp`
  (675 lines), preserving RTI-owned MOM exception reporting, parameter-error
  projection, and unchanged switch state.
  The malformed-value process HLAsetSwitches case is now isolated in
  `ieee1516_2025_connection_process_mom_malformed_value_catch2.cpp`
  (680 lines), preserving RTI-owned MOM exception reporting, parameter-error
  projection, and unchanged switch state.
  The predefined HLAexceptionReporting process HLAsetSwitches case is now
  isolated in
  `ieee1516_2025_connection_process_mom_exception_reporting_switch_catch2.cpp`
  (581 lines), preserving getter/setter state and unchanged companion
  switches.
  The process HLAreportException service-invocation case is now isolated in
  `ieee1516_2025_connection_process_mom_service_exception_report_catch2.cpp`
  (675 lines), preserving HLA_EVOKED/HLA_IMMEDIATE push-pull reporting
  behavior.
  The process exception-report concurrency case is now isolated in
  `ieee1516_2025_connection_process_exception_report_concurrency_catch2.cpp`
  (744 lines), preserving pull/push delivery, immediate callback reentrancy,
  typed failure ownership, and resignation cancellation together.
  The process MOM exception-report parameter case is now isolated in
  `ieee1516_2025_connection_process_mom_exception_report_parameters_catch2.cpp`
  (618 lines), preserving inherited HLAfederate projection and
  HLA_EVOKED/HLA_IMMEDIATE delivery together.
  The process service-report dimension-upper-bound case is now isolated in
  `ieee1516_2025_connection_process_service_report_dimension_upper_bound_catch2.cpp`
  (658 lines), preserving switch gating, exact serial-zero reporting, and
  report-file cleanup together.
  The process service-report dimension-handle case is now isolated in
  `ieee1516_2025_connection_process_service_report_dimension_handle_catch2.cpp`
  (655 lines), preserving switch gating, exact serial-zero reporting, and
  report-file cleanup together.
  The process service-report dimension-name case is now isolated in
  `ieee1516_2025_connection_process_service_report_dimension_name_catch2.cpp`
  (658 lines), preserving switch gating, exact serial-zero reporting, and
  report-file cleanup together.
  The process service-report invalid dimension-name failure case is now
  isolated in
  `ieee1516_2025_connection_process_service_report_dimension_name_failure_catch2.cpp`
  (658 lines), preserving switch gating, exact serial-zero reporting, and
  report-file cleanup together.
  The process service-report federation-unknown dimension-name failure case is
  now isolated in
  `ieee1516_2025_connection_process_service_report_dimension_name_unknown_failure_catch2.cpp`
  (666 lines), preserving switch gating, exact serial-zero reporting, and
  report-file cleanup together.
  The process service-report MOM interaction runner is consolidated in
  `ieee1516_2025_connection_process_service_report_interaction_helpers.hpp`
  (1,474 lines), and the unknown-handle interaction case is isolated in
  `ieee1516_2025_connection_process_service_report_dimension_name_unknown_handle_interaction_catch2.cpp`
  (483 lines); the remaining interaction matrix reuses that same runner.
  The binding-handle interaction case is now isolated in
  `ieee1516_2025_connection_process_service_report_dimension_name_binding_handle_interaction_catch2.cpp`
  (483 lines), reusing the same private interaction runner.
  The dimension-handle name-unknown interaction case is now isolated in
  `ieee1516_2025_connection_process_service_report_dimension_handle_name_unknown_interaction_catch2.cpp`
  (483 lines), reusing the same private interaction runner.
  The dimension-handle name-unknown file-destination case is now isolated in
  `ieee1516_2025_connection_process_service_report_dimension_handle_name_unknown_file_destination_catch2.cpp`
  (486 lines), reusing the same private interaction runner.
  Continue splitting the remaining 2025 suites only at complete focused
  lanes, with the next exact case boundary returned by the bounded work query.
- Split `ieee1516_2025_connection_catch2.cpp` by connection lifecycle,
  process-boundary, and service-report ownership.
- Split `umbra_rti_ambassador.cpp` by public method family only after the
  official API surface and internal state owners are inventoried.
- Split `federation_registry.cpp/.hpp` by registry state ownership and restore
  codecs, keeping persistence round trips and lifecycle tests together.
- Split `process_federation_service.cpp/.hpp` by wire decoding, dispatch, and
  service-operation groups without crossing the 2010/2025 or process/public
  boundaries.
- Leave the compliance JSON and historical ledger intact unless their consumer
  is deliberately taught to assemble validated fragments.

Each wave should reduce the largest file, update the size baseline downward,
and leave a focused next handoff. The policy is a ratchet: size may go down,
never silently up.
