# MATSIMU examples

Runnable examples beyond the default app. Use `./run.sh --example <name>` from the project root (no Qt required for CLI examples).

## 1. Lattice demo

```bash
./run.sh --example lattice
```

- Default cubic unit cell (1 m³ volume).
- Min-image in fractional coordinates.
- Demonstrates `Lattice` API: `volume()`, `min_image_frac()`.

## 2. Heat diffusion CLI

```bash
./run.sh --example heat
```

- 1D explicit heat diffusion with fixed boundaries.
- Parameters: α = 1e-5 m²/s, dx = 1 mm, dt below stability limit, end_time = 1 ms.
- Deterministic: same step count and final time on every run.
- Demonstrates `HeatDiffusionParams`, `Simulation(HeatDiffusionParams)`, stability check.

## 3. GUI-guided preset flow

1. Build with Qt: install `qt6-base-dev` (and optionally `qt6-tools-dev-tools`, `libgl1-mesa-dev`), then run `./run.sh`.
2. Open the **Simulation** tab: use the main parameters (grid spacing, time step, end time) with plain-language descriptions; run with default or adjusted values.
3. Open the **Lattice** tab: edit basis vectors (metres); volume updates live; 3D View reflects the unit cell.
4. Use **Run** (F5) / **Stop** (F6) / **Reset**; simulation steps run on a timer so the UI stays responsive.

No separate example binary; the app itself is the preset flow.

## Adding an example

Add a new `examples/<name>_example.cpp` and wire it in `src/main.cpp` with `--example <name>`. Keep examples minimal and document them here.
