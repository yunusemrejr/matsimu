#pragma once

#include <matsimu/core/types.hpp>
#include <matsimu/sim/model.hpp>
#include <matsimu/alloc/bounded_allocator.hpp>
#include <vector>
#include <string>
#include <optional>
#include <cstddef>

namespace matsimu {

/**
 * Parameters for 1D explicit heat diffusion (SI).
 * Invariants: alpha > 0, dx > 0, dt > 0, dt <= stability_limit (dx²/(2*alpha)).
 */
struct HeatDiffusionParams {
  Real alpha{1e-5};        /// Thermal diffusivity [m²/s]
  Real dx{1e-3};           /// Grid spacing [m]
  Real dt{1e-6};           /// Time step [s]
  Real end_time{1e-3};     /// End time [s]
  std::size_t max_steps{1000000};
  std::size_t n_cells{100}; /// Number of cells (1D rod)

  /// Stability limit for explicit Euler: dt <= dx²/(2*alpha).
  Real stability_limit() const;

  /// Returns error message if invalid, std::nullopt otherwise.
  std::optional<std::string> validate() const;
};

/**
 * 1D explicit heat diffusion model.
 * ∂T/∂t = α ∂²T/∂x²; Dirichlet boundaries (fixed end temperatures).
 * All units SI; conversions at I/O only.
 */
class HeatDiffusionModel : public ISimModel {
public:
  using HeatAllocator = bounded_allocator<Real>;

  explicit HeatDiffusionModel(const HeatDiffusionParams& params, 
                              std::size_t max_bytes = 256 * 1024 * 1024);

  bool step() override;
  bool finished() const override;
  Real time() const override;
  std::size_t step_count() const override;
  const std::string& error_message() const override;
  bool is_valid() const override;

  /// Temperature field [K] at current time (read-only).
  const std::vector<Real, HeatAllocator>& temperature() const { return T_; }
  /// Number of grid points.
  std::size_t n_cells() const { return n_; }

private:
  HeatDiffusionParams params_;
  std::size_t n_{0};
  std::vector<Real, HeatAllocator> T_;
  std::vector<Real, HeatAllocator> T_next_;
  Real time_{0};
  std::size_t step_count_{0};
  std::string error_msg_;
  bool valid_{false};

  void initialize();
};


}  // namespace matsimu
