#include <matsimu/physics/neighbor_list.hpp>
#include <algorithm>
#include <cmath>

namespace matsimu {

NeighborList::NeighborList(Real cutoff, Real skin)
    : cutoff_(cutoff), skin_(skin), num_pairs_(0) {
    set_cutoff(cutoff, skin);
}

void NeighborList::set_cutoff(Real cutoff, Real skin) {
    cutoff_ = cutoff;
    skin_ = skin;
    cutoff_sq_ = (cutoff_ + skin_) * (cutoff_ + skin_);
    skin_half_sq_ = (skin_ * 0.5) * (skin_ * 0.5);
}

std::size_t NeighborList::build(const ParticleSystem& system, const Lattice* lattice) {
    std::size_t n = system.size();
    neighbors_.clear();
    neighbors_.resize(n);
    last_positions_.resize(n);
    num_pairs_ = 0;
    
    // Store positions
    for (std::size_t i = 0; i < n; ++i) {
        last_positions_[i] = {system[i].pos[0], system[i].pos[1], system[i].pos[2]};
    }
    
    // Build list
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            Real dx[3];
            Real r2 = distance_sq(system[i], system[j], lattice, dx);
            if (r2 < cutoff_sq_) {
                neighbors_[i].push_back(j);
                ++num_pairs_;
            }
        }
    }
    
    return num_pairs_;
}

bool NeighborList::needs_rebuild(const ParticleSystem& system, const Lattice* lattice) const {
    if (system.size() != last_positions_.size()) return true;
    
    for (std::size_t i = 0; i < system.size(); ++i) {
        Real dx[3];
        dx[0] = system[i].pos[0] - last_positions_[i][0];
        dx[1] = system[i].pos[1] - last_positions_[i][1];
        dx[2] = system[i].pos[2] - last_positions_[i][2];
        
        if (lattice) {
            // Check drift using min-image to ignore periodic wraps
            Real frac[3];
            lattice->cartesian_to_fractional(dx, frac);
            // We want the displacement from the origin in fractional space, wrapped to [-0.5, 0.5]
            for (int d = 0; d < 3; ++d) {
                frac[d] = frac[d] - std::round(frac[d]);
            }
            lattice->fractional_to_cartesian(frac, dx);
        }
        
        Real dr2 = dx[0]*dx[0] + dx[1]*dx[1] + dx[2]*dx[2];
        if (dr2 > skin_half_sq_) return true;
    }
    
    return false;
}


void NeighborList::clear() {
    neighbors_.clear();
    last_positions_.clear();
    num_pairs_ = 0;
}

Real NeighborList::distance_sq(const Particle& p1, const Particle& p2,
                                const Lattice* lattice, Real dx[3]) {
    dx[0] = p2.pos[0] - p1.pos[0];
    dx[1] = p2.pos[1] - p1.pos[1];
    dx[2] = p2.pos[2] - p1.pos[2];
    
    if (lattice) {
        // Use lattice's general min-image displacement (correct for all lattice types).
        lattice->min_image_displacement(p1.pos, p2.pos, dx);
    }
    
    return dx[0]*dx[0] + dx[1]*dx[1] + dx[2]*dx[2];
}

// NeighborForceField implementation
NeighborForceField::NeighborForceField(std::shared_ptr<Potential> potential,
                                       Real cutoff, Real skin)
    : potential_(std::move(potential)), nlist_(cutoff, skin) {}

Real NeighborForceField::compute_forces(ParticleSystem& system, const Lattice* lattice) {
    if (nlist_.needs_rebuild(system, lattice)) {
        nlist_.build(system, lattice);
    }
    return compute_forces_internal(system, lattice);
}

Real NeighborForceField::compute_energy(const ParticleSystem& system, const Lattice* lattice) {
    if (nlist_.needs_rebuild(system, lattice)) {
        nlist_.build(system, lattice);
    }
    
    if (!potential_) return 0.0;
    
    Real epot = 0.0;
    std::size_t n = system.size();
    
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j : nlist_.neighbors(i)) {
            // Calculate actual distance
            Real dx[3];
            Real r2 = NeighborList::distance_sq(system[i], system[j], lattice, dx);

            if (r2 < potential_->cutoff_squared()) {
                epot += potential_->energy(r2);
            }
        }
    }

    return epot;
}

Real NeighborForceField::compute_forces_internal(ParticleSystem& system, const Lattice* lattice) {
    if (!potential_) return 0.0;

    system.clear_forces();
    Real epot = 0.0;
    std::size_t n = system.size();

    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j : nlist_.neighbors(i)) {
            Real dx[3];
            Real r2 = NeighborList::distance_sq(system[i], system[j], lattice, dx);
            
            if (r2 < potential_->cutoff_squared()) {
                Real e = potential_->energy(r2);
                Real f_div_r = potential_->force_div_r(r2);
                
                epot += e;
                
                Real fx = f_div_r * dx[0];
                Real fy = f_div_r * dx[1];
                Real fz = f_div_r * dx[2];
                
                system[i].add_force(fx, fy, fz);
                system[j].add_force(-fx, -fy, -fz);
            }
        }
    }
    
    return epot;
}

} // namespace matsimu
