#include <matsimu/ui/view_3d.hpp>
#include <QSurfaceFormat>
#include <cmath>
#include <algorithm>

namespace matsimu {

View3D::View3D(QWidget* parent) : QOpenGLWidget(parent) {
  QSurfaceFormat fmt;
  fmt.setVersion(2, 1);
  fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
  fmt.setDepthBufferSize(24);
  fmt.setSamples(4);  // Anti-aliasing for smoother lines
  setFormat(fmt);
  setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);  // Full updates are more stable for 3D
}

View3D::~View3D() = default;

void View3D::set_lattice(const Lattice& lat) {
  lattice_ = lat;
  update();
}

void View3D::set_particle_system(std::shared_ptr<const ParticleSystem> system) {
  particle_system_ = std::move(system);
  update();
}

void View3D::clear_particle_system() {
  particle_system_.reset();
  update();
}

void View3D::set_scale(Real scale) {
  scale_ = scale;
  update();
}

void View3D::set_particle_radius(float radius) {
  particle_radius_ = std::max(0.001f, radius);
  update();
}

void View3D::set_show_particles(bool show) {
  show_particles_ = show;
  update();
}

void View3D::set_show_lattice(bool show) {
  show_lattice_ = show;
  update();
}

void View3D::initializeGL() {
  initializeOpenGLFunctions();
  glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
}

void View3D::resizeGL(int w, int h) {
  // Ensure viewport is valid: minimum 1x1 to avoid division by zero
  int vw = std::max(1, w);
  int vh = std::max(1, h);
  glViewport(0, 0, vw, vh);
}

void View3D::paintGL() {
  // Clear buffers
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  // Validate dimensions before rendering
  int w = std::max(1, width());
  int h = std::max(1, height());
  
  // Setup depth testing
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_TRUE);
  
  // Setup projection matrix
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  
  // Safe aspect ratio calculation
  float aspect = static_cast<float>(w) / static_cast<float>(h);
  
  // Perspective projection with safeguards
  constexpr float fov_deg = 45.0f;
  constexpr float fov_rad = fov_deg * static_cast<float>(M_PI) / 180.0f;
  constexpr float near_plane = 0.1f;
  constexpr float far_plane = 100.0f;
  
  float top = std::tan(fov_rad * 0.5f) * near_plane;
  float right = aspect * top;
  
  glFrustum(-right, right, -top, top, near_plane, far_plane);

  // Setup modelview matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0f, 0.0f, -3.0f);
  glRotatef(rot_x_, 1.0f, 0.0f, 0.0f);
  glRotatef(rot_y_, 0.0f, 1.0f, 0.0f);
  
  // Safe scale calculation with bounds checking
  float scale = 1.0f;
  if (std::isfinite(scale_) && scale_ > 0.0) {
    scale = static_cast<float>(scale_);
  }
  glScalef(scale, scale, scale);

  draw_axes();
  if (show_lattice_) {
    draw_lattice_cell();
  }
  if (show_particles_ && particle_system_) {
    draw_particles();
  }
  
  // Ensure matrix mode is reset for next frame
  glMatrixMode(GL_MODELVIEW);
}

void View3D::draw_axes() {
  glBegin(GL_LINES);
  glColor3f(0.9f, 0.2f, 0.2f);
  glVertex3f(0, 0, 0);
  glVertex3f(1, 0, 0);
  glColor3f(0.2f, 0.8f, 0.2f);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 1, 0);
  glColor3f(0.2f, 0.2f, 0.9f);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 0, 1);
  glEnd();
}

void View3D::draw_lattice_cell() {
  // Validate lattice vectors are finite
  auto is_finite_vec = [](const Real v[3]) -> bool {
    return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
  };
  
  if (!is_finite_vec(lattice_.a1) || !is_finite_vec(lattice_.a2) || !is_finite_vec(lattice_.a3)) {
    return;  // Skip drawing invalid lattice
  }
  
  // Use a safe, clamped scale to prevent extreme distortion
  float s = 1.0f;
  if (std::isfinite(scale_) && scale_ > 0.0) {
    // Clamp scale to reasonable visual range
    s = static_cast<float>(std::min(std::max(scale_, 1e-6), 1e6));
  }
  
  // Compute lattice vectors in display space
  float a1[3] = { static_cast<float>(lattice_.a1[0]) * s, 
                  static_cast<float>(lattice_.a1[1]) * s, 
                  static_cast<float>(lattice_.a1[2]) * s };
  float a2[3] = { static_cast<float>(lattice_.a2[0]) * s, 
                  static_cast<float>(lattice_.a2[1]) * s, 
                  static_cast<float>(lattice_.a2[2]) * s };
  float a3[3] = { static_cast<float>(lattice_.a3[0]) * s, 
                  static_cast<float>(lattice_.a3[1]) * s, 
                  static_cast<float>(lattice_.a3[2]) * s };
  
  // Define unit cell vertices
  float o[3]    = {0.0f, 0.0f, 0.0f};
  float p1[3]   = {a1[0], a1[1], a1[2]};
  float p2[3]   = {a2[0], a2[1], a2[2]};
  float p3[3]   = {a3[0], a3[1], a3[2]};
  float p12[3]  = {a1[0]+a2[0], a1[1]+a2[1], a1[2]+a2[2]};
  float p13[3]  = {a1[0]+a3[0], a1[1]+a3[1], a1[2]+a3[2]};
  float p23[3]  = {a2[0]+a3[0], a2[1]+a3[1], a2[2]+a3[2]};
  float p123[3] = {a1[0]+a2[0]+a3[0], a1[1]+a2[1]+a3[1], a1[2]+a2[2]+a3[2]};

  // Draw unit cell edges with slightly brighter color for visibility
  glColor3f(0.75f, 0.75f, 0.85f);
  glLineWidth(1.5f);
  glBegin(GL_LINES);
  #define E(A,B) glVertex3fv(A); glVertex3fv(B);
  // Bottom face edges
  E(o, p1); E(o, p2); E(o, p3);
  // Side edges
  E(p1, p12); E(p1, p13);
  E(p2, p12); E(p2, p23);
  E(p3, p13); E(p3, p23);
  // Top face edges
  E(p12, p123); E(p13, p123); E(p23, p123);
  #undef E
  glEnd();
  glLineWidth(1.0f);  // Reset line width
}

