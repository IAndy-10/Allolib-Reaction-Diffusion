# The Dance of Laplace
**Italo Rojas — MAT 201B — UCSB**

Gray-Scott reaction-diffusion on the interior of a sphere with spatialized audio. Built for the AlloSphere.

The piece runs a Bi-Laplacian Gray-Scott simulation on the GPU and projects it onto the inside of a sphere. Three audio stems (Others, Bass, Drums) are independently spatialized via VBAP through the AlloSphere speaker array, each moving along a different 3D trajectory. Drum onsets trigger color palette changes. A scripted camera journey moves from outside the sphere to its center, through a slow rotation, and ends in a fade-to-white closing sequence. The simulation auto-resets every 8 seconds and cycles palettes throughout.

---

## Dependencies

- [Allolib](https://github.com/AlloSphere-Research-Group/allolib) with Cuttlebone and Gamma
- C++17

---

## Files required (not in repo — too large)

Place in the same directory as `1.cpp`:
- `Nala Sinephro - Continuum 1 [2026-06-01 182908] (Bass).wav`
- `Nala Sinephro - Continuum 1 [2026-06-01 182908] (Drums).wav`
- `Nala Sinephro - Continuum 1 [2026-06-01 182908] (Others).wav`
- `drums_onsets.csv`

---

## Build

```bash
cd allolib_playground
./run.sh italo/eoy/1.cpp
```

---

## LLM Use

**Claude Sonnet 4.6** (Claude.ai and Claude Code) was used throughout development. LLM-generated or LLM-assisted work includes: C++ port from p5.js, GLSL shader structure, FBO ping-pong scaffolding, Python onset extraction script, color palette parameters, and closing sequence uniforms.

The initial GLSL was incompatible with Allolib's distributed rendering context and required manual debugging with lab support. Generated code was reviewed and corrected against the actual deployment environment.

---

## Controls

| Key | Action |
|---|---|
| `Space` | Reset simulation |
| `C` | Cycle color palette |
| `3` | Jump to stage three |
| `4` | Jump to stage four |
