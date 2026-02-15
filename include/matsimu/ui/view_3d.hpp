#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <matsimu/core/types.hpp>
#include <matsimu/lattice/lattice.hpp>
#include <matsimu/physics/particle.hpp>
#include <cstddef>
#include <memory>
#include <vector>

namespace matsimu {

/**
 * 3D OpenGL view: draws lattice unit cell (axes + box) and particle system.
 * Read-only use of Lattice and ParticleSystem; no modification of simulation state.
 */
class View3D : public QOpenGLWidget, protected QOpenGLFunctions {
   Q_OBJECT
  public:
   explicit View3D(QWidget* parent = nullptr);
   ~View3D() override;

   /// Set lattice to visualize (copy; thread-safe from UI thread).
   void set_lattice(const Lattice& lat);

   /// Set particle system to visualize (stores shared_ptr, thread-safe if system is not modified during render).
   void set_particle_system(std::shared_ptr<const ParticleSystem> system);

   /// Clear particle system reference.
   void clear_particle_system();

   /// Optional: set scale for view (e.g. from config).
   void set_scale(Real scale);

   /// Set particle display radius (in GL units).
   void set_particle_radius(float radius);

   /// Enable/disable particle rendering.
   void set_show_particles(bool show);

   /// Enable/disable lattice cell rendering.
   void set_show_lattice(bool show);

   /// Set a 2D temperature field for heatmap visualization.
   /// T: row-major temperature array (size nx*ny), T_cold/T_hot: fixed colormap bounds.
   void set_temperature_field(const std::vector<Real>& T,
                              std::size_t nx, std::size_t ny,
                              Real T_cold, Real T_hot);

   /// Clear the temperature field (switch back to particle/lattice view).
   void clear_temperature_field();

   /// Update simulation state used for live 3D feedback while running.
   void set_simulation_state(bool running, Real time_s, Real end_time_s, std::size_t step_count);

   /// Reset camera orientation/zoom.
   void reset_view();

  protected:
   void initializeGL() override;
   void resizeGL(int w, int h) override;
   void paintGL() override;
   void mousePressEvent(QMouseEvent* event) override;
   void mouseMoveEvent(QMouseEvent* event) override;
   void wheelEvent(QWheelEvent* event) override;

  private:
   void draw_axes();
   void draw_lattice_cell();
   void draw_particles();
   void draw_particle_sphere(const float pos[3], float radius, const float color[3]);
   void draw_temperature_field();
   static void colormap_thermal(float t, float& r, float& g, float& b);

   Lattice lattice_;
   std::shared_ptr<const ParticleSystem> particle_system_;
   Real scale_{1.0};   // display scale: lattice (m) * scale_ = GL coords
   float particle_radius_{0.08f};
   bool show_particles_{true};
   bool show_lattice_{true};
   bool sim_running_{false};
   Real sim_time_{0.0};
   Real sim_end_time_{0.0};
   std::size_t sim_step_count_{0};
   // Temperature field (2D heatmap)
   std::vector<Real> temp_field_;
   std::size_t temp_nx_{0};
   std::size_t temp_ny_{0};
   Real temp_T_cold_{0.0};
   Real temp_T_hot_{1.0};

   float rot_x_{0.f};
   float rot_y_{0.f};
   QPoint last_pos_;
};

}  // namespace matsimu
