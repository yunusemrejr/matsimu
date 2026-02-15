#include <matsimu/physics/thermostat.hpp>
#include <random>
#include <cmath>

namespace matsimu {

// Boltzmann constant [J/K]
constexpr Real kB = 1.380649e-23;

// VelocityRescaleThermostat implementation
VelocityRescaleThermostat::VelocityRescaleThermostat(Real target_T, Real tau)
    : target_T_(target_T), tau_(tau) {}

void VelocityRescaleThermostat::apply(ParticleSystem& system, Real dt) {
    Real current_T = system.temperature();
    if (current_T <= 0.0 || target_T_ <= 0.0) return;
    
    // Berendsen scaling factor
    // lambda^2 = 1 + (dt/tau) * (T_target/T_current - 1)
    Real lambda_sq = 1.0 + (dt / tau_) * (target_T_ / current_T - 1.0);
    if (lambda_sq <= 0.0) return;
    
    Real lambda = std::sqrt(lambda_sq);
    
    for (auto& p : system.particles()) {
        p.vel[0] *= lambda;
        p.vel[1] *= lambda;
        p.vel[2] *= lambda;
    }
}

// AndersenThermostat implementation
class AndersenThermostat::Impl {
public:
    explicit Impl(unsigned seed) : gen_(seed) {}
    
    std::mt19937 gen_;
    std::normal_distribution<Real> dist_{0.0, 1.0};
};

AndersenThermostat::AndersenThermostat(Real target_T, Real nu, unsigned seed)
    : target_T_(target_T), nu_(nu) {
    if (seed == 0) {
        std::random_device rd;
        seed = rd();
    }
    impl_ = std::make_unique<Impl>(seed);
}

AndersenThermostat::~AndersenThermostat() = default;

void AndersenThermostat::apply(ParticleSystem& system, Real dt) {
    // Collision probability per particle per timestep
    Real prob = 1.0 - std::exp(-nu_ * dt);
    std::uniform_real_distribution<Real> uniform(0.0, 1.0);
    
    // Standard deviation of Maxwell-Boltzmann distribution
    // sigma = sqrt(kB * T / m)
    for (auto& p : system.particles()) {
        if (uniform(impl_->gen_) < prob) {
            Real sigma = std::sqrt(kB * target_T_ / p.mass);
            
            p.vel[0] = sigma * impl_->dist_(impl_->gen_);
            p.vel[1] = sigma * impl_->dist_(impl_->gen_);
            p.vel[2] = sigma * impl_->dist_(impl_->gen_);
        }
    }
}


Real AndersenThermostat::target_temperature() const {
    return target_T_;
}

void AndersenThermostat::set_target_temperature(Real T) {
    target_T_ = T;
}

} // namespace matsimu
