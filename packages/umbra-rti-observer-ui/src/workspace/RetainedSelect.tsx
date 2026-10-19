import { Children, Fragment, isValidElement } from "react";
import type { ComponentProps, ReactNode } from "react";

function text(node: ReactNode): string {
  return Children.toArray(node).map((child) => typeof child === "string" || typeof child === "number" ? String(child)
    : isValidElement<{ children?: ReactNode }>(child) ? text(child.props.children) : "").join("");
}
function options(node: ReactNode): string[] {
  return Children.toArray(node).flatMap((child) => {
    if (!isValidElement<{ value?: string | number; children?: ReactNode }>(child)) return [];
    if (child.type === "option") return [String(child.props.value ?? text(child.props.children))];
    return child.type === Fragment || child.type === "optgroup" ? options(child.props.children) : [];
  });
}

export function RetainedSelect({ value, children, ...props }: ComponentProps<"select">) {
  const missing = value != null && !Array.isArray(value) && !options(children).includes(String(value));
  return <select {...props} value={value} title={missing ? String(value) : props.title}>
    {missing && <option value={String(value)}>Saved selection — not currently reported</option>}
    {children}
  </select>;
}
