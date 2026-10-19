import { createContext, useCallback, useContext, useEffect, useRef, useState } from "react";
import type { Dispatch, ReactNode, SetStateAction } from "react";
import type { WorkspaceDocument, WorkspaceLibrary, WorkspaceSettings } from "../api/types";
import { decodeLibrary, freshLibrary, parseLibrary, parseWorkspace, STORAGE_KEY } from "./codec";

type Section = { [K in keyof WorkspaceSettings]: WorkspaceSettings[K] extends object
  ? WorkspaceSettings[K] extends readonly unknown[] ? never : K : never }[keyof WorkspaceSettings];
type UpdateRoot = <K extends keyof WorkspaceSettings>(key: K, action: SetStateAction<WorkspaceSettings[K]>) => void;
type UpdateField = <S extends Section, K extends keyof WorkspaceSettings[S]>(section: S, key: K, action: SetStateAction<WorkspaceSettings[S][K]>) => void;
type PerspectiveSections = Pick<WorkspaceSettings, "events" | "declarations" | "time">;
type UpdateFederateField = <S extends keyof PerspectiveSections, K extends keyof WorkspaceSettings[S]>(participant: string, section: S, key: K, action: SetStateAction<WorkspaceSettings[S][K]>) => void;

interface WorkspaceOwner {
  settings: WorkspaceSettings;
  saved: WorkspaceDocument[];
  generation: number;
  error?: string;
  notice?: string;
  blocked: boolean;
  update: UpdateRoot;
  updateField: UpdateField;
  updateFederateField: UpdateFederateField;
  apply: (document: WorkspaceDocument) => void;
  save: (document: WorkspaceDocument, replace?: WorkspaceDocument) => void;
  remove: (name: string) => void;
  reload: () => void;
  reset: () => void;
  announce: (message?: string) => void;
}

function initialLibrary(): { library: WorkspaceLibrary; raw: string | null; blocked: boolean; error?: string } {
  let raw: string | null = null;
  try {
    raw = window.localStorage.getItem(STORAGE_KEY);
    return { library: raw === null ? freshLibrary() : decodeLibrary(raw), raw, blocked: false };
  } catch {
    return { library: freshLibrary(), raw, blocked: raw !== null,
      error: raw !== null ? "Stored workspaces are invalid or from another version. The original browser copy was kept unchanged; observation views still work."
        : "Browser storage is unavailable. Settings work in this tab; download a workspace to keep them." };
  }
}

const WorkspaceContext = createContext<WorkspaceOwner | null>(null);

function settingsForParticipant(settings: WorkspaceSettings, participant?: string): WorkspaceSettings {
  if (participant === undefined) return settings;
  const entry = settings.federate_views.find((row) => row.participant === participant);
  // A read-only overlay: never replace the global settings in the owner.
  return { ...settings, events: entry?.events ?? settings.federate_events,
    declarations: entry?.declarations ?? settings.federate_declarations, time: entry?.time ?? settings.federate_time };
}

