# Roadmap

## Completed: standards baseline

- [x] Vendor the unmodified IEEE 1516.1-2025 C++ header tree with its required
      attribution.
- [x] Pin its provenance to the 2025 API source used by the HLA Requirements
      Lab and verify every vendored header digest.
- [x] Compile a C++20 smoke test directly against the official declarations.
- [x] Generate and check the 244-member abstract binding inventory.
- [x] Record a small Requirements-Lab API baseline for Connect, Join, Resign,
      Disconnect, Connection Lost, and Federate Resigned.
- [x] Wire Catch2 and the Requirements-Lab sidecar workflow without creating
      premature implementation evidence.

The baseline is not a whole RTI or a conformance claim. There is no
Umbra-owned replacement public API.

## Foundation: first implementation tranche

- [x] Select the initial static-linkage policy and propagate the official
      STATIC_RTI definition to library consumers.
- [x] Generate and review the complete RTIambassador and FederateAmbassador
      override inventories from the official headers.
- [x] Implement the standard factory, rtiName, and rtiVersion entry points
      with a generated fallback that explicitly rejects unimplemented services.
- [x] Implement the private top-level federate lifecycle projection from the
      Requirements Lab, then bind its Connect and Disconnect transitions to the
      official C++ API.
- [x] Implement the standard connection support types and all four official
      C++ Connect overloads for the embedded backend.
- [x] Implement Disconnect with the official factory surface and add a real
      Catch2 test catalog entry plus raw JUnit sidecar evidence.
- [x] Establish the first private runtime boundaries: top-level federate
      lifecycle, embedded federation registry, FederateHandle value support,
      and callback dispatcher.
- [x] Add an opt-in libxml2 XML/XSD validator for individual official
      1516.2-2025 model documents.
- [x] Add a private Annex C-guided FOM/MIM composition preflight with
      Requirements-Lab source traceability and targeted Catch2 cases.
- [x] Implement the private Annex C.8 first-setting switch merge behavior,
      including ignored equivalent duplicates and private warnings for
      non-equivalent duplicates.
