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
- [x] Account for all 181 official C++ `RTIambassador` API surfaces in the
      Catch2 plan, with individual Requirements-Lab API contracts where the
      source supports a bounded implementation slice. This is declaration and
      test-plan traceability only: it neither resolves the Lab's aggregate
      Connect mapping ambiguity nor establishes complete behavior, review,
      package, interoperability, or conformance evidence.
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
- [x] Extend the official C++ BasicDataElements implementation with the
      bounded 1516.2 Table 29 integer/pair family:
      HLAinteger16BE/LE, HLAunsignedInteger16BE/LE, HLAoctetPairBE/LE,
      HLAinteger32LE, HLAunsignedInteger32LE, HLAinteger64BE/LE, and
      HLAunsignedInteger64BE/LE. Catch2 pins signed two's-complement and
      unsigned endian vectors, pair ordering, two-, four-, and eight-octet
      boundaries, round trips, and malformed input. This remains a header/SDK
      foundation rather than complete basic-element support, adapter exposure,
      interoperability, or conformance; RL-053 records the Requirements Lab's
      table-row evidence boundary.
- [x] Add the raw-octet and ASCII helper slice from the official
      `RTI/encoding/BasicDataElements.h`: `HLAoctet`, `HLAbyte`,
      `HLAASCIIchar`, and `HLAASCIIstring`. Catch2 fixes one-octet raw-byte
      preservation, ASCII rejection, the signed `HLAinteger32BE` element
      count, and malformed input. The 1516.2 BasicDataElements contract is now
      also an independent CTest Requirements-Lab check. This remains bounded
      SDK foundation work, not generic constructed encoding, adapter,
      interoperability, or conformance evidence.
- [x] Add all four official IEEE-754 basic helpers: `HLAfloat32BE/LE` and
      `HLAfloat64BE/LE`. The implementation has an explicit 32-bit/64-bit
      IEC 559 platform requirement—consistent with the existing reference-time
      implementation—and converts the representation bits before applying the
      standard endian order. Catch2 fixes exact byte vectors, boundaries,
      round trips, and truncation behavior. This is still bounded SDK
      foundation work, not generic encoding, adapter, interoperability, or
      conformance evidence.
- [x] Complete the `RTI/encoding/BasicDataElements.h` helper catalog with
      `HLAunicodeChar` as exactly one UTF-16BE code unit. The implementation
      preserves a surrogate code unit, rejects a directly representable
      supplementary scalar, and fixes source-side unpaired-surrogate rejection
      in `HLAunicodeString` to match decode behavior. Direct helper definitions
      and source-derived Catch2 vectors are complete; generic constructed
      encoding, adapter exposure, interoperability, review/package evidence,
      and conformance remain open.
- [x] Implement the separate official `RTI/encoding/HLAopaqueData.h` C++
      helper. Its 1516.2 Table 35 representation is a dynamic
      `HLAvariableArray` of `HLAbyte`, so Catch2 fixes the signed
      `HLAinteger32BE` element count, raw byte payload, four-octet boundary,
      nested decode, and malformed-input behavior. The official external
      storage surface remains caller-owned borrowed memory: encoding observes
      caller changes and `set`/`decode` write within the declared capacity;
      insufficient capacity raises a deterministic error rather than
      reallocating, assuming ownership, or silently detaching. This is a
      bounded SDK foundation with separate 1516.1/1516.2 traceability, not
      generic constructed encoding, adapter exposure, interoperability, or
      conformance evidence.
- [x] Implement the official `RTI/encoding/HLAfixedRecord.h` C++ helper.
      Fields encode in declaration order starting at offset zero, with zero
      padding only between fields as required by the following field's octet
      boundary and no final padding. Catch2 covers the 1516.2 source example,
      reverse order, nesting, malformed padding, structural type checks,
      cloning, and the explicit non-owning raw-pointer policy. This is a
      bounded constructed-data foundation with direct 1516.2 traceability;
      generic arrays or variants, adapters, interoperability, and conformance
      remain open. RL-054 records the Requirements Lab provenance drift.
- [x] Implement the official `RTI/encoding/HLAfixedArray.h` C++ helper.
      Fixed cardinality, declaration-order elements at offset zero, zero
      inter-element padding at the prototype boundary, and no final padding
      follow §4.14.10.4 directly. Catch2 exercises fixed-record elements,
      nesting, malformed padding, templated wrappers, prototype cloning,
      type checks, and the header-documented caller-owned raw-pointer
      lifetime. This remains a bounded constructed-data foundation: FOM-level
      materialization, adapters, interoperability, and conformance remain open.
      RL-054 captures the source-candidate boundary.
- [x] Implement the official `RTI/encoding/HLAvariableArray.h` C++ helper.
      A signed `HLAinteger32BE` element count begins at offset zero; leading
      zero padding aligns the first element using the maximum of four and its
      octet boundary, while subsequent padding follows the fixed-array rule
      without a final pad. Catch2 fixes the §4.14.10.5 float64 vector and
      covers fixed-record elements, malformed count/padding data, cardinality
      changes during decode, templated wrappers, type checks, and the
      header-documented caller-owned raw-pointer lifetime. This remains a
      bounded constructed-data foundation: FOM-level materialization, adapters,
      interoperability, and conformance remain open. RL-044 captures the
      source-candidate boundary.
- [x] Implement the official `RTI/encoding/HLAvariantRecord.h` C++ helper.
      The discriminant begins at offset zero. A mapped alternative follows
      zero padding chosen from the maximum octet boundary of all alternatives;
      an unmapped discriminant has no following padding or value, and no
      padding follows a mapped alternative. Catch2 covers the max-boundary
      vector, malformed input, unmapped decoding, type/mapping controls,
      cloning, templated wrappers, and the header-documented borrowed
      raw-pointer lifetime. This remains a bounded constructed-data
      foundation: FOM-level enumerator-range/HLAother expansion, model-specific
      factories, adapters, interoperability, and conformance remain open.
      RL-054 records the source-candidate boundary.
- [x] Implement the official `RTI/encoding/HLAextendableVariantRecord.h` C++
      helper. It writes a discriminant at offset zero, zero padding to the
      four-octet `HLAinteger32BE` encoded-length field, and then zero padding
      to its predefined eight-octet alternative boundary. The encoded length
      excludes the latter padding and allows an unmapped alternative to be
      skipped. Catch2 fixes exact mapped, unmapped, nested, malformed, and
      max-boundary vectors; it also covers mapping/type checks, clones,
      templated wrappers, and the header-documented caller-owned raw-pointer
      lifetime. This remains a bounded constructed-data foundation: FOM-level
      enumerator-range/HLAother expansion, model-specific factories, adapters,
      interoperability, and conformance remain open. RL-054 records the
      source-candidate boundary.
- [x] Implement the official `RTI/encoding/HLAlogicalTime.h` and
      `HLAlogicalTimeInterval.h` helpers. They delegate opaque bytes, initial
      or zero values, and value copies to the selected `LogicalTimeFactory`;
      Umbra deliberately defines no bespoke logical-time wire form. Catch2
      covers both required reference factories, exact vectors, nested decode,
      mismatch and malformed input, and clones. `decodeFrom` is intentionally
      bounded to the current fixed-width reference profile because the factory
      API does not return a decoded byte count. Custom variable-width time
      providers, public service evidence, adapters, interoperability, and
      conformance remain open. RL-055 records the Lab provenance drift.
- [x] Implement the official `RTI/auth/HLAplainTextPassword.h` credential
      value with the exact `HLAunicodeString` representation, including
      malformed UTF-16/length rejection, plus the native reference
      `HLAauthorizer`, its factory, and the separate static
      `AuthorizerFactoryFactory` forwarding boundary. The private test seam
      verifies global-password matching without exposing a password in public
      settings. The currently unconfigured embedded authorization profile
      accepts the standard empty `HLAnoCredentials` form but rejects other
      supplied credentials with `Unauthorized` rather than silently accepting
      them; it does not invoke the reference authorizer. Secure RID
      configuration, runtime selection/lifecycle, Create/Destroy/Join checks,
      custom library loading, adapters, and conformance remain a later
      coordinated slice. RL-056 records the Lab provenance drift and RL-057
      the source/header factory-method spelling discrepancy.
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
- [x] Make the private FOM source boundary distinguish a missing designator
      from an existing non-regular/unreadable path, map the latter through
      embedded Create Federation Execution to `ErrorReadingFOM`, and record
      the Lab's missing row-level diagnostic relation as RL-153.
- [x] Produce a strict-OMT-positive complete-model vector. The FDD materializer
      now retains validated source model-identification metadata and inserts
      `Composed_From` references before the schema's trailing `other`/`glyph`
      fields. An Umbra-owned complete DIF fixture composes and validates under
      the official `IEEE1516-OMT-2025.xsd` in a dedicated Catch2/CTest lane.
      The official MIM+Restaurant composition remains a separate negative: its
      Restaurant custom basic-data row is partial, and RL-009 records the
      representation-reference tension between the 2025 examples and the
      OMT XSD's basic-data-only keyref. No metadata or encoding is invented to
      force that corpus through OMT.
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
      hierarchy and require an exact representation-type match. The dedicated
      IEEE 1516.2 exception for `HLAobjectInstanceName` and
      `HLAobjectInstanceHandle` is also enforced: it requires the standardized
      `HLAunicodeString` and `HLAobjectInstanceHandle` representations without
      inventing an ordinary attribute row. Its exact Lab candidate is selected
      by the private composition test plan; RL-087 records the earlier mapping
      omission.
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
      range updates, complete commit, owner-scoped support lookups,
      FOM upper-bound validation, handle decoding, and the §9.4 in-use guard
      (including passive regional subscriptions). The bounded interaction
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
       Direct time-constrained timestamped default-region callback coverage and
       default-source object and interaction mixed-fanout/retraction cases are
       now present. The default-source object-update companion also drives one
       ordinary timestamped passel through FQR, TARA, and NMRA, preserving
      callback-before-grant ordering and the supplied-empty region marker;
      a focused interaction companion also keeps one queued default-source
      passel across a Time Constrained disable/re-enable transition before
      its grant. A focused non-regional directed-interaction companion now
      keeps one queued target-qualified payload across the same Time
      Constrained disable/re-enable transition and proves exactly one
      directed callback before the matching grant with its original metadata;
      RL-134 records the missing cross-service lifecycle relation. A focused
      directed save/restore companion now preserves one queued target-qualified
      payload and its live retraction ledger across an untimed snapshot,
      delivers it through Flush Queue Request, and proves Request Retraction
      from the original designator after delivery; RL-135 records the missing
      save/restore relation. A focused explicit-source regional-interaction
      save/restore companion now preserves one overlap-qualified timestamped
      payload, its source RegionHandle set, and live retraction ledger across
      an untimed snapshot, delivers it through Flush Queue Request, and proves
      Request Retraction from the original designator; RL-136 records the
      missing regional save/restore relation. A focused
      explicit-source regional object-update companion
      also proves that replacing an update-region association before a queued
      callback suppresses the stale passel rather than retargeting it, while a
      later passel uses the replacement region. The remaining
      explicit-source regional object-update save/restore companion now also
      preserves one queued timestamped attribute passel, its source
      RegionHandle set, object/update association, and live retraction ledger
      across an untimed snapshot, then proves Flush Queue delivery and Request
      Retraction; RL-137 records the missing regional attribute recovery
      relation. A matching default-source interaction recovery companion now
      preserves one queued timestamped payload, its private default source,
      supplied-empty callback projection, and live retraction ledger across an
      untimed snapshot before Flush Queue delivery and Request Retraction;
      RL-138 records the corresponding default-source recovery relation.
      A matching non-regional attribute recovery companion now preserves one
      queued typed Update Attribute Values passel, its ordinary recipient
      ledger, and live retraction designator through an untimed snapshot before
      Flush Queue reflection and Request Retraction; RL-139 records the
      corresponding non-regional attribute recovery relation. A bounded
      three-member companion now restores that same queued passel for two
      constrained recipients, proves independent Flush Queue delivery, and
      requests retraction from each restored recipient; broader multi-member
      pending/in-transit recovery remains open. A one-member terminal-tombstone
      companion also preserves a saved timestamped attribute
      MessageCanNoLongerBeRetracted classification while rejecting a discarded
      post-save designator; terminal-lifetime coverage for the other payload
      families remains open. A matching one-member terminal-deletion restore
      companion now preserves the saved `MessageCanNoLongerBeRetracted`
      tombstone after deletion payload/object/name reclamation, rejects a
      distinct post-save handle, and restores the released-name state; other
      terminal-lifetime families remain open. A bounded two-member directed
      interaction companion now preserves a terminal timestamped designator
      alongside a separate live admission passel, rejects a discarded
      post-save directed handle, and keeps directed callback delivery outside
      the claim; regional and remaining terminal families remain open. A
      matching explicit-source regional-interaction companion now preserves a
      terminal timestamped designator across restore alongside an
      overlap-qualified admission passel, rejects a discarded post-save
      regional handle, and leaves regional callback/live-payload recovery
      outside the claim.
      A matching explicit-source regional-attribute companion now preserves a
      terminal timestamped Update Attribute Values classification across restore
      alongside a separate overlap-qualified admission passel, rejects a
      discarded post-save regional-attribute handle, and leaves regional
      callback/live-payload recovery outside the claim.
      A matching default-source regional-attribute companion now preserves a
      terminal timestamped Update Attribute Values classification across restore
      alongside a separate live private-default admission passel, rejects a
      discarded post-save default-region handle, and leaves callback/live-
      payload recovery outside the claim.
      A matching default-source regional-interaction companion now preserves a
      terminal timestamped Send Interaction classification across restore
      alongside a separate live private-default admission passel, rejects a
      discarded post-save default-region interaction handle, and leaves
      callback/live-payload recovery outside the claim.
      A matching default-source attribute recovery companion now preserves one
      ordinary-registration timestamped passel, its private default source,
      supplied-empty callback projection, and live retraction ledger through an
      untimed snapshot before Flush Queue reflection and Request Retraction;
      RL-140 records the corresponding default-source recovery relation.
      Timed counterparts now schedule save at logical time 6 while retaining
      timestamp-8 default-source attribute and interaction passels plus a
      target-qualified directed passel, cross the boundary for both members,
      and restore the post-save-terminalized designators for FQR
      reflection/interaction/directed delivery and Request Retraction; RL-141
      records the missing save-boundary relation. Durable
      restore, other payload/advance variants, region mutation, and broader recovery remain
      future slices. The remaining
      timestamped/re-enable matrix and broader DDM routing remain future slices.
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
       subscriber and remains supplied as an empty callback set; a further
       object and interaction cases split nonconstrained delivery and
       constrained pending retraction under that same private source
       realization. A bounded default-source object-update companion now also
       covers FQR/TARA/NMRA and producer-TAR frontiers for both object and
       interaction updates. A bounded interaction re-enable companion now
       retains one queued default-source passel across a Time Constrained
       disable/re-enable transition before its grant. A bounded default-source
       interaction save/restore companion now preserves one queued timestamped
       payload, its private source realization, supplied-empty callback
       projection, and live retraction ledger through an untimed snapshot
       before Flush Queue delivery and Request Retraction; RL-138 records the
       missing Lab cross-service relation. The regional object
       update slice also has a bounded explicit-source association-replacement
       regression covering stale-passel suppression and later replacement
       delivery. The remaining
       timestamped/re-enable matrix, broader relaxed-DDM coverage, timed/durable
      save/restore, transport, package evidence, and conformance remain
      separate.
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
- [x] Exercise the explicit `HLAfloat64Time` representation through the public
      embedded federation-management path: initial logical time,
      callback-gated regulation/lookahead, and time-advance grant mutation are
      covered by a focused Catch2/CTest lane and Requirements-Lab contract.
      This remains development-profile source/API traceability, not full time
      coordination, JUnit evidence, protected review, or conformance evidence.
