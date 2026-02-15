#include <matsimu/sim/simulation.hpp>
#include <cmath>
#include <algorithm>

namespace matsimu {

std::optional<std::string> SimulationParams::validate() const {
    if (!std::isfinite(dt) || dt <= 0.0) {
        return "Time step 'dt' must be positive and finite.";
    }
    if (!std::isfinite(end_time) || end_time < 0.0) {
        return "End time must be non-negative and finite.";
    }
    if (max_steps == 0) {
        return "Maximum steps must be greater than 0.";
    }
    if (!std::isfinite(temperature) || temperature < 0.0) {
        return "Temperature must be non-negative and finite.";
    }
    if (!std::isfinite(cutoff) || cutoff <= 0.0) {
        return "Force cutoff must be positive and finite.";
    }
    if (!std::isfinite(neighbor_skin) || neighbor_skin < 0.0) {
        return "Neighbor skin must be non-negative and finite.";
    }
    if (end_time > 0.0 && dt > end_time) {
        return "Time step cannot be greater than end time.";
    }
    return std::nullopt;
}

Simulation::Simulation(const SimulationParams& params,
                       std::shared_ptr<Potential> potential)
    : mode_(SimMode::MD), params_(params), time_(0), step_count_(0), valid_(false), last_epot_(0.0) {

    auto validation_error = params_.validate();
    if (validation_error) {
        error_msg_ = *validation_error;
        return;
    }

    integrator_ = std::make_unique<VelocityVerlet>(params_.dt);
    if (potential)
        set_potential(potential);
    valid_ = true;
}

Simulation::Simulation(const HeatDiffusionParams& heat_params)
    : mode_(SimMode::HeatDiffusion), params_(), time_(0), step_count_(0), valid_(false) {
    model_ = std::make_unique<HeatDiffusionModel>(heat_params);
    valid_ = model_->is_valid();
    if (!valid_)
        error_msg_ = model_->error_message();
}

void Simulation::set_potential(std::shared_ptr<Potential> pot) {
    if (params_.use_neighbor_list) {
        neighbor_force_field_ = std::make_unique<NeighborForceField>(
            pot, params_.cutoff, params_.neighbor_skin);
        force_field_.reset();
    } else {
        force_field_ = std::make_unique<ForceField>(pot);
        neighbor_force_field_.reset();
    }
}

void Simulation::set_integrator(std::unique_ptr<VelocityVerlet> integrator) {
    integrator_ = std::move(integrator);
}

void Simulation::initialize() {
    if (system_.empty()) return;
    
    // Zero center of mass velocity
    system_.zero_com_velocity();
    
    // Compute initial forces
    compute_forces();
}

void Simulation::compute_forces() {
    const Lattice* lat = has_lattice() ? &lattice_ : nullptr;
    
    if (neighbor_force_field_) {
        last_epot_ = neighbor_force_field_->compute_forces(system_, lat);
    } else if (force_field_) {
        last_epot_ = force_field_->compute_forces(system_, lat);
    } else {
        system_.clear_forces();
        last_epot_ = 0.0;
    }
}

bool Simulation::is_valid() const {
    if (model_) return model_->is_valid();
    return valid_;
}

const std::string& Simulation::error_message() const {
    if (model_) return model_->error_message();
    return error_msg_;
}

Real Simulation::time() const {
    if (model_) return model_->time();
    return time_;
}

std::size_t Simulation::step_count() const {
    if (model_) return model_->step_count();
    return step_count_;
}

bool Simulation::finished() const {
    if (model_) return model_->finished();
    if (!valid_) return true;
    if (step_count_ >= params_.max_steps) return true;
    if (params_.end_time > 0.0 && time_ >= params_.end_time) return true;
    return false;
}

bool Simulation::step() {
    if (model_) return model_->step();

    if (!valid_) {
        error_msg_ = "Simulation not properly initialized";
        return false;
    }
    if (finished()) return false;
    if (step_count_ >= params_.max_steps) {
        error_msg_ = "Maximum step count reached";
        return false;
    }

    integrator_->step1(system_);
    if (has_lattice())
        system_.apply_pbc(lattice_);
    compute_forces();
    integrator_->step2(system_);
    if (thermostat_)
        thermostat_->apply(system_, params_.dt);

    time_ += params_.dt;
    ++step_count_;

    if (!std::isfinite(time_)) {
        error_msg_ = "Time value became non-finite";
        valid_ = false;
        return false;
    }
    if (params_.end_time > 0.0) {
        const Real epsilon = params_.dt * 0.5;
        if (time_ >= params_.end_time - epsilon) {
            time_ = params_.end_time;
            return false;
        }
    }
    if (step_callback_)
        step_callback_(*this);
    return true;
}

void Simulation::run() {
    if (model_) {
        while (model_->step()) {}
        return;
    }
    initialize();
    while (step()) {}
}

}  // namespace matsimu
