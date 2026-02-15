---
name: 3d-simulation-material-science-expert
description: Provides domain expertise in 3D simulation and material science: crystal structures, lattices, discretization, numerical methods, and physical validation. Use when implementing or reviewing simulation logic, material models, mesh handling, time integration, or when the user mentions materials, crystals, 3D simulation, FEM, phase-field, or lattice.
---

# 3D Simulation and Material Science Expert

## When to Apply

Use this skill when working on:

- Crystal structures, Bravais lattices, unit cells, or atomic/molecular arrangements
- 3D spatial discretization (meshes, voxels, particles, finite elements)
- Time integration and numerical stability (explicit/implicit, CFL, stability limits)
- Material properties (elastic, thermal, electrical, phase transitions)
- Units, physical constants, and dimensional consistency
- Validation and sanity checks for simulation outputs

---

## Units and Dimensional Consistency

- **SI or consistent derived units** — Prefer SI (m, kg, s, K, A) or one consistent system (e.g. nm, eV, ps for atomistic). Document the unit system at the top of modules that define physics.
- **No silent unit mixing** — Length in nm vs m, time in fs vs s, energy in eV vs J: enforce one convention and convert at boundaries (I/O, external libs).
- **Angstrom (Å)** — Common in crystallography: 1 Å = 0.1 nm = 10⁻¹⁰ m. Be explicit when using Å.

---

## Crystal Structures and Lattices

### Bravais Lattices (3D)

- **Cubic**: simple (P), body-centered (I), face-centered (F)
- **Tetragonal**: P, I
- **Orthorhombic**: P, I, F, C
- **Rhombohedral**, **Hexagonal**, **Monoclinic**, **Triclinic**

Lattice is defined by **basis vectors** \(\mathbf{a}_1, \mathbf{a}_2, \mathbf{a}_3\) and optionally a **basis** (atoms per primitive cell). All lattice points: \(\mathbf{R} = n_1\mathbf{a}_1 + n_2\mathbf{a}_2 + n_3\mathbf{a}_3\).

### Common Conventions

- **Right-handed** coordinate system for \(\mathbf{a}_1, \mathbf{a}_2, \mathbf{a}_3\).
- **Fractional coordinates** in [0, 1) for atoms inside the unit cell.
- **Reciprocal lattice** for diffraction and k-space: \(\mathbf{b}_i \cdot \mathbf{a}_j = 2\pi \delta_{ij}\).

### Implementation Notes

- Store lattice as 3 vectors (or 3×3 matrix). Provide helpers for volume \(V = \mathbf{a}_1 \cdot (\mathbf{a}_2 \times \mathbf{a}_3)\), reciprocal vectors, and minimum image (periodic) distance.
- For periodic boundaries, use minimum-image convention so each pair is counted once and distance is within half the box in each direction.

---

## 3D Spatial Discretization

### Choice of Representation

| Approach | Best for | Tradeoffs |
|----------|----------|-----------|
| **Structured grid (voxels)** | Fields (density, concentration), simple BCs | Easy indexing; less accurate for curved boundaries |
| **Unstructured mesh** | Complex geometry, FEM | Flexible; needs mesh data and connectivity |
| **Particles / point cloud** | Molecular dynamics, discrete elements | No mesh; neighbor search cost (e.g. cell lists, k-d tree) |
| **Lattice sites** | Crystals, spin systems, discrete models | Natural for Bravais + basis; fixed topology |

### Grid Conventions

- **Index order** — Fix one convention (e.g. row-major: x fastest, then y, then z). Document it.
- **Cell-centered vs node-centered** — Be consistent. Cell-centered: value at cell center; node-centered: value at vertices. Affects gradients and boundary conditions.
- **Domain** — Define extent explicitly: \([x_0, x_1] \times [y_0, y_1] \times [z_0, z_1]\) or index ranges and spacing \((\Delta x, \Delta y, \Delta z)\).

### Boundary Conditions

- **Periodic** — Copy opposite side; use for bulk behavior, no surfaces.
- **Dirichlet** — Fixed value at boundary (e.g. temperature, concentration).
- **Neumann** — Fixed normal derivative (e.g. flux, insulation).
- **Free surface** — No constraint; ensure discrete operators don’t read past the domain (or use ghost cells).

---

## Time Integration and Stability

### Explicit vs Implicit

