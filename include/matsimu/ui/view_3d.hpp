#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <matsimu/core/types.hpp>
#include <matsimu/lattice/lattice.hpp>
#include <matsimu/physics/particle.hpp>
#include <memory>

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

   Lattice lattice_;
   std::shared_ptr<const ParticleSystem> particle_system_;
   Real scale_{1.0};   // display scale: lattice (m) * scale_ = GL coords
   float particle_radius_{0.05f};
   bool show_particles_{true};
   bool show_lattice_{true};
   float rot_x_{0.f};
   float rot_y_{0.f};
   QPoint last_pos_;
};

}  // namespace matsimu
