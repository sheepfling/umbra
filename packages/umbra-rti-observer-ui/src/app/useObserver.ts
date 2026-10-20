import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { ObserverClient } from "../api/client";
import type { ObserverStream } from "../api/client";
import type { ContractResponse, EventEnvelope, StateResponse } from "../api/types";
import { streamTransports } from "../api/stream";
import type { StreamMessage, StreamTransport } from "../api/stream";
import { reconcileLive } from "./liveState";
import type { LiveState } from "./liveState";

export interface MetricSample { at: number; eventCount: number; eventRate: number; errorCount: number }
export type ConnectionState = "connecting" | "live" | "polling" | "offline";
export interface ObserverViewModel {
  contract: ContractResponse | undefined;
  state: StateResponse | undefined;
  events: EventEnvelope[];
  samples: MetricSample[];
  connection: ConnectionState;
  transport: StreamTransport | "http" | undefined;
  error: string | undefined;
  notice: string | undefined;
  refresh: () => Promise<void>;
}

export function useObserver(active = true): ObserverViewModel {
  const client = useMemo(() => new ObserverClient(), []);
  const [contract, setContract] = useState<ContractResponse>();
  const [view, setView] = useState<LiveState>({ events: new Map() });
  const live = useRef(view);
  const [samples, setSamples] = useState<MetricSample[]>([]);
  const [connection, setConnection] = useState<ConnectionState>("connecting");
  const [transport, setTransport] = useState<ObserverViewModel["transport"]>();
  const [error, setError] = useState<string>();
  const [notice, setNotice] = useState<string>();
  const refreshOperation = useRef<() => Promise<void>>(async () => {});
  const refresh = useCallback(() => refreshOperation.current(), []);

  useEffect(() => {
    if (!active) return;
    let disposed = false;
    let networkOffline = !navigator.onLine;
    let stream: ObserverStream | undefined;
    let streamOpen = false;
    let connecting = false;
    let generation = 0;
    let updates = 0;
    let signature = "";
    let latestContract: ContractResponse | undefined;
    let pendingPoll: Promise<void> | undefined;
    let pollTimer: number | undefined;
    let retryTimer: number | undefined;
    let retry = 1000;
    const failed = new Set<StreamTransport>();
    const stopStream = () => {
      generation++;
      stream?.close(); stream = undefined; streamOpen = false; connecting = false;
      window.clearTimeout(retryTimer); retryTimer = undefined;
    };
    const apply = (state: StateResponse, rows: EventEnvelope[] = [], cursor?: LiveState["cursor"]) => {
      const previous = live.current;
      const next = reconcileLive(previous, state, rows, cursor);
      if (next === previous) return;
      const oldId = previous.state?.observer_lifetime?.instance_id;
      const newId = state.observer_lifetime?.instance_id;
      const restarted = Boolean(previous.state && oldId !== newId);
      live.current = next; setView(next); updates++;
      setError(undefined);
      if (restarted) setNotice("Observer backend changed. Previous live history was replaced; RTI membership and command authority were not inferred.");
      setSamples((previousSamples) => [...(restarted ? [] : previousSamples), {
        at: Date.now(), eventCount: state.live_metrics.event_count,
        eventRate: state.live_metrics.event_rate_per_second, errorCount: state.live_metrics.error_count,
      }].slice(-60));
    };
    const receive = (message: StreamMessage) => {
      if (message.type === "snapshot" || message.type === "events") {
        const oldId = live.current.state?.observer_lifetime?.instance_id;
        const newId = message.state.observer_lifetime?.instance_id;
        if (message.type === "events" && oldId && oldId !== newId) return;
        apply(message.state, message.type === "events" ? message.events : [], message.cursor ?? undefined);
        if (message.type === "snapshot" && message.resume === "gap") setNotice("Some history was dropped or cleared. The retained window was reconciled; missing observations were not reconstructed.");
        if (streamOpen) setConnection("live");
      } else if (message.type === "gap") {
        if (!message.observer_lifetime || message.observer_lifetime.instance_id === live.current.state?.observer_lifetime?.instance_id)
          setNotice("Some history was dropped or cleared. Charts and counts cover retained observations only.");
      } else if (message.type === "error") setError(message.message);
    };
    const connect = async (nextContract: ContractResponse) => {
      if (disposed || networkOffline) return;
      const nextSignature = JSON.stringify([nextContract.capabilities.websocket?.state, nextContract.capabilities.sse?.state,
        nextContract.api_paths.websocket, nextContract.api_paths.event_stream, nextContract.stream.resumption]);
      if (signature !== nextSignature) {
        stopStream(); failed.clear(); retry = 1000; signature = nextSignature;
        setConnection((current) => current === "offline" ? current : "polling");
      }
      if (stream || connecting || retryTimer !== undefined) return;
      const supported = streamTransports(nextContract);
      const selected = supported.find((candidate) => !failed.has(candidate));
      if (!selected) {
        setTransport("http");
        if (supported.length) retryTimer = window.setTimeout(() => {
          retryTimer = undefined; failed.clear(); void poll();
        }, retry);
        retry = Math.min(retry * 2, 15000);
        return;
      }
      connecting = true;
      const ownGeneration = ++generation;
      const closed = () => {
        if (disposed || ownGeneration !== generation) return;
        stream = undefined; connecting = false; streamOpen = false;
        failed.add(selected); setTransport("http");
        setConnection((current) => current === "offline" ? current : "polling");
        if (latestContract) void connect(latestContract);
      };
      try {
        const opened = await client.openStream(selected, (message) => {
          if (!disposed && ownGeneration === generation) receive(message);
        }, (status) => {
          if (disposed || ownGeneration !== generation) return;
          if (status === "open") {
            streamOpen = true; retry = 1000; setConnection("live"); setTransport(selected);
          } else if (status === "closed") closed();
        }, live.current.cursor);
        if (disposed || ownGeneration !== generation) opened.close();
        else { stream = opened; connecting = false; }
      } catch { closed(); }
    };
    const poll = (): Promise<void> => {
      if (disposed) return Promise.resolve();
      if (networkOffline) {
        setConnection("offline");
        return Promise.resolve();
      }
      if (pendingPoll) return pendingPoll;
      const ticket = updates;
      pendingPoll = (async () => {
        try {
          const nextContract = await client.getContract(true);
          if (disposed || networkOffline) return;
          latestContract = nextContract; setContract(nextContract);
          const state = await client.getState(true);
          if (disposed || networkOffline) return;
          // A stream cut received during this HTTP request takes precedence.
          if (ticket === updates) {
            const previous = live.current.state;
            if (previous && previous.observer_lifetime?.instance_id !== state.observer_lifetime?.instance_id) stopStream();
            apply(state);
          }
          setConnection(streamOpen ? "live" : "polling");
          await connect(nextContract);
        } catch (failure) {
          if (!disposed) {
            if (!streamOpen) setConnection("offline");
            setError(failure instanceof Error ? failure.message : "Observer unavailable; retrying discovery.");
          }
        } finally { pendingPoll = undefined; }
      })();
      return pendingPoll;
    };
    const pollAgain = async () => {
      await poll();
      if (!disposed) pollTimer = window.setTimeout(() => void pollAgain(), 5000);
    };
    refreshOperation.current = poll;
    const offline = () => {
      networkOffline = true; updates++; stopStream(); setTransport("http"); setConnection("offline");
      setError("Browser network is offline. The last observed state is shown; reconnecting when the network returns.");
    };
    const online = () => { networkOffline = false; failed.clear(); signature = ""; void poll(); };
    window.addEventListener("offline", offline);
    window.addEventListener("online", online);
    void pollAgain();
    return () => {
      disposed = true; stopStream(); window.clearTimeout(pollTimer);
      window.removeEventListener("offline", offline);
      window.removeEventListener("online", online);
      refreshOperation.current = async () => {};
    };
  }, [active, client]);

  const events = useMemo(() => [...view.events.values()].sort((a, b) => a.sequence - b.sequence), [view.events]);
  return { contract, state: view.state, events, samples, connection, transport, error, notice, refresh };
}
