#include <matsimu/sim/heat_diffusion_2d.hpp>
#include <cmath>
#include <algorithm>
#include <limits>

namespace matsimu {

// ---------------------------------------------------------------------------
// HeatDiffusion2DParams
// ---------------------------------------------------------------------------

Real HeatDiffusion2DParams::stability_limit() const {
    if (alpha <= 0.0 || dx <= 0.0) return 0.0;
    // 2D explicit Euler with 5-point stencil: dt ≤ dx² / (4·α)
    return (dx * dx) / (4.0 * alpha);
}

std::optional<std::string> HeatDiffusion2DParams::validate() const {
    if (!std::isfinite(alpha) || alpha <= 0.0)
        return "Thermal diffusivity alpha must be positive and finite.";
    if (!std::isfinite(dx) || dx <= 0.0)
        return "Grid spacing dx must be positive and finite.";
    if (!std::isfinite(dt) || dt <= 0.0)
        return "Time step dt must be positive and finite.";
    if (!std::isfinite(end_time) || end_time < 0.0)
        return "End time must be non-negative and finite.";
    if (max_steps == 0)
        return "Maximum steps must be greater than 0.";
    if (nx < 3)
        return "Grid dimension nx must be at least 3 (need interior cells).";
    if (ny < 3)
        return "Grid dimension ny must be at least 3 (need interior cells).";
    if (!std::isfinite(T_boundary) || T_boundary < 0.0)
        return "Boundary temperature must be non-negative and finite.";
    if (!std::isfinite(T_hot) || T_hot <= T_boundary)
        return "Hot temperature must be finite and greater than boundary temperature.";
    if (ic == HeatIC2D::HotCenter && (!std::isfinite(hot_radius_frac) || hot_radius_frac <= 0.0))
        return "Hot radius fraction must be positive and finite.";

    const Real limit = stability_limit();
    if (!std::isfinite(limit) || dt > limit)
        return "Time step dt exceeds 2D stability limit: dt <= dx^2 / (4*alpha). "
               "Reduce dt or increase dx.";

    return std::nullopt;
}

// ---------------------------------------------------------------------------
// HeatDiffusion2DModel
// ---------------------------------------------------------------------------

HeatDiffusion2DModel::HeatDiffusion2DModel(const HeatDiffusion2DParams& params, std::size_t max_bytes)
    : params_(params), nx_(params.nx), ny_(params.ny),
      T_(HeatAllocator(max_bytes)), T_next_(HeatAllocator(max_bytes)) {

    auto err = this->params_.validate();
    if (err) {
        this->error_msg_ = *err;
        return;
    }

    this->T_.resize(this->nx_ * this->ny_);
    this->T_next_.resize(this->nx_ * this->ny_);
    this->initialize();
    this->valid_ = true;
}

void HeatDiffusion2DModel::initialize() {
    switch (this->params_.ic) {
        case HeatIC2D::HotCenter:
            this->apply_initial_condition_hot_center();
            break;
        case HeatIC2D::UniformHot:
            this->apply_initial_condition_uniform_hot();
            break;
    }
    // Enforce Dirichlet boundaries on initial state.
    this->apply_boundary_conditions(this->T_);
    this->T_next_ = this->T_;
}

void HeatDiffusion2DModel::apply_initial_condition_hot_center() {
    // Gaussian hot spot centered in the domain.
    // T(x,y) = T_boundary + (T_hot - T_boundary) * exp(-r²/(2σ²))
    // where r = distance from center in fractional coords [0,1]×[0,1],
    //       σ = hot_radius_frac.
    const Real sigma = this->params_.hot_radius_frac;
    const Real inv_2sigma2 = 1.0 / (2.0 * sigma * sigma);
    const Real T_delta = this->params_.T_hot - this->params_.T_boundary;

    for (std::size_t j = 0; j < this->ny_; ++j) {
        const Real fy = (static_cast<Real>(j) + 0.5) / static_cast<Real>(this->ny_) - 0.5;
        for (std::size_t i = 0; i < this->nx_; ++i) {
            const Real fx = (static_cast<Real>(i) + 0.5) / static_cast<Real>(this->nx_) - 0.5;
            const Real r2 = fx * fx + fy * fy;
            this->T_[j * this->nx_ + i] = this->params_.T_boundary + T_delta * std::exp(-r2 * inv_2sigma2);
        }
    }
}

void HeatDiffusion2DModel::apply_initial_condition_uniform_hot() {
    // Interior: uniformly hot.  Boundary cells will be overwritten by BC.
    std::fill(this->T_.begin(), this->T_.end(), this->params_.T_hot);
}

void HeatDiffusion2DModel::apply_boundary_conditions(std::vector<Real, HeatDiffusion2DModel::HeatAllocator>& field) {
    const Real Tb = this->params_.T_boundary;
    // Top and bottom rows.
    for (std::size_t i = 0; i < this->nx_; ++i) {
        field[i] = Tb;                          // j = 0  (bottom)
        field[(this->ny_ - 1) * this->nx_ + i] = Tb;        // j = ny-1 (top)
    }
    // Left and right columns.
    for (std::size_t j = 0; j < ny_; ++j) {
        field[j * nx_] = Tb;                    // i = 0  (left)
        field[j * nx_ + (nx_ - 1)] = Tb;        // i = nx-1 (right)
    }
}

bool HeatDiffusion2DModel::step() {
    if (!valid_ || finished()) return false;

    // Diffusion number r = α · dt / dx²
    const Real r = params_.alpha * params_.dt / (params_.dx * params_.dx);

    // Explicit Euler with 5-point stencil:
    //   T_new[i,j] = T[i,j] + r * (T[i-1,j] + T[i+1,j] + T[i,j-1] + T[i,j+1] - 4·T[i,j])
    for (std::size_t j = 1; j + 1 < ny_; ++j) {
        for (std::size_t i = 1; i + 1 < nx_; ++i) {
            const std::size_t idx = j * nx_ + i;
            this->T_next_[idx] = this->T_[idx] + r * (
                this->T_[idx - 1] + this->T_[idx + 1] +       // x neighbors
                this->T_[idx - nx_] + this->T_[idx + nx_] -    // y neighbors
                4.0 * this->T_[idx]
            );
        }
    }

    // Fix Dirichlet boundaries.
    apply_boundary_conditions(T_next_);

    std::swap(T_, T_next_);
    time_ += params_.dt;
    ++step_count_;

    // Sanity: check time didn't go non-finite (should never happen with valid params).
    if (!std::isfinite(time_)) {
        error_msg_ = "Time became non-finite.";
        valid_ = false;
        return false;
    }
    return true;
}

bool HeatDiffusion2DModel::finished() const {
    if (!valid_) return true;
    if (step_count_ >= params_.max_steps) return true;
    if (params_.end_time > 0.0 && time_ >= params_.end_time) return true;
    return false;
}

Real HeatDiffusion2DModel::time() const { return time_; }
std::size_t HeatDiffusion2DModel::step_count() const { return step_count_; }
const std::string& HeatDiffusion2DModel::error_message() const { return error_msg_; }
bool HeatDiffusion2DModel::is_valid() const { return valid_; }

}  // namespace matsimu
