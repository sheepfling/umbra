# Federation internals

This domain owns federation-scoped runtime state: joined federates, lifecycle
transitions, the federation registry, the embedded transport seam, and the
coordinator that prepares federation-management work. It is the home for
state that exists because several federates participate in one federation.

## Key files

- federation_registry files hold the central private state and per-federation
  indexes.
- federate_lifecycle files represent member connection, join, resignation, and
  removal transitions.
- federation_management_coordinator files prepare MIM/FOM-backed management
  operations before runtime state is committed.
- federation_save_commit_store files provide the internal durable save-commit
  manifest seam, including the canonical route-free
  `umbra-federation-state/v1` payload.
- federation_state_image files define the strict state-image codec for
  federation/control-plane identity, official encoded temporal values,
  object/ownership metadata, and per-federate interaction declarations
  (ordinary/regional/directed publication/subscription plus interaction
  transport/order overrides), plus ordinary/directed timestamped interaction
  payload bytes, timestamped attribute-update and object-deletion delivery
  payloads, recipient projections, and source-region snapshots; queue and
  those payload families rehydrate during bounded restore with live callback/
  service-report routes, including the bounded timestamped-deletion
  invocation snapshot used for reconstitution, and the shared
  retraction-recipient ledger, and typed If Available/regular ownership-
  acquisition reservation ledgers (request identity, ordering sequence,
  desired/queued/unavailable attributes, owner-release callback partitions,
  user tag, cancellation identity/attribute sets, Divestiture-If-Wanted
  notification identity/attribute sets, and Confirm Divestiture notification
  identity/attribute sets, and pending attribute transportation-type changes).
  Negotiated-divestiture candidate/confirmation state is also retained per
  attribute, as are the retained ownership-assumption recipient/tag maps.
  Known-class projections and pending discovery/removal recipient sets are
  retained as the typed object-visibility basis. Connection-loss automatic/
  deferred removal classifications and timestamped-deletion message/recipient
  linkage are retained as the typed object-lifecycle basis.
  Accepted Update Attribute Values telemetry (lifetime count,
  class/transportation buckets, and distinct updated-object projections) is
  retained on the joined-federate member image as the first application-value
  ledger. The accepted application Reflect Attribute Values callback count is
  now retained with class/transportation buckets and distinct reflected-
  object/class projections. The four joined-federate object-lifecycle MOM
  counters (Registered, Deleted, Removed, and Discovered) are also carried as
  typed lifetime scalars. Accepted Send Interaction telemetry now carries the
  total/directed counters and class/transportation buckets. Accepted Receive
  Interaction telemetry now carries the matching total/directed receipt
  counters and class/transportation buckets. Per-federate object-class
  attribute publication/subscription, update-rate, regional subscription,
  default transportation/order, and delete-privilege declarations are also
  encoded in a typed canonical section and restored with catalog/region
  validation. Federation-owned region specifications (owner, dimensions,
  pending/committed ranges, commit state, and in-use state) are likewise
  encoded and restored before regional declaration validation. Pending
  synchronization-point labels, tags, participant and
  announcement sets, and achievement results are likewise encoded and
  restored with live-member validation. The latest typed object
  application-value ledger and the typed object-instance-name reservation
  section are also retained. Receive-order values commit at admission;
  timestamped values commit at callback/reclaim boundaries, and retracted
  payloads are ignored. Disk-only rehydration, process-restart recovery, and
  pending time-advance and time-role-enable generation identities plus
  deferred decreasing Modify Lookahead requests are now represented in the
  route-free temporal image. The temporal role-enable ledger now has a live
  callback-route rebinding seam through the ambassador factory, with stale
  pre-restore callback epochs fenced. The public HLA_EVOKED/HLA_IMMEDIATE
  regulation and constrained-role companions are green. Multi-member ordering
  evidence, the remaining pending application ledgers, and
  disk-only/process-restart rehydration remain open.
- embedded_transport files provide the bounded in-process transport seam.
- process_transport files provide the private socket endpoint and handshake/
  framed-data path used by the first process-boundary implementation slice;
  they are not a public HLA service or conformance claim.
- process_federation_service files bind the private framed service envelope to
  the registry for the first Create/Join and receive-order interaction slice;
  the default receiver queue remains an internal polling projection for the
  focused service tests. `process_federation_callback_bridge` converts a
  pushed receive-order event into the official C++ `FederateAmbassador`
  callback using the shared immediate/evoked callback-session seam.
- process_federation_client owns the private client-side request identities,
  response validation, unsolicited event buffering, and callback-bridge
  handoff. `process_federation_service_probe` and its Catch2 coordinator use
  that seam across independently launched helper processes; it remains
  process-boundary foundation evidence, not a public installable or conformance
  surface.

## Working here

- Put federation-owned state and cross-federate routing decisions here.
- Use [handles](../handles/README.md) for stable FOM-derived identifiers and
  [FOM](../fom/README.md) for validated model information.
- Keep logical-time scheduling in [time](../time/README.md), even when a
  federation operation triggers it.
- Keep callback queue mechanics in [callbacks](../callbacks/README.md) and
  diagnostic record encoding in [observability](../observability/README.md).
- Do not expose a private registry type through public headers.

## Tests and references

Start with federate_lifecycle_catch2.cpp, federation_registry_catch2.cpp, and
ieee1516_2025_federation_management_catch2.cpp under
[cpp/tests/](../../../tests/). The governing boundaries and evidence rules are
in [architecture](../../../../docs/architecture/ARCHITECTURE.md) and
[requirements and testing](../../../../docs/testing/REQUIREMENTS-AND-TESTING.md).
