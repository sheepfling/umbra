# Roadmap

## Completed: standards baseline

- [x] Vendor the unmodified IEEE 1516.1-2025 C++ header tree with its required
      attribution.
- [x] Pin its provenance to the 2025 API source used by the HLA Requirements
      Lab and verify every vendored header digest.
- [x] Compile a C++20 smoke test directly against the official declarations.
- [x] Generate and check the 244-member abstract binding inventory.
- [x] Record a small Requirements-Lab API baseline for Connect, Join, Resign,
      Disconnect, and Connection Lost.
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
      no-name registration, additive association/unassociation, regional
      attribute subscriptions, overlap-filtered discovery/reflection, and
      optional sent-region callback metadata. Named regional registration and
      the per-federate Attribute Scope Advisory path now covers committed
      overlap, update-region association, and subscription transitions for known
      objects through immediate/evoked callbacks with stale-work suppression.
      Timestamped sends, default-region synthesis, and broader DDM routing
      remain future slices.
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
- [x] Bind `getFederateHandle` and `getFederateName` in that profile to active
      membership in the caller's joined federation, with distinct invalid,
      unknown, membership, and connection paths plus exact Requirements-Lab
      C++ API-surface traceability only.
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
- [x] Give joined federates selected initial logical-time state and bind Time
       Advance Request, Time Advance Grant, and Query Logical Time in the
      no-TSO embedded profile, with callback-gated advancement and separate
      Requirements-Lab source/API traceability.
- [x] Bind callback-gated Enable/Disable Time Regulation, Enable/Disable Time
      Constrained, and Query Lookahead in that no-TSO profile. This preserves
      official time/interval types and pending-state exceptions, but is not
      full GALT/LITS or federation-wide coordination.
- [x] Add a federation-owned temporal snapshot, FDD Non-Regulated-Grant
      metadata, and no-TSO Query GALT/Query LITS. The read-only bounds include
      other regulators' current or pending time plus actual lookahead (and a
      factory epsilon at a forward zero-lookahead TAR boundary), but no TSO
      queue or timestamped message input.
- [x] Derive and test the private no-TSO TAR eligibility policy: a constrained
      federate is strictly below a defined GALT, while an undefined GALT uses
      the Non-Regulated-Grant switch.
- [x] Wire that policy into a limited cross-federate TAR scheduler. It queues
      eligible callback actions only after relevant federation time-state
      changes, rechecks the policy at delivery, and has Catch2 coverage for
      strict GALT, disabled/enabled NRG, regulator disable/resignation,
      additional-FOM NRG definition replacement, and time-constrained disable.
      It is not a TSO queue, full time coordinator, package, or conformance
      claim.
- [x] Add a private recipient-scoped TSO queue foundation using official
      LogicalTime values. It provides stable equal-timestamp ordering,
      recipient isolation, pending fanout retraction, terminal delivered/
      retracted state, and explicit inclusive/exclusive eligibility tests.
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
      delivery, callback ordering, and terminal post-delivery retraction;
      `compliance/timestamped-interaction-requirements-contract.json` traces
      this bounded public slice. It does not claim timestamped object/attribute
      updates, directed/regional interactions, alternate advance modes,
      request-retraction callbacks, transport, or conformance.
- [x] Add the second bounded public timestamped service family: the official
      non-regional `Update Attribute Values(..., LogicalTime)` overload and
      matching `Reflect Attribute Values` callback. It retains recipient-
      specific transportation passels, queues active time-constrained
      recipients, delivers reflections before the matching grant, and shares
      the official retraction/lower-bound plumbing. Catch2 covers two passels,
      lower-bound rejection, retraction-before-grant, exact-bound reflection,
      callback ordering, sender exclusion, timestamp/order fields, and terminal
      post-delivery retraction; `compliance/timestamped-attribute-update-
      requirements-contract.json` traces this bounded slice. Timestamped
      directed/regional forms, alternate advance modes,
      request-retraction callbacks, transport, and conformance remain out of
      scope.
- [x] Add the third bounded public timestamped service family: the official
      non-regional `Delete Object Instance(..., LogicalTime)` overload and
      timestamped `Remove Object Instance` callback. The embedded registry
      reserves the known-recipient removal boundary, queues time-constrained
      recipients, reconstitutes the object when a pending delete is retracted,
      and delivers the exact-bound removal before `Time Advance Grant` with
      official timestamp/order/retraction fields and no sender callback.
      Catch2 covers lower-bound rejection, retraction-before-grant with
      recipient reconstitution, exact-bound removal, callback ordering,
      sender exclusion, and terminal post-delivery retraction;
      `compliance/timestamped-object-deletion-requirements-contract.json` and
      its API companion trace this bounded slice. Mixed fanout reconciliation,
      directed/regional forms, alternate advance modes, request-retraction
      callbacks, transport, and conformance remain out of scope.
