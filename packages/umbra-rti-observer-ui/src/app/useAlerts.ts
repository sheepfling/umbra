import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { ObserverApiError, ObserverClient } from "../api/client";
import type { AlertReport } from "../api/types";

export function useAlerts(enabled: boolean, paused: boolean) {
  const client = useMemo(() => new ObserverClient(), []);
  const [data, setData] = useState<AlertReport>();
  const [error, setError] = useState<string>();
  const [feedback, setFeedback] = useState<string>();
  const [busy, setBusy] = useState(false);
  const active = useRef(false);
  const mutationVersion = useRef(0);
  const writing = useRef(false);
  useEffect(() => {
    active.current = enabled && !paused;
    if (!enabled || paused) return;
    const controller = new AbortController();
    let timer: number | undefined;
    const poll = async () => {
      const version = mutationVersion.current;
      try {
        const report = await client.getAlerts(AbortSignal.any([controller.signal, AbortSignal.timeout(10000)]));
        if (!controller.signal.aborted && version === mutationVersion.current && !writing.current) { setData(report); setError(undefined); }
      } catch {
        if (!controller.signal.aborted && version === mutationVersion.current && !writing.current) setError("Alerts could not refresh. The last evidence is shown; controls are disabled until a successful refresh.");
      } finally {
        if (!controller.signal.aborted) timer = window.setTimeout(() => void poll(), 1000);
      }
    };
    void poll();
    return () => { active.current = false; controller.abort(); window.clearTimeout(timer); };
  }, [client, enabled, paused]);

  const mutate = useCallback(async (operation: () => Promise<AlertReport>) => {
    if (!active.current || writing.current) return false;
    mutationVersion.current += 1; writing.current = true;
    setBusy(true); setFeedback(undefined);
    try {
      const report = await operation();
      if (active.current) { setData(report); setError(undefined); }
      return true;
    } catch (failure) {
      if (active.current) setFeedback(failure instanceof ObserverApiError && failure.status === 409
        ? "Alert configuration changed in another client. Review the refreshed rules before retrying; nothing was reapplied automatically."
        : failure instanceof ObserverApiError && [400, 413, 422].includes(failure.status)
          ? "Alert configuration was rejected. Check its version, rule fields, thresholds and size; existing rules are unchanged."
          : "The alert update could not be confirmed. Refresh and review the rules before retrying.");
      return false;
    } finally { mutationVersion.current += 1; writing.current = false; setBusy(false); }
  }, []);
  return { client, data, error, feedback, busy, mutate };
}