- **Explicit** — New state from old state only; easy to implement, subject to stability limits (e.g. CFL for transport/diffusion).
- **Implicit** — New state appears in the equation; often unconditionally stable for diffusion, but requires solve (linear or nonlinear).

### Stability and CFL

- **Diffusion** \(\partial_t u = D \nabla^2 u\): explicit 1D stability \(\Delta t \leq \frac{(\Delta x)^2}{2D}\) (factor varies with dimension and scheme).
- **Advection** \(\partial_t u + \mathbf{v} \cdot \nabla u = 0\): CFL condition \(|\mathbf{v}| \Delta t / \Delta x \lesssim 1\) (order 1).
- **Coupled systems** — Limit \(\Delta t\) by the most restrictive process (e.g. diffusion or fastest wave).

### Recommendations

- Use a single, documented time step for the whole system (or per-physics limit). Validate that \(\Delta t\) is below stability limits; optionally check at runtime and warn or abort.
- For stiff problems (e.g. fast + slow processes), prefer implicit or semi-implicit for the stiff part.

---

## Material Properties and Constitutive Relations

### Elasticity (small strain)

- **Stress** \(\sigma\), **strain** \(\varepsilon\); **Hooke’s law** \(\sigma_{ij} = C_{ijkl} \varepsilon_{kl}\).
- **Isotropic**: two parameters (e.g. Young’s modulus \(E\), Poisson’s ratio \(\nu\), or Lamé \(\lambda, \mu\)). Bulk modulus \(K\), shear modulus \(G\).
- Ensure symmetry and positive definiteness of stiffness; use consistent Voigt or full-tensor indexing.

### Thermal

- **Fourier**: \(\mathbf{q} = -k \nabla T\); \(\rho c_p \partial_t T = -\nabla \cdot \mathbf{q} + s\).
- **Units**: \(k\) (W/(m·K)), \(\rho\) (kg/m³), \(c_p\) (J/(kg·K)). Check that \(\alpha = k/(\rho c_p)\) has units m²/s (thermal diffusivity).

### Phase and Concentration

- **Phase-field** — Order parameter \(\phi \in [0,1]\); free energy and mobility; ensure boundedness and conservation if required.
- **Concentration / diffusion** — Fick’s law; possible coupling to stress (e.g. swelling) or temperature.

### Implementation Notes

- Store material parameters in one place (struct or config); same names as in docs or papers.
- Validate inputs: \(E > 0\), \(0 < \nu < 0.5\) for isotropic 3D, \(k > 0\), etc. Fail fast on invalid values.

---

## Validation and Sanity Checks

- **Conservation** — If mass, energy, or momentum should be conserved, integrate over domain and check (or monitor) over time.
- **Symmetry** — Use symmetric problems (e.g. sphere, cube) to check isotropy and absence of bias.
- **Limiting cases** — Compare to analytical solutions (e.g. 1D diffusion, steady heat in a slab).
- **Units** — Print or log key quantities in SI (or chosen system) once; spot-check that magnitudes are plausible.
- **Scaling** — If doubling resolution or halving \(\Delta t\), expect known convergence order (e.g. second order in space/time); document and test.

---

## Invariants to Enforce

- One consistent **unit system** per codebase; conversions only at I/O or library boundaries.
- **Lattice and basis** defined unambiguously; right-handed basis; periodic min-image when applicable.
- **Time step** within stability limits for any explicit scheme; documented and optionally checked at runtime.
- **Material parameters** validated on load (positive moduli, valid Poisson ratio, etc.).
- **Boundary conditions** implemented so that discrete operators do not read out-of-bounds; ghost cells or stencil truncation applied consistently.

---

## Quick Reference

| Topic | Typical pitfall | Fix |
|-------|------------------|-----|
| Units | Mixing nm and m, eV and J | Single unit system; convert at boundaries |
| Lattice | Left-handed basis, wrong volume | Right-handed; \(V = \mathbf{a}_1 \cdot (\mathbf{a}_2 \times \mathbf{a}_3)\) |
| Periodic BC | Double-counting or wrong min-image | Half-box criterion; one image per pair |
| Explicit time step | Instability, blow-up | Enforce CFL / diffusion limit; validate or warn |
| Elasticity | Wrong symmetry or negative stiffness | Enforce \(C_{ijkl}\) symmetry; validate \(E, \nu\) |
| Conservation | Drift in total mass/energy | Monitor integral; fix fluxes or time scheme |
