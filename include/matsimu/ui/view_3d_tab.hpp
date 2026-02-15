#pragma once

#include <QWidget>
#include <cstddef>
#include <vector>
#include <matsimu/core/types.hpp>
#include <matsimu/lattice/lattice.hpp>
#include <matsimu/physics/particle.hpp>

class QLabel;
class QPushButton;

namespace matsimu {

class View3D;

/**
 * Tab that hosts the 3D view and optional overlay controls.
 */
class View3DTab : public QWidget {
  Q_OBJECT
 public:
  explicit View3DTab(QWidget* parent = nullptr);
  ~View3DTab() override;

  void set_lattice(const Lattice& lat);
  void set_particles(const ParticleSystem& system);
  void clear_particles();
  void set_temperature_field(const std::vector<Real>& T,
                             std::size_t nx, std::size_t ny,
                             Real T_cold, Real T_hot);
  void clear_temperature_field();
  void set_simulation_state(bool running, Real time_s, Real end_time_s, std::size_t step_count);

 private:
  View3D* view_{nullptr};
  QLabel* lattice_info_{nullptr};
  QLabel* run_info_{nullptr};
  QLabel* state_dot_{nullptr};
  QPushButton* btn_reset_view_{nullptr};
};

}  // namespace matsimu
