import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import { App } from "./app/App";
import { WorkspaceProvider } from "./workspace/WorkspaceContext";
import "./styles.css";
import "./workspace.css";

createRoot(document.getElementById("root")!).render(
  <StrictMode>
    <WorkspaceProvider><App /></WorkspaceProvider>
  </StrictMode>,
);
