import { RecordingDecoder } from "./recordingCodec";
import type { ReplayWorkerRequest, ReplayWorkerResponse } from "./recordingCodec";

let decoder: RecordingDecoder | undefined;
let desired = 0, running = false, failed = false;
const send = (message: ReplayWorkerResponse) => postMessage(message);
async function run() {
  if (running || failed || !decoder) return;
  running = true;
  try {
    while (decoder.index !== desired) {
      if (desired < decoder.index) decoder.reset();
      decoder.advance();
      // Yield so a newer seek can replace queued work. No captured URL is read.
      await new Promise<void>((resolve) => setTimeout(resolve, 0));
    }
    if (decoder.frame) send({ type: "frame", index: decoder.index, frame: decoder.frame, samples: decoder.samples });
  } catch { failed = true; send({ type: "error" }); }
  finally { running = false; }
}
onmessage = (event: MessageEvent<ReplayWorkerRequest>) => {
  try {
    if (event.data.type === "load") { decoder = new RecordingDecoder(event.data.document); desired = 0; failed = false; }
    else {
      if (!decoder || !Number.isSafeInteger(event.data.index) || event.data.index < 0 || event.data.index >= decoder.document.frames.length) throw new Error("Invalid seek.");
      desired = event.data.index;
    }
    void run();
  } catch { failed = true; send({ type: "error" }); }
};
