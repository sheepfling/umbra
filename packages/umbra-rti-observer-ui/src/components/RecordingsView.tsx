import { useEffect, useMemo, useState } from "react";
import { ObserverClient } from "../api/client";
import type { CaptureStatus, PortableRecording } from "../api/types";
import type { ReplayController } from "../app/useReplay";
import { formatTime } from "../app/observations";

function download(document: PortableRecording) {
  const url = URL.createObjectURL(new Blob([JSON.stringify(document)], { type: "application/json" }));
  const anchor = window.document.createElement("a");
  anchor.href = url; anchor.download = "observer-recording.json"; anchor.click();
  URL.revokeObjectURL(url);
}

export function ReplayBanner({ replay, onClose }: { replay: ReplayController; onClose: () => void }) {
  const document = replay.document, frame = replay.frame;
  if (!document) return null;
  return <section className="replay-banner" aria-label="Replay controls">
    <div><p className="eyebrow">Recorded observations · never a live RTI connection</p><h2>Replay: {document.label}</h2><p>{frame ? `Captured state at ${formatTime(frame.captured_at)} · ${frame.change} · frame ${frame.index + 1} / ${document.frames.length}.` : "Preparing the recorded baseline."} Provider commands are disabled.</p>{replay.loading && <p role="status">Seeking recorded state {replay.cursor.index + 1}… {frame ? `Still showing state ${frame.index + 1} until reconstruction completes.` : "No live data is shown."}</p>}{replay.error && <p className="error-banner" role="alert">{replay.error}</p>}</div>
    <div className="replay-toolbar"><button className="primary-button" type="button" disabled={document.duration_seconds === 0} onClick={replay.toggle}>{replay.playing ? "Pause replay" : "Play replay"}</button><button className="secondary-button" type="button" disabled={replay.cursor.index === 0} onClick={() => replay.step(replay.cursor.index - 1)}>Previous recorded state</button><button className="secondary-button" type="button" disabled={replay.cursor.index === document.frames.length - 1} onClick={() => replay.step(replay.cursor.index + 1)}>Next recorded state</button><label>Speed<select aria-label="Replay speed" value={replay.speed} onChange={(event) => replay.setSpeed(Number(event.target.value))}>{[0.25, 0.5, 1, 2, 4, 8].map((speed) => <option key={speed} value={speed}>{speed}×</option>)}</select></label><button className="secondary-button" type="button" onClick={onClose}>Return to live</button></div>
    <div className="replay-seek"><label>Clock: {replay.cursor.seconds.toFixed(3)} / {document.duration_seconds.toFixed(3)} seconds<input aria-label="Replay clock" type="range" min={0} max={document.duration_seconds} step="any" value={replay.cursor.seconds} disabled={document.duration_seconds === 0} onChange={(event) => replay.seek(Number(event.target.value))} /></label><label>Exact recorded state<input aria-label="Replay frame" type="range" min={0} max={document.frames.length - 1} step={1} value={replay.cursor.index} onChange={(event) => replay.step(Number(event.target.value))} /></label></div>
    <p className="footnote">The view holds the last captured state between commits/checkpoints; reported pending ages and evidence are not extrapolated. Wall-clock jumps do not change replay ordering. End: {document.stop_reason}. Initial retained-history loss and inspector provenance remain visible.</p>
  </section>;
}