- [x] Add an opt-in, non-installable federation-management development profile
      that binds official Create/Destroy/Join/Resign methods only after MIM-first
      preparation, FDD materialization, and reference-time selection.
- [x] Exercise the explicit `createFederationExecutionWithMIM` overload with a
      valid filesystem designator for the official 2025 MIM. The focused native
      case proves that the reserved `HLAstandardMIM` spelling remains rejected,
      while a validated local MIM path reaches the same atomic FDD/create/join
      lifecycle. This is development-profile source/API traceability only;
      custom-MIM interoperability, remote transport, package evidence,
      protected review, and conformance remain open.
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
       overload and matching timestamped `FederateAmbassador::initiateFederateSave`
       callback in the embedded profile. The official logical-time
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
      Restored with status responses. The HLA_EVOKED resignation boundary now
      fails an in-flight restore for surviving members and resets the departing
      ambassador's callback dispatcher so queued restore callbacks cannot leak
      after resignation. Catch2 covers object rollback, one
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
      floor. An HLA_EVOKED companion accepts FQR before two in-process future
      TSO inputs arrive and proves both callbacks precede FQG; the pure grant
      calculator also treats an explicit in-transit payload as undelivered.
      The external IEEE-JAR JNI/JPype companion now accepts FQR before either
      producer send, queues future timestamped interactions at 9 and 12, and
      proves both standard Java callbacks precede FQG with actual time 5 and
      optimistic time 9.
      Its mixed default-region companion fans one ordinary source interaction
      to immediate and constrained regional recipients, then proves standard
      request-retraction delivery to the immediate member while the queued
      constrained callback is suppressed before its time-6 grant.
      The external IEEE-JAR ordinary-interaction companion mirrors the native
      three-member retraction case, including a later timestamped send after
      the constrained member resigns and independent retraction-ledger
      retirement for the surviving immediate member.
      The external object-update companion applies that stale-recipient check
      to `ReflectAttributeValues`: a departing constrained member receives no
      queued reflection, while the survivor receives the original producer,
      timestamp/order, and retraction metadata before its grant.
      The matching external timestamped-delete companion applies the same
      recipient-generation rule to `RemoveObjectInstance`, preserving the
      survivor's removal/retraction metadata and suppressing the departed
      member's stale callback.
      Its shared actual-grant calculation also supports strict
      timestamped-save admission before the private grant-state change and FQG,
      including mixed FQR/TAR constrained-member readiness. It has separate
      Requirements-Lab source/API contracts and Catch2 coverage; remote future
      transport, network in-transit coordination, Request Retraction for other
      TSO message families, and full time-management coordination remain future
      work.
- [x] Bind the 2025 `Enable Asynchronous Delivery` and `Disable Asynchronous
      Delivery` services in the embedded profile. Time-constrained federates
      default to time-advance-only receive-order delivery; enabling the switch
      releases deferred receive-order callbacks while idle, and disabling it
      restores time-advance gating. A regional receive-order companion applies
      the same gate to an overlap-qualified `Send Interaction With Regions`
      callback and preserves its source-region metadata across disable/re-enable.
      The paired Requirements-Lab contracts and Catch2 cases cover the public
      exceptions and callback boundary. TSO
      delivery, remote transport, MOM reporting, save/restore persistence of
      deferred callbacks, package evidence, and conformance remain open.
- [x] Parse the complete 2025 FDD support-switch table and bind the official
      Convey Region Designator Sets, Automatic Resign Directive, Service
      Reporting, Exception Reporting, Send Service Reports To File, Delay
      Subscription Evaluation, and Allow Relaxed DDM accessors in the embedded
      profile. Per-federate values are independent; static federation-wide
      values are captured at creation. The exact MOM report-service
      subscription/switch interlock is also covered for ordinary and regional
      subscriptions. This has Requirements-Lab contracts and Catch2 coverage;
      the bounded embedded Connection Lost path exercises directives 1 through
      5 and delivers `HLAreportFederateLost` to current ordinary or
      DDM-matching regional subscribers with a regulator's captured granted
      time; its ordinary and DDM-regional subscriber routes have direct
      `HLA_IMMEDIATE` coverage. The bounded `NO_ACTION` forced-resign policy
      now retains a known object, divests the lost member's owned attributes,
      and offers them to an eligible survivor without an automatic removal
      callback. The broader page-50 TSO-cutoff matrix beyond the
      current ordinary-interaction equality/strict-less-than, directed-
      interaction equality plus a `DELETE_OBJECTS` late-TAR collision,
      attribute-update equality/multi-recipient/automatic-delete collisions
      (including late-TAR, callback-boundary suppression, and independently
      gated survivor cleanup plus asynchronous postgrant release in both
      callback models), object-
      deletion equality/strict-less-than/automatic-delete/multi-recipient, and
      report/cutoff-correlation regressions, generic
      §11.5 report emission/file behavior, broader relaxed-DDM behavior,
      package evidence, and conformance remain open. RL-024 records the
      unresolved Lab/XSD automatic-resign default discrepancy.
- [x] Implement an explicit, bounded Allow Relaxed DDM policy at the shared
      committed-region overlap predicate. With the static federation switch
      enabled, only ranges that exactly touch at a boundary are added to the
      strict-overlap set; a positive gap never overlaps and strict overlap is
      preserved. Focused Restaurant-FOM interaction and object-attribute
      regressions exercise disabled, enabled, gapped, and strict cases, with
      the latter also proving discovery/reflection gating. `../design/RELAXED-DDM-POLICY.md`
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
      establish object discovery to isolate declaration timing. Explicit
      update-region attribute and regional interaction passels now have bounded
      timestamped regressions: an enabled federation can deliver after a
      regional declaration is restored, while both modes suppress a later
      callback after unsubscription. Directed interactions, complete
      callback-mode/lifecycle/retraction matrices, relaxed DDM, packaging, and
      conformance remain separate in this embedded Catch2 profile; the Python
      native and Java-provider paths now exercise the explicit regional
      interaction/update callback-boundary cases. RL-018 logs the Requirements
      Lab metadata mismatch for the source §8.1.8 candidates.
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
      `compliance/requirements-lab/timestamped-interaction-requirements-contract.json` traces
       this bounded public slice. A focused ordinary non-regional companion
      now also drives one queued timestamped interaction through independent
      FQR, TARA, and NMRA recipients, proving callback-before-grant ordering
      and preserving FQR actual/optimistic time plus timestamp/order/tag/
      producer metadata. A separately contracted ordinary lifecycle companion
      now queues the same non-regional service across a Time Constrained
      disable and callback-gated re-enable, proving one callback before the
      matching grant without DDM/source-region state. These remain bounded
      in-process frontiers; the item does not claim timestamped
      object/attribute updates, directed/regional interactions, a complete
      available-advance or re-enable matrix, cross-family request-retraction
      callbacks, transport, or conformance.
- [x] Add a bounded Request Retraction vertical slice for normal non-regional
      timestamped `Send Interaction`, `Update Attribute Values`, and `Send
      Directed Interaction`, plus region-context `Send Interaction With
      Regions` and bounded regional `Update Attribute Values`. A
      federation-owned recipient ledger distinguishes delivered and queued
      fanout; a legal Retract invokes `Request Retraction` for a delivered
      recipient and suppresses a queued recipient's original callback. Catch2
      covers strict equality rejection, post-delivery callbacks, mixed
      nonconstrained/constrained interaction, directed-interaction, and
       region-context interaction fanout, normal and regional pending passel
       and callback-boundary attribute suppression, and immediate-only attribute-update,
      directed-interaction, and region-context interaction cases with no
      temporal-queue fanout.
      `compliance/requirements-lab/request-retraction-requirements-contract.json` and its API
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
       immediate recipient's `Request Retraction` callback. A Connection Lost
       counterpart now retains one accepted reliable passel across staggered
       time-6 deliveries to two constrained recipients;
       `compliance/requirements-lab/timestamped-attribute-update-
       requirements-contract.json` traces this bounded slice. A focused
       ordinary non-regional lifecycle companion now preserves one queued
       timestamped attribute passel across callback-gated Time Constrained
       disable/re-enable and proves exactly one reflection before the matching
       grant with payload, tag, producer, timestamp/order, and retraction
       metadata. Its Requirements-Lab and API links are recorded in
       `compliance/requirements-lab/timestamped-attribute-update-requirements-contract.json`
       and `compliance/requirements-lab/timestamped-attribute-update-api-contract.json`.
       Timestamped
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
      sender exclusion, and Connection Lost time-5/time-6 cutoff removals
      retained through the lost regulator's configured unconditional-divest cleanup;
      a `DELETE_OBJECTS` regression additionally protects that accepted TSO
      removal from being replaced by automatic receive-order cleanup; a
      staggered two-survivor regression keeps the cutoff state live until both
      timestamped removals have been delivered;
      `compliance/requirements-lab/timestamped-object-deletion-requirements-contract.json` and
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
       at unchanged lookahead. A positive mixed-advance companion now drives one
       timestamped deletion through FQR, TARA, and NMRA, proving each
       Remove Object Instance callback precedes its own grant and preserving
      FQR actual/optimistic time plus the producer TAR completion. A separately
      contracted ordinary lifecycle companion now carries one timestamped
      removal across a Time Constrained disable and callback-gated re-enable,
      proving one removal callback before the matching grant without introducing
      DDM or alternate-advance state. Complete alternate-advance coverage
      beyond this case, broader re-enable, active in-flight ownership, other
      resignation, and save/restore recovery evidence, transport, and
      conformance remain out of scope. A separately contracted save/restore
      companion now saves the live queued deletion, terminalizes it after the
      save, restores the snapshot, and proves Remove Object Instance through FQR
      followed by Request Retraction and object/name reconstitution. Timed or
      durable restore and changed-membership/ownership recovery remain open.
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
      shared planner honors ownership and universal directed subscriptions.
      The focused timestamped selector scenario now distinguishes a target
      owner/default recipient, a known non-owner/default recipient, and a
      known non-owner/universal recipient, then rechecks a selector change and
      an unsubscription before later grants. Directed DDM, alternate advance
      modes beyond the new FQR/TARA/NMRA companion, region-context evidence,
      transport, and conformance remain out of scope. The companion sends one
      target-qualified timestamped payload to three universal recipients;
      each directed callback precedes its own grant, FQR preserves actual and
      optimistic time, and the producer TAR completes independently.
