import type { CommandDefinition, CommandRequest, CommandTarget, ResignPolicy } from "../api/types";

/** Named wire requests only: provider configuration never comes from form fields. */
export function buildCommand(action: CommandDefinition["action"], target: CommandTarget,
  fields: Record<string, string>, confirmed: boolean): CommandRequest {
  const base = { request_id: crypto.randomUUID(), target_id: target.target_id, confirmed,
    ...(target.version ? { expected_version: target.version } : {}) };
  switch (action) {
    case "session.connect": return { ...base, action, parameters: {} };
    case "session.disconnect": return { ...base, action, parameters: {} };
    case "federate.join": return { ...base, action, parameters: { federation: fields.federation ?? "",
      federate_name: fields.federate_name ?? "", federate_type: fields.federate_type ?? "Observer" } };
    case "federate.resign": return { ...base, action, parameters: { policy: resignPolicy(fields.policy ?? "no_action") } };
    case "session.inspect":
    case "session.recover":
      if (!target.version) throw new Error("This backend did not provide the owned target version required for inspection/recovery.");
      return { ...base, action, expected_version: target.version, parameters: {} };
    case "sync.register":
      return { ...base, action, expected_version: requireVersion(target), parameters: { label: fields.label ?? "", tag_text: fields.tag_text ?? "" } };
    case "sync.achieve":
      if (fields.result !== "achieved" && fields.result !== "failed") throw new Error("Select an explicit synchronization result.");
      return { ...base, action, expected_version: requireVersion(target), parameters: { label: fields.label ?? "", result: fields.result } };
    case "save.request":
    case "restore.request":
      return { ...base, action, expected_version: requireVersion(target), parameters: { label: fields.label ?? "" } };
    case "save.begun":
      return { ...base, action, expected_version: requireVersion(target), parameters: {} };
    case "save.complete":
    case "restore.complete":
      if (fields.result !== "completed" && fields.result !== "failed") throw new Error("Select an explicit local result.");
      return { ...base, action, expected_version: requireVersion(target), parameters: { result: fields.result } };
  }
}

function requireVersion(target: CommandTarget) {
  if (!target.version) throw new Error("Refresh the owned target version before sending a coordination command.");
  return target.version;
}

export function isCoordinationAction(action: CommandDefinition["action"]): boolean {
  return action.startsWith("sync.") || action.startsWith("save.") || action.startsWith("restore.");
}

function resignPolicy(value: string): ResignPolicy {
  if (value === "no_action" || value === "unconditionally_divest_attributes" || value === "delete_objects"
    || value === "cancel_pending_ownership_acquisitions" || value === "delete_objects_then_divest"
    || value === "cancel_then_delete_then_divest") return value;
  throw new Error("Select a supported resign policy.");
}