export function WorkspaceProvider({ children }: { children: ReactNode }) {
  const [initial] = useState(initialLibrary);
  const [settings, setSettings] = useState(initial.library.current);
  const [saved, setSaved] = useState(initial.library.saved);
  const [generation, setGeneration] = useState(0);
  const [error, setError] = useState(initial.error);
  const [notice, setNotice] = useState<string>();
  const [blocked, setBlocked] = useState(initial.blocked);
  const knownRaw = useRef(initial.raw);
  const protectedCopy = useRef(initial.blocked);
  const latest = useRef({ settings, saved });
  latest.current = { settings, saved };

  const update: UpdateRoot = useCallback((key, action) => {
    setSettings((previous) => ({ ...previous, [key]: typeof action === "function"
      ? (action as (value: typeof previous[typeof key]) => typeof previous[typeof key])(previous[key]) : action }));
  }, []);
  const updateField: UpdateField = useCallback((section, key, action) => {
    setSettings((previous) => {
      const old = previous[section][key];
      const value = typeof action === "function" ? (action as (value: typeof old) => typeof old)(old) : action;
      return { ...previous, [section]: { ...previous[section], [key]: value } };
    });
  }, []);
  const updateFederateField: UpdateFederateField = useCallback((participant, section, key, action) => {
    setSettings((previous) => {
      const scoped = settingsForParticipant(previous, participant);
      const current = previous.federate_views.find((row) => row.participant === participant) ?? {
        participant, events: previous.federate_events, declarations: previous.federate_declarations, time: previous.federate_time,
      };
      const old = scoped[section][key];
      const value = typeof action === "function" ? (action as (value: typeof old) => typeof old)(old) : action;
      const next = { ...current, [section]: { ...scoped[section], [key]: value } };
      return { ...previous, federate_views: [...previous.federate_views.filter((row) => row.participant !== participant), next].slice(-64) };
    });
  }, []);

  useEffect(() => {
    const flush = () => {
      if (protectedCopy.current) return;
      try {
        const library = parseLibrary({ library_version: "umbra-observer-workspace-library-v1", current: latest.current.settings, saved: latest.current.saved });
        const current = window.localStorage.getItem(STORAGE_KEY);
        if (current !== knownRaw.current) {
          protectedCopy.current = true; setBlocked(true);
          setError("Saved workspaces changed in another tab. Export this view if needed, then reload the browser copy before saving again.");
          return;
        }
        const raw = JSON.stringify(library);
        window.localStorage.setItem(STORAGE_KEY, raw); knownRaw.current = raw; setError(undefined);
      } catch {
        setError("Current settings could not be saved: storage is unavailable/full or configuration exceeds its bounds. Existing browser data was kept; observation views still work.");
      }
    };
    const timer = window.setTimeout(flush, 150);
    window.addEventListener("beforeunload", flush);
    return () => { window.clearTimeout(timer); window.removeEventListener("beforeunload", flush); };
  }, [settings, saved]);

  useEffect(() => {
    const changed = (event: StorageEvent) => {
      if ((event.key === STORAGE_KEY || event.key === null) && event.newValue !== knownRaw.current) {
        protectedCopy.current = true; setBlocked(true);
        setError("Saved workspaces changed in another tab. The current view was not replaced; reload its browser copy explicitly.");
      }
    };
    window.addEventListener("storage", changed);
    return () => window.removeEventListener("storage", changed);
  }, []);

  const apply = (document: WorkspaceDocument) => {
    const valid = parseWorkspace(document);
    latest.current = { ...latest.current, settings: valid.settings };
    setSettings(valid.settings); setGeneration((value) => value + 1);
    setNotice(`Applied workspace “${valid.name}”. No backend settings or provider actions were changed.`);
  };
  const save = (document: WorkspaceDocument, replace?: WorkspaceDocument) => {
    if (protectedCopy.current) throw new Error("Reload or reset the protected browser copy before modifying saved workspaces.");
    const valid = parseWorkspace(document);
    const existing = latest.current.saved.find((row) => row.name === valid.name);
    if (existing && (!replace || JSON.stringify(existing) !== JSON.stringify(replace))) {
      throw new Error("This saved workspace changed before replacement was confirmed. Review it and try again.");
    }
    if (!existing && latest.current.saved.length >= 12) {
      throw new Error("At most 12 named workspaces can be saved. Download or delete one before adding another.");
    }
    const next = parseLibrary({ library_version: "umbra-observer-workspace-library-v1", current: latest.current.settings,
      saved: [...latest.current.saved.filter((row) => row.name !== valid.name), valid] });
    latest.current = { ...latest.current, saved: next.saved };
    setSaved(next.saved); setNotice(`Saved workspace “${valid.name}”. Download it for a portable backup.`);
  };
  const remove = (name: string) => {
    if (protectedCopy.current) throw new Error("Reload the protected browser copy first.");
    const remaining = latest.current.saved.filter((row) => row.name !== name);
    latest.current = { ...latest.current, saved: remaining }; setSaved(remaining);
  };
  const reload = () => {
    const next = initialLibrary();
    if (next.error) throw new Error(next.error);
    knownRaw.current = next.raw; protectedCopy.current = false;
    latest.current = { settings: next.library.current, saved: next.library.saved };
    setBlocked(false); setError(undefined); setSettings(next.library.current); setSaved(next.library.saved);
    setGeneration((value) => value + 1); setNotice("Reloaded browser workspaces. No backend actions were sent.");
  };
  const reset = () => {
    // Only this app's exact key; never erase other application/browser storage.
    window.localStorage.removeItem(STORAGE_KEY);
    knownRaw.current = null; protectedCopy.current = false;
    const next = freshLibrary(); latest.current = { settings: next.current, saved: next.saved };
    setBlocked(false); setSettings(next.current); setSaved(next.saved);
    setGeneration((value) => value + 1); setError(undefined); setNotice("Reset this browser's workspace settings only. Federation data and backend configuration were not changed.");
  };

  return <WorkspaceContext.Provider value={{ settings, saved, generation, error, notice, blocked, update, updateField,
    updateFederateField, apply, save, remove, reload, reset, announce: setNotice }}>{children}</WorkspaceContext.Provider>;
}