- [x] Close the timestamped §6.14 directed service-report backend gap. The
      accepted `Send Directed Interaction(..., LogicalTime)` path now uses the
      shared file-or-interaction selector, and the focused filesystem lane
      preserves type-27/type-37/type-40/type-63/type-31 supplied forms, the
      type-34 Null return for a non-time-regulating sender, serial zero, and
      report-before-directed-callback ordering. Time-regulated type-33 return
      records, directed DDM, transport, and conformance remain separate.
- [x] Close the timestamped §6.12 ordinary interaction service-report backend
      gap. The accepted `Send Interaction(..., LogicalTime)` path now uses the
      shared file-or-interaction selector, and the focused filesystem lane
      preserves type-27/type-40/type-63/type-31 supplied forms, the type-34
      Null return for a non-time-regulating sender, serial zero, and
      report-before-`Receive Interaction` ordering. Regional/timestamped
      object-update/delete reports, time-regulated type-33 records, transport,
      and conformance remain separate.
- [x] Close the timestamped §6.16 object-deletion service-report backend gap.
      The accepted `Delete Object Instance(..., LogicalTime)` path now uses the
      shared file-or-interaction selector, and the focused filesystem lane
      preserves type-37/type-63/type-31 supplied forms, the type-34 Null return
      for a non-time-regulating sender, serial zero, and durable
      sender-report-before-`Remove Object Instance` ordering. Regional deletion,
      recipient-local callback file matrices, time-regulated type-33 records,
      transport, and conformance remain separate.
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
      interaction Requirements Lab contracts trace this bounded slice. Separate
      default-source and explicit-source interaction companions now exercise
      ordinary TAR/NMR plus TARA and NMRA at inclusive GALT/next-message
      boundaries; further
      alternate advances, object-region services, relaxed DDM, other fanout patterns,
      save/restore, transport, and conformance remain out of scope.
- [x] Close the accepted timestamped regional-interaction service-report
      backend gap. The `Send Interaction With Regions(..., LogicalTime)` path
      now uses the shared filesystem-or-MOM selector at TSO admission, and the
      focused filesystem lane preserves type-27/type-40/type-43/type-63/type-31
      supplied forms, the type-33 `MessageRetractionHandle` return, serial zero,
      and durable sender-report-before-regional-`Receive Interaction` ordering.
      Its paired HLA_IMMEDIATE public-MOM case now decodes service type 2, all
      five supplied forms, the quoted type-33 return, success/empty-exception
      fields, and serial zero before the constrained callback, then verifies
      source-region and timestamp/order/retraction metadata. Regional failure
      matrices, regional object-update/delete reports, transport, and
      conformance remain separate.
- [x] Cover the timestamped regional interaction service-report failure
      matrix. The focused filesystem and HLA_IMMEDIATE MOM lanes exercise
      invalid interaction-class, parameter, region, and logical-time inputs
      for `Send Interaction With Regions(..., LogicalTime)`, preserving serials
      zero through three, type-27/type-40/type-43/type-63/type-31 supplied
      forms, Null returns, false indicators, exact exception text, and no
      application callback. RL-105/RL-152 leave the conditional failure
      relation outside Requirements-Lab validation; broader regional failure,
      lifecycle, transport, and conformance families remain separate.
- [x] Cover the ordinary regional interaction service-report failure matrix.
      The focused filesystem and HLA_IMMEDIATE MOM lanes exercise invalid
      interaction-class, parameter, and region inputs for `Send Interaction
      With Regions`, preserving serials zero through two,
      type-27/type-40/type-43/type-63/type-34 supplied forms, Null returns,
      false indicators, exact exception text, and no application callback.
      RL-105/RL-152 leave the conditional failure relation outside
      Requirements-Lab validation; timestamped and other regional failure,
      lifecycle, transport, and conformance families remain separate.
- [x] Close the accepted ordinary regional-interaction service-report backend
      gap. The no-time `Send Interaction With Regions` path now has paired
      filesystem and HLA_IMMEDIATE MOM Catch2 cases preserving service type 2,
      type-27/type-40/type-43/type-63/type-34 supplied forms, the type-34 Null
      return, success true, serial zero, and report-before-constrained-callback
      ordering with source `RegionHandleSet` verification. Its focused
      `ordinary-regional-interaction-service-report` lane remains separate from
      timestamped and failure matrices; RL-105/RL-152 still leave the backend
      relation outside Lab validation.
- [x] Close the accepted regional interaction-subscription MOM-report gap.
      The §9.10/§9.11 `Subscribe/Unsubscribe Interaction Class With Regions`
      pair now has a native HLA_IMMEDIATE Catch2 decoder in addition to the
      configured filesystem lane. It preserves DDM service type 5,
      type-27/type-43 supplied forms, the type-6 passive-indicator inversion
      for a passive subscription followed by active replacement, Null returns,
      success/empty-exception fields, and serials zero through two. RL-152
      still leaves the conditional backend/lifecycle relation outside Lab
      validation; timestamped/retraction behavior, transport, packaging,
      and conformance remain separate.
- [x] Close the accepted timestamped regional attribute-update service-report
      backend gap. The `Update Attribute Values(..., LogicalTime)` path already
      uses the shared filesystem-or-MOM selector; the focused filesystem lane
      proves that an explicit committed regional object/attribute association
      does not bypass it, preserving type-37/type-2/type-63/type-31 supplied
      forms, the type-33 `MessageRetractionHandle`, serial zero, and durable
      sender-report-before-regional-`Reflect Attribute Values` ordering. The
      callback's source `RegionHandleSet` remains a separate delivery
      projection. The same focused lane now includes an HLA_IMMEDIATE public
      MOM decoder for the accepted service type 2 interaction, all four
      standard supplied forms, the quoted type-33 return, success/empty
      exception, and serial zero before the constrained callback. Failed
      regional-update matrices, regional deletion, re-enable/save/restore,
      transport, and conformance remain separate.
- [x] Cover the timestamped regional attribute-update service-report failure
      matrix. The focused filesystem lane exercises invalid object, invalid
      attribute, and invalid logical time after an explicit committed regional
      object/attribute association, preserving serials zero through two,
      type-37/type-2/type-63/type-31 supplied forms, Null returns, false
      indicators, exact exception text, and no callback delivery. Public MOM
      interaction delivery for this failure matrix is now covered by a
      matching HLA_IMMEDIATE decoder; other regional failure families,
      lifecycle, transport, and conformance remain separate.
- [x] Close the accepted ordinary regional attribute-update service-report
      backend gap. The no-time `Update Attribute Values` path already uses the
      shared filesystem-or-MOM selector; the focused filesystem lane proves
      that an explicit committed regional object/attribute association does not
      bypass it, preserving serial-zero type-37/type-2/type-63/type-34 sender
      forms and durable sender-report-before-regional-`Reflect Attribute
      Values` ordering. The callback's source `RegionHandleSet` remains a
      separate delivery projection. The ordinary invalid-object and
      invalid-attribute file matrix is covered separately. A paired
      HLA_IMMEDIATE public-MOM case now decodes service type 2,
      type-37/type-2/type-63/type-34 supplied forms, the type-34 Null return,
      success/empty-exception fields, and serial zero before the constrained
      callback, which verifies the source `RegionHandleSet` and receive-order
      metadata. The paired ordinary invalid-object/invalid-attribute failure
      matrix now also has an HLA_IMMEDIATE decoder preserving service type 2,
      the same four supplied forms, Null return, false indicator, exact
      exception text, and serials zero and one without reflection delivery.
      Other regional-update failure forms, re-enable/save/restore, transport,
      and conformance remain separate.
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
       queued until their grant. A focused immediate-recipient companion moves
       an initially overlap-qualified region to a valid disjoint range before
       callback dispatch, proving terminal reflection suppression and no
       synthetic `Request Retraction` after legal `Retract`; its
       time-constrained counterpart reaches only the matching grant before the
       sender's strict retraction boundary. Direct timestamped default-region callback coverage now has
       separate object and interaction regressions; a class-level regional
       request and explicit no-time response are separately tracked. A focused
       non-regional companion now exercises TARA at the defined-GALT boundary
       and NMRA at the next queued-message boundary, preserving
       reflection-before-grant ordering and source/time/order/retraction/tag
       metadata. A second non-regional companion covers FQR/Flush Queue by
       delivering all queued passels before the grant and preserving the
       actual-grant and optimistic-floor values. A separate HLA_EVOKED
       interaction companion covers in-process future input submitted after
      FQR acceptance but before callback dispatch. Regional explicit-source and
      default-source interaction companions preserve source-region metadata
      before FQG, including the supplied-empty marker for the private default
      region. A regional TAR/NMR companion now drives one overlap-qualified TSO
      payload through ordinary grants and proves each callback precedes its own
      grant. An explicit-source regional interaction companion now keeps its
      queued TSO payload and source RegionHandle through a Time Constrained
      disable/re-enable transition and proves exactly one callback before the
      post-re-enable grant. The corresponding default-source object-update
      companion now preserves an ordinary-registration timestamped passel across
      the same transition and proves one reflection before the post-re-enable
       grant with a supplied-empty default-region marker. The matching
       explicit-source regional object-update companion now preserves its source
       RegionHandle, callback order, timestamp, tag, and conveyed-region metadata
       across the same transition. A regional timestamped attribute-update companion now drives one
       overlap-qualified object passel through ordinary TAR/NMR grants and
       preserves its source-region metadata before each grant. A mixed-member
       regional interaction companion drives its payload through FQR, TARA, and
       NMRA recipients and proves each callback precedes its own grant. A
       matching mixed-member regional object-update companion now does the same
       for Reflect Attribute Values, preserving source-region metadata at each
       callback frontier. Its negative retraction companion withdraws an
       overlap-qualified timestamp-8 passel before all three alternate
       callbacks, proving FQR/TARA/NMRA and producer-TAR completion without
       reflection or Request Retraction. Remote future-input/in-transit FQR, regional directed/default-
       region forms outside these bounded cases, while the default-source
       TARA/NMRA matrix now includes bounded object- and interaction-update
       companions but remains in-process only,
       re-enable, save/restore, transport, package
       evidence, and conformance remain open.
- [ ] Extend timestamped public delivery to the remaining 2025 object,
      interaction, ownership, and federation service families only after their
      payload, callback-order, eligibility, and retraction semantics receive
      separate Requirements Lab contracts and real Catch2 scenarios. This
       excludes the now-bounded non-regional timestamped deletion/removal
       retraction path, but includes its remaining alternate-time cases beyond
       the focused non-regional deletion and attribute-update TAR/TARA/NMRA/FQR
       coverage plus the new directed-interaction FQR/TARA/NMRA case,
       in-flight-ownership, resignation, recovery, and transport cases. The
      timestamped federation-save control overload is tracked by the checked
      item above; it does not make the remaining timestamped delivery families
      complete.
