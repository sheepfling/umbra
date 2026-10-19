import { useEffect, useMemo, useState } from "react";
import { ObserverApiError, ObserverClient } from "../api/client";
import type { AnalyticsResponse } from "../api/types";

export function useAnalytics(enabled: boolean, windowSeconds: number) {
  const client = useMemo(() => new ObserverClient(), []);
  const [data, setData] = useState<AnalyticsResponse>();
  const [error, setError] = useState<string>();
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    setData(undefined);
    setError(undefined);
    if (!enabled) return;
    let stopped = false;
    let timer: number | undefined;
    const controller = new AbortController();
    setLoading(true);
    const refresh = async () => {
      try {
        const next = await client.getAnalytics(windowSeconds, undefined,
          AbortSignal.any([controller.signal, AbortSignal.timeout(10_000)]));
        if (stopped) return;
        setData(next);
        setError(undefined);
      } catch (requestError) {
        if (stopped) return;
        setError(requestError instanceof ObserverApiError && [404, 501].includes(requestError.status)
          ? "This backend does not provide observation analytics. Events and inspectors are still available."
          : "Analytics could not refresh. The last successful window is shown; retrying shortly.");
      } finally {
        if (!stopped) {
          setLoading(false);
          timer = window.setTimeout(() => void refresh(), 2_000);
        }
      }
    };
    void refresh();
    return () => {
      stopped = true;
      controller.abort();
      window.clearTimeout(timer);
    };
  }, [client, enabled, windowSeconds]);

  return { data, error, loading };
}
