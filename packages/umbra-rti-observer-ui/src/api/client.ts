import type {
  AlertConfiguration,
  AlertReport,
  AnalyticsResponse,
  CommandCatalog,
  CommandHistory,
  CommandRecord,
  CommandRequest,
  ContractResponse,
  EventPageResponse,
  StateResponse,
  CaptureStatus,
  PortableRecording,
  RecordingFrame,
  RecordingStatusResponse,
} from "./types";
import { parseStreamMessage } from "./stream";
import type { StreamMessage } from "./stream";

export class ObserverApiError extends Error {
  readonly status: number;

  constructor(status: number, message: string) {
    super(message);
    this.name = "ObserverApiError";
    this.status = status;
  }
}

export class ObserverClient {
  private readonly apiBase: string;
  private contractPromise: Promise<ContractResponse> | undefined;

  constructor(apiBase = "/api") {
    this.apiBase = apiBase.replace(/\/$/, "");
  }

  async getContract(): Promise<ContractResponse> {
    this.contractPromise ??= this.request<ContractResponse>(`${this.apiBase}/contract`).then((contract) => {
      if (contract?.contract_version !== "umbra-observer-contract-v1") {
        throw new ObserverApiError(409, "Incompatible observer contract; expected umbra-observer-contract-v1.");
      }
      return contract;
    }).catch((error: unknown) => {
      this.contractPromise = undefined;
      throw error;
    });
    return this.contractPromise;
  }

  async getAlerts(signal?: AbortSignal): Promise<AlertReport> {
    return this.request<AlertReport>(await this.resource("alerts"), signal);
  }

  async configureAlerts(configuration: AlertConfiguration, revision: number, instanceId: string): Promise<AlertReport> {
    return this.request<AlertReport>(await this.resource("alert_configuration"), undefined,
      { method: "PUT", body: JSON.stringify({ expected_instance_id: instanceId, expected_revision: revision, configuration }) });
  }

  async importAlerts(raw: string, revision: number, instanceId: string): Promise<AlertReport> {
    if (new TextEncoder().encode(raw).length > 128 * 1024) throw new ObserverApiError(413, "Alert configuration file exceeds 128 KiB.");
    let configuration: unknown;
    try { configuration = JSON.parse(raw); } catch { throw new ObserverApiError(422, "Alert configuration is not valid JSON."); }
    // Untrusted file input is validated by the endpoint's named discriminated
    // models. Never cast it into a rule or follow an imported resource URL.
    return this.request<AlertReport>(await this.resource("alert_configuration"), undefined,
      { method: "PUT", body: JSON.stringify({ expected_instance_id: instanceId, expected_revision: revision, configuration }) });
  }

  async resetAlert(ruleId: string, revision: number, instanceId: string): Promise<AlertReport> {
    return this.request<AlertReport>(await this.resource("alert_reset"), undefined,
      { method: "POST", body: JSON.stringify({ expected_instance_id: instanceId, expected_revision: revision, rule_id: ruleId }) });
  }

  async acknowledgeAlert(evidenceId: number, instanceId: string): Promise<AlertReport> {
    return this.request<AlertReport>(await this.resource("alert_acknowledge"), undefined,
      { method: "POST", body: JSON.stringify({ expected_instance_id: instanceId, evidence_id: evidenceId }) });
  }

  async getState(includeEvents = true): Promise<StateResponse> {
    return this.request<StateResponse>(`${await this.resource("state")}?include_events=${includeEvents}`);
  }

  async getEvents(after: number, limit = 500): Promise<EventPageResponse> {
    return this.request<EventPageResponse>(`${await this.resource("events")}?after=${after}&limit=${limit}`);
  }

  async getAnalytics(windowSeconds: number, federate?: string, signal?: AbortSignal): Promise<AnalyticsResponse> {
    const query = new URLSearchParams({ window_seconds: String(windowSeconds), bucket_count: "30" });
    if (federate) query.set("federate", federate);
    return this.request<AnalyticsResponse>(`${await this.resource("analytics")}?${query}`, signal);
  }

  async getCommandCatalog(signal?: AbortSignal): Promise<CommandCatalog> {
    return this.request<CommandCatalog>(await this.resource("command_catalog"), signal);
  }

  async getCommandHistory(signal?: AbortSignal): Promise<CommandHistory> {
    return this.request<CommandHistory>(await this.resource("commands"), signal);
  }

  async submitCommand(request: CommandRequest): Promise<CommandRecord> {
    return this.request<CommandRecord>(await this.resource("commands"), undefined, { method: "POST", body: JSON.stringify(request) });
  }

  async getRecordingStatus(signal?: AbortSignal): Promise<RecordingStatusResponse> {
    return this.request<RecordingStatusResponse>(await this.resource("recordings"), signal);
  }

  async startRecording(label: string, replaceId?: string): Promise<CaptureStatus> {
    const state = (await this.getContract()).capabilities.recording_compact?.state;
    const compact = state === "available" || state === "partial";
    return this.request<CaptureStatus>(await this.resource("recording_start"), undefined,
      { method: "POST", body: JSON.stringify({ label, replace_id: replaceId ?? null, max_frames: 600, max_bytes: 16 * 1024 * 1024,
        ...(compact ? { storage_format: "compact-v2" } : {}) }) });
  }