export function RecordingsView({ enabled, reason, onReplay }: { enabled: boolean; reason?: string | null; onReplay: (document: PortableRecording) => void }) {
  const client = useMemo(() => new ObserverClient(), []);
  const [capture, setCapture] = useState<CaptureStatus | null>();
  const [label, setLabel] = useState("Federation investigation");
  const [file, setFile] = useState<File>();
  const [error, setError] = useState<string>();
  const [statusError, setStatusError] = useState<string>();
  const [statusReady, setStatusReady] = useState(false);
  const [busy, setBusy] = useState(false);
  const [loading, setLoading] = useState(enabled);
  useEffect(() => {
    if (!enabled) { setCapture(undefined); setLoading(false); setStatusReady(false); return; }
    if (busy) return;
    const controller = new AbortController(); let timer: number | undefined;
    const refresh = async () => {
      try {
        const status = await client.getRecordingStatus(AbortSignal.any([controller.signal, AbortSignal.timeout(10_000)]));
        if (!controller.signal.aborted) { setCapture(status.recording); setLoading(false); setStatusReady(true); setStatusError(undefined); }
      } catch { if (!controller.signal.aborted) { setLoading(false); setStatusReady(false); setStatusError("Recording status could not refresh. No capture is assumed stopped; retrying shortly."); } }
      finally { if (!controller.signal.aborted) timer = window.setTimeout(() => void refresh(), 1000); }
    };
    void refresh();
    return () => { controller.abort(); window.clearTimeout(timer); };
  }, [client, enabled, busy]);
  const act = async (operation: () => Promise<void>) => {
    setBusy(true); setError(undefined);
    try { await operation(); } catch (failure) { setError(failure instanceof Error ? failure.message : "Recording operation failed."); }
    finally { setBusy(false); }
  };
  const start = () => {
    if (capture && !window.confirm("Replace the retained backend recording? Download it first if you want to keep it.")) return;
    void act(async () => setCapture(await client.startRecording(label, capture?.recording_id)));
  };
  return <section className="recordings-workspace" aria-label="Recordings workspace">
    <div className="section-intro"><div><p className="eyebrow">Bounded capture · portable files · deterministic inspection</p><h2>Recordings and replay</h2><p>Preserve validated observer states and complete committed envelope batches, then navigate the same workspaces without executing provider actions.</p></div></div>
    {!enabled && <p className="coverage-note">Recording unavailable: {reason ?? "This backend does not advertise recording/import resources."} Independent observation screens remain usable.</p>}
    {(error || statusError) && <p className="error-banner" role="alert">{error ?? statusError}</p>}
    <article className="panel" aria-label="Backend recording"><h3>Capture this observer</h3><p>One recording is retained by this backend process, shared by its browser clients. Collection and recording continue when you change tabs or close the page; restarting the backend loses an undownloaded capture.</p><label>Recording label<input aria-label="Recording label" maxLength={120} value={label} onChange={(event) => setLabel(event.target.value)} disabled={!enabled || busy || capture?.active} /></label>
      <div className="recording-actions"><button className="primary-button" type="button" disabled={!enabled || loading || !statusReady || busy || capture?.active || !label.trim()} onClick={start}>{capture ? "Start new recording" : "Start recording"}</button><button className="secondary-button" type="button" disabled={!enabled || !statusReady || busy || !capture?.active} onClick={() => void act(async () => setCapture(await client.stopRecording(capture!.recording_id)))}>Stop recording</button><button className="secondary-button" type="button" disabled={!enabled || !statusReady || busy || !capture || capture.active || capture.frame_count === 0} onClick={() => void act(async () => download(await client.getRecording(capture!.recording_id)))}>Download recording</button><button className="secondary-button" type="button" disabled={!enabled || !statusReady || busy || !capture || capture.active || capture.frame_count === 0} onClick={() => void act(async () => onReplay(await client.getRecording(capture!.recording_id)))}>Replay retained recording</button></div>
      {capture ? <div className="context-grid">
        <div className="context-item"><label>Capture {statusReady ? "" : "(last known)"}</label><strong>{capture.active ? "Recording" : `Stopped: ${capture.stop_reason}`}</strong><code>{capture.recording_id}</code></div>
        <div className="context-item"><label>Frames</label><span>{capture.frame_count} / {capture.max_frames}</span></div>
        <div className="context-item"><label>Encoded file budget</label><span>{capture.captured_bytes.toLocaleString()} / {capture.max_bytes.toLocaleString()} bytes</span></div>
        <div className="context-item"><label>Storage format</label><span>{capture.storage_format ?? "Not reported by this backend"}</span></div>
        <div className="context-item"><label>Last captured offset</label><span>{capture.duration_seconds.toFixed(3)} seconds</span></div>
        {capture.storage_format === "compact-v2" && <>
          <div className="context-item"><label>Reconstructed-state work</label><span>{capture.decoded_state_bytes?.toLocaleString()} / {capture.max_decoded_state_bytes?.toLocaleString()} bytes across all states</span></div>
          <div className="context-item"><label>Explicit edits</label><span>{capture.edit_count?.toLocaleString()} / 100,000</span></div>
          <div className="context-item"><label>Per-state bound</label><span>{capture.max_state_bytes?.toLocaleString()} bytes; no frames are sampled</span></div>
        </>}
      </div> : <p className="empty">{loading ? "Checking retained recording…" : statusReady ? "No recording retained by this backend." : "Recording status is unknown."}</p>}
      <p className="coverage-note">Limits stop capture; they never stop the federation or evict earlier captured states. Encoded file bytes and reconstruction work are not process-memory measurements. Event batches remain atomic, including envelopes immediately expired from ordinary history. The baseline preserves existing inspectors and explicitly retained history, not a complete earlier session. One-second checkpoints preserve clock-dependent fields between commits.</p>
    </article>
    <article className="panel" aria-label="Open portable recording"><h3>Open a recording</h3><p>Choose a versioned JSON file (up to 16 MiB). This backend validates every reconstructed state's named models, envelope cursors, timeline and bounds before the dashboard uses it. Compact v2 files use a complete baseline plus explicit edits, not sampled states; older v1 files remain supported. File paths, captured URLs and capabilities are never executed or followed.</p><label>Recording file<input type="file" accept=".json,application/json" aria-label="Recording file" disabled={!enabled || busy} onChange={(event) => setFile(event.target.files?.[0])} /></label><button className="secondary-button" type="button" disabled={!enabled || busy || !file} onClick={() => void act(async () => { if (file!.size > 16 * 1024 * 1024) throw new Error("Recording exceeds 16 MiB."); onReplay(await client.validateRecording(await file!.text())); })}>Open recording</button><p className="coverage-note">Recording files contain captured federation data, context, names/handles and provider extensions. Review them before sharing; they are not credential-scrubbed workspace configurations. No file is saved by the validation endpoint. Commands remain disabled throughout replay, even if the recording advertised control support.</p></article>
  </section>;
}