void View3D::mousePressEvent(QMouseEvent* event) {
  if (event && event->button() == Qt::LeftButton) {
    last_pos_ = event->position().toPoint();
  }
}

void View3D::mouseMoveEvent(QMouseEvent* event) {
  if (!event) return;
  
  QPoint current_pos = event->position().toPoint();
  int dx = current_pos.x() - last_pos_.x();
  int dy = current_pos.y() - last_pos_.y();
  
  // Update rotation angles with bounds checking to prevent overflow
  rot_y_ += 0.5f * dx;
  rot_x_ += 0.5f * dy;
  
  // Normalize angles to [0, 360) range to prevent float overflow
  rot_x_ = std::fmod(rot_x_, 360.0f);
  rot_y_ = std::fmod(rot_y_, 360.0f);
  if (rot_x_ < 0) rot_x_ += 360.0f;
  if (rot_y_ < 0) rot_y_ += 360.0f;
  
  last_pos_ = current_pos;
  update();
}

void View3D::wheelEvent(QWheelEvent* event) {
  if (!event) return;
  
  int dy = event->angleDelta().y();
  if (dy == 0) return;
  
  // Zoom factor: smoother scaling
  constexpr double zoom_factor = 1.1;
  constexpr double min_scale = 1e-12;
  constexpr double max_scale = 1e12;
  
  if (dy > 0) {
    scale_ *= zoom_factor;
  } else {
    scale_ /= zoom_factor;
  }
  
  // Clamp to safe range with explicit bounds
  if (!std::isfinite(scale_) || scale_ < min_scale) {
    scale_ = min_scale;
  } else if (scale_ > max_scale) {
    scale_ = max_scale;
  }
  
  event->accept();
  update();
}

void View3D::draw_particles() {
  if (!particle_system_ || particle_system_->empty()) {
    return;
  }

  // Use a safe scale for particle positions
  float s = 1.0f;
  if (std::isfinite(scale_) && scale_ > 0.0) {
    s = static_cast<float>(std::min(std::max(scale_, 1e-6), 1e6));
  }

  // Draw particles as small spheres (approximated by points for now)
  // Use a bright color for particles
  const float particle_color[3] = {1.0f, 0.8f, 0.2f};  // Golden yellow

  // Enable point smoothing for rounder particles
  glEnable(GL_POINT_SMOOTH);
  glPointSize(std::max(2.0f, particle_radius_ * 50.0f));  // Scale point size

  glBegin(GL_POINTS);
  glColor3fv(particle_color);

  for (std::size_t i = 0; i < particle_system_->size(); ++i) {
    const auto& p = (*particle_system_)[i];
    // Check for finite positions
    if (std::isfinite(p.pos[0]) && std::isfinite(p.pos[1]) && std::isfinite(p.pos[2])) {
      float px = static_cast<float>(p.pos[0]) * s;
      float py = static_cast<float>(p.pos[1]) * s;
      float pz = static_cast<float>(p.pos[2]) * s;
      glVertex3f(px, py, pz);
    }
  }

  glEnd();
  glDisable(GL_POINT_SMOOTH);

  // For larger particle radii, draw wireframe spheres
  if (particle_radius_ > 0.02f) {
    for (std::size_t i = 0; i < particle_system_->size(); ++i) {
      const auto& p = (*particle_system_)[i];
      if (std::isfinite(p.pos[0]) && std::isfinite(p.pos[1]) && std::isfinite(p.pos[2])) {
        float pos[3] = {
          static_cast<float>(p.pos[0]) * s,
          static_cast<float>(p.pos[1]) * s,
          static_cast<float>(p.pos[2]) * s
        };
        draw_particle_sphere(pos, particle_radius_ * s, particle_color);
      }
    }
  }
}

void View3D::draw_particle_sphere(const float pos[3], float radius, const float color[3]) {
  // Simple wireframe sphere approximation using GL_LINES
  const int segments = 12;
  const float pi = 3.14159265359f;

  glColor3f(color[0] * 0.8f, color[1] * 0.8f, color[2] * 0.8f);
  glLineWidth(1.0f);

  glBegin(GL_LINE_LOOP);
  for (int i = 0; i < segments; ++i) {
    float theta = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
    float x = pos[0] + radius * std::cos(theta);
    float y = pos[1] + radius * std::sin(theta);
    glVertex3f(x, y, pos[2]);
  }
  glEnd();

  glBegin(GL_LINE_LOOP);
  for (int i = 0; i < segments; ++i) {
    float theta = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
    float x = pos[0] + radius * std::cos(theta);
    float z = pos[2] + radius * std::sin(theta);
    glVertex3f(x, pos[1], z);
  }
  glEnd();

  glBegin(GL_LINE_LOOP);
  for (int i = 0; i < segments; ++i) {
    float theta = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
    float y = pos[1] + radius * std::cos(theta);
    float z = pos[2] + radius * std::sin(theta);
    glVertex3f(pos[0], y, z);
  }
  glEnd();
}

}  // namespace matsimu