  async stopRecording(recordingId: string): Promise<CaptureStatus> {
    return this.request<CaptureStatus>(await this.resource("recording_stop"), undefined,
      { method: "POST", body: JSON.stringify({ recording_id: recordingId }) });
  }

  async getRecording(recordingId: string): Promise<PortableRecording> {
    const status = await this.getRecordingStatus();
    if (status.recording?.recording_id !== recordingId) throw new ObserverApiError(409, "The retained recording changed. Refresh its status before exporting.");
    const query = new URLSearchParams({ recording_id: recordingId });
    if (status.recording.storage_format === "compact-v2") {
      const state = (await this.getContract()).capabilities.recording_compact?.state;
      if (state !== "available" && state !== "partial") throw new ObserverApiError(501, "This retained file requires a backend advertising compact v2 support.");
      query.set("storage_format", "compact-v2");
    }
    return this.request<PortableRecording>(`${await this.resource("recording_export")}?${query}`, AbortSignal.timeout(60_000));
  }

  async validateRecording(raw: string): Promise<PortableRecording> {
    try {
      if (new TextEncoder().encode(raw).length > 16 * 1024 * 1024) throw new ObserverApiError(413, "Recording exceeds 16 MiB.");
      let parsed: unknown;
      try { parsed = JSON.parse(raw); } catch { throw new ObserverApiError(422, "Recording is not valid JSON."); }
      const version = parsed !== null && typeof parsed === "object" && "recording_version" in parsed ? parsed.recording_version : undefined;
      if (version === "umbra-observer-recording-v2") {
        const state = (await this.getContract()).capabilities.recording_compact?.state;
        if (state !== "available" && state !== "partial") throw new ObserverApiError(501, "This backend does not advertise compact v2 recording validation. V1 recordings remain usable.");
      }
      return await this.request<PortableRecording>(await this.resource("recording_validate"), AbortSignal.timeout(60_000), { method: "POST", body: raw });
    } catch (error) {
      if (error instanceof ObserverApiError && error.status === 422) throw new ObserverApiError(422,
        "Recording rejected by the validated contract. Check its version, timeline, fields and bounds; the live view was not replaced.");
      throw error;
    }
  }

  async analyzeRecording(frame: RecordingFrame, windowSeconds: number, signal?: AbortSignal): Promise<AnalyticsResponse> {
    const contract = await this.getContract();
    const capability = contract.capabilities.recording_analytics_window?.state;
    const projected = capability === "available" || capability === "partial";
    const storageState = contract.capabilities.storage_inspection?.state;
    const parameters = projected ? {
      window: { contract_version: frame.state.contract_version, retention: frame.state.retention,
        normalized_events: frame.state.normalized_events ?? [], federate_roster: frame.state.federate_roster,
        ...(storageState === "available" || storageState === "partial" ? { storage: frame.state.storage } : {}) },
      captured_at: frame.captured_at, window_seconds: windowSeconds, bucket_count: 30,
    } : { frame, window_seconds: windowSeconds, bucket_count: 30 };
    return this.request<AnalyticsResponse>(await this.resource("recording_analyze"), signal,
      { method: "POST", body: JSON.stringify(parameters) });
  }

  async openStream(
    onMessage: (message: StreamMessage) => void,
    onStatus: (status: "open" | "closed" | "error") => void,
  ): Promise<WebSocket | undefined> {
    const contract = await this.getContract();
    const state = contract.capabilities.websocket?.state;
    if (state !== "available" && state !== "partial") return undefined;
    const configuredPath = await this.resource("websocket");
    const protocol = window.location.protocol === "https:" ? "wss" : "ws";
    const url = `${protocol}://${window.location.host}${configuredPath}`;
    const socket = new WebSocket(url);
    socket.addEventListener("open", () => onStatus("open"));
    socket.addEventListener("close", () => onStatus("closed"));
    socket.addEventListener("error", () => onStatus("error"));
    socket.addEventListener("message", (event) => {
      try {
        const parsed = parseStreamMessage(JSON.parse(String(event.data)));
        if (parsed) onMessage(parsed);
        else onStatus("error");
      } catch {
        onStatus("error");
      }
    });
    return socket;
  }

  private async resource(name: string): Promise<string> {
    const path = (await this.getContract()).api_paths?.[name];
    if (typeof path !== "string" || !path.startsWith("/") || path.startsWith("//")
      || path.includes("\\") || path.includes("?") || path.includes("#")
      || [...path].some((char) => char.charCodeAt(0) < 32)) {
      throw new ObserverApiError(501, `The backend did not advertise a safe ${name} resource path.`);
    }
    return path;
  }

  private async request<T>(path: string, signal?: AbortSignal, init?: Pick<RequestInit, "method" | "body">): Promise<T> {
    const response = await fetch(path, {
      cache: "no-store",
      headers: { Accept: "application/json", ...(init?.body ? { "Content-Type": "application/json" } : {}) },
      signal: signal ?? AbortSignal.timeout(10_000),
      ...init,
    });
    if (!response.ok) {
      const failure: unknown = await response.json().catch(() => undefined);
      const detail = failure !== null && typeof failure === "object" && "detail" in failure
        && typeof failure.detail === "string" ? failure.detail : `observer request failed: ${response.status}`;
      throw new ObserverApiError(response.status, detail);
    }
    return (await response.json()) as T;
  }
}

