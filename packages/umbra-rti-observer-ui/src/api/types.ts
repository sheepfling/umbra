import type { components } from "./generated";

type Generated = components["schemas"];
export type WorkspaceDocument = Generated["WorkspaceDocument-Output"];
export type WorkspaceLibrary = Generated["WorkspaceLibrary"];
export type WorkspaceSettings = Generated["WorkspaceSettings-Output"];
export type AlertReport = Generated["AlertReport"];
export type AlertConfiguration = Generated["AlertConfiguration"];
export type AlertRule = AlertConfiguration["rules"][number];
export type AlertResult = Generated["AlertResultModel"];
export type StorageCoverage = Generated["StorageCoverageResponse"];

export type AnalyticsResponse = Generated["AnalyticsResponse"];
// The generator deliberately maps recursive JSON to unknown. Refine only the
// explicit JSON value boundary, retaining all generated named DTO fields.
export type ObjectPerspective = Generated["ObjectPerspectiveResponse-Output"] & {
  object: Generated["ObjectInspectionResponse-Output"] & { attributes: JsonObject };
};
export type ObjectAttributeChange = Generated["ObjectAttributeChangeResponse-Output"] & {
  value: JsonValue; previous_value: JsonValue;
};
export type ObjectObservations = Omit<Generated["ObjectObservationsResponse-Output"], "snapshots" | "changes"> & {
  snapshots: ObjectPerspective[]; changes: ObjectAttributeChange[];
};
export type FederateActivity = Generated["FederateActivity"];
export type TimeObservation = Generated["TimeObservation"];
export type TimeInspection = Generated["TimeInspectionResponse"];
export type Declarations = Generated["DeclarationsResponse"];
export type DeclarationSnapshot = Generated["DeclarationSnapshotResponse"];
export type RelationshipSnapshot = Generated["RelationshipSnapshotResponse"];
export type ParticipantIdentity = Generated["ParticipantIdentityResponse"];
export type CoordinationSnapshot = Generated["CoordinationSnapshotResponse"];
export type InteractionFlowRecord = Generated["InteractionFlowRecordResponse"];
export type InteractionFlowReport = Generated["InteractionFlowReportResponse"];
export type OwnershipReport = Generated["OwnershipResponse"]["transitions"][number];
export type OwnershipTicket = Generated["OwnershipTicketResponse"];
export type RegionReport = Generated["RegionsResponse"]["snapshots"][number];
export type DimensionRange = Generated["DimensionRangeResponse"];
export type MomReport = Generated["MomResponse"]["snapshots"][number];
export type ParticipantReference = Generated["ParticipantReferenceResponse"];
export type FomReport = Generated["FomResponse"]["snapshots"][number];
export type FomField = Generated["FomFieldResponse"];
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
export type CapabilityResponse = Generated["CapabilityResponse-Output"] & {
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

export type EventEnvelope = Generated["EventEnvelope-Output"] & {
  sequence: number;
  event_type: string;
  family: string;
  source: string;
  observed_at: number;
  [key: string]: JsonValue | undefined;
};

type GeneratedInspectors = Generated["InspectorsResponse-Output"];
export type InspectorsResponse = Omit<GeneratedInspectors, "object_observations"> & {
  objects: NonNullable<GeneratedInspectors["objects"]>;
  interactions: NonNullable<GeneratedInspectors["interactions"]>;
  object_observations?: ObjectObservations | null;
};

type GeneratedState = Generated["StateResponse-Output"];
export type StateResponse = Omit<GeneratedState, "inspectors"> & {
  context: JsonObject;
  adapters: NonNullable<GeneratedState["adapters"]>;
  capabilities: Record<string, CapabilityResponse>;
  federate_roster: NonNullable<GeneratedState["federate_roster"]>;
  inspectors: InspectorsResponse;
  normalized_events?: EventEnvelope[] | null;
};

type GeneratedContract = Generated["ContractResponse-Output"];
export type ContractResponse = GeneratedContract & {
  capabilities: Record<string, CapabilityResponse>;
};

type GeneratedEventPage = Generated["EventPageResponse"];
export type EventPageResponse = GeneratedEventPage & {
  events: EventEnvelope[];
};

export type CaptureStatus = Generated["CaptureStatus"];
export type RecordingStatusResponse = Generated["RecordingStatusResponse"];
export type RecordingFrame = Omit<Generated["RecordingFrame-Output"], "state" | "arrivals"> & {
  state: StateResponse; arrivals: EventEnvelope[];
};
export type RecordingDocument = Omit<Generated["RecordingDocument-Output"], "contract" | "frames"> & {
  contract: ContractResponse; frames: RecordingFrame[];
};
export type RecordingEdit =
  | (Omit<Generated["RecordingSetEdit-Output"], "value"> & { value: JsonValue })
  | Generated["RecordingDeleteEdit"]
  | (Omit<Generated["RecordingSpliceEdit-Output"], "items"> & { items: JsonValue[] });
export type RecordingDeltaFrame = Omit<Generated["RecordingDeltaFrame-Output"], "arrivals" | "edits"> & {
  arrivals: EventEnvelope[]; edits: RecordingEdit[];
};
export type CompactRecordingDocument = Omit<Generated["CompactRecordingDocument-Output"], "contract" | "baseline" | "frames"> & {
  contract: ContractResponse; baseline: StateResponse; frames: RecordingDeltaFrame[];
};
export type PortableRecording = RecordingDocument | CompactRecordingDocument;
