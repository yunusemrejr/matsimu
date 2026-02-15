# ⚛️ MATSIMU
## Material Science Simulator
*A fast, accessible molecular dynamics engine in C++17*

---

## 📋 What Is MATSIMU?

**MATSIMU** is a **material science simulator**. In plain English, it's a program that pretends to be a box of atoms, then watches what those atoms do over time.

Think of it like a virtual chemistry set: instead of mixing real chemicals, you set up particles on a computer, define interaction rules, and press "play" to see what happens.

> **Core Details:**
> - Written in **C++17** (fast, compiled)
> - Runs on **Linux** (Ubuntu/Mint compatible)
> - Two modes: **CLI** (text) and **GUI** (with Qt 6)

---

## 🎯 Why Does It Exist?

Professional simulators like LAMMPS and GROMACS are powerful but intimidating — they have hundreds of settings and assume deep prior knowledge.

**MATSIMU's mission: Accessibility.**

| Problem with existing tools | How MATSIMU fixes it |
|---|---|
| Complicated build steps | One command: `./run.sh` |
| Heavy dependencies | Only needs a C++ compiler |
| Hard to understand code | Clean, layered architecture |
| No visual feedback | Optional 3D viewer (Qt 6) |
| Physics hidden in jargon | Plain-language analogies |

---

## 🔬 What Can You Do With It?

- **Simulate atoms interacting over time** — Molecular dynamics (MD). Watch particles settle like marbles in a jar.
- **Define repeating crystal structures** — A 3D lattice that tiles infinitely.
- **Visualize in 3D** — Rotate, zoom, and inspect the crystal structure.
- **Control temperature** — Heat or cool your virtual box.
- **Track energy** — Monitor kinetic and potential energy.
- **Run pre-built examples** — See it work immediately.

---

## 🚀 How Do You Run It?

### Terminal Mode (minimum setup)

```bash
sudo apt install build-essential
./run.sh
```

### With 3D Visuals

```bash
sudo apt install qt6-base-dev qt6-tools-dev qt6-tools-dev-tools libgl1-mesa-dev
./run.sh
```

### Other Commands

| Command | What it does |
|---|---|
| `./run.sh` | Build and run |
| `./run.sh --clean` | Delete compiled files |
| `./run.sh --debug` | Build with debug info |
| `./run.sh --example lattice` | Run built-in demo |
| `./run.sh --help` | Show all options |

---

## 📊 What Happens When You Run It?

### Terminal Mode

The program creates a simulation with default settings, steps through time, and prints progress:

```
Simulation step; time after step: 0 s
Simulation finished at t = 2e-15 s
```

*(2 femtoseconds — atoms move incredibly fast!)*

### With Qt (GUI Mode)

A window opens with three tabs:

- **Simulation Tab** — Set parameters and run.
- **Lattice Tab** — Edit crystal box dimensions.
- **3D View Tab** — Live 3D rendering; click to rotate, scroll to zoom.

---

## 🏗️ The Big Picture — Code Organization

The codebase is layered like a building. Lower layers are simple and stable. Upper layers depend on them but are never depended upon by lower layers.

```
┌──────────────────────────┐
│    UI (window)           │  ← Top: buttons, 3D graphics
├──────────────────────────┤
│  Sim (conductor)         │  ← Runs the clock
├──────────────────────────┤
│ Physics (engine room)    │  ← Forces, motion, temperature
├──────────────────────────┤
│ IO + Lattice             │  ← Reading files / crystal grid
├──────────────────────────┤
│ Alloc (memory manager)   │  ← Prevents runaway memory use
├──────────────────────────┤
│ Core (foundation)        │  ← Basic types and units
└──────────────────────────┘
```

**Golden Rule:** Arrows only point downward. No circular dependencies, no spaghetti code.

---

## 📚 Module-by-Module Breakdown

### 7.1 — Core: The Foundation Types

