# The Dance of Laplace

**Italo Rojas — MAT 201B — UCSB**

Gray-Scott reaction-diffusion on the interior of a sphere, with three audio stems spatialized by **VBAP over the AlloSphere's 54-speaker array**. Built in C++ with [Allolib](https://github.com/AlloSphere-Research-Group/allolib) and GLSL for the UCSB AlloSphere, and shown at the End of Year Show 2026.

[![The Dance of Laplace](https://img.youtube.com/vi/6F-28OYPWOY/maxresdefault.jpg)](https://www.youtube.com/watch?v=6F-28OYPWOY)

Performance at the EoY Show 2026: [watch on YouTube](https://www.youtube.com/watch?v=3L7j6RN6aeg) · [Project page](https://www.italorojas-portfolio.cl/projects/dance-of-laplace)

---

## What it is

The piece runs a Bi-Laplacian Gray-Scott simulation on the GPU and projects it onto the inside of a sphere. Three stems of Nala Sinephro's *Continuum 1* (Others, Bass, Drums) each move along their own 3D trajectory around the audience. Drum onsets trigger color palette changes. A scripted camera journey moves from outside the sphere to its center, through a slow rotation, and ends in a fade-to-white closing sequence. The simulation auto-resets every 8 seconds and cycles palettes throughout.

---

## Spatial audio

Each stem is rendered to its own audio bus and spatialized independently as a point source on the unit sphere:

| Stem | Trajectory |
|---|---|
| Others | Lissajous orbit: `(cos t, 0.6 sin 1.8t, sin t)` |
| Bass | Slow, wide horizontal circle slightly below the listener: `(cos t, −0.4, sin t)` |
| Drums | Faster, frontal figure-8: `(cos t, 0.75 sin 2t, sin t)` |

- **On the AlloSphere:** Allolib's `Vbap` (vector base amplitude panning) over `AlloSphereSpeakerLayoutCompensated()`. The layout has 54 speakers in three rings (12 above, 30 at ear level, 12 below), with per-speaker gain compensation for distance. Audio runs at 48 kHz, 512-sample blocks, 60 output channels (the device channel map of the array).
- **In development:** the same code falls back to a stereo panner on 2 channels.
- Each stem goes through a Gamma biquad low-pass before spatialization. In the closing stage, the drums drop out and all audio fades to silence over the last 5 seconds.

Spatialization went through LBAP before settling on VBAP; the LBAP setup is still in `1.cpp`, commented out.

---

## Simulation and rendering

- **Gray-Scott with hyper-diffusion.** Diffusion uses the Laplacian of the Laplacian (Bi-Laplacian) instead of the standard Laplacian, which produces finer, more intricate patterns. Defaults: `dA = 1.0`, `dB = 0.5`, `feed = 0.055`, `k = 0.062`, `dt = 0.009`.
- **GPU ping-pong.** Two 512×512 float textures attached to FBOs swap read/write roles every step, 4 steps per frame (`rd_sim.frag.glsl`). No CPU round-trips.
- **Display.** The state texture is mapped onto a 300×600 sphere mesh and displaced radially inward by the pattern (`rd_display.vert.glsl`). The fragment shader applies the color palette, plus blur and brightness for the closing fade (`rd_display.frag.glsl`).
- **Distributed rendering.** Built on `DistributedAppWithState`: the primary node runs the timeline and the audio, and shares the simulation parameters, palette and camera pose with the AlloSphere's render nodes.

### Timeline

| Stage | What happens |
|---|---|
| 1 | Camera travels from outside the sphere to the center (28 s), pauses (5 s), turns a full 360° (45 s) and travels back out (28 s) |
| 2 | Camera re-enters (28 s), pauses at the center (5 s), then drifts in a slow rotation (20 s) |
| 3 | Palette locked to black and white, auto-reset off (20 s) |
| 4 | Drums silenced, image fades to white and audio to silence (40 s) |

---

## Files

| File | Purpose |
|---|---|
| `1.cpp` | The application: simulation loop, timeline, audio playback and spatialization |
| `rd_sim.frag.glsl`, `rd_sim.vert.glsl` | Gray-Scott step with Bi-Laplacian diffusion |
| `rd_display.frag.glsl`, `rd_display.vert.glsl` | Sphere displacement, palettes, closing fade |
| `analyze_stems.py` | Detects drum onsets with librosa and writes `drums_onsets.csv` |
| `drums_onsets.csv` | Onset timestamps (seconds) that trigger palette changes |
| `distributed_app.toml` | Allolib distributed app configuration |

---

## Running it

### Dependencies

- [Allolib](https://github.com/AlloSphere-Research-Group/allolib) (with Gamma), via [allolib_playground](https://github.com/AlloSphere-Research-Group/allolib_playground)
- C++17
- Python 3 with `librosa` and `soundfile`, only to regenerate the onsets

### Audio stems (not in the repo)

The three stems are copyrighted and not distributed here. Place them next to `1.cpp`:

- `Nala Sinephro - Continuum 1 [2026-06-01 182908] (Bass).wav`
- `Nala Sinephro - Continuum 1 [2026-06-01 182908] (Drums).wav`
- `Nala Sinephro - Continuum 1 [2026-06-01 182908] (Others).wav`

To regenerate `drums_onsets.csv` from the drum stem:

```bash
pip install librosa soundfile
python3 analyze_stems.py
```

### Build and run

From `allolib_playground`, with this folder at `italo/eoy/`:

```bash
./run.sh italo/eoy/1.cpp
```

### Controls

| Key | Action |
|---|---|
| `Space` | Reset simulation |
| `C` | Cycle color palette |
| `3` | Jump to stage three |
| `4` | Jump to stage four |

The GUI also exposes the simulation parameters (feed, kill, diffusion rates, time step), displacement, palette, pan speed, filter cutoff and gain.

---

## LLM use

**Claude Sonnet 4.6** (Claude.ai and Claude Code) was used throughout development. LLM-generated or LLM-assisted work includes: C++ port from p5.js, GLSL shader structure, FBO ping-pong scaffolding, Python onset extraction script, color palette parameters, and closing sequence uniforms.

The initial GLSL was incompatible with Allolib's distributed rendering context and required manual debugging with lab support. Generated code was reviewed and corrected against the actual deployment environment.

The README was later revised with Claude Code (Claude Opus 5.5), checked against the code.
