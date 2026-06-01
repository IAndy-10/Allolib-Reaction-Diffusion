"""
analyze_stems.py
Analyzes drum stem with librosa and exports onset timestamps to CSV.

Usage:
    python3 analyze_stems.py

Output:
    drums_onsets.csv  — one timestamp per line (seconds), e.g. 0.432\n1.024\n...

Requirements:
    pip install librosa soundfile
"""

import os
import librosa
import numpy as np

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

DRUMS_FILE  = os.path.join(SCRIPT_DIR, "Nala Sinephro - Continuum 1 [2026-06-01 182908] (Drums).wav")
OUTPUT_FILE = os.path.join(SCRIPT_DIR, "drums_onsets.csv")

# ── Load drum stem ─────────────────────────────────────────────────────────────
print(f"Loading: {DRUMS_FILE}")
y, sr = librosa.load(DRUMS_FILE, sr=None, mono=True)
print(f"  Sample rate: {sr} Hz  |  Duration: {len(y)/sr:.2f}s")

# ── Onset detection ───────────────────────────────────────────────────────────
# backtrack=True snaps onsets to the nearest local energy peak (more accurate).
onset_frames = librosa.onset.onset_detect(
    y=y,
    sr=sr,
    units="frames",
    backtrack=True,
    pre_max=3,
    post_max=3,
    pre_avg=5,
    post_avg=5,
    delta=0.2,    # sensitivity — raise to get fewer (stronger) onsets
    wait=4        # minimum gap between onsets (in frames ~= ~93ms at 512 hop)
)

onset_times = librosa.frames_to_time(onset_frames, sr=sr)
print(f"  Detected {len(onset_times)} onsets")

# ── Write CSV ─────────────────────────────────────────────────────────────────
with open(OUTPUT_FILE, "w") as f:
    for t in onset_times:
        f.write(f"{t:.6f}\n")

print(f"Saved: {OUTPUT_FILE}")
