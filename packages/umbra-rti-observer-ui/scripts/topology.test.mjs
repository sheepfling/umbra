import assert from "node:assert/strict";
import test from "node:test";
import { layoutTopology } from "../src/app/topologyLayout.ts";

const keys = ["a/j1", "a/j2", "b", "c", "island-a", "island-b"];
const edges = [{ producer: "a/j1", recipient: "b" }, { producer: "b", recipient: "c" },
  { producer: "c", recipient: "a/j1" }, { producer: "a/j1", recipient: "b" },
  { producer: "b", recipient: "a/j1" }, { producer: "b", recipient: "b" },
  { producer: "island-a", recipient: "island-b" }];
for (const mode of ["circle", "layered", "components"]) test(`${mode} keeps joins distinct and geometry stable for cycles, parallel edges and islands`, () => {
  const result = layoutTopology(keys, edges, mode);
  const reversed = layoutTopology([...keys].reverse(), [...edges].reverse(), mode);
  assert.deepEqual(result, reversed);
  assert.equal(result.positions.size, keys.length);
  const coordinates = [...result.positions.values()];
  assert.equal(new Set(coordinates.map(({x,y}) => JSON.stringify([x,y]))).size, keys.length);
  for (const { x, y } of coordinates) {
    assert.ok(Number.isFinite(x) && x > 0 && x < result.width);
    assert.ok(Number.isFinite(y) && y > 0 && y < result.height);
  }
});
test("empty and self-only graphs stay finite", () => {
  for (const mode of ["circle", "layered", "components"]) {
    assert.equal(layoutTopology([], [], mode).positions.size, 0);
    assert.equal(layoutTopology(["same"], [{ producer:"same", recipient:"same" }], mode).positions.size, 1);
  }
});
