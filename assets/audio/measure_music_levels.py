# Measure music loudness and write playback gains for the game

from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import math
import re
import subprocess


ROOT = Path(__file__).resolve().parents[2]
TARGET_LUFS = -23.0
MAX_TRUE_PEAK_DBFS = -1.0


def measure(path: Path) -> tuple[str, float, float]:
    relative_path = path.relative_to(ROOT).as_posix()
    result = subprocess.run(
        [
            "ffmpeg", "-hide_banner", "-nostats", "-i", str(path),
            "-filter_complex", "ebur128=peak=true", "-f", "null", "-",
        ],
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
    )
    summary = result.stderr.rsplit("Summary:", 1)[-1]
    loudness = re.search(r"Integrated loudness:\s*I:\s*([-\d.]+) LUFS", summary)
    true_peak = re.search(r"True peak:\s*Peak:\s*([-\d.]+) dBFS", summary)
    if loudness is None or true_peak is None:
        raise ValueError(f"Could not measure {relative_path}")

    current_lufs = float(loudness.group(1))
    current_peak = float(true_peak.group(1))
    gain_db = min(TARGET_LUFS - current_lufs, MAX_TRUE_PEAK_DBFS - current_peak)
    gain = math.pow(10.0, gain_db / 20.0)
    return relative_path, gain, current_lufs + gain_db


def main() -> None:
    audio = ROOT / "assets/audio"
    paths = [
        audio / "menu.wav",
        *sorted((audio / "custom").glob("*.ogg")),
        *sorted((audio / "clean").glob("*.ogg")),
    ]
    with ThreadPoolExecutor(max_workers=4) as pool:
        rows = sorted(pool.map(measure, paths))
    output = audio / "music_levels.tsv"
    output.write_text(
        "".join(f"{path}\t{gain:.6f}\n" for path, gain, _ in rows),
        encoding="utf-8",
    )
    loudnesses = [loudness for _, _, loudness in rows]
    print(
        f"Measured {len(rows)} tracks; normalized range: "
        f"{min(loudnesses):.1f} to {max(loudnesses):.1f} LUFS"
    )


if __name__ == "__main__":
    main()
