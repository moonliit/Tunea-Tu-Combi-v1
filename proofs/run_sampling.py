"""Repite la prueba física y guarda los tiempos individuales en CSV."""

import argparse
import csv
import re
import statistics
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BINARY = ROOT / "build/benchmark-release/terrain_sampling_proof"
DEFAULT_OUTPUT = Path(__file__).resolve().parent / "benchmark_muestreo_terreno.csv"
TRIAL_PATTERN = re.compile(
    r"repeticion=(\d+), fijo_ms=([0-9.]+), adaptativo_ms=([0-9.]+), "
    r"recorrido_fijo_ms=([0-9.]+), recorrido_adaptativo_ms=([0-9.]+)"
)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=DEFAULT_BINARY)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--runs", type=int, default=3)
    args = parser.parse_args()
    if args.runs < 1:
        parser.error("--runs debe ser positivo")

    rows = []
    for run in range(1, args.runs + 1):
        process = subprocess.run(
            [str(args.binary.resolve())], cwd=ROOT, text=True,
            capture_output=True, check=True,
        )
        trials = TRIAL_PATTERN.findall(process.stdout)
        if len(trials) != 9:
            raise RuntimeError(f"Corrida {run}: se esperaban 9 repeticiones")
        rows.extend((run, *trial) for trial in trials)
        print(f"Corrida {run}: {process.stdout.splitlines()[6]}")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="") as file:
        writer = csv.writer(file)
        writer.writerow([
            "corrida", "repeticion", "reposo_fijo_ms",
            "reposo_adaptativo_ms", "recorrido_fijo_ms",
            "recorrido_adaptativo_ms",
        ])
        writer.writerows(rows)

    for name, fixed_column, adaptive_column, steps in (
        ("Ruedas en planos", 2, 3, 240),
        ("Recorrido", 4, 5, 600),
    ):
        fixed = statistics.median(float(row[fixed_column]) for row in rows)
        adaptive = statistics.median(float(row[adaptive_column]) for row in rows)
        reduction = (1 - adaptive / fixed) * 100
        print(
            f"{name}: {fixed:.3f} vs {adaptive:.3f} ms; "
            f"{fixed/steps:.3f} vs {adaptive/steps:.3f} ms/paso; "
            f"reducción {reduction:.1f} %"
        )
    print(f"Datos: {args.output}")


if __name__ == "__main__":
    main()