- [x] Record selected time-management and FOM stress scenarios from the legacy
      Python RTI in a revision-pinned, non-normative
      [backlog](../testing/LEGACY-PYTHON-RTI-TEST-BACKLOG.md),
      [resource register](../testing/LEGACY-PYTHON-RTI-TEST-RESOURCES.md), and
      [FOM stress-corpus backlog](../fom/FOM-STRESS-CORPUS-BACKLOG.md), without
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
- [x] Enforce the 2025 object-attribute and interaction-parameter data-type
      category rule in the private composition preflight. Both columns now
      reject a raw basic-data representation after complete-model resolution,
      while `HLAtoken` remains valid through its official array-data
      declaration. The generic resolver recognizes the complete schema key;
      each table-specific predicate determines the valid declaration family.
      The paired 1516.2 Requirements-Lab contract and Catch2 case are
      traceability only; the paired attribute-`NA` companion predicate and
      remaining table constraints remain separately scoped.
- [x] Enforce the bounded 2025 attribute-`NA` direct companion rule in the
      private composition preflight. For an attribute whose data type is `NA`,
      supplied transportation/order values must be non-`NA`, while supplied
      update type/update condition values must be `NA`. A partial DIF attribute
      can be completed by a later compatible module before FDD materialization;
      it does not receive invented values. The DIF schema has no per-attribute
      available-dimensions field, so Umbra does not turn that source phrase
      into a class-wide restriction (RL-051). The paired 1516.2 Requirements-
      Lab contract and Catch2 case are traceability only; the separate
      `Static`/Update Condition tension, remaining table constraints, and
      conformance remain open.
- [x] Enforce the bounded 2025 attribute `sharing=Neither` / Value Required
      companion rule in the private composition preflight. A supplied Value
      Required field must be `false`; an omitted field remains representable in
      a partial DIF row. The paired 1516.2 Requirements-Lab contract and
      Catch2 case are traceability only; the rule does not infer defaults,
      FOM/SOM declaration state, runtime behavior, or conformance. RL-052
      records the source candidate's broad `clause-4` provenance.
- [x] Enforce the bounded 2025 dynamic-attribute Update Condition predicate in
      the private composition preflight. A supplied condition for a
      Conditional or Periodic update type must be nonempty and non-NA; an
      omitted condition remains representable in a partial DIF row. This does
      not parse periodic-rate grammar or initial-condition prose. The paired
      1516.2 Requirements-Lab contract and Catch2 case are traceability only.
      The separate Static/NA direction remains deferred under RL-046 because
      supplied official 2025 material uses Static with On change.
- [x] Enforce the 2025 standard root-hierarchy rule in the private composition
      preflight. A completed object or interaction table must be rooted by
      `HLAobjectRoot` or `HLAinteractionRoot`; omitted tables remain valid for
      incomplete DIF modules. The paired 1516.2 Requirements-Lab contract and
      Catch2 case are traceability only. The separate Static/NA direction is
      logged as RL-046 and is intentionally not enforced through a homegrown
      rejection rule.
- [x] Enforce the 2025 enumerated-data representation rule in the private
      composition preflight. A supplied enumerated representation must resolve
      to a basic-data declaration, while an omitted DIF field remains
      representable. The paired 1516.2 Requirements-Lab contract and Catch2
      case are traceability only; the distinct simple-data interpretation
      remains deferred under RL-009.
- [x] Enforce the 2025 array-data Element Type category rule in the private
      composition preflight. A supplied element type must resolve to a simple,
      enumerated, reference, fixed-record, array, or variant-record declaration;
      raw basic-data representations fail after complete-model resolution.
      Omitted Element Type remains representable for incomplete DIF rows. The
      paired 1516.2 Requirements-Lab contract and Catch2 case are traceability
      only; remaining data-type/table constraints remain open.
- [x] Enforce the 2025 fixed-record Field Type and variant-record Alternative
      Type category rules in the private composition preflight. Supplied member
      types must resolve to a simple, enumerated, reference, fixed-record,
      array, or variant-record declaration; raw basic-data representations fail
      after complete-model resolution, while omitted member types remain
      representable for incomplete DIF rows. The paired 1516.2 Requirements-Lab
      contract and Catch2 case are traceability only; RL-047 records that the
      exact source candidates currently export broad `clause-4` provenance.
- [x] Enforce the 2025 variant-record Discriminant Type category rule in the
      private composition preflight. A supplied discriminant type must resolve
      specifically to an enumerated-data declaration; raw basic-data
      representations, other declaration families, and `NA` fail after
      complete-model resolution. An omitted field remains representable for an
      incomplete DIF row. The paired 1516.2 Requirements-Lab contract and
      Catch2 case are traceability only; companion enumerator slices cover
      membership and range semantics, while broader encoding/conformance work
      remains deferred.
- [x] Enforce the 2025 variant-record Discriminant Enumerator lexical rules in
      the private composition preflight. Supplied fields use comma-separated
      enumerators or bracketed two-endpoint ranges; `HLAother` is standalone
      and can occur only once per record. Omitted fields remain representable
      for incomplete DIF rows. The paired 1516.2 Requirements-Lab contract and
      Catch2 case are traceability only; companion slices cover membership and
      range semantics, while broader encoding/conformance work remains
      deferred.
- [x] Enforce the 2025 variant-record Discriminant Enumerator membership
      boundary in the private composition preflight. Each supplied individual
      enumerator and range endpoint must be declared by the selected
      enumerated-data type after the complete module set merges. An
      enumeration with no supplied members remains representable as incomplete
      DIF. The paired 1516.2 Requirements-Lab contract and Catch2 case are
      traceability only; RL-049 records the source candidate's broad
      `clause-4` provenance, while the companion range-semantics slice covers
      table-order expansion, overlap, and `HLAother` complement behavior.
- [x] Enforce bounded 2025 variant-record Discriminant Enumerator range
      semantics in the private composition preflight. Enumerator-table source
      declaration order is retained; bracketed ranges expand over the inclusive
      table span, and a member cannot be assigned through direct/range entries
      to more than one named alternative after composition. `HLAother` is
      represented as the complement of explicit members, while a selected
      enumeration with no declared members remains incomplete DIF. The paired
      1516.2 Requirements-Lab contract and Catch2 case are traceability only;
      RL-049 and RL-050 record the candidate provenance limits. Complete Annex
      C merge, runtime encoding, and conformance remain open.
- [x] Enforce the 2025 Dimension-table Input data type category rule in the
      private composition preflight. Each supplied input type must resolve to a
      simple, enumerated, reference, fixed-record, array, or variant-record
      declaration; raw basic-data representations fail after complete-model
      resolution, while `NA` remains valid. Suitable-type selection and the
      semantic judgment of input-description text remain separately scoped.
      The paired 1516.2 Requirements-Lab contract and Catch2 case are
      traceability only; RL-048 records the candidate's broad `clause-4`
      provenance.
- [x] Enforce the bounded 2025 Dimension-table Input data type `NA`-marker
      exclusivity rule in the private composition preflight. The no-suitable-
      type `NA` marker cannot coexist with a supplied named input type in one
      DIF `inputDataTypes` sequence. The paired 1516.2 Requirements-Lab
      contract and Catch2 case are traceability only; suitability, cardinality,
      duplicate names, description unambiguity, and complete Annex C merge
      remain open.
- [x] Enforce the bounded 2025 Dimension-table no-named-input description rule
      in the private composition preflight. An empty DIF `inputDataTypes` list
      (or the retained explicit `NA` marker) requires non-`NA` Input data type
      description text. Named inputs retain both official Restaurant forms:
      `NA` when no amplifying text is needed and textual amplification when it
      is useful. The paired 1516.2 Requirements-Lab contract and Catch2 case
      are traceability only; suitable-type selection and whether a description
      is unambiguous remain open.
- [x] Enforce the 2025 strictly positive supplied update-rate table constraint
      in the private composition preflight. Incomplete DIF rows remain
      representable, while supplied zero/non-positive values fail with explicit
      diagnostics; the 2025 FOM XSD already enforces positive dimension upper
      bounds. The paired 1516.2 Lab contract and Catch2 case are traceability
      only; lookahead-sign and remaining table constraints remain open. RL-144
      records that the lookahead rule lacks a machine-readable DIF/XSD domain
      predicate, so Umbra does not infer signedness from the standard carrier.
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
- [x] Retain the completed 2025 synchronization table in the immutable FOM
      catalog. Each declared label now carries its tag data type, capability,
      semantics, and remapped note references, with deterministic label lookup
      and enumeration. Live registration/achievement state remains owned by the
      federation registry; this projection does not invent a second public
      synchronization API or claim conformance.
- [x] Apply the 2025 Annex C.5 same-label synchronization-point composition
      rule. Identical duplicates are ignored against the first definition;
      differing label/data-type/capability/semantics/note sub-elements fail
      composition before FDD materialization. Equivalent referenced note
      content remains equal across Umbra's per-module note-label remapping.
      The paired Requirements-Lab contract and Catch2 scenario are private
      composition traceability only.
- [x] Apply the 2025 Annex C.6 same-name transportation-type composition rule.
      Identical duplicates are ignored against the first definition; differing
      name/semantics/note sub-elements fail composition before FDD materialization;
      equivalent referenced note content survives per-module label remapping.
      The paired Requirements-Lab contract and Catch2 scenario are private
      composition traceability only and do not claim transport behavior.
- [x] Apply the 2025 Annex C.7 same-name update-rate composition rule. Identical
      duplicates are ignored against the first definition; differing
      name/rate/semantics/note sub-elements fail composition, while omitted DIF
      fields remain representable for partial rows. Equivalent referenced note
      content survives per-module label remapping. The paired
      Requirements-Lab contract and Catch2 scenario are private composition
      traceability only.
- [x] Apply the 2025 Annex C.4 same-name dimension composition rule. Identical
      duplicates are ignored against the first definition; differing
      name/data-type/upperBound/normalization/value/note sub-elements fail
      composition, while equivalent referenced note content remains stable
      across Umbra's per-module note-label remapping and unique dimension names
      are inserted into the composed dimensions table. The paired
      Requirements-Lab contract and Catch2 scenario are private composition
      traceability only.
- [x] Enforce the 2025 data-type name-uniqueness boundary after module
      composition. Independently valid DIF modules that introduce a same-name
      basic/simple data type conflict now fail before catalog/FDD materialization,
      covering the 4.14.1 uniqueness rule and the Annex C.3 differing-element
      merge failure. The paired Requirements-Lab contract and Catch2 scenario
      are private composition traceability only.
- [x] Verify the installed SDK's IEEE 1516.2 resource payload during package
      smoke testing. The staged schemas, MIM, Restaurant examples, and reviewed
      digest manifest are checked after `cmake --install`, before the consumer
      package is configured. This closes resource-copy integrity only; the
      embedded federation-management profile remains non-installable while its
      libxml2 dependency and runtime resource lookup are designed separately.
- [x] Canonicalize the official 2025 `switchType/@isEnabled` omission default
      before Annex C.8 duplicate comparison. An omitted boolean and an explicit
      `false` now compose equivalently in either module order without a spurious
      warning or catalog difference. Automatic Resign Action remains outside
      this normalization because RL-024 records its unresolved Lab/XSD default
      precedence conflict; this is FOM traceability only, not conformance.
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
       Connect. Bounded multi-federate cases set `DELETE_OBJECTS`,
       `UNCONDITIONALLY_DIVEST_ATTRIBUTES`, and `DELETE_OBJECTS_THEN_DIVEST`
       through the official support service, proving delete-privileged removal,
       current eligible ownership-assumption delivery, and the combined
       delete-before-divest path respectively. A fourth case sets
       `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`, suppresses a stale queued owner
       release request after loss, and proves a current surviving candidate can
       receive the later assumption offer. A fifth case sets
       `CANCEL_THEN_DELETE_THEN_DIVEST` and proves all three mandated stages:
       stale acquisition cleanup, known delete-privileged object removal, and
       retained-attribute re-offer. A bounded `NO_ACTION` forced-resign case
       now proves retained-object, ownership-divestiture, and survivor-
       assumption behavior without automatic removal. A separate final-member
       transport-loss case now proves the 4.12.4 directive-two override by
       rejoining and reusing the deleted object's name. Remote transport, the
       remaining forced-resign matrix is still open, although a focused
       directive-three case now cancels a pending negotiated transfer and its
       stale owner callbacks. A three-member report-ordering case now drains
       each eligible survivor independently and proves `HLAreportFederateLost`
       precedes that survivor's automatic `Remove Object Instance` callback;
       it intentionally makes no global ordering claim between callback
       queues. A selector-mutation companion now changes a queued directed
       recipient from universal to by-ownership after the source fault,
       suppresses only the stale directed callback, and releases the separate
       automatic removal at its next receive-order gate. A regional companion
       commits the receiver to a disjoint range after a queued timestamped
       attribute update and source fault, suppressing only the regional
       reflection while retaining the same independent cleanup boundary.
       Package support, protected review, and conformance remain open.
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
      `HLAprivilegeToDeleteObject`, directive 3 now has standalone voluntary
      cancellation regressions for regular pending acquisition work, a
      selected negotiated-divestiture confirmation, and an If Available
      reservation; the latter two suppress their stale owner/requester callback
      forms, and direct
      directive 4 now has a mixed voluntary delete-then-divest regression that
      deletes one privileged object before re-offering a retained attribute;
      directive 5
      cancels the resigning federate's pending acquisition work before applying
      delete/divest cleanup.
      The final-federate rule also forces directive 2 even when the supplied
       action is `NO_ACTION`. The official `FederateOwnsAttributes` and
       `OwnershipAcquisitionPending` preconditions are covered. Bounded
       continuation after later publication, discovery, and join is covered;
      terminal callback re-search/arbitration, remaining automatic-resign
      directive combinations, RTI-owned state, remote transport, and
      conformance remain separate work;
      this item has source/API traceability only.
