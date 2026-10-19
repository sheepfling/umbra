import { test as base, expect } from "@playwright/test";
import { spawn, execFile, type ChildProcess } from "node:child_process";
import { existsSync } from "node:fs";
import { resolve, delimiter } from "node:path";
import { fileURLToPath } from "node:url";
import { promisify } from "node:util";

export type BackendVariant = "rich" | "limited" | "read-only" | "no-analytics" | "empty"
  | "java-full" | "java-retention-loss" | "java-limited" | "java-empty" | "java-partial" | "java-no-analytics";

interface ObserverFixtures {
  backendVariant: BackendVariant;
  observerArgs: string[];
  observerURL: string;
  frontendHealth: void;
}

const uiRoot = fileURLToPath(new URL("../", import.meta.url));
const root = resolve(uiRoot, "../..");
const privatePython = resolve(root, ".cache/observer-playtest/venv",
  process.platform === "win32" ? "Scripts/python.exe" : "bin/python");

async function stopBackend(child: ChildProcess): Promise<void> {
  if (child.exitCode !== null || child.signalCode !== null || !child.pid) return;
  const exited = new Promise<void>((done) => child.once("exit", () => done()));
  if (process.platform === "win32") {
    // Windows venv executables can redirect to a second Python process. Stop
    // only this owned test tree, not all Pythons or an existing playtest server.
    await promisify(execFile)("taskkill.exe", ["/PID", String(child.pid), "/T", "/F"])
      .catch((error: unknown) => { if (child.exitCode === null) throw error; });
  } else {
    child.kill("SIGTERM");
  }
  let timer: ReturnType<typeof setTimeout> | undefined;
  try {
    await Promise.race([
      exited,
      new Promise<never>((_, reject) => {
        timer = setTimeout(() => { child.kill("SIGKILL"); reject(new Error("Test backend did not stop")); }, 5_000);
      }),
    ]);
  } finally {
    clearTimeout(timer);
  }
}

export const test = base.extend<ObserverFixtures>({
  backendVariant: ["rich", { option: true }],
  observerArgs: [[], { option: true }],
  observerURL: async ({ backendVariant, observerArgs }, use, testInfo) => {
    const python = process.env.UMBRA_OBSERVER_TEST_PYTHON
      || (existsSync(privatePython) ? privatePython : "python");
    let program = python;
    let args = ["-u", "-m", "umbra_rti_observer", "--host", "127.0.0.1", "--port", "0"];
    if (backendVariant !== "empty") args.push("--showcase", "--interval", "0.15");
    if (["rich", "limited", "no-analytics"].includes(backendVariant)) args.push("--enable-controls");
    if (backendVariant === "limited") args.push("--profile", "limited");
    if (backendVariant === "no-analytics") args.push("--no-analytics");
    args.push(...observerArgs);
    if (backendVariant.startsWith("java-")) {
      program = process.env.UMBRA_OBSERVER_TEST_JAVA || "java";
      args = ["--add-modules", "jdk.httpserver",
        resolve(root, "packages/umbra-rti-observer-java-fixture/src/ObserverFixtureServer.java"),
        resolve(root, "packages/umbra-rti-observer/conformance/fixtures", backendVariant.slice(5)),
        resolve(uiRoot, "dist")];
    }
    const child = spawn(program, args, {
      cwd: root,
      windowsHide: true,
      env: {
        ...process.env,
        PYTHONHOME: "",
        PYTHONPATH: [resolve(root, "packages/umbra-rti-observer/src"), process.env.PYTHONPATH]
          .filter(Boolean).join(delimiter),
        UMBRA_OBSERVER_UI_DIST: resolve(uiRoot, "dist"),
        NO_COLOR: "1",
      },
      stdio: ["ignore", "pipe", "pipe"],
    });
    let log = "";
    const prerequisite = backendVariant.startsWith("java-")
      ? "Check JDK 17+ and the shared conformance fixture directory."
      : "Run the playtest setup or install umbra-rti-observer[web].";
    try {
      const url = await new Promise<string>((ready, reject) => {
        const timer = setTimeout(() => reject(new Error(`Backend startup timed out. ${prerequisite}\n${log}`)), 15_000);
        const cleanup = () => clearTimeout(timer);
        const output = (chunk: Buffer) => {
          log = (log + chunk.toString()).slice(-64_000);
          const match = log.match(/(?:Uvicorn running on |OBSERVER_FIXTURE_URL=)(http:\/\/127\.0\.0\.1:\d+)/);
          if (match?.[1]) { cleanup(); ready(match[1]); }
        };
        child.stdout?.on("data", output);
        child.stderr?.on("data", output);
        child.once("error", (error) => { cleanup(); reject(error); });
        child.once("exit", (code) => {
          cleanup();
          reject(new Error(`Test backend exited (${code}). ${prerequisite}\n${log}`));
        });
      });
      await use(url);
    } finally {
      await stopBackend(child);
      if (testInfo.status !== testInfo.expectedStatus) {
        await testInfo.attach("observer-backend.log", { body: log, contentType: "text/plain" });
      }
    }
  },
  baseURL: async ({ observerURL }, use) => { await use(observerURL); },
  frontendHealth: [async ({ page }, use) => {
    const errors: string[] = [];
    page.on("pageerror", (error) => errors.push(error.message));
    page.on("requestfailed", (request) => {
      if (request.resourceType() === "stylesheet") errors.push(`Stylesheet failed: ${request.url()} (${request.failure()?.errorText ?? "unknown network failure"})`);
    });
    page.on("response", (response) => {
      if (response.request().resourceType() === "stylesheet" && response.status() >= 400) errors.push(`Stylesheet HTTP ${response.status()}: ${response.url()}`);
    });
    await use();
    expect.soft(errors, "No uncaught frontend errors or missing stylesheets").toEqual([]);
  }, { auto: true }],
});

export { expect } from "@playwright/test";