export function useWorkspaces(): WorkspaceOwner {
  const owner = useContext(WorkspaceContext);
  if (!owner) throw new Error("Workspace preferences require their settings owner.");
  return owner;
}

export function useWorkspaceRoot<K extends keyof WorkspaceSettings>(key: K): [WorkspaceSettings[K], Dispatch<SetStateAction<WorkspaceSettings[K]>>] {
  const { settings, update } = useWorkspaces();
  return [settings[key], useCallback((action) => update(key, action), [key, update])];
}

export function useWorkspaceField<S extends Section, K extends keyof WorkspaceSettings[S]>(section: S, key: K): [WorkspaceSettings[S][K], Dispatch<SetStateAction<WorkspaceSettings[S][K]>>] {
  const { settings, updateField } = useWorkspaces();
  return [settings[section][key], useCallback((action) => updateField(section, key, action), [section, key, updateField])];
}

export function useScopedWorkspaceField<S extends keyof PerspectiveSections, K extends keyof WorkspaceSettings[S]>(section: S, key: K, participant?: string): [WorkspaceSettings[S][K], Dispatch<SetStateAction<WorkspaceSettings[S][K]>>] {
  const { settings, updateField, updateFederateField, announce } = useWorkspaces();
  const entry = settings.federate_views.find((row) => row.participant === participant);
  const value = settingsForParticipant(settings, participant)[section][key];
  const capped = settings.federate_views.length === 64 && !entry;
  const setter = useCallback((action: SetStateAction<WorkspaceSettings[S][K]>) => {
    if (participant === undefined) updateField<S, K>(section, key, action);
    else {
      if (capped) announce("Federate preferences have a 64-perspective limit. The oldest saved perspective expired; no observations were removed.");
      updateFederateField(participant, section, key, action);
    }
  }, [participant, section, key, updateField, updateFederateField, capped, announce]);
  return [value, setter];
}

export function useScopedTimeChoice(participant?: string): [string | undefined, Dispatch<SetStateAction<string | undefined>>] {
  const [selected, update] = useScopedWorkspaceField("time", "selected", participant);
  return [selected ?? undefined, useCallback((action) => update((old) => (typeof action === "function" ? action(old ?? undefined) : action) ?? null), [update])];
}

type ChoiceSection = "time" | "federate_time" | "regions" | "fom" | "mom";
export function useWorkspaceChoice(section: ChoiceSection): [string | undefined, Dispatch<SetStateAction<string | undefined>>] {
  const { settings, updateField } = useWorkspaces();
  const selected = section === "mom" ? settings.mom.counter : settings[section].selected;
  const setter = useCallback((action: SetStateAction<string | undefined>) => {
    const next = (old: string | null) => (typeof action === "function" ? action(old ?? undefined) : action) ?? null;
    if (section === "mom") updateField("mom", "counter", next);
    else updateField(section, "selected", next);
  }, [section, updateField]);
  return [selected ?? undefined, setter];
}

export function useObjectChart(scope: string | null, objectKey: string): [string, (attribute: string) => void] {
  const { settings, update, announce } = useWorkspaces();
  const current = settings.object_charts.find((row) => row.scope === scope && row.object_key === objectKey)?.attribute ?? "";
  return [current, (attribute) => {
    if (settings.object_charts.length === 64 && !settings.object_charts.some((row) => row.scope === scope && row.object_key === objectKey)) {
      announce("Numeric chart preferences have a 64-target limit. The oldest target preference expired; no observations were removed.");
    }
    update("object_charts", (old) => [...old.filter((row) => row.scope !== scope || row.object_key !== objectKey),
      ...(attribute ? [{ scope, object_key: objectKey, attribute }] : [])].slice(-64));
  }];
}
