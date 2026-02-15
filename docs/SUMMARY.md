# MATSIMU — Full Project Briefing

> **Last updated:** February 2026
>
> **Audience:** Everyone — product managers, non-programmers, web developers, C++ beginners, C++ experts, and people who haven't touched physics since high school. No prior knowledge assumed.

---

## Table of Contents

1. [What Is MATSIMU?](#1-what-is-matsimu)
2. [Why Does It Exist?](#2-why-does-it-exist)
3. [What Can You Do With It?](#3-what-can-you-do-with-it)
4. [How Do You Run It?](#4-how-do-you-run-it)
5. [What Happens When You Run It?](#5-what-happens-when-you-run-it)
6. [The Big Picture — How the Code Is Organized](#6-the-big-picture--how-the-code-is-organized)
7. [Module-by-Module Breakdown](#7-module-by-module-breakdown)
   - 7.1 [Core — The Foundation Types](#71-core--the-foundation-types)
   - 7.2 [Alloc — The Memory Bouncer](#72-alloc--the-memory-bouncer)
   - 7.3 [Lattice — The Repeating Grid](#73-lattice--the-repeating-grid)
   - 7.4 [Physics — The Engine Room](#74-physics--the-engine-room)
   - 7.5 [Sim — The Conductor](#75-sim--the-conductor)
   - 7.6 [IO — The Translator](#76-io--the-translator)
   - 7.7 [UI — The Visual Interface](#77-ui--the-visual-interface)
8. [The Physics, Explained Simply](#8-the-physics-explained-simply)
9. [Key Design Decisions](#9-key-design-decisions)
10. [File Map (Where Everything Lives)](#10-file-map-where-everything-lives)
11. [Dependency Rules (What Talks to What)](#11-dependency-rules-what-talks-to-what)
12. [How to Add New Code](#12-how-to-add-new-code)
13. [Glossary](#13-glossary)

---

## 1. What Is MATSIMU?

**MATSIMU** is a **material science simulator**. In plain English, it's a program that pretends to be a box of atoms, then watches what those atoms do over time.

Think of it like a virtual chemistry set: instead of mixing real chemicals, you set up a bunch of tiny particles on a computer, define the rules of how they push and pull on each other, and then press "play" to see what happens.

- It's written in **C++17** (a fast, compiled programming language).
- It runs on **Linux** (Ubuntu/Mint first, but any Linux with a C++ compiler works).
- It has **two modes**: a text-only terminal mode (CLI) and a visual window mode (GUI) if you install Qt 6.

---

## 2. Why Does It Exist?

Most professional material science simulators (like LAMMPS, GROMACS, etc.) are powerful but intimidating — they have hundreds of settings, require special build systems, and assume you already know what a "Lennard-Jones potential" is.

**MATSIMU's goal is accessibility:**

| Problem with existing tools | How MATSIMU fixes it |
|---|---|
| Complicated build steps (CMake, Make, etc.) | One command: `./run.sh` |
| Need to install many dependencies | Only needs a C++ compiler (comes with `build-essential`) |
| Hard to understand the code | Clean, layered architecture with one job per file |
| No visual feedback | Optional 3D viewer built in (with Qt 6) |
| Physics hidden behind jargon | Documented with plain-language analogies |

---

## 3. What Can You Do With It?

Today, MATSIMU can:

- **Simulate atoms (particles) interacting over time** — called "molecular dynamics" (MD). Think: dropping marbles into a jar and watching them settle.
- **Define a repeating crystal structure** (a "lattice") — imagine wallpaper but in 3D. A tiny pattern that tiles forever in all directions.
- **Visualize the crystal structure** in a 3D window where you can click and drag to rotate it.
- **Control temperature** — like heating or cooling your virtual box of particles.
- **Track energy** — how much energy is stored as motion (kinetic) vs. how much is stored in the forces between atoms (potential).
- **Run examples** — pre-built demos to see it work immediately.

---

## 4. How Do You Run It?

### Minimum setup (terminal mode only)

```bash
# Install the C++ compiler (one time)
sudo apt install build-essential

# Run MATSIMU
./run.sh
```

That's it. The `run.sh` script does everything: checks your system, compiles the code, and runs the program.

### With the 3D visual window

```bash
# Install Qt 6 (one time)
sudo apt install qt6-base-dev qt6-tools-dev qt6-tools-dev-tools libgl1-mesa-dev

# Run MATSIMU (it auto-detects Qt and launches the window)
./run.sh
```

### Other options

| Command | What it does |
|---|---|
| `./run.sh` | Build and run the app |
| `./run.sh --clean` | Delete all compiled files (start fresh) |
| `./run.sh --debug` | Build with extra error-checking info |
| `./run.sh --example lattice` | Run the built-in lattice demo |
| `./run.sh --help` | Show all available options |

---

## 5. What Happens When You Run It?

### Without Qt (terminal mode)

The program creates a simple simulation with default settings, steps through a few time increments, and prints the current time to your terminal:

```
Simulation step; time after step: 0 s
Simulation finished at t = 2e-15 s
```

*(That's 2 femtoseconds — two quadrillionths of a second. Atoms move fast, so their "clock" ticks in incredibly tiny increments.)*

### With Qt (window mode)

A window opens with three tabs:

1. **Simulation Tab** — Set parameters (time step, end time, temperature) and press Run/Stop/Reset.
2. **Lattice Tab** — Edit the shape and size of your crystal box by changing three direction vectors.
3. **3D View Tab** — A live 3D rendering of the lattice cell. Click and drag to rotate, scroll to zoom.

---

## 6. The Big Picture — How the Code Is Organized

Imagine a building with floors. The bottom floors are simple and stable. The top floors are more feature-rich and depend on the ones below. Information flows **upward** — lower layers never reach up to touch upper layers.

```
┌──────────────────────────────────────────────────┐
│                   UI (window)                    │  ← Top: buttons, 3D graphics
├──────────────────────────────────────────────────┤
│                Sim (conductor)                   │  ← Runs the clock, calls physics
├──────────────────────────────────────────────────┤
│            Physics (engine room)                 │  ← Forces, motion, temperature
├──────────┬──────────┬────────────────────────────┤
│    IO    │  Lattice │                            │  ← Reading files / crystal grid
├──────────┴──────────┴────────────────────────────┤
│             Alloc (memory bouncer)               │  ← Prevents runaway memory use
├──────────────────────────────────────────────────┤
│              Core (foundation)                   │  ← Basic number types and units
└──────────────────────────────────────────────────┘
```

**The rule:** arrows only point **downward**. The Simulation layer can use Lattice and Physics, but Lattice can never use Simulation. This prevents spaghetti tangles in the code.

---

## 7. Module-by-Module Breakdown

### 7.1 Core — The Foundation Types

**Analogy:** The measuring system. Before you build anything, you decide "I'll measure in metres, weigh in kilograms, and time in seconds."

**What it contains:**

| File | Purpose |
|---|---|
| `core/types.hpp` | Defines three number types: `Real` (decimal numbers like `3.14`), `Index` (whole numbers for counting grid positions), and `Size` (for counting how many of something). |
| `core/units.hpp` | Defines the measurement system — metres, seconds, kilograms, kelvin, joules, pascals. Everything inside the simulation uses SI units (the international standard). |

**Why it matters:** If one part of the code used centimetres and another used metres, the simulation would produce garbage results. Having a single, agreed-upon system prevents that. (This is literally what crashed the [Mars Climate Orbiter](https://en.wikipedia.org/wiki/Mars_Climate_Orbiter) — a unit mix-up.)

**For C++ experts:** `Real` is `double`, `Index` is `int`, `Size` is `unsigned long`. The unit constants are `constexpr double` set to `1.0`, acting as semantic documentation rather than runtime multipliers. Conversions happen only at I/O boundaries.

---

### 7.2 Alloc — The Memory Bouncer

**Analogy:** A nightclub bouncer for computer memory. "Sorry, you've used your limit — no more allocations."

**What it contains:**

| File | Purpose |
|---|---|
| `alloc/bounded_allocator.hpp` | A custom memory manager that enforces a hard limit on how much memory a piece of code can use. If you try to exceed it, the program immediately crashes with a clear error instead of silently eating all your RAM. |

**Why it matters:** Scientific simulations can accidentally try to create billions of particles or allocate terabytes of data due to bugs or bad input. This allocator catches those mistakes instantly rather than letting your computer slow to a crawl and eventually freeze.

**For C++ experts:** Template allocator wrapping `std::allocator` (or any inner allocator) with a byte counter. Throws `std::bad_alloc` on exhaustion. Supports `rebind`. Not thread-safe unless you synchronize externally.

---

### 7.3 Lattice — The Repeating Grid

**Analogy:** Imagine a single tile on a bathroom floor. If you know the shape of one tile, you know the entire floor because the pattern just repeats forever. A lattice is a 3D version of that tile.

**What it contains:**

| File | Purpose |
|---|---|
| `lattice/lattice.hpp` | Defines the shape of one "tile" (unit cell) using three direction vectors: `a1`, `a2`, `a3`. By default, it's a simple cube. |
| `lattice/lattice.cpp` | The math: computing the volume, wrapping particles that walk out of one side back in on the opposite side (periodic boundaries), converting between coordinate systems, and validating that the lattice makes physical sense. |

**Key capabilities:**

- **Volume calculation** — How much 3D space does one tile occupy?
- **Minimum-image convention** — When measuring the distance between two particles in a repeating grid, always use the *shortest* path (which might go through a "wall" and come out the other side, like Pac-Man).
- **Coordinate conversion** — Switch between "real-world" positions (in metres) and "grid" positions (fractions of the tile).
- **Periodic wrapping** — A particle that leaves the right side reappears on the left side. Like the edges of the screen in Pac-Man or Asteroids.
- **Validation** — Checks that the vectors actually form a valid 3D box (not a flat pancake or an inside-out shape).

**For C++ experts:** The `Lattice` struct uses raw `Real[3]` arrays for basis vectors. Volume is the scalar triple product `a1 · (a2 × a3)`. `validate()` returns `std::optional<std::string>` — `nullopt` for valid, error message otherwise. Reciprocal vectors satisfy `b_i · a_j = 2π δ_ij`.

---

### 7.4 Physics — The Engine Room

This is where the actual science lives. It has five components:

#### 7.4.1 Particles (`physics/particle.hpp`)

**Analogy:** Each particle is a tiny billiard ball with three properties: where it is, how fast it's moving, and how hard it's being pushed right now.

- **`Particle`** — A single atom/particle: position, velocity, force, and mass (all in SI units).
- **`ParticleSystem`** — A collection of particles. Think: the whole pool table of billiard balls. It also calculates bulk properties like total kinetic energy, temperature, and centre of mass.

#### 7.4.2 Potentials (`physics/potential.hpp`)

**Analogy:** The "rules of attraction and repulsion." Imagine two magnets. If they're far apart, they gently pull each other closer. If they're smashed together, they violently push apart. The potential describes exactly *how strong* that push/pull is at every distance.

- **`Potential`** (base class) — The contract: "any interaction rule must be able to compute energy and force at a given distance."
- **`LennardJones`** — The most famous atomic interaction: atoms attract weakly at medium range and repel strongly at very short range. Used for noble gases like Argon. Described by two numbers: `epsilon` (how deep the energy "well" is) and `sigma` (how big the atom effectively is).
- **`HarmonicPotential`** — A spring: the farther you pull two atoms apart from their happy distance, the harder they pull back. Used for bonds between atoms in a molecule.
- **`ForceField`** — The calculator that loops over every pair of particles and adds up all the pushes and pulls using the chosen potential.

**For C++ experts:** Potentials use virtual dispatch. `force_div_r(r²)` returns `F/r` so callers can compute `F_vec = (F/r) * r_vec` without an extra `sqrt`. LJ precomputes `σ⁶` and `σ¹²`. Energy is shifted at cutoff for continuity. `ForceField::compute_forces` iterates all N(N-1)/2 pairs — O(N²).

#### 7.4.3 Neighbor List (`physics/neighbor_list.hpp`)

**Analogy:** If you're at a huge party and want to know who's standing close to you, you don't measure the distance to *everyone* in the room every second. Instead, you make a short list of the 10 people nearby and only re-check the full room if someone new walks close. That's a neighbor list.

- **`NeighborList`** — Maintains a list of which particles are close to each other. Only rebuilds the list when a particle has moved far enough that the list might be wrong.
- **`NeighborForceField`** — Uses the neighbor list to compute forces, checking only nearby pairs instead of all possible pairs. Dramatically faster for large systems.

**Why it matters:** Without this, a simulation of 10,000 atoms checks ~50 million pairs per step. With it, it checks only a few hundred thousand.

**For C++ experts:** Verlet list with skin distance. Rebuild is triggered when `max_displacement > skin/2`. Cutoff for list building is `cutoff + skin`. Stores per-particle neighbor indices in `vector<vector<size_t>>`. Positions at last build are cached for displacement check.

#### 7.4.4 Integrator (`physics/integrator.hpp`)

**Analogy:** A camera taking rapid-fire photos. Between each photo, you calculate "based on the current push/pull (force), where will each ball be, and how fast will it be going?" The integrator is the algorithm that answers this question.

- **`VelocityVerlet`** — The "gold standard" integrator for molecular dynamics. It's split into two half-steps:
  1. Update velocities halfway, then move positions.
  2. Compute new forces at the new positions.
  3. Update velocities the rest of the way.
  This split is what makes it accurate and energy-conserving.
- **`EulerIntegrator`** — A simpler, less accurate integrator included for comparison and testing. Not recommended for real simulations (energy will slowly drift over time).

**For C++ experts:** `VelocityVerlet` is symplectic and time-reversible (Hamiltonian-preserving). The split-step design requires an external force calculation between `step1` and `step2`. A convenience `integrate(system, force_fn)` method wraps this. `EulerIntegrator` is explicit forward Euler — O(dt) vs O(dt²) for Verlet.

#### 7.4.5 Thermostat (`physics/thermostat.hpp`)

**Analogy:** An air conditioner for your simulation. Without it, the total energy stays constant (like a perfectly insulated room). A thermostat lets you set a target temperature and the simulation gradually adjusts.

- **`Thermostat`** (base class) — The contract: "apply yourself to the particles and adjust their speeds."
- **`VelocityRescaleThermostat`** — The simple approach: look at the current temperature, calculate a correction factor, and scale all velocities up or down. Like turning a dial. Fast but not physically rigorous.
- **`AndersenThermostat`** — The rigorous approach: randomly pick some particles and give them new speeds drawn from the correct probability distribution for the target temperature. Like occasionally replacing a player with one who's running at the "right" speed. Physically correct but can disrupt dynamics.
- **`NullThermostat`** — Does nothing. Used when you want a constant-energy simulation (no temperature control).

**For C++ experts:** Thermostats use virtual dispatch. Berendsen-style (velocity rescale) uses coupling time `tau`. Andersen uses Poisson collision probability `P = nu * dt`. Andersen uses pimpl (`std::unique_ptr<Impl>`) to hide RNG implementation.

---

### 7.5 Sim — The Conductor

**Analogy:** An orchestra conductor. They don't play any instruments themselves. Instead, they tell the violins (integrator) when to play, the brass (force calculator) when to kick in, and the percussion (thermostat) when to adjust.

**What it contains:**

| File | Purpose |
|---|---|
| `sim/simulation.hpp` | Defines `SimulationParams` (settings: time step, end time, temperature, cutoff) and `Simulation` (the main object that orchestrates everything). |
| `sim/simulation.cpp` | The step-by-step logic for each time step. |

**What a single simulation step does:**

```
1. Validate that settings make sense (time step > 0, temperature ≥ 0, etc.)
2. Half-update velocities using current forces  (Integrator step 1)
3. Move particles to new positions               (Integrator step 1)
4. Wrap particles that went outside the box       (Periodic boundaries)
5. Calculate new forces at the new positions       (Force field or neighbor force field)
6. Finish updating velocities with new forces     (Integrator step 2)
7. Apply thermostat adjustment                     (Temperature control)
8. Advance the clock                               (time += dt)
9. Check if we're done                             (time ≥ end_time or max steps reached)
```

**For C++ experts:** `Simulation` owns the `ParticleSystem`, `VelocityVerlet`, `ForceField`/`NeighborForceField`, `Thermostat`, and `Lattice`. Potential is `shared_ptr` so multiple objects can reference it. `step()` returns `bool` (proceed or stop). Validation is fail-fast: `SimulationParams::validate()` returns `optional<string>`. An invalid simulation sets `valid_ = false` and stores the error message.

---

### 7.6 IO — The Translator

**Analogy:** A customs officer at a border. Internally, everything is in SI units (metres, seconds). But when data comes in from a file or goes out to a display, it might need to be in nanometres, femtoseconds, or electron-volts. IO handles those conversions, and *only* at the border.

**What it contains:**

| File | Purpose |
|---|---|
| `io/config.hpp` | `load_config(path)` — Loads simulation settings from a file. Returns defaults if no file is specified. |
| `io/config.cpp` | Implementation of config loading. |

**Key rule:** All unit conversions happen here and *only* here. The rest of the codebase never worries about unit conversion.

---

### 7.7 UI — The Visual Interface

**Requires:** Qt 6 (version ≥ 6.2). If Qt isn't installed, the app runs in terminal/CLI mode instead — no crash, no error.

**What it contains:**

| File | Purpose |
|---|---|
| `ui/main_window.hpp/.cpp` | The main window — menu bar, tab container, and status bar. Owns a timer that drives simulation steps without freezing the window. |
| `ui/simulation_tab.hpp/.cpp` | The "Simulation" tab — spin boxes for time step, end time, temperature. Run/Stop/Reset buttons. |
| `ui/lattice_tab.hpp/.cpp` | The "Lattice" tab — nine spin boxes for the three basis vectors. Shows the computed volume. |
| `ui/view_3d.hpp/.cpp` | The 3D renderer — an OpenGL widget that draws the lattice cell as a wireframe box with coloured axes. Supports mouse rotation and scroll zoom. |
| `ui/view_3d_tab.hpp/.cpp` | Wrapper tab that hosts the 3D view and any overlay controls. |

**How the GUI stays responsive:** The simulation doesn't run in `while(step())` — that would freeze the window. Instead, a `QTimer` fires periodically (like a metronome), runs one or more simulation steps, updates the display, and gives the window a chance to handle mouse clicks and repaints. When the simulation finishes, the timer stops.

**For C++ experts:** `MainWindow` uses pimpl idiom (`std::unique_ptr<Impl>`). Qt MOC (Meta Object Compiler) is automatically invoked by `run.sh` for all headers in `include/matsimu/ui/`. Signal/slot connections: `SimulationTab::run_requested → MainWindow::on_run`, `LatticeTab::lattice_changed → View3DTab::set_lattice`, etc.

---

## 8. The Physics, Explained Simply

### What is Molecular Dynamics (MD)?

Imagine you have a box of balls:

- Each ball has a **position** (where it is), a **velocity** (how fast and in which direction it's moving), and a **mass** (how heavy it is).
- Between every pair of balls, there's a rule about whether they attract or repel each other, and how strongly.
- You advance time by a tiny amount (the "time step"), figure out all the forces, move the balls accordingly, and repeat.

That's molecular dynamics. MATSIMU does exactly this, but the "balls" are atoms and the "rules" are physics equations.

### What is a Lennard-Jones potential?

The most common rule for how atoms interact. In English:

- **Very far apart:** No significant interaction. They ignore each other.
- **Medium distance:** Slight attraction. Like two magnets gently pulling closer.
- **Very close:** Extreme repulsion. Like trying to push two solid balls into each other.

The formula is: `U(r) = 4ε [(σ/r)¹² − (σ/r)⁶]`

- `r` = distance between two atoms
- `ε` (epsilon) = depth of the "energy well" — how strongly they attract at the sweet spot
- `σ` (sigma) = the effective "size" of the atom

### What is a time step?

Real atoms vibrate at about 10¹³ times per second. To track their motion accurately, the simulation needs to take snapshots much faster than that. A typical time step is **1 femtosecond** (10⁻¹⁵ seconds). MATSIMU's default is exactly `1e-15` seconds.

### What is temperature in a simulation?

Temperature is directly related to how fast the particles are moving. Hotter = faster. The formula is: `T = 2 × E_kinetic / (3 × N × k_B)`, where `N` is the number of particles and `k_B` is Boltzmann's constant.

---

## 9. Key Design Decisions

| Decision | Rationale |
|---|---|
| **Single entry point (`run.sh`)** | No CMake, no Makefile, no build system to learn. One script handles dependencies, compilation, and execution. |
| **C++17 with no heavy dependencies** | The C++17 standard library is powerful enough for everything the simulator needs. No Boost, no Eigen, no external math libraries (yet). |
| **Qt 6 is optional** | If you don't need or can't install graphics libraries, the app still works as a command-line tool. The code uses `#ifdef MATSIMU_USE_QT` to include or exclude the GUI at compile time. |
| **SI units throughout** | Prevents the most common class of scientific software bugs: unit mismatches. Every number inside the simulation is in metres, seconds, kilograms, etc. |
| **Fail-fast validation** | Every piece of input (lattice vectors, simulation parameters, material properties) is validated the moment it enters the system. If something is wrong, the program crashes with a clear error message instead of producing wrong results silently. |
| **One file, one job** | Each header file defines one concept. `lattice.hpp` does lattice math. `simulation.hpp` does simulation orchestration. No 5000-line god files. |
| **Dependencies flow inward** | Upper layers (UI, Sim) depend on lower layers (Core, Lattice). Lower layers never depend on upper layers. This prevents circular dependency tangles. |

---

## 10. File Map (Where Everything Lives)

```
matsimu/
├── run.sh                              ← THE entry point. Build & run in one command.
├── README.md                            ← Quick-start guide
├── docs/
│   ├── SUMMARY.md                       ← This document
│   ├── ARCHITECTURE.md                  ← Dependency rules and code layout
│   └── UNITS.md                         ← Unit system conventions
├── include/matsimu/                     ← PUBLIC API (header files)
│   ├── core/
│   │   ├── types.hpp                    ← Real, Index, Size type aliases
│   │   └── units.hpp                    ← SI unit constants (m, s, kg, K, J, Pa)
│   ├── alloc/
│   │   └── bounded_allocator.hpp        ← Memory-limited allocator
│   ├── lattice/
│   │   └── lattice.hpp                  ← Crystal lattice (3 basis vectors, volume, PBC)
│   ├── physics/
│   │   ├── particle.hpp                 ← Particle and ParticleSystem
│   │   ├── potential.hpp                ← Potentials: LennardJones, Harmonic, ForceField
│   │   ├── neighbor_list.hpp            ← Verlet neighbor list + NeighborForceField
│   │   ├── integrator.hpp               ← VelocityVerlet and EulerIntegrator
│   │   └── thermostat.hpp               ← Temperature control: Rescale, Andersen, Null
│   ├── sim/
│   │   └── simulation.hpp               ← SimulationParams + Simulation orchestrator
│   ├── io/
│   │   └── config.hpp                   ← Config file loading
│   └── ui/                              ← GUI (only compiled if Qt 6 is present)
│       ├── main_window.hpp              ← Main window with tabs + timer
│       ├── simulation_tab.hpp           ← Simulation parameter controls
│       ├── lattice_tab.hpp              ← Lattice vector editor
│       ├── view_3d.hpp                  ← OpenGL 3D lattice renderer
│       └── view_3d_tab.hpp             ← Tab wrapper for 3D view
├── src/                                 ← IMPLEMENTATION (source files)
│   ├── main.cpp                         ← Program entry point (CLI or GUI)
│   ├── lattice/lattice.cpp              ← Lattice math implementation
│   ├── sim/simulation.cpp               ← Simulation loop implementation
│   ├── io/config.cpp                    ← Config loading implementation
│   ├── physics/
│   │   ├── particle.cpp                 ← ParticleSystem methods
│   │   ├── potential.cpp                ← LJ, Harmonic, ForceField implementations
│   │   ├── neighbor_list.cpp            ← Neighbor list build/check/force calculation
│   │   ├── integrator.cpp               ← Verlet + Euler step implementations
│   │   └── thermostat.cpp               ← Thermostat implementations
│   └── ui/                              ← GUI implementations (only with Qt 6)
│       ├── main_window.cpp
│       ├── simulation_tab.cpp
│       ├── lattice_tab.cpp
│       ├── view_3d.cpp
│       └── view_3d_tab.cpp
├── examples/
│   └── lattice_example.cpp              ← Standalone lattice demo
└── build/                               ← Compiled output (auto-created, git-ignored)
```

---

## 11. Dependency Rules (What Talks to What)

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
  │   Lattice   │   IO   │ ──▶ Core, (IO also ▶ Sim for loading SimulationParams)
  └─────┬───────┴────┬───┘
        │            │
  ┌─────▼────────────▼────┐
  │     Alloc + Core      │ ──▶ nothing (they are the foundation)
  └───────────────────────┘
```

**Forbidden:**
- Core/Alloc must never import Lattice, Physics, Sim, IO, or UI.
- Lattice must never import Sim or UI.
- No circular dependencies. If A uses B, then B must not use A (directly or indirectly).

---

## 12. How to Add New Code

### Adding a new module (e.g., a new force type)

1. Create the header: `include/matsimu/physics/my_force.hpp`
2. Create the implementation: `src/physics/my_force.cpp`
3. The build system (`run.sh`) **auto-discovers** all `.cpp` files in `src/` (up to 2 levels deep), so no manual registration is needed.
4. Keep the dependency rules: your new file can use `core/`, `alloc/`, `lattice/`, and other `physics/` files — but not `sim/`, `io/`, or `ui/`.

### Adding a new UI component

1. Create the header in `include/matsimu/ui/` and the implementation in `src/ui/`.
2. `run.sh` auto-discovers all UI headers for MOC processing and all UI `.cpp` files for compilation.
3. The only requirement: Qt 6 (≥ 6.2) must be installed for UI code to compile.

### Adding a new example

1. Create a file in `examples/` (e.g., `examples/my_example.cpp`).
2. Wire it up in `src/main.cpp` by adding another `if (ex && std::string(ex) == "my_example")` branch.
3. Run it with `./run.sh --example my_example`.

---

## 13. Glossary

| Term | Plain English |
|---|---|
| **Atom / Particle** | A tiny ball with position, speed, and mass. The basic building block. |
| **CLI** | Command Line Interface — text-only mode, no windows or graphics. |
| **Cutoff** | A distance beyond which forces are ignored (too weak to matter). Saves computation time. |
| **dt (time step)** | How much time passes between each "frame" of the simulation. Smaller = more accurate but slower. |
| **Force** | A push or pull on a particle. Measured in newtons (N). |
| **GUI** | Graphical User Interface — the window with buttons and 3D graphics. |
| **Integrator** | The algorithm that figures out where particles move next, based on their current forces and velocities. |
| **Lattice** | A repeating 3D pattern defined by three direction vectors. The building block of crystals. |
| **Lennard-Jones (LJ)** | A math formula that describes how atoms attract at medium range and repel at short range. |
| **MD (Molecular Dynamics)** | A simulation method: compute forces, move atoms, repeat. |
| **Minimum image** | When computing distances in a repeating grid, always use the shortest possible path (even if it goes through a wall). |
| **MOC** | Qt's Meta Object Compiler — a code generator that enables Qt's signal/slot system. |
| **Neighbor list** | A cached list of which particles are close to each other, to avoid checking all possible pairs every step. |
| **PBC (Periodic Boundary Conditions)** | Particles that leave one side of the box re-enter from the opposite side, like Pac-Man. |
| **Potential** | A mathematical rule describing how the energy between two particles changes with distance. |
| **SI** | International System of Units: metres, kilograms, seconds, kelvin, etc. The global standard for measurement. |
| **Symplectic** | A property of the Velocity Verlet integrator: it preserves the total energy of the system over long times. |
| **Thermostat** | A mechanism to control the temperature of the simulated system. |
| **Unit cell** | The smallest piece of a crystal that, when repeated, fills all of space. Same as the lattice cell. |

---

*This document is part of the MATSIMU project. For architecture rules, see `ARCHITECTURE.md`. For unit conventions, see `UNITS.md`.*
