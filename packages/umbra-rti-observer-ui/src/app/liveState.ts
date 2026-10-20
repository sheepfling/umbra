import type { EventEnvelope, StateResponse } from "../api/types";
import type { StreamCursor } from "../api/stream";

export interface LiveState {
  state?: StateResponse;
  events: Map<number, EventEnvelope>;
  cursor?: StreamCursor;
}

/** Reconcile an atomic state cut. Sequence numbers are local to its lifetime. */
export function reconcileLive(previous: LiveState, state: StateResponse, incoming: EventEnvelope[] = [],
  cursor?: StreamCursor): LiveState {
  const oldLife = previous.state?.observer_lifetime;
  const life = state.observer_lifetime;
  if (life && oldLife && life.instance_id === oldLife.instance_id && life.revision < oldLife.revision) return previous;
  const restarted = Boolean(life && oldLife && life.instance_id !== oldLife.instance_id)
    || Boolean(previous.state && Boolean(life) !== Boolean(oldLife));
  const retention = state.retention;
  // A complete snapshot is authoritative even when the array is empty.
  const events = state.normalized_events != null ? new Map<number, EventEnvelope>()
    : restarted ? new Map<number, EventEnvelope>() : new Map(previous.events);
  for (const row of [...state.normalized_events ?? [], ...incoming]) events.set(row.sequence, row);
  for (const sequence of events.keys()) {
    if (!retention.retained_events || sequence < retention.first_retained_sequence || sequence > retention.last_sequence) events.delete(sequence);
  }
  const bounded = new Map([...events].sort(([left], [right]) => left - right).slice(-2000));
  const nextCursor = cursor ?? (life && state.normalized_events != null
    ? { ...life, after: retention.next_sequence - 1 } : restarted ? undefined : previous.cursor);
  return { state, events: bounded, cursor: nextCursor && previous.cursor
    && nextCursor.instance_id === previous.cursor.instance_id && nextCursor.after < previous.cursor.after
    ? previous.cursor : nextCursor };
}