**Analogy:** The measurement system. Before building anything, agree on units.

| File | Purpose |
|---|---|
| `core/types.hpp` | Defines `Real` (decimals), `Index` (counts), `Size` (sizes) |
| `core/units.hpp` | SI units: metres, seconds, kilograms, kelvin, joules |

**Why?** The Mars Climate Orbiter crashed because engineers mixed up units (metres vs. feet). MATSIMU prevents this.

### 7.2 — Alloc: The Memory Bouncer

**Analogy:** A nightclub bouncer for RAM. "Sorry, you've hit your memory limit."

| File | Purpose |
|---|---|
| `alloc/bounded_allocator.hpp` | Enforces hard memory limits. Prevents accidental allocation of terabytes. |

**Why?** Scientific simulations can accidentally try to allocate massive amounts of memory. This catches mistakes instantly.

### 7.3 — Lattice: The Repeating Grid

**Analogy:** A bathroom tile pattern. Know one tile, know the entire floor (it repeats forever).

**Key capabilities:**

- Define a crystal shape using three basis vectors.
- Compute volume and properties.
- Wrap particles that escape the box (periodic boundaries — Pac-Man style).
- Use minimum-image convention for distances.

### 7.4 — Physics: The Engine Room

This is where the actual science lives. Five components:

#### **7.4.1 Particles** (`physics/particle.hpp`)

Each particle is a tiny billiard ball with:
- Position
- Velocity
- Force
- Mass

#### **7.4.2 Potentials** (`physics/potential.hpp`)

The "rules of attraction and repulsion."

- **`LennardJones`** — The most famous atomic interaction (noble gases like Argon).
- **`HarmonicPotential`** — A spring: atoms pull back when stretched.
- **`ForceField`** — The calculator that loops over every pair of particles.

#### **7.4.3 Neighbor List** (`physics/neighbor_list.hpp`)

**Analogy:** At a huge party, don't measure distance to everyone every second. Make a short list of nearby people instead.

- **`NeighborList`** — Maintains which particles are close to each other.
- **`NeighborForceField`** — Uses the list to compute forces much faster.

#### **7.4.4 Integrator** (`physics/integrator.hpp`)

A camera taking rapid-fire photos. Figures out where each particle will be next.

- **`VelocityVerlet`** — The "gold standard." Accurate and energy-conserving.
- **`EulerIntegrator`** — Simpler, less accurate. For testing only.

#### **7.4.5 Thermostat** (`physics/thermostat.hpp`)

An air conditioner for your simulation. Controls temperature.

- **`VelocityRescaleThermostat`** — Scale all velocities. Fast but simple.
- **`AndersenThermostat`** — Randomly kick particles. Physically correct but more complex.
- **`NullThermostat`** — Does nothing. For constant-energy simulations.

### 7.5 — Sim: The Conductor

**Analogy:** An orchestra conductor. Doesn't play instruments; orchestrates everything.

**What a single simulation step does:**

1. Validate that settings make sense
2. Half-update velocities using current forces (Integrator step 1)
3. Move particles to new positions (Integrator step 1)
4. Wrap particles that went outside the box (Periodic boundaries)
5. Calculate new forces at the new positions (Force field or neighbor force field)
6. Finish updating velocities with new forces (Integrator step 2)
7. Apply thermostat adjustment (Temperature control)
8. Advance the clock (`time += dt`)
9. Check if we're done (`time ≥ end_time` or max steps reached)

### 7.6 — IO: The Translator

**Analogy:** A customs officer at a border. Inside: SI units everywhere. At the border: convert to whatever format is needed.

| File | Purpose |
|---|---|
| `io/config.hpp` | Load simulation settings from a file. |
| `io/config.cpp` | Implementation of config loading. |

**Key rule:** All unit conversions happen here and *only* here.

### 7.7 — UI: The Visual Interface

**Requires:** Qt 6 (version ≥ 6.2). If Qt isn't installed, the app runs in terminal/CLI mode instead.

