import { useEffect, useMemo, useRef, useState } from "react";
import { ObserverClient } from "../api/client";
import type { AnalyticsResponse, PortableRecording, RecordingFrame } from "../api/types";
import type { ReplayWorkerResponse } from "./recordingCodec";
import { advancePosition, frameAt } from "./replay";

export function useReplay() {
  const [document, setDocument] = useState<PortableRecording>();
  const [cursor, setCursor] = useState({ seconds: 0, index: 0 });
  const [playing, setPlaying] = useState(false);
  const [speed, setSpeed] = useState(1);
  const worker = useRef<Worker | null>(null);
  const [resolved, setResolved] = useState<{ document: PortableRecording; response: ReplayWorkerResponse }>();
  useEffect(() => {
    setResolved(undefined);
    if (!document) return;
    let next: Worker;
    try { next = new Worker(new URL("./replay.worker.ts", import.meta.url), { type: "module" }); }
    catch { setResolved({ document, response: { type: "error" } }); setPlaying(false); return; }
    worker.current = next;
    next.onmessage = (event: MessageEvent<ReplayWorkerResponse>) => { setResolved({ document, response: event.data }); if (event.data.type === "error") setPlaying(false); };
    next.onerror = () => { setResolved({ document, response: { type: "error" } }); setPlaying(false); };
    next.postMessage({ type: "load", document });
    return () => { next.terminate(); worker.current = null; };
  }, [document]);
  useEffect(() => { worker.current?.postMessage({ type: "seek", index: cursor.index }); }, [document, cursor.index]);
  const response = resolved?.document === document ? resolved?.response : undefined;
  const frame = response?.type === "frame" ? response.frame : undefined;
  const samples = response?.type === "frame" ? response.samples : [];
  const error = response?.type === "error" ? "Recording reconstruction failed its bounded codec checks. No live view or provider action is substituted. Return to live or open another validated recording." : undefined;
  const loading = Boolean(document && !error && frame?.index !== cursor.index);
  const load = (next: PortableRecording) => { setPlaying(false); setDocument(next); setCursor({ seconds: 0, index: 0 }); setSpeed(1); };
  const close = () => { setPlaying(false); setDocument(undefined); setCursor({ seconds: 0, index: 0 }); };
  const seek = (seconds: number) => {
    if (!document || !Number.isFinite(seconds)) return;
    setPlaying(false);
    const bounded = Math.max(0, Math.min(document.duration_seconds, seconds));
    setCursor({ seconds: bounded, index: frameAt(document.frames, bounded) });
  };
  const step = (index: number) => {
    if (!document || !Number.isFinite(index)) return;
    setPlaying(false);
    const bounded = Math.max(0, Math.min(document.frames.length - 1, Math.trunc(index)));
    setCursor({ seconds: document.frames[bounded]!.offset_seconds, index: bounded });
  };
  const toggle = () => {
    if (!document || document.duration_seconds === 0 || error) return;
    if (!playing && cursor.seconds >= document.duration_seconds) setCursor({ seconds: 0, index: 0 });
    setPlaying((value) => !value);
  };
  useEffect(() => {
    if (!playing || !document) return;
    const started = performance.now(), origin = cursor.seconds;
    const tick = () => {
      const seconds = advancePosition(origin, (performance.now() - started) / 1000, speed, document.duration_seconds);
      setCursor({ seconds, index: frameAt(document.frames, seconds) });
      if (seconds >= document.duration_seconds) setPlaying(false);
    };
    const timer = window.setInterval(tick, 50);
    return () => window.clearInterval(timer);
    // Seeking pauses first; the origin is captured only when play/speed changes.
  }, [document, playing, speed]);
  return { document, cursor, playing, speed, setSpeed, frame, samples, loading, error, load, close, seek, step, toggle };
}

export type ReplayController = ReturnType<typeof useReplay>;

export function useRecordingAnalytics(frame: RecordingFrame | undefined, enabled: boolean, windowSeconds: number) {
  const client = useMemo(() => new ObserverClient(), []);
  const [result, setResult] = useState<{ frame: RecordingFrame; windowSeconds: number; data?: AnalyticsResponse; error?: string }>();
  useEffect(() => {
    setResult(undefined);
    if (!enabled || !frame) return;
    const controller = new AbortController();
    void client.analyzeRecording(frame, windowSeconds, AbortSignal.any([controller.signal, AbortSignal.timeout(10_000)]))
      .then((next) => { if (!controller.signal.aborted) setResult({ frame, windowSeconds, data: next }); })
      .catch(() => { if (!controller.signal.aborted) setResult({ frame, windowSeconds, error: "Recorded analytics could not be computed. Events and captured inspectors remain available; no live window is substituted." }); });
    return () => controller.abort();
  }, [client, frame, enabled, windowSeconds]);
  return enabled && result && result.frame === frame && result.windowSeconds === windowSeconds
    ? { data: result.data, error: result.error } : { data: undefined, error: undefined };
}
