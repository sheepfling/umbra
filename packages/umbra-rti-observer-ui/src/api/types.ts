import type { components } from "./generated";

type Generated = components["schemas"];

export type JsonValue =
  | null
  | boolean
  | number
  | string
  | JsonValue[]
  | { [key: string]: JsonValue };
export type JsonObject = { [key: string]: JsonValue };

export type CapabilityState = "available" | "partial" | "disabled" | "unsupported" | "unknown";
export type CapabilityResponse = Generated["CapabilityResponse"] & {
  state: CapabilityState;
  label: string;
  description: string;
  owner: string;
  source?: string | null;
  owners: string[];
  sources: string[];
  observations: JsonObject[];
  reason?: string | null;
  reasons: string[];
  event_types: string[];
  [key: string]: JsonValue | undefined;
};

export type EventEnvelope = Generated["EventEnvelope"] & {
  sequence: number;
  event_type: string;
  family: string;
  source: string;
  observed_at: number;
  [key: string]: JsonValue | undefined;
};

type GeneratedInspectors = Generated["InspectorsResponse"];
export type InspectorsResponse = GeneratedInspectors & {
  objects: NonNullable<GeneratedInspectors["objects"]>;
  interactions: NonNullable<GeneratedInspectors["interactions"]>;
};

type GeneratedState = Generated["StateResponse"];
export type StateResponse = GeneratedState & {
  context: JsonObject;
  adapters: NonNullable<GeneratedState["adapters"]>;
  capabilities: Record<string, CapabilityResponse>;
  federate_roster: NonNullable<GeneratedState["federate_roster"]>;
  inspectors: InspectorsResponse;
  normalized_events?: EventEnvelope[] | null;
};

type GeneratedContract = Generated["ContractResponse"];
export type ContractResponse = GeneratedContract & {
  capabilities: Record<string, CapabilityResponse>;
};

type GeneratedEventPage = Generated["EventPageResponse"];
export type EventPageResponse = GeneratedEventPage & {
  events: EventEnvelope[];
};
