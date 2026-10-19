import type { ReactNode } from "react";

export const chartColors = ["#5eead4", "#7dd3fc", "#b9a4ff", "#f6c66d", "#fb8f8f", "#f0abfc"];
export const chartTooltip = { background: "#102238", border: "1px solid #2a4b68", borderRadius: 10, color: "#e8f1f7" };
export const chartTick = { fill: "#8ea5b8", fontSize: 11 };

export function ChartPanel({ title, eyebrow, note, children }: {
  title: string; eyebrow: string; note?: string; children: ReactNode;
}) {
  return <article className="panel chart-panel">
    <div className="panel-heading"><div><p className="eyebrow">{eyebrow}</p><h2>{title}</h2></div>{note && <span className="panel-note">{note}</span>}</div>
    <div className="chart-wrap">{children}</div>
  </article>;
}

export function EmptyPanel({ title, children }: { title: string; children: ReactNode }) {
  return <section className="panel empty-panel"><h2>{title}</h2><p>{children}</p></section>;
}
