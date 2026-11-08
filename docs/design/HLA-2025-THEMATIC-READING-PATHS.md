# IEEE 1516.1-2025 Thematic Reading Paths

Follow these reader routes across the 2025 topic guides. They are navigation aids, not required service sequences or callback-order claims.

## Thematic reading paths

Use these paths to follow a common HLA question across related guides. They are
reading routes, not required service sequences or callback-order claims.

- **From a model to a receiver-observed value:** start with the Cross-Cutting
  guide's [map of independent questions for one attribute-value update](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md#an-object-attribute-value-touches-several-independent-boundaries),
  which includes producer authority, receiver interest, message properties,
  logical-time eligibility, update-rate admission, and callback observation.
  Then use [FOM admission and composition](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md),
  [DataElement encoding](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md),
  [object and interaction information](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md),
  [subscription state](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md),
  [DDM and regions](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md),
  [delay subscription evaluation](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md),
  and [callback observation](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md)
  for the specialized explanations. The Cross-Cutting diagram's dashed links
  mean “also ask this question”; they do not form one universal send pipeline.
- **From object-class interest to initial value reflection:** the Subscription
  State guide's [late-subscription comparison](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#a-subscription-can-be-late-relative-to-two-different-events)
  separates an existing object first becoming known, a prior publisher update
  whose delivery evaluation is still pending, and adding an attribute after
  discovery. Compare the Subscription State guide's [profile chart](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#process-late-subscription-path-same-selectors-an-unresolved-initial-value-edge)
  with Object and Interaction's [first-reflection evidence map](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#first-reflection-after-discovery-order-and-evidence-boundary)
  and [one-attribute path](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#follow-one-object-attribute-end-to-end).
  For regional interest, see DDM's [class-ancestry projection](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#object-class-ancestry-regional-discovery-and-value-projection)
  and [commit-triggered discovery](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#a-committed-range-change-can-discover-an-existing-object).
  An initial reflection may report current stored values; it is not a replay
  of the publisher's earlier update. The embedded case observes discovery and
  reflection without asserting their relative order. Process pull and push
  evidence is separate and does not establish profile parity. The licensed
  §5.8 reading remains open.
- **From the Active/Passive choice to a receiver-side outcome:** compare the
  Subscription State guide's [separate discovery and value gates](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#2-discovery-and-value-delivery-are-separate-active-subscription-gates)
  and [passive-receiver case](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#passive-is-not-a-hard-delivery-veto)
  with Object and Interaction's [service-specific relevance gates](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#why-passive-declarations-use-service-specific-relevance-gates).
  The embedded object path can use another member's active declaration to
  establish discovery or per-attribute update relevance, while checking a
  passive receiver's own class, attribute, and region match separately. That
  three-role object result is source-derived and lacks a focused test; process
  declaration tests do not exercise cross-member delivery. The Relevance
  Advisory guide's [signal table](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md#start-by-separating-advisories-from-delivery-gates)
  separates owner-facing advice from reflection. [Update-rate admission](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md#2-admit-or-suppress-each-best-effort-value-at-delivery)
  is a later embedded value gate; the [process rate boundary](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md#process-endpoint-subscription-metadata-is-not-callback-admission)
  records retained metadata without verified per-value admission.
- **From an update-rate name to receiver value admission:** begin with the
  Update Rate guide's [public vocabulary](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md#the-public-vocabulary)
  to distinguish a model-defined designator name, numeric query results, and
  a reflected value. Its per-attribute query reports a rate, not proof that a
  value was admitted; the embedded aggregation is an implementation detail.
  Relevance Advisory's [subscription fan-out](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md#one-object-subscription-separate-calculations)
  shows owner-facing `Turn Updates On/Off` separately from later reflection.
  Continue with Update Rate's [rate resolver](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md#1-resolve-a-rate-for-each-receiver-and-attribute)
  and [best-effort admission gate](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md#2-admit-or-suppress-each-best-effort-value-at-delivery).
  The embedded gate uses steady-clock time and suppresses without scheduling
  a replacement callback; reliable values bypass it. The [process boundary](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md#process-endpoint-subscription-metadata-is-not-callback-admission)
  records designator metadata without verified per-value rate admission.
- **From discovery to the different attribute-value effects:** use the
  Subscription State guide's [late-subscription comparison](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#a-subscription-can-be-late-relative-to-two-different-events)
  to distinguish an object newly known to a receiver from an attribute added
  after discovery. Then compare the [Auto Provide neighboring-path table](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md#keep-these-neighboring-paths-distinct),
  the [Relevance Advisory guide](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md)'s
  owner-facing `Turn Updates On/Off` notification, and the RAVU guide's
  [HLA-facing solicitation model](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md#1-hla-facing-model-solicitation-and-value-delivery-are-distinct).
  This is a reading route, not a callback-order pipeline: receiver reflection,
  provider solicitation, and owner advice remain separate effects. Configured
  process Auto Provide remains unverified.
- **From a late subscription to an update already in flight:** use the
  Subscription State guide's [two kinds of lateness](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#a-subscription-can-be-late-relative-to-two-different-events)
  to separate current-state reflection for a newly known object from a prior
  publisher update awaiting receiver-eligibility evaluation. The [Delay
  Subscription Evaluation guide](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md#a-retained-candidate-uses-the-current-subscription-generation-at-delivery)
  explains the retained-update boundary; use [Time Management](HLA-2025-TIME-MANAGEMENT-GUIDE.md)
  for TSO time/grant eligibility and [Callback and Service Ordering](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md)
  for callback observation. These are connected questions, not one shared
  delivery pipeline: process DSE evidence covers regional receive-order
  attribute updates, while other process routes have narrower or source-only
  evidence.
- **From an additional FOM module to ordinary object discovery:** start with
  the FOM guide's [Join replacement and membership phases](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md#join-with-additional-fom-modules-replace-definition-and-add-member-together),
  then its [catalog-replacement boundary](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md#catalog-replacement-does-not-itself-discover-ordinary-application-objects).
  Use [Federation Lifecycle](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md)
  for the new member's lifetime and [Object and Interaction Information](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#object-path-declarations-registration-and-discovery)
  plus [Subscription State](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#class-ancestry-affects-separate-decisions)
  for ordinary per-receiver class interest and discovery. Keep membership-driven
  `HLAfederate` discovery and the embedded `HLAcurrentFDD` reflection separate
  from callbacks for ordinary application objects. The Federation MOM guide's
  [projection table](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#the-effective-model-mom-projections-and-module-reports-are-different-views)
  separates the effective composed model, federation-level
  `HLAFOMmoduleDesignatorList`/`HLAcurrentFDD`, and the joining member's
  Join-scoped `HLAFOMmoduleDesignatorList`; its [configured-process Join trace](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#2025-process-profile-federation-mom-after-an-additional-module-join)
  records that no Federation-level Conditional-update enqueue was found on
  that path. The [joined-federate MOM lifecycle](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md#join-builds-the-object-before-making-the-member-visible)
  traces discovery of the new member's `HLAfederate` object to eligible existing
  subscribers. No focused test combines a successful module-adding Join with
  discovery of an already-registered application object; the source trace does
  not settle the normative callback duty or establish profile equivalence.
- **From a module-added attribute to a value on an existing object:** begin with
  the FOM guide's [new-member-on-existing-class trace](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md#a-newly-added-class-attribute-does-not-retrofit-registered-instances).
  It separates successful catalog replacement from per-instance ownership and
  existing declarations; the inspected implementation does not retrofit either
  during Join. Then follow [If-Available ownership](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#if-available-evaluate-at-the-result-callback)
  for a separate later ownership transition. That focused ownership case uses
  an attribute present at Create, so it does not verify a Join-added attribute.
  Use the Subscription State guide's
  [already-known object case](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#a-subscription-can-be-late-relative-to-two-different-events)
  to follow the receiver's separate subscription change. Once a value is actually
  sent, use Object and Interaction's [one-attribute route](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#follow-one-object-attribute-end-to-end).
  No focused test demonstrates a successful additional-module Join adding an
  attribute to an existing class, and no test combines that Join with later
  ownership and reflection. The current-value and normative §5.8 outcomes
  remain unresolved; these links are separate questions, not a promised Join
  callback sequence.
- **From a module-added interaction class to a later Receive Interaction:**
  start with the FOM guide's [interaction-catalog growth boundary](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md#interaction-catalog-growth-is-not-a-receive-interaction-callback),
  which separates Join-time catalog/handle changes from a later ordinary send.
  Then follow Object and Interaction's [ordinary interaction route](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#follow-one-ordinary-interaction-end-to-end)
  for publication, receiver subscriptions, closest-class selection, parameter
  projection, and callback observation. The FOM guide's retained-ancestor
  subscription path is source-derived: focused Join tests check class/parameter
  lookup, while the inherited-receipt test creates the hierarchy before Join;
  none combines Join with a later send of the added class. The configured-process
  additional-FOM case is object-only. Keep this distinct from directed
  interaction delivery; the word “Directed” in one fixture's class name does
  not make its lookup test a directed send. This route does not claim a tested
  post-Join receipt or embedded/process parity.
- **From an input FOM to execution creation and member lifetime:** use the FOM
  guide's [three validity stages](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md#three-different-meanings-of-valid)
  and [prepare-before-name-reservation flow](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md#create-prepare-the-entire-definition-before-reserving-its-name),
  then continue to Federation Lifecycle's [Create/Join/Destroy map](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#create-join-and-destroy).
  This keeps per-module validation, complete-set composition, FDD materialization,
  execution creation, and member Join as distinct outcomes; creating the
  execution does not itself join its creator. The focused embedded and
  configured-process preparation cases are separate evidence and do not
  establish profile parity or general conformance. For scoped Connect and
  federation-management authorization, see the Authorization guide's
  [callback-scope map](HLA-2025-AUTHORIZATION-FLOW-GUIDE.md#three-authorization-scopes).
- **From an ordinary interaction class to receiver observation:** follow the
  [interaction reader route](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#follow-one-ordinary-interaction-end-to-end)
  through class/parameter projection, class relevance, optional DDM region
  scope, then select its receive-order or timestamped temporal path. That
  reader route also links configured-process queue and callback stages with
  source-bounded notes. A directed send names a target object, then the RTI
  selects zero or more receiving federates; follow the Directed Interaction
  guide's [three-identity map](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md#keep-three-identities-separate)
  and [receiver selector](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md#the-declaration-matrix-and-receiver-selector),
  plus its [target-versus-region distinction](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md#a-target-object-is-not-a-ddm-region).
  Universal/by-ownership selection is not an ordinary DDM overlap test. These
  links connect existing explanations; they do not define one required service
  sequence.
- **From a directed send to its profile-specific delivery boundary:** the
  Directed Interaction guide's [receive-order profile map](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md#2025-receive-order-delivery-has-a-profile-boundary)
  contrasts embedded callback-time revalidation with configured-process
  send-time event projection. Its [recipient-lane timing map](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md#process-projection-timing-depends-on-the-recipient-lane)
  shows process TSO copies being reprojected at the recipient's delivery
  boundary. Process stale-state behavior after send acceptance remains
  source-only; the guide found no focused test for that race. For a throwing
  process `receiveDirectedInteraction` and the skipped TSO ACK, follow the
  Directed Interaction guide's [timestamped-send state map](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md#timestamped-send-is-the-same-routing-question-at-a-different-time-boundary)
  → the Process Endpoint guide's [callback-completion boundary](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#3-a-timestamped-process-delivery-stays-in-transit-through-callback-completion).
- **From an interaction call result to receiver outcome:** compare the
  ordinary [accepted-send boundary](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#embedded-accepted-send-boundary-and-recipient-fan-out)
  with Directed Interaction's [receive-order validation and fan-out](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md#receive-order-send-validate-fan-out-then-dispatch).
  These embedded maps separate rejection before acceptance, accepted sends
  with no planned callback, later stale-callback suppression, and callback
  exceptions after accepted effects. The Callback Ordering guide shows when
  an `HLA_IMMEDIATE` exception can return on the Send service stack. For
  process calls, follow the [ordinary](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#failure-mapping-local-typed-errors-versus-remote-rejection)
  and [directed](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#failure-mapping-configured-process-send-directed-interaction)
  failure maps and the [recipient stream](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#process-mapping-an-interaction-request-and-its-recipient-stream).
  Remote process planner rejection loses the typed distinction in the source
  path; focused public process tests cover successful sends, not these negative
  results. Callback exceptions after an accepted embedded send are detailed in
  [Callback Ordering](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#embedded-send-interaction-accepted-effects-precede-callback-failure).
  For the configured-process receiver's distinct exception boundary, see
  [process callback-exception attribution](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#process-endpoint-callback-exception-attribution).
  That source-derived path does not establish that a recipient callback failure
  becomes the sending federate's service result; no focused test combines an
  accepted process send with a throwing `receiveInteraction` callback.
- **From accepted FQR to callback observation across Disable Time Constrained:**
  follow the Time Management guide's [FQR/Disable sequence](HLA-2025-TIME-MANAGEMENT-GUIDE.md#disable-time-constrained-between-fqr-acceptance-and-evoke),
  then its [configured-process Evoke boundary](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#process-evoke-multiple-separates-event-admission-from-callback-draining).
  The review distinguishes embedded callback-time FQG calculation from the
  process coordinator's already-selected values and identifies an untested
  client-FIFO admission window. Treat this as an implementation trace, not a
  portable callback-order rule.
- **From a configured-process receive-order interaction to callback entry:**
  start with the Process Endpoint guide's [recipient stream](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#process-mapping-an-interaction-request-and-its-recipient-stream),
  then follow the Callback guide's [pushed event during an Evoke wait](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#a-pushed-interaction-can-arrive-during-an-evoke-wait)
  and the Time Management guide's [receive-order temporal gate](HLA-2025-TIME-MANAGEMENT-GUIDE.md#configured-process-receive-order-interaction-gate).
  This is a configured-process source trace; polling versus push transport,
  `HLA_IMMEDIATE` versus `HLA_EVOKED`, and the temporal gate remain separate
  decisions.
- **From a reserved name to each federate's object view:** follow the
  [object-lifetime reader route](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#follow-a-reserved-name-into-each-federates-object-view)
  from reservation outcome through named registration and per-receiver
  discovery, then separate federation Delete/Remove from caller-local forget.
  Timestamped deletion branches to its own retraction path. Existing diagrams
  provide the detail; this is not one end-to-end tested trace.
- **From “the object disappeared” to the state that changed:** start with the
  Object/Interaction guide's [disappearance comparison](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#diagnose-disappearance-by-asking-which-state-changed),
  then follow Ownership's [delete-privilege transfer](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#a-new-owner-can-gain-delete-authority-without-becoming-the-registrant)
  when asking who could delete, its [accepted-Delete cleanup](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#accepted-object-deletion-invalidates-pending-ownership-work)
  for ownership effects, or the Lifecycle guide's [ResignAction map](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#resignaction-names-requested-cleanup-they-are-not-the-leave-transition)
  when resignation triggered cleanup. The [DDM object/region lifetime](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#object-deletion-and-update-region-lifetime),
  [timestamped deletion/retraction boundary](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md#service-specific-effects-deletion-is-special),
  and Save/Restore's [queued-deletion restore](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-queued-timestamped-object-deletion-returns-after-fresh-registry-restore)
  and [post-Restore Retract case](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-retracting-a-restored-deletion-after-one-recipient-receives-it)
  answer separate follow-up questions. Those restore cases use a fresh registry
  in one test process, not an OS-process restart. Receiver state changes and
  callback entry are per-recipient gates; this route does not assert a global
  callback order or one combined test.
- **From a timestamped send to receiver delivery:** [time management](HLA-2025-TIME-MANAGEMENT-GUIDE.md) →
  [order and transportation](HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md) →
  [TSO retraction](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md) →
  [callback and service ordering](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md).
  For embedded best-effort attribute updates, the Update Rate guide's
  [wall-clock/TSO boundary](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md#3-keep-the-wall-clock-outside-the-tso-state-machine)
  separates logical-time eligibility from per-value steady-clock admission.
  Suppression does not schedule a later reflection or by itself retire the
  retraction handle; configured-process rate admission remains unverified.
- **From producer Disable Time Regulation to old and new timestamped messages:**
  follow the Time Management guide's [queued-TSO grant example](HLA-2025-TIME-MANAGEMENT-GUIDE.md#one-queued-tso-still-limits-an-undefined-galt-grant)
  for already-accepted recipient work, the Order and Transportation guide's
  [future-send classification](HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md#disable-time-regulation-is-a-separate-input-to-future-send-interaction-classification),
  and the TSO Retraction guide's [live-designator lifecycle](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md#disable-time-regulation-temporarily-blocks-retract-while-the-designator-remains-live).
  These routes have separate gates; no single test combines all three timelines.
- **From a regional Update to its time and callback boundaries:** start with
  the DDM guide's [regional update across three advance services](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#one-regional-update-three-available-advance-choices),
  compare the Time Management guide's [timestamped attribute projection map](HLA-2025-TIME-MANAGEMENT-GUIDE.md#timestamped-attribute-updates-time-eligibility-and-receiver-projection),
  then use Callback/Service Ordering's [process Evoke boundary](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#process-evoke-multiple-separates-event-admission-from-callback-draining)
  for event admission versus application callback entry. Evidence is split:
  the embedded regional test covers FQR/TARA/NMRA together, while process
  public cases cover regional NMR/NMRA under `HLA_EVOKED` and `HLA_IMMEDIATE`.
  The process cases do not mutate regions between update acceptance and event
  creation; no universal callback order or process stale-region result is
  implied.
- **From a timestamped regional interaction to callback observation:** start
  with the Object and Interaction Information guide's [later receiver
  projection](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#timestamped-interactions-resolve-the-receiver-projection-later),
  then trace its [regional subscription replacement case](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#a-replacement-regional-interaction-subscription-does-not-retarget-queued-tso-work)
  and the Time Management guide's [timestamped-message boundary](HLA-2025-TIME-MANAGEMENT-GUIDE.md#timestamped-message-lifecycle-and-callback-order).
  Finish with Callback/Service Ordering's [process Evoke boundary](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#process-evoke-multiple-separates-event-admission-from-callback-draining)
  for event admission versus application callback entry. The subscription-
  replacement outcome is tested in the embedded profile; the process recheck
  is source-derived, and focused process cases do not mutate the subscription
  while a delivery is queued. Keep this interaction path distinct from
  timestamped object-attribute projection.
- **From a live Region to saved regional state:** start with DDM's [pending,
  committed, and delete lifecycle](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#region-template-lifecycle),
  then distinguish a restored Region definition, a saved object update-region
  association, and a queued interaction's source-region snapshot in the
  [Save/Restore guide](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-a-committed-region-returns-from-the-saved-image).
  The focused evidence includes [association reuse after member-handle
  rebinding](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-an-update-region-association-and-queued-tso-survive-fresh-registry-restore)
  and [queued regional TSO restoration](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-a-saved-regional-tso-interaction-becomes-live-again).
  These traces do not establish a new send routed through a rehydrated regional
  interaction subscription or configured-process parity.
- **From a Region definition to object-attribute delivery:** begin with DDM's
  [pending and committed Region lifecycle](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#region-template-lifecycle),
  then follow the per-object/per-attribute source association in
  [object-attribute routing](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#object-attributes-source-association-discovery-relevance-and-updates),
  including the [stale-Region admission boundary](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#a-stale-region-rejects-the-association-before-the-map-changes).
  Source association and the receiver's regional subscription are separate
  state. For the embedded route, compare [commit-triggered discovery](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#a-committed-range-change-can-discover-an-existing-object)
  with [receive-order reflection after overlap changes](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#receive-order-value-routing-after-regional-overlap-changes).
  For the configured-process route, follow the separate [send-time source
  snapshot and receive-time receiver projection](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#process-boundary-case-send-time-source-snapshot-receive-time-receiver-projection).
  These are distinct cases and profiles: they do not form one tested pipeline
  or establish embedded/process parity or callback order.
- **From a conveyed Region designator to its callback lifetime:** use the DDM
  guide's [Region-copy lifetime and test boundary](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#a-conveyed-region-designator-is-not-a-surviving-live-region)
  with Callback/Service Ordering's [embedded and process callback boundary](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#conveyed-region-designators-follow-callback-completion).
  Then compare the Save/Restore guide's [regional Update after producer Resign](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-a-restored-regional-update-outlives-its-producers-resign).
  The source distinguishes callback-scoped Region copies, process delivery
  leases, and the producer's live Region; the cited tests do not query a copied
  Region after callback return. The DDM guide records the [source-only configured-process receive-order gap](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#receive-order-region-designators-have-profile-specific-callback-windows):
  one pull interaction has a session lease ending at the next poll, while
  ordinary process reflection and push routes lack source-demonstrated
  receiver-copy creation. This is not a test result; the focused correction
  handoff remains open.
- **From a queued regional interaction through Restore or Resign:** compare
  the Save/Restore guide's [single-recipient case that Retracts after Restore](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-a-saved-regional-tso-interaction-becomes-live-again)
  with its [fresh-registry case](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#a-fresh-embedded-registry-can-rebind-two-queued-regional-recipients),
  which restores two recipient entries under both callback models but only
  checks handle validity. Contrast both with the DDM guide's [source-Resign
  transition](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#a-queued-regional-interaction-survives-source-resignation),
  where the payload survives while the producer's current retraction record is
  terminal. The fresh-registry test runs in one OS process; it is not process-
  endpoint or process-restart evidence.
- **From a restored regional TSO payload to receiver delivery:** compare the
  Save/Restore guide's [regional AttributeUpdate case](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-an-update-region-association-and-queued-tso-survive-fresh-registry-restore)
  with its [regional interaction case](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#a-fresh-embedded-registry-can-rebind-two-queued-regional-recipients).
  Both public fresh-registry tests restore one TSO message and two recipient
  entries under `HLA_EVOKED` and `HLA_IMMEDIATE`, but they retain different
  payload-family state. Their fresh-registry cases check a valid retraction
  handle without invoking `Retract`; the separate development-profile cases
  exercise the post-Restore retraction service. Follow DDM and Time Management
  for receiver projection and the logical-time gate. These are embedded
  registry cases within one test process, not process-endpoint or OS-restart
  evidence.
- **From Restore through source Resign before regional Update delivery:** use
  the Save/Restore guide's [queued AttributeUpdate trace](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-a-restored-regional-update-outlives-its-producers-resign)
  for the saved source snapshot, post-Restore source mutation, producer
  Resign, and receiver FlushQueueRequest. Its one- and two-recipient tests
  observe each reflection before that receiver's flush grant and retain a
  valid handle value, but do not call `Retract` after Resign or establish
  cross-receiver callback order. The DDM guide explains the separate live
  source-Region cleanup; the retraction guide owns service eligibility rules.
- **From connection through federation protocols:** [connection and configuration](HLA-2025-CONNECTION-AND-CONFIGURATION-FLOW-GUIDE.md) →
  [federation and federate lifecycle](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md) →
  [synchronization](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md) →
  [Save/Restore](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md).
- **From Connect credentials to federation membership:** start with the
  Connection guide's [Connect preconditions and commit](HLA-2025-CONNECTION-AND-CONFIGURATION-FLOW-GUIDE.md#1-connect-validates-first-prepares-then-commits),
  then follow Authorization's [Connect profile and authorizer gate](HLA-2025-AUTHORIZATION-FLOW-GUIDE.md#1-connect-loads-the-profile-asks-once-then-commits).
  For later Create, Destroy, or Join calls, use the separate
  [connected-credentials gate](HLA-2025-AUTHORIZATION-FLOW-GUIDE.md#2-connected-credentials-gate-create-destroy-and-join)
  and Federation Lifecycle's [membership transitions](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#create-join-and-destroy).
  The pinned 2025 interface scopes authorizer callbacks to Connect,
  Create/Destroy, and Join; it does not define a callback for every RTI service.
  `AUTHORIZED` passes that operation's gate, while service preconditions and
  state commit remain separate. Umbra's RID profile selection and exception
  mapping are implementation evidence, and process authorization has no
  focused integration test.
- **From Join to an active protocol's participant set:** use Lifecycle's
  [cross-protocol map](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#a-join-does-not-define-every-active-protocols-participant-set),
  then compare Synchronization's [late-join expansion](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md#late-joiners-change-only-an-expandable-barrier)—an
  embedded behavior for omitted-set points—with the [Save/Restore Join
  boundary](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#join-crosses-a-separate-membership-admission-boundary). The pinned API lists
  `SaveInProgress` and `RestoreInProgress`, while exact Join admission and its
  effect on Save/Restore participants remain unresolved. In the inspected
  registry code, operation status maps are initialized from current membership
  and the Join path shows no update to an active map; no focused test attempts
  Join during an active operation. Do not transfer the synchronization rule to
  Save/Restore or treat source as the normative contract.
- **From a member exit during active federation work:** compare Lifecycle's
  [active Save/Restore exit flow](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#active-federation-save-and-restore-during-member-exit)
  with Synchronization's [pending-barrier departure](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md#member-departure-can-close-a-pending-barrier):
  Save/Restore ends a federation operation, while barrier departure removes one
  participant and reevaluates the remaining set. The Save/Restore guide's
  [active-Restore exit boundary](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#member-exit-during-an-active-restore)
  records the configured-process Restore-notification source gap. Use Callback
  and Service Ordering's [local ConnectionLost path](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#connectionlost-producer-exception-containment-and-backend-specific-cleanup)
  only for the local callback and queue boundary; it does not establish ordering
  against federation-operation notifications. Focused member-exit cases cover
  bounded embedded voluntary-Resign paths; active-operation ConnectionLost and
  configured-process member-exit outcomes remain untested. These are distinct
  service state transitions, not one cross-service protocol or a normative
  callback-order claim.
- **From a destroyed execution to restored federation state:** start with
  Lifecycle's [same-name recreation flow](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#same-name-recreation-is-fresh-restore-must-be-explicit),
  then use Save/Restore's [fresh-registry admission boundary](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#restore-is-not-an-assumed-crash-and-rejoin-recipe)
  and [durable-image identity map](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#embedded-durable-image-identity-matching).
  The focused public multi-member case explicitly destroys and recreates the
  same execution name before Restore under both callback models. It constructs
  a new registry over the same filesystem store within one OS process; it does
  not establish an RTI process restart or arbitrary crash recovery. The exact
  normative persistence scope remains a §4.1.2 question.
- **From a participant's last report to a federation outcome:** Synchronization's
  [achievement fan-in](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md#achievement-is-a-fan-in-not-a-per-caller-completion)
  shows how the final report closes the point and creates participant-specific
  `federationSynchronized` notifications. Compare Save/Restore's
  [Save completion boundary](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#configured-process-completion-participant-success-versus-federation-outcome)
  and [Restore completion boundary](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#final-participant-completion-and-restore-operation-lifetime):
  participant completion, a configured-process private `applied` response, and
  the later federation outcome are separate events. Callback and Service
  Ordering traces [process Synchronization responses and events](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#process-endpoint-service-responses-and-synchronization-events-are-separate);
  the Save/Restore guide also documents a [source-only pushed-notification
  failure edge](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#pushed-notification-failure-after-save-state-resolution).
  `HLA_IMMEDIATE` versus `HLA_EVOKED` and process push versus polling are
  separate choices. These paths do not establish a total callback order across
  recipients or protocols.
- **From execution existence to what a federate observes:** start with the
  Lifecycle guide's [catalog and member-roster snapshots](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#the-execution-catalog-and-member-roster-expose-different-state),
  then compare the execution-scoped `HLAfederatesInFederation` reflection in
  the [Federation MOM guide](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#one-membership-change-two-mom-observation-paths)
  with each member's `HLAfederate` discovery/removal in the
  [Joined-Federate MOM guide](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md#readers-model-membership-is-not-observer-knowledge).
  The listing services return request-time snapshots; MOM callbacks follow
  receiver object knowledge and are separate observations. The Lifecycle
  guide's [final-member Resign/Destroy boundary](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#final-resign-and-destroy-have-distinct-observer-paths)
  explains why the embedded terminal membership reflection has no remaining
  joined receiver, while a connected nonmember can query the empty roster before
  Destroy or the missing-execution result afterward. Those terminal listing
  outcomes are source-derived and not covered by a focused test; the
  configured-process membership-reflection path remains separate.
- **From regional scope inputs to callback observation:** the [RAVU guide's
  regional request map](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md#regional-request-scope-is-not-response-ddm)
  separates provider solicitation from a later value-bearing Update. Compare
  that with the DDM guide's [accepted source snapshot and receiver projection](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#process-boundary-case-send-time-source-snapshot-receive-time-receiver-projection),
  then follow the Attribute Scope guide's [process stale-work checkpoints](HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md#process-event-transport-has-two-stale-work-checkpoints).
  The three guides describe different service paths: HLA receiver eligibility
  does not prescribe Umbra's queue checks, and no universal callback cutoff or
  ordering is claimed. The process scope-advisory test covers ordinary
  transitions, not a stale-event race.
- **Ownership and observable updates:** start with the Ownership guide's
  [four ownership paths](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#four-ownership-paths-who-initiates-and-what-reports-the-outcome)
  and [call, per-attribute outcome, and observed-ownership comparison](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#compare-the-call-per-attribute-outcome-and-observed-ownership).
  They separate requester-initiated and owner-initiated transfer paths,
  service return values, callbacks, and observed owner state. For the
  `HLA_IMMEDIATE` service-stack timing boundary, see Callback and Service
  Ordering's [callback-on-service-stack map](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#hlaimmediate-callback-work-can-be-on-the-service-stack).
  Then follow value production and observation through [object and interaction
  information](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md) and
  [callback and service ordering](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md).
  Branch to [attribute value update requests](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md)
  for provider solicitation, [update-rate admission](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md)
  for rate control, or [ownership persistence and resignation](HLA-2025-OWNERSHIP-PERSISTENCE-AND-RESIGNATION-FLOW-GUIDE.md)
  for ownership changes around exit and restore.
- **From pending ownership work to lifecycle cleanup:** compare the Ownership
  guide's [accepted object-deletion flow](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#accepted-object-deletion-invalidates-pending-ownership-work)
  with the Lifecycle guide's [ResignAction map](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#resignaction-names-requested-cleanup-they-are-not-the-leave-transition)
  and [pending-acquisition preflight](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#the-pending-acquisition-preflight-is-role-sensitive).
  For receiver-side assumption/removal events waiting through Resign, follow
  the Object/Interaction guide's [receiver Resign trace](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#receiver-resign-while-assumption-and-removal-callbacks-are-pending)
  and the Callback guide's [process queue cleanup](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#process-profile-resign-and-queued-callback-work).
- **From pending ownership work through Save/Restore:** compare Ownership
  Persistence's [saved release request](HLA-2025-OWNERSHIP-PERSISTENCE-AND-RESIGNATION-FLOW-GUIDE.md#1-a-pending-release-callback-across-save-and-restore)
  with its [live callback queue versus restored work](HLA-2025-OWNERSHIP-PERSISTENCE-AND-RESIGNATION-FLOW-GUIDE.md#4-a-pushed-callback-queue-differs-from-restored-ownership-work),
  then read Save/Restore's [protocol-specific dispositions](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#in-flight-work-has-protocol-specific-restore-dispositions).
  A restored release request is not itself an ownership transfer; a live
  process callback frame is not the same thing as reconstructed ledger work.
  These are selected cases, not a general persistence or crash-recovery rule.
