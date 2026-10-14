import type { CapabilityResponse, CapabilityState } from "../api/types";

const states: CapabilityState[] = ["available", "partial", "disabled", "unsupported", "unknown"];

interface CapabilityGridProps {
  capabilities: Record<string, CapabilityResponse>;
  known: string[];
}

export function CapabilityGrid({ capabilities, known }: CapabilityGridProps) {
  const keys = [...new Set([...known, ...Object.keys(capabilities)])].sort();
  const usable = keys.filter((key) => isUsable(capabilities[key]?.state)).length;
  return (
    <section className="panel">
      <div className="panel-heading">
        <div>
          <p className="eyebrow">Feature surface</p>
          <h2>Capabilities</h2>
        </div>
        <span className="panel-note">{usable} usable · {keys.length} reported</span>
      </div>
      <div className="capability-grid">
        {keys.map((key) => {
          const capability = capabilities[key];
          const state = normalizeState(capability?.state);
          return (
            <article className="capability" key={key}>
              <div className="capability-title">
                <strong>{capability?.label ?? humanize(key)}</strong>
                <span className={`state-pill ${state}`}>{state}</span>
              </div>
              <p>{capability?.description ?? explanation(state)}</p>
              <small>{capability?.reason ?? capability?.owner ?? "Provider did not advertise ownership"}</small>
            </article>
          );
        })}
      </div>
    </section>
  );
}

function normalizeState(value: CapabilityState | undefined): CapabilityState {
  return value && states.includes(value) ? value : "unknown";
}

function isUsable(value: CapabilityState | undefined): boolean {
  return value === "available" || value === "partial";
}

function humanize(value: string): string {
  return value.replaceAll("_", " ").replace(/\b\w/g, (letter) => letter.toUpperCase());
}

function explanation(state: CapabilityState): string {
  if (state === "unsupported") return "The connected provider does not expose this feature.";
  if (state === "disabled") return "This feature is disabled for this observer.";
  if (state === "unknown") return "The provider did not advertise support.";
  if (state === "partial") return "Only part of this feature surface is available.";
  return "Available for observation.";
}
