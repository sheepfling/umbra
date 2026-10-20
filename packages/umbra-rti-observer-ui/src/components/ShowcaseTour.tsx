import type { WorkspaceSettings } from "../api/types";

const steps: { view: WorkspaceSettings["view"]; label: string; action: string }[] = [
  { view: "federation", label: "Federation", action: "Change the chart window and observation grouping." },
  { view: "federates", label: "Federates", action: "Switch participant perspectives and inspect attributed reports." },
  { view: "objects", label: "Objects", action: "Choose load as a numeric attribute, then compare perspectives." },
  { view: "declarations", label: "Declarations", action: "Filter explicit publications and subscriptions." },
  { view: "topology", label: "Topology", action: "Try all three layouts, select an edge, then focus a participant join." },
  { view: "interactions", label: "Interaction flow", action: "Compare paired transmissions with unpaired or unknown evidence." },
  { view: "ownership", label: "Ownership", action: "Compare reported owner state with acquisition and divestiture tickets." },
  { view: "regions", label: "Regions / DDM", action: "Inspect ranges and associations, including exact large bounds." },
  { view: "mom", label: "MOM reports", action: "Choose a counter history and inspect unresolved member reports." },
  { view: "fom", label: "FOM browser", action: "Follow supplied class, datatype and dimension references." },
  { view: "time", label: "Logical time", action: "Compare the rich request/grant ledger with the grant-only participant." },
  { view: "synchronization", label: "Synchronization", action: "Follow a paced barrier; inspect its local and collective progress cards." },
  { view: "persistence", label: "Save / restore", action: "Watch alternating collective success and failure after local completion." },
  { view: "events", label: "Events", action: "Search, filter, expand an envelope and export evidence." },
  { view: "inspectors", label: "Inspectors", action: "Inspect retained raw object, federate and interaction observations." },
  { view: "recordings", label: "Recordings", action: "Start a capture, stop it, replay it, seek, then return to live." },
  { view: "alerts", label: "Alerts", action: "Create an error rule, inspect evidence, and download its configuration." },
  { view: "actions", label: "Actions", action: "Connect and join the synthetic session. Register/achieve a barrier; request save, report begun/result, then restore that exact label." },
  { view: "diagnostics", label: "Diagnostics", action: "Check transport, capabilities and independent storage limits." },
];

export function ShowcaseTour({ limited, onNavigate }: { limited: boolean; onNavigate: (view: WorkspaceSettings["view"]) => void }) {
  return <section className="panel" aria-label="Demo walkthrough">
    <p className="eyebrow">Synthetic playtest</p><h2>Try every part of the observer</h2>
    <p>Traffic keeps running while you explore. Use Pause view to freeze the display. Saved workspaces remembers your view choices and exports a configuration file.</p>
    {limited ? <p className="notice-banner">Limited profile: timing, attributes, events and lifecycle controls remain usable. Unavailable inspectors explain their coverage; coordination controls are omitted.</p> :
      <p className="notice-banner">The rich demo cycles coordination phases over 40 traffic updates. A full pair of cycles includes successful and failed save/restore reports. Owned-session commands generate separate reports when you try them.</p>}
    <div className="showcase-tour-grid">{steps.map((step) => <div key={step.view}><button type="button" className="secondary-button" onClick={() => onNavigate(step.view)}>{step.label}</button><p>{step.action}</p></div>)}</div>
    <p className="coverage-note">Also try Pause / Resume, Sync now, Saved workspaces and the limited launcher. Demo controls affect the separate synthetic session; no real RTI is connected.</p>
  </section>;
}
