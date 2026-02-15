#pragma once

#include <matsimu/core/types.hpp>
#include <matsimu/physics/particle.hpp>
#include <matsimu/physics/integrator.hpp>
#include <matsimu/physics/potential.hpp>
#include <matsimu/physics/neighbor_list.hpp>
#include <matsimu/physics/thermostat.hpp>
#include <matsimu/lattice/lattice.hpp>
#include <matsimu/sim/model.hpp>
#include <matsimu/sim/heat_diffusion.hpp>
#include <matsimu/sim/heat_diffusion_2d.hpp>
#include <memory>
#include <optional>
#include <string>
#include <functional>

namespace matsimu {

/**
 * Enhanced simulation parameters for molecular dynamics.
 * dx is used by UI and by heat-diffusion; for MD it may be unused.
 */
struct SimulationParams {
    Real dt{1e-15};           // time step [s]
    Real dx{1e-9};            // grid/spacing [m] (heat diffusion; UI)
    Real end_time{0};         // simulation end time [s]
    std::size_t max_steps{10000000};
    
    // Physics parameters
    Real temperature{300.0};  // target temperature [K]
    Real cutoff{1.0e-9};      // force cutoff [m]
    bool use_neighbor_list{true};  // use neighbor list optimization
    Real neighbor_skin{0.2e-9};   // neighbor list skin [m]
    
    std::optional<std::string> validate() const;
};

/**
 * Simulation mode: which physics model is active.
 */
enum class SimMode { MD, HeatDiffusion, HeatDiffusion2D };

/**
 * Molecular dynamics simulation with full physics engine.
 * 
 * Features:
 * - Velocity Verlet integration
 * - Pairwise potentials (Lennard-Jones, etc.)
 * - Neighbor list for O(N) efficiency
 * - Thermostats for temperature control
 * - Periodic boundary conditions
 * - Energy and trajectory tracking
 */
class Simulation {
public:
    /// Construct MD simulation.
    Simulation(const SimulationParams& params,
               std::shared_ptr<Potential> potential = nullptr);

    /// Construct 1D heat-diffusion simulation (orchestrates stepping; math in HeatDiffusionModel).
    explicit Simulation(const HeatDiffusionParams& heat_params);

    /// Construct 2D heat-diffusion simulation (math in HeatDiffusion2DModel).
    explicit Simulation(const HeatDiffusion2DParams& heat_2d_params);

    /// Check if simulation is valid
    bool is_valid() const;

    /// Get error message (never null)
    const std::string& error_message() const;

    /// Advance one step (MD or heat depending on mode)
    bool step();

    /// Run simulation to completion
    void run();

    /// Check if finished
    bool finished() const;

    /// Current simulation mode
    SimMode mode() const { return mode_; }

    // Accessors
    Real time() const;
    std::size_t step_count() const;
    const SimulationParams& params() const { return params_; }
    
    // System access
    ParticleSystem& system() { return system_; }
    const ParticleSystem& system() const { return system_; }
    
    // Lattice/boundary conditions
    void set_lattice(const Lattice& lat) { lattice_ = lat; }
    const Lattice* lattice() const { return &lattice_; }
    bool has_lattice() const { return lattice_.volume() != 0.0; }
    
    // Potential
    void set_potential(std::shared_ptr<Potential> pot);
    Potential* potential() const { return force_field_ ? force_field_->potential() : nullptr; }
    
    // Thermostat
    void set_thermostat(std::shared_ptr<Thermostat> therm) { thermostat_ = std::move(therm); }
    Thermostat* thermostat() const { return thermostat_.get(); }
    
    // Integrator
    void set_integrator(std::unique_ptr<VelocityVerlet> integrator);
    VelocityVerlet* integrator() const { return integrator_.get(); }
    
    // Energy tracking
    Real kinetic_energy() const { return system_.kinetic_energy(); }
    Real potential_energy() const { return last_epot_; }
    Real total_energy() const { return kinetic_energy() + potential_energy(); }
    Real temperature() const { return system_.temperature(); }
    
    /// Access the 2D heat model (nullptr if mode != HeatDiffusion2D).
    const HeatDiffusion2DModel* heat_2d_model() const;

    // Callbacks
    using StepCallback = std::function<void(const Simulation&)>;
    void set_step_callback(StepCallback cb) { step_callback_ = std::move(cb); }

private:
    SimMode mode_{SimMode::MD};
    std::unique_ptr<ISimModel> model_;  /// When set, step/time/finished delegate to model

    SimulationParams params_;
    Real time_{0};
    std::size_t step_count_{0};
    bool valid_{false};
    std::string error_msg_;

    // Physics components (MD only)
    ParticleSystem system_;
    Lattice lattice_;
    std::unique_ptr<VelocityVerlet> integrator_;
    std::unique_ptr<ForceField> force_field_;
    std::unique_ptr<NeighborForceField> neighbor_force_field_;
    std::shared_ptr<Thermostat> thermostat_;

    // State (MD only)
    Real last_epot_{0.0};
    StepCallback step_callback_;

    void initialize();
    void compute_forces();
};

}  // namespace matsimu
