#pragma once

#include <matsimu/core/types.hpp>
#include <matsimu/physics/particle.hpp>
#include <matsimu/lattice/lattice.hpp>
#include <matsimu/physics/potential.hpp>
#include <vector>
#include <memory>
#include <array>

namespace matsimu {

/**
 * Verlet neighbor list for efficient force calculation.
 * 
 * Instead of checking all N*(N-1)/2 pairs, we only check neighbors
 * within a skin radius (cutoff + skin).
 * 
 * The list is rebuilt when any particle moves more than skin/2.
 */
class NeighborList {
public:
    /**
     * Construct neighbor list.
     * 
     * @param cutoff Force cutoff distance
     * @param skin Buffer distance for list rebuilds (typically 0.2-0.3 * cutoff)
     */
    NeighborList(Real cutoff, Real skin);
    
    /// Set cutoff and skin distances
    void set_cutoff(Real cutoff, Real skin);
    
    /// Get cutoff distance
    Real cutoff() const { return cutoff_; }
    
    /// Get skin distance
    Real skin() const { return skin_; }
    
    /// Get total cutoff (cutoff + skin)
    Real total_cutoff() const { return cutoff_ + skin_; }
    
    /**
     * Build or rebuild the neighbor list.
     * Should be called when particles have moved significantly.
     * 
     * @param system Particle system
     * @param lattice Lattice for periodic boundaries (nullptr = non-periodic)
     * @return Number of neighbor pairs
     */
    std::size_t build(const ParticleSystem& system, const Lattice* lattice = nullptr);
    
    /**
     * Check if list needs to be rebuilt.
     * Compares current positions with positions at last build.
     * 
     * @param system Particle system
     * @param lattice Lattice for periodic boundaries
     * @return true if any particle moved more than skin/2
     */
    bool needs_rebuild(const ParticleSystem& system, const Lattice* lattice = nullptr) const;
    
    /// Access neighbors of particle i
    const std::vector<std::size_t>& neighbors(std::size_t i) const {
        return neighbors_[i];
    }
    
    /// Total number of neighbor pairs
    std::size_t num_pairs() const { return num_pairs_; }
    
    /// Number of particles in list
    std::size_t size() const { return neighbors_.size(); }
    
    /// Clear the list
    void clear();
    
    /// Calculate squared distance with optional PBC, returns displacement vector
    static Real distance_sq(const Particle& p1, const Particle& p2,
                           const Lattice* lattice, Real dx[3]);

private:
    Real cutoff_;           // Force cutoff
    Real skin_;             // Skin buffer
    Real cutoff_sq_;        // (cutoff + skin)^2
    Real skin_half_sq_;     // (skin/2)^2 for rebuild check
    
    std::vector<std::vector<std::size_t>> neighbors_;  // neighbors_[i] = list of neighbors of i
    std::vector<std::array<Real, 3>> last_positions_;  // Positions at last build
    std::size_t num_pairs_ = 0;
    
    /// Check if distance squared is within cutoff
    bool within_cutoff(Real r2) const { return r2 < cutoff_sq_; }
};

/**
 * Force field calculator using neighbor lists.
 * More efficient than all-pairs for large systems.
 */
class NeighborForceField {
public:
    NeighborForceField(std::shared_ptr<Potential> potential, 
                       Real cutoff, Real skin);
    
    /// Set the potential
    void set_potential(std::shared_ptr<Potential> potential) {
        potential_ = std::move(potential);
    }
    
    /// Get the potential
    Potential* potential() const { return potential_.get(); }
    
    /// Access the neighbor list
    NeighborList& neighbor_list() { return nlist_; }
    const NeighborList& neighbor_list() const { return nlist_; }
    
    /**
     * Calculate forces using neighbor list.
     * Automatically rebuilds list if needed.
     * 
     * @param system Particle system
     * @param lattice Lattice for periodic boundaries
     * @return Total potential energy
     */
    Real compute_forces(ParticleSystem& system, const Lattice* lattice = nullptr);
    
    /// Calculate energy only (uses neighbor list)
    Real compute_energy(const ParticleSystem& system, const Lattice* lattice = nullptr);

private:
    std::shared_ptr<Potential> potential_;
    NeighborList nlist_;
    
    Real compute_forces_internal(ParticleSystem& system, const Lattice* lattice);
};

} // namespace matsimu
