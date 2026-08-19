#!/usr/bin/env python3
"""Plot a CSV produced by RuntimeInstrumentationCsvSampler.

Example:
  python tools/plot_runtime_instrumentation.py runtime.csv -o runtime.png

The sampler CSV is intentionally a simple time series. The script plots
call throughput, average duration, failure rate, and selected callback
queue/execute/invocation durations when those operation names are present.
"""

from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from pathlib import Path


def load_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def plot(rows: list[dict[str, str]], output: Path | None, show: bool) -> None:
    try:
        import matplotlib.pyplot as plt
    except ImportError as exc:
        raise SystemExit(
            "plotting requires matplotlib; install it in the active Python environment"
        ) from exc

    by_operation: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        by_operation[(row["layer"], row["operation"])].append(row)

    figure, axes = plt.subplots(4, 1, figsize=(13, 14), sharex=True)

    for (layer, operation), operation_rows in sorted(by_operation.items()):
        x = [float(row["elapsed_ms"]) / 1000.0 for row in operation_rows]
        calls = [float(row["delta_calls"]) for row in operation_rows]
        failures = [float(row["delta_failures"]) for row in operation_rows]
        durations = [float(row["delta_duration_ns"]) / 1_000_000.0 for row in operation_rows]
        average_ms = [
            duration / call if call else 0.0
            for duration, call in zip(durations, calls)
        ]
        failure_rate = [
            100.0 * failure / call if call else 0.0
            for failure, call in zip(failures, calls)
        ]
        label = f"{layer}.{operation}"
        if any(calls):
            axes[0].plot(x, calls, label=label)
            axes[1].plot(x, average_ms, label=label)
            axes[2].plot(x, failure_rate, label=label)
        if operation in {
            "queue_delay.immediate",
            "queue_delay.evoked",
            "execute.immediate",
            "execute.evoked",
            "invoke.immediate",
            "invoke.evoked",
        }:
            axes[3].plot(x, average_ms, label=label)

    axes[0].set_ylabel("calls / sample")
    axes[0].set_title("Runtime instrumentation throughput")
    axes[1].set_ylabel("average ms")
    axes[1].set_title("Average duration per sampled call")
    axes[2].set_ylabel("failure %")
    axes[2].set_title("Sampled failure rate")
    axes[3].set_ylabel("average ms")
    axes[3].set_title("Callback queue, execution, and invocation timing")
    axes[3].set_xlabel("elapsed seconds")
    for axis in axes:
        axis.grid(True, alpha=0.25)
        handles, labels = axis.get_legend_handles_labels()
        if handles:
            axis.legend(loc="upper left", fontsize="small")
    figure.tight_layout()

    if output is not None:
        figure.savefig(output, dpi=150)
    if show:
        plt.show()
    plt.close(figure)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="sampler CSV path")
    parser.add_argument("-o", "--output", type=Path, help="PNG/SVG output path")
    parser.add_argument("--show", action="store_true", help="show an interactive window")
    args = parser.parse_args()
    rows = load_rows(args.input)
    if not rows:
        raise SystemExit("the instrumentation CSV contains no samples")
    plot(rows, args.output, args.show)


if __name__ == "__main__":
    main()
