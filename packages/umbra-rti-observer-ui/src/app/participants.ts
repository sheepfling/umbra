import type { AnalyticsResponse, EventEnvelope, JsonValue, ParticipantPerspective, StateResponse } from "../api/types";

function label(value: unknown): string | undefined {
  // Match Python's display-label whitespace policy, including NEXT LINE but
  // excluding BOM. Opaque IDs/session keys never pass through this function.
  if (typeof value === "string") return value.replace(/^[\u0009-\u000d\u001c-\u0020\u0085\u00a0\u1680\u2000-\u200a\u2028\u2029\u202f\u205f\u3000]+|[\u0009-\u000d\u001c-\u0020\u0085\u00a0\u1680\u2000-\u200a\u2028\u2029\u202f\u205f\u3000]+$/gu, "") || undefined;
  return typeof value === "number" && Number.isSafeInteger(value) ? String(value) : undefined;
}

export function reportedFederate(event: EventEnvelope): string | undefined {
  const value = event.federate ?? event.participant;
  if (typeof value === "string" || typeof value === "number") return label(value);
  if (value && typeof value === "object" && !Array.isArray(value)) {
    for (const field of ["name", "id", "handle"]) {
      const item: JsonValue | undefined = value[field] as JsonValue | undefined;
      const name = label(item);
      if (name) return name;
    }
  }
  return label(event.federate_name) ?? label(event.participant_name);
}

export function eventFederate(event: EventEnvelope): string | undefined {
  return event.participant_attribution?.participant.display_name || reportedFederate(event)
    || (event.participant_attribution ? "Unnamed participant" : undefined);
}

export function legacyParticipantKey(name: string): string {
  return JSON.stringify(["legacy-name", name]);
}

export function eventParticipantKey(event: EventEnvelope, provenance: boolean): string | undefined {
  const identity = event.participant_attribution;
  const source = label(event.source) ?? "unknown";
  if (identity) return JSON.stringify(["explicit-join", source, identity.session_id, identity.identity_namespace,
    identity.federation_id, identity.participant.participant_id, identity.participant.membership_id]);
  const name = reportedFederate(event);
  if (!name) return undefined;
  if (!provenance) return legacyParticipantKey(name);
  const session = event.time_observation?.session_id ?? event.declaration?.session_id ?? event.session_id ?? null;
  return JSON.stringify(["legacy-provenance", source, session, label(event.federation) ?? null, name]);
}

export function eventParticipantPerspective(event: EventEnvelope, provenance: boolean): ParticipantPerspective | undefined {
  const key = eventParticipantKey(event, provenance);
  if (!key || !provenance && !event.participant_attribution) return undefined;
  return { key, basis: event.participant_attribution ? "explicit-join" : "legacy-provenance",
    source: label(event.source) ?? "unknown", session_id: event.participant_attribution?.session_id
      ?? event.time_observation?.session_id ?? event.declaration?.session_id ?? event.session_id ?? null,
    federation: label(event.federation) ?? null, reported_name: reportedFederate(event) ?? null, attribution: event.participant_attribution };
}

export function participantKey(row: { federate?: string | null; perspective?: ParticipantPerspective | null }): string | undefined {
  return row.perspective?.key ?? (row.federate ? legacyParticipantKey(row.federate) : undefined);
}

export function objectPerspectiveKey(row: { federate: string | null; perspective?: ParticipantPerspective | null }): string | null {
  return row.federate === null ? null : participantKey(row) ?? legacyParticipantKey(row.federate);
}

export function objectKey(row: { object_key: string; observation_key?: string | null; object_context?: { identity_namespace: string; federation_id: string } | null }): string {
  // Tag legacy keys too: a raw handle is allowed to look exactly like the
  // serialized key of a different, explicitly scoped object.
  return row.object_context ? JSON.stringify(["federation-object", row.object_context.identity_namespace,
    row.object_context.federation_id, row.object_key]) : JSON.stringify(["legacy-object", row.object_key]);
}

export function reportingScopeKey(scope: { source: string; session_id: string | null; federation: string | null; federate: string | null; perspective?: ParticipantPerspective | null }): string {
  if (scope.perspective?.attribution) return scope.perspective.key;
  const original = [scope.source, scope.session_id, scope.federation, scope.federate];
  const canonical = JSON.stringify(["legacy-provenance", ...original]);
  // Retain the original v1 time/declaration selection format where it denotes
  // the same legacy scope. A distinct reported root session must not collapse.
  return scope.perspective && scope.perspective.key !== canonical ? scope.perspective.key : JSON.stringify(original);
}

export function supportsParticipantProvenance(state: StateResponse, events: EventEnvelope[] = []): boolean {
  const feature = state.capabilities.participant_provenance?.state;
  return feature === "available" || feature === "partial" || state.federate_roster.some((row) => row.perspective != null)
    || events.some((event) => event.participant_attribution != null);
}

export function perspectiveDescription(perspective?: ParticipantPerspective | null): string {
  if (!perspective) return "Legacy name-only view · reporter and join identity not established";
  const reporter = `${perspective.source} / ${perspective.session_id ?? "session unreported"}`;
  const identity = perspective.attribution;
  return identity ? `${reporter} · ${identity.identity_namespace} / ${identity.federation_id} · ${identity.participant.participant_id} / ${identity.participant.membership_id}`
    : `${reporter} · legacy metadata; join IDs unreported`;
}

export interface ParticipantChoice {
  key: string;
  label: string;
  perspective?: ParticipantPerspective | null;
  unresolved?: boolean;
  handle?: string | null;
}

export function unavailableParticipant(key: string): ParticipantChoice {
  return { key, label: key.length > 128 || key.startsWith("[") ? "Saved participant perspective" : key, unresolved: true };
}

export function participantChoices(state: StateResponse, events: EventEnvelope[] = [], analytics?: AnalyticsResponse): ParticipantChoice[] {
  const rows = new Map<string, ParticipantChoice>();
  const add = (row: { federate?: string | null; perspective?: ParticipantPerspective | null; handle?: string | null }) => {
    const key = participantKey(row);
    if (key && row.federate) rows.set(key, { key, label: row.federate, perspective: row.perspective, handle: row.handle });
  };
  // These supplemental views keep known choices reachable after roster expiry.
  for (const row of state.inspectors.object_observations?.snapshots ?? []) if (row.federate !== null) add(row);
  for (const row of state.inspectors.time?.snapshots ?? []) add(row.scope);
  for (const row of state.inspectors.time?.requests ?? []) add(row.scope);
  for (const row of state.inspectors.declarations?.snapshots ?? []) add(row.scope);
  for (const row of state.inspectors.interactions) add(row);
  const provenance = supportsParticipantProvenance(state, events);
  for (const event of events) {
    const key = eventParticipantKey(event, provenance), name = eventFederate(event);
    if (key && name) rows.set(key, { key, label: name, perspective: eventParticipantPerspective(event, provenance) ?? rows.get(key)?.perspective });
  }
  for (const row of analytics?.participants ?? []) add(row);
  for (const row of state.federate_roster) add(row);
  return [...rows.values()].sort((a, b) => a.label.localeCompare(b.label) || a.key.localeCompare(b.key));
}