- [x] Close the bounded terminal-callback reservation hole in the
      Unconditional Attribute Ownership Divestiture assumption search. If a
      candidate unpublishes before its queued `Request Attribute Ownership
      Assumption` callback, the stale callback is suppressed and its candidate
      reservation is released; a later publication receives one fresh grouped
      offer carrying the originating divestiture tag while the attribute
      remains unowned. The focused
      `ownership-assumption-research` Catch2/Requirements-Lab lane is source/API
      traceability only. Full ownership arbitration, RTI-owned state, the
      remaining divestiture/resignation matrix, transport, package/JUnit/
      protected-review evidence, and conformance remain open.
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
- [x] Complete public API traceability for the eight official handle-decoding
      services in the non-installable development profile: federate,
      object-class, interaction-class, object-instance, attribute, parameter,
      dimension, and message retraction. The seven previously inherited
      decoders now enforce the public connection/member boundary before using
      their matching typed decoder; the focused regression also covers the
      preexisting message-retraction decoder. It proves those boundary
      exceptions, public-service round trips, and malformed-encoding rejection.
      The Requirements Lab exports API surfaces but no direct requirement IDs
      for this group, so this has API traceability only—not interoperability,
      package, protected-review, or conformance evidence.
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
      complete update-rate producer/timing evidence, remaining object-attribute regional forms, and
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
      complete update-rate producer/timing evidence, package evidence, and conformance remain
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
- [x] Add paired filesystem and HLA_IMMEDIATE failure-report matrices for the
      §6.2/§6.4 single-name reservation and release services. The focused lane
      preserves type-53 Name arguments, Null returns, false indicators, exact
      exception text, and serial ordering around accepted calls; RL-152 keeps
      the evidence at development traceability rather than Lab validation or
      conformance.
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
- [x] Add paired filesystem and HLA_IMMEDIATE failure-report matrices for
      §6.18 `Local Delete Object Instance`. The focused lane preserves type-37
      supplied handles, Null returns, false indicators, exact
      `ObjectInstanceNotKnown` text, and serial ordering around the accepted
      local-forget record; RL-152 keeps this at development traceability rather
      than Lab validation or conformance.
- [x] Add paired filesystem and HLA_IMMEDIATE failure-report matrices for
      receive-order §6.16 `Delete Object Instance`. The focused lane preserves
      type-37 object handles, type-63 base-64 user tags, type-34 Null optional
      timestamps, Null returns, false indicators, exact
      `ObjectInstanceNotKnown` text, and serial ordering around the accepted
      serial-one deletion. Accepted emission is outside native locks; RL-152
      keeps this at development traceability rather than Lab validation or
      conformance.
- [x] Add paired filesystem and HLA_IMMEDIATE failure-report matrices for the
      timestamped §6.16 `Delete Object Instance` pre-admission boundary. The
      focused lane preserves type-37/type-63/type-31 supplied forms, Null
      returns, false indicators, exact `ObjectInstanceNotKnown` and
      `InvalidLogicalTime` text, and serials zero and one. Retraction results
      and recipient-local §6.17 callback reporting remain separate; the
      accepted sender file route is covered by the dedicated service-report
      lane. RL-152 keeps this at development traceability rather than Lab
      validation or conformance.
- [x] Add paired filesystem and HLA_IMMEDIATE failure-report matrices for the
      timestamped §6.12 `Send Interaction` pre-admission boundary. The focused
      lane preserves type-27/type-40/type-63/type-31 supplied forms, Null
      returns, false indicators, exact
      `InteractionClassNotDefined`/`InteractionParameterNotDefined`/
      `InvalidLogicalTime` text, and serials zero through two. Accepted sender
      output, retraction, and recipient delivery remain separate; RL-152 keeps
      this at development traceability rather than Lab validation or conformance.
- [x] Add paired filesystem and HLA_IMMEDIATE failure-report matrices for the
      timestamped §6.14 `Send Directed Interaction` pre-admission boundary. The
      focused lane preserves type-27/type-37/type-40/type-63/type-31 supplied
      forms, Null returns, false indicators, exact
      `InteractionClassNotDefined`/`ObjectInstanceNotKnown`/
      `InteractionParameterNotDefined`/`InvalidLogicalTime` text, and serials
      zero through three. Accepted sender output, retraction, and directed
      recipient delivery remain separate; RL-152 keeps this at development
      traceability rather than Lab validation or conformance.
- [x] Add paired filesystem and HLA_IMMEDIATE failure-report matrices for the
      timestamped §6.10 `Update Attribute Values` pre-admission boundary. The
      focused lane preserves type-37/type-2/type-63/type-31 supplied forms,
      Null returns, false indicators, exact
      `ObjectInstanceNotKnown`/`AttributeNotDefined`/`InvalidLogicalTime`
      text, and serials zero through two. The accepted sender file route is
      covered by the dedicated service-report lane; broader TSO delivery
      remains separate. RL-152 keeps this at development traceability rather
      than Lab validation or conformance.
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
      complete update-rate producer/timing evidence, ownership transfer, custom
      transportation beyond the separately covered ordinary, nonregional
      timestamped, and timestamped-regional interaction cases, FOM sharing
      policy, save/restore, and remote transport remain unimplemented. This has
      source/API traceability only and no
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
- [x] Translate the sibling two-dimensional multi-attribute `RegionalThing`
      stress vector into the official native C++ API. The focused Catch2 case
      gives Flavor and Organic independent two-dimensional source regions,
      proves initial overlap, X-only and Y-only disjointness, restored delivery,
      and the corresponding conveyed source `RegionHandleSet`. Its dedicated
      `ddm-regional-multi-attribute` lane runs the native case with separate
      Requirements-Lab and API contracts. This is a sibling-derived
      development-profile stress vector; timestamped/retraction, relaxed DDM,
      scope advisories, regional requests, transport, package evidence, and
      conformance remain open.
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
      complete update-rate producer/timing evidence, package evidence, and
      conformance remain future work. A separate bounded UpdateRateGate and
      timestamped two-federate Catch2 lane now exercise the development-profile
      delivery boundary.
- [x] Retain composed 2025 FDD `updateRates` metadata and bind the official
      `getUpdateRateValue` / `getUpdateRateValueForAttribute` support queries
      in the non-installable development profile. Named Restaurant FOM rates,
      the `HLAdefault` no-reduction boundary, invalid-designator handling, and
      known-object/defined-attribute validation have exact Requirements-Lab
      contracts and Catch2 coverage. Ordinary and regional subscription
      declarations now retain FDD designators, and the attribute query reports
      the corresponding rate or the default `0.0` after unsubscribe. A separate
      bounded UpdateRateGate and timestamped two-federate Catch2 lane now cover
      per-attribute best-effort suppression and reliable bypass. Complete
      producer-rate/timing matrices, MOM, package evidence, and conformance
      remain future work.
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
       sibling object-class form is tracked separately; a distinct class-level
       regional request/response case is also tracked separately. Timestamped/
       retraction behavior, broader DDM,
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
       interlock. The public joined-federate MOM route now re-reflects changed
       predefined switch values from both individual setters and this accepted
       subset, with bounded untimed ordinary, untimed directed, and timestamped
       directed `HLAreportServiceInvocation` delivery cases (alongside the
       existing timestamped/regional route); failure matrices and broader
       public MOM interaction delivery remain open.
       Other MOM
      control/reporting families, complete multi-owner/regional/update-rate
      behavior, package evidence, protected review, JUnit
      promotion, and conformance remain future work.
