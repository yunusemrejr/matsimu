#include <matsimu/sim/heat_diffusion.hpp>
#include <cmath>
#include <algorithm>
#include <limits>

namespace matsimu {

Real HeatDiffusionParams::stability_limit() const {
  if (alpha <= 0 || dx <= 0) return 0;
  return (dx * dx) / (2.0 * alpha);
}

std::optional<std::string> HeatDiffusionParams::validate() const {
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
  if (n_cells < 2)
    return "Number of cells must be at least 2.";
  Real limit = stability_limit();
  if (!std::isfinite(limit) || dt > limit)
    return "Time step dt exceeds stability limit (dt <= dx²/(2*alpha)).";
  return std::nullopt;
}

HeatDiffusionModel::HeatDiffusionModel(const HeatDiffusionParams& params, std::size_t max_bytes)
    : params_(params), n_(params.n_cells), 
      T_(HeatAllocator(max_bytes)), T_next_(T_.get_allocator()) {
  auto err = params_.validate();
  if (err) {
    error_msg_ = *err;
    return;
  }
  T_.resize(n_);
  T_next_.resize(n_);
  initialize();
  valid_ = true;
}

void HeatDiffusionModel::initialize() {
  std::fill(T_.begin(), T_.end(), 0.0);
  if (n_ >= 2) {
    T_[0] = 0.0;
    T_[n_ - 1] = 0.0;
    for (std::size_t i = 1; i < n_ - 1; ++i)
      T_[i] = 300.0;  // Initial temperature [K] in interior
  }
  T_next_ = T_;
}

bool HeatDiffusionModel::step() {
  if (!valid_ || finished()) return false;

  Real r = params_.alpha * params_.dt / (params_.dx * params_.dx);
  // Explicit Euler: T_new[i] = T[i] + r*(T[i-1] - 2*T[i] + T[i+1])
  for (std::size_t i = 1; i + 1 < n_; ++i) {
    T_next_[i] = T_[i] + r * (T_[i - 1] - 2.0 * T_[i] + T_[i + 1]);
  }
  T_next_[0] = T_[0];
  T_next_[n_ - 1] = T_[n_ - 1];
  std::swap(T_, T_next_);

  time_ += params_.dt;
  ++step_count_;

  if (!std::isfinite(time_)) {
    error_msg_ = "Time became non-finite.";
    valid_ = false;
    return false;
  }
  return true;
}

bool HeatDiffusionModel::finished() const {
  if (!valid_) return true;
  if (step_count_ >= params_.max_steps) return true;
  if (time_ >= params_.end_time) return true;
  return false;
}

Real HeatDiffusionModel::time() const { return time_; }
std::size_t HeatDiffusionModel::step_count() const { return step_count_; }
const std::string& HeatDiffusionModel::error_message() const { return error_msg_; }
bool HeatDiffusionModel::is_valid() const { return valid_; }

}  // namespace matsimu
