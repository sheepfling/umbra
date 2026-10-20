/** Stable display geometry only. Coordinates carry no RTI, timing or distance meaning. */
export type TopologyLayout = "circle" | "layered" | "components";
export interface LayoutEdge { producer: string; recipient: string }
export interface TopologyGeometry { width: number; height: number; positions: Map<string, { x: number; y: number }> }

export function layoutTopology(keys: string[], edges: LayoutEdge[], mode: TopologyLayout): TopologyGeometry {
  const nodes = [...new Set(keys)].sort();
  const positions = new Map<string, { x: number; y: number }>();
  const adjacency = new Map(nodes.map((key) => [key, new Set<string>()]));
  for (const edge of edges) {
    adjacency.get(edge.producer)?.add(edge.recipient);
    adjacency.get(edge.recipient)?.add(edge.producer);
  }
  const visited = new Set<string>();
  const components: string[][] = [];
  for (const seed of nodes) {
    if (visited.has(seed)) continue;
    const component: string[] = [], queue = [seed];
    visited.add(seed);
    while (queue.length) {
      const key = queue.shift()!;
      component.push(key);
      for (const next of [...(adjacency.get(key) ?? [])].sort()) {
        if (visited.has(next) || !adjacency.has(next)) continue;
        visited.add(next); queue.push(next);
      }
    }
    components.push(component.sort());
  }
  if (mode === "circle") {
    nodes.forEach((key, index) => {
      const angle = index * Math.PI * 2 / nodes.length - Math.PI / 2;
      positions.set(key, { x: 450 + 290 * Math.cos(angle), y: 240 + 155 * Math.sin(angle) });
    });
    return { width: 900, height: 490, positions };
  }
  if (mode === "components") {
    components.forEach((component, index) => component.forEach((key, at) => {
      const angle = at * Math.PI * 2 / component.length - Math.PI / 2;
      positions.set(key, { x: 225 + index % 2 * 450 + (component.length > 1 ? 130 * Math.cos(angle) : 0),
        y: 170 + Math.floor(index / 2) * 340 + (component.length > 1 ? 85 * Math.sin(angle) : 0) });
    }));
    return { width: 900, height: Math.max(340, Math.ceil(components.length / 2) * 340), positions };
  }
  let top = 0;
  for (const component of components) {
    const set = new Set(component);
    const outgoing = new Map(component.map((key) => [key, new Set<string>()]));
    const incoming = new Set<string>();
    for (const edge of edges) if (set.has(edge.producer) && set.has(edge.recipient) && edge.producer !== edge.recipient) {
      outgoing.get(edge.producer)!.add(edge.recipient); incoming.add(edge.recipient);
    }
    const levels = new Map<string, number>();
    const visit = (seeds: string[]) => {
      const queue = [...seeds];
      for (const seed of seeds) levels.set(seed, 0);
      while (queue.length) {
        const key = queue.shift()!;
        for (const next of [...outgoing.get(key)!].sort()) if (!levels.has(next)) {
          levels.set(next, levels.get(key)! + 1); queue.push(next);
        }
      }
    };
    visit(component.filter((key) => !incoming.has(key)));
    // Cycles and disconnected directed islands are anchored deterministically.
    for (const key of component) if (!levels.has(key)) visit([key]);
    const max = Math.max(0, ...levels.values());
    const columns = Array.from({ length: max + 1 }, (_, level) => component.filter((key) => levels.get(key) === level));
    const height = Math.max(300, Math.max(...columns.map((column) => column.length)) * 115 + 120);
    columns.forEach((column, level) => column.forEach((key, index) => positions.set(key, {
      x: max ? 90 + level / max * 720 : 450, y: top + 100 + index * 115,
    })));
    top += height;
  }
  return { width: 900, height: Math.max(300, top), positions };
}