- [x] Resolve direct `dataType` references against the completed composed
      2025 model (including the OMT key's basic-data names), allowing later
      modules to provide a referenced type.
- [x] Resolve each reference-data type's named object class against the
      completed 2025 object hierarchy.
- [x] Resolve ordinary reference-data attributes through the completed class
      hierarchy and require an exact representation-type match; the two
      standard instance identifier names remain a dedicated future rule.
- [x] Resolve each directed-interaction name against the completed 2025
      interaction hierarchy, without bypassing the FDD schema's cardinality
      limit for the supplied extension example.
- [x] Resolve object and interaction available-dimension references against
      the completed 2025 dimension table.
- [x] Retain 2025 dimension associations and upper bounds in the composed FOM
      catalog, allocate stable `DimensionHandle` values, and exercise the
      official available-dimension/name/upper-bound lookup services.
- [x] Exercise the metadata-only 2025 region-template/specification lifecycle
      through official `RegionHandle`/`RangeBounds` values: create, pending
      range updates, complete commit, delete, owner-scoped support lookups,
      FOM upper-bound validation, and handle decoding. The bounded interaction
      regional declaration/send slice separately consumes committed specs for
      independent regional subscriptions, 2025 overlap filtering, empty-set
      suppression, callback rechecks, and official exception mapping. The
      bounded object-attribute regional slice now consumes committed specs for
      no-name registration, additive association/unassociation, active/passive
      regional attribute subscriptions, active-overlap-filtered
      discovery/reflection, and optional sent-region callback metadata. Named regional registration and
      the per-federate Attribute Scope Advisory path now covers committed
      overlap, update-region association, and subscription transitions for known
      objects through immediate/evoked callbacks with stale-work suppression.
      Direct time-constrained timestamped default-region callback coverage is
      now present; a complete timestamped matrix and broader DDM routing
      remain future slices.
- [x] Implement the bounded IEEE 1516.1-2025 default-region realization for
      dimensional receive-order and direct time-constrained timestamped object
      attributes and interactions. The RTI
      derives the full-range default privately instead of exposing a synthetic
      `RegionHandle`; ordinary and explicit regional declarations retain
      independent state while their effective default/non-default
      realizations are mutually exclusive. The regression pair covers source
      association replacement/restoration, discovery and reflection/receive
      routing, scope/relevance continuity, and supplied-empty Convey Region
      Designator Sets metadata. Companion TSO regressions prove that an
      ordinary source default survives queueing to a constrained regional
      subscriber and remains supplied as an empty callback set. A complete
      timestamped matrix, broader relaxed-DDM coverage, save/restore,
      transport, package evidence, and conformance remain separate.
- [x] Resolve attribute and interaction transportation references against
      the completed 2025 transportation table.
- [x] Restrict logical-time and interval representations to the 2025
      table's permitted data-type families (or `NA`).
- [x] Restrict user-supplied and synchronization tag types to their 2025
      permitted data-type families (or `NA`).
- [x] Reject object attributes and interaction parameters that overload
      names inherited from a 2025 superclass.
- [x] Implement the two official IEEE reference logical-time value/interval
      types, factories, HLAfloat64Time default selection, and static
      `umbra::fedtime` forwarding boundary.
- [x] Materialize a schema-validated composed FDD with Annex C
      `Composed_From` metadata, Notes-table remapping, and service-utilization
      OR rules for representable module sets.
- [x] Select the official `HLAfloat64Time` default or explicit reference
      logical-time factory privately, rejecting a conflict with documented FDD
      time data types and leaving custom fedtime providers unavailable.
- [x] Add an opt-in, non-installable federation-management development profile
      that binds official Create/Destroy/Join/Resign methods only after MIM-first
      preparation, FDD materialization, and reference-time selection.
- [x] Bind `getTimeFactory` in that profile to the immutable selected federation
      time representation, with exact Requirements-Lab C++ API-surface
      traceability and no catalog or conformance claim.
- [x] Bind `getFederateHandle` and `getFederateName` in that profile to the
      caller's joined federation: names resolve only for active members, while
      a returned federate designator retains its immutable name after normal or
      forced resignation. Invalid, unknown, membership, and connection paths
      remain distinct, with exact Requirements-Lab source/API traceability only.
- [x] Bind `getObjectClassHandle` and `getObjectClassName` in that profile to
      the current composed FOM catalog, preserving issued values across a
      compatible additional-FOM join and keeping UTF-8/XML names separate from
      the official wide-string boundary. This has exact Requirements-Lab C++
      API-surface traceability only.
- [x] Bind `getInteractionClassHandle` and `getInteractionClassName` in that
      profile to the current composed FOM catalog, preserving issued values
      across a compatible additional-FOM join and keeping UTF-8/XML names
      separate from the official wide-string boundary. This has exact
      Requirements-Lab C++ API-surface traceability only.
- [x] Bind `getAttributeHandle` and `getAttributeName` in that profile to the
      current composed FOM catalog, resolving attributes through their defining
      class so inherited values retain one stable handle. This distinguishes
      invalid class/attribute handles from an attribute not defined for the
      supplied class and has exact Requirements-Lab C++ API-surface traceability only.
- [x] Bind `getParameterHandle` and `getParameterName` in that profile to the
      current composed FOM catalog, resolving parameters through their defining
      interaction class so inherited values retain one stable handle. This
      distinguishes invalid interaction-class/parameter handles from a
      parameter not defined for the supplied interaction class and has exact
      Requirements-Lab C++ API-surface traceability only.
- [x] Bind List Federation Executions and List Federation Execution Members in
      that profile, including the three official report callbacks, immediate /
      evoked delivery, callback-lifetime shutdown fencing, and separate
      Requirements-Lab source/API traceability.
- [x] Bind the 2025 federation synchronization-point services in the embedded
      profile. Registration supports the global and explicit-set forms, emits
      asynchronous confirmation and announcement callbacks with the supplied
      tag, tracks success/failure achievement, expands the set for a late
      join, removes resigning members, and emits Federation Synchronized once
      the remaining set is complete. Separate Requirements-Lab contracts and
      Catch2 coverage record this development-profile slice; FOM/SOM
      synchronization-table enforcement, transport, save/restore interaction,
      protected evidence, and conformance remain future work.
- [x] Bind the untimed 2025 federation-save control plane in the embedded
      profile. The non-time-constrained baseline invokes every joined member
      through the official Initiate Federate Save callback; the bounded
      time-constrained path holds the request until all constrained members
      have ordinary pending grants, invokes each constrained member directly
      before its own Time Advance Grant, and only then queues the
      non-time-constrained members. Begin, Complete, Not Complete, Abort, and
      Query Status maintain per-member state and deliver Federation Saved,
      Federation Not Saved, and status callbacks with the official failure
      reasons. Resignation fails and clears an initiated operation for
      remaining participants. Separate Requirements-Lab contracts and Catch2
      cases record this development-profile slice; stable membership/time-role
      state and non-Flush-Queue dispatch are assumed. Durable snapshot
      serialization, restore of pending advances, all-service save interlocks,
      transport, protected evidence, and
      conformance remain future work.
- [x] Bind the timestamped 2025 `Request Federation Save(label, LogicalTime)`
      overload in the embedded profile. The official logical-time
      implementation is cloned and validated; one pending request may be
      replaced; constrained members must cross the timestamp; queued and
      in-transit TSO payloads at or below the boundary are drained before a
      direct Initiate Federate Save callback and the matching grant. A
      source-linked TSO-ordering case proves that callback occurs while the
      constrained recipient remains Time Advancing, and a second three-member
      TAR case proves non-time-constrained members are notified only after all
      constrained admissions. A TARA case proves the distinct exclusive
      boundary: equality does not initiate a save, while a later grant does.
      A separate next-message case proves NMR's inclusive boundary and
      NMRA's exclusive boundary. A Flush Queue case proves strict actual-FQG
      admission and TSO/direct-initiation/FQG ordering, while two mixed FQR/TAR
      cases prove all constrained members are prequalified before the save
      starts; a TARA/NMRA three-member case proves the two strict ordinary
      modes together; and a six-member case combines all five advance modes.
      A cross-member TSO case proves each constrained member's queued payload
      still precedes its own initiation, and an in-transit callback case proves
      a newly requested save cannot initiate until that recipient's callback
      returns. Durable serialization, timed restore,
      complete save interlocks, transport, protected evidence, and conformance
      remain open.
- [x] Bind the bounded untimed 2025 federation-restore path in the embedded
      profile. Completed saves retain a process-local copyable federation
      snapshot; an accepted restore sends the official request-success,
      federation-begun, and initiate callbacks to every current member, waits
      for all restore-complete indications, restores object/declaration/region
      and time/TSO state, and delivers Federation Restored or Federation Not
      Restored with status responses. Catch2 covers object rollback, one
      logical-time/actual-lookahead rollback boundary, one deferred-lookahead
      rollback boundary, missing labels,
      participant failure, and abort, with exact Requirements-Lab contracts.
      Timed restore, durable external persistence, post-restore handle
      remapping, all-service interlocks, transport, protected evidence, and
      conformance remain future work. A separate checked contract/test slice
      now applies the shared SaveInProgress/RestoreInProgress gate to
      object-class and interaction declaration forms, directed declaration,
      regional services, order/transport, ownership, time-role/query,
      scope-advisory, synchronization, and representative object, DDM, update,
      interaction, and time services; other service families are still open.
- [x] Apply a shared federation-operation gate to representative services
      during active save/restore coordination. The exact 2025 C++ API and
      service-clause selections live in the paired save/restore interlock
      contracts, and Catch2 verifies both official exception paths before
      state mutation. The current slice covers declaration, directed
      declaration, regional services, order/transport, ownership-disposition,
      time-role/query, scope-advisory, synchronization, object, DDM, update,
      interaction, and time-advance services; extend it to other remaining
      service families.
- [x] Give joined federates selected initial logical-time state and bind Time
       Advance Request, Time Advance Grant, and Query Logical Time in the
      no-TSO embedded profile, with callback-gated advancement and separate
      Requirements-Lab source/API traceability.
- [x] Bind callback-gated Enable/Disable Time Regulation, Enable/Disable Time
      Constrained, and Query Lookahead in that no-TSO profile. This preserves
      official time/interval types and pending-state exceptions, but is not
      full GALT/LITS or federation-wide coordination.
- [x] Bind the 2025 `Modify Lookahead` service in the embedded profile. A
      nondecreasing request changes actual lookahead immediately; a lower
      request is applied by the elapsed logical-time delta at each grant, and
      accepted changes re-evaluate the limited TAR scheduler. This has exact
      Requirements-Lab source/API traceability and Catch2 coverage, but does
      not claim the remaining time-advance variants or full coordination.
- [x] Bind the 2025 `Next Message Request` service in the embedded profile.
      When a currently queued TSO message for the recipient is at or below the
      requested time, the effective grant target is that message timestamp and
      its equal-timestamp cohort is delivered before `Time Advance Grant`;
      otherwise the requested time remains the target. The caller boundary is
      retained separately for GALT and timestamp validation. This has exact
      Requirements-Lab source/API traceability and Catch2 coverage, but does
      not claim future transport input, `Flush Queue Request`, or full
      coordination.
- [x] Bind the 2025 `Time Advance Request Available` and `Next Message Request
      Available` services in the embedded profile. The Available forms reuse
      the callback-gated grant path, select only currently queued TSO input in
      the development backend, deliver the selected cohort before
      `Time Advance Grant`, and apply an explicit inclusive defined-GALT rule.
      They have separate Requirements-Lab source/API contracts and Catch2
      coverage. `Flush Queue Request`/`Flush Queue Grant`, future transport
      coordination, and full time-management coordination remain future work.
- [x] Bind the 2025 `Flush Queue Request`/`Flush Queue Grant` pair in the
      embedded profile. The bounded path flushes the current in-process TSO
      queue, computes the actual grant from request/GALT/delivered-timestamp
      minima, reports the optimistic logical time, and retains its next-advance
      floor. Its shared actual-grant calculation also supports strict
      timestamped-save admission before the private grant-state change and FQG,
      including mixed FQR/TAR constrained-member readiness. It has separate
      Requirements-Lab source/API contracts and Catch2 coverage; future
      transport, in-transit coordination, Request Retraction for other TSO
      message families, and full time-management coordination remain future
      work.
- [x] Bind the 2025 `Enable Asynchronous Delivery` and `Disable Asynchronous
      Delivery` services in the embedded profile. Time-constrained federates
      default to time-advance-only receive-order delivery; enabling the switch
      releases deferred receive-order callbacks while idle, and disabling it
      restores time-advance gating. The paired Requirements-Lab contracts and
      Catch2 cases cover the public exceptions and callback boundary. TSO
      delivery, remote transport, MOM reporting, save/restore persistence of
      deferred callbacks, package evidence, and conformance remain open.
- [x] Parse the complete 2025 FDD support-switch table and bind the official
      Convey Region Designator Sets, Automatic Resign Directive, Service
      Reporting, Exception Reporting, Send Service Reports To File, Delay
      Subscription Evaluation, and Allow Relaxed DDM accessors in the embedded
      profile. Per-federate values are independent; static federation-wide
      values are captured at creation. The exact MOM report-service
      subscription/switch interlock is also covered for ordinary and regional
      subscriptions. This has Requirements-Lab contracts and Catch2 coverage,
      but connection-loss execution, MOM report emission/file behavior,
      broader relaxed-DDM behavior, the remaining delayed-subscription matrix,
      package evidence, and conformance remain open. RL-024 records the
      unresolved Lab/XSD automatic-resign default discrepancy.
- [x] Implement an explicit, bounded Allow Relaxed DDM policy at the shared
      committed-region overlap predicate. With the static federation switch
      enabled, only ranges that exactly touch at a boundary are added to the
      strict-overlap set; a positive gap never overlaps and strict overlap is
      preserved. Focused Restaurant-FOM interaction and object-attribute
      regressions exercise disabled, enabled, gapped, and strict cases, with
      the latter also proving discovery/reflection gating. `docs/RELAXED-DDM-POLICY.md`
      records the implementation-defined decision; RL-030 records that the
      Lab has an immutable getter candidate but no corresponding
      delivery-policy candidate. Multi-dimension, timestamped, advisory,
      update-rate, transport, package, and conformance matrices remain open.
- [x] Implement a bounded IEEE 1516.1-2025 §8.1.8 Delay Subscription
      Evaluation path for ordinary `Send Interaction` and ordinary `Update
      Attribute Values`. When enabled at federation creation, an unsubscribed
      joined non-source recipient remains a route candidate and is reprojected
      at its HLA_EVOKED or constrained TSO grant boundary; when disabled,
      original generation-time ineligibility is retained. Catch2 covers both
      modes and callback-time unsubscribe suppression; attribute cases first
      establish object discovery to isolate declaration timing. Regional sends,
      explicit update-region attribute passels, directed interactions, complete
      callback-mode/lifecycle/retraction matrices, relaxed DDM, packaging, and
      conformance remain separate. RL-018 logs the Requirements Lab metadata
      mismatch for the source §8.1.8 candidates.
- [x] Apply the recipient's Convey Region Designator Sets switch at the
      callback boundary for the bounded regional reflection and interaction
      paths. Receive-order and timestamped Catch2 cases prove that a disabled
      switch omits optional sent-region metadata while an enabled switch carries
      the sent update-region realization. Default-region/conveyed-region use,
      directed regional callbacks, remote transport, and conformance remain
      open.
- [x] Add a federation-owned temporal snapshot, FDD Non-Regulated-Grant
      metadata, and no-TSO Query GALT/Query LITS. The read-only bounds include
      other regulators' current or pending request boundary plus actual
      lookahead (and a factory epsilon at a forward zero-lookahead boundary),
      together with queued/in-transit/delivered TSO message input.
- [x] Derive and test the private no-TSO TAR eligibility policy: a constrained
      federate is strictly below a defined GALT, while an undefined GALT uses
      the Non-Regulated-Grant switch.
- [x] Wire that policy into a limited cross-federate TAR scheduler. It queues
      eligible callback actions only after relevant federation time-state
      changes, rechecks the policy at delivery, and has Catch2 coverage for
      strict GALT, disabled/enabled NRG, regulator disable/resignation,
      static NRG retention across an additional-FOM join, and time-constrained disable.
      It is not a TSO queue, full time coordinator, package, or conformance
      claim.
- [x] Add a private recipient-scoped TSO queue foundation using official
      LogicalTime values. It provides stable equal-timestamp ordering,
      recipient isolation, pending fanout retraction, explicit recipient
      pending/delivered/retracted state, and inclusive/exclusive eligibility
      tests.
      It is not connected to GALT/LITS, TAR grants, callbacks, or public
      timestamped services.
- [x] Integrate the private TSO queue into the federation-owned temporal
      snapshot and grant coordinator. The private registry carries queued,
      in-transit, and delivered-since-last-advance timestamps into GALT/LITS;
      callback completion, fanout retraction boundaries, and resign/disconnect
      recipient cleanup are covered by Catch2 and a separate 2025 Lab contract.
      This remains the private coordination layer; the first bounded public
      timestamped interaction family is tracked in the next item.
- [x] Add the first bounded public timestamped service family: the official
      non-regional `Send Interaction(..., LogicalTime)` overload, `Retract`,
      and `MessageRetractionHandle` decode/value plumbing. Time-regulating
      sends validate current/requested time plus lookahead, queue for active
      time-constrained recipients, deliver the timestamped callback before
      `Time Advance Grant`, and support pending-message retraction. Catch2
      covers lower-bound rejection, retraction-before-grant, exact-bound
      delivery, and callback ordering;
      `compliance/timestamped-interaction-requirements-contract.json` traces
       this bounded public slice. This interaction item by itself does not claim
       timestamped object/attribute updates, directed/regional interactions,
       alternate advance modes, cross-family request-retraction callbacks,
       transport, or conformance.
- [x] Add a bounded Request Retraction vertical slice for normal non-regional
      timestamped `Send Interaction`, `Update Attribute Values`, and `Send
      Directed Interaction`, plus region-context `Send Interaction With
      Regions` and bounded regional `Update Attribute Values`. A
      federation-owned recipient ledger distinguishes delivered and queued
      fanout; a legal Retract invokes `Request Retraction` for a delivered
      recipient and suppresses a queued recipient's original callback. Catch2
      covers strict equality rejection, post-delivery callbacks, mixed
      nonconstrained/constrained interaction, directed-interaction, and
      region-context interaction fanout, normal and regional pending attribute
      passel suppression, and immediate-only attribute-update,
      directed-interaction, and region-context interaction cases with no
      temporal-queue fanout.
      `compliance/request-retraction-requirements-contract.json` and its API
      companion trace this deliberately narrow behavior. The separately
      contracted deletion/removal slice includes its required object/ownership
      restoration; complete alternate-advance, re-enable, save/restore,
      transport, and conformance behavior remains open.
- [x] Add the second bounded public timestamped service family: the official
      non-regional `Update Attribute Values(..., LogicalTime)` overload and
      matching `Reflect Attribute Values` callback. It retains recipient-
      specific transportation passels, queues active time-constrained
       recipients, delivers reflections before the matching grant, and shares
       the official retraction/lower-bound plumbing plus the normal
       recipient-retraction ledger. Catch2 covers two passels, lower-bound
       rejection, retraction-before-grant, exact-bound reflection, callback
       ordering, sender exclusion, timestamp/order fields, and a delivered
       immediate recipient's `Request Retraction` callback;
       `compliance/timestamped-attribute-update-
       requirements-contract.json` traces this bounded slice. Timestamped
       directed/regional forms, alternate advance modes,
       separate region-context request-retraction evidence, transport, and
       conformance remain out of scope.
- [x] Add the third bounded public timestamped service family: the official
      non-regional `Delete Object Instance(..., LogicalTime)` overload and
      timestamped `Remove Object Instance` callback. The embedded registry
      reserves the known-recipient removal boundary, queues time-constrained
      recipients, reconstitutes the object when a pending delete is retracted,
      and delivers the exact-bound removal before `Time Advance Grant` with
      official timestamp/order/retraction fields and no sender callback.
      Catch2 covers lower-bound rejection, retraction-before-grant with
      recipient reconstitution, exact-bound removal, callback ordering,
      and sender exclusion;
      `compliance/timestamped-object-deletion-requirements-contract.json` and
      its API companion trace this bounded slice. A second mixed-fanout Catch2
      scenario now proves legal post-delivery Request Retraction: the delivered
      nonconstrained recipient receives `Remove Object Instance`, the
      execution-owned invocation snapshot restores object/name/known state and
      committed split ownership, the same recipient then receives Request
      Retraction, and the constrained recipient's pending removal is
      suppressed. A third scenario proves a delivered owner that resigns before
      Retract is neither reconstituted nor notified, and its former attribute
      remains unowned. A terminal no-recipient case preserves
      `MessageCanNoLongerBeRetracted` while releasing the deletion snapshot and
      object name for a fresh registration. A focused normal-interaction
      regression now covers one Disable Time Regulation/re-enable lifetime path
      at unchanged lookahead. Complete alternate-advance, broader re-enable,
      active in-flight ownership, other resignation, and save/restore recovery
      evidence, transport, and conformance remain out of scope.
- [x] Add the fourth bounded public timestamped service family: the official
      non-regional `Send Directed Interaction(..., LogicalTime)` overload and
      timestamped `Receive Directed Interaction` callback. The registry retains
      recipient-specific target/projection state, queues active constrained
      recipients, delivers before the matching grant, and records every
      recipient in the shared retraction ledger. Catch2 covers lower-bound
      rejection, known-target routing, exact-bound callback ordering,
      timestamp/order/retraction propagation, pending constrained-recipient
      suppression, and post-delivery `Request Retraction` for a
      nonconstrained directed recipient. The paired timestamped-directed-
      interaction Requirements Lab contracts trace this bounded slice. The
      shared planner honors ownership and universal directed subscriptions,
      although the timestamped scenarios do not independently distinguish
      those modes. Directed DDM, alternate advance modes, region-context
      evidence, transport, and conformance remain out of scope.
- [x] Add the fifth bounded public timestamped service family: the official
      non-regional-plus-region-context `Send Interaction With Regions(...,
      LogicalTime)` overload and timestamped `Receive Interaction` callback.
      The existing committed region/overlap planner now feeds the TSO payload,
      preserves sent-region callback data, delivers before the matching grant,
      and records overlap-qualified recipients in the shared retraction ledger.
      Catch2 covers lower-bound rejection, strict overlap, exact-bound callback
      ordering, sent-region propagation, pending constrained-recipient
      suppression, and post-delivery `Request Retraction` for a
      nonconstrained overlap-qualified recipient; paired timestamped-regional-
      interaction Requirements Lab contracts trace this bounded slice. Object
      region services, relaxed DDM, other fanout patterns, alternate advance
      modes, save/restore, transport, and conformance remain out of scope.
- [x] Extend the bounded timestamped attribute-update family to committed
      object-region associations. The regional `Update Attribute Values(...,
      LogicalTime)` path preserves each recipient's update-region projection,
      queues time-constrained reflections before the matching grant, supports
      pending retraction, and rechecks the recipient's Convey Region Designator
      Sets switch before exposing optional sent-region callback metadata. The
      paired Requirements-Lab contracts and Catch2 scenario cover lower-bound
      validation, pending constrained-recipient suppression, Request Retraction
      for a delivered immediate recipient, exact-bound reflection, and callback
      ordering. Mixed immediate/TSO fanout is covered by the same
      scenario: non-time-constrained recipients receive the accepted
      timestamped callback immediately while constrained recipients remain
       queued until their grant. Direct timestamped default-region callback coverage now has
       separate object and interaction regressions; regional request forms,
       alternate advance modes, transport, package evidence, and conformance
       remain open.
- [ ] Extend timestamped public delivery to the remaining 2025 object,
      interaction, ownership, and federation service families only after their
      payload, callback-order, eligibility, and retraction semantics receive
      separate Requirements Lab contracts and real Catch2 scenarios. This
       excludes the now-bounded non-regional timestamped deletion/removal
       retraction path, but includes its remaining regional/directed,
       alternate-time, in-flight-ownership, resignation, recovery, and
       transport cases. The
      timestamped federation-save control overload is tracked by the checked
      item above; it does not make the remaining timestamped delivery families
      complete.
- [x] Record selected time-management and FOM stress scenarios from the legacy
      Python RTI in a revision-pinned, non-normative
      [backlog](LEGACY-PYTHON-RTI-TEST-BACKLOG.md),
      [resource register](LEGACY-PYTHON-RTI-TEST-RESOURCES.md), and
      [FOM stress-corpus backlog](FOM-STRESS-CORPUS-BACKLOG.md), without
      importing its code, FOMs, or evidence claims.
- [x] Adjudicate the sibling's pending-advance lower-bound split against the
      Requirements Lab: a time-advancing regulator's requested time plus
      lookahead supplies the outgoing-TSO lower bound.  Preserve it with a
      direct no-TSO Catch2 regression; it is not a complete GALT claim.
- [x] Port the sibling-inspired three-federate GALT minimum and regulator-resign
      scenario through the official C++ API in the embedded no-TSO profile.
- [x] Extend the private 2025 FOM composition preflight to resolve simple and
      enumerated representation names, reject ordinary reference-data types
      that use basic/reference representations, and validate the standardized
      `HLAobjectInstanceName` / `HLAobjectInstanceHandle` reference exception.
      The MIM/Restaurant `HLAboolean` compatibility interpretation remains
      recorded as RL-009; this slice has a paired 1516.2 Requirements-Lab
      contract and Catch2 coverage, but is not FOM conformance evidence.
- [x] Enforce the 2025 strictly positive supplied update-rate table constraint
      in the private composition preflight. Incomplete DIF rows remain
      representable, while supplied zero/non-positive values fail with explicit
      diagnostics; the 2025 FOM XSD already enforces positive dimension upper
      bounds. The paired 1516.2 Lab contract and Catch2 case are traceability
      only; lookahead-sign and remaining table constraints remain open.
- [x] Enforce the 2025 Dimension-table Value When Unspecified rule in the
      private composition preflight. Integer and half-open range forms are
      checked as nonnegative subranges of `[0, Dimension Upper Bound)` and
      `Excluded` remains valid. The paired 1516.2 Lab contract and Catch2 case
      are traceability only; lookahead-sign, synchronization capability, and
      remaining table constraints remain open.
- [x] Enforce the 2025 array-data `Cardinality` lexical rule in the private
      composition preflight. Supplied scalar, comma-separated, bounded-range,
      mixed-component, and `Dynamic` forms are accepted; malformed or reversed
      ranges fail before FDD materialization, while omitted values remain
      representable for incomplete DIF modules. The paired 1516.2 Lab contract
      and Catch2 case are traceability only. The same preflight rejects a
      schema-valid one-dimensional mismatch between fixed/varying cardinality
      and the predefined `HLAfixedArray`/`HLAvariableArray` encoding pair;
      multidimensional/provider-defined encoding interpretation and remaining
      table constraints remain open.
- [ ] Complete remaining Annex C, table-specific reference-resolution, and
      switch-default rules,
      resolve the supplied extension's directed-interaction/FDD-schema mismatch,
      and package the XML/resource dependency contract before widening the
      federation-management runtime boundary.
- [ ] Resolve the Requirements-Lab aggregate C++ Connect crosswalk so its four
      runtime-tested overloads can be cataloged with selected surfaces.
- [ ] Obtain protected review acceptance for the raw Disconnect evidence before
      calling that binding verified.

## Federation-management vertical slice

- [x] Implement private callback scheduling plus the standard Evoke/Enable/
      Disable control methods, then use it for federation-listing reports in
      the non-installable development profile.
- [x] Drive the Connection Lost callback from a private embedded transport
       endpoint. A one-shot fault applies the member's Automatic Resign
       Directive through forced registry cleanup, transitions the ambassador to
       Not Connected, queues the official callback, and permits a fresh
       Connect. The bounded multi-federate case sets `DELETE_OBJECTS` through
       the official support service and proves delete-privileged removal.
       Remote transport, the remaining directive combinations, package support,
       protected review, and conformance remain open.
- [x] Drive the distinct Federate Resigned callback from a private embedded
       in-session RTI-control seam. The bounded path removes a clean joined
       member through the ordinary `NO_ACTION` registry transition, retains its
       connection, queues the official callback, and permits a fresh Join; a
       federate-initiated resign and Connection Lost do not emit this callback.
       Real session watchdog/administration input, forced-resign ownership
       policy, remote transport, package support, protected review, and
       conformance remain open.
- [x] Exercise federation creation, destruction, join, and resign using official
      exceptions and handles in the non-installable development profile. This
      has source/test traceability only and no catalog or conformance claim.
- [x] Implement and test the bounded 2025 resign-action disposition slice in the
      non-installable development profile. Directive 1 unconditionally divests
      owned attributes and offers current eligible recipients, directive 2
      removes objects for which the resigning federate owns
      `HLAprivilegeToDeleteObject`, and directive 5 cancels the resigning
      federate's pending acquisition work before applying delete/divest cleanup.
      The final-federate rule also forces directive 2 even when the supplied
       action is `NO_ACTION`. The official `FederateOwnsAttributes` and
       `OwnershipAcquisitionPending` preconditions are covered. Bounded
       continuation after later publication, discovery, and join is covered;
       terminal callback re-search/arbitration, remaining automatic-resign
       directive combinations, RTI-owned state, remote transport, and
       conformance remain separate work;
       this item has source/API traceability only.
- [x] Exercise joined-name and departed-designator lookup through the official
      support services in the non-installable development profile, including
      invalid and cross-federation handles plus normal and Connection Lost
      resignation. This has source/API traceability only and no catalog or
      conformance claim.
- [x] Implement the five official handle-normalization support services in the
      non-installable development profile. The registry provides stable
      execution-scoped point coordinates for equal valid federate,
      object-class, interaction-class, and live object-instance designators;
      `HLAserviceGroup` stays inside its fixed standard dimension range. The
      focused regression covers connection/member/input exceptions,
      cross-member equality, and departed federate-designator stability. MOM
      point-region realization/report routing, distributed execution, package
      evidence, and conformance remain separate work; this item has only
      source/API traceability.
- [x] Exercise object-class name/handle lookup through the official support
      services in the non-installable development profile, including invalid
      handles and stable values after a compatible additional-FOM join. This
      has API traceability only and no catalog or conformance claim.
- [x] Exercise interaction-class name/handle lookup through the official
      support services in the non-installable development profile, including
      invalid handles and stable values after a compatible additional-FOM join.
      This has API traceability only and no catalog or conformance claim.
- [x] Exercise inherited attribute name/handle lookup through the official
      support services in the non-installable development profile, including
      invalid class/attribute handles, an unrelated-class rejection, and stable
      values after a compatible additional-FOM join. This has API traceability
      only and no catalog or conformance claim.
- [x] Exercise inherited parameter name/handle lookup through the official
      support services in the non-installable development profile, including
      invalid interaction-class/parameter handles, an unrelated-interaction
      rejection, and stable values after a compatible additional-FOM join. This
      has API traceability only and no catalog or conformance claim.
- [x] Exercise the four interaction publication/subscription declaration
      methods in the non-installable development profile. The private registry
      keeps independent per-federate state and clears it on resign; only active
      subscriptions feed the limited receive-order interaction slice, while
      passive declarations remain state without arranging delivery; the state
      also feeds the bounded
      ordinary Turn Interactions On/Off relevance advisories, including the
      per-federate relevance switch. This has API traceability only and no
      catalog or conformance claim.
- [x] Exercise the four non-region object-class attribute declaration methods
      in the non-installable development profile. The private registry retains
      per-federate, per-class explicit publication and active/passive
      subscription state, validates inherited attributes, and clears it on
      resign. Only active declaration state feeds the limited unnamed
      registration/discovery slice and the bounded ordinary Start/Stop Registration relevance
      advisories, including the per-federate relevance switch. Full ownership,
      update-rate enforcement, remaining object-attribute regional forms, and
      broader DDM remain unimplemented. This has source/API
      traceability only and no catalog or conformance claim.
- [x] Bind ordinary declaration-management relevance advisories in the
      non-installable development profile. Hierarchy-aware effective
      publication/active-subscription transitions emit Start/Stop Registration
      For Object Class and Turn Interactions On/Off exactly once per relevance
      transition, while passive subscriptions remain non-relevant. The
      per-federate switch values are seeded from the composed FDD, with the
      1516.2 Disabled default for omitted entries, and remain mutable through
      the official accessors. Regional declaration advisories, MOM behavior,
      update-rate enforcement, package evidence, and conformance remain
      separate work; the paired Requirements-Lab contracts and Catch2 scenario
      are traceability only.
- [x] Bind the whole-object-class `unpublishObjectClass` and
      `unsubscribeObjectClass` services in the non-installable development
      profile. Whole publication teardown removes every currently published
      ordinary attribute, including the implicit delete privilege, clears the
      corresponding ownership on registered instances, and rejects stale
      updates; whole subscription teardown removes ordinary declarations while
      preserving independent regional declarations. Separate Requirements-Lab
      contracts and Catch2 coverage record this bounded slice; regional
      declaration advisories, regional whole-class teardown, save/restore,
      transport, and conformance remain future work.
- [x] Exercise unnamed non-region `Register Object Instance`, generated names,
      `Discover Object Instance`, and the three known-instance support services
      in the non-installable development profile. The private registry assigns
      unique handles, snapshots currently published attributes, promotes to the
      closest active subscribed superclass, rechecks queued discovery, and uses both
      callback models. Timestamped/retraction, DDM, ownership transfer,
      save/restore, FOM sharing policy, and full resign-action object disposition
      remain unimplemented. This has source/API
      traceability only and no catalog or conformance claim.
- [x] Exercise the 2025 single and multiple object-instance name reservation
      and release services in the non-installable development profile. The
      registry rejects empty and `HLA.` names, commits reservation state before
      the four official result callbacks, reports mixed multiple outcomes,
      validates multiple release atomically, avoids generated-name collisions,
      and returns names to the federation-wide pool on resignation. This item
      has source/API traceability only and no catalog or conformance claim.
- [x] Exercise reservation-consuming named `Register Object Instance` and
      `Register Object Instance With Regions` in the non-installable development
      profile. The private registry enforces reservation ownership, preserves a
      reservation across publication failure, consumes it only after object/name
      commit, reports standard name-in-use/not-reserved outcomes, and retains
      uniform named lookup/discovery. This item has source/API traceability only
      and no catalog or conformance claim.
- [x] Exercise non-region, receive-order `Delete Object Instance` and no-time
      `Remove Object Instance` in the non-installable development profile. The
      private registry verifies the current `HLAprivilegeToDeleteObject` owner,
      makes the deleting federate unknown immediately, retains each other
      known recipient until its callback, and preserves tag/producer identity
      across both callback models. Timestamped/retraction deletion, ownership
      transfer, DDM, FOM sharing policy,
      save/restore, and the remaining resign-action disposition remain unimplemented. This
      has source/API traceability only and no catalog or conformance claim.
- [x] Exercise the 2025 `Local Delete Object Instance` service in the
      non-installable development profile. The private registry removes only
      the invoking federate's known-instance state, rejects owned attributes
      and pending ownership acquisition, leaves the federation-wide object and
      other federates intact, and permits a later eligible rediscovery. This
      has source/API traceability only and no catalog or conformance claim.
- [x] Exercise the non-timestamped, non-region `Update Attribute Values`
      overload and matching no-time `Reflect Attribute Values` callback in the
      non-installable development profile. The private registry requires source
      ownership, groups supplied values into FOM transportation passels,
      projects them at each receiver's known class and current active subscription,
      suppresses passive and source delivery, rechecks queued delivery, and preserves
      tag/producer/type through both callback models. A separate regional
      object-attribute case covers active committed-overlap discovery/reflection,
      passive suppression, and optional sent-region callback metadata. Timestamped/retraction behavior,
      additional regional request edge cases, default-region
      synthesis, broader DDM routing,
      update-rate reduction, ownership transfer, custom
      transportation, FOM sharing policy, save/restore, and remote transport
      remain unimplemented. This has source/API traceability only and no
      catalog or conformance claim.
- [x] Exercise the 2025 regional object-attribute forms in the non-installable
      development profile: no-name `Register Object Instance With Regions`,
      additive `Associate Regions For Updates`/`Unassociate Regions For
      Updates`, and regional `Subscribe/Unsubscribe Object Class Attributes
      With Regions`. The private registry validates committed region ownership
      and object-class dimension context, keeps regional declarations separate,
      filters discovery and no-time reflection by active overlap while passive
      triples remain declared without arranging delivery, preserves the
      optional sent-region callback set, and treats empty region sets as no-ops.
      Additional regional forms, timestamped default-region coverage,
      timestamped/retraction behavior, broader DDM
      routing, package/catalog evidence, and conformance remain out of scope.
      This has source/API traceability only.
- [x] Bind the 2025 ownership/DDM boundary for explicit object-attribute
      update-region associations. The current owner's association is cleared
      when If Available plus Divestiture If Wanted transfers ownership, so a
      former explicit region cannot route stale updates; the default source
      realization resumes unless the new owner explicitly associates a region.
      The same cleanup helper is
      used by Confirm Divestiture, unconditional divestiture, unpublish, and
      resignation paths. The paired Requirements-Lab contracts and Catch2 case
      are traceability only; complete transfer arbitration, timestamped default-region
      synthesis, advisory scope callbacks, package evidence, and conformance
      remain open.
- [x] Exercise the 2025 Attribute Scope Advisory path in the non-installable
      development profile. The per-federate advisory switch gates grouped
      `attributesInScope` / `attributesOutOfScope` callbacks for known objects
      whose committed regional overlap, update-region association, or ordinary/
      regional subscription changes. The integration case proves immediate and
      evoked delivery, callback-time stale-transition suppression, and the
      official switch accessors. Timestamped default-region coverage,
      timestamped/retraction behavior, broader DDM routing, package/catalog
      evidence, and conformance remain separate. This has source/API
      traceability only.
- [x] Parse and expose the 2025 Advisories Use Known Class switch in the
      non-installable development profile. The FDD composer applies the
      Disabled omission default, federation creation captures one static
      federation-wide value, and the official read-only
      `getAdvisoriesUseKnownClassSwitch` getter maps membership/save/restore/
      connection boundaries. The paired Requirements-Lab contracts and
      Catch2 case remain traceability only; complete known-class advisory
      generation, MOM behavior, package evidence, and conformance remain
      future work.
- [x] Bind the bounded 2025 Attribute Relevance Advisory path in the
      non-installable development profile. The registry separates effective
      attribute scope from the Attribute Scope Advisory switch, plans Turn
      Updates On/Off callbacks for ordinary and regional subscription/update-
      region transitions, and the adapter rechecks the Attribute Relevance
      switch, ownership, known-instance state, and current scope before callback
      entry. Omitted/default subscriptions use the no-rate overload; explicit
      retained designators use the rate-bearing overload, with queued callback
      entry re-resolving the current designator. The paired Requirements-Lab
      contracts and Catch2 cases are traceability only. Initial registration/discovery advisories, complete regional DDM,
      update-rate enforcement, package evidence, and conformance remain future
      work.
- [x] Retain composed 2025 FDD `updateRates` metadata and bind the official
      `getUpdateRateValue` / `getUpdateRateValueForAttribute` support queries
      in the non-installable development profile. Named Restaurant FOM rates,
      the `HLAdefault` no-reduction boundary, invalid-designator handling, and
      known-object/defined-attribute validation have exact Requirements-Lab
      contracts and Catch2 coverage. Ordinary and regional subscription
      declarations now retain FDD designators, and the attribute query reports
      the corresponding rate or the default `0.0` after unsubscribe. Throttling,
      rate reduction, MOM, package
      evidence, and conformance remain future work.
- [x] Exercise the object-instance `Request Attribute Value Update` overload
      and matching `Provide Attribute Value Update` callback in the
      non-installable development profile. The private registry validates the
      requester's known class, groups requested currently owned attributes by
      provider, suppresses unowned and requester-owned callback targets,
      preserves the tag, enforces at most one callback per provider group, and
      rechecks a queued provider after resignation. A separate bounded response
      case lets the provider invoke non-timestamped `Update Attribute Values`
      from inside `Provide Attribute Value Update` and verifies requester-side
      `Reflect Attribute Values` tag, producer, and mandatory transport. This
      is explicit provider code rather than RTI-automatic provision. The
      sibling object-class form is tracked separately; regional request forms,
      timestamped/retraction behavior, DDM,
      update-rate reduction, ownership transfer, FOM sharing policy,
      save/restore, and remote transport remain unimplemented. This has
      source/API traceability only and no catalog or conformance claim.
- [x] Bind the bounded federation-wide Auto Provide switch in the
      non-installable development profile. The composed FDD retains the
      Disabled omission default, federation creation captures the initial
      dynamic value, and the official `getAutoProvideSwitch` getter returns
      it to every current member. When enabled, a newly completed discovery
      groups the discovered object's in-scope owned attributes by provider and
      invokes `Provide Attribute Value Update` with the mandatory empty tag;
      the existing callback-time recheck suppresses stale provider work. The
      standard federation-wide `HLAsetSwitches` MOM interaction now accepts
      the official `HLAswitch` encoding and changes the value for all current
      members. The separately bounded joined-federate `HLAsetSwitches` path
      updates its sender's selected support switches, handles compatible
      extension parameters/subclasses, and preserves the report-service
      interlock without claiming normal MOM reporting. Other MOM
      control/reporting families, complete multi-owner/regional/update-rate
      behavior, package evidence, protected review, JUnit
      promotion, and conformance remain future work.
- [x] Exercise the object-class `Request Attribute Value Update` overload and
      matching `Provide Attribute Value Update` callback in the non-installable
      development profile. The private registry validates selected-class
      attributes, expands a base-class request across all current registered
      subclass instances without requester discovery, groups work per provider
      and object instance, preserves the tag, suppresses requester-owned
      callbacks, and rechecks queued providers after resignation. It only
      solicits the callback: additional regional request forms, automatic provision, a
      resulting value update, timestamped/retraction behavior, DDM,
      update-rate reduction, ownership transfer, FOM sharing policy,
      save/restore, and remote transport remain unimplemented. This has
      source/API traceability only and no catalog or conformance claim.
- [x] Exercise the class-level 2025 `Request Attribute Value Update With
      Regions` overload and matching `Provide Attribute Value Update` callback
      in the non-installable development profile. The private registry validates
      committed request-region ownership/context, treats an empty pair as a
      no-op, filters explicit update associations by overlap, retains
      default-region eligibility, preserves the tag, and rechecks the request
      regions at callback entry. Provider responses remain explicit user code;
      resulting reflection, automatic provision, timestamped/retraction
      behavior, broader DDM, package/catalog
      evidence, and conformance remain separate work. This has source/API
      traceability only.
- [x] Exercise `Query Attribute Ownership` and its 2025 C++ federate-owned /
      unowned result callbacks in the non-installable development profile. The
      private registry validates the requester's known class, groups attributes
      by joined owner or available-for-acquisition state, preserves the official
      callback distinction, and nullifies queued reports after receive-order
      `Remove Object Instance` begins. RTI-owned results are intentionally not
      synthesized: complete regular/negotiated acquisition, remaining divestiture
      flows, RTI-owned state, complete resign-action disposition, DDM, save/restore, and
      conformance remain separate work.
- [x] Exercise the read-only 2025 `Is Attribute Owned By Federate` service in
      the non-installable development profile. The test distinguishes the
      invoking current owner from a remote owner and a defined-but-unowned
      attribute, while retaining the known-instance, known-class, and removal
      boundaries. It does not implement an ownership-state transition or
      broaden the RTI-owned or remaining acquisition/divestiture scope.
- [x] Exercise the bounded 2025 `Attribute Ownership Acquisition If Available`
      service with its official `Attribute Ownership Acquisition Notification`
      and `Attribute Ownership Unavailable` callbacks. The private registry
      retains a pending willing-to-acquire request, transfers an attribute only
      at callback delivery when it remains unowned, reports joined remote-owned
      attributes unavailable without a release callback, preserves an existing
      WTA state on repeat while admitting eligible additional attributes,
      rejects required-publication removal with `OwnershipAcquisitionPending`,
      preserves the tag, and cancels the pending request on resignation or
      receive-order removal.
      Complete regular and negotiated acquisition, the continuing
      ownership-assumption search behind unconditional divestiture, negotiated
      divestiture, RTI-owned state, and full resign-action ownership disposition
      remain separate work.
- [x] Exercise the bounded 2025 regular `Attribute Ownership Acquisition` and
      `Attribute Ownership Release Denied` path. A private regular request
      overrides the same federate's WTA reservation, transfers an unowned
      attribute only at its acquisition-notification callback, and requests
      release from a joined remote owner with the original acquisition tag.
      Repeated requests do not duplicate a release callback; release denied
      retains ownership and terminates every matching regular request through
      unavailable callbacks carrying its denial tag. Negotiated acquisition,
      remaining divestiture flows, RTI-owned state, full resign-action ownership
      disposition, and conformance remain separate work.
- [x] Exercise bounded 2025 `Cancel Attribute Ownership Acquisition` and its
      `Confirm Attribute Ownership Acquisition Cancellation` callback. An
      accepted cancellation targets only a pending regular request, invalidates
      its queued notification/release work, preserves the paired publication
      guard until a single grouped confirmation callback begins, and then
      permits unpublication. Competing in-flight cancellation races that yield
      notification or unavailable, negotiated acquisition, remaining divestiture flows,
      RTI-owned state, full resign-action disposition, and conformance remain
      separate work.
- [x] Exercise bounded 2025 `Attribute Ownership Divestiture If Wanted` with
      its returned attribute set and official `Attribute Ownership Acquisition
      Notification` callback. The profile validates the entire supplied owner
      set, returns only attributes with an already-pending regular or If
      Available acquirer, transfers those attributes synchronously, propagates
      the divestiture tag, and retains the selected acquirer's publication guard
      until notification begins. Its deterministic earliest-accepted mixed-form
      selection is a private development-profile policy rather than an IEEE
      arbitration rule; stale old-owner work is suppressed and later regular
      requests are replanned at the new owner. The separate bounded
      Unconditional/Assumption path, negotiated divestiture, RTI-owned state,
      full resign-action disposition, and conformance remain separate work.
- [x] Exercise bounded 2025 `Unconditional Attribute Ownership Divestiture`
      with the official `Request Attribute Ownership Assumption` callback. The
      profile validates the entire owner set, immediately leaves it unowned,
      retains any existing standard acquisition work, and sends one grouped
      tagged offer to each currently eligible non-pending joined federate. The
      offer rechecks publication, known-instance, pending, and unowned state
       immediately before callback delivery; it does not itself transfer
       ownership. The registry retains unowned search state and rechecks it
       after later joins, discovery, or publication changes, suppressing
       duplicate offers. Terminal callback re-search and full owner arbitration
       remain separate work.
- [x] Exercise bounded 2025 `Negotiated Attribute Ownership Divestiture`,
      `Request Divestiture Confirmation`, `Confirm Divestiture`, and `Cancel
      Negotiated Attribute Ownership Divestiture`. The current owner stays
      owner while private Waiting state selects an already pending regular
      acquirer; the owner gets one confirmation callback with the acquisition
      tag. Confirm transfers ownership synchronously and forwards its own tag
      to the standard acquisition notification. Cancellation removes the
      pending negotiated state and restores ordinary regular-release planning,
      including the stale-callback boundary; cancellation of the selected
      acquirer yields `NoAcquisitionPending`. This does not implement the
      complete negotiated owner-search lifecycle, Willing-to-Acquire selection,
      negotiated acquisition, RTI-owned state, full resign-action disposition,
      or conformance.
- [x] Exercise the mandatory 2025 `HLAreliable` and `HLAbestEffort`
      transportation-type name/handle support services in the non-installable
      development profile. The bounded receive-order interaction and
      attribute-update paths now use the effective per-federate type, while
      custom types and message transport remain absent. This has API
      traceability only with no catalog or conformance claim.
- [x] Exercise the mandatory 2025 `Receive` and `TimeStamp` order-type
      name/value lookup services in the non-installable development profile.
      Invalid names/types and connection/membership preconditions map to the
      official exception types. The adjacent order-control slice now captures
      prospective per-federate class defaults, per-instance preferred order,
      publisher-scoped interaction overrides, and mixed Receive/TimeStamp
      callback metadata for timestamped interaction and attribute delivery.
      A negotiated transfer case also verifies that the acquiring federate's
      default replaces the old instance override; If Available and remaining
      ownership-disposition variants, alternate time/TSO modes, save/restore,
      and conformance remain separate work.
- [x] Exercise the three official 2025 order-control services in the
      non-installable development profile. `Change Default Attribute Order
      Type` affects future registrations, `Change Attribute Order Type` affects
      future owned updates, and `Change Interaction Order Type` affects future
      sends by that publisher. The Catch2 case proves a mixed immediate/TSO
      boundary and official invalid-order failure behavior. Requirements Lab
      contracts record source/API traceability only; save/restore, complete
      ownership-disposition coverage, alternate time modes, and conformance
      remain open.
- [x] Exercise the bounded 2025 transportation-type control services. Per-
      federate attribute defaults are prospective and captured when future
      instances are registered; instance changes commit only at
      `Confirm Attribute Transportation Type Change`, and queries report the
      current effective type. Published interaction changes likewise commit at
      `Confirm Interaction Transportation Type Change` and feed future ordinary
      and regional sends for that publisher. Only the two mandatory standard
      transport names are supported; directed-interaction policy, complete
      relevance-advisory preconditions, custom/remote transport, save/restore,
      package support, protected evidence, and conformance remain outside this
      bounded slice. Separate Requirements Lab contracts and Catch2 coverage
      record the source/API traceability.
- [x] Exercise the non-timestamped, non-region `Send Interaction` overload
      and matching no-time `Receive Interaction` callback in the
      non-installable development profile. The routing kernel chooses the
      closest active subscribed class, projects available parameters, excludes the
      sender, rechecks unsubscribe-before-delivery, and uses the recipient's
      immediate or evoked callback model. Timestamped/retraction behavior,
      remaining object-attribute regional forms, broader DDM routing,
      FOM sharing-policy enforcement,
      other timestamped/local object-lifecycle delivery, custom transportation, and
      message transport are outside this receive-order item; separate bounded
      timestamped families are tracked above. This has
      source/API traceability only and no catalog or
      conformance claim.
- [x] Exercise the bounded 2025 object-class directed-interaction declarations
      and non-timestamped, non-DDM `Send Directed Interaction` / `Receive
      Directed Interaction` path in the non-installable development profile.
      Delivery requires a known target object and a declared directed
      publication/subscription pair, excludes the sender, preserves the tag,
      producer, and mandatory FOM-selected transportation, and rechecks
      declaration/lifecycle state at callback entry for both immediate and
      evoked delivery. The ownership/universal selector is implemented: a
      missing or false selector requires an owned target attribute, true
      accepts every known target, an empty class set preserves modes, and
      re-subscribing a supplied class changes that class's mode.
      Timestamped/retraction behavior beyond the separate bounded slice,
      directed DDM, ordering, FOM sharing-policy enforcement,
      target-departure cleanup, packaging, Lab mapping resolution, evidence,
      and conformance remain outside this slice.
- [x] Exercise the 2025 regional `Subscribe Interaction Class With Regions`,
      `Unsubscribe Interaction Class With Regions`, and no-time `Send
      Interaction With Regions` overloads in the non-installable development
      profile. Regional declarations remain independent from ordinary
      subscriptions; only active committed region-set overlap gates delivery,
      passive pairs remain declared without arranging delivery, empty sent
      sets suppress delivery, and queued callbacks recheck active overlap. Remaining
      object-attribute regional forms, timestamped/retraction, realization, broader
      DDM routing, package/catalog evidence, and conformance remain out of
      scope. This has source/API traceability only.
- [ ] Add multi-federate callback ordering, connection-loss error-path,
      complete timestamped/region update-reflection, local/timestamped
      deletion, ownership-disposition, and federation-wide time-management
      scenarios beyond the listing and
      per-federate temporal reports.
- [ ] Promote accepted Requirements-Lab evidence only after protected review.

## Remaining RTI capability groups

- [ ] Complete FOM module management and declaration management beyond the
      interaction declaration/delivery foundation, including sharing and
      advisory behavior.
- [ ] Complete object lifecycle/updates/ownership, interaction, and data
      distribution management.
- [ ] Time management, ownership management, and save/restore.
- [ ] MOM services after the corresponding base service behavior is stable.
- [ ] Embedded and remote transports, multi-process interoperability, and
      independently reviewed conformance evidence.

## Follow-on bindings

Python or Java adapters are downstream work. They must wrap a stable native
C++ RTI rather than become a second implementation.