- [x] Expose the first public RTI-owned joined-federate MOM object-management
      slice in the non-installable development profile. Active ordinary and
      immutable-point-matching regional subscriptions discover each joined federate's
      `HLAmanager.HLAfederate` object, receives a reliable initial
      `HLAreportServiceFile` reflection using the immutable filesystem path,
      can request that known value directly, and receives removal on the
      represented federate's resignation. The complete private snapshot and
      public initial projection both retain all seven required MIM values and
      identity metadata. Regional discovery filters subscriptions against the
      immutable `HLAfederate` point in both callback models. A companion
      federation-MOM membership case now discovers the single execution-scoped
      `HLAmanager.HLAfederation` object from a membership-only subscription,
      decodes `HLAfederatesInFederation` as the official nested
      `HLAfederateReferenceList`, and proves direct plus Join/Resign conditional
      values at 1→2→1. The Java/JPype companion covers the same bridge shape.
      event-driven regression reflects all nine predefined conditional switch
      attributes after successful setters and the accepted `HLAsetSwitches`
      subset, plus four bounded temporal-state attributes after successful role
      transitions and all five time-advance request/grant forms, and answers
      their current values by direct known-object request in both callback
      models. Direct known-object requests also supply the MIM-periodic
      `HLAlogicalTime` and `HLAlookahead` values from the selected official
      time provider before any report period is configured. A companion
      GALT/LITS case now projects `HLAGALT` and `HLALITS` through the same
      federation-owned time-bounds calculator used by Query GALT/Query LITS,
      including the official empty-array undefined form. A companion
      HLATSOlength case now projects the queued TSO count from the coordinator
      through direct and HLAsetTiming requests before and after delivery. An
      ownership-backed `HLAobjectInstancesThatCanBeDeleted` case now projects
      the HLAcount from the live `HLAprivilegeToDeleteObject` ownership ledger
      around registration, periodic reflection, and deletion. A
      successful-update-count case now projects `HLAupdatesSent` from an
      RTI-owned joined-membership counter at the accepted Update Attribute
      Values boundary, proving direct 0/1/2 values and periodic reflection of
      2 after two successful invocations. The companion updated-object-count
      case retains accepted object handles, proving repeated updates to one
      object remain at `HLAobjectInstancesUpdated=1` while a second updated
      object raises the value to 2, with periodic reflection alongside
      `HLAupdatesSent=3`. The same common registration path now projects
      `HLAobjectInstancesRegistered`, proving direct 0/1/2 values and periodic
      2 after two successful object registrations. The deletion statistic now
      projects `HLAobjectInstancesDeleted` from accepted receive-order deletion
      and timestamped queue-admission boundaries, proving direct 0/1/2 and
      periodic 0 before deletion and 2 after two deletions. The receiving
      federate's `HLAobjectInstancesRemoved` counter now advances at the
      committed no-time and timestamped Remove Object Instance callback
      boundaries for ordinary application objects, with direct and periodic
      evidence for two receive-order callbacks. The companion
       `HLAobjectInstancesDiscovered` counter now advances at the ordinary
       application-object Discover Object Instance callback boundary and
       includes a third discovery after Local Delete Object Instance and an
       eligible resubscription. A
       sender-side interaction-statistics companion now projects
       `HLAinteractionsSent` and `HLAdirectedInteractionsSent` at the accepted
       Send Interaction service boundary. The native MOM case proves total
       0/1/2/3/4/5/6 and directed 0/0/1/1/2/2 across ordinary, directed,
       timestamped, regional, and timestamped-regional sends, with periodic
       6/2 reflection; recipient fan-out does not inflate the sender counts.
       The same native case now projects receiver-side
       `HLAinteractionsReceived` at direct 0/1/2/3/4/5/6 and periodic 6, plus
       `HLAdirectedInteractionsReceived` at direct 0/0/1/1/2/2 and periodic 2.
       Receiver values advance once at the accepted application callback
       boundary; RTI-originated MOM callbacks and suppressed deliveries are
       excluded. The Java/JPype bridge retains its smaller direct 0/1/2 and
       0/0/1 vector.
       A focused reflection-statistics case now projects
       `HLAreflectionsReceived` and `HLAobjectInstancesReflected` from the
       receiving federate's application callback boundary. Repeated updates
       to one object produce total 1/2 with distinct-object value 1; a second
       object produces total 3 with distinct-object value 2, and a queued
       timestamped reflection raises only the total to 4. Direct and
       HLAsetTiming periodic requests prove the same 4/2 values, while
       RTI-owned MOM reflections remain excluded.
       A focused receive-order queue case now projects `HLAROlength` from the
       represented federate's callback/deferred-receive ledger: direct and
       periodic values are 0 before a send, 1 while one application callback
       remains queued, and 0 after the target crosses its callback boundary.
       Native `HLA_EVOKED` evidence is paired with a Java/JPype matrix covering
       both callback models; RL-108 records the generic Lab relation gap.
       A focused native MOM request/report case now consumes the Subscribe-only
       `HLArequestObjectInstancesUpdated` interaction and emits one reliable
       RTI-originated `HLAreportObjectInstancesUpdated` interaction. The report
       uses the official nested `HLAobjectClassBasedCounts` encoding, groups
       joined-lifetime update responsibility by registered object class, and
       rechecks the target/report subscription at the callback boundary. RL-109
       records the generic Lab relation gap; the other MOM request/report
       families remain open.
       A second focused native MOM request/report case now consumes the
       Subscribe-only `HLArequestObjectInstancesThatCanBeDeleted` interaction
       and emits one reliable RTI-originated
       `HLAreportObjectInstancesThatCanBeDeleted` interaction. Its nested
       class-count value is derived from live
       `HLAprivilegeToDeleteObject` ownership, so deleting one of two registered
       objects removes only that class from the next response. RL-110 records
       the generic Lab relation gap; remaining public MOM request/report
       families remain open.
       A third focused native MOM request/report case now consumes the
       Subscribe-only `HLArequestObjectInstancesReflected` interaction and
       emits one reliable RTI-originated
       `HLAreportObjectInstancesReflected` interaction. Its nested
       class-count value is derived from the target's accepted application
       reflection ledger, grouped by registered class, so a repeated
       reflection of one object remains one count. RL-111 records the generic
       Lab relation gap; remaining public MOM request/report families remain
       open.
       A fourth focused native MOM request/report case now consumes the
       Subscribe-only `HLArequestUpdatesSent` interaction and emits one
       reliable RTI-originated `HLAreportUpdatesSent` interaction for each
       supported transportation type, including an empty `HLAupdateCounts`
       NULL response. Its `HLAtransportation` parameter identifies
       `HLAreliable` or `HLAbestEffort`, while nested `HLAupdateCounts` groups
       accepted updates by registered class. RL-112 records the generic Lab
       relation gap; custom or remote transportation and the remaining public
       MOM request/report families remain open.
       A fifth focused native MOM request/report case now consumes the
       Subscribe-only `HLArequestInteractionsSent` interaction and emits one
       reliable RTI-originated `HLAreportInteractionsSent` interaction for
       each supported transportation type, including an empty
       `HLAinteractionCounts` NULL response. Its value groups accepted sends by
       sent interaction class and includes a dimensioned regional send. RL-113
       records the generic Lab relation gap; custom or remote transportation
       and the remaining public MOM request/report families remain open.
       A sixth focused native MOM request/report case now consumes the
       Subscribe-only `HLArequestDirectedInteractionsSent` interaction and
       emits one reliable RTI-originated
       `HLAreportDirectedInteractionsSent` interaction for each supported
       transportation type, including an empty `HLAinteractionCounts` NULL
       response. Its value groups accepted directed sends by sent interaction
       class, separately from the all-interactions sender ledger. RL-114
       records the generic Lab relation gap; custom or remote transportation
       and the remaining public MOM request/report families remain open.
       A seventh focused native MOM request/report case now consumes the
       Subscribe-only `HLArequestInteractionsReceived` interaction and emits
       one reliable RTI-originated `HLAreportInteractionsReceived` interaction
       for each supported transportation type, including an empty
       `HLAinteractionCounts` NULL response. Its
       `HLAinteractionCounts` value groups accepted application receive
       callbacks by original sent interaction class and transportation. RL-115
       records the generic Lab relation gap; custom or remote transportation,
       directed-receipt reports, and the remaining public MOM request/report
       families remain open.
       An eighth focused native MOM request/report case now consumes the
       Subscribe-only `HLArequestDirectedInteractionsReceived` interaction and
       emits one reliable RTI-originated
       `HLAreportDirectedInteractionsReceived` interaction for each supported
       transportation type, including an empty `HLAinteractionCounts` NULL
       response. Its value groups accepted directed receive callbacks by
       original sent interaction class, separate from ordinary receives. RL-116
       records the generic Lab relation gap; custom or remote transportation
       and the remaining public MOM request/report families remain open.
       A shared native sender-count regression now requests the three
       sender-side MOM report classes against an empty ledger and proves both
       supported transport buckets for each family with empty official count
       arrays. RL-117 records the generic Lab relation gap; custom or remote
       transportation and the remaining public MOM request/report families
       remain open.
       A ninth focused native MOM request/report case now consumes the
       Subscribe-only `HLArequestReflectionsReceived` interaction and emits
       one reliable RTI-originated `HLAreportReflectionsReceived` interaction
       for each supported transportation type, including an empty
       `HLAreflectCounts` NULL response. Its nested
       `HLAobjectClassBasedCounts` value groups accepted application reflection
       callbacks by registered class and effective transportation; the case
        proves reliable and best-effort buckets plus both empty buckets for a
        joined federate with no reflections. RL-118 records the generic Lab
        relation gap; custom or remote transportation and the remaining public
        MOM request/report families remain open.
        A tenth focused native MOM request/report case now consumes the
        Subscribe-only `HLArequestObjectInstanceInformation` interaction and
        emits one reliable RTI-originated
        `HLAreportObjectInstanceInformation` response through the requesting
        federate's private `HLAfederate` point. Its official nested
         `HLAattributeHandleList` proves the known-versus-NULL parameter shape,
         registered/known class values, and the registering federate's
         `Efficiency` plus implicit `HLAprivilegeToDeleteObject` ownership.
         RL-119 records the generic Lab relation gap; custom or remote
         transportation and the remaining public MOM request/report families
         remain open.
         An eleventh focused native MOM request/report case now consumes the
         Subscribe-only `HLArequestPublications` interaction and emits the
         required reliable RTI-originated interaction-publication,
         object-class-publication, and directed-interaction-publication
         reports. It decodes the official nested handle lists, proves implicit
         `HLAprivilegeToDeleteObject` publication and the distinct MIM NULL
         shapes after unpublication, and checks private target-point routing,
         empty tags, default-invalid RTI producers, and callback gating. RL-120
         records the generic Lab relation gap; custom or remote transportation
         and the remaining public MOM request/report families remain open.
         A twelfth focused native MOM request/report case now consumes the
         Subscribe-only `HLArequestSubscriptions` interaction and emits
         reliable object-class, interaction, and directed-interaction
         subscription reports. It decodes active/passive groups and maximum
         update-rate names, official nested attribute and interaction
         subscription lists, and the local 2025 MIM directed shape; then
        verifies all three NULL responses, private target-point routing,
        empty tags, default-invalid RTI producers, and callback gating. RL-122
        records the generic Lab relation gap plus the local MIM versus
        semantic-reconstruction `HLAuniversal` discrepancy.
        A thirteenth focused native MOM request/report case now consumes the
        Subscribe-only `HLArequestFOMmoduleData` interaction and emits a
        reliable RTI-originated `HLAreportFOMmoduleData` response through the
        reported federate's private `HLAfederate` point. Join-time validated
        module content is retained in first-designator order; the case proves
        the official indicator/content encodings, reliable transport, empty
        tag, default-invalid producer, callback-time endpoint revalidation,
        and deterministic invalid-index rejection. RL-124 records the generic
        Lab content-access relation gap. A fourteenth focused native
        request/report case now covers the dimensionless federation-scoped
        `HLArequestFOMmoduleData`/`HLAreportFOMmoduleData` and
        `HLArequestMIMdata`/`HLAreportMIMdata` pairs. It retains validated
        federation FOM/MIM XML, broadcasts reliable reports to current
        subscribers, and proves official encodings, empty tags,
        default-invalid RTI producers, callback gating, callback-time
        subscription revalidation, strict MIM parameter handling, and
        invalid-index rejection. RL-124 now records both bounded relations;
        federation-scoped current-FDD access and the remaining public MOM
        request/report families remain open. A fifteenth focused native
        request/report case now covers the dimensionless federation-scoped
        `HLArequestSynchronizationPoints`/`HLAreportSynchronizationPoints`
        and `HLArequestSynchronizationPointStatus`/
        `HLAreportSynchronizationPointStatus` pairs. It decodes the official
        `HLAsyncPointList` and `HLAsyncPointFederateList` values, derives
        `MovingToSyncPoint`/`WaitingForRestOfFederation` from the retained
        achievement ledger, proves unknown-label and post-completion empty
        arrays, reliable broadcast, empty tags, default-invalid RTI producer,
        `HLA_EVOKED` gating, callback-time subscription revalidation, and
        strict request-parameter failures. RL-125 records that the Lab's
        independent MOM table candidates do not encode this request/report
        relation; remaining public MOM families, remote transport,
        JUnit/protected review, and conformance remain open.
        A sixteenth focused native MOM request/report case now covers the
        subscription-selected `HLAreportMOMexception` response for malformed
        or precondition-rejected MOM `Send Interaction` requests. The
        empty-parameter `HLAsetSwitches` request preserves
        `InteractionParameterNotDefined` for the sender and emits a reliable
        RTI-originated report with the fully qualified MOM interaction name,
        exception text, and `HLAparameterError=true`; a well-formed
        service-reporting adjustment rejected by its report-subscription
        precondition emits `HLAparameterError=false`. Both target the
        sender's private `HLAfederate` point. It proves default-invalid
        producer, empty tag, `HLA_EVOKED` gating, and callback-time
        subscription withdrawal. Generic `HLAservice` spoofing, other
        malformed MOM families, remote transport, JUnit/protected review, and
        conformance remain open.
        A seventeenth focused native MOM object case now exposes the
        federation-scoped `HLAcurrentFDD` attribute as an official
        `HLAunicodeString` carrying the materialized, schema-validated current
        FDD. It discovers the RTI-owned `HLAfederation` object, requests the
        composed base FDD, observes a reliable conditional refresh after a
        compatible additional FOM Join contributes
        `UmbraReferenceFixtureClass`, and verifies that a direct request agrees
        with the refreshed value. Default-invalid RTI producer, no regions,
        and empty tag are checked; RL-127 records the Lab's cross-cutting
        current-FDD modeling gap. Full module/FDD access matrices, remote
        transport, JUnit/protected review, and conformance remain open.
        An eighteenth focused native MOM object case now covers federation save
        conditionals through the public C++ route. It verifies empty initial
        `HLAnextSaveName`/`HLAnextSaveTime` and `HLAlastSaveName`/
        `HLAlastSaveTime`, a pending timestamped request, the reliable
        two-attribute next-value clear at constrained Initiate Federate Save
        admission, and the last-value update only after all three joined
        federates complete the snapshot. Official unicode/logical-time
        encodings and default-invalid/no-region/empty-tag metadata are
        checked; RL-128 records the Lab's missing save-ledger lifecycle
        relation. Restore-specific conditionals, remote transport,
        JUnit/protected review, and conformance remain open.
        A save/restore companion now projects the official bounded
      `HLAfederateState` enumeration from the real operation ledgers at
      initiation/begin and completion boundaries in both callback models; its
      save-state event reflection is suppressed at the federate that is itself
      saving while direct known-object AVU remains available. A bounded
      `HLAsetTiming` case now accepts the official
      `HLAfederateReference`/`HLAseconds` pair and emits the catalog-declared
      periodic subset after a target-local wall-clock deadline at an
      `HLA_EVOKED` callback boundary; its companion `HLA_IMMEDIATE` case
      delivers the same subset without an Evoke call, and zero disables later
      updates. Native Catch2 and JNI/JPype evidence now also cover the
      `HLAtimeGrantedTime` and `HLAtimeAdvancingTime` duration attributes
      through direct AVU and consume-once periodic reflections in both callback
      models. The remaining periodic/other conditional attributes (including
       traffic/statistical values beyond the bounded
       deletable-object, successful-update-count, updated-object-count,
       registered-object-count, deleted-object-count, removed-object-count,
       discovered-object-count, reflection-count, interaction-send, and interaction-receive
       projections),
        optional/inherited non-initial attributes, broader public MOM interactions
         beyond the bounded service-report invocation and
        `HLArequestPublications`/`HLAreportObjectClassPublication` plus
        `HLAreportInteractionPublication` and
        `HLAreportDirectedInteractionPublication` and
       `HLArequestSubscriptions`/`HLAreportObjectClassSubscription` plus
       `HLAreportInteractionSubscription` and
       `HLAreportDirectedInteractionSubscription` and
       `HLArequestFOMmoduleData`/`HLAreportFOMmoduleData` and
       federation-scoped `HLArequestMIMdata`/`HLAreportMIMdata` and
       `HLArequestSynchronizationPoints`/`HLAreportSynchronizationPoints` and
       `HLArequestSynchronizationPointStatus`/
       `HLAreportSynchronizationPointStatus` and
       `HLArequestObjectInstanceInformation`/`HLAreportObjectInstanceInformation`
        and
       `HLArequestObjectInstancesUpdated`/`HLAreportObjectInstancesUpdated` and
       `HLArequestObjectInstancesThatCanBeDeleted`/
       `HLAreportObjectInstancesThatCanBeDeleted` and
       `HLArequestObjectInstancesReflected`/
       `HLAreportObjectInstancesReflected` and
       `HLArequestUpdatesSent`/`HLAreportUpdatesSent` cases,
       plus `HLArequestInteractionsSent`/`HLAreportInteractionsSent`,
       plus `HLArequestDirectedInteractionsSent`/
       `HLAreportDirectedInteractionsSent`,
       plus `HLArequestInteractionsReceived`/
       `HLAreportInteractionsReceived`,
       plus `HLArequestDirectedInteractionsReceived`/
       `HLAreportDirectedInteractionsReceived`,
       plus `HLArequestReflectionsReceived`/
       `HLAreportReflectionsReceived`,
       source-backed producer mapping,
       JUnit/protected-review evidence, remote transport, and conformance remain
       future work; the paired Requirements-Lab contract is traceability only.
