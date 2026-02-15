#pragma once

#include <matsimu/core/types.hpp>
#include <matsimu/sim/model.hpp>
#include <vector>
#include <string>
#include <optional>
#include <cstddef>

namespace matsimu {

/**
 * Initial condition presets for 2D heat diffusion.
 *
 * HotCenter:   Gaussian hot spot at domain center; T decays to T_boundary.
 *              Models thermal shock from a point heat source.
 *
 * UniformHot:  Entire interior starts at T_hot; boundaries fixed at T_boundary.
 *              Models quenching / rapid cooling from edges.
 */
enum class HeatIC2D { HotCenter, UniformHot };

/**
 * Parameters for 2D explicit heat diffusion on a uniform NxN grid (SI).
 *
 * PDE:   ∂T/∂t = α ∇²T   (isotropic Fourier heat equation)
 * BCs:   Dirichlet — all edges fixed at T_boundary.
 * IC:    Determined by the `ic` field (see HeatIC2D).
 *
 * Invariants:
 *   - alpha > 0, dx > 0, dt > 0
 *   - dt ≤ stability_limit()  (explicit Euler, 5-point stencil)
 *   - nx >= 3, ny >= 3  (at least one interior cell)
 *   - T_hot > T_boundary >= 0
 *   - hot_radius_frac > 0 (only used for HotCenter)
 *
 * Unit system: SI throughout (m, s, K, m²/s).
 */
struct HeatDiffusion2DParams {
    Real alpha{1.11e-4};         ///< Thermal diffusivity [m²/s] (copper ≈ 1.11e-4)
    Real dx{1.25e-3};            ///< Grid spacing [m]
    Real dt{3.0e-3};             ///< Time step [s]
    Real end_time{0.0};          ///< End time [s]; 0 = run continuously
    std::size_t max_steps{10000000};
    std::size_t nx{80};          ///< Grid cells in x
    std::size_t ny{80};          ///< Grid cells in y
    Real T_boundary{300.0};      ///< Dirichlet boundary temperature [K]
    HeatIC2D ic{HeatIC2D::HotCenter}; ///< Initial condition preset
    Real T_hot{1200.0};          ///< Hot region temperature [K]
    Real hot_radius_frac{0.12};  ///< Gaussian σ as fraction of domain width (HotCenter only)

    /// Stability limit for 2D explicit Euler: dt ≤ dx² / (4·α).
    Real stability_limit() const;

    /// Returns error message if invalid, std::nullopt otherwise.
    std::optional<std::string> validate() const;
};

/**
 * 2D explicit heat diffusion model on a uniform Cartesian grid.
 *
 * ∂T/∂t = α (∂²T/∂x² + ∂²T/∂y²)
 *
 * Discretization:  5-point Laplacian, forward Euler in time.
 * Storage:         Row-major (T[j * nx + i] → cell at column i, row j).
 * Boundaries:      Dirichlet (first/last row/column fixed at T_boundary).
 *
 * All units SI; conversions at I/O only.
 */
class HeatDiffusion2DModel : public ISimModel {
public:
    explicit HeatDiffusion2DModel(const HeatDiffusion2DParams& params);

    bool step() override;
    bool finished() const override;
    Real time() const override;
    std::size_t step_count() const override;
    const std::string& error_message() const override;
    bool is_valid() const override;

    /// Temperature field [K] at current time (row-major, read-only).
    const std::vector<Real>& temperature() const { return T_; }

    /// Grid dimensions.
    std::size_t nx() const { return nx_; }
    std::size_t ny() const { return ny_; }

    /// Fixed colormap bounds — use these for consistent visualization.
    Real T_cold() const { return params_.T_boundary; }
    Real T_hot()  const { return params_.T_hot; }

private:
    HeatDiffusion2DParams params_;
    std::size_t nx_{0};
    std::size_t ny_{0};
    std::vector<Real> T_;
    std::vector<Real> T_next_;
    Real time_{0};
    std::size_t step_count_{0};
    std::string error_msg_;
    bool valid_{false};

    void initialize();
    void apply_initial_condition_hot_center();
    void apply_initial_condition_uniform_hot();
    void apply_boundary_conditions(std::vector<Real>& field);
};

}  // namespace matsimu