| File | Purpose |
|---|---|
| `ui/main_window.hpp/.cpp` | Main window — menu bar, tab container, status bar. |
| `ui/simulation_tab.hpp/.cpp` | "Simulation" tab — parameter controls and buttons. |
| `ui/lattice_tab.hpp/.cpp` | "Lattice" tab — basis vector editor. |
| `ui/view_3d.hpp/.cpp` | OpenGL 3D renderer. |
| `ui/view_3d_tab.hpp/.cpp` | Tab wrapper for 3D view. |

**How the GUI stays responsive:** The simulation doesn't run in a blocking loop. A `QTimer` fires periodically, runs one simulation step, updates the display, and lets the window handle mouse clicks.

---

## ⚛️ The Physics, Explained Simply

### What is Molecular Dynamics (MD)?

Imagine you have a box of balls:

- Each ball has a **position** (where it is), a **velocity** (how fast and in which direction it's moving), and a **mass** (how heavy it is).
- Between every pair of balls, there's a rule about whether they attract or repel each other, and how strongly.
- You advance time by a tiny amount (the "time step"), figure out all the forces, move the balls accordingly, and repeat.

That's molecular dynamics.

### What is Lennard-Jones Potential?

The most common rule for how atoms interact. In English:

- **Very far apart:** No significant interaction. They ignore each other.
- **Medium distance:** Slight attraction. Like two magnets gently pulling closer.
- **Very close:** Extreme repulsion. Like trying to push two solid balls into each other.

The formula is:

```
U(r) = 4ε [(σ/r)¹² − (σ/r)⁶]
```

- `r` = distance between two atoms
- `ε` (epsilon) = depth of the "energy well" — how strongly they attract at the sweet spot
- `σ` (sigma) = the effective "size" of the atom

### What is a Time Step?

Real atoms vibrate at about 10¹³ times per second. To track their motion accurately, the simulation needs to take snapshots much faster than that. A typical time step is **1 femtosecond** (10⁻¹⁵ seconds).

MATSIMU's default is exactly `1e-15` seconds.

### What is Temperature in a Simulation?

Temperature is directly related to how fast the particles are moving. Hotter = faster.

The formula is:

```
T = 2 × E_kinetic / (3 × N × k_B)
```

Where:
- `N` = number of particles
- `k_B` = Boltzmann's constant

---

## 🎨 Key Design Decisions

| Decision | Rationale |
|---|---|
| **Single entry point (`run.sh`)** | No CMake, no Makefile. One script handles everything. |
| **C++17 with no heavy dependencies** | The C++17 standard library is powerful enough. No Boost, Eigen, etc. |
| **Qt 6 is optional** | GUI is optional; CLI mode works standalone. |
| **SI units throughout** | Prevents unit-mismatch bugs (see Mars Climate Orbiter). |
| **Fail-fast validation** | Crash immediately with clear error, never silently produce wrong results. |
| **One file, one job** | No 5000-line god files. Clean separation of concerns. |
| **Dependencies flow inward** | Upper layers depend on lower layers. Never the reverse. |

---

## 📂 File Map

```
matsimu/
├── run.sh                    ← THE entry point
├── README.md
├── include/matsimu/
│   ├── core/                 ← Types & units
│   ├── alloc/                ← Memory manager
│   ├── lattice/              ← Crystal grid
│   ├── physics/              ← Atoms, forces, integrators
│   ├── sim/                  ← Simulation orchestrator
│   ├── io/                   ← Config loading
│   └── ui/                   ← GUI (Qt 6)
├── src/                      ← Implementation
│   ├── main.cpp
│   ├── lattice/
│   ├── sim/
│   ├── io/
│   ├── physics/
│   └── ui/
├── examples/
│   └── lattice_example.cpp
└── build/                    ← Auto-created output
```

---

## 🔗 Dependency Rules

```
       ┌──────┐
       │  UI  │ ──▶ Sim, Lattice, Core
       └──┬───┘
          │
       ┌──▼───┐
       │ Sim  │ ──▶ Physics, Lattice, Core
       └──┬───┘
          │
    ┌─────▼────────┐
    │   Physics     │ ──▶ Lattice, Core
    └─────┬────────┘
          │
  ┌───────▼─────┬────────┐
  │   Lattice   │   IO   │ ──▶ Core
  └─────┬───────┴────┬───┘
        │            │
  ┌─────▼────────────▼────┐
  │   Alloc + Core        │ ──▶ nothing (foundation)
  └───────────────────────┘
```

**Forbidden:**
- Core/Alloc must never import Lattice, Physics, Sim, IO, or UI.
- Lattice must never import Sim or UI.
- No circular dependencies.

---

## 🛠️ How to Add New Code

### Adding a new module (e.g., a new force type)

1. Create the header: `include/matsimu/physics/my_force.hpp`
2. Create the implementation: `src/physics/my_force.cpp`
3. The build system (`run.sh`) **auto-discovers** all `.cpp` files in `src/` (up to 2 levels deep), so no manual registration is needed.
4. Keep the dependency rules: your new file can use `core/`, `alloc/`, `lattice/`, and other `physics/` files — but not `sim/`, `io/`, or `ui/`.

### Adding a new UI component

1. Create the header in `include/matsimu/ui/` and the implementation in `src/ui/`.
2. `run.sh` auto-discovers all UI headers for MOC processing and all UI `.cpp` files for compilation.
3. Qt 6 (≥ 6.2) must be installed for UI code to compile.

### Adding a new example

1. Create a file in `examples/` (e.g., `examples/my_example.cpp`).
2. Wire it up in `src/main.cpp` by adding another `if` branch.
3. Run it with `./run.sh --example my_example`.

### Merge strategy for configs

To keep local config overrides when pulling (merge conflicts in config files resolve to our version), set the merge driver once:

```bash
git config merge.ours.driver true
```

`.gitattributes` marks config-related paths to use this driver.

---

## 📖 Glossary

| Term | Definition |
|---|---|
| **Atom / Particle** | Tiny ball with position, speed, and mass. |
| **CLI** | Command Line Interface — text-only mode. |
| **Cutoff** | Distance beyond which forces are ignored (weak). |
| **dt (time step)** | Time between simulation frames. Smaller = more accurate but slower. |
| **Force** | Push or pull on a particle. Measured in newtons (N). |
| **GUI** | Graphical User Interface — window with buttons and 3D graphics. |
| **Integrator** | Algorithm that figures out where particles move next. |
| **Lattice** | Repeating 3D pattern defined by three basis vectors. |
| **Lennard-Jones (LJ)** | Math formula for atomic attraction/repulsion. |
| **MD (Molecular Dynamics)** | Compute forces, move atoms, repeat. |
| **Minimum image** | Always use shortest distance, even through walls. |
| **MOC** | Qt's Meta Object Compiler — code generator for signal/slot system. |
| **Neighbor list** | Cache of nearby particles to speed up force calculation. |
| **PBC (Periodic Boundary Conditions)** | Particles exiting one side re-enter from the opposite (Pac-Man). |
| **Potential** | Mathematical rule for energy vs. distance between atoms. |
| **SI** | International System: metres, kilograms, seconds, etc. |
| **Symplectic** | Property of Velocity Verlet: preserves total energy. |
| **Thermostat** | Mechanism to control simulation temperature. |
| **Unit cell** | Smallest piece of crystal that tiles to fill space. |

---

<footer>
<div align="center">

## ⚛️ MATSIMU
**Material Science Simulator**

*Last updated: February 2026*

For architecture details, see `ARCHITECTURE.md`. For units, see `UNITS.md`.
*MATSIMU is a project created by Yunus Emre Vurgun.*
</div>
</footer>