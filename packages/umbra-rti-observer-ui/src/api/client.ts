import type {
  AnalyticsResponse,
  CommandCatalog,
  CommandHistory,
  CommandRecord,
  CommandRequest,
  ContractResponse,
  EventPageResponse,
  StateResponse,
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