- [ ] Extend timestamped public delivery to the remaining 2025 service families
      only after their payload, callback-order, eligibility, and retraction
      semantics receive separate Requirements Lab contracts and real Catch2
      scenarios.
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
- [ ] Connection-lost callback driven by a real embedded transport or
      federation event; Connect/Disconnect behavior is implemented but not
      conformance-verified.
- [x] Exercise federation creation, destruction, join, and resign using official
      exceptions and handles in the non-installable development profile. This
      has source/test traceability only and no catalog or conformance claim.
- [x] Exercise active federate name/handle lookup through the official support
      services in the non-installable development profile, including invalid
      and cross-federation handles. This has API traceability only and no
      catalog or conformance claim.
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
      keeps independent per-federate state and clears it on resign; that state
      now feeds the limited receive-order interaction slice. This has API
      traceability only and no catalog or conformance claim.
- [x] Exercise the four non-region object-class attribute declaration methods
      in the non-installable development profile. The private registry retains
      per-federate, per-class explicit publication and active/passive
      subscription state, validates inherited attributes, and clears it on
      resign. That state now feeds the limited unnamed registration/discovery
      slice; declaration advisory behavior, full ownership, update-rate
      enforcement, remaining object-attribute regional forms, and broader DDM
      remain unimplemented. This has source/API
      traceability only and no catalog or conformance claim.
- [x] Exercise unnamed non-region `Register Object Instance`, generated names,
      `Discover Object Instance`, and the three known-instance support services
      in the non-installable development profile. The private registry assigns
      unique handles, snapshots currently published attributes, promotes to the
      closest subscribed superclass, rechecks queued discovery, and uses both
      callback models. Timestamped/retraction, DDM, ownership transfer,
      save/restore, FOM sharing policy, and resign-action object disposition
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
      save/restore, and resign-action disposition remain unimplemented. This
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
      projects them at each receiver's known class and current subscription,
      suppresses the source, rechecks queued delivery, and preserves
      tag/producer/type through both callback models. A separate regional
      object-attribute case covers committed overlap discovery/reflection and
      optional sent-region callback metadata. Timestamped/retraction behavior,
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
      filters discovery and no-time reflection by overlap, preserves the
      optional sent-region callback set, and treats empty region sets as no-ops.
      Additional regional forms, default-region synthesis,
      timestamped/retraction behavior, broader DDM
      routing, package/catalog evidence, and conformance remain out of scope.
      This has source/API traceability only.
- [x] Exercise the 2025 Attribute Scope Advisory path in the non-installable
      development profile. The per-federate advisory switch gates grouped
      `attributesInScope` / `attributesOutOfScope` callbacks for known objects
      whose committed regional overlap, update-region association, or ordinary/
      regional subscription changes. The integration case proves immediate and
      evoked delivery, callback-time stale-transition suppression, and the
      official switch accessors. Default-region synthesis,
      timestamped/retraction behavior, broader DDM routing, package/catalog
      evidence, and conformance remain separate. This has source/API
      traceability only.
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
      ownership. This is one current-recipient sweep only, not a continuing
      owner-search lifecycle for later joins, discovery, or publication.
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
      development profile. The limited receive-order path applies the
      FOM-selected mandatory type; custom types and message transport remain
      absent. This has API traceability only with no catalog or conformance
      claim.
- [x] Exercise the non-timestamped, non-region `Send Interaction` overload
      and matching no-time `Receive Interaction` callback in the
      non-installable development profile. The routing kernel chooses the
      closest subscribed class, projects available parameters, excludes the
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
      evoked delivery. The official `universally` argument is outside this
      bounded semantic slice; timestamped/retraction
      behavior, directed DDM, ordering, FOM sharing-policy enforcement,
      target-departure cleanup, packaging, Lab mapping resolution, evidence,
      and conformance remain outside this slice.
- [x] Exercise the 2025 regional `Subscribe Interaction Class With Regions`,
      `Unsubscribe Interaction Class With Regions`, and no-time `Send
      Interaction With Regions` overloads in the non-installable development
      profile. Regional declarations remain independent from ordinary
      subscriptions; committed region-set overlap gates delivery, empty sent
      sets suppress delivery, and queued callbacks recheck overlap. Remaining
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