- [x] Add the bounded 2025 DDM non-void service-report slice to the
      non-installable C++ profile. `Create Region` now records its type-11
      `DimensionHandleSet` supplied value and type-42 `RegionHandle` return;
      `Get Range Bounds` records type-42/type-10 supplied values and a type-41
      `RangeBounds` lower/upper return. The filesystem writer keeps the same
      joined-federate file and serial sequence across switch gating, and C++
      unit plus integration lanes cover the rendered Table 5 one-element
      returned-argument array. Requirements-Lab row-level Table 5 extraction
      is still incomplete (RL-042/RL-067/RL-076), so broader non-void/failure
      matrices, public MOM promotion, protected review, and conformance remain
      open.
- [x] Add the paired DDM §9.2/§10.27 failure matrices. Filesystem and
      HLA_IMMEDIATE lanes preserve type-11 invalid DimensionHandleSet or
      type-42/type-10 invalid-dimension supplied forms, Null returns, false
      indicators, exception descriptions, and serials zero/one before
      successful CreateRegion/GetRangeBounds serials two/three. RL-152 still
      records the Requirements-Lab conditional failure-mapping gap; broader
      failure families, protected review, and conformance remain open.
- [x] Add the paired DDM mutation failure matrices for §9.3/§9.4/§10.28.
      Filesystem evidence covers invalid RegionHandleSet, RegionHandle,
      dimension, and range-bound inputs for CommitRegionModifications,
      DeleteRegion, and SetRangeBounds with Null returns, false indicators,
      exception text, and serials zero through three. The HLA_IMMEDIATE lane
      carries the same failed service-type-5 records followed by successful
      SetRangeBounds/CommitRegionModifications/DeleteRegion records with true
      indicators, Null returns, and serials four through six.
- [x] Add paired §9.10/§9.11 regional interaction-subscription failure
      matrices. The production filesystem lane and HLA_IMMEDIATE MOM lane
      cover invalid interaction-class and region-set inputs with the official
      type-27/type-43 forms plus Subscribe's type-6 passive indicator, Null
      returns, false indicators, exception descriptions, switch-gated
      suppression, and stable serial ordering around successful regional
      Subscribe/Unsubscribe records. RL-152 still blocks Lab validation;
      broader DDM coverage, protected review, and conformance remain open.
- [x] Add paired §9.8/§9.9 regional object-class subscription failure
      matrices. The production filesystem and HLA_IMMEDIATE MOM lanes cover
      invalid object-class and attribute/region-pair inputs with official
      type-36/type-4 forms plus Subscribe's type-6 passive and type-53/type-34
      update-rate slots, Null returns, false indicators, exception text,
      switch-gated suppression, and stable serial ordering around successful
      regional Subscribe/Unsubscribe records. RL-152 still blocks Lab
      validation; broader regional DDM coverage, protected review, and
      conformance remain open.
- [x] Add paired §9.6/§9.7 regional object-attribute association failure
      matrices. The production filesystem and HLA_IMMEDIATE MOM lanes cover
      invalid object-instance and region inputs with official type-37/type-4
      forms, Null returns, false indicators, exception text, switch-gated
      suppression, and stable serial ordering around successful
      Associate/Unassociate records. RL-152 still blocks Lab validation;
      broader regional DDM coverage, protected review, and conformance remain
      open.
- [x] Add the paired §5.4/§5.5 interaction-class declaration failure
      matrices. The production filesystem and HLA_IMMEDIATE MOM lanes cover
      invalid Publish/Unpublish handles with the official type-27 supplied
      form, Null returns, false indicators, exact exception text, and serials
      zero/one before accepted publication/removal records at serials two and
      three. Accepted MOM emission now occurs outside native locks so immediate
      observers can re-enter safely. RL-152 still blocks Lab validation;
      broader declaration failures, protected review, and conformance remain
      open.
- [x] Add the paired §5.10/§5.11 ordinary interaction-subscription failure
      matrices. The production filesystem and HLA_IMMEDIATE MOM lanes cover
      invalid Subscribe/Unsubscribe handles with the official type-27 form,
      Subscribe's type-6 passive indicator, Null returns, false indicators,
      exact exception text, and serials zero/one before accepted
      subscription/removal records at serials two/three. Accepted MOM
      emission remains outside native locks. RL-152 still blocks Lab
      validation; broader declaration failures, protected review, and
      conformance remain open.
- [x] Add the paired §5.6/§5.7 directed-declaration failure matrices. The
      production filesystem and HLA_IMMEDIATE MOM lanes cover invalid object
      and interaction handles across explicit-set type-36/type-28 forms and
      the whole-class Unpublish type-34 Null slot, with Null returns, false
      indicators, exact exception text, and serials zero through three before
      accepted explicit-set records at four/five and a whole-class record at
      six. Accepted MOM emission remains outside native locks. RL-152 still
      blocks Lab validation; directed subscriptions, protected review, and
      conformance remain open.
- [x] Add the paired §5.12/§5.13 directed-subscription failure matrices. The
      production filesystem and HLA_IMMEDIATE MOM lanes cover invalid object
      and interaction handles across type-36/type-28 forms, Subscribe's type-6
      universal selector, and the whole-class Unsubscribe type-34 Null slot.
      They preserve Null returns, false indicators, exact exception text, and
      failure serials zero through three before accepted records at four
      through six. Accepted MOM emission remains outside native locks. RL-152
      still blocks Lab validation; broader directed routing, protected review,
      and conformance remain open.
- [x] Add the paired §5.2/§5.3 object-attribute declaration failure matrices.
      The production filesystem and HLA_IMMEDIATE MOM lanes cover invalid
      object-class and attribute inputs with type-36/type-1 forms, Null
      returns, false indicators, exact exception text, and failure serials
      zero through three before accepted Publish/Unpublish records at four and
      five. Accepted MOM emission remains outside native locks. RL-152 still
      blocks Lab validation; broader object-attribute failures, protected
      review, and conformance remain open.
- [x] Add the paired §5.8/§5.9 object-class attribute subscription failure
      matrices. The production filesystem and HLA_IMMEDIATE MOM lanes cover
      invalid Subscribe/Unsubscribe subset and whole-class inputs with
      type-36/type-1 forms, Subscribe's type-6 passive and type-53 update-rate
      arguments, and the whole-class type-34 Null slot. They preserve Null
      returns, false indicators, exact exception text, and failure serials
      zero through four before accepted records at five through seven.
      Accepted MOM emission remains outside native locks. RL-152 still blocks
      Lab validation; broader delivery, protected review, and conformance
      remain open.
- [x] Extend the bounded 2025 support-service report lane through §10.21--§10.26.
      Available-dimension lookups record type-36/type-27 object and interaction
      class handles and return type-11 `DimensionHandleSet` values; name/handle,
      upper-bound, and region dimension-set lookups cover the type-53/type-10,
      type-35, and type-42/type-11 forms. The focused C++ unit and filesystem
      integration cases prove switch-gated setup, one stable file, and serial
      ordering. Table 5 row-level extraction remains incomplete
      (RL-042/RL-067/RL-076), so this is development-profile traceability only;
      failures, broader non-void forms, public MOM promotion, protected review,
      and conformance remain open.
- [x] Add paired §10.21--§10.26 failure matrices for dimension and region
      lookups. Filesystem and HLA_IMMEDIATE public-interaction evidence
      preserve the type-36/type-27/type-53/type-10/type-42 supplied forms, Null
      returns, false indicators, exception text, and serials zero through five
      before a successful serial-six GetDimensionHandle. RL-152 remains the
      conditional failure-mapping gap; other failure families, protected review,
      and conformance remain open.
- [x] Extend the bounded 2025 support-service report lane through §10.17--§10.20.
      The mandatory order and transportation lookup pairs now record type-53
      names with type-38 quoted `RECEIVE`/`TIMESTAMP` order values or type-59
      quoted transportation-handle values, in both directions. Focused C++
      unit and filesystem integration selectors prove switch-gated setup, one
      stable report file, and serial ordering. The Lab has no row-level Table 5
      ReturnArgument mapping for these forms (RL-147), so this remains
      development-profile traceability only; failures, public MOM promotion,
      protected review, and conformance remain open.
- [x] Extend the bounded 2025 support-service report lane through §10.2--§10.5.
      The federate and object-class lookup pairs now record type-53 names with
      type-15 or type-36 handle return forms in both directions. Focused C++
      unit and filesystem integration selectors prove one immutable report file
      and serial ordering. The Lab has no row-level Table 5 ReturnArgument
      mapping for these forms (RL-149), so this remains development-profile
      traceability only; failures, public MOM promotion, protected review, and
      conformance remain open.
- [x] Make production service-report file allocation transactional with Join.
      A pathname reserved before the private MOM object and report routes are
      committed is now removed on every exceptional Join rollback; successful
      resignation still retains the completed lifetime's file. The focused
      filesystem-store unit lane proves invalid initial-record conversion
      removes the reserved artifact. This is an internal durability invariant,
      not a new Requirements-Lab service mapping or conformance claim.
- [x] Add a focused production-filesystem service-report lifetime lane. The
      `service-report-file-lifecycle` Catch2/CTest slice proves eager creation
      of one absolute joined-federate path, initial-record retention, append
      suppression while `Send Service Reports To File` is disabled, and
      append continuation after re-enable without replacement or rotation.
      Dedicated Requirements-Lab and official C++ API contracts pin the join,
      routing, switch-setter, and report-producing lookup surfaces. This is
      development-profile traceability only; public MOM reflection, remote
      transport, package/JUnit evidence, protected review, and conformance
      remain open.
- [x] Extend the bounded 2025 support-service report lane through §10.13--§10.16.
      The interaction-class and parameter lookup pairs now record type-53
      names with type-27 interaction-class handles, and type-27/type-53 or
      type-39 parameter supplied/returned forms in both directions. Focused
      C++ unit and filesystem integration selectors prove one immutable report
      file and serial ordering. The Lab has no row-level Table 5 ReturnArgument
      mapping for these forms (RL-150), so this remains development-profile
      traceability only; failures, public MOM promotion, protected review, and
      conformance remain open.
