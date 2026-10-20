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
  }
}

function resignPolicy(value: string): ResignPolicy {
  if (value === "no_action" || value === "unconditionally_divest_attributes" || value === "delete_objects"
    || value === "cancel_pending_ownership_acquisitions" || value === "delete_objects_then_divest"
    || value === "cancel_then_delete_then_divest") return value;
  throw new Error("Select a supported resign policy.");
}
