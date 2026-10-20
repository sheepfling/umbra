import assert from "node:assert/strict";
import test from "node:test";
import { buildCommand } from "../src/app/commandRequests.ts";

const legacy = { target_id: "owned" };
const target = { ...legacy, version: { instance_id: "registration", revision: 7 } };

test("older command backends receive the original lifecycle shape without conditional fields", () => {
  for (const action of ["session.connect", "session.disconnect", "federate.join", "federate.resign"]) {
    const request = buildCommand(action, legacy, { federation: "Allowed", federate_name: "Owner" }, true);
    assert.deepEqual(Object.keys(request).sort(), ["action", "confirmed", "parameters", "request_id", "target_id"]);
    assert.equal(request.target_id, "owned");
  }
});

test("each supported request binds the exact owned incarnation/revision, not a display name or observer instance", () => {
  for (const action of ["session.connect", "session.disconnect", "federate.join", "federate.resign", "session.inspect", "session.recover"]) {
    const request = buildCommand(action, target, {}, true);
    assert.deepEqual(request.expected_version, target.version);
    assert.equal(request.target_id, target.target_id);
  }
});

test("inspection and recovery never invent a missing target version or forward provider options", () => {
  for (const action of ["session.inspect", "session.recover"]) {
    assert.throws(() => buildCommand(action, legacy, {}, true), /owned target version/);
    assert.deepEqual(buildCommand(action, target, { credentials: "never-send", driver_factory: "never-run" }, true).parameters, {});
  }
});

test("lifecycle fields are whitelisted and resign policy is explicit", () => {
  assert.deepEqual(buildCommand("federate.join", target, { federation: "Allowed", federate_name: "Owner", arbitrary_provider_call: "ignored" }, false).parameters,
    { federation: "Allowed", federate_name: "Owner", federate_type: "Observer" });
  assert.deepEqual(buildCommand("federate.resign", target, {}, true).parameters, { policy: "no_action" });
  assert.throws(() => buildCommand("federate.resign", target, { policy: "destroy_everything" }, true), /supported resign policy/);
});

test("coordination requires owned versions, preserves exact labels, and whitelists named fields", () => {
  const specs = [
    ["sync.register", {label:"  exact  ",tag_text:"tag"}, {label:"  exact  ",tag_text:"tag"}],
    ["sync.achieve", {label:"  exact  ",result:"failed"}, {label:"  exact  ",result:"failed"}],
    ["save.request", {label:"  exact  "}, {label:"  exact  "}],
    ["save.begun", {}, {}],
    ["save.complete", {result:"completed"}, {result:"completed"}],
    ["restore.request", {label:"  exact  "}, {label:"  exact  "}],
    ["restore.complete", {result:"failed"}, {result:"failed"}],
  ];
  for (const [action, fields, expected] of specs) {
    assert.throws(() => buildCommand(action, legacy, fields, true), /owned target version/);
    const request = buildCommand(action, target, {...fields, provider_path:"never-send", method:"never-run"}, true);
    assert.deepEqual(request.parameters, expected);
    assert.deepEqual(request.expected_version, target.version);
  }
  assert.throws(() => buildCommand("sync.achieve", target, {label:"x",result:"maybe"}, true), /explicit synchronization/);
  assert.throws(() => buildCommand("save.complete", target, {result:"success-ish"}, true), /explicit local result/);
});
