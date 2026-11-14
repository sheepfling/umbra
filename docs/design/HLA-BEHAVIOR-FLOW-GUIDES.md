# HLA Behavior Flow Guides: Umbra Backlog

This is the living index for contributor-facing HLA flow diagrams in Umbra.
The guides explain difficult behavior before asking a reader to navigate the
implementation; they are not replacements for IEEE standards, implementation
contracts, or test evidence.
The [2025-only documentation roadmap](HLA-2025-DOCUMENTATION-ROADMAP.md)
summarizes the remaining visual, standards-text, and evidence-boundary work.
For per-diagram identities and current screenshot-review status, see the
[2025 diagram visual-review ledger](HLA-2025-DIAGRAM-VISUAL-REVIEW-LEDGER.json).
**Start here:** read the [2025 cross-cutting model of state, events, and
observation](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md) for the
portable HLA reading lens, then use the roadmap's
[API-to-guide map](HLA-2025-DOCUMENTATION-ROADMAP.md#2025-c-api-to-guide-coverage)
to find the service family you need. The topic guides then supply the
service-specific state, callbacks, evidence, and known limits.

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
  Subscription State guide's [late-subscription comparison](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#late-subscription-can-mean-two-different-receiver-states)
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
  [already-known object case](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#late-subscription-can-mean-two-different-receiver-states)
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
  Ordering's [callback-on-service-stack map](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#hla_immediate-callback-work-can-be-on-the-service-stack).
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

## Documentation status snapshot — 2026-11-04

This is the short action view; the detailed topic inventory, evidence notes, and
history remain in the [prioritized guide backlog](#prioritized-guide-backlog).
“Covered” means a reader-facing guide exists and has been surveyed, not that the
topic is normatively complete or every diagram has been visually reviewed.

| Status | Work |
| --- | --- |
| **Covered** | The principal 2025 areas have dedicated guides: time management, ownership, DDM/regions, object and interaction information, subscriptions/relevance, FOM admission, lifecycle/synchronization, save/restore, callbacks/reporting, MOM, encoding, names/handles, authorization, and process-endpoint flows. See the backlog for profile limits and evidence gaps. |
| **2025 public API/callback symbol review** | The family map covers the pinned 2025 service and callback surfaces. The follow-up makes bulk name-reservation subset callback names and the explicit-MIM Create method directly findable, and states that current callback submission order is not asserted by focused tests. No major guide family or extra diagram was needed. |
| **2025 non-timing topic survey and execution observability** | The pinned API family map has a dedicated HLA-first guide or clear cross-link for each principal 2025 service/callback family; no missing major topic emerged. A new thematic route connects the [Lifecycle terminal observer boundary and catalog/roster snapshots](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#final-resign-and-destroy-have-distinct-observer-paths) to the [Federation MOM membership reflection](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#one-membership-change-two-mom-observation-paths) and [Joined-Federate MOM object lifecycle](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md#readers-model-membership-is-not-observer-knowledge). Focused tests do not request an existing empty roster or query a destroyed execution. Embedded source shows the final one-to-zero membership update has no remaining joined receiver; the configured-process Resign path has no matching membership-list enqueue, and neither path has a post-Destroy observer test. Continue targeted reader-gap review; do not add guides to increase counts. |
| **2025 joined-federate MOM query-target cross-review** | The [MOM reports guide](HLA-2025-MOM-OBJECT-AND-INTERACTION-REPORTS-FLOW-GUIDE.md#query-sender-reported-target-and-report-observer-are-separate-roles) maps `HLAfederate` decoding, planner target regions, and focused request shapes across report families. The embedded recipient planners use a target-derived `HLAfederate` endpoint region; focused cases subscribe without a region and do not directly assert dimension filtering. Their `sentRegionsSupplied == false` checks concern the callback sent-region argument, not that internal region. Object-instance information remains the reviewed sender-versus-target exception. This is source/test-boundary documentation, not configured-process evidence or a conformance claim. No chart was added. |
| **2025 configured-process nonregional receive-order interaction path** | The [Process Endpoint guide](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#process-mapping-an-interaction-request-and-its-recipient-stream) already traces public request, service admission, registry planning, per-recipient push/poll, and callback bridge. The raw public-send case verifies bytes through polling; the separate multi-recipient case verifies exact-class callback FIFO under its HLA_EVOKED polling and HLA_IMMEDIATE push fixture. Existing charts and evidence notes are sufficient; no diagram change was warranted. The tests do not establish callback order relative to sender return, inherited-class projection, regional routing, or general callback-model/transport pairing. |
| **2025 configured-process AttributeUpdate Push/Pull checkpoints** | The [Process Endpoint map](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#process-mapping-receive-order-object-attribute-update) distinguishes a full pushed event, a readiness marker followed by deferred claim/recheck, and a polling response with a rechecked `attributeEvent`. The regional process RAVU test pairs Push with `HLA_IMMEDIATE` and Pull with `HLA_EVOKED`; that is fixture configuration, not a callback-model rule. Time Management and RAVU link to the receiver-path explanation. No normative or profile-parity claim is made. |
| **2025 configured-process cross-family callback queue order** | The [Process Endpoint guide](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#cross-family-queue-order-and-test-boundary) now includes a client-routing flowchart: unsolicited frames and typed poll results enter the heterogeneous `pendingEvents_` FIFO, while an interaction returned by `receive_interaction` is submitted directly by callback-control; both routes converge on the bridge and shared callback dispatcher. The correlated request attempts to submit buffered push frames before returning its poll result, but a competing client dispatch lease prevents a total-order guarantee in every interleaving. Existing process tests assert interaction-before-grant and reflection-before-grant separately; no focused mixed-family callback-entry sequence was found. This documents source scheduling, not a portable HLA callback-order rule. |
| **2025 process Federate-Lost report and local callback paths** | The [Federation MOM guide](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#configured-process-report-delivery-and-local-loss-use-separate-paths) traces survivor Push/Pull delivery and the failed member's local `connectionLost` callback as independent paths. Push enters `pendingEvents_`; a polled interaction returns separately and dispatches directly to the bridge. Callback-disable holds each recipient's dispatcher task. If a survivor unsubscribes before release, embedded delivery recomputes the current route at callback entry; configured-process delivery retains the service-captured projection, with no unsubscribe-driven report purge or report-specific callback recheck found. Embedded forced loss resets the departing dispatcher; process local loss preserves its disabled state. Existing embedded regional tests unsubscribe only after observing the report; no loss-specific disabled/unsubscribe case or process report test was found. This remains source-derived, not a normative delivery rule. |
| **2025 configured-process Resign and ConnectionLost queues** | The [Callback and Service Ordering guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#process-profile-resign-and-queued-callback-work) distinguishes successful Resign cleanup from connection-loss retention: success clears the client FIFO and pending dispatcher tasks, while a task already selected can finish. ConnectionLost retains the failed client and can race its local task against buffered events; public `Disconnect()` is rejected in the resulting `NotConnected` state. A later `Connect()` or ambassador destruction closes the old client, clears remaining FIFO events, closes its callback session, and resets pending dispatcher tasks, potentially including a not-yet-selected `ConnectionLost` task. No focused public process test covers either pending-work edge or establishes callback-entry order. A late frame after a later Join remains an open source/test boundary. These are implementation observations, not normative ordering claims. |
| **2025 configured-process Disconnect/reconnect callback lifetime** | The [connection guide](HLA-2025-CONNECTION-AND-CONFIGURATION-FLOW-GUIDE.md#possible-process-reconnect-during-off-lock-disconnect-cleanup) now diagrams the source-level overlap: after Disconnect publishes `not_connected`, Connect can install a replacement session before old client cleanup resets the shared dispatcher. If replacement work is pending, that reset can clear it. No focused test covers the interleaving; the process Connect/Disconnect case has no queued callback and same-ambassador reconnect is embedded-only. The pinned [RTIambassador interface](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L46) has no general thread-safety statement; behavior and contract scope remain unresolved. |
| **2025 configured-process AttributeUpdate callback gating** | The [Callback and Service Ordering guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#configured-process-attributeupdate-receipt-and-disabled-callback-gates) separates stream/poll receipt, the client FIFO, bridge submission, and callback entry. The cross-review confirms two independent gates: `enableCallbacks()` opens the callback dispatcher but does not drain the client FIFO; already-submitted `HLA_IMMEDIATE` work may drain inline, while `HLA_EVOKED` still requires Evoke. A later client drain submits events after the gate opens. Full reflections capture their event; deferred markers claim and recheck authoritative state only when their callback task runs. Public evidence includes an immediate Push/deferred-marker outcome, an evoked Pull/TSO reflection with explicit disable/enable, and a separate evoked Pull ordinary reflection that rechecks eligibility at callback admission. No focused case inspects private queues or pairs a deferred marker with `HLA_EVOKED`. The chart describes current source behavior, not a normative transport rule or profile parity. |
| **2025 update-region batch failure boundary** | The [DDM/regions guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#a-stale-region-rejects-the-association-before-the-map-changes) now shows whole-input validation before active/deferred association-map changes. A focused embedded case observes `InvalidRegion` and later `attributesOutOfScope` from the original association. The configured-process handler enqueues events after registry commit; its post-commit enqueue-failure path is source-only and does not establish an HLA exception schedule. |
| **2025 whole-class versus selected-subset unsubscription** | The [subscription-state guide](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#whole-class-and-selected-subset-unsubscribe-have-different-observed-effects) now contrasts both public services. The subset case preserves `Name` delivery across two existing objects and leaves a peer's full subscription intact; the whole-class case observes no later reflection for an existing object and no discovery for a later registration. Both recorded portable C++ TCK runs passed under `HLA_EVOKED` and `HLA_IMMEDIATE`. The empty-set subset meaning remains unresolved; neither test asserts an `attributesOutOfScope` advisory or independent-provider conformance. |
| **2025 receive-order class selection and inherited parameters** | The [object-and-interaction guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#the-same-sent-class-at-three-subscription-depths) now shows the receiver adding Base, Middle, and exact-class subscriptions before three sends of the same Grandchild interaction. The closest subscribed class is reported, its inherited parameters are projected, and each send produces one callback. The recorded installed-package C++ TCK passed under both callback models; this case does not establish licensed-clause interpretation, directed/TSO behavior, or independent-provider interoperability. |
| **2025 interaction relevance and publisher advisories** | The [interaction guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#interaction-relevance-class-promotion-and-parameter-projection) now explicitly connects federation-wide active relevance to publisher-facing `turnInteractionsOn`/`turnInteractionsOff` advisories, while keeping those class-level signals separate from per-send receiver delivery and regional overlap. The focused embedded declaration test observes active/passive On/Off transitions. No additional diagram was needed because the interaction-delivery flow and relevance-advisory state chart already show the two sides; process behavior and exact normative conditions remain open. |
| **2025 active/passive object and interaction relevance** | The [Object/Interaction guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#why-passive-declarations-use-service-specific-relevance-gates) compares the service-specific gates, and the [Object-Attribute Subscription guide](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#2-discovery-and-value-delivery-are-separate-active-subscription-gates) shows the object discovery A/B cross-attribute example and its profile-specific commit boundary. Object discovery can be opened by one relevant instance attribute while a receiver separately matches another; interaction relevance is class-scoped, while a passive regional recipient still needs its own overlapping pair. The DDM guide explains region qualification, and the DSE guide's callback gate is interaction-specific. Existing diagrams already show the paths, so no duplicate chart was added. Cross-member outcomes remain source-bounded: the embedded passive-only test has no separate active establisher, and the configured-process subscription test has no ordinary object or separate active member. The process three-role discovery result remains untested. |
| **2025 passive interaction candidates with DSE** | The [DSE guide](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md#an-interactions-passive-gate-is-recomputed-with-its-active-relevance) now explains that the embedded callback-entry selector recomputes both active class relevance and the receiver's current subscription/region. A retained passive route can become eligible or be suppressed as another member's active declaration changes before dispatch. The focused DSE interaction test changes the receiver's own declaration; the three-member transition remains source-only. Configured-process receive-order events have a separate event-construction boundary and no equivalent callback-entry recheck shown. Existing DSE diagrams already show the callback gate; no duplicate chart was needed. |
| **2025 configured-process ordinary interaction DSE route** | The [Process Endpoint](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#process-mapping-an-interaction-request-and-its-recipient-stream) and [DSE](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md) guides now link to the existing [Object/Interaction projection sequence](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#process-ordinary-interactions-snapshot-the-promoted-callback-view). That sequence already shows route-only candidate resolution before event construction, event-time projection, Push/Pull delivery, and callback-bridge dispatch, so no duplicate process chart was added. The inspected process bridge has no equivalent callback-entry eligibility recheck; no focused process interaction DSE mutation test was found. These process details remain source-derived, without an embedded-parity or normative claim. |
| **2025 configured-process interaction callbacks while disabled** | The [Callback and Service Ordering guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#a-process-receive-poll-can-admit-a-task-while-callbacks-are-disabled) distinguishes process event receipt from callback execution: an `HLA_EVOKED` receive poll can admit one interaction task while disabled, and a later Evoke dispatches after re-enable; the focused `HLA_IMMEDIATE` fixture uses pushed delivery and drains on enable. This is source/test evidence for that configured-process path only; its private transport pairing is not an HLA rule. |
| **2025 configured-process Evoke Multiple receive pump** | The [Callback and Service Ordering guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#process-evoke-multiple-separates-event-admission-from-callback-draining) separates the pre-dispatch receive pump, timed callback drain, and post-wait admission. If a callback task is already queued, the initial pump is skipped even if the client FIFO still has frames; the post-wait pass attempts to submit any frames left there, regardless of when they arrived. If another thread already owns the client dispatch lease, that call can return before admission; the Evoke result reflects the callback-dispatcher queue, not unsent client-FIFO work. The background client dispatch loop or a correlated request can also submit during the timed wait, and the pump avoids another blocking poll while callback admission is in progress. The generic [dispatcher minimum-wait case](../../cpp/tests/callback_dispatcher_catch2.cpp#L335) covers dispatcher wakeup, but not the process reader/client FIFO/bridge handoff. The focused public [process case](../../cpp/tests/ieee1516_2025_connection_receive_evoke_catch2.cpp#L743) covers one event admitted while callbacks are disabled; enabled multi-poll drain, pre-buffered FIFO with a queued task, dispatch-loop handoff during the timed wait, and the competing dispatch lease remain source-derived. No normative poll-count or process-profile parity claim is made. |
| **2025 Restore notifications during member exit** | The [Save/Restore guide](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#member-exit-during-an-active-restore) now traces registry planning through member-exit dispatch and the ordinary process Restore event pipeline. The embedded resignation case observes survivor failure callbacks; the configured-process dispatcher omits Restore notifications for both voluntary Resign and connection loss before serialization. No focused process test covers active Restore followed by either exit. The Lifecycle guide's existing chart already shows the profile routes, so no duplicate diagram was needed. This is a source-level process projection gap, not a runtime callback, profile-parity, or normative failure-reason claim. |
| **2025 reverse-order TSO arrivals during pending NMR/NMRA** | The [Time Management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md#reverse-order-arrivals-retain-the-later-tso-across-the-grant) now traces T=18 then T=15 while NMR(20) is pending: E retargets to 15, the callback precedes grant 15, and T=18 remains queued for NMRA(25) and grant 18. The focused configured-process case covers HLA_EVOKED and HLA_IMMEDIATE. Equal-target arrival and retraction among multiple queued messages remain untested in this scenario. |
| **2025 process NMR/NMRA grant reevaluation after Retract** | The [Time Management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md#a-later-retract-removes-tso-work-without-restoring-the-old-target) now distinguishes TSO removal from grant scheduling: process Retract itself does not reevaluate, while a later ACK for a retired delivery or another accepted time-advance request can trigger a scheduler pass. The combined later grant/callback result remains untested; focused NMR/NMRA delivery and process Retract tests cover separate sequences. |
| **2025 configured-process RECEIVE gate** | The [Time Management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md#configured-process-receive-order-interaction-gate) distinguishes the bridge-local interaction gate from the embedded `FederateTimeState` gate; its [process AttributeUpdate order split](HLA-2025-TIME-MANAGEMENT-GUIDE.md#process-endpoint-the-same-order-split-crosses-a-different-boundary) traces receiver reflections separately. Focused public interaction tests cover pending advance and asynchronous enable/disable under both callback models. Combined process attribute-reflection coverage remains open; no normative timing or profile-parity claim is made. |
| **2025 configured-process Enable Time Regulation** | The Time Management guide distinguishes a direct enabled-time response from a constrained member's implicit TARA: eligible TSO precedes the regulation-enable event, which produces `TimeRegulationEnabled` rather than a public `TimeAdvanceGrant`. The process-service `HLA_EVOKED` case observes the named TSO-before-role-callback sequence; the `HLA_IMMEDIATE` callback-route case uses a controlled server. Source/test behavior only, with no normative target formula or process/embedded parity claim. |
| **2025 additional-module Join lookup boundary** | The Name/Handle guide now summarizes embedded public evidence that existing object-class, attribute, interaction-class, parameter, and transportation-type handles survive a compatible module Join while selected new symbols resolve. Dimension-directory reconciliation is source-only; the configured-process test proves added object-class lookup only. The FOM guide already explains atomic definition/membership commit and post-commit setup limits, and the Object/Interaction guide already links catalog growth to its non-callback boundary, so no duplicate map was needed. |
| **2025 receiver Resign around regional TSO** | The TSO retraction guide separates embedded recipient-gate rejection from configured-process event/acknowledgement phases. It records an untested source-level process Region-copy lease cleanup gap; no public callback or HLA outcome is claimed. The Time Management guide cross-links the temporal boundary. |
| **2025 Evoke Multiple wait window** | The callback/service-ordering guide now distinguishes its monotonic elapsed-time window from HLA logical time and shows that the implementation checks the maximum only after a callback returns. The focused dispatcher test covers an already-queued FIFO batch, not empty-queue waits, late arrivals, or deadline overshoot. This is a 2025-source explanation, not a normative §10.58 interpretation. |
| **2025 pending transportation change and unconditional divestiture** | The order/transport guide now shows the embedded `HLA_EVOKED` path: queued confirmation rechecks its request ID after unconditional divestiture; a partial request continues for remaining owned attributes, while an erased or ineligible request produces no confirmation. The revised lifecycle state chart and new sequence were rendered and screenshot-reviewed locally. The save/restore interlock rejects both services, and the custom-control test confirms a different object before divesting, so neither covers the interleaving. This remains an implementation observation, with no normative ordering or process-profile claim. |
| **2025 acquisition cancellation and divestiture notification** | The ownership guide already traces cancellation-reservation-first versus notification-begin-first and distinguishes configured-process push and pull commit points. The focused process cancellation and divestiture cases are separate; the embedded cancel-before-divest case concerns later assumption-search eligibility, and the save/restore interlock rejects both calls. No combined process race test or HLA ordering guarantee is claimed. This review corrected shifted `receive_interaction` source anchors; no duplicate diagram was needed. |
| **2025 forced connection loss during synchronization** | The [synchronization guide](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md) now shows how the shared member-removal path re-evaluates a pending barrier: keep it pending, close for remaining reporters, or erase it when none remain. The chart separates synchronization completion from automatic `ResignAction` cleanup and shows embedded callback versus configured-process event projection. This remains source-derived; no focused forced-loss-during-active-synchronization test was found. The new chart passed local render and screenshot review. |
| **2025 synchronization participant-set and partial-state cross-review** | The synchronization and Lifecycle guides already distinguish an omitted-set late-Join expansion from explicit-set barriers and from membership loss. The Save/Restore guide keeps its Join-admission question separate and links the synchronization ledger's persisted participant, announcement, expansion, and achievement state. A codec fixture preserves a `false` result, but public Restore tests cover a single-member unachieved point, not a multi-member partially achieved barrier; the partial-achievement-before-late-Join sequence is also source-derived and untested. Existing routes and diagrams explain these boundaries, so no duplicate chart was warranted. |
| **2025 partial-achievement then late-Join cross-check** | The synchronization guide's existing sequence and state view match the registry: an expandable pending point adds the newcomer while retaining earlier per-federate results, and the current participant-set size controls when the barrier closes. The focused late-join test joins before any achievement; the public test does not combine a partial report with a later Join. Configured-process handling delegates to the same registry transition, but no focused process test covers that ordering. The guide and roadmap now state this evidence boundary. No duplicate diagram, test, build, normative completion rule, or 2010 claim was added. |
| **2025 synchronization repeated-achievement result** | Registry `emplace` preserves the first boolean for a participant, so a later opposite result does not replace it in Umbra's current implementation. Public tests report once per participant and do not exercise the repeat. The synchronization guide records this as source-derived and leaves normative interpretation open. |
| **2025 resignation after synchronization achievement** | Shared cleanup removes the departing member and any stored result before checking remaining reports. The focused HLA_EVOKED test has one successful remaining member and one unachieved departing member; it does not test departing after `false` or a departure that leaves the barrier pending. The guide separates source behavior, that tested case, and normative uncertainty. |
| **2025 outside-set registrant departure** | The synchronization point stores no registrant identity; departure of a registrant outside the explicit participant set leaves the pending point's barrier sets unchanged. Configured-process receiver-only tests resign the sender only after completion, so pending-interval behavior remains source-derived. The guide now states that timing boundary. |
| **2025 synchronization registration rejection** | Embedded duplicate-label tests confirm rejected registration preserves the original point through later Join and completion; configured-process HLA_EVOKED/HLA_IMMEDIATE tests check duplicate failure and caller-not-in-set rejection. Registry membership preflight for a requested nonjoined member lacks focused implementation coverage. The synchronization guide now separates this gap from tested duplicate behavior. |
| **2025 synchronization announcement delivery cross-check** | Existing guide evidence already distinguishes embedded submission order from process event queueing and client dispatch. The HLA_EVOKED same-participant process case observes success before the sender's announcement across separate callback service calls; receiver-only process cases do not prove that ordering for one recipient, and no configured-process HLA_IMMEDIATE same-participant case was found. No new diagram was needed; next review is initial-registration announcement enqueue failure after registry commit. |
| **2025 initial registration announcement enqueue failure** | Added a configured-process flow for registry commit followed by announcement enqueue/send failure: the service returns an internal error, the client maps it to `RTIinternalError`, and the point is not rolled back. Polling may retain already queued recipient events; push staging-hook and transport-send failures have distinct possible effects. The existing failure injection covers late-Join rollback in polling mode only, not initial registration. Rendered and reviewed the new diagram locally; no tests/builds or HLA transaction claim. |
| **2025 configured-process Auto Provide discovery cross-check** | Process source commits discovery and plans ordinary selected initial values separately; the scanned Auto Provide planner call remains embedded-only. The focused process late-subscription case proves selected-value reflection but does not instrument provider solicitation; the separate switch test checks state without discovery. Clarified that neither proves process Auto Provide presence or absence. No diagram or tests were needed. |
| **2025 regional RAVU same-object provider grouping** | The registry partitions each object's requested attributes by current owner and filters regional eligibility per attribute. The process service narrows persisted request-region pairs to each group and carries the original request tag into that group's projected event; region handles are not event fields. Focused tests cover one provider/object/attribute, not one object's split across providers. Clarified the guide's source example and exact evidence boundary; no diagram or tests were needed. |
| **2025 regional RAVU invalid-pair whole-request preflight** | Source validates all supplied pairs before provider expansion. The embedded filtering case has provider-owned objects, submits invalid single-pair requests, then observes an empty callback queue; it does not combine invalid and valid pairs in one request. The configured-process exception-mapping case has no provider route and does not establish event suppression. No focused pending-ledger or corrected-retry assertion exists. The RAVU guide now distinguishes these evidence scopes from source control flow. |
| **2025 FederationSynchronized recipient departure** | The callback guide now traces successful process Resign suppression and cleanup: the client skips its normal callback drain, clears buffered events and pending bridge tasks, then releases suppression. Embedded Resign and forced loss also clear queued dispatcher tasks. A frame that crosses the local cleanup boundary remains unverified. Existing tests observe synchronization callbacks before later Resign; no focused post-departure case was found. The corrected sequence was rendered and screenshot-reviewed locally. |
| **2025 joined-federate MOM removal route outcomes** | The lifecycle guide now compares the registry's explicit-Resign route preflight with forced `ConnectionLost`: an unavailable route rejects ordinary Resign before state mutation, while forced cleanup can skip that observer. The shared flow then shows how planned removals clear known/pending state and when the private snapshot is erased or retained. Focused embedded and process tests cover successful explicit Resign only; the missing-route and forced-loss branches remain source-derived. The revised chart was rendered and screenshot-reviewed locally. |
| **2025 Remove callback exception boundary** | Added separate sequences for embedded inline versus evoked callbacks and configured-process Push versus Pull. They show when the receiver becomes unknown, whether the producer is still on the submission stack, and whether the process server can observe a callback failure. The focused MOM removal cases use non-throwing callbacks; no exception recovery or cross-profile parity is claimed. Both diagrams rendered and passed local screenshot review. |
| **2025 receive-order versus timestamp-order at a grant** | No new diagram was warranted. The [time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md#timestamped-message-lifecycle-and-callback-order) already separates embedded TSO callback completion from the matching grant, and its [embedded mixed-recipient case](HLA-2025-TIME-MANAGEMENT-GUIDE.md#one-timestamped-update-two-recipient-paths) and [process order-split flow](HLA-2025-TIME-MANAGEMENT-GUIDE.md#process-endpoint-the-same-order-split-crosses-a-different-boundary) contrast RECEIVE and TIMESTAMP delivery paths. Focused tests cover bounded scenarios, not universal profile parity. |
| **2025 FQR actual grant, optimistic floor, and arrival cutoff** | The time-management guide already separates queued TSO delivery, actual grant, and optimistic floor, and its embedded HLA_EVOKED sequence shows the callback-time extraction boundary. The profile table keeps the configured-process acceptance-time result distinct; neither policy is presented as prescribed by §§8.12–8.14. Fixed a semicolon that prevented Mermaid parsing, shortened the sequence participants, then rendered and screenshot-reviewed the existing chart. No duplicate process sequence was needed. |
| **2025 NMR/NMRA late-arrival retraction boundary** | The time-management guide now separates the tested request-time-selected NMRA target from the source-only late-arrival retarget case. The inspected 2025 retraction path removes queued/in-transit TSO state without recomputing E; an embedded test shows an already selected E=8 remains the grant after retraction and the reflection is suppressed. No focused case combines late-arrival retarget, retraction, and the next grant trigger, so that result remains open. No duplicate chart or normative claim was added. |
| **2025 cross-federate pending TAR** | The time-management guide shows one configured-process path where a constrained member's TAR(5) remains pending until a regulating peer also advances to 5, after which the grant is observed. The focused case covers the transition under both callback models; the guide does not infer a GALT value, TSO behavior, a complete trigger set, or normative callback timing. A follow-up found that configured-process Resign and embedded forced member removal both re-evaluate pending grants in source, but no reviewed focused test shows departure alone releasing one. The lifecycle guide's lost-regulator TSO cutoff remains a separate forced-loss case. The FOM module-admission review found no new reader gap; its existing guide already explains composition, admission, commit, and bounded post-commit effects. |
| **2025 directed-interaction profile boundary** | A bounded review found the existing guide already contrasts embedded callback-time recipient re-planning with configured-process receive-order event materialization. The guide identifies the missing process test for declaration/target mutation after send acceptance; its exact-class multi-recipient test does not establish stale-event behavior. The review repaired stale Catch2 anchors for push-mode configuration and callback-model sections. No duplicate diagram is warranted, and the §4.4 normative scope remains open pending authorized text. |
| **2025 object-instance lifecycle survey** | The latest cross-review confirms the existing [reservation lifecycle](HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md#4-reservation-ownership-and-lifetime) and [same-name recreation](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#same-name-recreation-is-fresh-restore-must-be-explicit) diagrams cover the state edges: named registration consumes the owner's reservation; Resign releases an unconsumed reservation; a registered name remains occupied until global object/name purge; Restore reattaches saved reservations to their federate identity; Destroy/recreate is fresh unless an image is explicitly restored. Receiver-local forget is a separate view transition. Focused tests cover explicit-Resign reuse, registration consumption, and restored owner identity in separate cases; no one test spans the whole lifecycle. Forced-loss reuse and configured-process purge behavior remain source-only. No extra cross-cutting diagram was needed. |
| **2025 process NMR/NMRA after TSO Retract** | The Time Management and TSO Retraction guides trace queue withdrawal, the retained effective target, and later scheduler triggers. Process NMR/NMRA delivery cases do not invoke Retract. A separate process attribute test observes a pending TAR(5) grant after retracting an update at 5, but does not establish the NMR/NMRA sequence or its callback order. No new diagram was needed. |
| **2025 process TSO acknowledgment after Retract** | The pull-mode interaction case observes ACK after pre-callback Retract but leaves its order relative to two API fences unspecified. A pushReceiveOrderEvents-enabled Resign/rejoin case observes callback suppression but makes no ACK assertion. The existing client-buffer/bridge flow is sufficient; no new diagram was needed. |
| **2025 process MOM report/error timing** | The Exception Reporting guide now keeps the pushed report event and rejected service result on separate observation paths: client-side event dispatch precedes status mapping, `HLA_EVOKED` observes the callback on a later Evoke, and the `HLA_IMMEDIATE` path remains source-only. The focused case asserts caller error then later MOM report; it does not assert `HLAreportException` or all three report routes. No normative or profile-parity claim. |
| **2025 FOM additional-module Join transaction review** | The existing guide separates complete-set validation/composition, shared registry commit of the replacement definition and handle directories with new membership, and profile-specific post-commit setup/rollback. The focused process fixture checks unchanged module count after one pre-commit rejection and a valid retry; no focused test covers post-commit failure or replacement while populated members remain. The Lifecycle guide's three-lifetime map supplies the outer connection/membership/execution context. Existing diagrams and evidence map are sufficient; do not infer a tested rollback guarantee or normative/profile parity. |
| **2025 subscription-state late-subscription visual review** | The revised receiver-state chart distinguishes discovery-triggered initial values for a newly known object, unresolved current-value catch-up after a known-object subscription, and a later update evaluated against current ownership/subscription state. The generic update reflection path has public process test evidence, but no focused case combines it with a Join-added attribute and a prior-known receiver. The chart rendered legibly without clipping at Mermaid CLI 12.0.0. No normative §5.8 conclusion or cross-profile parity claim. |
| **2025 joined-federate MOM removal callback review** | Three existing charts were rendered/reviewed: explicit Resign versus forced-loss route handling, embedded immediate/evoked callback failure, and the process Push/Pull boundary. Their labels are readable and unclipped. These remain source-only where no focused throwing-callback test exists; no new chart, normative claim, or cross-profile inference was added. |
| **2025 Save/Restore process callback-failure review** | The [Save/Restore guide](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md) sequences distinguish terminal callback observation from service-report timing, and immediate initiation callback failure after server-side save admission from the evoked queue path. Both existing charts rendered clearly with no clipping. Throwing-callback outcomes remain source-derived; no focused public process test exercises them. No chart rewrite, normative claim, or profile-parity inference was added. |
| **2025 deletion authority and producer identity review** | The [Attribute Ownership guide](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md) transfer sequence reads clearly. Its configured-process identity sequence was narrowed to the requester-B versus snapshot-A boundary; the revised render is readable and unclipped. The distinct-registrant process path remains untested, so no normative or process-parity claim is made. |
| **2025 ResignAction callback-path visual review** | The preceding flowchart already owns the admission gates and final-member delete overlay; the sequence now traces only a successful embedded commit and survivor-route dispatch. Mermaid CLI 12.0.0 screenshot review passed. The focused evidence is one embedded `HLA_EVOKED` action case, not a general callback-order or process-parity claim. |
| **Remaining annotation** | The major 2025 API areas have reader-facing guides; remaining annotation is driven by demonstrated reader gaps, not method coverage or a diagram quota. Recent process DSE, ResignAction, Connect/configuration, synchronization, callback/service-ordering, and object-discovery/initial-value reviews found existing diagrams sufficient for the bounded questions surveyed. The process DDM/Attribute Scope stale-event audit found a useful implementation distinction: server-poll events are rechecked at dequeue, while pushed events are checked before transport and have no later registry recheck shown; the focused process test does not exercise a stale race. A new 2025 source-bounded chart records these cutoffs without turning them into HLA callback-timing rules. The current cross-review links this Attribute Scope path to RAVU provider-solicitation regions and the later Update source snapshot/receiver projection in DDM; these are separate service paths, not one callback timeline. The 2025 active-establisher/passive-receiver review now compares two service-specific gates: object discovery can use one discoverable attribute to open the path while the receiver separately matches another; interaction relevance is class-scoped, and a passive regional receiver still needs its own overlap. Both cross-member cases remain source-bounded: focused passive-only tests have no separate active establisher, the last recorded regional two-receiver interaction run failed at federation creation before its assertions, and process tests check declarations without cross-member delivery. The Object/Interaction guide's comparison table and existing diagrams explain the distinction without adding another chart. The partial-achievement/late-Join synchronization sequence is correctly source-derived, and stale source anchors were corrected. Two focused process tests cover receive-order interaction async-gate transitions under `HLA_IMMEDIATE` and `HLA_EVOKED`; the separate process AttributeUpdate route does not call that gate, and no focused test combines AttributeUpdate with constrained, asynchronous, and pending-advance state. The Request Attribute Value Update guide distinguishes class expansion, per-instance current-owner grouping, requester-owned attributes, provider callbacks, and later reflection; focused tests do not cover one object's attributes split across providers or ownership changes at the embedded pre-enqueue/callback boundaries. The updated chart shows full-set persistence validation separately from callback-time subset projection. It now also traces the conditional source-only path in which an earlier pending record left by a later-group failure is captured and rebound into provider work after restore; no test exercises that failed-request-to-restore chain, so neither a recovery guarantee nor an HLA rule is claimed. The 2025 Order and Transportation guide adds a source- and test-bounded TSO order-conversion example, and the reservation guide adds an embedded restore case. The ownership guide connects transfer of `HLAprivilegeToDeleteObject` to deletion authority and the embedded removal-callback assertion. The process trace maps callback producer identity to the stored registrant snapshot; both focused process deletion tests use the same federate to register and delete, so transferred-owner callback identity remains untested there. Continue bounded 2025 reviews; each candidate must produce either a useful HLA-first explanation or an evidence-backed no-change disposition. |
| **2025 RAVU response boundary** | The later Update call is planned against current provider ownership and update-region state; the original request region is not carried forward. Embedded ownership loss maps to `AttributeNotOwned`. Configured-process remote planner failures differ by overload: instance and class-without-regions requests become generic rejection surfaced as `RTIinternalError`, while regional class requests preserve typed planner status and matching API exceptions. The guide also records source-only whole-request regional validation before provider expansion: the focused test uses separate invalid-pair calls and does not prove suppression of otherwise-eligible work. The new overload map is source-only; focused process tests cover success for the first two and typed invalid-region outcomes for the regional form. No focused case transfers ownership between Provide and response. The active Region-registry source now checks regional dimensions and overlap in the requested attribute declaration-class context. The focused InvalidRegionContext case uses Flavor declared directly on Soda, so it does not distinguish inherited-attribute scope; a separate focused 2025 DDM CTest that shares this helper built and failed at regional subscription; that test does not exercise RAVU admission. No §9.1.5 interpretation is made. A separate reader review distinguishes RAVU provider solicitation from later receiver selection: the inspected update plan does not consume an earlier RAVU record, while enabled Delay Subscription Evaluation can defer ordinary receiver eligibility to delivery. The focused late-subscription DSE test makes no RAVU request, so direct §6.21 entitlement remains unresolved. The DSE guide now links back to the RAVU receiver-eligibility explanation: its focused attribute test makes no RAVU request, while the RAVU tests do not keep the requester unsubscribed through response. The existing service-specific diagrams were sufficient; no combined diagram or entitlement claim was added. See the [RAVU overload map](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md#configured-process-remote-failure-mapping-varies-by-overload), [later-update boundary](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md#the-later-update-call-revalidates-current-provider-state), and [receiver-eligibility review](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md#a-request-does-not-replace-receiver-eligibility). |
| **2025 RAVU and timing-gate composition** | The [RAVU guide](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md#the-solicitation-receive-order-gate-and-tso-frontier-are-separate) now connects provider solicitation, the later value-bearing Update, receiver DDM, receive-order asynchronous/time-advance gates, timestamped TSO eligibility, and callback-model observation. The embedded regional timestamped-response test observes reflection before grant; the process regional RAVU case does not exercise Time Constrained or asynchronous delivery. Process async tests cover interactions, and the inspected Provide callback uses a separate bridge route. Process attribute-reflection parity remains unverified; this is not a normative or cross-profile claim. |
| **2025 time grants and ownership callbacks** | The Time Management, Ownership, and Callback/Service Ordering guides now link the TSO-before-grant path to separate ownership-transfer, assumption, and cancellation outcomes. That timing rule covers selected timestamped deliveries only. Embedded and process callback routes remain service-specific; a bounded Catch2 source scan found no file naming both timeAdvanceGrant and the reviewed ownership callback families. No cross-service callback order or extra diagram is claimed. |
| **2025 pending ownership work through Delete/Resign** | The Ownership, Lifecycle, Object/Interaction, and Callback guides separate per-instance federation Delete, member-scoped ResignAction guards, receiver-side pending assumption/removal events, and process callback queues. Embedded Delete cases cover selected query, release, If-Available, and cancellation callback suppression. The nearest process receiver-Resign case consumes the assumption before Resign; pending process cleanup remains source-derived. Added a thematic reader route and reciprocal Delete/Resign links. No new chart or normative cleanup/order claim. |
| **2025 RAVU fan-out/preflight review** | 2026-10-17 review confirmed the existing [RAVU flow guide](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md) already shows regional-pair validation before provider expansion, grouping by object/current provider, and configured-process preflight rejection, so no duplicate chart was warranted. Added a compact source-derived example of one object's requested attributes splitting across providers. The focused public process test covers one provider across two objects, not this same-object split. A separate private configured-process service test commits a disjoint requester region and verifies zero recipients plus no callback on the next poll for one object/provider/attribute; this is not public RTIambassador evidence. A mixed-validity review confirms source preflights supplied pairs before recipient expansion, while focused tests submit invalid cases separately and do not test suppression of otherwise-eligible work. No focused test covers same-object multi-provider splitting or later group/Push-send failure injection. The persistence review also identifies a source-visible no-rollback path after earlier groups are stored, but this is not a tested runtime scenario. The class-pending restore test covers one object/provider group only. The revised chart has local screenshot review; no normative atomicity claim. A separate process Save/Restore review confirmed that the existing evidence row already distinguishes pending registry records from consumed provider notifications; no focused process callback-count/replay case exists, and no new diagram was warranted because the queue fate is source-only. |
| **2025 MOM synchronization-query profiles** | The chart distinguishes the parameterless active-point list from the named-point status roster and their embedded/configured-process boundary. Embedded source and the focused test cover the list and roster. The MIM marks both requests Subscribe; the configured-process publication registry accepts any defined interaction handle without checking sharing, so the published branch is source-reachable (an implementation observation, not normative permission). If unpublished, Send Interaction is rejected and maps to RTIinternalError; if published, generic send routes the original request to ordinary recipients and creates no paired report. No focused process test executes this request sequence, so actual execution remains unobserved. The core synchronization guide cross-links these query observations separately from registration, achievement, and barrier completion. Mermaid CLI 12.0.0 rendering and local screenshot review pass without clipping or overlap; the selected queue is 58 reviewed/0 pending. See the [combined query flow](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#federation-synchronization-queries-active-point-list-and-roster-status) and [configured-process evidence boundary](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#configured-process-request-route-evidence-boundary). |
| **2025 cross-cutting query versus transition** | The cross-cutting state/event guide now uses the paired synchronization queries as a source/test-bounded example: reports expose the active-point list or a named point's roster, while the embedded request path does not itself announce, achieve, or complete the point. The focused test observes the status change after a separate achievement; configured-process behavior remains unverified. This sharpens the existing `FACTS` → `QUERY` model without duplicating a diagram. |
| **2025 process name-reservation to peer-discovery review** | The existing [reservation guide](HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md#process-reservation-to-discovery-evidence-boundary) already separates the owner's reservation callback, named-registration commit, and later peer discovery. The [process registration handler](../../cpp/src/internal/federation/process_federation_service_request_handlers.cpp#L464) plans discovery only after registration succeeds. The focused [named-registration case](../../cpp/tests/ieee1516_2025_connection_named_object_registration_catch2.cpp#L457) covers reserve/register under both callback models in one client session; a separate [process discovery case](../../cpp/tests/ieee1516_2025_connection_object_discovery_catch2.cpp#L457) registers without a reserved name. No test composes reservation, named registration, and peer discovery end-to-end, so cross-session callback order remains unclaimed. No duplicate chart warranted. |
| **2025 process MOM staging** | A source review distinguishes pull-mode discovery/value event queueing from push-mode sends for initial values. The configured-process MOM case uses an evoked receiver and does not assert discovery/reflection callback order. Ordinary-object process late subscription is separately observed in the [public process case](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L652): it reflects selected stored `Name` values after discovering a previously unknown object, under both callback-model sections ([assertions](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L952)). This still does not settle cross-federate callback order or the already-known-object/new-attribute case. |
| **2025 ordinary late-subscription and embedded reentrancy review** | The current process source plans ordinary initial values as part of a newly accepted discovery, and the focused public test source asserts discovery plus a reflection containing only the selected stored `Name` value under both callback-model sections ([source](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L90), [test](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L652), [assertions](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L952)). The cited process implementation and test files are clean in the current worktree; this documentation pass did not build or run the test, so no passing-result claim is made. The revised chart separates process-side planning from the embedded post-discovery path and shows the embedded callback reentrancy checkpoint: current membership and recipient eligibility are checked after `discoverObjectInstance` returns. The source comment names reentrant unsubscribe, resignation, and region changes; the cited tests do not exercise that fence. It also preserves the unresolved question for adding an attribute to an already-known object. The checked workspace has pinned 2025 API headers but no full standard text; the [official IEEE 1516.1-2025 listing](https://standards.ieee.org/ieee/1516.1/6688/) offers purchase or subscription access, so licensed §5.8 remains unreviewed. No normative callback-order or broad profile-parity claim; the revised chart has been rendered and screenshot-reviewed with Mermaid CLI 12.0.0; it is readable without clipping. |
| **2025 nonregional process receive-order reflection review** | The existing [process endpoint guide](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#process-mapping-receive-order-object-attribute-update) already separates untimed update planning/commit, per-recipient push versus polling transport, and receiver callback dispatch. It explicitly says `receive_interaction` is the polling fence, pushed attribute frames use `receive_attribute_update`, and consuming an event does not itself invoke the callback; callback scheduling and transport mode are separate, with no cross-federate callback-order claim. The [focused public process reflection case](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L457) observes one byte-preserving reflection under both callback models ([value assertion](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L821)). In this fixture only, `HLA_IMMEDIATE` selects pushed transport and `HLA_EVOKED` selects polling ([fixture setting](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L545), [callback-model sections](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L880)); this is not a universal pairing rule. No additional diagram was warranted. |
| **2025 process Auto Provide** | Re-audit confirmed the existing [Auto Provide guide](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md#configured-process-discovery-route-source-only-boundary) already has the process discovery/switch chart and evidence table. The only `planAutoProvideForDiscovery` call site remains the embedded discovery callback. The process subscription handler replans and commits discovery, then the event service delivers discovery, separate ordinary advisories, or MOM values; its switch handler only updates shared switch state. Public process tests separately cover discovery and switch storage, not a provider callback. No duplicate chart or normative-omission claim is warranted. |
| **2025 Auto Provide switch boundary review** | The existing [Auto Provide guide](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md) already distinguishes a disabled switch at discovery from a switch-off while provider work is queued. The embedded disabled-baseline case confirms discovery with no provider callback in its HLA_EVOKED fixture; the two-provider case disables the switch in the first callback, suppresses the queued second solicitation, preserves the first response, and solicits again on a later discovery after re-enable. The configured-process test changes the shared switch value but does not arrange object discovery or assert provider solicitation. Strengthened the test links to the exact setup and assertions; no additional diagram. Automatic configured-process solicitation remains unverified, not a normative omission. |
| **2025 process Federation MOM FOM updates** | The 2025 MIM distinguishes the federation-level Conditional list and `HLAcurrentFDD` from the joined-member Static list. The inspected process Join has no federation-list enqueue, while explicit requests project live state. An embedded Create-designator case verifies the supplied list only through an explicit request; neither it nor the Current-FDD-only Join observer asserts the list’s automatic Conditional update. No focused public process list observer test was found. Existing diagrams suffice; keep the process gap source-only, not a conformance or parity claim. |
| **2025 Federation FOM state across Join and Restore** | The [Federation MOM guide](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#catalog-and-federation-mom-values-across-join-failures-and-restore) now compares successful Join, pre-commit rejection, post-commit setup failure, and both Restore sources. A retained private snapshot restores its save-time definition; the durable state image has no FOM catalog and leaves the live definition selected. Both MOM values project whichever definition is selected, but no focused test covers their reflection across post-commit failure or Restore. The pinned MIM names Create/Join triggers; it does not settle Restore semantics. Existing Join and Restore flows suffice; no duplicate chart. |
| **2025 process Federation MOM membership list** | A 2026-10-16 re-audit confirms the [existing Join/Resign trace](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#2025-process-profile-membership-list-updates-at-join-and-resign) already separates the MIM's federation-list Conditional trigger from member-object discovery/removal and the explicit current-value request. Source projects the live membership map and shows no process Federation-list update enqueue. The only focused list-reflection test is embedded; the process resignation test covers member-object removal/rejoin ordering, not `HLAfederatesInFederation`. No diagram duplication or normative/parity claim is warranted. |
| **2025 `HLAsetSwitches` scope and decoding** | The Federation MOM guide contrasts federation-wide `HLAautoProvide` with joined-member switches and `HLAautomaticResignAction`; the flow distinguishes special RTI consumption, staged member updates, extensions, the Service Reporting interlock, and current MOM projection. Process tests cover selected values/errors; embedded conditional-reflection evidence remains separately qualified. The chart rendered with Mermaid CLI 12.0.0 and a local screenshot review found both class paths and decision branches readable without clipping. GitHub rendering remains unverified. No tests or builds. |
| **2025 MOM exception reporting** | Audited malformed and precondition-rejected `HLAsetSwitches` reporting across the pinned MIM, embedded catch, configured-process server path, recipient planning, and focused tests. The existing [exception guide](HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md#route-b-a-mom-interaction-is-rejected) already separates `HLAreportMOMexception` from `HLAreportException`, `HLAreportServiceInvocation`, and the caller's public exception; it maps the `HLAfederate` dimension/parameter projection and the process-versus-embedded catch boundary. A later guide-wide anchor audit repaired stale source/test line references for the exception hook, callback/time gate, embedded and process routing/rechecks, separate service-report requests, and focused tests. The guide's behavioral distinctions remain accurate; no diagram or prose change was warranted. No tests rerun. |
| **2025 FOM module-data request/report** | The Federation MOM guide already maps the duplicate federation/member `HLArequestFOMmoduleData` paths, their separate module-index lists, request parameters, paired reports, and embedded evidence. Added the source-only configured-process boundary: the public request uses the generic server interaction path, which requires publication, while both 2025 MIM request classes are Subscribe; no process FOM-report planner or focused process case was found. Keep the actual process outcome unverified; do not infer parity, a precise public exception, or conformance from source absence. Twenty-nine local links in the section resolve. No build or tests. |
| **2025 FOM variant-record composition** | The guide now distinguishes composition, configured-process commitment, and application-side encoding. The configured endpoint delegates explicit FOM preparation to the configured coordinator before registry commit; public process evidence confirms Restaurant Create acceptance and, in a separate explicit-MIM case, committed FOM/MIM module provenance and logical-time selection. Neither process test inspects `DrinkGarnish` alternatives. The bridge makes the manual value-level mapping concrete: Restaurant FOM names `Margarita`/`DryMartini` and values 1/2 must be associated by application code with typed discriminant and payload `DataElement`s. The catalog projection stores names/type references, while `addVariant` takes runtime elements. No catalog-to-octets integration is shown or tested; the example is illustrative, not a mandated application architecture or normative claim. |
| **2025 additional-module Join** | The FOM guide shows distinct embedded and configured-process post-commit setup paths after the shared replacement-definition/member commit. Its object path separates catalog lookup from later descendant registration, receiver discovery, discovery-time initial-value planning, and future updates; its interaction path shows retained ancestor matching and closest-class parameter projection. A focused embedded test successfully adds an attribute to an existing class after an object is registered: the joining member discovers that object, and ownership changes only after acquisition notification. It does not test current-value reflection. The configured-process additional-FOM test uses an object-only extension and checks object-class lookup, so it gives no add-attribute or profile-parity evidence. Both rollback paths resign the new member; no focused test observes the catalog after post-commit failure with added modules. Existing ownership and subscription charts cover the adjacent transitions; no duplicate diagram was needed. |
| **2025 transferred delete privilege, known receivers, and removal identity** | The existing ownership sequence now shows that B's accepted delete first reserves removals for other known members A and C, then removes B from the known-object map. A and C remain subject to separate receiver commit gates; the embedded test directly observes only C's callback carrying original registrant A and B's tag, not A's callback or B's known-state query. Configured-process source uses the registrant snapshot, while process tests register and delete through the same federate. The revised existing chart rendered and passed local screenshot review; no new chart or process-parity claim. |
| **2025 object discovery and first reflection** | The object-information and subscription-state guides distinguish discovery from stored-value reflection. In the process path, ordinary initial values are planned for newly accepted discoveries, not only MOM objects ([planner](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L90)). The public process case observes one discovery and a `Name`-only reflection with the stored bytes under both callback-model sections ([case](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L652), [assertions](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L952)). This does not establish a universal callback-order rule or the value effect of adding an attribute to an already-known object. Exact §5.8 wording remains unavailable here. The [existing profile chart](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#late-subscription-can-mean-two-different-receiver-states) already captures these boundaries. |
| **2025 configured-process object-update failure mapping** | Corrected the maps after tracing the full status path: a session/federate mismatch becomes a generic rejected transport error, but a valid session receives structured registry statuses that the public adapter translates to typed object/attribute exceptions. The timestamped overload uses the same planner-status translation and adds local finite-time validation. The focused process test covers only successful untimed calls, so negative public outcomes remain source-derived. One detailed diagram now lives in the Process Endpoint guide; the Object/Interaction guide links its object-semantic summary to it. |
| **2025 configured-process interaction-send failure mapping** | Corrected the ordinary Send Interaction map: a session mismatch or non-applied registry plan returns generic rejection and becomes `RTIinternalError`; the service does not preserve remote class-publication or parameter-status distinctions. Added a separate Send Directed Interaction map because its registry plan also checks the target object. Its local invalid object wrapper maps to `ObjectInstanceNotKnown`, while remote non-applied planner statuses collapse to generic rejection. For timestamped sends, a focused process test observes ordinary below-grant `InvalidLogicalTime`; the ordinary adapter maps the process status back to that exception. The directed process handler has no visible sender-floor check, and its adapter maps non-OK client results to `RTIinternalError`; no focused directed below-floor test was found. These are source/test observations only, with no normative conformance or exception-precedence claim. |
| **Open process-timing evidence** | A bounded source trace finds process attribute and interaction RECEIVE events passing through the process callback dispatcher without the embedded `FederateTimeState::receiveOrderDeliveryAllowed`/deferred-queue path. Process time-management code uses constrained-recipient state for TSO queues; that does not establish a RECEIVE temporal gate. Process Join does not initialize the local embedded time state, and the public asynchronous-delivery adapters have no process-client branch; source falls through to the embedded service-state path. No focused process endpoint test calls those services or combines a constrained receiver, asynchronous-delivery off/on, and a pending advance. The time guide records this source-only boundary; the public process outcome and normative HLA meaning remain unresolved. Do not infer parity or add a diagram without new evidence. |
| **2025 process timestamped attribute callback groups** | The process service labels projected recipient passels `more` and `final`; only the final group crosses the message-level TSO ACK boundary. The process map now diagrams callback exception outcomes: a throwing `more` task is consumed but does not cancel later queued events, so a later successful or retraction-suppressed `final` can ACK; a throwing `final` skips completion and leaves the recipient in transit. The focused synthetic bridge case verifies normal grouped completion under both callback models and HLA_EVOKED retraction before final submission, but no grouped callback-throw case was found. The [Process Endpoint guide](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#grouped-timestamped-attribute-callbacks-share-one-completion-boundary) labels these exception branches source-reviewed, not test-demonstrated or normative. |
| **2025 process TSO acknowledgment transport deferral** | The client queues a message-level TSO ACK only while a transport request is awaiting its correlated response, then flushes at request completion or another eligible event-pump point. This depth does not cover the full public C++ call: an immediate callback dispatched after the response can send its ACK before that public method returns. The public HLA_EVOKED fan-out test expects ACK before the receiver's later TAR(7), but no focused test forces the active-request queue path or establishes general API-return ordering. The [Process Endpoint sequence](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#a-completion-ack-waits-only-for-an-active-process-request) now distinguishes these boundaries; its revised Mermaid sequence was locally rendered and screenshot-reviewed. The Time Management guide links both the final-passel and client-transport boundaries from its TSO grant-frontier explanation. |
| **Normative follow-up** | Several precise claims still need canonical 1516.1-2025 prose unavailable in this checkout: the FQR argument/delivery boundary (§8.12), regional-subscription rule, full voluntary `ResignAction` applicability/effect ordering (§§4.12.3–4.12.4), automatic-resign semantics, lifecycle cutoff wording (§4.4), and Restore effects on the Current FOM/Federation MOM projection. IEEE’s [official 1516.1-2025 listing](https://standards.ieee.org/ieee/1516.1/6688/) offers purchase or subscription access; source and tests cannot settle these normative questions. |
| **2025 callback-enable exception boundary** | The callback guide now connects `Enable Callbacks` state change with exception propagation: immediate draining can throw after the enabled flag is set, consuming the failed task while leaving later work queued; Evoked mode defers invocation to Evoke. This is source-only; existing dispatcher tests cover normal enable/disable behavior, not throwing callbacks. The chart rendered with Mermaid CLI 12.0.0; local screenshot review found its exception branches readable without clipping or overlap. |
| **2025 process synchronization-point callback exception** | The synchronization guide now diagrams the configured-process push path: the registry erases the completed barrier before event delivery, and the completing client's response drain submits the event before the public success-report step. A throwing `HLA_IMMEDIATE` callback can unwind after commit; known process protocol/client/bridge errors map to `RTIinternalError`, RTI `Exception` objects reach the outer exception-report path, and no general conversion/report path is shown for arbitrary C++ exceptions. `HLA_EVOKED` defers application invocation to a later Evoke. Focused process tests use a non-throwing recorder and do not inject callback failure or assert post-error barrier/report state. Mermaid 12.1 syntax passed; Mermaid CLI 12.0.0 rendering and local screenshot review found the exception branches readable without clipping or overlap. No normative callback-order or rollback claim. |
| **2025 process Federation Save initiation exception** | For an untimed configured-process request with no time-constrained members, source sets an active save and marks participants instructed before pushing initiation callbacks and returning the correlated response. If the caller's `HLA_IMMEDIATE` initiation callback throws during the post-response drain, its public request can throw after admission; the wrapper attempts `HLAreportException`, skips the later success report, and shows no rollback or failed-invocation report. `HLA_EVOKED` queues the task. This is source-only; the focused tests use non-throwing callbacks and do not assert the exception outcome. |
| **2025 process Federation Save completion boundary** | The last participant's process `federateSaveComplete` can queue `federationSaved` only after state-image commit, or `federationNotSaved(RTI_UNABLE_TO_SAVE)` after an encode/store failure. For pushed events, `HLA_IMMEDIATE` drains the terminal callback before the wrapper's successful Complete-report append; a callback HLA `Exception` can escape after the server-side outcome is resolved and skip that append. `HLA_EVOKED` queues the callback so the service can return first. The one-member process test proves HLA_EVOKED callback deferral in polling mode; a pushed multi-member HLA_EVOKED case later observes the save callback but does not assert zero immediately after Complete. Commit-failure and exception/report-order branches remain source/internal-test evidence, not process-endpoint tests. Two focused sequences now separate coordinator outcome from application callback/report observation; they do not strengthen the untested process failure or exception branches. |
| **Diagram validation** | Current 2025-only inventory: 411 charts across 31 guides (234 flowcharts, 153 sequences, 24 state diagrams; 403 backtick-fenced and 8 tilde-fenced). The 409-chart whole-topic Mermaid 12.2.0 pass plus focused Mermaid CLI 12.0.0 renders of the two newest sequences cover all current diagram bodies; no single whole-topic pass after those edits covers 411. The prior aggregate local-link audit covers 5,511 occurrences, 5,415 file targets, and 5,056 anchors; four destinations in the voluntary-Resign review and nine in the forced-ConnectionLost follow-up were checked separately. The curated screenshot queue is 144 reviewed / 0 pending, not a whole-inventory visual audit. GitHub rendering and layout remain unverified. See the [2025 documentation roadmap](HLA-2025-DOCUMENTATION-ROADMAP.md#current-baseline). |
| **2025 configured-process session recovery** | The [Process Endpoint guide](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#4-temporary-process-session-recovery) now diagrams resumption of the same live private session, retained request/response and pushed-event history, and callback delivery after recovery. Existing private recovery CTests passed; the interleaved replay case passed 28 assertions and ten repeated rounds (30 executions). This is not HLA Save/Restore, host-restart durability, public-provider behavior, or normative HLA evidence. |
| **2025 Save completion link audit** | The new configured-process completion subsection adds 16 local destinations checked separately: 10 source-line anchors and six focused-test anchors. These are not folded into the aggregate inline-link scan. |
| **2025 inline-link audit** | A focused scan of the 31 `HLA-2025-*` guides checked 3,288 local inline destinations: no missing files, 2,629 valid source line anchors, and 264 resolved Markdown section anchors. It repaired two stale section links. Later additions contributed 59 local destinations, checked separately; all 55 added source-line anchors, the schema path, and three additional guide-heading fragments resolve. Six source anchors in the configured-process interaction subsection, four source anchors plus one guide-section link in the variant-record process review, two source anchors plus one guide-section link in the runtime codec boundary review, 11 source anchors plus two guide-heading links in the payload-layer review, eight source anchors in the Send Interaction failure review, and nine source anchors plus one guide-heading link in the Update Attribute Values failure review were checked individually and are not folded into the aggregate. The latest RAVU overload review adds ten separately checked local links (nine source-line anchors and one guide-heading anchor), also not folded into the aggregate. The Federation Save initiation exception subsection adds 17 separately checked destinations (16 source-line anchors and one guide-heading link), not folded into the aggregate. The new synchronization callback-exception subsection adds 15 separately checked source/test line anchors, not folded into the aggregate. Reference-style links, external URLs, full Markdown parsing, and rendered GitHub presentation are outside these checks; section anchors follow [GitHub's documented rules](https://docs.github.com/en/get-started/writing-on-github/getting-started-with-and-formatting-syntax#section-links). |
| **2025 FOM/runtime bridge link check** | The new bridge subsection adds four local destinations checked individually: three focused-test line anchors and one guide-heading anchor. These are not folded into the historical aggregate. |
| **2025 order-conversion link check** | The revised section adds one local guide link to the 2025 time-management guide; the destination exists. This addition is checked separately, not folded into the historical aggregate. |
| **2025 authorization credential lifetime** | Bounded rereview (2026-10-17) confirms the existing flow covers the typed credential envelope, Connect authorization and local commit, the connection-scope snapshot used by Create/Destroy/Join, joined Disconnect rejection, and snapshot/authorizer cleanup after successful Disconnect. Repaired stale source anchors for the connected-operation helpers, Connect commit, and Disconnect path. The authorization test source uses Disconnect for cleanup but does not assert same-ambassador reauthorization; keep that detail source-derived and do not infer HLA-mandated credential retention. Its selected chart has local render/screenshot review. No second chart is warranted. |
| **2025 Federation MOM forced connection loss** | The MIM marks `HLAfederatesInFederation` Conditional on Join/Resign. Current embedded source also queues the federation-list update after forced connection loss; the inspected configured-process loss dispatcher projects cleanup effects but has no corresponding list-update enqueue. The only focused test naming this attribute covers ordinary Join/Resign, not loss; no process membership-list reflection test was found. The source-bounded flow and federation-object lifetime state chart were rendered and screenshot-reviewed locally with Mermaid CLI 12.0.0; both remain readable without clipping. This is not a normative expansion or process-parity claim. |
| **2025 object existence, receiver knowledge, and DDM** | Added a source-bounded synthesis connecting object existence, per-receiver known-class state, current regional discovery/value eligibility, and removal planning. The three-dimensional regional test and embedded delete test cover separate pieces; no reviewed focused case combines a disjoint-region transition followed by removal. The combined relation remains source-only, with no normative or configured-process parity claim. Its selected chart has local render/screenshot review. |
| **2025 directed-interaction target/recipient review** | No new chart was warranted. The guide already separates producer, target object, and receiver identities; distinguishes registered-class FOM/publication checks from per-receiver known-class selector lookup; and keeps universal/by-ownership selection separate from DDM regions. Embedded ancestry/selector/known-target tests and an exact-class process multi-recipient test support different bounded slices; process ancestor selection and normative inheritance remain unclaimed. |
| **2025 Save/Restore admission and completion review** | No new chart was warranted. The existing guide distinguishes source-image and live-membership validation before request success/begun callbacks, each participant's restore report, the all-participant barrier, subsequent RTI-state rehydration, and the federation-wide outcome. It separates the retained process-local snapshot from the bounded durable-image path. The exact Join-during-active-Save/Restore rule remains open: inspected 2025 API annotations/source and the representative interlock test do not establish it. No build or test rerun. |
| **2025 restored time state versus callback queues** | Added a source/test-bounded flow distinguishing restored time-coordinator state and selected TSO/retraction ledgers from already-admitted RECEIVE closures and configured-process pushed frames. Existing tests cover restored logical time/lookahead, a pending TAR rebuilt against a fresh live route under both callback models, and one same-live-process pushed-frame case. No test establishes general deferred RECEIVE closure replay or OS-process restart. Its selected chart has local render/screenshot review. |
| **2025 DataElement to public value path** | Added a reciprocal link from the Data Element guide to the object guide's FOM-member-type → compatible DataElement → handle-keyed value map → service/callback → receiver-decode flow. The Data Element guide remains focused on value octets and codec rules. The existing views are complementary; no combined chart or all-types/profile end-to-end claim was warranted. The new heading link resolves; no build or test rerun. |
| **2025 order/transport evidence review** | Added exact links to the per-recipient TSO test's shared metadata, received-order, and callback-before-grant assertions. The same accepted sent-TSO is observed as RECEIVE by one unconstrained receiver and TIMESTAMP by one constrained receiver in a single embedded HLA_EVOKED schedule; full §8.1.4 Table 3 interpretation remains open. The interaction query note now distinguishes queried federate from callback requester: embedded source applies a target override only when the target publishes, while the process test queries its own handle only. Cross-target and post-unpublish outcomes remain source-only. |
| **2025 DataElement byte-order audit** | Refined the guide to enumerate the pinned BE/LE type set (signed/unsigned 16/32/64-bit integers, float32/64, and `HLAoctetPair`) and link literal-vector/decode tests for every listed family. No 8-bit integer BE/LE pair exists; one-octet types need no order. Rechecked fixed BE Boolean, UTF-16BE/BE-count strings, BE variable-array count, factory-owned logical-time bytes, and the separate BE process frame. No diagram changed or test rerun. |
| **2025 class hierarchy and publication gates** | Cross-reviewed the Object and Interaction, Subscription State, and DDM guides. Their existing maps already distinguish subscription matching, discovery class selection, inherited parameter projection, and regional dimension availability. Added class-keyed object-registration and exact-sent-class interaction-publication gates to the existing cross-path map. The cited registry cases use same-class publication and operation; parent-publication/child-operation behavior remains untested and source-bounded. The revised chart was rendered and locally screenshot-reviewed. No tests, builds, or CI. |
| **2025 timestamped-delivery cross-review** | Compared Time Management, Object and Interaction, DDM/Regions, and Callback and Service Ordering. Their current diagrams consistently separate per-recipient TSO eligibility, embedded RECEIVE/TIMESTAMP attribute passels, profile-specific projection timing, regional eligibility, callback admission, and grant observation. No contradiction or missing diagram was found; no duplicate chart was added. The exact `reflectAttributeValues` interpretation remains open pending an authorized canonical copy. |
| **2025 ownership cross-guide and visual review** | Cross-read Attribute Ownership with Ownership Persistence and Resignation. Existing diagrams already distinguish request acceptance, transfer, callback-entry ownership transitions, queued work, and selected restore/resignation paths. Re-rendered the negotiated-divestiture cancellation sequence with Mermaid CLI 12.0.0; local screenshot review found it readable without clipping, clearing its stale validation note. The curated visual-QA queue is now 81 reviewed, 0 pending. This is not whole-tree or GitHub-renderer sign-off. |
| **2025 RAVU entitlement cross-review** | Existing guides and source distinguish solicitation, provider response, ordinary receiver eligibility, and delivery. The §6.21 direct-entitlement question remains open because the local canonical clause text is unavailable; focused RAVU cases use subscribed requesters, while the separate DSE case is not RAVU. Existing diagrams suffice; selected visual QA remains 81 reviewed. |
| **2025 synchronization/save boundary review** | Existing guides distinguish the synchronization barrier outcome from the save/restore participant protocol; the partial-progress barrier image path remains source-derived and is not an end-to-end tested multi-member restore. |
| **2025 timed-save/logical-time cross-review** | The save-time TSO cutoff and post-restore delivery before the flush grant agree with the Time Management guide’s separate restored time-state, TSO ledger, and live callback-queue boundaries. No universal persistence or callback-order claim. |
| **2025 voluntary-resign/connection-loss cross-review** | Voluntary Resign preflight/action, forced-loss directive and cleanup, and synchronization participant-set removal remain separate; forced-loss synchronization and process Save/Restore notification evidence gaps remain explicit. |
| **2025 object-deletion/ResignAction cross-review** | Existing maps distinguish delete request, removal callback, local forget, per-instance delete privilege, ResignAction cleanup, and connection-loss forced cleanup. No synthesis chart or broader callback-order claim was warranted. |
| **2025 interaction-subscription/DDM cross-review** | The guides distinguish global relevance, receiver matching/class promotion, region overlap, parameter projection, and timestamp/callback gates. The inherited-class and process test limits remain explicit; no new chart was needed. |
| **2025 FOM admission/composition and name-resolution cross-review** | Existing maps distinguish module/schema validation, completed-model composition, Create/Join commit, joined-execution typed lookup, new application declarations, existing object discovery, and MOM reflection. Embedded handle stability and process observation limits remain scoped; no synthesis chart was needed. |
| **2025 order/transportation/time-management cross-review** | Effective sent order, per-recipient received order, transportation metadata, DDM selection, time eligibility, and callback observation remain distinct. Existing recipient-split and passel diagrams agree; normative mapping and physical QoS remain open. |
| **2025 relevance/scope advisory cross-review** | Declaration relevance, owner-side per-instance attribute relevance, and receiver-local Attribute Scope callbacks have separate state, audiences, and triggers. The special self-unsubscribe scope case stays isolated; process class-advisory and race evidence remains bounded. |
| **Latest 2025 review** | Added two source-bounded Object and Interaction sequences that separate a non-final voluntary Resign from forced ConnectionLost while timestamped deletion is pending. Embedded tests cover both at-cutoff time 6 and before-cutoff time 5, automatic `DELETE_OBJECTS` cleanup, and two recipients whose requests straddle loss. Each receiver observes its own removal before its grant. The tests do not establish a later-than-cutoff outcome, post-Resign Retract, configured-process behavior, or cross-recipient order. Both diagrams rendered with Mermaid CLI 12.0.0 and passed selected screenshot review; nine new local destinations resolve. Inventory is 411 diagrams / 31 guides; selected visual review is 144/0. No source, tests, builds, CI, Requirements Lab, or 2010 changes. |
| **Next** | Review the 2025 timestamped object-deletion path for a message strictly later than a lost regulator's last granted time. Trace forced cleanup and automatic Resign directives to determine what Umbra's current source retains, clears, or deletes; compare object-deletion evidence with other TSO families. Keep source-observed behavior distinct from the cutoff obligation and label unsupported outcomes. 2025 only; no builds or test reruns. |
| **2025 renderer retry** | Historical checkpoint: a 2026-10-17 Mermaid CLI smoke test with installed Chrome failed before rendering (process code 2147483651); Edge reported an existing profile and Windows credential errors. The refreshed Chromium window displayed the Windows lock screen, so UI interaction stopped there. Mermaid 12.1 parsed 45 selected charts across four guides, but the four then-pending charts had no screenshot review at that checkpoint; the following renderer-follow-up entry records their later chart-level review. |
| **2025 renderer follow-up** | Historical record: on 2026-10-17, Mermaid CLI 12.0.0 full-Markdown renders for the 2025 TSO retraction, RAVU, and callback/service-ordering guides produced no PNGs, and the authorization flow was not rendered. Later chart-level records in the [completed visual-review batch](HLA-2025-DOCUMENTATION-ROADMAP.md#completed-visual-review-batch-2026-10-17) document local rendering and screenshot inspection for the RAVU, synchronization-exception, Delete/Retract, and authorization credential-lifetime charts. Their current Mermaid blocks are unchanged from `HEAD`; they are not pending local visual review. GitHub rendering remains unverified. |
| **Pending validation** | Current inventory is 411 charts (234 flowcharts, 153 sequences, 24 state diagrams) across 31 guides. The 409-chart whole-topic render plus focused renders of the newest two sequences cover all 411 diagram bodies; whole-inventory visual review and a new full-tree render remain outstanding. The prior aggregate local-link audit covers 5,511 occurrences / 5,415 file targets / 5,056 anchors; 13 destinations across the two timestamped-deletion departure reviews were checked separately. The curated screenshot queue is 144 reviewed / 0 pending and is not a whole-inventory visual audit. GitHub page rendering and layout remain unverified. See the [2025 documentation roadmap](HLA-2025-DOCUMENTATION-ROADMAP.md#current-baseline). |

The configured-process regional class-RAVU per-group failure subsection adds
nine separately checked local source/test links; these are not folded into the
aggregate link audit.

The 2025 class-level RAVU object-awareness question was rechecked. The existing
guide already distinguishes the known-object guard on instance requests from
class expansion, and states that the process test discovers both targets before
the request; no focused test targets an undiscovered object. The API exception
list is not the clause text, so it does not settle normative applicability. The
Auto Provide and subscription-state guides likewise already distinguish
discovery solicitation from explicit requests and initial reflection while
preserving the unverified ordinary process edges. No duplicate chart was
warranted.

The configured-process Federation MOM review found the existing 2025 sequence
clear: it separates federation-level Conditional reflection from new-member
HLAfederate discovery, shows explicit requests projecting the live Current FDD,
and distinguishes process class-lookup evidence from the embedded reflection
test. The process reflection gap remains source/test-only; no normative
conclusion or duplicate chart was added.

The variant-record model-to-runtime boundary is also already explicit: the FOM
guide describes merged discriminant/alternative resolution and its composer
tests, while the DataElement guide shows a runtime map configured through
`addVariant` and states that it does not consume the composed catalog. The
process Restaurant-FOM Create assertion is labeled incidental baseline
coverage, not variant-merge, discriminant-validation, or codec evidence. No
additional chart or broader process claim was warranted.

The 2025 additional-module Join failure boundary is already explicit in the
FOM admission guide. Its flow and evidence note separate the registry's atomic
catalog/member commit from later report/MOM/session/event setup, and say that
rollback resigns the member without an inspected prior-catalog restore. The
guide also states that existing failure tests are ordinary Join or pre-commit
rejection and do not assert catalog state after a post-commit added-module
failure. This remains source-only; no duplicate chart was warranted.

The latest 2025 RAVU review separates three kinds of Save/Restore evidence: an
embedded public fresh-registry test observes a regional-request callback after
`federationRestored`; configured-process source uses the shared registry image
for request metadata; and the process callback event remains a separate live
session event or pushed frame with no RAVU-specific Save/Restore test. This
does not establish configured-process replay or callback timing. In the
process profile, pull polling checks pending Restore notifications before the
RAVU queue, while pushed frames and callback tasks retain client/dispatcher
arrival order; neither is an HLA callback-order guarantee. The preceding
regional-validation review also found that mixed valid/invalid suppression is
source-derived, not tested. See the [RAVU guide](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md#configured-process-path-region-check-before-event-routing).

A bounded 2025 mixed-validity review confirmed that the existing RAVU guide
already explains the configured-process boundary: the registry validates all
normalized region/attribute pairs before building provider recipients, and a
typed rejection returns before the service enters its provider loop. Focused
process tests exercise invalid selectors in separate calls; none combines an
invalid pair with eligible work or asserts that such work is suppressed. The
guide's source-only qualification is accurate, so no duplicate prose or chart
was added. This is not a normative atomicity, exception-precedence, or
embedded/process parity claim.

A further review of the configured-process regional path confirmed that the
guide's planner chart already shows owner partitioning and per-attribute
request/source-region filtering, but it did not show the distinct error
checkpoints after groups are planned. Pull appends each eligible group event
inside the group loop; Push stages frames and sends them only after group
preparation finishes. A later registration failure or sequential send failure
can therefore occur after different amounts of source-visible progress. Added
a separate failure-phase chart and explicitly limited it to source behavior:
no focused regional process test covers multiple providers, a disjoint
provider, a later group failure, or a Push send failure. Nine local source/test
anchors in the new section were checked; the chart passed an individual
Mermaid 12.1.0 core syntax parse but remains unrendered. No normative atomicity,
callback delivery/order, or cross-profile rule is inferred.

The latest 2025 Attribute Scope Advisory review distinguishes switch-off at
three event positions: embedded queued closures recheck at callback entry;
process pull events recheck at server dequeue; pushed frames and client
callback tasks have no later switch check or switch-triggered purge visible in
the source. No focused test toggles the switch across already-admitted scope
work. These are implementation observations, not normative cancellation or
delivery rules. See the [Attribute Scope Advisory guide](HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md#switching-off-is-not-a-common-queue-flush).

The latest 2025 process ordinary-interaction review traced event projection
through push/pull routing. The process service consumes predefined federation
`HLAsetSwitches` before ordinary recipient planning; separately, pushed
recipient frames are sent sequentially, and a later send failure returns an
error without visible rollback of earlier frames. The client/public adapter
maps that response to `RTIinternalError`, but no focused test injects a
mid-fan-out failure or proves which callbacks occur. The existing guide now
labels these as source-only boundaries and links the control path to the MOM
guide; no duplicate chart was needed. See the [ordinary-interaction process
section](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#process-ordinary-interactions-snapshot-the-promoted-callback-view).

The 2025 process late-subscription review now has a focused public test source
for an existing ordinary object that becomes newly discoverable after the
receiver subscribes: it asserts discovery and a reflection with only the
selected stored value under both callback-model sections. The current service
source plans those ordinary values in the discovery enqueue path; the
implementation change is uncommitted and this documentation pass did not run
the test. The separate MOM snapshot path remains distinct. Adding an attribute
to an already-known object still has only provider-advisory evidence, and the
licensed §5.8 rule remains unreviewed. See the [subscription-state guide's
process comparison](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#process-late-subscription-path-same-selectors-an-unresolved-initial-value-edge).

The 2025 configured-process Unconditional Divestiture review confirmed that
the ownership guide already distinguishes registry commit, later acquisition
work, ownership-assumption offers, and the recipient-specific push/pull gates.
Push can begin and commit an acquisition notification before sending its frame;
pull begins it at receive polling. The selected process tests cover a later
acquisition after observing unowned state and assumption offers, not a regular
request pending at divestiture or an injected post-commit enqueue/send failure.
The failure trace distinguishes push preparation gates from the later batch
send loop, pull's incremental queue insertion from later polling gates, and the
subsequent assumption-offer projection. A helper failure maps to
`RTIinternalError`; inspected source shows no rollback, but no untested runtime
residual effect is asserted. The embedded 2025 case does cover a regular
request already pending: it observes the later notification, original request
tag, and resulting ownership. The process-path failure and pending-request
branches remain untested. The guide's failure-phase chart and its overview
chart were syntax-parsed individually, not rendered. No fairness,
callback-timing, normative rollback, or profile-parity claim was added. See the
[process divestiture section](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#process-endpoint-unconditional-divestiture-and-recipient-work-are-separate).

The 2025 configured-process Release Denied/cancellation audit separated
cancellation acceptance, denial planning, and callback-delivery gates. A
pending cancellation reservation does not itself remove the request; an
Unavailable delivery can consume a later reservation, while denial planning
can consume one that already exists. Push checks owner-release eligibility
before send and claims again before the public callback; pull checks at receive
poll and examines Unavailable before acquisition/cancellation events. An
embedded test verifies one arranged after-entry schedule, but the configured-
process tests do not combine after-entry Release Denied with cancellation.
The ownership guide now explains both the source-derived process branches and
the embedded/process boundary; its updated Mermaid chart passed core syntax
parsing only. No rendering, tests, builds, CI, or Requirements Lab work.

The 2025 configured-process cancellation/Unconditional Divestiture review
confirmed that cancellation planning and notification begin are competing
registry transitions. If cancellation planning wins, it reserves the attribute
without immediately removing it from the pending request; notification begin
then suppresses transfer. Push begins cancellation in the service handler
before sending its confirmation frame, while pull keeps the reservation until
the recipient polls. If notification begin wins, it assigns ownership before
callback projection and a later cancellation is rejected. Existing process
cancellation and divestiture tests exercise the paths separately; the embedded
pending-request divestiture case does not add cancellation. A separate embedded
assumption-search case does cancel before divestiture, but tests later
assumption-callback eligibility rather than the configured-process queued-
notification race. The ownership guide now makes the two gates and push/pull
timing explicit. The configured-process race remains source-derived, not a
normative ordering or profile-parity claim. Mermaid core syntax parsing passed
with a no-op sanitizer; the chart remains unrendered.

Open normative or test-evidence questions stay attached to their topic rows; a
diagram does not resolve them. Do not infer embedded/process parity without
evidence from both relevant 2025 profiles.

## Remaining roadmap and finish condition — 2026-11-06

The 2025 portfolio has 32 topic guides and 460 diagrams. All 460 diagram bodies
have local render coverage; selected screenshot review is 233/460, while the
curated screenshot queue is 223 reviewed / 0 pending. These are coverage
measures, not normative completeness or GitHub
rendering. The remaining work is a small, evidence-gated close-out queue rather
than a promise to diagram every service. The content-level audit of all 32
IEEE 1516.1-2025 topic guides is complete
(32/32, 100% for this audit pass). This is not overall documentation completion
or HLA coverage; the guides retain open evidence, standards-text, and GitHub
rendering work. The latest review is recorded in the
[2025 documentation roadmap](HLA-2025-DOCUMENTATION-ROADMAP.md#2025-configured-process-connectionlost-tso-cutoff-boundary-increment-41--2026-11-06). It separates recipient-side TSO cleanup from producer-side ConnectionLost cutoff delivery and records a configured-process gap: the loss handler does not directly re-evaluate a survivor's already-pending TAR, while no focused process cutoff test establishes that outcome. This remains source/test uncertainty, not a normative failure claim. The open standards-text question is
RAVU entitlement in IEEE 1516.1-2025 §6.21. The latest configured-process
nonregional receive-order review found its request/admission/planning, push and
poll delivery, callback bridge, and focused-test boundaries already documented
across the Process Endpoint, Callback and Service Ordering, and Object and
Interaction guides. The latest review found a focused embedded Join/ownership case for an attribute added to an existing class. It verifies discovery of the pre-existing object and ownership only after acquisition notification; it does not send a value update or test receiver reflection. The new sequence is locally rendered and screenshot-reviewed. Current-value reflection and normative §5.8 remain open.

| Priority | Work remaining | Current state and finish condition |
| --- | --- | --- |
| **1 — Active** | 2025 evidence-gated reader-gap closeout | Continue bounded 2025 reviews, adding a portable explanation only when available source/test evidence answers a real reader question; otherwise record a no-change disposition or preserve the gap. Open profile/test boundaries include process RECEIVE timing outside the focused interaction async-gate tests. The [Callback and Service Ordering](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#configured-process-attributeupdate-receipt-and-disabled-callback-gates) and Time Management guides now distinguish that interaction time gate from the configured-process AttributeUpdate dispatcher route; no focused test combines a time-constrained receiver, asynchronous-delivery changes, a pending advance, and AttributeUpdate. Other open boundaries include regional Request Attribute Value Update pull/push checkpoints, the documented but untested same-object multi-provider and invalid-plus-eligible regional RAVU combinations, source-documented regional-scope recheck checkpoints with post-checkpoint races untested, the already-documented but untested three-role passive-discovery outcome, partial-achievement/late-Join synchronization, configured-process Auto Provide solicitation, the source-documented transferred-owner removal identity path with no distinct-owner process test, and the embedded failed-multi-group class-request Save/Restore chain. Its source-only guide now shows the route-free image-admission gate and restore-time owner/member checks; no focused test covers the combined failure, save, and restore. The embedded disabled-switch and queued-switch Auto Provide cases are covered; configured-process switch storage does not establish provider solicitation. The completed regional-request order comparison is source/test-only: embedded and process checks differ, the API exception list does not define precedence, and combined-fault plus process save/restore outcomes remain untested. Other recent source/test findings are retained in topic guides and the progress log; do not infer normative HLA behavior or cross-profile parity from them. Keep the paused visual-review queue visible. |
| **2 — Evidence-gated** | Open profile/test boundaries | Keep untested process RECEIVE paths—especially nonregional receive-order push and other temporal-gate interleavings—the separate active-establisher/passive-receiver object-attribute scenario, post-checkpoint stale-scope races, the partial-achievement-before-expanding-Join interleaving, and the RAVU mixed-work combinations as explicit source/test gaps. The Cross-Cutting guide summarizes the DDM, Attribute Scope, and RAVU checkpoint differences; the service guides hold the detailed paths. The RAVU guide already shows same-object provider grouping, whole-request regional preflight, per-group Pull/Push failure points, and restore of a surviving pending record; focused evidence does not cover one object's attributes split across providers, an invalid region pair alongside otherwise-eligible work, or injected later-group/send failure. These are implementation-evidence gaps, not missing diagrams or normative atomicity rules. The receiver-role explanation already has a source-bounded chart in the Subscription State guide; its three-role outcome remains untested. The focused regional process attribute-update case covers pull and configured push checkpoints, but does not close broader process RECEIVE coverage. The MOM report guide also records a configured-process joined-federate query/report gap: the request path falls through to generic Send Interaction planning and no focused query/report case was found; keep this source-only profile boundary explicit. Do not transfer the interaction-service test result or present the object source path as normative. The process Join-rollback test covers a different event sequence and does not close the synchronization gap. The FOM guide now diagrams the source-only difference between catalog growth and pre-existing instance owner state, and the known receiver's separate subscription/discovery path. A promoted public C++ TCK case rejects selected invalid modules without admitting the member or extension class, accepts one identical duplicate, rejects both object-attribute and interaction-class order conflicts without admitting the member, rejects a dimension upper-bound conflict (101 against the existing 100) while preserving the original bound, and succeeds on a valid retry. The catalog identifies seven invalid fixtures that validate against the pinned DIF XSD; this is selected evidence, not exhaustive validation. A separate focused embedded case covers the successful add-attribute Join and subsequent ownership notification, but not receiver value reflection. Keep current-value reflection and canonical §5.8 questions open. Revisit only when focused checked-in evidence or authorized standard text changes the conclusion. |
| **3 — Standards-text dependent** | Resolve precise normative questions | The current authorized-text handoff is RAVU entitlement (§6.21). Other unresolved questions include Flush Queue Request delivery (§8.12), regional-subscription rules (§9.1.5), voluntary `ResignAction` applicability/effect ordering (§§4.12.3–4.12.4), automatic-resign action semantics, lifecycle cutoff wording (§4.4), and TLS 1.3 transport scope (§12.13.5). Keep them labeled unverified until authorized canonical text is available; source code and tests cannot substitute for the standard. |
| **4 — Environment dependent** | Render and inspect selected diagrams | The curated 2025 visual-QA queue has 223 locally rendered and screenshot-reviewed charts, with none pending. This is not a whole-inventory visual audit. All 459 diagram bodies have local render coverage; actual GitHub page rendering remains unverified. Add changed or newly important charts to the visual queue and report local rendering separately from GitHub presentation. |

The dated reader-gap summaries now live in the separate [reader-review
archive](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md); their original headings remain here
as legacy anchors. The historical Progress Log and Review Ledger are stable heading indexes with
direct links to date-based archives. The configured-process AttributeUpdate receive route under
time-constrained asynchronous delivery and a pending advance has been cross-reviewed; existing
charts are sufficient and the combined interleaving remains untested. The latest content review
cross-checks inherited `HLAfederate` target decoding, the target-derived endpoint region, and
focused test limits across the joined-federate MOM report families. The following configured-process nonregional receive-order review found the end-to-end path and test boundaries already covered; no duplicate chart was added. The Join-added-attribute follow-up separates value-only subscription from a later update through the normal recipient path. The subsequent RAVU/Auto Provide cross-review found their source-derived gaps and test limits already clearly documented, so no duplicate diagrams were added. The transferred-owner removal review confirms that the embedded test and process source have different evidence strength: the embedded case directly separates registrant, current delete-privilege owner, and deleter, while process source preserves registrant identity but focused process tests use the same registrant and deleter. The class-request Save/Restore review now distinguishes route-free image admission, saved-identity/current-owner validation, and post-restore provider work; the full failed-multi-group chain remains untested. Next, cross-check the 2025 synchronization barrier's partial-achievement/late-Join interleaving against current source and focused tests, keeping source-derived participant-set behavior separate from any normative completion rule.
This documentation goal is ready for handoff when every concrete reader gap is
either explained with bounded evidence or explicitly dispositioned, unresolved
normative/profile questions retain their limits, and each changed diagram's
syntax/render/visual status is accurately recorded. It does not require complete
normative coverage, visual review of every diagram, or a fixed diagram count.
The later Requirements Lab translation remains a separate goal.

## Guide pattern and boundaries

Use the [2025 time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md) as the
first structural example. Each new guide should:

- State its API edition and implementation profile at the top. A 2010 guide
  must be separate from a 2025 guide; never combine state machines or imply
  compatibility unless that equivalence has been demonstrated.
- Use GitHub-renderable Mermaid diagrams, with service calls, callbacks, and
  state-commit points labeled. Prefer several cohesive diagrams over one
  all-encompassing chart.
- Separate standard concepts, current Umbra behavior, test observations, and
  known limits. The standard is normative; code and tests describe only the
  implementation and scenarios exercised.
- Apply a portability test to each primary diagram: could its actors, states,
  events, and observable effects still explain HLA if Umbra were replaced by a
  different RTI? Keep registry layouts, queues, callback dispatchers, and
  transport plumbing in a clearly labeled implementation mapping unless they
  establish a protocol invariant. Never promote a source-only implementation
  detail into a general HLA rule.
- Link implementation claims to the relevant source and focused tests. Treat
  a passing test as evidence for its scenario, not proof of complete support or
  conformance.
- Identify interactions with other areas (time, ownership, regions, save and
  restore) without silently absorbing those state machines into the guide.
- Keep this work in Umbra documentation. Any later translation into or
  expansion of the HLA Requirements Lab is a separate phase and is not part of
  this backlog's current authorization.

## Long-running goal and delivery phases

**Goal:** Turn the durable HLA logic and infrastructure concepts discovered
while implementing Umbra into clear, reader-centered diagrams and guides.
Explain what a federate can do, what state changes, which participants become
eligible, and when effects become observable—not merely how Umbra's C++ happens
to implement them. Use Umbra source and existing focused-test evidence to
ground examples, while keeping portable HLA concepts distinct from implementation
choices and exact normative claims. Lead with the durable HLA mental model;
include Umbra's internal state and architecture only when they explain an HLA
invariant, an observable consequence, or an important implementation limit.
Use the 2025 time-management guide as a quality and structure example, not as
the topic-selection bias: keep building equally clear guides for the other
major HLA behavior areas. Prefer synthesizing relationships that cross service
families when that resolves a real reader gap; improve a focused guide where
that is clearer. There is no diagram-count quota.

Deliver the goal in reviewable phases:

1. **Keep the map current.** Maintain the topic inventory, reader paths, and
   prioritized backlog here. A candidate area starts with a bounded comparison
   of existing guides, relevant implementation/profile boundaries, and focused
   tests; a missing method-name mention alone is not a gap.
2. **Translate implementation discoveries into durable HLA explanations.**
   Prioritize portable concepts and cross-cutting invariants that are hard to
   see in isolated requirements or service code. Use Umbra as a case study,
   not the subject of the explanation: translate each useful C++ discovery back
   into the HLA actors, protocol state, eligibility rules, and observation
   boundary it helps explain. Identify edition and profile boundaries; link
   existing source and focused-test evidence in a separate implementation
   mapping; and label source-only or unverified edges plainly. Reuse verified
   evidence; do not compile, rerun tests, or start CI for documentation work by default.
   Keep the main implementation worker's build capacity free. Request fresh
   runtime verification only when a material claim cannot be responsibly
   documented otherwise. Link adjacent guides instead of absorbing their state
   machines.
3. **Validate documentation proportionately.** Distinguish grammar acceptance
   from actual Markdown preview, visual inspection, and post-publication GitHub
   rendering. Keep each pending check visible and do not upgrade evidence based
   on a parser-only pass. Reuse already-verified focused-test evidence. Record
   setup/environment failures separately from assertions that ran. When the
   permitted preview path is unavailable, leave visual review pending and keep
   substantive HLA documentation moving; do not unlock the desktop or route
   around the restriction. Documentation-only increments do not justify
   competing C++ builds or CI runs.
4. **Close or disposition each backlog item.** An area is complete when its
   guide and diagrams meet the per-guide completion check below, or when the
   survey records why another standalone diagram would only duplicate existing
   coverage. Leave a concrete next handoff for anything still open.

IEEE 1516.1-2010 and IEEE 1516.1-2025 remain separate behavior streams, with
separate guides, source paths, and test evidence. The 2025 embedded and process
profiles are also distinguished where their behavior or evidence differs.
Shared explanations require evidence from every stream they purport to cover.
The eventual idea of adding richer diagrams to the HLA Requirements Lab is a
separate later goal: this work improves Umbra documentation only and makes no
Requirements Lab edits.

## Edition-boundary text audit — 2026-10-09

A bounded scan of the 29 current 2025 guides for `2010` and
`rti1516_2010` markers found only explicit separation notes, legacy-model input
or catalog-only compatibility descriptions, and clearly labeled parallel
implementation references. The DataElement guide's comparison diagram gives
2010 and 2025 separate subgraphs and source paths; it does not combine codec
behavior or tests. In particular, `fomEdition=2010` is described as a
compatibility input to the 2025 ambassador, not as the 2010 RTI stream. The
separate 2010 reference-RTI guide and the new 2010 encoding companion remain
edition-specific; neither is used to fill evidence gaps in a 2025 guide. This is
a documentation-marker review, not evidence of cross-edition equivalence;
future shared explanations still need direct evidence from both streams.

## 2010 scope resurvey — 2026-10-10

A bounded path inventory found eight test files with `2010` in their names;
these include both 2010 runtime smoke programs and external-FOM edition or
composition cases, so a filename alone does not establish 2010 RTI behavior.
The existing [2010 reference-RTI guide](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md)
already covers the supported process-local lifecycle, object and interaction
flows, callback behavior, and the source-only synchronization barrier; it also
marks time-managed execution and other absent services as unsupported. The
separate [2010 encoding guide](HLA-2010-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md)
covers selected scalar and composite byte behavior. The 2010 RTI binding smoke
is a single `HLA_IMMEDIATE` integration scenario; the time and encoding files
are value/marshalling smoke programs, not evidence of a 2010 time-management
state machine.

The separate `external_2010_fom_catch2.cpp` cases exercise FOM validation and
composition or the 2025 API with a 2010 model input. They are useful edition-
boundary evidence, not behavior evidence for the separate 2010 reference RTI.
The synchronization flow has no focused 2010 test in this bounded inventory.
The current evidence does not justify another 2010 service-flow guide: it
would duplicate the bounded reference guide or borrow 2025 behavior. Keep the
sync test gap visible, do not re-enable 2010 CI, and revisit only when new
2010-specific implementation or focused evidence creates a distinct reader
need.

## 2025 callback/service-ordering survey — 2026-10-10

A bounded comparison of the pinned 2025 API, callback guide, dispatcher, and
existing focused tests found no missing reader-facing distinction that needs a
new diagram. The guide already explains the durable model—producing/accepting a
callback task is distinct from invoking it and from the application observing
it—then shows the different HLA_IMMEDIATE and HLA_EVOKED drain boundaries in
separate diagrams. The pinned API exposes those callback models, the Evoke
operations, and callback enable/disable services; its declarations alone are
not a substitute for normative service prose. As implementation evidence, the
dispatcher drains enabled immediate work from submission and only invokes
evoked work when an Evoke operation claims it. Existing unit tests cover
synchronous immediate dispatch/FIFO and one-at-a-time evoked dispatch; these
tests were inspected, not rerun. Keep the known gaps explicit: the public
FederationSynchronized integration does not assert exact service-specific
HLA_IMMEDIATE-before-return timing, and the generic dispatcher lacks a focused
throwing-task recovery case. Record this area as surveyed rather than adding a
duplicative synthesis. The next bounded topic was whether the 2025 exception-
reporting and service-invocation-reporting guides needed one portable account
of service outcome, report production, callback invocation, and observation;
that survey and its disposition are recorded below.

## 2025 service outcome/report survey — 2026-10-10

The exception-reporting and service-invocation-reporting guides already give
service-specific report families, selectors, recipients, profile boundaries,
and detailed source/test limits. The broader cross-cutting guide already had
portable milestones for request validity, state transition, effects, caller
result, and observation, but it did not connect those milestones around the
surprising case where reporting fails after a state commit or changes a public
failure result. A fifth synthesis flowchart now relates those traces and three
2025 case studies connect it to the existing guides; no new normative rule or
2010 claim is made. The combined process two-report schedule and timestamped
queue/report failure are source-derived; the switch-append test body did not
reach its assertions in the last recorded run. No C++ source changed and no
build, test, or CI was run. The later whole-worktree grammar pass recorded
below accepts this chart; it generated no SVG. Rendered-layout review remains
separate and pending.

### Cancellation, invalidation, and already-observed effects — 2026-10-10

The bounded 2025 comparison found a reader-facing distinction worth
synthesizing, but no universal cancellation rule. The cross-cutting guide now
has a decision lens that scopes work per operation/request/recipient, separates
committed/query-visible facts from callback phase, and asks whether pending
work is re-evaluated, withdrawn, invalidated, or preserved—or whether an
already-observed effect has a service-specific follow-up. Six examples link
per-attribute subscription eligibility, accepted object deletion and its
per-recipient removal gate, ordinary versus directed interaction publication
withdrawal, ownership cancellation on resignation, recipient-specific TSO
Retract, and member-route invalidation in lifecycle cleanup.

The chart is explanatory, not normative. Its evidence remains bounded to the
linked 2025 guides, selected embedded tests, and any explicitly source-derived
edges; it does not assert process parity, a complete resignation-action
matrix, or rollback semantics. No C++ build, test rerun, CI, or Requirements
Lab change was made. The later whole-worktree grammar pass recorded below
accepts this block; that pass generated no SVG. Permitted visual review remains
separate and pending in the review inventory.

### Durable image state, live routes, and participant lifetime — 2026-10-10

The bounded 2025 comparison found no need for another state-machine diagram.
The save/restore guide already distinguishes federate-owned application state,
the process-local snapshot, supported durable RTI records, callback
routes/closures, and route-free restart projection, with an explicit limit
against assuming arbitrary crash recovery. The ownership companion separately
models ledger, route, and membership; the lifecycle guide distinguishes the
ambassador connection, federate membership, and federation execution, then
records what remains after resignation. The cross-cutting guide now adds a
short reader map connecting those explanations. A new chart would repeat these
existing models rather than resolve a remaining gap.

Evidence is still limited to the linked 2025 implementation and focused-test
scopes; no 2010 or general RTI parity, arbitrary process restart, or blanket
portability of callback-bearing operations is claimed. No C++ build, test
rerun, CI, or Requirements Lab change was made.

### Participant-set changes across synchronization and save/restore — 2026-10-10

The bounded comparison found a genuine reader gap: the focused guides explain
each protocol, but do not make the participant-set difference visible beside
one another. The seventh 2025 cross-cutting diagram shows that this registry
defaults an empty synchronization set to current membership but expands on
later Join only when the set was omitted; resignation removes the member and
rechecks barrier completion. Restore admission instead checks the saved/current
roster before restore begins, while resignation during an active save or
restore fails the operation for survivors. Connection loss reuses the
forced-resignation cleanup path, but pending-protocol notification outcomes
remain source-derived without a focused loss/protocol cross-product test.

The source audit also found an unresolved Join edge. Both 2025 Join overloads
in the pinned interface list `SaveInProgress` and `RestoreInProgress`, but the
inspected embedded adapter, shared registry transaction, and process Join
handler have no matching operation-state gate. Save/restore participant-status
maps are initialized from the cohort present at operation start, and the Join
path does not update them. The bounded test search found no focused
Join-during-save/restore case. Treat this as an apparent API/source gap, not a
verified runtime outcome or confirmed conformance defect; the exact 2025
§4.11 conditions still need an authorized full-text check.

Evidence remains selected 2025 source and focused tests. Late-Join expansion,
the explicit-empty distinction, and restore-roster rejection are source-derived;
the focused synchronization and active save/restore resignation cases cover
only their named interleavings. Keep process evidence separate, claim no 2010
rule, and make no C++ build, test rerun, CI, or Requirements Lab change.

### 2025 attribute-ownership reader-model review — 2026-10-10

A bounded content review found no missing reader-facing ownership diagram in
the existing [2025 ownership guide](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md).
Its opening teaches per-object-attribute ownership, separates publication and
object knowledge from ownership, and makes accepted acquisition distinct from
the later transfer outcome. The existing flows then distinguish owner denial,
unconditional and negotiated divestiture, assumption offers, query snapshots,
and stale callback invalidation. Those are supported by linked focused
scenarios and are explicitly scoped to the selected 2025 profiles; a new
summary chart would repeat the current model. This was a documentation/source
survey only: no normative interpretation was added, and no C++ build, test
rerun, CI, or Requirements Lab change was made.

A follow-up boundary audit found one useful reader improvement without needing
another diagram: the If-Available and If-Wanted sections described service
acceptance, typed failures, callback outcomes, and ownership queries in separate
places. Added a compact comparison table to the
[ownership guide](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#compare-the-call-per-attribute-outcome-and-observed-ownership).
It marks typed exceptions as synchronous service outcomes, then separately
compares per-attribute callbacks and `isAttributeOwnedByFederate` observations.
The exception names are examples from the pinned API and focused tests, not an
exhaustive matrix. The evidence is selected embedded `HLA_EVOKED` behavior;
process and 2010 parity remain unclaimed. No chart or diagram-count change;
no build, test rerun, CI, or Requirements Lab edit.

### 2025 delete-privilege transfer cross-service review — 2026-10-16

The ownership guide already separated `HLAprivilegeToDeleteObject` from
ordinary attribute authority, while the object-information guide explained the
delete and removal lifecycle. A focused current-worktree 2025 embedded test
source exposed the useful seam between them: a prospective owner publishes and
acquires the special attribute, the current owner divests it if wanted, and
the new owner can delete without becoming the object's registrant. The paired
source scenario also isolates publication eligibility. Added one sequence to
the ownership guide; it preserves the per-attribute versus delete-privilege
distinction and labels the observed callback producer identity and deletion tag
as assertions in this selected embedded `HLA_EVOKED` path. The test sources were
not run in this documentation review.

The process timestamped-delete path carries separate producer metadata, so
the new chart explicitly makes no process-profile parity claim. The follow-up
source trace is recorded in the next review entry: it maps the public process
callback to the stored registrant snapshot, while the focused process case
does not separate registrant and deleter. The new chart parses with Mermaid
12.1.0, but no rendered image or visual review was possible because browser
launch remains denied. At that review, the inventory was 307 diagrams; a broad local-link
check found no missing destinations or invalid line anchors. No build, test,
CI, or Requirements Lab work was performed.

### 2025 DDM/regions reader-model review — 2026-10-10

A bounded learner review found one useful cross-topic seam that the DDM guide
stated in prose but had not diagrammed directly: regional scope, logical-time
eligibility, and application callback dispatch are separate questions. Added
a conceptual flow for one regional update or interaction and one receiver. It
shows that a region match alone does not prove callback delivery, receive-order
delivery does not use the TSO grant frontier, and a timestamp does not make a
disjoint regional path eligible. It leaves DDM evaluation/recheck timing
service-specific and explicitly says the arrows are not a universal internal
algorithm. The existing embedded and process examples remain separate evidence
cases; no 2010 behavior or profile parity is inferred.

The new flow passed the VS Code 1.141.0 Mermaid grammar parser, but no SVG or
screenshot was produced; local visual and GitHub-render review remain pending.
This was a documentation/source review only; no C++ build, test rerun, CI, or
Requirements Lab change was made.

### 2025 regional-interaction context and rejected-operation effects — 2026-10-10

A bounded reader review found a useful DDM distinction: a valid region handle
can still be incompatible with a particular interaction class because one of
its dimensions is not available to that class. Two separate embedded 2025
`HLA_EVOKED` tests exercise the sender and receiver sides. The send case rejects
a mixed valid/unavailable region set before any delivery is observed, then
delivers a valid-only send once. The subscription case rejects an unavailable-
dimension region, verifies a later send does not expose a hidden declaration,
then accepts a valid regional subscription and observes the later send.

Added an HLA-first flow to the [DDM guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md)
to show the shared class-dimension constraint and keep the two service effects
separate. The test comments point to §§9.1.3.3 and 9.1.4; the complete standard
text is unavailable here, so the prose does not enlarge those tested cases into
a universal transaction/rollback rule. The focused C++ test source was read,
not rerun. The new diagram has not yet had a fresh grammar or visual review;
there is no process or 2010 parity claim and no Requirements Lab change.

At this 2026-10-10 reader-review checkpoint, the inventory was 173 Mermaid blocks across 31 diagram-bearing guides:
86 flowcharts, 65 sequences, and 22 state diagrams. The separate 2010 stream
remains 10 diagrams; the 2025 set has 163. A fresh whole-docs VS Code 1.141.0
grammar pass accepted all 173 blocks, including the new DDM flow. It generated
no SVGs or screenshots. The earlier 178-block inventory is a previous
worktree snapshot, not the current count; visual/GitHub review is a separate
check.

### 2025 FOM admission/composition reader-model review — 2026-10-10

A bounded learner review found that the existing guide already makes the
important distinction clear: a module can pass DIF validation yet fail when
the ordered set is composed, when references are resolved against the complete
model, or when the resulting model is materialized and checked as an FDD. Its
separate Create and additional-module Join flows then show where the prepared
definition crosses into shared federation state. A fourth diagram would repeat
those stages rather than teach another durable HLA concept. Existing focused
tests cover cross-module reference resolution, an FDD-schema rejection, Create
failure without name reservation, and replacement-definition-plus-membership
commit; these sources/tests were inspected, not rerun. No C++ build, test rerun,
CI, Requirements Lab edit, or 2010 behavior claim was made. Corrected the
official IEEE 1516.2-2025 product link after checking the IEEE SA listing.

### 2025 order/transportation reader-model review — 2026-10-10

A bounded review confirmed the guide already separates sender-selected order
from the transportation category, receiver relevance, and time-management
eligibility. One useful test-backed nuance was less visible in the diagram:
calling a timestamped update/send service does not, by itself, establish that
the receiver observes a TSO event. The existing order-control test sets the
publisher's effective interaction order to `RECEIVE`, supplies a logical-time
argument, and observes receive-order metadata with no valid retraction handle.
Added that as an explicitly embedded-2025 test example in the delivery flow;
it is not presented as an independent normative interpretation. The existing
change-lifecycle and query diagrams already cover the other distinct reader
questions, so no additional diagram was needed. Source and focused-test evidence
were inspected, not rerun. No C++ build, test, CI, 2010 claim, or Requirements
Lab change was made. The added sequence note uses Mermaid's documented
multi-participant note and line-break syntax; no local visual render was made,
so visual and post-publication GitHub review remain pending.

Next bounded candidate: compare the 2025 publication/subscription, relevance,
and attribute-scope guides for a concise HLA-first explanation of declaration
changes versus class- and instance-level advisories. Keep current eligibility
and callback gates separate, avoid repeating the DDM/time diagrams, and create
or revise a flow only if the comparison exposes a real learner gap. Reuse
existing evidence and do not run C++ builds, tests, or CI by default.

### 2025 subscription and advisory reader-model review — 2026-10-10

A bounded comparison of the object-attribute subscription, declaration and
attribute relevance, and Attribute Scope guides found the distinctions already
explicit: subscription declarations are receiver state; class-level relevance
is reported to a publisher; object-attribute relevance is reported to an owner;
scope changes are reported to the receiving federate; and none of those
advisories is the attribute-value update itself. The relevance guide already
shows a subscription mutation feeding independent class and attribute
calculations, with discovery and scope planning kept separate. The scope guide
has the four-signal comparison; the subscription guide owns active/passive
declaration state. Current source paths and focused cases support these
separate lanes, while the cited profile and test limits are clearly labeled.
No combined chart would add a new state transition without duplicating those
guides, so no diagram was added. Source and test evidence were inspected, not
rerun; no C++ build, CI, 2010 claim, or Requirements Lab change was made.

Next bounded candidate: review the 2025 names/handle guide for a junior-friendly
distinction between declaration names, execution/member-scoped identities,
category-typed opaque API handles, and encoded handle values. Keep HLA API
semantics separate from Umbra's internal integer representation; add a flow
only if the current prose/diagrams leave a real learner gap. Reuse pinned API,
source, and existing test evidence without rebuilding or rerunning tests.

### 2025 HLA-first timing and reporting reader review — 2026-10-10

The time guide already puts the portable role, request, and grant mental model
before Umbra's effective-target, frontier, queue, and callback-policy details;
the existing diagrams and labels maintain that boundary, so no additional
timing chart was justified. The exception-reporting guide distinguishes
`HLAreportServiceInvocation`, `HLAreportException`, and
`HLAreportMOMexception`, while the cross-cutting guide separately connects the
service result, state effect, report path, and observer visibility. Together,
they already teach the difference between a thrown service exception and an
independent report; another combined diagram would duplicate the existing
model.

A bounded inventory of the six service-group headings in the pinned 2025
[RTIambassador.h](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h)
found contributor-guide families for Federation Management,
Declaration Management, Object Management, Ownership Management, Time
Management, and RTI Support Services. This is topic-level navigation coverage,
not a claim that every service, overload, exception, or implementation profile
is documented. No source or tests were changed or rerun, and no C++ build, CI,
2010 behavior claim, or Requirements Lab edit was made.

Next bounded candidate: audit the 2025 object-instance deletion/removal flow
against the existing object-information, ownership, and time guides. Separate
ordinary deletion from timestamped deletion, receiver-known/removal effects,
ownership consequences, and time/retraction gates. Reuse existing evidence and
revise a flow only for a concrete HLA learner gap; do not build or rerun tests.

### 2025 Declaration Management publication-withdrawal review — 2026-10-10

A bounded method-level review of the pinned 2025 Declaration Management group
confirmed existing guide coverage for publication/subscription, active/passive
state, relevance, regional scope, directed interactions, delayed evaluation,
Auto Provide, update-rate admission, and explicit attribute-value requests.
One real reader gap remained: ordinary object-class publication withdrawal
has an ownership consequence that was not visible in the object/interaction
overview. Added a focused 2025 embedded flow to
[that guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md):
selected versus whole-class withdrawal, superclass/subclass declaration
separation, the pending-acquisition rejection, ownership removal on matching
instances, and profile-specific metadata cleanup.

The pinned API declares `OwnershipAcquisitionPending` for the §5.3 withdrawal
services. The existing class-hierarchy case exercises withdrawal at a subclass
while a superclass publication remains; the pending-acquisition case observes
rejection. The registry source maps the guard and owner-loss rule to §5.3.3 and
shows the associated update-region/transportation/order cleanup, but the cited
tests do not assert each cleanup postcondition. The process adapter forwards
the services; the new explanation makes no process/embedded parity claim. The
new Mermaid flow has not yet been parser- or visually reviewed. No C++ build,
test rerun, CI, 2010 behavior claim, or Requirements Lab edit was made.

The follow-up ordinary interaction review found a second, distinct boundary.
In the current embedded path, `planReceiveOrderInteraction` checks publication
before acceptance; ordinary callback dispatch later rechecks the receiver's
eligibility and projection, not the sender's publication set. An unpublish
between acceptance and `HLA_EVOKED` callback dispatch therefore does not itself
fence that ordinary queued route in the inspected code. The separate directed
path explicitly rechecks its publication pair at callback time, and its focused
case covers suppression after withdrawal. Added a source-labeled ordinary
send/unpublish/evoke sequence to the object/interaction guide and connected the
contrast to the cross-cutting stale-work lens. No focused ordinary
send → unpublish → evoke test was found, so that behavior remains source-only;
no IEEE-wide rule, process parity, or 2010 behavior is asserted.

The four new 2025 diagrams—the object-class publication-withdrawal flow,
ordinary interaction unpublish-after-acceptance sequence, save/restore
responsibility map, and federation/federate lifecycle map—subsequently passed
targeted, non-rendering Mermaid grammar parsing with the installed VS Code
1.141.0 modules. The ordinary sequence's semicolon label was changed to a
comma after the parser identified it as a statement separator. This confirms
grammar acceptance only: no SVG or screenshot was generated, so visual layout
and GitHub rendering remain pending. No C++ build, test rerun, CI, 2010
behavior claim, or Requirements Lab edit was made.

## Suggested reader paths

The guides are standalone topic explanations. These paths are a navigation
aid, not a claim that adjacent topics are one normative state machine or that
the implementation profiles are equivalent.

### A conceptual map of one HLA federation execution (2025)

This is an application-facing orientation, not a required call sequence. Two
different questions are shown separately: how an execution and its members
come into and leave existence, and how joined federates exchange information.
Arrows show relationships, not one required per-federate script; use the
focused guides for each service's exact rules.

#### Execution model and member lifetime

```mermaid
flowchart TD
  AMB["Connected RTI ambassador"] --> CHOICE{"Execution path"}
  CHOICE -->|"create a new execution"| CREATE["Create federation execution"]
  CREATE --> EXEC["Execution exists<br/>creator is not thereby a member"]
  CHOICE -->|"join an existing execution"| EXEC
  MODULES["FOM modules<br/>shared class, member, and data-type vocabulary"] -. "supplied at Create or Join where applicable" .-> EXEC
  EXEC --> JOIN["Join federation execution<br/>may supply additional modules"]
  JOIN --> MEMBER["Federate membership begins"]
  MEMBER --> RESIGN["Resign ends this membership lifetime<br/>the execution may remain"]
  EXEC --> DESTROY["Destroy execution<br/>separate from Resign"]
```

Creation establishes an execution; it does not make the creator a member.
Join creates the member relationship, and Resign ends that member lifetime.
Destroy is a distinct execution-lifecycle operation. FOM modules provide
vocabulary for the execution and may be supplied through the applicable
Create/Join path. The [lifecycle](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md)
and [FOM composition](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md)
guides define the detailed boundaries.

#### From ambassador construction to observer discovery

This 2025-only orientation joins five focused reader paths without replacing
their service rules. It is not a required script for every federate: the
publisher and observer roles may be different members, and the arrows show
prerequisites rather than a total call order. FOM admission supplies the
class/attribute vocabulary; it does not create object instances. An explicit
name is optional, reservation success is not registration, and discovery is
not a value reflection.

```mermaid
flowchart TD
  APP["Application obtains an RTI ambassador<br/>from RTIambassadorFactory"] --> CONNECT["Connect<br/>RTI session established"]
  CONNECT --> UNJOINED["Connected, not a federation member"]
  UNJOINED --> CHOICE{"Create an execution or join an existing one?"}
  CHOICE -->|Create| CREATE["Create Federation Execution<br/>provide initial FOM modules"]
  CHOICE -->|Existing| EXISTING["Choose an existing execution"]
  CREATE --> EXEC["Execution exists<br/>creator is not yet a member"]
  EXISTING --> EXEC
  EXEC --> JOIN["Join Federation Execution<br/>additional FOM modules are optional"]
  EXEC -. "Admitted FOM supplies vocabulary,<br/>not live object instances" .-> MODEL["Execution object model"]
  JOIN --> MEMBER["Membership begins on successful Join"]
  JOIN -. "Join may add accepted modules" .-> MODEL
  MEMBER --> PUB["Publisher publishes object-class attributes"]
  MEMBER --> SUB["Observer subscribes to object-class attributes"]
  MODEL -.-> PUB
  MODEL -.-> SUB
  PUB --> NAMED{"Use an explicit object name?"}
  NAMED -->|Yes| RESERVE["Reserve the name<br/>await reservation-success callback"]
  RESERVE --> NAMED_REG["Register with the reserved name"]
  NAMED -->|No| UNNAMED_REG["Register without a caller-supplied name"]
  NAMED_REG --> INSTANCE["Object instance is registered"]
  UNNAMED_REG --> INSTANCE
  INSTANCE --> ELIGIBLE{"Is this peer eligible under the applicable<br/>object-visibility rules?"}
  SUB --> ELIGIBLE
  ELIGIBLE -->|Yes| DISCOVER["Peer observes discoverObjectInstance<br/>under its callback model"]
  ELIGIBLE -->|Not for this peer/event| NO_DISCOVER["No discovery for this peer<br/>from this registration event"]
```

This is a synthesis of separate API/service examples, not one end-to-end test.
The pinned API landmarks are [factory construction](../../third_party/ieee1516.1-2025/include/RTI/RTIambassadorFactory.h#L34),
[Connect](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L55),
[Create](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L128),
[Join](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L212),
[name reservation](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L647),
[registration](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L697),
and [discovery callback](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L229).
Focused examples are the [Connect/Create/Join lifecycle case](../../cpp/tests/ieee1516_2025_federation_resign_lifecycle_catch2.cpp#L4),
[reservation](../../cpp/tests/object_instance_name_reservation_catch2.cpp#L95),
[named registration](../../cpp/tests/object_instance_named_registration_catch2.cpp#L93),
and [registration/discovery](../../cpp/tests/object_instance_registration_discovery_catch2.cpp#L78)
cases. They cover different slices and profiles; they do not assert this whole
sequence or all normative discovery-eligibility rules. The detailed [connection](HLA-2025-CONNECTION-AND-CONFIGURATION-FLOW-GUIDE.md),
[lifecycle](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md),
[FOM admission](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md),
[name reservation](HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md),
and [object information](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md)
guides retain their separate evidence and profile boundaries. This chart does
not describe the 2010 stream or claim embedded/process callback parity.

#### Information paths and application observation

```mermaid
flowchart TD
  MEMBER["Joined federate"] --> DECL["Federates publish or subscribe independently<br/>capability and interest, not data transfer"]
  DECL -. "declaration state influences eligibility" .-> OBJECT["Object information<br/>registration / discovery / attribute update and reflection"]
  DECL -. "declaration state influences eligibility" .-> INTERACTION["Interaction information<br/>send / receiver selection / receive"]
  OBJECT --> OELIG["Per-receiver object knowledge<br/>and delivery eligibility"]
  INTERACTION --> IELIG["Per-receiver class<br/>and relevance eligibility"]
  OELIG --> OBS["Application observes an effect"]
  IELIG --> OBS
```

The object and interaction branches are not interchangeable. Discovery and
known-object state matter to object services; interaction delivery has its own
class and relevance rules. Regions, time/order, and callback model can further
constrain delivery or observation only where the selected service uses them.
See the [object/interaction information](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md),
[callback/service-ordering](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md),
[time-management](HLA-2025-TIME-MANAGEMENT-GUIDE.md), and
[DDM/regions](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md) guides.
This overview is 2025-only; it does not summarize the separate 2010 stream or
assert embedded/process parity.

### Navigate by the IEEE 1516.1-2025 API service family

The pinned public `RTIambassador` interface groups services by family. Use this
map to find a reader path; a row is not a method-by-method coverage claim, and
the guides intentionally link across families where one HLA behavior crosses
service boundaries.

| Public API family | Reader path |
| --- | --- |
| Federation Management Services | [Connection and configuration](HLA-2025-CONNECTION-AND-CONFIGURATION-FLOW-GUIDE.md), [federation and federate lifecycle](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md), [FOM admission and composition](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md), [synchronization points](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md), [save and restore](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md), and [federation-scoped MOM](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md). |
| Declaration Management Services | [Object and interaction information](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md), [object-attribute subscription state](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md), [relevance advisories](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md), [Attribute Scope Advisory](HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md), [Auto Provide](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md), and [Delay Subscription Evaluation](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md). |
| Object Management Services | [Object and interaction information](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md), [object-instance name reservation](HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md), [attribute-value update requests](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md), [directed interactions](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md), [order and transportation](HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md), and [TSO retraction](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md). |
| Ownership Management Services | [Attribute ownership](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md) explains pending acquisition, owner response, If Available, cancellation confirmation, Divestiture If Wanted's output-set result versus later notification, and ownership queries; [ownership persistence and resignation](HLA-2025-OWNERSHIP-PERSISTENCE-AND-RESIGNATION-FLOW-GUIDE.md) covers selected pending work across those boundaries. |
| Time Management Services | [Time management](HLA-2025-TIME-MANAGEMENT-GUIDE.md), with adjacent [TSO retraction](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md) and [attribute update-rate admission](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md) for their own service-specific interactions with time. |
| Data Distribution Management | [DDM and regions](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md) and the regional paths in [object and interaction information](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md). |
| RTI Support Services | The cross-cutting [switch-control map](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md#a-support-switch-has-an-owner-a-lifetime-and-a-control-surface) distinguishes static federation policy, federation-wide Auto Provide mutation through the MOM path, Join-seeded member defaults, and member-local controls, then asks when each consuming service samples the value and how its effect becomes observable. Follow [names and handle resolution](HLA-2025-NAME-AND-HANDLE-RESOLUTION-FLOW-GUIDE.md), [callback](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md), [update-rate](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md), [advisory](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md), [exception-reporting](HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md), and [service-invocation-reporting](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md) for operation-specific rules. |
| API-specific services | [Time management](HLA-2025-TIME-MANAGEMENT-GUIDE.md) explains the federation-selected logical-time representation and `getTimeFactory`; [DataElement encoding](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md) covers the logical-time element codec handoff; [names and handle resolution](HLA-2025-NAME-AND-HANDLE-RESOLUTION-FLOW-GUIDE.md) covers typed-handle value boundaries. |

This index follows the 2025 API grouping only. Read the separate 2010 guides
on their own; this family map does not infer 2010 behavior or parity.

- **A portable way to reason across HLA services:** begin with the
  [cross-cutting state, event, and observation model](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md),
  then use the focused time-management, object/interaction, DDM, and callback
  guides for operation-specific rules. Its examples are 2025-only and do not
  bridge the separate 2010 stream.
- **From ambassador construction to a first eligible object observer** (2025
  reading path, not a required call sequence):
  [connection/configuration](HLA-2025-CONNECTION-AND-CONFIGURATION-FLOW-GUIDE.md)
  → [authorization](HLA-2025-AUTHORIZATION-FLOW-GUIDE.md) →
  [federation lifecycle](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md)
  → [FOM admission/composition](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md)
  → [object/interaction information](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md).
  Create and Join remain distinct execution/member transitions, and discovery
  is a per-receiver eligibility effect, not an automatic consequence of Join.
  Read [name reservation](HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md)
  alongside registration only for the optional named-registration path:
  unnamed registration has no reservation step, and a reservation neither
  creates an instance nor notifies peers. Continue to [federation-scoped MOM](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md)
  for execution-level reports; [joined-federate MOM object and interaction reports](HLA-2025-MOM-OBJECT-AND-INTERACTION-REPORTS-FLOW-GUIDE.md)
  covers requester-local queries, while the [joined-federate MOM lifecycle](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md)
  guide traces the per-member `HLAfederate` object. The focused guides define
  each boundary and its evidence.
- **Connection, membership, and execution are separate:**
  [Connect, Disconnect, and configuration](HLA-2025-CONNECTION-AND-CONFIGURATION-FLOW-GUIDE.md) explains the local RTI
  session gates; [authorization and credentials](HLA-2025-AUTHORIZATION-FLOW-GUIDE.md) follows the 2025 authorizer from
  Connect through Create/Destroy/Join and distinguishes the typed credential envelope from authorizer-specific policy;
  [federation lifecycle](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md) distinguishes Connect/Disconnect,
  Join/Resign, and Create/Destroy, including rejoining on the same connected ambassador in one focused embedded case.
- **Why an attribute update reached a federate:** start with the cross-cutting
  [attribute-value concept map](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md#an-object-attribute-value-touches-several-independent-boundaries),
  then use the
  [delivery and observation checklist](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md#keep-delivery-and-observation-questions-separate).
  The focused guides distinguish publication and per-instance ownership, receiver knowledge and subscription/DDM,
  optional solicitation, update-rate admission, logical-time eligibility, and callback observation; follow only the
  gates relevant to the service. Begin with
  [object/interaction information](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md), then choose
  [subscription state](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md),
  [attribute-value requests](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md),
  [ownership transfer](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md),
  [DDM/regions](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md),
  [update-rate admission](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md),
  [time management](HLA-2025-TIME-MANAGEMENT-GUIDE.md), or
  [callback/service ordering](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md) as needed. For broader
  update/interaction routing and advisory paths, see [relevance advisories](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md),
  [attribute scope](HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md),
  [Auto Provide](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md),
  [Delay Subscription Evaluation](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md), or
  [directed interactions](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md).
- **How timestamp, order, transport, callbacks, and retraction relate:**
  [time management](HLA-2025-TIME-MANAGEMENT-GUIDE.md) →
  [callback/service ordering](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md) →
  [order and transportation](HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md) →
  [TSO retraction](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md). Then read
  [DSE](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md) or
  [directed interactions](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md) for their recipient boundaries.
- **What happens when in-flight work becomes stale:** start with the cross-cutting guide's
  [withdraw/invalidate/preserve/account lens](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md#when-work-becomes-stale-withdraw-invalidate-preserve-or-account),
  then follow the service-specific subscription, ownership/resignation, TSO-retraction, or lifecycle guide. For a queued
  regional TSO interaction after the receiver replaces its subscription, see the DDM guide's
  [bounded no-retarget trace](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#a-replacement-regional-interaction-subscription-does-not-retarget-queued-tso-work).
  Do not merge their cancellation and observation rules.
- **How a service and callback cross the 2025 process endpoint:**
  [process request/callback flow](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md) separates private
  request correlation, pushed frames, explicit receive polling, callback-model dispatch, the per-recipient TSO
  callback-acknowledgment seam, source-derived poll-family priority versus pushed-stream FIFO, local typed checks and
  generic remote planner rejection for interaction sends, and structured planner-status results for Update Attribute Values.
- **Which protocol result am I waiting for, and when can the app observe it?** Use the cross-cutting
  [completion-boundary map](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md#different-protocols-have-different-completion-boundaries)
  to keep time-advance, synchronization, and save/restore outcomes distinct from callback dispatch; then follow
  [federation lifecycle](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md) →
  [synchronization points](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md) →
  [save/restore](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md), including a bounded embedded trace of a live regional
  TSO interaction →
  [ownership persistence and resignation](HLA-2025-OWNERSHIP-PERSISTENCE-AND-RESIGNATION-FLOW-GUIDE.md) →
  [joined-federate MOM lifecycle](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md); add
  [TSO retraction](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md) when in-flight timestamped work is involved.
- **How a failure becomes a report or callback:** begin with the cross-cutting guide's
  [service-result/state/report distinction](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md#a-service-result-committed-state-and-a-report-are-separate-traces),
  then follow Exception Reporting's [three distinct report paths](HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md#the-three-report-paths-relevant-to-exception-reporting),
  including the [MOM-rejection branch](HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md#route-b-a-mom-interaction-is-rejected) →
  [service invocation reporting](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md) →
  [callback/service ordering](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md) for the separate mechanisms and
  callback boundary.
- **From a reportable service call to an observer callback:** use the Service
  Invocation Reporting guide's [three-trace map](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md#three-distinct-traces-call-outcome-invocation-report-requested-mom-report)
  and [destination flow](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md#a-reportable-service-call-selects-one-destination)
  to distinguish the caller's result, its invocation record, and the separate
  observer selected through subscription/DDM. `HLAreportServiceInvocation` is
  an RTI-originated interaction, not the application's original interaction
  callback; an explicitly requested MOM report is a third trace. For those
  request/report pairs, continue to the [federation-scoped MOM flow](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#federation-reports-are-receive-order-interactions)
  or the [joined-federate MOM object and interaction reports guide](HLA-2025-MOM-OBJECT-AND-INTERACTION-REPORTS-FLOW-GUIDE.md)
  to follow the separate data scopes and report projections. The
  [configured-process recipient boundary](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md#configured-process-recipient-snapshot-and-callback-boundary)
  differs from embedded callback revalidation. File-append failures may affect
  caller outcomes in selected wrappers; no universal report-before-return or
  observer callback order is claimed.
- **Why a set-valued operation may have partial or all-or-nothing outcomes:** start with the cross-cutting
  [service-contract versus argument-shape map](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md), then compare
  the [name-reservation](HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md),
  [FOM composition](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md), and
  [attribute ownership](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md) guides. Read outcomes from each service contract;
  Umbra's state-commit units are profile-specific examples, not semantics inferred from the argument's shape.
- **Model declarations, identity, and encoded values:** Begin with the 2025 cross-cutting guide's
  model-vocabulary/runtime-state distinction, then use
  [FOM module admission/composition](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md) for model assembly;
  [names and handle resolution](HLA-2025-NAME-AND-HANDLE-RESOLUTION-FLOW-GUIDE.md) for typed lookup, active federate
  names, and known-object scope;
  [object-instance name reservation](HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md) for the distinct
  owner-reservation and peer-discovery paths; and
  [2025 DataElement encoding](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md) plus its
  [separate 2010 companion](HLA-2010-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md) for edition-specific public C++ value bytes.
  Keep model representation, API encoding, and RTI transport framing distinct.
- **Bounded 2010 reference behavior:** read the
  [separate 2010 reference-RTI guide](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md) on its own. The 2025 paths above are not
  evidence of 2010 behavior.

## Local Mermaid rendering and syntax audit

The current 2025-only census finds 305 Mermaid fences across 31
`HLA-2025-*` topic guides (172 flowcharts, 110 sequences, 23 state diagrams).
A Mermaid 12.0.0 core syntax pass with a Node-only no-op DOMPurify adapter
accepted all 305 charts with zero grammar failures. This validates grammar
only; it does not validate sanitization, SVG rendering, layout, visual clarity,
or GitHub presentation. The queued process regional-attribute DDM sequence
remains unrendered: a targeted attempt with the installed Chromium executable
failed to launch (code 3221225506; an earlier attempt failed with 2147483651).

A focused check also validated all 23 local links and 22 source line anchors in
the process-recipient subsection; these are not merged into the historical link
aggregate.

The per-recipient order-conversion sequence in the 2025 Order and Transportation
guide and the reservation-owner restore sequence pass the whole-tree syntax
scan. The new ownership/delete-privilege sequence also parses separately; all
three await local render and visual review.

A separate local-link integrity pass on 2026-10-16 scanned all 43 Markdown
files under `docs/design`. It checked 3,513 local inline link destinations and
2,634 GitHub `#L...` line anchors; every referenced file exists and every line
range fell within its target at the time. The scan excludes fenced code,
external URLs, reference-style link definitions, and non-line fragments, so it
does not validate heading slugs or GitHub rendering. A broad post-edit local
link check found no missing targets or invalid line anchors; its total counts
do not match this prior audit's counting method and are not substituted here.

The dated static fence census (2026-10-11 working-tree scan) found 247 Mermaid
diagrams across 34 diagram-bearing Markdown files (41 design Markdown files
total). Count both backtick and tilde Mermaid fences. This counts fenced
blocks only; it does not establish syntax validity, rendering, visual review,
or GitHub presentation. A bounded recount on 2026-10-12 after the new
Federation MOM observation-path flowchart found 256 diagrams at that time
across 34 of 43 design Markdown files: 14 in the separate 2010 guides, 227 in
2025 topic guides, and 15 in the cross-cutting guide and this index. Types at
that time were 148 flowcharts, 85 sequence diagrams, and 23 state diagrams.

A fresh 2026-10-14 working-tree census counted 299 Mermaid fences in 34 of 43
design Markdown files: 16 diagrams in the separate 2010 guides, 267 in 2025
topic guides, 13 in the cross-cutting guide, and 3 in this index. The current
diagram types at that census were 169 flowcharts, 107 sequence diagrams, and
23 state diagrams. A 2026-10-15 update-rate generation flowchart brings the
current count to 300 fences across the same 34 files: 16 separate 2010 charts,
268 2025 topic charts, 13 cross-cutting charts, and 3 index charts (170
flowcharts, 107 sequences, 23 state diagrams). The update-rate chart had no
syntax or render verdict at that census because Puppeteer could not launch;
the 2026-10-16 parser-only pass later accepted all 303 fences. Rendering and
visual review remain pending; the census is not whole-tree visual sign-off.

The successful local render evidence is a historical 247-diagram snapshot
(the 242-chart baseline plus five later charts), not a render of the current
305 source fences. On 2026-10-14, Mermaid CLI 12.0.0 rendered all six charts in
the Attribute Value Update Request guide; the new process-path comparison was
visually reviewed. This is targeted coverage, not a whole-tree render. Do not
infer an exact pending-chart count from the historical render totals: edits to
existing charts also make old renders stale, while some charts have separate
targeted rerender evidence. The review
ledger records evidence by chart; without a current source-to-render manifest,
no exact current pending count is claimed. The process attribute DSE chart's
syntax is included in the 2026-10-16 whole-tree parser pass; it has no local
render or visual verdict because Mermaid CLI could not launch its browser (exit
code 2147483651). The living backlog's historical
reader-gap notes have been moved to the non-diagram
[progress log](HLA-BEHAVIOR-FLOW-GUIDES-PROGRESS-LOG.md).

- The separate 2010 guides contain 16 diagrams across 2 files.
- The current scan of 2025-prefixed topic guides contains 305 diagrams across 31 files.
- The cross-cutting guide and this index contain 16 reader-model/navigation
  flowcharts across 2 files; these do not assert edition-specific behavior.

The current 2025-only census comprises 172 flowcharts, 110 sequence diagrams, and 23 state diagrams.
The 2026-10-11 local Mermaid CLI 12.0.0 pass rendered the 242-block baseline;
five later charts were separately rendered and screenshot-reviewed, producing
the historical 247-block evidence set. On 2026-10-12, a fresh source-fence
inventory, before the timestamped-deletion chart was added, counted the
then-current set as 14 2010 diagrams,
223 2025 topic-guide diagrams, 13 cross-cutting-guide diagrams, and 2 index
diagrams. A full current-set render was attempted with the already available
Mermaid CLI 12.0.0, but Puppeteer/Chrome could not launch: it reported an
already-running browser profile and then a Windows account-launch error, even
with a fresh profile under the permitted validation workspace. No SVGs were
produced, so this attempt establishes neither current render nor syntax
coverage. Existing chart-specific evidence remains valid only for the exact
source snapshot recorded in the review ledger.
Both fence styles were extracted explicitly because the CLI's Markdown mode
silently skips tilde fences. The pass found two existing 2025 semicolon-label
syntax errors; after correction, all six diagrams in the reservation guide
and all four in the process request/callback guide rendered. The 11 callback-
ordering diagrams rendered after the new exception sequence was reflowed.
One dense 2025 callback sequence and a separate 2010 DataElement flow were
visually inspected in the baseline review. The five charts that extended the
242-chart baseline to the historical 247-chart snapshot have their own
targeted screenshot review; this does not review later edits or additions.
GitHub rendering remains a separate post-publication check. Current local
render and screenshot status must be reconciled chart-by-chart in the review
ledger rather than inferred from aggregate counts.
No C++ build, test rerun, CI, or Requirements Lab work was performed.

The 2025 credential-envelope chart was locally rendered from its source
Markdown with Mermaid CLI 12.0.0 and visually inspected on 2026-10-11. All
four authorization-guide diagrams rendered as PNGs; this targeted check does
not establish GitHub rendering or validate the full diagram set. The review
ledger records the renderer setup and chart-specific status.
The temporal-admission/callback-entry synthesis chart was revised after a
source/test boundary review: it now distinguishes deferred receive-order work,
dispatcher-pending work, the temporary pending-advance gate, and the bounded
pre-grant callback observation. It is included in these counts and received a
targeted local Mermaid render and visual review on 2026-10-11. That review
caught unquoted parenthetical node labels; the `HLA_IMMEDIATE` endpoint now
states callback entry explicitly. At that chart-specific checkpoint, the
complete inventory had not yet been audited; the current full-set result is
recorded above. A prior whole-set non-rendering grammar pass accepted a
different 205-block snapshot and is historical, not the current result.
The eleven-item review queue below was recorded for an earlier 214-block
snapshot; it is historical evidence, not a current pending-review count. That
214 total has no saved per-file manifest, so the identity of the six-block
difference cannot be reconstructed from the totals. Reconcile current charts
against dated, chart-specific evidence before claiming their status. The
historical queue named the TSO strict producer-boundary
sequence, “Retract a regional update before three available-advance callbacks,”
the TSO ownership-change timeline, the support-switch map, the revised HLA
contract/outcome map, the reservation/peer-visibility synthesis, the revised
single-name reservation callback sequence (HLA_IMMEDIATE versus HLA_EVOKED),
the portable TSO-retraction outcome diagram, the cross-cutting attribute-value
map, the order/transportation orientation map, and the new HLA/API-to-process
identity map. The associated historical notes explicitly marked the first two
TSO sequences pending and recorded no review for the ownership timeline. These
are not asserted as current statuses; check the current chart and dated ledger
before treating any item as open. The
handle-keyed value-path map has now been locally rendered and screenshot-
reviewed; its HLA API container, typed member key, encoded value, and separate
tag boundaries are clear. It does not imply a wire-format or Umbra process-
frame contract. The DataElement representation-boundary map, the
HLA-first subscription-interest reader map, and the HLA/API authorization
admission map have been rendered and visually reviewed locally; GitHub
rendering remains open for all three. The DDM mixed-advance sequence and the
synchronization late-join chart and partial-result sequence have also been
rendered and screenshot-reviewed locally; their test/source-specific limits
remain explicit. Local visual review and post-publication GitHub rendering are
separate checks; individual status remains in the review ledger. Earlier
local-only syntax/layout corrections are separately tracked for rerender. On
2026-10-11, the installed Mermaid CLI and its matching Chrome headless shell
successfully rendered the four diagrams in the 2025 Authorization guide; the
credential-envelope diagram was screenshot-reviewed and its labels and paths
were legible without overlap. This is a targeted local visual check, not a
whole-set render or GitHub verification. No repository-derived diagram text
was sent to an external preview.
An earlier
197-block pass found and led to fixes for two malformed labels: a
parenthesized `HLAsetTiming` flowchart edge label and a semicolon in a
sequence-diagram note. The member-specific periodic-MOM cadence sequence,
current Join-boundary map, and time-representation/factory bridge were also
included in that earlier pass.
No SVGs or screenshots were produced; visual layout and GitHub rendering
remain unverified.

The following parser and inventory notes preserve historical checkpoints; the
205-fence result above is the latest whole-set grammar checkpoint, not a syntax
result for later 209-, 214-, or current 220-block inventories.

A non-rendering whole-docs grammar pass on 2026-10-10 accepted the previous
173-block snapshot using the Mermaid modules bundled with VS Code 1.141.0,
including the DDM scope/time/callback chart. Four later 2025 diagrams are not
included in that parser result or rendered: the object-class publication
withdrawal flow, ordinary interaction unpublish-after-acceptance sequence, and
the save/restore-responsibility and lifecycle-scope orientation flows. That
pass generated no SVGs or screenshots and did not establish layout or GitHub
parity. Older 178-block totals below describe a different earlier worktree
snapshot.

A later targeted VS Code 1.141.0 grammar pass accepted all four named blocks
above, including the ordinary-interaction sequence after its semicolon label
was changed to a comma. This pass did not render those diagrams. Since that
checkpoint, the two conceptual maps in this guide were visually inspected in
the local VS Code Markdown preview and displayed without Mermaid error panels.
That visual check covers only those two maps, not the then-current 188-block set;
GitHub rendering remains unverified. The recorded whole-worktree and targeted
parser results have not been reconciled against every current block. Do not
claim a fresh whole-set syntax pass or infer that any unmatched block is
invalid.

On 2026-10-10, an initial syntax sweep parsed all 155 blocks with the Mermaid grammar
modules bundled in VS Code 1.141.0: the separate 2010 set passed 10/10 (7
flowcharts and 3 sequences), and the 2025 set passed 145/145 (70 flowcharts,
53 sequences, and 22 state diagrams). The first sweep exposed five sequence
charts with semicolon-separated prose and one authorization flowchart with
unquoted function-call labels; those source blocks were corrected before the
clean rerun. The parser ran in a non-rendering Node harness with a minimal
DOMPurify shim. The initial sweep established grammar acceptance only; it
generated no SVGs or screenshots and did not prove preview layout readability.
A current sequence-only rerun on 2026-10-10 accepted all 65 sequence blocks
with the VS Code 1.141.0 bundled grammar: 3 in the separate 2010 guides and
62 in 2025 guides. It caught a semicolon-delimited label in the new process-
endpoint DDM sequence; the Mermaid label punctuation was repaired and the
complete set then passed 65/65. This parser-only rerun generated no SVGs or
screenshots and did not prove rendered readability. A whole-set grammar pass
on the current inventory then accepted all 165 blocks using the same VS Code
1.141.0 Mermaid modules with a minimal browser/DOMPurify shim: 78 flowcharts,
65 sequences, and 22 state diagrams. The separate 2010 stream passed 7
flowcharts and 3 sequences (10 total); 2025 passed 71 flowcharts, 62
sequences, and 22 state diagrams (155 total). This establishes parser
acceptance only; it generated no SVGs or screenshots and does not prove
GitHub rendering or visual readability. The first four diagrams in the
cross-cutting state/event guide were separately parsed with the same VS Code
1.141.0 Mermaid grammar modules, bringing the recorded inventory to 169. Later
2026-10-10 static scans counted 172 blocks across 31 guides, then 178 across
33 diagram-bearing guides. A fresh 2026-10-10 VS Code 1.141.0 grammar pass
accepted that 178-block snapshot (87 flowcharts,
67 sequences, 24 state diagrams; 2010: 10, 2025: 168). It generated no SVGs or
screenshots, so rendered layout and GitHub parity remain unverified.
An earlier post-clarification run on 2026-10-10 reaccepted all 165 then-current
blocks with the same bundled grammar (78 flowcharts, 65 sequences, and 22
state diagrams). A fresh whole-worktree parser pass on 2026-10-10 accepted all
169 blocks in the inventory at that time (82 flowcharts, 65 sequences, and 22
state diagrams) using the VS Code Mermaid module in a non-rendering Node
harness with a minimal DOMPurify shim. It generated no SVGs or screenshots:
grammar acceptance does not establish GitHub rendering or visual readability.
The two chart-boundary revisions described below first brought the total to
157. The evidence-backed regional-TSO restore trace brought it to 158; the
timed directed-TSO restore trace brought it to 159. Two additional 2025
object-deletion restore traces brought the total at that point to 161: 78
flowcharts, 61 sequences, and 22 state diagrams (2010 remains 10/10; 2025 is
151). The test-shaped producer-boundary trace in the TSO-retraction guide adds
one 2025 sequence, bringing the total to 162 (78 flowcharts, 62 sequences,
22 state diagrams; 2010 remains 10/10; 2025 is now 152).

A first preview attempt on 2026-10-10 found no open app or browser tabs and
could not reach its temporary loopback endpoint. After explicit user permission
for local sockets, the 12 charts that had been pending were rendered and
screenshot-reviewed with the installed VS Code Mermaid Markdown feature
bundle. Each rendered to one SVG with no Mermaid error panel. The preview
covered 2025 charts only; it did not merge or re-review the separate 2010 set,
and it does not substitute for GitHub verification after publication.

Visual review found the class-relevance self-loop crowded, the embedded
ConnectionLost sequence too wide, and the federation-scope chart combining
independent lifetimes. The relevance labels were clarified; the two latter
charts were split at their real ownership boundaries (registry commit versus
post-commit cleanup, and federation scope versus per-member seeding). All
resulting diagrams were rendered and inspected. These two thematic splits—not
line-count targets—account for the increase from 155 to 157. The regional-TSO
worked restore sequence brought the set to 158; the timed directed-TSO save
boundary trace is one additional evidence-backed sequence and brings the
earlier checkpoint count to 159.

Before this parser sweep, 102 diagrams had been rendered through the Mermaid
Markdown renderer bundled with the installed VS Code build and produced SVGs
with zero renderer error nodes. The
three ownership-persistence/resignation diagrams were separately rendered
through that same bundle in a loopback-only Edge preview; all three produced
SVGs with zero diagram errors and were visually inspected. One semicolon in a
sequence-note label was replaced after the first parse attempt, then all three
were rendered and inspected again. The added object-removal invalidation
flowchart and revised deletion sequence were rendered through the same bundle
in a loopback-only Edge preview; both produced SVGs with no diagram errors and
were visually inspected. The accepted-send,
interaction-relevance/projection, and three attribute-update validation,
projection, and callback-boundary flowcharts all parsed and were visually
inspected in local Chrome. The three update-rate charts and four
attribute-value-request charts also parsed and were visually inspected. The
request-scope planning view was split into scope expansion and per-group
filtering after the first preview exposed intertwined loops; both views are
now separately readable. The request-response sequence's logical-time queue
note was revised to avoid implying an internal self-call. The
handle-normalization support-service flowchart, the
automatic-resign directive lifecycle sequence, and all three subscription-state
diagrams were rendered with the Mermaid bundle shipped in VS Code and
individually screenshot-reviewed in local Chrome. The passive/active gate
sequence needed two semicolon-to-comma repairs. Visual review also caught
registry self-call labels crossing the automatic-resign chart's lifeline; the
chart now distinguishes its registry-owned per-member directive field from
external service calls. All five final charts parse; normalization and regional
subscription are tall but readable when scrolled. The six remaining charts—
federate-local object forget, connection-loss regional TSO, FQR EVOKED delivery
frontier, directed-interaction unpublish, directed queued-callback fence, and
federation member-list snapshot—now all parse and have been visually reviewed
in local Chrome. The regional sequence's semicolon parse failure was repaired;
registry/queue self-loops were replaced with boundary notes; and the member-
list sequence now places EVOKED invocation after `evokeCallback` and IMMEDIATE
invocation inside submission. The object-forget and unpublish flowcharts are
tall but remain readable while scrolling. The no-time regional object-
attribute update sequence was also rendered and visually inspected locally
after two note-label punctuation repairs; it is tall but readable when
scrolled, with no text or lifeline collisions. These are local renderer
findings only, not GitHub verification.
The five baseline diagrams in the 2025 federation-synchronization guide were
parsed by the VS Code Mermaid bundle and individually screenshot-reviewed in
local Chrome. The registration chart is tall but readable while scrolling; the
late-join chart separates expandable from fixed participant sets; and the
achievement state and sequence views make resignation completion, false-result
fan-in, and callback dispatch legible. The persistence sequence traces the
saved achievement ledger but labels multi-member partial progress as
source-derived: public API tests restore one-member unachieved points, while the
codec test only round-trips a false result. The late-join chart and the partial-
result-before-join sequence have now also been rendered and screenshot-reviewed
locally. The trace shows A's false report retained across C's expanding join,
then requires both B and C before the barrier closes; its caveat states that the
focused direct and public tests join before any achievement. This remains a
source-derived 2025 implementation path, not a standard-mandated late-join
rule. GitHub verification remains pending publication.
The callback and service-ordering guide's new FederationSynchronized flow was
also parsed and screenshot-reviewed locally. It connects registry completion
and point removal to per-recipient report logging and callback submission,
including immediate-drainer contention and the HLA_EVOKED evoke gate. Its
service-specific IMMEDIATE-before-return ordering remains source-derived, not
asserted by the public synchronization-point test. GitHub verification is
pending publication.

The prior
102-diagram audit
includes the four federation-scoped MOM diagrams; its state chart was
shortened after an initial preview showed overlapping terminal/self-loop
labels. The earlier
review also includes the six valid tilde-fenced diagrams in the Auto Provide
and Attribute Scope guides (two flowcharts, two sequence diagrams, and two
state diagrams), which were missed by the original backtick-only scan and
were then individually inspected. That review found a clipped note in the
Auto Provide discovery sequence; an explicit line break fixed it, and the
updated chart was rendered and checked again. This is local renderer evidence,
not GitHub renderer or published-page verification.

The 22 previously queued local-only or revised diagrams were opened
individually and visually inspected. The added object-removal invalidation
flowchart, revised deletion sequence, no-time regional update sequence, and the
five-chart handle-normalization/automatic-resign/subscription batch brought
that reviewed set to 30. The five object/interaction charts and seven charts
from the update-rate/request batch (including the additional request-scope
view) have since been rendered and visually inspected, bringing the reviewed
local-only/revised set to 42 diagrams. The six formerly pending charts were
then rendered and inspected, bringing that set to 48; no chart in the current
local-review backlog remains pending. Two additional 2025 ownership-query
diagrams now cover classification/result grouping and the callback-time
revalidation fence. Both parsed in the installed VS Code Mermaid core and were
individually screenshot-reviewed in local Chrome; the tall classification
flow is readable while scrolling. The reviewed local-only/revised set is now
50 diagrams. Two more diagrams in the 2025 timing guide show asynchronous
receive-order admission and a test-shaped EVOKED/IMMEDIATE callback trace
across enable, disable, and pending-advance gates. Both parsed and were
individually screenshot-reviewed in local Chrome. That brings the reviewed
local-only/revised set to 52 diagrams. Two further save/restore status-query
sequences were rendered and individually inspected in local Chrome. They
distinguish read-only callback snapshots from operation completion and are
bounded to the named embedded `HLA_IMMEDIATE` tests; this brings the reviewed
local-only/revised set to 54 diagrams. These local checks do not establish
GitHub renderer parity. A 2010-only sequence in the separate reference-RTI
guide now shows that its generated shell accepts but does not load FOM paths,
then lazily allocates class handles through the reference provider directory.
It was rendered and screenshot-reviewed locally against the 2010 shell/source
and binding-smoke evidence, bringing the reviewed local-only/revised set to 55.
It does not imply 2025 behavior or normative 2010 FOM semantics.
A separate embedded 2025 ownership flowchart now shows accepted object
deletion invalidating queued query, regular owner-release, If-Available, and
cancellation work while retaining an independently gated receiver-removal
callback. It rendered and was screenshot-reviewed locally; the four cited
tests cover those callback families separately and do not establish global
ordering or process parity, bringing the reviewed local-only/revised set to 56.
Targeted rechecks confirmed the repaired
ownership and synchronization labels/sequences and the revised TSO
recipient-state layout. The time-grant chart remains tall, but its decisions
and outcomes are legible in the expanded preview. Tall charts still require
scrolling and are called out in the backlog where relevant.

The 2025 service-invocation guide adds an embedded switch/report failure
sequence: the Exception Reporting switch commits before its selected file
append, an injected append failure reaches the caller as `RTIinternalError`,
and a getter still observes the committed switch. The source-derived MOM
projection enqueue after the append is skipped on this path; its state is not
asserted by the focused test. The diagram parsed in the local VS Code Mermaid
renderer and was screenshot-reviewed, bringing the reviewed local-only/revised
set to 57. A sixth service-reporting diagram now traces timestamped attribute
update TSO admission before file-report persistence. Its source-derived failure
branch has no TSO rollback and returns no retraction handle; the existing tests
cover the normal TSO/report route and writer failure on a different service,
not that combined schedule. It parsed and was screenshot-reviewed locally,
bringing the reviewed local-only/revised set to 58. Neither diagram establishes
process-endpoint or 2010 behavior.

An earlier isolated headless check parsed 72 flowchart and sequence blocks
with identity text sanitization. It is historical and is superseded for local
syntax/render coverage by the complete 98-chart browser pass; neither check validates
GitHub's renderer, sanitization behavior, or published-page layout. Recheck
locally changed diagrams in GitHub after publication, and do not claim GitHub
parity from this preview. The lifecycle sequence's participant ID was changed
from `Link` to `Transport` because the installed renderer rejected `Link` while
GitHub rendered it; the visible participant label is unchanged, so this is a
local renderer-compatibility adjustment, not evidence of a GitHub defect.

## Normative source access — 2026-10-09

The [official IEEE 1516.1-2025 page](https://standards.ieee.org/ieee/1516.1/6688/)
identifies the standard and offers purchase/subscription access, but its public
metadata does not expose clause 4.4. Keep the lifecycle cutoff flow explicitly
scoped to Umbra's implementation until the exact normative text can be checked
in an authorized copy; do not infer the clause from implementation comments.

On 2026-10-10, the same official listing was checked for the exact §4.11
conditions attached to Join's `SaveInProgress` and `RestoreInProgress`
exceptions. Its public page exposes purchase and subscription access, not the
clause text. The API exception list is not enough to establish those
conditions, so the Join/source discrepancy remains an unresolved question,
not a conformance conclusion. This access limit does not block the
documentation goal: continue bounded, content-first guide reviews using
existing evidence and leave the Join outcome unknown until an authorized copy
is available.

The official [IEEE 1516.1-2025 listing](https://standards.ieee.org/ieee/1516.1/6688/)
was also checked against the test-authored references to §§9.1.3.3 and 9.1.4
for the new interaction-region context flow. The page identifies the active
2025 edition and its supersession of 2010, but does not expose those clauses;
the chart therefore remains a bounded record of the tests and their cited
sections, not an independently verified normative interpretation.

On 2026-10-11, I checked the [official IEEE downloads page](https://standards.ieee.org/downloads/)
and retrieved its linked `1516.1-2025_downloads.zip` under the page's personal-use
terms. The archive contains a nested `1516-2025_API_XML_2025_08_14.zip`; its
contents are C++/Java API files plus protobuf, XML, and schema files, with no
PDF or HTML normative document. It therefore does not supply §6.21 text. The
standard listing still identifies purchase/subscription access, so the exact
requester-response entitlement remains open pending an authorized copy. Do
not infer it from the API archive or Umbra's planner.

On 2026-10-12, the [official API archive](https://standards.ieee.org/wp-content/uploads/2025/08/1516.1-2025_downloads.zip)
was checked for the discovery/first-reflection question. Its
`FederateAmbassador.h` labels `discoverObjectInstance` as §6.9 and
`reflectAttributeValues` as §6.11, but contains API declarations rather than
the clause prose or an ordering rule. The process selector's
discovery-before-reflection comment therefore remains an implementation
interpretation; the focused process test checks both callbacks occurred but
does not assert their relative order. Keep this question open until the
authorized standard text is available.

The [official IEEE 1516.1-2025 listing](https://standards.ieee.org/ieee/1516.1/6688/)
was rechecked on 2026-10-12 for the §8.22 Retract/ownership-transfer question.
It offers purchase and subscription access and does not expose the clause
prose; the public API download is not the normative standard text. Keep the
registry's missing attribute-owner check labeled as an embedded source fact,
not a portable Retract rule or a verified post-transfer service result.

### 2025 DDM recipient-path survey — 2026-10-12

A bounded comparison of the DDM, object/interaction, relevance-advisory, and
directed-interaction guides found the reader paths already separate the three
regional questions. Object-attribute delivery compares the publisher's
per-attribute update-region association with each receiver's regional
subscription; ordinary interaction delivery uses its own explicit/default
source realization and receiver subscription; and logical-time eligibility
and callback observation remain separate gates. The focused ordinary
interaction case demonstrates that a disjoint explicit regional subscription
does not fall back to a retained ordinary subscription, while removing the
regional declaration restores the ordinary default-region path. The empty
`sendInteractionWithRegions` set is separately documented as no-send, not as
the default region.

One cross-topic nuance is already covered by the relevance-advisory guide:
in the inspected 2025 source, an active regional declaration can establish a
publisher's class-level relevance signal without a particular object or a
region-overlap result. That signal does not establish receiver delivery. A
directed send instead names an object target and uses a directed receiver
selector; its API has no region-set argument. The directed guide correctly
leaves any normative directed-DDM relationship open. Regional state found in
the directed source file belongs to declaration-level relevance logic and is
not evidence for adding an overlap gate to directed delivery.

Conclusion: no new synthesis diagram is warranted; the existing guides and
their reciprocal pointers cover the distinct eligibility questions without
merging them. The pinned API, source, and focused test bodies were inspected,
not run. No guide diagram changed, so the 261-diagram inventory and prior
per-chart render status are unchanged. No build, test rerun, CI, 2010 claim, or
Requirements Lab edit was made.

### 2025 federation lifecycle versus connection teardown review — 2026-10-12

A bounded reader audit of the lifecycle guide and the revised connection guide
found the important exits already form separate, understandable paths. The
2025 lifecycle map distinguishes ambassador connection, member lifetime, and
execution existence; successful Resign ends membership, Disconnect is only
available after resignation, and connection loss enters RTI-forced cleanup
with a per-member automatic directive plus its forced safety effects. In the
selected embedded final-member cases, both voluntary resignation and loss can
apply the final-member object cleanup while leaving the execution available
until explicit Destroy. This is selected Umbra evidence, not a complete
normative ResignAction matrix or cross-profile rule.

The updated connection flow owns the separate adapter/client teardown details:
process-client close is not a Resign operation, and the service-side Detach
call remains conditional and unproven as an API-caused action. The lifecycle
guide already points to that companion; save/restore, synchronization, and
TSO consequences likewise remain in their own guides. Its process member-list
route gap and the full-standard clause gaps are labeled. Focused final-member,
forced-`NO_ACTION`, and connection-loss test sources were inspected, not run.
No additional lifecycle diagram or prose change is warranted; diagram count
remains 261, and no new chart requires a render check. No build, test rerun,
CI, 2010 behavior claim, or Requirements Lab edit was made.

## Prioritized guide backlog

### Portfolio triage and priorities beyond timing — 2026-10-11

A bounded inventory of the current guide map, matching source paths, and
focused test names found that the principal 2025 service families already have
dedicated guides: object/interaction flow, ownership, DDM, FOM composition,
federation lifecycle, synchronization, save/restore, callbacks, and exception
reporting. The next useful increment is therefore a cross-guide explanation
only where a reader must join distinct service concepts; do not commission
another guide simply because a service method or source file is not named in a
diagram.

The active review queue is IEEE 1516.1-2025 only. Keep edition-specific guides and evidence in their own sections; do not use one edition to fill evidence gaps in another.

Current priority order:

1. **Object existence versus local knowledge (completed synthesis).** The detailed
   object guide already has registration/discovery, federation deletion, and
   local-forget flows, but the cross-cutting guide's concise explanation was
   prose-only. A small relationship map now connects those concepts without
   absorbing their service gates or callback timing.
2. **Ownership transfer and persistence (reviewed).** The core guide covers
   per-attribute transfer and callback-time state; its persistence companion
   covers selected image records, resignation, and recipient validation. A
   new process-profile map now distinguishes a callback already queued for
   HLA_EVOKED invocation from value-only ownership work reconstructed by
   restore. These are separate tested paths, not one state machine or an
   HLA-wide ordering rule. The ownership guide now also has a four-path HLA-first
   selector for requester-driven If Available, owner-driven Unconditional and
   If Wanted divestiture, and negotiated confirmation; each route remains
   linked to its own detailed diagram and bounded test evidence.
3. **Synchronization and save/restore participant sets (reviewed; forced-loss closure audited; active Join unresolved).**
   The cross-guide audit distinguishes synchronization late-join expansion,
   save/restore participation, and resignation-during-save outcomes. Exact
   Join-during-save admission remains unresolved without licensed §4.11 text;
   do not infer a shared participant rule. A 2026-11-03 source/test recheck
   confirmed the Join paths have no visible save/restore phase gate and the
   representative interlock case joins both members before Save. Repaired
   stale process `handleJoin` pointers in the Save/Restore and cross-cutting
   guides and the interaction-routing ledger; no behavioral change or new
   chart was warranted.
4. **Resign versus Disconnect with in-flight work (surveyed; profile teardown clarified).**
   The lifecycle guide separates ambassador connection, federate membership,
   and execution lifetimes; the callback guide distinguishes server-queued,
   client-buffered, and callback-bridge work at Resign; the TSO guide shows
   process Resign versus transport Detach for its retraction route. The
   object/interaction guide now traces pending ownership-assumption and removal
   effects across receiver Resign and links back to the callback guide for
   dispatcher behavior; the combined race remains untested. The lifecycle guide now maps the
   six voluntary ResignAction values to current embedded flags, guards, and
   selected tests; full action-by-ledger combinations remain open. The
   connection guide now branches the existing Disconnect flow by profile and
   separates adapter CallbackSession close, process-client socket/bridge close,
   and server `ProcessFederationService::detach`. The checkout has no production
   callsite for that service cleanup, and the focused process test calls it
   separately in its server fixture. Keep this integration limit explicit;
   post-Resign process behavior remains source-scoped and not publicly tested.
   A 2026-10-14 HLA-first lifecycle audit found the existing three-lifetime
   model and Create/Join/Destroy and Resign/Disconnect paths already explain
   the core membership/execution distinction; no duplicate map was warranted.
   All 15 current lifecycle diagrams rendered locally. The active-save
   resignation sequence was simplified and visually reviewed; the connection-
   loss TSO cutoff flow was also visually reviewed. GitHub rendering is still
   pending publication, and exact §4.4 wording remains unverified without the
   full standard text.
5. **2025 data-distribution management and regions (reader-model and restore-profile survey reviewed).**
   The DDM guide and object/interaction companions already separate per-
   attribute update-region associations, effective regional subscriptions,
   ordinary interaction source regions, directed target selection, and
   regional provider solicitation. Keep their time gates separate. Save/Restore
   now has distinct embedded examples for an unassociated committed Region and
   for an active object-attribute association plus queued TSO update across a
   fresh `EmbeddedFederationRegistry`. The selected tests verify restored
   handles/bounds and queued delivery; source confirms association capture and
   reconstruction. A new source-derived 2025 process map shows that the
   configured client/service delegates Save/Restore to the registry's typed
   image path, including Region definitions and restore-time catalog/member
   validation. This is not a process Region test: the inspected process public
   cases cover lifecycle/status/callbacks without Region operations, while
   the file-backed Region test creates a second registry in the same test
   process. Keep the actual process-endpoint persistence outcome unverified;
   do not infer OS-process restart or 2010 behavior.
6. **2025 Join during active Save/Restore (open normative follow-up).** The
   current Lifecycle, Synchronization, and Save/Restore guides separate
   source-derived Join and participant-set behavior from the unresolved
   normative rule. Both pinned §4.11 Join overloads list `SaveInProgress` and
   `RestoreInProgress`, but the inspected embedded entry, process handler, and
   registry `joinImpl` have no explicit admission branch for those phases. The
   representative interlock test joins both members before Save; it does not
   exercise Join after Save or Restore begins. These API annotations and source
   observations do not settle the exact IEEE 1516.1-2025 requirements. The [official
   IEEE SA listing](https://standards.ieee.org/ieee/1516.1/6688/) offers
   purchase/subscription access; its [public downloads archive](https://standards.ieee.org/wp-content/uploads/2025/08/1516.1-2025_downloads.zip)
   contains API and HLA model/schema assets but no copy of the standard. Obtain
   authorized §4.11 text before resolving the rule, and keep synchronization
   late-Join expansion separate.

Completed 2025 audits of FOM admission, callback-exception ordering, DataElement encoding, and public API coverage remain recorded in the progress log; they are not active backlog items.

### 2025 object-attribute subscription ancestry audit — 2026-10-15

A bounded comparison of the subscription-state, object-information, relevance-
advisory, and DDM guides with the embedded discovery resolver, attribute
recipient planner, and focused tests found the main mechanics already diagrammed
but their ancestry roles scattered across guides. Added a concise crosswalk to
the subscription guide: class-level active relevance is distinct from
per-receiver discovery and saved known-class selection; later value projection
uses that receiver-known class; regional declarations remain exact class/attribute
entries even when the embedded discovery walk inspects ancestors. The existing
object-ancestry and regional-DDM diagrams already carry the detailed flows, so
no duplicate chart was added.

The crosswalk preserves the evidence limits: the embedded child-object/base-
subscription case does not test a separate passive receiver; the process
parent/child case shows one callback for overlapping active subscriptions only;
and parent-keyed regional lookup for a child object remains source-only because
the reviewed regional tests use the same class. Publisher declaration
advisories, owner relevance advisories, receiver scope advisories, discovery,
and value reflection remain separate effects. No 2010 or profile-parity claim,
build, test rerun, CI, or Requirements Lab edit.

### 2025 subscription-generation and update-rate audit — 2026-10-15

A bounded comparison of the subscription and update-rate guides, registry
mutation paths, admission-key builder, deterministic gate test, and public
regional-subscription case found a useful gap between declaration changes and
the rate filter's history key. Added one flowchart to the update-rate guide.
The embedded source advances the receiver-wide generation on ordinary
subscription mutation paths; for regional subscription, an empty outer map
returns unchanged, a nonempty map advances it even when an inner region set is
empty, and regional unsubscribe advances it only after removing a stored pair.
The generation is part of every receiver/object/attribute key, so a new value
stream can begin after a mutation even when that mutation did not change the
stored regional pairs.

This remains a composed source/test inference: the private rate-gate test uses
synthetic keys to show a new generation has fresh history, while the public
empty-region case checks retained declaration behavior but does not measure a
rate window across the call. Its §9.8.4 no-op comment is test-author
interpretation, not independent normative evidence. The 2026-10-16
parser-only pass accepts the new chart's syntax; Mermaid CLI still cannot
launch Puppeteer, so rendering and visual review remain pending. No
build, test rerun, CI, Requirements Lab, 2010 behavior, or profile-parity claim.

### 2025 additional-module Join and object-subscription audit — 2026-10-15

A bounded comparison of the FOM Join, subscription, and object-information
guides found a reader gap between two already-documented paths: the Join commit
does not replan ordinary discovery for existing instances, while the discovery
resolver walks a registered object's class toward subscribed ancestors. The
FOM guide now distinguishes an existing object registered before Join from a
new object registered later under a Join-added descendant. Its existing chart
shows the source path from that later registration through the current-catalog
resolver to the receiver-known class used by subsequent value projection.

This is a cross-path embedded-source inference. The additional-FOM lookup test
checks catalog visibility and preservation of an earlier handle; the
child-object/base-subscription test uses a class already present at Create.
No focused case combines retained ancestor subscription, Join-added class,
later registration, discovery, and subsequent reflection. The configured-
process Join case also stops at lookup. The chart makes no process-parity,
2010-parity, or normative Join-to-callback claim. Its changed Mermaid source
needs a fresh syntax/render and visual review; no C++, build, test, CI, or
Requirements Lab work was done.

### 2025 additional-module Join and interaction-subscription audit — 2026-10-15

A bounded source/test review confirmed the FOM guide already traces interaction
handle reconciliation, retained per-federate declarations, current-catalog
ancestor matching on a later send, closest callback-class selection, and
parameter projection. The object/interaction guide already diagrams the
ordinary interaction event path and the separate active-relevance,
receiver-selection, and parameter-projection decisions. Added direct links from
the Join section instead of duplicating that state machine.

The evidence remains split: the embedded receipt case exercises a
base/middle/child hierarchy present at Create and uses an empty parameter map;
the separate registry projection case filters parameters for distinct
receivers. The configured-process additional-FOM case checks object-class
lookup, while the process send path delegates to the shared registry planner.
No focused test combines Join-added descendant, retained ancestor interaction
subscription, subsequent send, and callback projection. This is not process
parity or normative evidence. No diagram source changed; no build, test, CI,
Requirements Lab, or 2010 work was done.

### 2025 additional-module Join and object DDM audit — 2026-10-15

The DDM guide already has a Join-specific source chart: added class-dimension
references can grow current class availability while an existing region keeps
its captured dimension set. The bounded audit found the missing bridge was
between that context-admission check and the later object subscription/update
route. Added prose and cross-links to show that regional subscription and
update-region association requests are independently validated against current
class availability, while their declaration/association state and later
source/receiver overlap remain separate transitions.

The joined scenario remains source-only. The additional-module dimension test
adds a separate class/dimension and checks stable prior handles; the focused
three-dimensional regional object test covers overlap without a later Join.
No test combines a same-class dimension extension, retained regions, new
regional subscription/association state, and an observed later update. No new
chart was warranted because the class-ancestry and overlap paths already have
separate diagrams. No normative or process/2010 parity claim; no build, test,
CI, Requirements Lab, or 2010 work.

### 2025 additional-module Join and regional-interaction reader audit — 2026-10-16

The bounded audit confirmed that the DDM guide already traces current
dimension context, exact-class regional declaration storage, and later
descendant-send parent/overlap selection; the FOM guide links directly to that
trace. The focused embedded cases separately cover a newly added class and
dimension lookup, and a same-class regional send, not their Join-plus-retained-
parent-subscription combination. Added the configured process source mapping:
process Join supplies its prepared definition to the registry, regional
subscriptions call the registry declaration methods, and receive-order send
uses the same registry planner. The process fan-out test joins without an
additional module and does not cover the combined route. This is shared
source-path evidence, not process parity or a normative callback rule; no
second chart was warranted.

Next: perform a bounded reader-gap audit of the 2025 Delay Subscription
Evaluation guide. Clarify deferred subscription/relevance evaluation versus
service validity and delivery guarantees; compare receive-order, timestamped,
directed, and process routes against existing focused evidence. Revise diagrams
only for a demonstrated reader gap. Keep 2010 and 2025 separate, distinguish
embedded/process evidence, and do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 Delay Subscription Evaluation reader audit — 2026-10-16

The guide already distinguishes sender-side validity, receiver eligibility,
receive-order versus TSO boundaries, directed selectors, and embedded versus
process rechecks. A remaining timing seam was that `HLA_EVOKED`/
`HLA_IMMEDIATE` could be mistaken for the only receive-order delivery gate.
Added a concise boundary note and cross-links to the existing 2025 time guide
and focused asynchronous-delivery/TSO-exclusion tests: in the embedded profile,
time-constrained receive-order work can be deferred until asynchronous
delivery is enabled or a time advance is pending, while asynchronous delivery
does not release TSO work early. This is adjacent time-management evidence,
not additional DSE or process evidence. No fourth chart was warranted because
the time guide already diagrams this temporal gate; no 2010/2025 parity or
normative §8.1.8 claim was added.

### 2025 FOM post-Join chart: local render and visual review — 2026-10-16

A fresh isolated Mermaid CLI 12.0.0 render of the revised consequence chart
succeeded after earlier browser-launch failures. Visual review found the
vertical branches legible without clipping. It also exposed a chronology
ambiguity: the entry box said “Join succeeds” before the registry commit. The
chart now starts at a prepared additional-module Join request and labels the
atomic catalog/handle-directory/member commit. The preceding Join-phase chart
already covers composition and post-commit setup/rollback, so no duplicated
phase chart was added. The normative Join-to-callback duty and the complete
post-Join behavior remain unresolved; the configured-process test checks
catalog lookup only. This local render does not establish GitHub presentation.

### 2025 per-recipient sent-TSO order-conversion chart: render and visual review — 2026-10-16

Rendered the per-recipient order-conversion sequence from the 2025 Order and
Transportation guide with Mermaid CLI 12.0.0 and visually reviewed the result.
Review found that the prior sequence omitted the test's `HLA_EVOKED` polling,
which could make callbacks look asynchronous, and its polling loop could read
as if a grant repeated on every poll. The revised sequence shows the
unconstrained receiver polling for its receive-order callback, the constrained
receiver polling before its time advance with no interaction yet, the test's
sender `timeAdvanceRequest(2)`, and constrained polling until the TSO callback
and grant are observed.

The evidence remains one embedded `HLA_EVOKED` test: both receivers report a
sent `TIMESTAMP` order and valid retraction designator, while only the
time-constrained receiver reports received `TIMESTAMP`; it also checks the
interaction before the grant, payload, tag, transportation, and logical time.
The source-comment mapping to §8.1.4 Table 3 is not independently verified
against licensed standard text. No configured-process parity or complete
order-conversion table is claimed. The 2025 inventory remains 317 diagrams
across 31 guides (179 flowcharts, 115 sequences, 23 state diagrams); twenty-two
selected charts have local visual review. GitHub presentation and whole-tree
review remain pending. Next: review the 2025 reservation-owner restore flow.
No C++ build, test, CI, or Requirements Lab work.

### 2025 reservation-owner restore chart: render and visual review — 2026-10-16

Rendered the saved-reservation sequence in the 2025 Object-Instance Name
Reservation guide with Mermaid CLI 12.0.0 and visually reviewed it. The
sequence is readable without clipping; shortened its saved-image note while
preserving the assertion that the image contains the name and original
Federate A owner ID.

The source and focused two-federate public test support the depicted boundary:
the state image records the reserved name and owner ID; a fresh registry is
created in the same test process; both federates rejoin with their original
handles; restore validation reattaches the reservation to the live owner;
the peer cannot release or reserve it; after the owner releases it, the peer
can reserve it and the former owner can no longer release it. The guide
correctly limits this to the embedded profile and notes that callback reports
are checked after a follow-up helper, not at a universal service-return
boundary. It does not establish OS-process-restart behavior or a normative
save/restore rule. The 2025 inventory remains 317 diagrams across 31 guides
(179 flowcharts, 115 sequences, 23 state diagrams); twenty-three selected
charts have local visual review. GitHub presentation and whole-tree review
remain pending. Next: review the updated embedded Request Attribute Value
Update flow. No C++ build, test, CI, or Requirements Lab work.

### 2025 Request Attribute Value Update: direct requester entitlement — 2026-10-16

Rechecked the existing guide's open §6.21 question against its selected source
and tests. The embedded requester-as-receiver example subscribes before the
request; the process request case seeds subscription state through the registry
fixture. Neither isolates an otherwise ineligible requester receiving a
provider's later update. The pinned API header provides the signature and
exception list, not clause prose. The official IEEE product page offers
purchase/subscription access; its linked ZIP could not be inspected in the
browser reader, so no authorized §6.21 text was reviewed. The existing guide
already labels the entitlement question unresolved, so no wording or diagram
change was warranted. No normative rule was inferred.

The former process-endpoint receive-order timing handoff was completed on
2026-10-16. The bounded audit confirmed a source-path difference from the
embedded temporal gate, but found no checked-in public process case for the
constrained/asynchronous/pending-advance matrix. The public outcome remains
unresolved; no additional timing diagram or parity claim is warranted.

Next: continue the non-timing, gap-driven reader audit in the current roadmap
above. Revisit this timing edge only when direct source or focused test evidence
changes. Do not build, rerun tests, start CI, or edit the Requirements Lab.

Syntax/render validation, local visual inspection, and post-publication GitHub
review are separate checks. The last full census before the process
callback-exception diagrams counted 294 diagrams. The current post-edit fence
count is 305 across 34 diagram-bearing files (172 flowcharts, 110 sequences,
and 23 state diagrams), counting both backtick and tilde Mermaid fences. The
two callback routes and wrapper/report chart have been locally rendered and
visually reviewed. Mermaid CLI 12.0.0
is installed globally through the existing Scoop Node.js/npm installation;
its compatible Puppeteer browser runtime is cached locally. An earlier local
render pass covered 48 diagrams in five sources: this index (3), the separate
2010 reference guide (13), the 2025 Federation MOM guide (10), the 2025
Connect/configuration guide (4), and the 2025 Save/Restore guide (18). A later
whole-guide pass rendered all 13 diagrams currently in the Federation MOM
guide, so the same five-source set now has 51 rendered charts. The three new
MOM charts were also rendered individually and visually inspected. This is
targeted source coverage, not a render of all 306 diagrams. Nine previously
queued charts received local PNG visual review; their chart-specific findings
are in the review ledger. The former parser-only snapshot and 247-chart render
remain historical, partial results. GitHub rendering remains pending until
publication; do not infer whole-tree visual coverage from the selected files.

Separately, six targeted 2025 DDM charts were rendered with Mermaid CLI 12.0.0
and visually inspected on 2026-10-14: the fifth Attribute Value Update
Request flow, Region deletion gate, interaction-region dimension context,
FOM-dimension-to-Region bridge, queued regional-attribute TSO revalidation,
and queued regional-interaction subscription replacement. These are
chart-specific reviews, not a whole-guide or whole-tree rendering claim.

A seventh chart was reviewed later that day: the conceptual “Keep time and DDM
as separate filters” map now branches region scope, order/time, callback model,
and callback enablement as independent questions rather than one implied
service sequence. Its focused process case uses `HLA_EVOKED` and a separate
callback-enable switch; it is not embedded or 2010 parity evidence. The revised
chart was rendered and visually reviewed locally.

| Order | Area | Purpose and likely diagrams | Status |
| --- | --- | --- | --- |
| Synthesis | Cross-cutting state, event, and observation model | Teach portable ways to distinguish model vocabulary from runtime declarations, then separate admission, state ownership, recipient eligibility, pending effects, application observation, distinct coordination-completion contracts, public service outcome versus report/state effects, protocol-specific participant-set changes, configuration scope, and operation-defined HLA outcomes versus profile-specific commit points; link focused guides instead of copying their flows. | The current guide contains 13 diagrams; the latest 2026-10-16 inventory is 314 diagrams across 34 files (179 flowcharts, 112 sequences, 23 state diagrams). All nine entries in the current local render-review queue still have local PNG visual review. They cover the HLA-first onboarding map, a separate 2010 binding-shell trace, two 2025 process Federation MOM traces, Connect finalization, and four Save/Restore views. Separately, a whole-guide Mermaid CLI pass rendered all 13 current Federation MOM diagrams, and its three newly added diagrams were visually reviewed. The same five targeted Markdown sources now cover 51 diagrams; this is not a whole-tree render. GitHub rendering remains pending until publication. Keep these conceptual lenses explanatory, not a universal RTI architecture or normative state machine. |
| Baseline | Time management | Role enable/disable, advance requests, grant bounds, TSO queue delivery, callback ordering, and the receive-order asynchronous-delivery gate. | Four of five baseline diagrams were individually inspected on GitHub (2026-10-09): the two independent role charts are clear, though the constrained-state label sits under the preview controls; the grant-decision chart is very tall with converging/crossing branches; and the TSO queue-phase chart is readable. The advance-lifecycle sequence fails baseline parsing where semicolon prose runs into the next message; the local comma repair is present. The grant chart is simplified locally into mode/role/GALT gates, leaving exact boundary cases in the adjacent matrix. The additional source/test-bounded FQR EVOKED delivery-frontier sequence now parses and has been visually inspected locally; clause 8.12 interpretation remains unverified. The receive-order asynchronous-delivery gate flow and focused EVOKED/IMMEDIATE sequence now parse and were screenshot-reviewed locally; they distinguish callback admission from dispatcher policy and keep TSO/grant rules separate. A new 2025 embedded Modify Lookahead flow distinguishes immediate increases from deferred decreases, partial lookahead reduction at successive grants, and retry-safe grant preflight. Its focused internal state test passed locally (18 assertions); the process public-API test encodes before/after grant queries but failed at initial RTI connection in this environment, before lookahead assertions. Modify Lookahead was rendered and screenshot-reviewed in the 2026-10-10 local preview; GitHub verification of local revisions remains pending. The 2026-10-12 recipient audit clarified that the queue chart follows an individual TSO-queued copy; an adjacent table records one embedded attribute mixed-fanout test's distinct order and retract outcomes. The existing retraction-guide timeline already teaches generic mixed-recipient behavior, so no chart or inventory-count change was warranted. The 2026-11-06 client-FIFO review refined the configured-process FQR/Disable sequence: the public process case covers a task admitted before Disable, while a client-FIFO event admitted afterward retains timestamped form in current source; combined FQR/FQG order remains untested. The revised chart was rendered and screenshot-reviewed. Inventory remains 451 diagrams and selected review is 198/0. |
| 1 | Attribute ownership | Per-attribute regular and If-Available acquisition, denial, negotiated/unconditional divestiture, assumption offers, callback-time state changes, cancellation/staleness edges, query-only ownership classification, and deletion-driven invalidation of pending ownership callbacks. | All three baseline parse-clean diagrams were opened and visually reviewed on GitHub (2026-10-09); the negotiated-divestiture sequence is legible, the unconditional-divestiture flow is tall but readable while scrolling, and the requester-state chart has stray states from a semicolon label. The other two acquisition sequences failed baseline parsing; local comma repairs are in place. The 2025 embedded query classification/revalidation diagrams and the accepted-delete invalidation flow were added from source and focused-test review, rendered, and visually reviewed locally. The tall classification flow remains readable while scrolling. The deletion chart is limited to selected embedded HLA_EVOKED query, regular owner-release, If-Available, and cancellation tests; If-Wanted/negotiated/Confirm Divestiture deletion remains source-observed only. The guide now distinguishes void acquisition calls and synchronous typed exceptions from per-attribute callback outcomes, and records a bounded HLA_IMMEDIATE process observation at its actual subsequent-call delivery boundary. It does not claim global callback ordering or process parity. Verify all local ownership revisions in GitHub after publication A bounded cross-guide HLA-first survey on 2026-10-11 found the per-attribute request, callback-result, service-return, and transfer boundaries already explicit in the core guide; the persistence companion separately explains saved ownership facts, live callback routes, and membership validation. The existing cross-cutting milestone model supplies the overview, so no duplicate acquisition/resignation map is warranted. Normative arbitration and untested profile behavior remain open. The 2026-10-14 full local Mermaid audit rendered and visually reviewed all 18 current guide diagrams. It fixed a semicolon that blocked sequence parsing and shortened crowded labels in acquisition, If-Available, negotiated-divestiture, process cancellation, and If-Wanted charts while leaving exact API names and citations in surrounding prose. Revised charts were rerendered and visually checked. GitHub review remains pending publication; no test rerun, build/CI, Requirements Lab, or 2010 behavior work. |
| 2 | DDM and regions | FOM dimensions and class availability; region range/commit lifecycle; source/receiver overlap; object discovery and per-attribute relevance; interaction routing; and the separate logical-time gate. | The 20-diagram guide version rendered and visually inspected on 2026-10-14. That pass fixed two inline-code backtick labels, compacted the Region-template lifecycle chart, and reduced process fan-out lanes while retaining separate strict/relaxed routes and making no cross-receiver callback-order claim. Three later DDM diagrams remain pending. This turn revised the commit-triggered discovery sequence to distinguish pending bounds, the committed routing snapshot, and the embedded `HLA_EVOKED` callback boundary. Mermaid 12.1 syntax parsing passed; the current 23-diagram guide now has four charts pending fresh render/visual review. GitHub rendering of current revisions remains pending publication; no build, tests, or CI. |
| 3 | Save and restore | Federate save coordination, callback gates, save completion/failure, restore selection/rejoin, restoration of deferred runtime work, read-only status snapshots, and the separation between void request services and callback-reported protocol outcomes. | The HLA-first orientation map distinguishes each federate's application-owned checkpoint from the RTI's federation-owned checkpoint, and local completion reports from the federation-level outcome; it is explicitly a responsibility model, not a normative sequence. The service vocabulary now also distinguishes void request returns from callback-reported restore admission and terminal outcome, without assuming callback entry always follows the service call. The detailed diagrams retain service/profile boundaries. All four baseline diagrams were GitHub-rendered and visually reviewed (2026-10-09). The two worked 2025 embedded `HLA_EVOKED` traces contrast an untimed regional interaction save with a timed directed-interaction cutoff; a separate evidence row links the regional attribute-update payload case without duplicating its lifecycle diagram. The timed trace preserves the focused test's `FlushQueueRequest` callback-before-grant sequence and following `evokeCallback(0.0) == false` observation as implementation evidence, not a general rule. Both traces are test-bounded and explicitly exclude process/durable-restart parity and 2010. The revised restore sequence and two `HLA_IMMEDIATE` status-query sequences were rendered and inspected locally; they do not claim process/embedded parity or all status transitions. Verify changed diagrams on GitHub after publication. The bounded 2026-10-11 Join audit found the unresolved §4.11 admission/participant contract already framed accurately; it now explicitly distinguishes synchronization-point late-join expansion from save/restore participation. A 2026-10-12 embedded `HLA_IMMEDIATE` sequence now shows an unassociated committed Region deleted after Save, then restored and queried with the same handle, dimensions, and bounds. It distinguishes the live Region object from queued TSO source-region metadata; pending edits, active associations, process endpoint, host restart, and 2010 remain unproven. Included in the targeted local Mermaid render and individually visually reviewed; GitHub rendering remains pending until publication. No normative conclusion is inferred without the standard text. |
| 4 | Object and interaction information flow | Publication/subscription, registration/discovery, update/request, interaction delivery, and removal/resignation effects; connect typed value encoding to the RTI service/callback boundary without merging routing into serialization. | Baseline and subsequent deletion, removal-invalidation, accepted-send/report, interaction-relevance/projection, attribute-update, receiver-projection, callback-boundary, and federate-local forget diagrams were rendered and reviewed locally or on GitHub as recorded above. The twelfth regional active/passive relevance sequence was included in the 2026-10-10 preview and screenshot review. Its focused test still fails during federation creation before reaching the transition assertions, so visual review does not upgrade the behavior to runtime-verified. The ordinary publication-withdrawal-after-send sequence now distinguishes send admission from callback-time receiver eligibility and the directed path's separate publication fence; the ordinary edge is source-only and has no focused send → unpublish → evoke case. The API-header-grounded value path now connects the FOM member type, typed value map, service call, corresponding callback, and receiver decode. It was parsed, rendered, and visually inspected locally on 2026-10-11; object update/reflection and interaction send/receive stay separate through callback entry before joining at decode. The cited element-encoding, embedded interaction-callback, and configured-process attribute-reflection cases provide layered evidence, not one end-to-end proof across all data types or profiles. GitHub verification remains pending. Both diagrams were locally rendered and visually reviewed on 2026-10-11. The publication-withdrawal sequence was compacted to three lifelines so service calls, successful returns, the still-pending accepted route, and receiver eligibility recheck are legible; it remains an embedded 2025 source observation with no focused send → unpublish → evoke test. The callback-boundary chart is tall but clear. GitHub verification remains pending. The public process active/passive declaration test verifies only Boolean state storage through the endpoint, not interaction delivery. The two-recipient regional transition remains source/test-intent evidence because the last recorded local run failed before its assertions. The accepted-send chart now points readers to the separate temporal-admission and callbacks-enabled gates; this is a cross-guide clarification, not a new diagram or process-parity claim.  A bounded 2026-10-11 cross-guide audit found the existing HLA mental model already separates declarations, per-instance ownership, receiver selection, object knowledge, and application observation. The object guide now labels the distinct 2025 embedded callback-time versus process pre-transport known-object commit points as source-only; the cited process test covers eventual discovery, not pre-callback handle use. No universal delivery pipeline or new diagram was added.  A bounded process late-subscription audit confirmed the existing subscription guide accurately records the ordinary initial-reflection source/test gap: the process discovery branch constructs initial values only for RTI-owned MOM objects, while selected ordinary-process tests cover discovery or advisories but not late-subscription values. This is not a conformance finding; §5.8 authority remains unavailable.  A bounded 2026-10-11 interaction audit confirmed that the guide already separates ordinary active relevance, recipient matching/class promotion/parameter projection, and directed target-class selector lookup versus sent-class parameter namespace. The embedded callback-time recheck and process stored-projection paths remain distinct; no duplicate map was warranted. Exact inheritance rules and untested declaration-change behavior remain source-only.  A bounded 2026-10-11 review of focused unpublication candidates found no test asserting that an already-accepted ordinary interaction route survives sender unpublication through evoke/receive: the public process send case receives immediately, and the inspected regional candidates drain before unpublishing. The guide's source-only status remains accurate; no additional diagram was warranted.  A bounded 2026-10-11 directed-interaction audit confirmed the dedicated guide already distinguishes sender, target, and receiving federates; target-class ancestry and selector checks; ownership versus universal selection; and embedded callback revalidation versus process event snapshots. Its process test is exact-class/two-recipient evidence only and does not prove ancestor lookup or stale-event suppression; no duplicate diagram was needed. |
| 5 | Federation and federate lifecycle | Create/join/resign/destroy, connection loss, final-member behavior, member-scoped automatic resign directives, member-list snapshots, and cleanup of member-owned state. | All five baseline diagrams were inspected in GitHub's renderer (2026-10-09). Two execution-state self-loops were consolidated locally. The sixth, source-reviewed 2025 diagram on the lost-regulator TSO cutoff rendered and was visually reviewed locally; verify GitHub after publication. The seventh diagram tracing FDD-default seeding, per-member override, and connection-loss behavior now parses and has been screenshot-reviewed locally; the chart makes the registry-owned per-member directive field explicit to avoid self-loop label collisions. Verify GitHub after publication. The eighth source/test-bounded member-list snapshot sequence now parses and is visually reviewed locally; its callback branches show EVOKED-after-evoke versus IMMEDIATE-inline submission. Direct IMMEDIATE coverage for this member-list service remains a test gap. A new HLA/API-facing map now shows the one-shot member-list request/report boundary, `NotConnected`, callback-model observation, and the need for a fresh request after membership changes. It deliberately makes no exact snapshot-instant or callback-order claim; Mermaid grammar/render/visual review remains pending. Existing embedded evidence copies member records before callback submission; focused HLA_EVOKED success and missing-execution cases do not cover mutation between query and evoke or direct IMMEDIATE delivery. The ninth source/test-backed sequence explains callback-time resignation rejection and active-save failure for survivors; it passes the bundled Mermaid syntax check, while local screenshot review remains pending. A new HLA/API-facing ResignAction map separates requested cleanup, RTI admission, successful membership exit, and execution lifetime; it is vocabulary orientation, not a normative action decision table, and awaits grammar/render review. A bounded HLA-first reader review confirmed the three lifetimes and their scope map remain distinct; the guide now links the pinned API return types (including Join's `FederateHandle`) without conflating the returned member identity with observer discovery. The newer API vocabulary table distinguishes connection failures from membership failures and explicitly limits claims about exception precedence. That lifetime-scope review needed no duplicate map. Verify exact IEEE 1516.1-2025 clause 4.4 wording against the full standard before treating implementation flows as normative guidance. A bounded HLA-first survey on 2026-10-11 confirmed that the guide already separates caller-visible ResignAction results, successful membership exit, execution lifetime, the local connectionLost report, and conditional survivor notifications. No extra generic callback map is warranted. The full normative action matrix and total survivor-callback ordering remain explicitly unresolved; 2010 behavior stays in its separate guide. |
| 6 | Callback and service ordering | Queueing, callback models, service re-entry boundaries, ordering guarantees, and interactions with time-managed delivery not already covered elsewhere. | The four baseline diagrams were GitHub-rendered and visually reviewed (2026-10-09). FederationSynchronized dispatch, task-exception cleanup, and process-profile Resign remain locally parsed and screenshot-reviewed; the Resign explanation preserves the distinction between pending EVOKED work, disabled IMMEDIATE backlog, and event-specific staleness guards. The eighth 2025 ConnectionLost producer flow distinguishes process-local cleanup (no shared-dispatcher reset) from embedded forced resignation (dispatcher reset before local callback), and both producer catch-all boundaries. It is source-derived: the cited embedded report test fails during federation creation before its callback assertions, and no cited test injects an exception from ConnectionLost. Do not infer profile parity. The embedded service-report sequence is now split at registry commit versus post-commit cleanup; both rendered views and the matching process producer flow were screenshot-reviewed with the installed VS Code Mermaid Markdown bundle. Focused dispatcher tests still lack a generic throwing-task case; public synchronization-point tests do not assert exact IMMEDIATE-before-return timing. The callback-exception prose distinguishes the process recipient-side post-response dispatch path from the producer's Send Interaction stack. The process TSO sequence now branches when a recipient callback throws: the task is consumed, its private completion ACK is skipped, and the coordinator's inTransit entry remains visible to time-bound/grant and save-readiness calculations until completion or recipient cleanup. This is source-derived; no focused throw/recovery test exists. No 2010 or embedded parity is inferred. Verify local-only changes in GitHub after publication A bounded HLA-first cross-guide survey on 2026-10-11 compared this guide with the time-management and cross-cutting state/event/observation guides. No new map is warranted: service acceptance/return, callback submission and eligibility, callback entry/return, application observation, and service-specific state/completion boundaries are already separately explained and cross-linked. Existing source/test examples remain path-specific; this does not establish universal ordering or process/2010 parity. The 2026-10-14 full local visual audit rendered all 12 current guide diagrams. Shortened labels in the dense process synchronization registration/achievement sequence; it still separates private correlated responses, recipient events, push/poll paths, and callback/report order. All 12 charts rendered after revision. GitHub review remains pending publication; no build/tests/CI, Requirements Lab, or 2010 work. |
| 7 | Federation synchronization points | Teach a named synchronization barrier and its federation-level result; keep participation/achievement distinct from logical-time grants, ordinary execution membership, and callback observation. Preserve omitted versus explicit set behavior, late joins, resignation completion, and in-flight save/restore as separate cases. | Added a compact HLA/API comparison of synchronization, time-management, and membership service paths with links to the time and lifecycle guides; no extra diagram was warranted because it would repeat the portable barrier model and existing flows. Five baseline diagrams were parsed with the VS Code Mermaid bundle and screenshot-reviewed in local Chrome. The late-join state chart and sixth, source-derived partial-result sequence were also rendered and screenshot-reviewed locally on 2026-10-11. The sequence shows A's false report retained through C's join and makes the larger three-report fan-in visible; the direct and public tests join participants before any achievement, so this interleaving remains source-derived rather than test-observed or a standard-mandated late-join rule. Multi-member partial restore is also source-derived; public tests restore one-member unachieved points and the codec test only round-trips a false result. Preserve the separate 2010 stream |
| 8 | Service invocation reporting | Per-member reporting/subscription interlock, interaction-vs-file routing, DDM recipient selection, serial reservation, callback ordering, and report-file lifetime. | The current guide has eleven Mermaid diagram blocks. The 2026-10-14 eight-chart full-review set corresponds to the eight pre-expansion diagrams; a diagram comparison finds seven unchanged, while the report-destination chart now includes a callback-time-recheck label added after that review. The ordinary Send Interaction wrapper-specific failure chart received a separate local visual review on 2026-10-16. The configured-process regional-send chart is explicitly unrendered, and the configured-process recipient-snapshot/callback-boundary chart has no per-chart review record. Together with the revised report-destination chart, these are the three reporting charts still pending; regional-send was already queued, and the other two are newly itemized. The reconciliation is complete, but guide-wide visual review is not. The guide remains source/test-bounded: process tests cover successful ordinary send and selected regional public-preflight failures, not server rejection; timestamped process send-report behavior remains untested. The injected writer test’s last recorded run failed during federation creation before target assertions. GitHub rendering remains pending publication. No build, tests, CI, Requirements Lab, or 2010 behavior work. |
| 9 | FOM module admission and composition | Per-module DIF validation, ordered Annex C merge, whole-model reference resolution, FDD materialization, and prepare-before-Create plus registry-atomic additional-module Join boundaries. | The guide's diagrams cover Create, composition, additional-module Join, the edition boundary, post-Join catalog/runtime/MOM observations, existing-instance ownership, known-receiver subscription/discovery, and later interaction paths. The two new source-bounded charts distinguish catalog growth from owner-map state on pre-existing instances, then trace the known receiver's separate subscription/discovery path and the unresolved initial-value reflection question. A promoted public C++ TCK case rejects selected invalid modules without admitting the member/extension class, accepts a matching `TckObject.Value` duplicate, rejects both the `TckObject.Value` attribute-order conflict and the `HLAinteractionRoot.TckInteraction` order conflict without admitting the member, and rejects a `TckDuplicateDimension` upper-bound change from 100 to 101 while retaining 100; the scenario succeeds on a valid retry under both callback models. Seven standard-invalid fixtures validate against the pinned 1516.2-2025 DIF XSD; this is not exhaustive coverage. A separate focused embedded Join/ownership case adds an attribute to an existing class, verifies discovery of the pre-existing object, and observes ownership only after acquisition notification; it sends no update and asserts no receiver reflection. The embedded and configured-process routes remain distinct. The value-only reflection and normative §5.8 outcomes remain open. Both new diagrams have local Mermaid CLI 12.0.0 render and visual-review evidence; no process-parity claim, build/test/CI, or Requirements Lab work. |
| 10 | Joined-federate MOM object lifecycle | Private RTI snapshot versus public discovery/reflection, Join-scoped values, save/restore `HLAfederateState`, and pending resign-removal lifetime. | Three baseline diagrams were visually inspected on GitHub (2026-10-09); the Join flow is tall but readable and `HLAfederateState` compact. The resignation chart's stray states and the subscription/discovery sequence's parse failure were local semicolon repairs. On 2026-10-14, all six diagrams rendered with Mermaid CLI 12.0.0; both repaired charts were visually inspected and are legible without clipping. The resignation chart separates pending removals and snapshot retention; the sequence separates Join, observer discovery/reflection, explicit value request, and resign removal. Local revisions still await GitHub rerender after publication. The guide keeps member-level `HLAfederate` distinct from federation-level membership, process evidence separate, and the 2010 RTI boundary explicit. No builds, tests, CI, or Requirements Lab. |
| 11 | Exception reporting | Separate the caller's typed service return/exception from observer-side `receiveInteraction` reports; distinguish `HLAreportException`, `HLAreportMOMexception`, and `HLAreportServiceInvocation`, including switch gates, subscriber/DDM selection, process rechecks, and payload projection. | Six Mermaid diagrams render locally with Mermaid CLI 12.0.0 and were visually inspected on 2026-10-14. Split the Route A sequence into separate embedded-registry and process-endpoint traces because they are distinct routing paths; shorter lane labels improve legibility. Keep the caller's typed exception distinct from HLAreportException observer delivery, HLAreportMOMexception, and HLAreportServiceInvocation. The combined process two-report trace remains source-derived: its focused cases each enable only one switch and do not test the combined configuration, report arrival order, or the second-report failure outcome. The 2026-10-09 GitHub audit covers only baseline charts; verify the current six-chart revision after publication. No builds, tests, CI, 2010, or Requirements Lab work. The 2026-10-14 full local render/visual audit covered all six current charts. Shortened crowded messages and lane names in four sequence diagrams while keeping exact protocol names and caller/API/hook/client/server/observer roles clear. The final render succeeded; the combined two-report path remains explicitly source-derived, not a tested arrival-order claim. GitHub review awaits publication; no build/tests/CI, Requirements Lab, or 2010 behavior work. |
| 12 | TSO message retraction | Producer Retract legality, per-recipient pending/delivered/suppressed phases, Request Retraction, mixed fan-out, and timestamped deletion. | All 10 Mermaid diagrams render locally with Mermaid CLI 12.0.0. All 10 have local visual review as of 2026-10-14. The latest full-guide render visually inspected the timestamped-delete restoration chart: callback-entered restoration/filter/retraction and no-callback reservation-cancel paths are legible without clipping. The preceding review inspected per-recipient state, mixed-recipient walkthrough, focused trace, and receiver-Resign route, and refreshed the ownership timeline. The ownership map is now top-down; regional labels were shortened and adjacent-note/semicolon syntax corrected without changing behavior or evidence. The timestamped-delete chart retains its 2026-10-09 GitHub visual review; the per-recipient chart was also refreshed locally after revision. The mixed-recipient walkthrough is one embedded 2025 HLA_EVOKED interaction case and does not establish process parity or callback invocation at send; no test was rerun. The portable outcome diagram is orientation only, not a complete §§8.22–8.23 decision table. Infer no normative equivalence, process parity, or 2010 behavior. Local review does not verify GitHub rendering of later revisions; that check remains pending publication. No builds, tests, CI, or Requirements Lab changes. |
| 13 | Object-instance name reservation | Single and bulk reservation callbacks, partial reserve versus atomic release, owner-only named registration, and the distinction between a reservation claim, a live instance, and peer discovery; also resignation and restored reservation state. | Freshly rendered and visually inspected all seven diagrams on 2026-10-14 with Mermaid CLI 12.0.0. Shortened reservation-lifetime edge labels after preview showed they stretched the chart; failure, release, resignation-before-registration, named-registration, global deletion, and save/restore transitions remain distinct. The owner/peer map separates reservation consumption from later discovery. Its two focused embedded cases are complementary, not one end-to-end test. HLA-level void request versus callback outcomes remain distinct from Umbra routing. Registered-to-Available is global object/name purge, not local forgetting; the local-delete case uses an unnamed object, so named-name retention is source-derived. Bulk-reserve per-name outcomes do not prescribe a registry lock; rollback is embedded-only. Current revisions need GitHub review after publication. No C++ checks, 2010 behavior, or Requirements Lab work. |
| 14 | Order and transportation controls | Keep RO/TSO order separate from transport type; trace class defaults, per-instance overrides, callback-gated transportation changes, queries, and what receivers observe. | The four baseline diagrams were visually inspected on GitHub (2026-10-09). A targeted local render of all six Markdown diagrams with Mermaid CLI 12.0.0 caught and fixed a semicolon parse error in the transportation pending-state chart; the revised chart is now visually inspected and readable. The source-level write-before-callback ordering is marked embedded-only; confirmation remains the federate-visible boundary, with no portable or normative timing claim. It separates admission/rejection, pending state, dispatch-time revalidation and commit-before-confirmation, stale/cancelled discard, and pending save/restore. Prose now distinguishes RTI commit from public callback observation and notes HLA_IMMEDIATE inline versus HLA_EVOKED dispatch; both callback-model restore tests are linked. GitHub re-render remains pending until publication, and full-guide/whole-tree rendering remains incomplete. The §6.21 exact normative entitlement question remains open pending authorized clause text. No builds, tests, CI, 2010 changes, or Requirements Lab. |
| 15 | Declaration and attribute relevance advisories | Distinguish class-level publisher notifications from per-instance owner notifications; explain active/passive subscriptions, switches, class hierarchy, regional boundaries, known-object state, and update-rate reissues. | All four baseline parse-clean diagrams were individually opened and visually reviewed on GitHub (2026-10-09): the combined subscription-flow chart is balanced, the declaration-relevance state chart is compact, and the switch-off decision chart is tall but readable while scrolling. The per-instance state chart has crowded self-loop labels; its redundant no-change/no-advisory self-loop is removed locally, retaining the real update-rate reissue edge. GitHub rerender is pending. A 2026-10-11 bounded cross-guide survey found no synthesis gap: the object/interaction guide separates producer declarations, receiver selection, object knowledge, and observation; the subscription guide treats interest as input to distinct outcomes; the relevance guide shows one subscription mutation feeding separate class, owner, scope, discovery, and later value paths; and the scope guide distinguishes four signals by recipient and implication. A 2026-10-12 HLA-first audit added bounded owner-switch evidence: selected embedded tests suppress On/Off callbacks during disabled-switch subscription changes, but do not determine whether an already-relevant state is replayed after re-enabling; the declaration-switch no-backlog result remains scoped to its own family. No new diagram. Existing source/test evidence is profile-bounded. 2026-10-14 owner-switch audit: turning the switch on does not invoke the 2025 relevance planner, but callbacks planned/admitted earlier follow profile-specific delivery-time gates. The embedded test drains while off then tests a fresh post-enable transition; process callback tests enable the switch in setup and do not cover an off/on pending-work case. Added this source-observed distinction to the guide; no standard or 2010 claim. 2026-10-14 process class-level audit: process wire protocol provides switch GETs; public object-/interaction-switch Set methods still use the embedded registry path; the configured-process HLAsetSwitches test changes values and reads getters, not callback delivery or direct Set calls. Process declaration handlers do not call the class-level planner, and no process class-level advisory event route or callback test was found. The guide now marks the conceptual class-level branch embedded-only and records the process source gap. Source observation only; no standard rule or 2010/parity claim. 2026-10-14 process member-level follow-up: HLAsetSwitches stores object-class and interaction-relevance values in member state; getters read them back, while the HLAfederate current-value projection is a separate observation route. The MIM marks these fields Conditional, but the inspected process handler enqueues automatic conditional work only when exceptionReporting is supplied; the focused process test checks getter readback, not observer reflection. The Federation MOM guide documents this source-observed gap, and the cross-cutting map now distinguishes embedded direct-setter reflection from the process MOM route. This is not a conformance or parity claim. No builds, tests, CI, Requirements Lab, or 2010 comparison. |
| 16 | Attribute Scope Advisory | Model receiver-local per-object-attribute scope, region and subscription transitions, the self-final-unsubscribe exception, receiver switch, grouping, and stale callback rechecks; keep it separate from owner relevance and data delivery. | The three pre-existing diagrams render on GitHub at commit `5392bde4f41e7da33a03835ff16a78c663d62022`; the state chart and mutation-to-callback sequence were visually reviewed on 2026-10-09, and the repaired decision flow on 2026-10-11. The fourth process-event transport chart was rendered with Mermaid CLI 12.0.0 and screenshot-reviewed locally on 2026-11-03; both queue and pushed-event lanes are readable. The audit distinguishes the server polling queue's dequeue-time registry recheck from pushed events' pre-send check; the client dispatch path shows no later registry scope check. The public process test configures polling for HLA_EVOKED and pushed events for HLA_IMMEDIATE, but tests ordinary transitions, not the stale race. Treat the possible after-check race as untested implementation behavior, not an HLA cutoff. The embedded HLA_EVOKED stale-In case and HLA_IMMEDIATE ordering remain separately scoped. The embedded switch-off test coincides with self-final-unsubscribe, so switch suppression for an otherwise-reportable transition is source-only; the process test keeps the switch enabled. |
| 17 | Auto Provide | Trace the federation-wide FDD/MOM switch, discovery-triggered provider grouping, mandatory empty tag, regional eligibility, provider response separation, and callback-time switch/scope fences; call out the unverified process automatic path. | All three current diagrams were rendered and visually reviewed in VS Code Markdown Preview (2026-10-11). The switch chart now shows initialization and switch transitions, with discovery effects in an adjacent table; the sequence shows initial reflection as separate embedded data delivery before Auto Provide planning. The initial-value test covers reflection separately, not the combined order. The wide sequence and tall eligibility flow are readable while scrolling. GitHub rendering remains pending after publication; no process or 2010 behavior is inferred. |
| 18 | Directed interactions | Separate sender, target object, and receiver identities; explain object-class/interaction-class publication, universal versus by-ownership selection, receiver fan-out, callback-time rechecks, and the TSO/retraction boundary. | Two baseline diagrams were GitHub-rendered and visually reviewed (2026-10-09); the declaration selector and tall timestamped-routing flow are legible when expanded/scrolled. The locally repaired receive-order sequence rendered and was visually reviewed; verify GitHub after publication. The implementation-scoped unpublish-state and queued-callback-fence diagrams now both parse and are visually reviewed locally; the former is tall but clear when scrolled. Verify GitHub after publication |
| 19 | Delay Subscription Evaluation | Explain that DSE changes when receiver subscription/relevance is evaluated, not whether delivery is guaranteed. Keep sender-service validity, receiver eligibility and service-specific region/selector predicates, receive-order callback dispatch, TSO grant eligibility, and current-state rechecks distinct; show Umbra's route-only work item only as a labeled implementation case. | The guide leads with the HLA timing distinction and labels the §8.1.8 summary as a conservative paraphrase pending an authorized copy. Umbra's route-only candidate remains explicitly implementation-specific. A bounded 2010 reference-profile scan found no DSE-specific symbol/case; this is not a claim about IEEE 1516.1-2010 normative behavior or a full RTI. All three original diagrams were rendered and visually reviewed locally. The 2026-10-12 audit clarified that the DSE TSO grant lane is the time-constrained recipient case and that embedded focused TSO tests cover time-constrained recipients. A 2026-10-15 source re-audit corrected process attribute receive-order context handling. The 2026-10-16 test-tag audit found configured-process regional attribute cases for HLA_EVOKED receive-fence and HLA_IMMEDIATE callback-task-admission rechecks, plus one callback/Resign fence case. Nonregional receive-order push remains on the existing pre-event path pending a separate DSE slice and has no focused DSE test. The revised process attribute chart was rendered with Mermaid CLI 12.0.0 and screenshot-reviewed locally on 2026-11-03; its three timing lanes are readable. Verify GitHub after publication and do not infer cross-edition or process parity |
| 20 | DataElement encoding | Trace explicit scalar byte order, composite counts and alignment, discriminant-selected variants, cursor-based decoding, and the logical-time factory boundary; distinguish public value encoding from handle encoding, FOM/OMT input, and transport framing; keep edition-specific codec layers separate. | A bounded audit found that ordinary `HLAvariantRecord` and `HLAextendableVariantRecord` needed a clearer unknown-discriminant comparison. The revised chart shows discriminant-only ordinary handling versus the extendable record's signed length field, fixed eight-octet alternative boundary, and ability to skip a length-delimited unknown payload; this is current 2025 source/test evidence, not a normative-parity claim. The 2025 guide's eight Mermaid diagrams were rendered with Mermaid CLI 12.0.0 and screenshot-reviewed locally on 2026-11-03. The scalar, representation-boundary, composite, variant, and logical-time flows are readable; the edition-boundary sketch remains a separation note. This review clears the previously pending composite, logical-time, and revised variant chart views. Local rendering does not verify GitHub rendering. |
| 21 | Names and handle resolution | Explain distinct typed handle categories, model-name and active federate-name lookups, inherited attribute/parameter resolution, per-federate object-instance knowledge, handle encode/decode versus hashes, and 2025 support-service normalization versus execution-scoped coordinates. | All eight diagrams have local render coverage. The new sequence records the portable 2025 TCK's bounded checks: 15 representative cross-category encodings match across two federates and are pairwise distinct in one execution; 11 user-defined FDD-name encodings repeat across two executions using the same RTI and FDD path. OrderType remains a separate enum assertion, and the normalized-coordinate test still does not establish cross-category uniqueness. The prior flows distinguish model declarations, active federate identity, member-scoped class lookup, federate-local instance knowledge, opaque typed handles, encoded values, and normalization coordinates. Tall and parallel charts remain readable while scrolling. The additional-module Join sequence remains one embedded test pattern, not process/2010 parity. The post-resignation identity asymmetry remains labeled as one process-test observation. GitHub rendering remains pending after publication |
| 22 | Connect, Disconnect, and configuration | Separate factory object construction from Connect-time RID authorization-profile selection, ambassador connection from federation membership, and embedded versus process address selection; show overload/precondition/auth gates, optional-setting outcomes, result reporting, and post-resign teardown. | The three established diagrams were rendered and visually reviewed in the installed VS Code Mermaid renderer. The revised 2025 Connect-ordering chart has since been locally rendered and screenshot-reviewed in the official Mermaid Live Editor. The fourth factory/RID sequence was rendered individually and visually reviewed with Mermaid CLI 12.0.0 on 2026-10-14. The local preview showed that full participant labels and the HLAauthorizer branch stretched the sequence; shortening display labels improved readability without changing branches or timing. Its focused test last failed while creating the temporary RID file before assertions and was not rerun; the sequence remains source-grounded, not passing-test evidence. The absent-connection `NotConnected` behavior remains flagged against the pinned API comment for normative review. Verify local revisions on GitHub after publication |
| 23 | Federation-scoped MOM | Distinguish the execution-scoped `HLAmanager.HLAfederation` object and its conditional values from each member's `HLAfederate` object; separate object reflections from federation request/report interactions, switch scopes, and mark the embedded 2025 test boundary. | The four existing diagrams rendered through the VS Code Mermaid bundle and were visually inspected in a loopback preview; the first state-chart layout was shortened after review. The former fifth switch-scope chart is now two diagrams because federation-wide Create-time capture and per-member Join-time seeding have distinct owners and lifetimes, and map to two separate focused test bodies. Both test executables built, but the tests still fail at `createFederationExecution` with `Unknown exception` before assertions; both flows remain source-derived, not runtime-verified. Both revised views rendered and were screenshot-reviewed locally. A seventh, HLA-first scope map now relates the MIM's execution and member projections to their distinct observer paths; bounded embedded tests observe membership-list reflection and member-object removal separately, while a process case orders member removal before its rediscovery/reflection only. No cross-path order or profile parity is asserted. A follow-up audit clarifies that the list's `FederateHandle` reference is identity, not the receiver-facing `ObjectInstanceHandle` or proof of local object knowledge; the existing focused cases do not test member-object lookup from the list. A second audit clarifies late-observer handling: request the current membership value after discovery rather than assuming its own Join's conditional update will be replayed; the focused test uses observer-first ordering and does not test a late observer, so no universal no-replay rule is claimed. The targeted local Mermaid render covered the guide's diagrams, including the late-observer flow; that flow was not one of the nine charts individually visually reviewed in this pass. Do not infer general switch mutability, backend parity, or 2010 behavior. Verify changed diagrams in GitHub after publication |
| 24 | Ownership persistence and resignation | Explain the boundary between HLA-visible membership/attribute ownership and Umbra's durable pending-work ledger/live callback routes; separate the resign service's void-or-typed-exception result from the selected action's object/ownership effects, the reject-versus-cancel fork, and surviving recipients across restore. | Three diagrams rendered through the VS Code-bundled Mermaid renderer in a loopback Edge preview and visually inspected. A bounded reader review now labels the ledger/route table as an Umbra implementation model, links the pinned `resignFederationExecution` contract, and confines the pending-acquisition rejection/retry example to its focused embedded test. The broader `ResignAction` table stays in the lifecycle guide; no duplicate action diagram was added. Claims remain limited to selected embedded 2025 development-profile tests; verify GitHub after publication |
| 25 | Attribute update-rate admission | Resolve active ordinary/regional designators per receiver and attribute, then show best-effort wall-clock admission at receive-order and TSO callback boundaries without merging the logical-time state machine. | All four charts now pass the Mermaid 12.0.0 whole-tree syntax pass (2026-10-16). The three pre-existing 2025-only diagrams were parsed and visually inspected locally; the admission chart is tall but readable when scrolled. The fourth subscription-generation chart was rendered and screenshot-reviewed with Mermaid CLI 12.0.0; its tall layout remains readable while scrolling. Process-profile parity and full normative rate semantics are not claimed |
| 26 | Attribute-value update requests | Distinguish object-instance, class expansion, regional routing, provider callback versus later value response, and the different pending-request/RO-delivered/TSO-queued restore cut points. | Four existing 2025-only diagrams based on API, source, and focused tests parsed and were visually inspected locally. The scope-planning view was split into expansion and per-group filtering to untangle routing loops. A bounded HLA-first review makes explicit that embedded request planning does not itself create a subscription or special reflection recipient; the embedded baseline calls public Subscribe, while the process fixture seeds subscription state directly in the registry. Neither tests an unsubscribed requester, so direct response entitlement for an otherwise ineligible receiver remains an unverified normative question. A follow-on fan-out audit found that focused class tests show multiple instances routed to one remote provider, not one object's attributes split across multiple remote providers; that branch remains source-derived and untested. A new fifth 2025 flow distinguishes embedded regional provider solicitation from the later update's ordinary per-receiver DDM, without transferring request-region state or claiming normative/process parity; Mermaid CLI 12.0.0 rendered it, and local screenshot review found the tall layout readable with clear, uncrossed branches. This confirms syntax and layout only, not normative completeness or process/embedded parity. Exhaustive overload coverage is not claimed. A bounded 2026-10-14 HLA-first audit compared the §6.21 API declaration, embedded provider planning, ordinary receiver planning, and focused 2025 tests. The vendored header contains the operation signature and exception list, not clause prose; the official IEEE listing says access is by purchase or subscription. Embedded request/response examples subscribe the requester; the configured-process object-request fixture seeds subscription directly in the registry and later values travel through the ordinary update path. The process class-request test ends after provider callbacks and does not send a later value update. The ordinary receiver planner separately applies subscription and regional eligibility. No focused case tests a provider response to an unsubscribed requester, so the guide correctly leaves that normative question open. Added an IEEE access reference only; no diagram or behavior claim changed. No builds, tests, CI, Requirements Lab, or 2010 comparison. |
| 27 | Object-attribute subscription state | Explain per-attribute additive ordinary declarations, active/passive coexistence, discovery versus value eligibility, separate regional pair state, and how a late interest change affects discovery, initial values, scope, provider advice, and Auto Provide. | The original embedded diagrams and revised regional subscribe/unsubscribe chart were visually reviewed locally. The HLA-first reader-interest map rendered and was visually reviewed in VS Code Markdown Preview (2026-10-11); it shows subscription interest as one input to separate discovery and per-attribute value gates, then leaves later routing/delivery gates explicit. Its labels are readable at the available preview width; it does not claim a full algorithm or callback ordering. On 2026-10-14, Mermaid CLI 12.0.0 rendered all eight diagrams; local visual review covered the source-derived passive-recipient chart, HLA-first late-interest effects chart, and embedded regional evidence sequence. All three are legible in the expanded preview, with no source edits warranted. The embedded/process late-subscription comparison was also visually inspected; the profile lanes and separate MOM-only branch are legible, and the ordinary process gap remains clearly unresolved. No chart edit or normative inference was warranted. The focused passive test covers passive-only suppression and a passive-to-active transition, not a separate active establisher plus passive receiver. The embedded regional-routing test shows the latest accepted value reflected with discovery. Process source has no ordinary initial-reflection planner callsite in the bounded scan; existing process tests do not resolve that late-discovery edge. Treat this as an open source/test/normative question, not a conformance finding. The empty-region/update-rate generation inference and §9.8.4 author-comment limit remain unchanged. No 2010 or broad process-parity claim. A bounded 2026-10-12 audit found the known-object flow used one scope-change decision for both the receiver scope-advisory gate and the unresolved new-attribute value question, which made independent effects look mutually exclusive. The chart now evaluates them separately from a shared scope delta. Existing tests still do not settle ordinary current-value reflection for a newly added attribute; process MOM values remain a distinct exception. No chart count change. On 2026-10-14, the revised known-object scope-change diagram was rendered individually with Mermaid CLI 12.0.0 and visually reviewed. Its tall layout remains readable; the receiver scope-advisory gate and newly added subscribed-attribute value question branch independently from the per-attribute scope delta. It retains the embedded post-discovery initial-value/Auto Provide path separately and does not fill the unresolved ordinary current-value result. The process MOM-only exception and absence of an ordinary initial-value event on the inspected process route remain separate. No new transition or normative, process-parity, or 2010 claim was added. |
| 28 | 2025 authorization and credentials | Teach authorization as an operation-scoped admission gate, distinct from service preconditions, state effects, and caller-visible outcomes; explain the typed credential envelope without generalizing Umbra's reference policy. | The guide has four diagrams: the portable HLA/API admission flow, a credential-envelope flow, and two Umbra case studies for Connect and connected operations. All four were rendered locally with Mermaid CLI 12.0.0 on 2026-10-11; the credential-envelope diagram was also screenshot-reviewed and its labels and paths were legible. GitHub rendering remains pending. A previously recorded focused CTest run passed 7/10; the RID fixture and Destroy/Join cases failed in setup before target assertions, and tests were not rerun for this documentation review. Both no-Credentials Connect overloads declare Unauthorized but omit InvalidCredentials, while Umbra's shared mapper can map INVALID_CREDENTIALS to InvalidCredentials; the full §4.2 question remains open. The process-operation gate is source-observed, with no process authorization integration test. No 2010 equivalence is claimed. |
| 29 | Process endpoint request and callback flow | Explain private request/response correlation, pushed event frames versus explicit receive polling, callback-model timing, and the per-recipient TSO callback-ack boundary without conflating transport request identity with HLA time. | On 2026-10-14, Mermaid CLI 12.0.0 rendered all 11 diagrams and each was visually reviewed. Reorganized sequences at real ownership boundaries: sender request versus recipient stream; event acquisition versus callback bridge; receive-order attribute planning/commit versus recipient delivery; TSO callback outcome versus the private completion ACK. The HLA/API-to-process map and polling-family selector use top-down layout. These are local previews; GitHub review of this revision remains pending publication. Process details remain source-derived, not HLA ordering guarantees: public void results, private recipientCount, requestId, TSO messageId, public MessageRetractionHandle, and LogicalTime remain distinct. A throwing timestamped callback consumes its task, skips the private ACK, and leaves the coordinator recipient inTransit; that entry can constrain time bounds/grant advancement and timed-save readiness. Successful Resign clears recipient TSO phases and event buffers but leaves the client and callback bridge connected; Disconnect follows Resign. handleResign removes producer-owned routes, while cached pushed-recipient sessions are pruned only on service detach; handleRetract drains that cache separately from the registry's current-member notification list. A stale post-Resign request_retraction frame is a source-level risk only, not verified runtime or normative behavior. No focused test covers callback throw → receiver Resign → producer Retract or disconnect-to-detach timing. The 2025 process value-boundary review now states explicitly that ordinary untimed attribute payload octets pass through process planning/events and are rebuilt into `VariableLengthData` without RTI-side `DataElement` decoding; the raw-byte test checks preservation, not typed decoding or general validation. Other endpoint CTest failures remain environment-limited as recorded in the guide. No builds, tests, CI, or Requirements Lab were run for this review; no 2010 behavior is inferred. |
| Separate 2010 profile | Bounded reference RTI service slice | Federation membership, discovery/update/interaction, synchronization barrier, provider-directory handle lookup, callback dispatch/failure boundaries, and explicit unsupported-flow map for the 2010 shell only. | Three of four baseline diagrams parsed and were individually visually reviewed on GitHub (2026-10-09). The membership chart is broad but zoom-readable; the interaction-routing chart was too wide at default preview scale and is now top-down locally; the synchronization barrier is tall but legible while scrolling. The discovery/update sequence failed baseline parsing at semicolon-separated message text; its local comma repair and the interaction layout revision await GitHub rerender. The fifth source-reviewed 2010-only chart distinguishes value/factory smoke coverage from unavailable ambassador time services. The sixth source/test-backed 2010-only sequence shows accepted-but-unloaded FOM paths and lazy provider-directory class handles. A seventh source-only sequence makes the synchronization completion-callback failure/retry edge visible; no focused 2010 barrier test was found or run. The eighth, source-derived chart explains inline callback dispatch, service-local exception conversion, and the unavailable evoke path; callback-error and HLA_EVOKED tests are absent. The separate 2010 DataElement guide adds two source/test-backed, locally reviewed diagrams. These guides make no 2010 time-advancement or standard FOM-loading claim. Keep both 2010 guides and their evidence separate from 2025 behavior |

On 2026-10-15, a bounded audit of the 2025 Names and Handle Resolution guide
rechecked the model lookup, active-versus-known federate identity, inherited
class-member resolution, per-federate object knowledge, typed handle encoding,
and normalization evidence. All 54 local source/test links and line anchors
resolve against the current tree. It found one reader ambiguity: the shared
`unsigned long` return type of the four typed normalization services could be
mistaken for a global handle-identity space. The guide now describes values as
category/dimension-scoped coordinates and cautions against cross-category
numeric identity comparisons, with the relevant source and test limits. No
diagram changed, so its seven chart render reviews remain applicable; the
separate 2010 stream was not touched. The next bounded audit covers 2025
Connect, Disconnect, and configuration.

On 2026-10-16, the Connect/Disconnect reader audit corrected a stale process
cleanup claim. The 2025 `ProcessFederationServiceHost` implementation calls
`connectionLost` and then `detach` after its serving loop exits; client
`Disconnect` only closes its local socket and does not wait for that remote
worker. The public lifecycle fixture manually calls `detach`, while the host
integration test disconnects clients before host shutdown but does not assert
server-session cleanup state or timing. The repository search found host
construction only in tests. Updated the existing cleanup chart and kept the
production-host implementation distinct from a non-test server launch path.
The revised chart now needs a fresh render/visual review; the 300-chart count is
unchanged. The 2010 stream remains separate.

On 2026-10-16, the bounded Federation and Federate Lifecycle audit corrected a
stale process-endpoint claim: the host worker calls `connectionLost(session)`
after its serving loop exits (and attempts cleanup on exceptions), then calls
`detach` and closes the connection. The session-loss operation removes joined
membership and dispatches eligible survivor effects; `detach` clears service
routing. This is implemented source behavior, but this checkout has no
non-test host construction site, and the host integration tests do not exercise
loss of a joined session or assert cleanup timing. The separate current-process
fault fixture remains manually driven. Clarified that the final-member loss
test reconnects through the same RTI ambassador object but starts a fresh
connection and member lifetime; it does not resume the old session. Revised
the existing process-loss chart and evidence mapping. Fresh visual review is
pending; 2010 remains separate, and no build, test, CI, or Requirements Lab
work was done.

The 2026-10-16 DSE reader audit found one specific process-path omission. In
the process attribute service, pull delivery and regional receive-order push
use the receiving-boundary recheck, while nonregional receive-order push stays
on its existing pre-event route pending a separate DSE slice. An unprojected
candidate on that route is re-resolved before event construction; it is not
covered by the receiving-boundary path. The source comment and branch now have
an explicit note and label in the existing chart. A configured-process test
covers regional receive-order attributes in HLA_EVOKED and HLA_IMMEDIATE
modes, but no focused test covers nonregional push. The chart was revised, not
added; fresh syntax/render and visual review remain pending. No 2010 behavior
or process conformance is inferred, and no C++, build, test, CI, or
Requirements Lab work was done.

This order is a working prioritization, not a statement that one area is more
normative or more complete than another. At the outset of this documentation
effort, a bounded 2025 survey found dedicated runtime and focused test families
for ownership, DDM/regions, save/restore, information flow, and federation
membership, but few cohesive contributor flow guides beyond timing. A focused
synchronization-point survey then identified
participant-set expansion, achievement fan-in, resignation, callback, and
state-image boundaries suitable for their own 2025 guide. A bounded
service-report survey found an existing implementation dossier and focused
tests for the report-subscription interlock, file sink, interaction sink, and
callback order, but no learner-facing flow guide; this guide narrows itself to
invocation reports rather than the whole MOM. The FOM-admission survey found
an extensive validation design and focused tests, but no compact flow guide
for the distinction between per-module DIF checks, complete-set composition,
FDD validation, and transactional Create/additional-Join behavior. The joined-
federate MOM survey found public-lifecycle and save/restore evidence for the
RTI-owned `HLAfederate` instance, but no learner-facing account of its private
snapshot, subscriber-visible callbacks, conditional state values, and
callback-time removal lifetime. A separate federation-scoped MOM survey found
the existing service-reporting design dossier and focused membership,
current-FDD/FOM-module, and synchronization-report tests, while the joined-
federate guide explicitly left the execution-scoped `HLAfederation` projection
out of scope. The new companion keeps its object/reflection lifecycle distinct
from federation request/report interactions and limits implementation claims
to the tested embedded development profile. The core ownership guide explicitly
left persistence and resignation out of scope, so a follow-up survey traced
typed ownership-image capture/restore, the pending-acquisition resignation
guard, and focused fresh-registry and cancellation tests. That evidence supports
a companion on selected pending release work, candidate pruning, and the
reject-versus-cancel fork; it does not imply every ownership operation is
restartable. An exception-report survey then found separate
focused routes and tests for `HLAreportException` and `HLAreportMOMexception`,
plus an existing design note documenting an inconsistency in the vendored 2025
MIM wording; the new guide makes the reporting-switch and subscription rules,
process callback rechecks, payload projection, and known source ambiguity
explicit without editing the Requirements Lab. The separate 2010 survey found a narrower but
real in-process reference service slice: the binding-shell smoke exercises
membership, object information, and interaction delivery, while synchronization
has source but no focused test in the surveyed family. The 2010 guide documents
only that slice and its known absences; it does not infer parity from the 2025
guides. A follow-up compared the generated 2010 time-service overrides and
timestamped overloads with the eight test files whose names identify the 2010
stream. The reference guide now lists all 17 unavailable time-management
overrides, the unavailable timestamped update/send/delete/region-send/save
overloads, and the ignored create-time logical-time implementation name. The
bounded test search found no ambassador time-service calls in those files;
their time smoke cases exercise value/factory/marshalling operations only.
This remains a separate 2010 support-boundary explanation, not an imported
2025 time state machine. A retraction survey found that the timing overview's
queue diagram did not fully explain the producer's strict time/lookahead
boundary, recipient ledger, or the distinction between withdrawing pending
fan-out and requesting reversal after delivery. Focused interaction,
attribute, regional, process, delete, and save/restore tests support a separate
2025 companion deep dive; it links back to the overview instead of restating
all time-management behavior. Test-file-name searches are topic-discovery
signals, not coverage measurements. The object-instance-name survey then found
a distinct state machine not explained by the generic registration flow:
single-name collisions
report asynchronously, multi-name reserve permits a successful subset while
multi-name release validates atomically, and named registration consumes only
the owner's reservation at commit. It also found focused embedded, process,
restore, and restart tests. A test that feeds a 2010 FOM to the 2025 API is
explicitly treated as 2025 behavior with legacy-model input, not evidence about
the separate 2010 reference RTI. The order/transportation survey then found a
separate control plane with synchronous order changes and callback-gated
transportation changes, class defaults captured by later registrations,
per-instance overrides, callback-visible order and transport metadata, and
selected pending-change restore coverage. Existing time and DDM guides explain
the adjacent grant and recipient-selection boundaries, so this guide focuses
on the control decisions rather than duplicating those state machines.

The next advisory survey found a distinct class-level declaration signal and
per-object-attribute relevance signal. A 2025 object subscription can feed both
planners, but callbacks differ in recipient, granularity, region criteria, and
switch behavior. The new guide separates them and links regional details back
to the DDM guide instead of repeating its overlap state machine.

The Attribute Scope Advisory survey found distinct embedded and public
process-endpoint test cases, both covering HLA_EVOKED and HLA_IMMEDIATE. The
new 2025-only guide gives scope its own receiver-centered state machine,
documents the implementation's self-final-unsubscribe exception and
delivery-time stale-work filtering, and links back to the broader DDM and
relevance guides instead of merging their state machines. Keep the 2010
profile separate when repairing or reviewing diagrams, and do not infer its
behavior from 2025. The direct-render status is recorded below.

The Auto Provide survey found a distinct discovery-triggered path beside
explicit Request Attribute Value Update: the 2025 embedded planner groups
currently owned in-scope attributes by provider, supplies an empty tag, and
rechecks switch and scope before provider code. Focused tests cover disabled
discovery-only behavior, regional/reentrant changes, multi-provider fan-out,
MOM switch mutation, and timestamped admission. The process profile exposes
explicit requests and switch mutation, but no automatic discovery planner or
focused process Auto Provide test was found in this bounded survey; the guide
marks that path unverified rather than claiming edition/profile parity.

The directed-interaction survey found a distinct object-context routing model:
the sender supplies a target instance, while the RTI fans out according to
object-class/interaction-class declarations and each receiver's universal or
by-ownership selector. Focused evidence includes embedded selector, known-
target, timestamped delayed-subscription and retraction tests, plus a process
multi-recipient endpoint case. The new guide keeps embedded callback-time
revalidation separate from the process evidence and links timestamp/grant and
retraction state back to their dedicated guides.

The DSE survey found one cross-cutting 2025 state machine across ordinary
interactions, attribute updates, regional traffic, and directed interactions.
The focused embedded tests assert late-subscribe admission only when enabled,
and callback/grant-time unsubscribe suppression in either switch mode. The
guide treats HLA_EVOKED/HLA_IMMEDIATE and TSO grants as separate dispatch
boundaries, notes that the switch is initialized and saved with federation
state, and marks process behavior as source-observed rather than test-proven.

A focused ordinary attribute-update survey found a separate receive-order
delivery boundary: the sender's attributes are validated against its registered
class and ownership, while each receiver's projection is filtered through its
own known class. One API update may form distinct transportation/order/region
passels; queued delivery re-evaluates the receiver for each passel, so an
unsubscribe can suppress it without merging the passels. The embedded test
directly exercises class promotion, reliable/best-effort callbacks, immediate
versus evoked delivery, validation failures, and unsubscribe suppression. The
guide keeps source-derived copy/re-plan, report/counter, and lock-release detail
distinct from that test evidence. The focused update-rate companion now traces
applicable active ordinary/regional designators, the per-attribute steady-clock
gate, and its receive-order/TSO delivery boundaries. It separates this filter
from owner relevance advisories and logical-time grant selection, and labels
the implementation/test scope as embedded 2025 evidence.

A follow-up subscription-state survey found that ordinary declarations
accumulate by selected attribute, active/passive mode affects discovery
separately from attribute projection, and regional attribute/region entries
occupy a distinct declaration map. The new guide bounds these claims to
embedded focused tests and flags the unresolved update-rate-key effect of a
regional subscribe call whose supplied region sets are all empty.

A follow-up interaction-subscription survey compared the passive, selector,
and regional focused tests against the existing object/interaction,
directed-interaction, relevance-advisory, DDM, and DSE guides. Those guides
already separate active class-level relevance from passive recipient matching,
selector choice, class/parameter projection, and regional/time gates. A new
standalone subscription guide would currently repeat those flows, so the next
survey moves back to RTI service families to find any genuinely uncovered
state machine.

A bounded API-heading survey then checked the seven 2025 `RTIambassador`
service groups—Federation, Declaration, Object, Ownership, Time, Data
Distribution Management, and RTI Support—plus callback controls against the
existing reader paths and guide backlog. Each top-level group already has
relevant guide coverage, so this scan did not justify another umbrella guide.
It was an inventory of service headings, not proof that every overload,
callback boundary, or state transition is fully explained; future additions
should come from a specific source- and test-backed gap inside a covered area.
The 2010 reference stream was not folded into this survey.

A cross-guide review found one useful conceptual seam: the FOM admission guide
explains how a model becomes available to an execution, while the object and
interaction guide explains member declarations and runtime instances. Neither
guide's service-level flows should be repeated, but the synthesis guide had not
yet shown the portable distinction between shared model vocabulary and dynamic
publication, subscription, registration, discovery, and delivery facts. Its
fourth diagram now connects those layers and links to the existing guides'
source/test evidence. This adds no new runtime claim or test result; the
service-specific examples and embedded/process limits remain in the linked
guides. The new chart passed the bundled Mermaid grammar but still needs visual
review when the permitted preview is accessible.

The DataElement survey found a bounded value-encoding pipeline relevant to
the original byte-order/serialization concerns: public 2025 scalar and
composite elements, explicit BE/LE selection, cursor-returning nested decode,
composite alignment, variant discriminants, and factory-defined logical time.
One generic byte-order primitive is genuinely shared, while edition-specific
element sources and 2010/2025 composite helpers remain distinct. The new guide
documents the 2025 path only and does not infer 2010 parity or transport-wire
behavior. Its next gate is the same rendered-diagram review as the existing
set. A later Connect/Disconnect survey found a separate local-session state
boundary beside Join/Resign: four public overloads converge on callback-model,
authorization, configuration, resource-preparation, and commit gates, while
Disconnect rejects joined members and releases callback/endpoint resources
after its state transition. Focused tests cover overloads, malformed settings,
credentials, duplicate Connect, callback model, and Disconnect guards. The new
2025-only companion flags a difference between Umbra's absent-connection
`NotConnected` result and the pinned API header's 4.3 exception annotation for
comparison with the full normative clause; it does not turn that observation
into a conformance claim.

A process-endpoint survey found a distinct implementation boundary not covered
by the HLA service-flow guides: one service request can share a session stream
with unsolicited event frames, while recipient events and sender responses use
different sessions. The new 2025-only companion diagrams trace request-ID
correlation, push-versus-poll arrival, event buffering, transaction-lock
release, callback-model dispatch, and the Evoke receive fallback. Source and
test cases exist at transport, private-client, and public-ambassador layers,
but the current local focused run was blocked before callback assertions or by
localhost socket restrictions. Keep the guide implementation-scoped; do not
infer 2010 behavior or reinterpret request IDs as HLA logical time.

### 2025 federation/federate lifecycle reader review — 2026-10-11

A bounded HLA-first review found the lifecycle guide already answers the
handoff's reader questions without another chart. It starts from three
different lifetimes—ambassador connection, federate membership, and
federation execution—then compares successful voluntary Resign and forced
connection loss for both peer and final-member exits. It states that Resign
does not itself Destroy the execution, and marks final-member cleanup,
per-member automatic directives, forced safety cleanup, and connection-loss
callback/report paths as current Umbra evidence. The API declarations supply
the 2025 service/callback vocabulary and clause pointers, but not the full
normative decision table; that limitation remains explicit. The separate 2010
reference-RTI guide is linked without borrowing 2025 behavior. No additional
diagram was justified.

## Completion check for each guide

A guide is ready only when a reader can follow the HLA behavior without first
understanding Umbra's internal class structure:

1. **Start with the portable HLA question.** Name the edition and the service or
   protocol boundary. State what a federate can request, what result the HLA
   contract defines, and which effects or callbacks are relevant. Link
   normative claims to the exact standard section; distinguish API declarations
   from standard prose.
2. **Trace the behavior, not the code layout.** Show the important decision,
   state change, affected participant or recipient, and point when an effect
   becomes observable. Keep accepted requests, committed state, pending work,
   callback eligibility, and callback observation distinct. Do not imply that
   a sequence diagram is a shared call stack or a callback-return guarantee.
3. **Use implementation details only to explain an HLA consequence.** Bring in
   Umbra source when it clarifies a required invariant, observable outcome, or
   profile limit. Put implementation commit points and private structures in a
   clearly labeled case-study lane, not in the portable HLA model.
4. **Label evidence and uncertainty.** Separate standard requirements, pinned
   API behavior, source-observed paths, focused-test observations, and open
   questions. A passing focused test supports only its exercised case and
   profile; it does not establish universal behavior, process/embedded parity,
   or conformance.
5. **Preserve edition and profile boundaries.** Keep IEEE 1516.1-2010 and
   1516.1-2025 behavior in separate guides and diagrams. Compare them only
   where equivalence is demonstrated. Mark 2025 embedded/process differences
   and source-only inferences explicitly.
6. **Make every diagram earn its place.** Use a flowchart for decisions and
   state changes, a sequence for ordering/causality, and a state diagram for
   durable lifecycle phases. Show receiver-specific or per-attribute state
   separately when aggregation would hide distinct outcomes. Do not add maps
   just to increase the count.
7. **Verify the artifact at the right layer.** Parse and visually inspect each
   new or changed Mermaid diagram in the local Markdown preview, then verify
   GitHub rendering after publication. Parser acceptance and fence counts do
   not prove legibility or remote rendering. Prose/diagram changes do not
   require C++ rebuilds, test reruns, or CI by default; reuse existing evidence.
8. **Leave a handoff.** Update the current backlog and roadmap with unresolved
   reader questions, evidence/render gaps, and one concrete next step. Keep
   detailed dated audit history in the separate
   [review ledger](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md); use this index
   for current navigation and priorities.

## Historical reader-gap review summaries

The full dated review history is preserved in the separate
[reader-gap review archive](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md).
The original headings remain below as legacy anchors; each links to its
archived detail. Counts and pending-review states in these entries are
historical snapshots, not current status.

### 2025 provider solicitation vs. relevance notification — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-provider-solicitation-vs-relevance-notification--2026-10-11)

### 2025 subscription-state reader-model review — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-subscription-state-reader-model-review--2026-10-11)

### 2025 DSE and provider-facing value paths — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-dse-and-provider-facing-value-paths--2026-10-11)

### 2025 save/restore Join boundary: authorized-text check — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-saverestore-join-boundary-authorized-text-check--2026-10-11)

### 2025 FOM module admission/composition reader audit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-fom-module-admissioncomposition-reader-audit--2026-10-11)

### 2025 save/restore HLA-question reader audit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-saverestore-hla-question-reader-audit--2026-10-11)

### 2025 joined-federate MOM HLA reader model — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-joined-federate-mom-hla-reader-model--2026-10-11)

### 2025 federation lifecycle: three scopes and MOM representation — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-federation-lifecycle-three-scopes-and-mom-representation--2026-10-11)

### 2025 synchronization API vocabulary — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-synchronization-api-vocabulary--2026-10-11)

### 2010 synchronization boundary: same API name, separate source behavior — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2010-synchronization-boundary-same-api-name-separate-source-behavior--2026-10-11)

### 2025 directed-interaction target versus DDM API boundary — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-directed-interaction-target-versus-ddm-api-boundary--2026-10-11)

### 2025 order, transportation, and callback observation — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-order-transportation-and-callback-observation--2026-10-11)

### 2025 FOM composition: keep API inputs and profile ordering distinct — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-fom-composition-keep-api-inputs-and-profile-ordering-distinct--2026-10-11)

## 2025 TSO retraction reader-boundary audit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-tso-retraction-reader-boundary-audit--2026-10-11)

## 2025 support-switch map disposition — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-support-switch-map-disposition--2026-10-11)

## 2025 save/restore Join boundary disposition — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-saverestore-join-boundary-disposition--2026-10-11)

## 2025 update-rate names, values, and observations — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-update-rate-names-values-and-observations--2026-10-11)

## 2025 attribute-value request: solicitation and response boundary — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-attribute-value-request-solicitation-and-response-boundary--2026-10-11)

## 2025 ownership/request revalidation reader review — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-ownershiprequest-revalidation-reader-review--2026-10-11)

## Attribute-value concepts stitched across the 2025 guide set — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#attribute-value-concepts-stitched-across-the-2025-guide-set--2026-10-11)

## Ordinary versus directed interaction delivery review — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#ordinary-versus-directed-interaction-delivery-review--2026-10-11)

## FOM vocabulary is not runtime declaration — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#fom-vocabulary-is-not-runtime-declaration--2026-10-11)

## Name-and-handle identity scopes — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#name-and-handle-identity-scopes--2026-10-11)

## Reservation ownership, commit, and callback observation — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#reservation-ownership-commit-and-callback-observation--2026-10-11)

## 2025 order and transportation: keep call form, order, and observation separate — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-order-and-transportation-keep-call-form-order-and-observation-separate--2026-10-11)

## 2025 ResignAction intent and membership boundary — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-resignaction-intent-and-membership-boundary--2026-10-11)

## 2010 ResignAction evidence boundary — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2010-resignaction-evidence-boundary--2026-10-11)

## 2025 Declaration-management boundary survey — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-declaration-management-boundary-survey--2026-10-11)

## 2025 Ownership, delete privilege, and receiver-local forgetting — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-ownership-delete-privilege-and-receiver-local-forgetting--2026-10-11)

## 2025 Timestamped delivery and callback-observation boundary — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-timestamped-delivery-and-callback-observation-boundary--2026-10-11)

## 2025 Save/restore ownership and completion boundary — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-saverestore-ownership-and-completion-boundary--2026-10-11)

### 2025 lifecycle observation and state-error vocabulary — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-lifecycle-observation-and-state-error-vocabulary--2026-10-11)

### 2025 producer Retract and recipient observation survey — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-producer-retract-and-recipient-observation-survey--2026-10-11)

### 2025 reservation, registration, and peer-knowledge audit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-reservation-registration-and-peer-knowledge-audit--2026-10-11)

### 2025 attribute update request: solicitation versus delivery audit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-attribute-update-request-solicitation-versus-delivery-audit--2026-10-11)

### 2025 federation synchronization: HLA barrier-path audit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-federation-synchronization-hla-barrier-path-audit--2026-10-11)

### 2025 directed interaction and DDM boundary audit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-directed-interaction-and-ddm-boundary-audit--2026-10-11)

### 2025 declaration and attribute-relevance advisory audit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-declaration-and-attribute-relevance-advisory-audit--2026-10-11)

### 2025 Attribute Scope Advisory cross-guide audit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-attribute-scope-advisory-cross-guide-audit--2026-10-11)

### 2025 Auto Provide: solicitation versus value delivery — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-auto-provide-solicitation-versus-value-delivery--2026-10-11)

### 2025 object-attribute subscription state: interest, knowledge, delivery — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-object-attribute-subscription-state-interest-knowledge-delivery--2026-10-11)

### 2010 object information: profile path and empty-set edge — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2010-object-information-profile-path-and-empty-set-edge--2026-10-11)

### 2010 object state and interaction-event boundary — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2010-object-state-and-interaction-event-boundary--2026-10-11)

### 2010 synchronization-point barrier: roles, failures, and callback boundary — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2010-synchronization-point-barrier-roles-failures-and-callback-boundary--2026-10-11)

### 2025 DataElement byte-stream boundary review — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-dataelement-byte-stream-boundary-review--2026-10-11)

### 2025 names and handles: identity and scope audit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-names-and-handles-identity-and-scope-audit--2026-10-11)

### 2025 authorization: gate versus service outcome audit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-authorization-gate-versus-service-outcome-audit--2026-10-11)

### 2025 Connect ordering: preflight precedence and commit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-connect-ordering-preflight-precedence-and-commit--2026-10-11)

### 2010 connection and execution membership: Connect/Disconnect edges — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2010-connection-and-execution-membership-connectdisconnect-edges--2026-10-11)

### 2025 cross-service synthesis survey — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-cross-service-synthesis-survey--2026-10-11)

### 2025 HLA-first portfolio survey and inventory reconciliation — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-hla-first-portfolio-survey-and-inventory-reconciliation--2026-10-11)

### Six-example stale-work evidence audit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#six-example-stale-work-evidence-audit--2026-10-11)

### Already-known object subscription evidence audit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#already-known-object-subscription-evidence-audit--2026-10-11)

### Attribute Value Update Request guide evidence audit — 2026-10-11

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#attribute-value-update-request-guide-evidence-audit--2026-10-11)

### 2025 DDM region-template deletion gate — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-ddm-region-template-deletion-gate--2026-10-12)

### 2025 Delay Subscription Evaluation: recipient-specific TSO boundary — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-delay-subscription-evaluation-recipient-specific-tso-boundary--2026-10-12)

### 2025 federation synchronization: participant result vs service failure — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-federation-synchronization-participant-result-vs-service-failure--2026-10-12)

### 2025 FOM module admission and additional-module Join commit boundary — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-fom-module-admission-and-additional-module-join-commit-boundary--2026-10-12)

### 2025 federation membership: rejected Disconnect and post-commit Join effects — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-federation-membership-rejected-disconnect-and-post-commit-join-effects--2026-10-12)

### 2025 DataElement carrier versus encoded-value length — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-dataelement-carrier-versus-encoded-value-length--2026-10-12)

### 2025 Federation MOM projection versus execution lifecycle — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-federation-mom-projection-versus-execution-lifecycle--2026-10-12)

### 2025 invocation reports versus requested Federation MOM reports — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-invocation-reports-versus-requested-federation-mom-reports--2026-10-12)

### 2025 exception-report scope and report-family boundary — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-exception-report-scope-and-report-family-boundary--2026-10-12)

### 2025 object information and subscription-state wayfinding — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-object-information-and-subscription-state-wayfinding--2026-10-12)

### 2025 explicit request versus Auto Provide boundary — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-explicit-request-versus-auto-provide-boundary--2026-10-12)

### 2025 object-name reservation versus local handle knowledge — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-object-name-reservation-versus-local-handle-knowledge--2026-10-12)

### 2025 model extension versus name-to-handle resolution — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-model-extension-versus-name-to-handle-resolution--2026-10-12)

### 2025 model vocabulary versus DataElement value encoding — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-model-vocabulary-versus-dataelement-value-encoding--2026-10-12)

### 2010/2025 DataElement implementation-boundary audit — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#20102025-dataelement-implementation-boundary-audit--2026-10-12)

### 2025 exception-report callback failure boundary — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-exception-report-callback-failure-boundary--2026-10-12)

### 2025 DDM empty-region-set and object/interaction boundary audit — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-ddm-empty-region-set-and-objectinteraction-boundary-audit--2026-10-12)

### 2025 save/restore responsibility and temporal-boundary audit — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-saverestore-responsibility-and-temporal-boundary-audit--2026-10-12)

### 2025 synchronization participant-set boundary audit — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-synchronization-participant-set-boundary-audit--2026-10-12)

### 2025 synchronization callback, process-event, and service-report boundary audit — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-synchronization-callback-process-event-and-service-report-boundary-audit--2026-10-12)

### 2025 process polling priority versus pushed-stream FIFO — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-process-polling-priority-versus-pushed-stream-fifo--2026-10-12)

### 2025 callback-order scope audit — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-callback-order-scope-audit--2026-10-12)

### 2025 discovery versus first reflection — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-discovery-versus-first-reflection--2026-10-12)

### 2025 Auto Provide later-scope boundary — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-auto-provide-later-scope-boundary--2026-10-12)

### 2025 timestamped object deletion and per-receiver removal — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-timestamped-object-deletion-and-per-receiver-removal--2026-10-12)

### 2025 save/restore responsibility-map audit — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-saverestore-responsibility-map-audit--2026-10-12)

### 2025 ordinary and directed interaction delivery survey — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-ordinary-and-directed-interaction-delivery-survey--2026-10-12)

### 2010 reference-RTI service evidence survey — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2010-reference-rti-service-evidence-survey--2026-10-12)

### 2025 callback and service-ordering boundary audit — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-callback-and-service-ordering-boundary-audit--2026-10-12)

### 2025 first-reflection ordering evidence audit — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-first-reflection-ordering-evidence-audit--2026-10-12)

### 2025 class ancestry: path-specific behavior and evidence limits — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-class-ancestry-path-specific-behavior-and-evidence-limits--2026-10-12)

### 2025 negotiated divestiture: cancellation after confirmation — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-negotiated-divestiture-cancellation-after-confirmation--2026-10-12)

### 2025 ownership transfer and update-region association — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-ownership-transfer-and-update-region-association--2026-10-12)

### 2025 accepted TSO update across ownership transfer — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-accepted-tso-update-across-ownership-transfer--2026-10-12)

### 2025 Retract authority after attribute ownership transfer — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-retract-authority-after-attribute-ownership-transfer--2026-10-12)

### 2025 Request Attribute Value Update: requester and receiver roles — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-request-attribute-value-update-requester-and-receiver-roles--2026-10-12)

### 2025 Federation MOM: effective FOM, module lists, and reports — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-federation-mom-effective-fom-module-lists-and-reports--2026-10-12)

### 2025 Federation MOM federation-list update evidence — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-federation-mom-federation-list-update-evidence--2026-10-12)

### 2025 Federation MOM duplicate-designator scope — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-federation-mom-duplicate-designator-scope--2026-10-12)

### 2025 Federation MOM module-identity evidence boundary — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-federation-mom-module-identity-evidence-boundary--2026-10-12)

### 2025 Object deletion and ownership-cleanup wayfinding — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-object-deletion-and-ownership-cleanup-wayfinding--2026-10-12)

### 2025 ResignAction cross-guide boundary — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-resignaction-cross-guide-boundary--2026-10-12)

### 2025 Connection-loss resignation: directive versus forced cleanup — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-connection-loss-resignation-directive-versus-forced-cleanup--2026-10-12)

### 2025 Connection loss during active Save/Restore — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-connection-loss-during-active-saverestore--2026-10-12)

### 2025 Forced connection loss during a pending synchronization point — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-forced-connection-loss-during-a-pending-synchronization-point--2026-10-12)

### 2025 FederationSynchronized recipient departure before observation — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-federationsynchronized-recipient-departure-before-observation--2026-10-12)

### 2025 Process resignation: delete and divest across surviving receivers — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-process-resignation-delete-and-divest-across-surviving-receivers--2026-10-12)

### 2025 process Request Retraction after receiver Resign — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-process-request-retraction-after-receiver-resign--2026-10-12)

### 2025 process timestamped-delete retraction-route comparison — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-process-timestamped-delete-retraction-route-comparison--2026-10-12)

### 2025 process Disconnect versus server Detach — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-process-disconnect-versus-server-detach--2026-10-12)

### 2025 committed Region state through Save/Restore — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-committed-region-state-through-saverestore--2026-10-12)

### 2025 HLA-first onboarding from ambassador construction to discovery — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-hla-first-onboarding-from-ambassador-construction-to-discovery--2026-10-12)

### 2010 reference smoke: Connect through object reflection — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2010-reference-smoke-connect-through-object-reflection--2026-10-12)

### 2025 additional-FOM Join and existing-object visibility — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-additional-fom-join-and-existing-object-visibility--2026-10-12)

### 2025 federation MOM conditional-list evidence audit — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-federation-mom-conditional-list-evidence-audit--2026-10-12)

### 2025 process Federation MOM after additional-module Join — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-process-federation-mom-after-additional-module-join--2026-10-12)

### 2025 process Federation MOM membership-list update audit — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-process-federation-mom-membership-list-update-audit--2026-10-12)

### 2025 process Federation Synchronization Point route audit — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-process-federation-synchronization-point-route-audit--2026-10-12)

### 2025 process Federation Save/Restore status-query audit — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-process-federation-saverestore-status-query-audit--2026-10-12)

### Local Mermaid renderer setup and nine-chart review — 2026-10-12

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#local-mermaid-renderer-setup-and-nine-chart-review--2026-10-12)

## 2025 object discovery and first-reflection boundary audit — 2026-10-14

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-object-discovery-and-first-reflection-boundary-audit--2026-10-14)

## 2025 FOM additional-module Join observer and failure-boundary audit — 2026-10-16

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-fom-additional-module-join-observer-and-failure-boundary-audit--2026-10-16)

## 2025 Delay Subscription Evaluation reader review — 2026-10-16

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-delay-subscription-evaluation-reader-review--2026-10-16)

## 2025 process RECEIVE temporal-gate audit — 2026-10-16

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-process-receive-temporal-gate-audit--2026-10-16)

### 2025 Request Attribute Value Update ownership/persistence audit — 2026-10-16

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-request-attribute-value-update-ownershippersistence-audit--2026-10-16)

### 2025 Federation MOM `HLAcurrentFDD` process Join audit — 2026-10-16

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-federation-mom-hlacurrentfdd-process-join-audit--2026-10-16)

### 2025 process object-removal producer identity audit — 2026-10-16

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-process-object-removal-producer-identity-audit--2026-10-16)

## 2025 exception and service-invocation reporting overlap audit — 2026-10-16

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-exception-and-service-invocation-reporting-overlap-audit--2026-10-16)

## 2025 configured-process regional service-report boundary — 2026-10-16

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-configured-process-regional-service-report-boundary--2026-10-16)

## 2025 configured-process service-report recipient and callback lifecycle — 2026-10-16

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-configured-process-service-report-recipient-and-callback-lifecycle--2026-10-16)

### 2025 discovery versus first attribute reflection — 2026-10-16

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-discovery-versus-first-attribute-reflection--2026-10-16)

### 2025 regional class-request and request-region scope audit — 2026-10-16

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-regional-class-request-and-request-region-scope-audit--2026-10-16)

### 2025 object deletion, ownership invalidation, and region lifetime — 2026-10-16

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-object-deletion-ownership-invalidation-and-region-lifetime--2026-10-16)

### 2025 TSO deletion and pending ownership boundary — 2026-10-16

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-tso-deletion-and-pending-ownership-boundary--2026-10-16)

### 2025 embedded ownership callback closures across TSO Retract — 2026-10-16

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-embedded-ownership-callback-closures-across-tso-retract--2026-10-16)

### 2025 joined-federate MOM removal on Resign versus ConnectionLost — 2026-10-17

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-joined-federate-mom-removal-on-resign-versus-connectionlost--2026-10-17)

### 2025 ConnectionLost and survivor Remove callback streams — 2026-10-17

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-connectionlost-and-survivor-remove-callback-streams--2026-10-17)

### 2025 forced-loss survivor Remove callback failure — 2026-10-17

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-forced-loss-survivor-remove-callback-failure--2026-10-17)

### 2025 configured-process Push removal failure after forced loss — 2026-10-17

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-configured-process-push-removal-failure-after-forced-loss--2026-10-17)

## 2025 Service Invocation Reporting evidence-link audit — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-service-invocation-reporting-evidence-link-audit--2026-11-03)

## 2025 Time Management request/grant/callback reader-gap review — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-time-management-requestgrantcallback-reader-gap-review--2026-11-03)

## 2025 Attribute Ownership negotiated-divestiture review — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-attribute-ownership-negotiated-divestiture-review--2026-11-03)

## 2025 object identity lifecycle cross-review — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-object-identity-lifecycle-cross-review--2026-11-03)

## 2025 RAVU requester/receiver eligibility review — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-ravu-requesterreceiver-eligibility-review--2026-11-03)

## 2025 late-subscription state-boundary cross-review — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-late-subscription-state-boundary-cross-review--2026-11-03)

## 2025 ResignAction source/evidence review — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-resignaction-sourceevidence-review--2026-11-03)

## 2025 configured-process time-role callback boundary review — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-configured-process-time-role-callback-boundary-review--2026-11-03)

## 2025 configured-process Enable Time Regulation and implicit-TARA review — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-configured-process-enable-time-regulation-and-implicit-tara-review--2026-11-03)

## 2025 Save/Restore member-exit adapter review — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-saverestore-member-exit-adapter-review--2026-11-03)

## 2025 C++ ambassador API-to-guide coverage — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-c-ambassador-api-to-guide-coverage--2026-11-03)

## 2025 handle/name query coverage cross-audit — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-handlename-query-coverage-cross-audit--2026-11-03)

## 2025 scope advisory and regional subscription cross-audit — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-scope-advisory-and-regional-subscription-cross-audit--2026-11-03)

## 2025 interaction transportation publication loss and query checkpoints — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-interaction-transportation-publication-loss-and-query-checkpoints--2026-11-03)

## 2025 interaction transportation callback-dispatch cross-audit — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-interaction-transportation-callback-dispatch-cross-audit--2026-11-03)

## 2025 object subscription and reflection reader-path review — 2026-11-03

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-object-subscription-and-reflection-reader-path-review--2026-11-03)

## 2025 name reservation and receiver-known callback-entry cross-review — 2026-11-06

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-name-reservation-and-receiver-known-callback-entry-cross-review--2026-11-06)

## 2025 timestamped attribute-update cross-guide review — 2026-11-06

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-timestamped-attribute-update-cross-guide-review--2026-11-06)

## 2025 ordinary receive-order Update Attribute Values reader-route review — 2026-11-06

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-ordinary-receive-order-update-attribute-values-reader-route-review--2026-11-06)

## 2025 Join-added attribute and existing-instance reader-gap review — 2026-11-06

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-join-added-attribute-and-existing-instance-reader-gap-review--2026-11-06)

## 2025 public additional-module Join duplicate/conflict evidence review — 2026-11-06

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-public-additional-module-join-duplicateconflict-evidence-review--2026-11-06)

## 2025 public additional-module Join conflict-coverage cross-review — 2026-11-06

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-public-additional-module-join-conflict-coverage-cross-review--2026-11-06)

## 2025 public additional-module Join dimension-bound cross-review — 2026-11-06

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-public-additional-module-join-dimension-bound-cross-review--2026-11-06)

## 2025 FOM handle encoding uniqueness and repeatability cross-review — 2026-11-06

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-fom-handle-encoding-uniqueness-and-repeatability-cross-review--2026-11-06)

## 2025 Disable Time Constrained client-FIFO admission review — 2026-11-06

2025 Disable Time Constrained client-FIFO admission review (2026-11-06): Refined the existing configured-process FQR/Disable sequence to separate the callback-dispatcher queue from the earlier client `pendingEvents_` FIFO. The focused public-API process integration case uses Query GALT as an ordered stream barrier, verifies Disable does not invoke the callback, then observes the already-submitted TSO interaction delivered as receive order after Evoke; it does not include FQR/FQG. Current source shows that a TSO interaction still in `pendingEvents_` at Disable success is submitted under the new disable generation, so the generation-mismatch conversion does not strip its timestamp/order metadata and the callback bridge retains timestamped form. This second admission branch and its relative FQG order are source-only, with no normative or profile-parity claim. Updated the existing process sequence; no new chart. Mermaid CLI 12.0.0 render and screenshot review passed. 23 local link occurrences across 22 unique targets resolve; git diff --check passed. Inventory remains 451 diagrams / 32 guides; selected screenshot review is 198/0; curated visual-QA queue remains 145/0. No tests, builds, Requirements Lab, or 2010 work.


## 2025 MOM Object and Interaction Reports guide reader review — 2026-11-06

[Archived review](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-mom-object-and-interaction-reports-guide-reader-review--2026-11-06)


## 2025 documentation archive link-integrity review — 2026-11-06

[Progress record](HLA-BEHAVIOR-FLOW-GUIDES-PROGRESS-LOG.md#2025-documentation-archive-link-integrity-review--2026-11-06); [review record](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-documentation-archive-link-integrity-review--2026-11-06). All 1,332 archive paths and anchors resolve; the 17 linked archive pages are staged with the indexes.

## 2025 configured-process TLS 1.3 Connect flow reader-gap increment — 2026-11-06

[Archived detail](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-configured-process-tls-13-connect-flow-reader-gap-increment--2026-11-06)


## 2025 TLS peer-validation-mode and negative Connect evidence review — 2026-11-06

[Progress record](HLA-BEHAVIOR-FLOW-GUIDES-PROGRESS-LOG.md#2025-tls-peer-validation-mode-and-negative-connect-evidence-review--2026-11-06); [archived review](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md#2025-tls-peer-validation-mode-and-negative-connect-evidence-review--2026-11-06); [review ledger](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-tls-peer-validation-mode-and-negative-connect-evidence-review--2026-11-06).


## 2025 RAVU §6.21 authorized-text access check — 2026-11-06

[Progress record](HLA-BEHAVIOR-FLOW-GUIDES-PROGRESS-LOG.md#2025-ravu-621-authorized-text-access-check--2026-11-06)


## 2025 late-subscription and mixed Confirm Divestiture boundary review — 2026-11-06

[Progress record](HLA-BEHAVIOR-FLOW-GUIDES-PROGRESS-LOG.md#2025-late-subscription-and-mixed-confirm-divestiture-boundary-review--2026-11-06)

## 2025 Confirm Divestiture fan-out visual review — 2026-11-06

[Progress record](HLA-BEHAVIOR-FLOW-GUIDES-PROGRESS-LOG.md#2025-confirm-divestiture-fan-out-visual-review--2026-11-06)

## 2025 active Save and Restore member-exit diagram review — 2026-11-06

[Progress record](HLA-BEHAVIOR-FLOW-GUIDES-PROGRESS-LOG.md#2025-active-save-and-restore-member-exit-diagram-review--2026-11-06)

## 2025 synchronization barrier late-join diagram review — 2026-11-06

[Progress record](HLA-BEHAVIOR-FLOW-GUIDES-PROGRESS-LOG.md#2025-synchronization-barrier-late-join-diagram-review--2026-11-06)
