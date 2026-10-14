import type {
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
    this.contractPromise ??= this.request<ContractResponse>("/contract");
    return this.contractPromise;
  }

  async getState(includeEvents = true): Promise<StateResponse> {
    await this.getContract();
    return this.request<StateResponse>(`/state?include_events=${includeEvents}`);
  }

  async getEvents(after: number, limit = 500): Promise<EventPageResponse> {
    return this.request<EventPageResponse>(`/events?after=${after}&limit=${limit}`);
  }

  openStream(
    onMessage: (message: StreamMessage) => void,
    onStatus: (status: "open" | "closed" | "error") => void,
  ): WebSocket {
    const configuredPath = document
      .querySelector<HTMLMetaElement>('meta[name="observer-stream-path"]')
      ?.content || "/ws/events";
    const protocol = window.location.protocol === "https:" ? "wss" : "ws";
    const url = /^(wss?:)?\/\//.test(configuredPath)
      ? configuredPath
      : `${protocol}://${window.location.host}${configuredPath}`;
    const socket = new WebSocket(url);
    socket.addEventListener("open", () => onStatus("open"));
    socket.addEventListener("close", () => onStatus("closed"));
    socket.addEventListener("error", () => onStatus("error"));
    socket.addEventListener("message", (event) => {
      const parsed = parseStreamMessage(JSON.parse(String(event.data)));
      if (parsed) onMessage(parsed);
    });
    return socket;
  }

  private async request<T>(path: string): Promise<T> {
    const response = await fetch(`${this.apiBase}${path}`, {
      cache: "no-store",
      headers: { Accept: "application/json" },
    });
    if (!response.ok) {
      throw new ObserverApiError(response.status, `observer request failed: ${response.status}`);
    }
    return (await response.json()) as T;
  }
}

