import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import test from "node:test";
import { eventFederate, eventParticipantKey, eventParticipantPerspective, legacyParticipantKey,
  objectKey, objectPerspectiveKey, participantChoices, participantKey, reportingScopeKey, unavailableParticipant } from "../src/app/participants.ts";

const cases = JSON.parse(readFileSync(new URL("../../umbra-rti-observer/conformance/participant-keys-v1.json", import.meta.url), "utf8"));

test("participant keys match the shared Python/TypeScript Unicode and identity corpus", () => {
  for (const row of cases) {
    assert.equal(eventParticipantKey(row.event, row.provenance), row.components === null ? undefined : JSON.stringify(row.components), row.name);
  }
  assert.equal(eventFederate(cases[0].event), "Same");
  assert.equal(eventFederate(cases[1].event), "Alias");
  assert.equal(eventFederate(cases[2].event), "Unnamed participant");
});

test("objects, time and declaration keys never use mutable labels for explicit joins", () => {
  const first = eventParticipantPerspective(cases[0].event, true), renamed = eventParticipantPerspective(cases[1].event, true);
  assert.equal(participantKey({ federate: "Same", perspective: first }), participantKey({ federate: "Alias", perspective: renamed }));
  const scope = { source: first.source, session_id: first.session_id, federation: null, federate: "Same", perspective: first };
  assert.equal(reportingScopeKey(scope), first.key);
  assert.equal(objectPerspectiveKey({ federate: "Alias", perspective: renamed }), first.key);
  assert.equal(objectPerspectiveKey({ federate: null }), null);
  const scopedObject = { object_key: "raw", object_context: { identity_namespace: "ns", federation_id: "fed" } };
  assert.equal(objectKey(scopedObject), '["federation-object","ns","fed","raw"]');
  assert.notEqual(objectKey({ object_key: objectKey(scopedObject) }), objectKey(scopedObject));
  assert.notEqual(objectKey({ ...scopedObject, object_key: " raw " }), objectKey(scopedObject));
  const legacy = { source: "a", session_id: null, federation: null, federate: "A" };
  assert.equal(reportingScopeKey(legacy), '["a",null,null,"A"]');
  assert.equal(participantKey(legacy), legacyParticipantKey("A"));
});

test("independent inspectors retain exact choices after roster/event expiry, without mining nested subjects", () => {
  const perspective = eventParticipantPerspective(cases[0].event, true);
  const renamed = eventParticipantPerspective(cases[1].event, true);
  const state = { capabilities: { participant_provenance: { state: "available" } }, federate_roster: [],
    inspectors: { object_observations: { snapshots: [{ federate: "Same", perspective }] },
      time: { snapshots: [], requests: [{ scope: { federate: "Same", perspective } }] },
      declarations: { snapshots: [{ scope: { federate: "Alias", perspective: renamed } }] },
      interactions: [{ federate: "Alias", perspective: renamed }], mom: { subject: { display_name: "Not an actor" } } } };
  assert.deepEqual(participantChoices(state).map((row) => [row.key, row.label]), [[perspective.key, "Alias"]]);
  const saved = unavailableParticipant("Alice");
  assert.equal(saved.key, "Alice");
  assert.equal(saved.unresolved, true);
  assert.notEqual(saved.key, legacyParticipantKey("Alice"));
});
