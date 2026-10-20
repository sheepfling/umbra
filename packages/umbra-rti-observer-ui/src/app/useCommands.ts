import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { ObserverClient } from "../api/client";
import type { CommandCatalog, CommandHistory, CommandRequest } from "../api/types";

export function useCommands(enabled: boolean, paused: boolean) {
  const client = useMemo(() => new ObserverClient(), []);
  const [catalog, setCatalog] = useState<CommandCatalog>();
  const [history, setHistory] = useState<CommandHistory>();
  const [error, setError] = useState<string>();
  const active = useRef(true);
  const mutationVersion = useRef(0);
  const refreshSequence = useRef(0);
  const refresh = useCallback(async (signal?: AbortSignal) => {
    if (!active.current || signal?.aborted) return;
    const version = mutationVersion.current;
    const sequence = ++refreshSequence.current;
    const [nextCatalog, nextHistory] = await Promise.all([client.getCommandCatalog(signal), client.getCommandHistory(signal)]);
    if (signal?.aborted || !active.current || version !== mutationVersion.current || sequence !== refreshSequence.current) return;
    setCatalog(nextCatalog);
    setHistory(nextHistory);
    setError(undefined);
  }, [client]);

  useEffect(() => {
    active.current = enabled && !paused;
    if (!enabled || paused) return;
    const controller = new AbortController();
    let timer: number | undefined;
    const poll = async () => {
      const version = mutationVersion.current;
      try {
        await refresh(AbortSignal.any([controller.signal, AbortSignal.timeout(10_000)]));
      } catch {
        if (!controller.signal.aborted && version === mutationVersion.current) setError("Session controls could not refresh. The last result is shown; retrying shortly.");
      } finally {
        if (!controller.signal.aborted) timer = window.setTimeout(() => void poll(), 1_000);
      }
    };
    void poll();
    return () => { controller.abort(); window.clearTimeout(timer); };
  }, [enabled, paused, refresh]);

  useEffect(() => () => { active.current = false; }, []);

  const submit = useCallback(async (request: CommandRequest) => {
    if (!active.current) throw new Error("Resume a live, enabled session view before submitting commands.");
    mutationVersion.current++;
    try {
      const record = await client.submitCommand(request);
      mutationVersion.current++;
      await refresh().catch(() => { if (active.current) setError("The command was accepted, but its status could not refresh yet."); });
      return record;
    } finally { mutationVersion.current++; }
  }, [client, refresh]);

  return { catalog, history, error, submit };
}
