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
  const socketRef = useRef<WebSocket | undefined>(undefined);

  const applyState = useCallback((nextState: StateResponse) => {
    setState(nextState);
    setError(undefined);
    setSamples((previous) => [
      ...previous,
      {
        at: Date.now(),
        eventCount: nextState.live_metrics.event_count,
        eventRate: nextState.live_metrics.event_rate_per_second,
        errorCount: nextState.live_metrics.error_count,
      },
    ].slice(-60));
    if (nextState.normalized_events) {
      setEventsBySequence((previous) => mergeEvents(previous, nextState.normalized_events ?? []));
    }
  }, []);

  const applyMessage = useCallback((message: StreamMessage) => {
    if (message.type === "snapshot" || message.type === "events") {
      applyState(message.state);
      if (message.type === "events") {
        setEventsBySequence((previous) => mergeEvents(previous, message.events));
      }
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
    void refresh();
    const poll = window.setInterval(() => {
      if (socketRef.current?.readyState !== WebSocket.OPEN) void refresh();
    }, 2_000);
    const socket = client.openStream(
      applyMessage,
      (status) => {
        if (status === "open") setConnection("live");
        else if (status === "closed" || status === "error") {
          setConnection((current) => current === "live" ? "polling" : current);
        }
      },
    );
    socketRef.current = socket;
    return () => {
      window.clearInterval(poll);
      socket.close();
      socketRef.current = undefined;
    };
  }, [applyMessage, client, refresh]);

  const events = useMemo(
    () => [...eventsBySequence.values()].sort((left, right) => left.sequence - right.sequence),
    [eventsBySequence],
  );

  return { contract, state, events, samples, connection, error, refresh };
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
