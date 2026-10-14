interface MetricCardProps {
  label: string;
  value: string | number;
  tone?: "teal" | "blue" | "violet" | "amber" | "red";
  unavailable?: boolean;
}

export function MetricCard({ label, value, tone = "teal", unavailable = false }: MetricCardProps) {
  return (
    <article className={`metric-card tone-${tone}${unavailable ? " unavailable" : ""}`}>
      <span>{label}</span>
      <strong>{unavailable ? "—" : value}</strong>
    </article>
  );
}
