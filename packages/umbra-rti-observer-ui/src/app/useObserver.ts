import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { ObserverClient } from "../api/client";
import type {
  ContractResponse,
  EventEnvelope,
  StateResponse,
} from "../api/types";
import type { StreamMessage } from "../api/stream";

export interface MetricSample {
  at: number;
  eventCount: number;
  eventRate: number;
  errorCount: number;
}

export type ConnectionState = "connecting" | "live" | "polling" | "offline";

export interface ObserverViewModel {
  contract: ContractResponse | undefined;
  state: StateResponse | undefined;
  events: EventEnvelope[];
  samples: MetricSample[];
  connection: ConnectionState;
  error: string | undefined;
  notice: string | undefined;
  refresh: () => Promise<void>;
}

export function useObserver(): ObserverViewModel {
  const client = useMemo(() => new ObserverClient(), []);
  const [contract, setContract] = useState<ContractResponse>();
  const [state, setState] = useState<StateResponse>();
  const [eventsBySequence, setEventsBySequence] = useState<Map<number, EventEnvelope>>(
    new Map(),
  );
  const [samples, setSamples] = useState<MetricSample[]>([]);
  const [connection, setConnection] = useState<ConnectionState>("connecting");
  const [error, setError] = useState<string>();
  const [notice, setNotice] = useState<string>();
  const socketRef = useRef<WebSocket | undefined>(undefined);

  const applyState = useCallback((nextState: StateResponse) => {
    setState(nextState);
    setError(undefined);
    if (!nextState.retention.dropped_events && !nextState.retention.cleared_events) setNotice(undefined);
    setSamples((previous) => [
      ...previous,
      {
        at: Date.now(),
        eventCount: nextState.live_metrics.event_count,
        eventRate: nextState.live_metrics.event_rate_per_second,
        errorCount: nextState.live_metrics.error_count,
      },
    ].slice(-60));
    setEventsBySequence((previous) => nextState.normalized_events
      ? mergeEvents(new Map(), nextState.normalized_events)
      : new Map([...previous].filter(([sequence]) => sequence >= nextState.retention.first_retained_sequence
        && sequence <= nextState.retention.last_sequence && nextState.retention.retained_events > 0)));
  }, []);

  const applyMessage = useCallback((message: StreamMessage) => {
    if (message.type === "snapshot" || message.type === "events") {
      applyState(message.state);
      if (message.type === "events") {
        const retention = message.state.retention;
        const retained = message.events.filter((event) => retention.retained_events > 0
          && event.sequence >= retention.first_retained_sequence && event.sequence <= retention.last_sequence);
        setEventsBySequence((previous) => mergeEvents(previous, retained));
      }
    } else if (message.type === "gap") {
      setNotice("Some history was dropped or cleared. Charts and counts cover retained observations only.");
    } else if (message.type === "error") {
      setError(message.message);
    }
  }, [applyState]);

  const refresh = useCallback(async () => {
    try {
      const [nextContract, nextState] = await Promise.all([
        client.getContract(),
        client.getState(true),
      ]);
      setContract(nextContract);
      applyState(nextState);
      setConnection((current) => current === "live" ? current : "polling");
    } catch (requestError) {
      setConnection("offline");
      setError(requestError instanceof Error ? requestError.message : "observer unavailable");
    }
  }, [applyState, client]);

  useEffect(() => {
    let disposed = false;
    let reconnect: number | undefined;
    let poll: number | undefined;
    let retry = 1_000;
    const connect = async () => {
      if (disposed) return;
      try {
        const socket = await client.openStream(applyMessage, (status) => {
          if (disposed) return;
          if (status === "open") { retry = 1_000; setConnection("live"); }
          if (status === "closed") {
            setConnection((current) => current === "offline" ? "offline" : "polling");
            reconnect = window.setTimeout(() => void connect(), retry);
            retry = Math.min(retry * 2, 15_000);
          }
          if (status === "error") setConnection((current) => current === "offline" ? "offline" : "polling");
        });
        if (disposed) socket?.close();
        else {
          socketRef.current = socket;
          if (!socket) setConnection((current) => current === "offline" ? current : "polling");
        }
      } catch {
        if (!disposed) reconnect = window.setTimeout(() => void connect(), retry);
        retry = Math.min(retry * 2, 15_000);
      }
    };
    const pollState = async () => {
      await refresh();
      if (!disposed) poll = window.setTimeout(() => void pollState(), 5_000);
    };
    void pollState();
    void connect();
    return () => {
      disposed = true;
      window.clearTimeout(poll);
      window.clearTimeout(reconnect);
      socketRef.current?.close();
      socketRef.current = undefined;
    };
  }, [applyMessage, client, refresh]);

  const events = useMemo(
    () => [...eventsBySequence.values()].sort((left, right) => left.sequence - right.sequence),
    [eventsBySequence],
  );

  return { contract, state, events, samples, connection, error, notice, refresh };
}

function mergeEvents(
  current: Map<number, EventEnvelope>,
  incoming: EventEnvelope[],
): Map<number, EventEnvelope> {
  const next = new Map(current);
  for (const event of incoming) next.set(event.sequence, event);
  const retained = [...next.entries()].sort(([left], [right]) => left - right).slice(-2_000);
  return new Map(retained);
}
