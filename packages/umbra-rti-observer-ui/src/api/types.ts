import type { components } from "./generated";

type Generated = components["schemas"];

export type AnalyticsResponse = Generated["AnalyticsResponse"];
// The generator deliberately maps recursive JSON to unknown. Refine only the
// explicit JSON value boundary, retaining all generated named DTO fields.
export type ObjectPerspective = Generated["ObjectPerspectiveResponse"] & {
  object: Generated["ObjectInspectionResponse"] & { attributes: JsonObject };
};
export type ObjectAttributeChange = Generated["ObjectAttributeChangeResponse"] & {
  value: JsonValue; previous_value: JsonValue;
};
export type ObjectObservations = Omit<Generated["ObjectObservationsResponse"], "snapshots" | "changes"> & {
  snapshots: ObjectPerspective[]; changes: ObjectAttributeChange[];
};
export type FederateActivity = Generated["FederateActivity"];
export type TimeObservation = Generated["TimeObservation"];
export type TimeInspection = Generated["TimeInspectionResponse"];
export type TimeScope = Generated["TimeScopeResponse"];
export type TimeRequest = Generated["TimeRequestResponse"];
export type CommandCatalog = Generated["CommandCatalog"];
export type CommandHistory = Generated["CommandHistory"];
export type CommandRecord = Generated["CommandRecord"];
export type CommandTarget = Generated["CommandTarget"];
export type CommandDefinition = Generated["CommandDefinition"];
export type CommandRequest = CommandRecord["request"];
export type ResignPolicy = NonNullable<Generated["ResignParameters"]["policy"]>;

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
export type InspectorsResponse = Omit<GeneratedInspectors, "object_observations"> & {
  objects: NonNullable<GeneratedInspectors["objects"]>;
  interactions: NonNullable<GeneratedInspectors["interactions"]>;
  object_observations?: ObjectObservations | null;
};

type GeneratedState = Generated["StateResponse"];
export type StateResponse = Omit<GeneratedState, "inspectors"> & {
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