- [x] Extend the bounded 2025 support-service report lane through §10.6--§10.12.
      The known-object, object-instance, attribute, and update-rate lookup
      services now record the official type-37/type-36 handle pairs, type-53
      object/attribute names, type-0 attribute handles, and type-35 maximum
      update-rate Number returns. Focused C++ unit and filesystem integration
      selectors prove switch-gated setup, one immutable report file, and serial
      ordering across all seven successful lookups. The Lab has no row-level
      Table 5 ReturnArgument mapping for these forms (RL-151), so this remains
      development-profile traceability only; failures, public MOM promotion,
      protected review, and conformance remain open.
- [x] Add the bounded §10.6--§10.12 failed-service report matrix. The seven
      known-object, object-instance, attribute, and update-rate lookups now
      preserve supplied forms, Null returns, false success, exception text,
      and serial ordering in a focused C++ filesystem test. RL-152 records the
      Lab's missing conditional failure mapping; other service families, public
      MOM promotion, protected review, and conformance remain open.
- [x] Route the first failed support lookup through the public MOM interaction
      sink. A focused HLA_IMMEDIATE observer case decodes failed
      `GetObjectClassHandle` service type 6, its type-53 supplied argument,
      Null return, false indicator, source-backed exception text, and serial
      zero before a successful serial-one lookup. The seven-case interaction
      matrix, other failure families, protected review, and conformance remain
      open.
- [x] Extend the public MOM failure route through the seven-case §10.6--§10.12
      interaction matrix. HLA_IMMEDIATE evidence now decodes every known-object,
      object-instance, attribute, and update-rate failure with its official
      supplied forms, Null return, false indicator, exception text, and serials
      zero through six, followed by a successful serial-seven lookup. Other
      failure families, protected review, and conformance remain open.
- [x] Add paired §10.13--§10.16 failure matrices for interaction-class and
      parameter lookups. Filesystem and HLA_IMMEDIATE public-interaction
      evidence preserve the type-53/type-27/type-39 supplied forms, Null
      returns, false indicators, exception text, and serials zero through three
      before a successful serial-four lookup. Other failure families, protected
      review, and conformance remain open.
- [x] Add paired §10.17--§10.20 failure matrices for order and transportation
      lookups. Filesystem and HLA_IMMEDIATE public-interaction evidence
      preserve the type-53/type-38/type-59 supplied forms, Null returns, false
      indicators, exception text, and serials zero through three before a
      successful serial-four lookup. Invalid OrderType uses the deterministic
      type-38 UNSUPPORTED diagnostic because the closed encoder has no canonical
      invalid-enum spelling. Other failure families, protected review, and
      conformance remain open.
- [x] Extend the bounded 2025 support-service report lane through §10.29--§10.33.
      The five handle-normalization services now record the type-50 ServiceGroup
      or type-15/type-36/type-27/type-37 supplied forms and return the
      execution-scoped coordinate as a type-35 Number. Focused C++ unit and
      filesystem integration selectors prove switch-gated setup, one stable
      report file, and serial ordering. The Lab has no row-level Table 5
      ReturnArgument mapping for these forms, and its `HLAnormalized*` MIM
      datatypes are not Table 5 argument types (RL-148); this remains
      development-profile traceability only, with failures, public MOM
      promotion, protected review, and conformance open.
- [x] Add paired §10.29--§10.33 failure matrices for handle normalization.
      Filesystem and HLA_IMMEDIATE public-interaction evidence preserve the
      type-50/type-15/type-36/type-27/type-37 supplied forms, Null returns,
      false indicators, exception text, deterministic `UNSUPPORTED` text for
      invalid ServiceGroup, and serials zero through four before a successful
      serial-five NormalizeServiceGroup. RL-152 remains the conditional
      failure-mapping gap; other failure families, protected review, and
      conformance remain open.
 - [x] Exercise the object-class `Request Attribute Value Update` overload and
      matching `Provide Attribute Value Update` callback in the non-installable
      development profile. The private registry validates selected-class
      attributes, expands a base-class request across all current registered
      subclass instances without requester discovery, groups work per provider
      and object instance, preserves the tag, suppresses requester-owned
      callbacks, and rechecks queued providers after resignation. It only
      solicits the callback in the base owner-solicitation case. A separate
      class-expansion companion has the provider invoke `Update Attribute
      Values` from that callback and verifies a timestamped reflection before
      the constrained requester's matching grant, including tag, producer,
      order, time, and retraction metadata. Additional regional request forms,
      automatic provision, alternate advances, broader timestamped/retraction
      behavior, DDM, update-rate reduction, ownership transfer, FOM sharing
      policy, save/restore, and remote transport remain unimplemented. This has
      source/API traceability only and no catalog or conformance claim.
- [x] Exercise the class-level 2025 `Request Attribute Value Update With
      Regions` overload and matching `Provide Attribute Value Update` callback
      in the non-installable development profile. The private registry validates
      committed request-region ownership/context, treats an empty pair as a
       no-op, filters explicit update associations by overlap, retains
       default-region eligibility, preserves the tag, and rechecks the request
       regions at callback entry. A companion case has the overlap-qualified
      provider explicitly respond through no-time `Update Attribute Values` and
      verifies the response reflection's tag, producer, transport, and source
      region when the requester enables conveyance. A second focused companion
      changes the requester subscription to a valid disjoint range before the
      queued reflection boundary and verifies suppression. Automatic provision,
      broader DDM, package/catalog evidence, and conformance remain separate
      work. A third focused companion has the provider invoke timestamped
      `Update Attribute Values` from the official callback, queues that
      overlap-qualified response for a constrained requester, and verifies one
      reflection before the grant with timestamp/order, tag, producer,
      retraction, and source-region metadata. Alternate advances, automatic
      provision, and broader timestamped/retraction behavior remain separate
      work. This has source/API traceability only.
- [x] Exercise `Query Attribute Ownership` and its 2025 C++ federate-owned /
      unowned result callbacks in the non-installable development profile. The
      private registry validates the requester's known class, groups attributes
      by joined owner or available-for-acquisition state, preserves the official
      callback distinction, and nullifies queued reports after receive-order
      `Remove Object Instance` begins. The same path now recognizes the separate
      RTI-owned joined-federate MOM ledger and emits one grouped
      `Attribute Is Owned By RTI` callback for queried predefined attributes;
      the read-only `Is Attribute Owned By Federate` service returns false for
      that MOM state. Complete regular/negotiated acquisition, remaining
      divestiture flows, RTI-owned mutation/resign disposition, DDM,
      save/restore, and conformance remain separate work.
- [x] Exercise the bounded 2025 MOM `HLAmodifyAttributeState` control path in the
      non-installable development profile. The official inherited target,
      object, attribute, and `HLAownership` parameters are decoded at the
      `Send Interaction` boundary and applied synchronously through the private
      application ownership ledger. `Owned` requires target knowledge and
      publication at the target's known class; `Unowned` clears the application
      owner; neither transition synthesizes acquisition or release callbacks.
      The focused case also rejects an unpublished target and rejects a
      predefined `HLAmanager.HLAfederate` attribute as RTI-owned without
      mutating the application ledger. Full MOM exception-report delivery,
      malformed/missing parameter and membership matrices, pending arbitration,
      complete RTI-owned ownership semantics, resign disposition, transport,
      package/JUnit/protected review, and conformance remain separate work;
      RL-142 records the Requirements Lab row-level candidate gap.
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
      duplicate offers. The focused ownership-assumption-research lane also
      proves stale callback suppression, reservation release, and a later
      re-offer with the originating divestiture tag. Full owner arbitration
      remains separate work.
- [x] Exercise bounded 2025 `Negotiated Attribute Ownership Divestiture`,
      `Request Divestiture Confirmation`, `Confirm Divestiture`, and `Cancel
      Negotiated Attribute Ownership Divestiture`. The current owner stays
      owner while private Waiting state selects an already pending regular
      acquirer; the owner gets one confirmation callback with the acquisition
      tag. Confirm transfers ownership synchronously and forwards its own tag
      to the standard acquisition notification. The focused extension also
      selects one already pending Willing-to-Acquire request, carries that
      request's tag to the confirmation callback, and consumes its stale
      If Available callback after transfer. Cancellation removes the
      pending negotiated state and restores ordinary regular-release planning,
      including the stale-callback boundary; cancellation of the selected
      acquirer yields `NoAcquisitionPending`. The mixed-form choice is a
      deterministic private policy, not standards-level arbitration. This does
      not implement the complete negotiated owner-search lifecycle, negotiated
      acquisition, RTI-owned state, full resign-action disposition, or
      conformance.
- [x] Exercise the mandatory 2025 `HLAreliable` and `HLAbestEffort`
      transportation-type name/handle support services in the non-installable
      development profile. The bounded receive-order interaction and
      attribute-update paths now use the effective per-federate type. A paired
      FOM provider/consumer case now also resolves declared custom names to
      deterministic execution-scoped opaque handles shared by joined federates;
      focused ordinary and ordinary-regional interaction cases, timestamped
      interaction/attribute cases, ordinary/timestamped regional attribute
      cases, and ordinary/timestamped directed interaction cases carry that
      declared custom type through the relevant callback surfaces. Remote and
      message-transport semantics remain
      absent. This has API traceability only with no catalog or conformance
      claim.
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
      and regional sends for that publisher. The separate composed-FDD delivery
      cases cover one declared custom name for ordinary receive-order, ordinary
      regional, nonregional timestamped interaction/attribute, timestamped
      regional interaction, ordinary/timestamped regional attribute, and
      ordinary/timestamped directed interaction delivery; the callback reports
      preserve the same
      execution-scoped handle through the TSO grant and regional source metadata.
      Complete relevance-advisory preconditions, remote transport, save/restore,
      package support, protected evidence, and
      conformance remain outside this bounded slice. Separate Requirements Lab
      contracts and Catch2 coverage
      record the source/API traceability.
- [x] Exercise the non-timestamped, non-region `Send Interaction` overload
      and matching no-time `Receive Interaction` callback in the
      non-installable development profile. The routing kernel chooses the
      closest active subscribed class, projects available parameters, excludes the
      sender, rechecks unsubscribe-before-delivery, and uses the recipient's
      immediate or evoked callback model. Timestamped/retraction behavior,
      remaining object-attribute regional forms, broader DDM routing,
      FOM sharing-policy enforcement,
      other timestamped/local object-lifecycle delivery, custom transportation
      outside the separate composed-FDD interaction/attribute cases, and
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
      evoked delivery. An accepted target deletion before an evoked callback
      suppresses the stale directed interaction while its removal callback
      remains deliverable; the TSO counterpart delivers removal at time 6 and
      suppresses a queued directed callback at time 7 before its grant. The
      ownership/universal selector is implemented: a
      missing or false selector requires an owned target attribute, true
      accepts every known target, an empty class set preserves modes, and
      re-subscribing a supplied class changes that class's mode.
       Timestamped/retraction behavior beyond the separate bounded slice,
       ordering, FOM sharing-policy enforcement,
       packaging, Lab mapping resolution, evidence,
       and conformance remain outside this slice. IEEE 1516.1-2025 does not
       define a directed-interaction region-context overload or a region
       designator on `ReceiveDirectedInteraction`; Umbra therefore keeps
       directed delivery target/declaration based and does not invent a
       bespoke directed-DDM Python/JNI surface.
- [x] Exercise the 2025 regional `Subscribe Interaction Class With Regions`,
      `Unsubscribe Interaction Class With Regions`, and no-time `Send
      Interaction With Regions` overloads in the non-installable development
      profile. Regional declarations remain independent from ordinary
      subscriptions; only active committed region-set overlap gates delivery,
      passive pairs remain declared without arranging delivery, empty sent
      sets suppress delivery, and queued callbacks recheck active overlap. Remaining
      object-attribute regional forms, timestamped/retraction, realization, broader
      DDM routing, package/catalog evidence, and conformance remain out of
      scope. The same slice now has a focused production-filesystem service-
      report lane for the accepted §9.10/§9.11 subscription transitions: each
      record preserves the type-27 interaction designator and type-43 region
      set, with the type-6 passive indicator on subscription. This remains
      development-profile traceability, not a conformance claim.
- [ ] Expand multi-federate callback ordering beyond the new per-survivor
      report-ordering slice; complete timestamped/region
      update-reflection, local/timestamped
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
