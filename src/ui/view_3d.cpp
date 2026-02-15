#include <matsimu/ui/view_3d.hpp>
#include <QSurfaceFormat>
#include <cmath>
#include <algorithm>
#include <limits>

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

void View3D::set_temperature_field(const std::vector<Real>& T,
                                   std::size_t nx, std::size_t ny,
                                   Real T_cold, Real T_hot) {
  if (T.size() != nx * ny || nx < 3 || ny < 3 || T_hot <= T_cold) return;
  temp_field_ = T;
  temp_nx_ = nx;
  temp_ny_ = ny;
  temp_T_cold_ = T_cold;
  temp_T_hot_ = T_hot;
  // Clear particle view when showing temperature field.
  particle_system_.reset();
  update();
}

void View3D::clear_temperature_field() {
  temp_field_.clear();
  temp_nx_ = 0;
  temp_ny_ = 0;
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

void View3D::set_simulation_state(bool running, Real time_s, Real end_time_s, std::size_t step_count) {
  sim_running_ = running;
  sim_time_ = std::isfinite(time_s) ? time_s : 0.0;
  sim_end_time_ = (std::isfinite(end_time_s) && end_time_s > 0.0) ? end_time_s : 0.0;
  sim_step_count_ = step_count;
  update();
}

void View3D::reset_view() {
  rot_x_ = 20.0f;
  rot_y_ = -30.0f;
  scale_ = 1.0;
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
  if (sim_running_ && (!particle_system_ || particle_system_->empty())) {
    // Visual cue: while running, slowly spin like a turntable.
    const float spin = static_cast<float>(std::fmod(sim_time_ * 1e15 * 8.0, 360.0));
    glRotatef(spin, 0.0f, 1.0f, 0.0f);
  }
  
  // Safe scale calculation with bounds checking
  float scale = 1.0f;
  if (std::isfinite(scale_) && scale_ > 0.0) {
    scale = static_cast<float>(scale_);
  }
  glScalef(scale, scale, scale);

  draw_axes();
  if (!temp_field_.empty() && temp_nx_ >= 3 && temp_ny_ >= 3) {
    draw_temperature_field();
  } else if (show_particles_ && particle_system_ && !particle_system_->empty()) {
    draw_particles();  // Also draws simulation box in particle coordinate space
  } else if (show_lattice_) {
    draw_lattice_cell();  // Standalone lattice cell when no particles
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
  
  // Auto-fit the full cell into view so lattice edits are always visible.
  auto norm3 = [](const float v[3]) -> float {
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  };
  
  // Compute lattice vectors in display space
  float a1_raw[3] = { static_cast<float>(lattice_.a1[0]),
                      static_cast<float>(lattice_.a1[1]),
                      static_cast<float>(lattice_.a1[2]) };
  float a2_raw[3] = { static_cast<float>(lattice_.a2[0]),
                      static_cast<float>(lattice_.a2[1]),
                      static_cast<float>(lattice_.a2[2]) };
  float a3_raw[3] = { static_cast<float>(lattice_.a3[0]),
                      static_cast<float>(lattice_.a3[1]),
                      static_cast<float>(lattice_.a3[2]) };
  
  // Define unit cell vertices
  float o[3]    = {0.0f, 0.0f, 0.0f};
  float p1_raw[3]   = {a1_raw[0], a1_raw[1], a1_raw[2]};
  float p2_raw[3]   = {a2_raw[0], a2_raw[1], a2_raw[2]};
  float p3_raw[3]   = {a3_raw[0], a3_raw[1], a3_raw[2]};
  float p12_raw[3]  = {a1_raw[0]+a2_raw[0], a1_raw[1]+a2_raw[1], a1_raw[2]+a2_raw[2]};
  float p13_raw[3]  = {a1_raw[0]+a3_raw[0], a1_raw[1]+a3_raw[1], a1_raw[2]+a3_raw[2]};
  float p23_raw[3]  = {a2_raw[0]+a3_raw[0], a2_raw[1]+a3_raw[1], a2_raw[2]+a3_raw[2]};
  float p123_raw[3] = {a1_raw[0]+a2_raw[0]+a3_raw[0], a1_raw[1]+a2_raw[1]+a3_raw[1], a1_raw[2]+a2_raw[2]+a3_raw[2]};

  float max_norm = std::max({norm3(p1_raw), norm3(p2_raw), norm3(p3_raw),
                             norm3(p12_raw), norm3(p13_raw), norm3(p23_raw), norm3(p123_raw)});
  if (!std::isfinite(max_norm) || max_norm <= 1e-12f) {
    max_norm = 1.0f;
  }
  const float fit = 1.2f / max_norm;
  const float activity_pulse = sim_running_
                                 ? (0.94f + 0.06f * std::sin(static_cast<float>(sim_time_ * 1e15 * 0.2)))
                                 : 1.0f;
  const float s = fit * activity_pulse;

  float p1[3]   = {p1_raw[0] * s, p1_raw[1] * s, p1_raw[2] * s};
  float p2[3]   = {p2_raw[0] * s, p2_raw[1] * s, p2_raw[2] * s};
  float p3[3]   = {p3_raw[0] * s, p3_raw[1] * s, p3_raw[2] * s};
  float p12[3]  = {p12_raw[0] * s, p12_raw[1] * s, p12_raw[2] * s};
  float p13[3]  = {p13_raw[0] * s, p13_raw[1] * s, p13_raw[2] * s};
  float p23[3]  = {p23_raw[0] * s, p23_raw[1] * s, p23_raw[2] * s};
  float p123[3] = {p123_raw[0] * s, p123_raw[1] * s, p123_raw[2] * s};

  // Draw unit cell edges; color warms up as simulation progresses.
  float progress = 0.0f;
  if (sim_end_time_ > 0.0 && std::isfinite(sim_time_)) {
    progress = static_cast<float>(std::min(std::max(sim_time_ / sim_end_time_, 0.0), 1.0));
  }
  const float step_glint = sim_running_ ? (0.02f * static_cast<float>(sim_step_count_ % 10)) : 0.0f;
  const float edge_r = std::min(1.0f, 0.72f + 0.20f * progress + step_glint);
  const float edge_g = 0.74f - 0.18f * progress;
  const float edge_b = 0.88f - 0.28f * progress;
  glColor3f(edge_r, edge_g, edge_b);
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

  // Compute bounding box of all finite-position particles
  float min_p[3] = { std::numeric_limits<float>::max(),
                     std::numeric_limits<float>::max(),
                     std::numeric_limits<float>::max() };
  float max_p[3] = { -std::numeric_limits<float>::max(),
                     -std::numeric_limits<float>::max(),
                     -std::numeric_limits<float>::max() };
  for (std::size_t i = 0; i < particle_system_->size(); ++i) {
    const auto& p = (*particle_system_)[i];
    if (!std::isfinite(p.pos[0]) || !std::isfinite(p.pos[1]) || !std::isfinite(p.pos[2])) continue;
    const float px = static_cast<float>(p.pos[0]);
    const float py = static_cast<float>(p.pos[1]);
    const float pz = static_cast<float>(p.pos[2]);
    min_p[0] = std::min(min_p[0], px); min_p[1] = std::min(min_p[1], py); min_p[2] = std::min(min_p[2], pz);
    max_p[0] = std::max(max_p[0], px); max_p[1] = std::max(max_p[1], py); max_p[2] = std::max(max_p[2], pz);
  }

  const float cx = 0.5f * (min_p[0] + max_p[0]);
  const float cy = 0.5f * (min_p[1] + max_p[1]);
  const float cz = 0.5f * (min_p[2] + max_p[2]);
  const float span_x = max_p[0] - min_p[0];
  const float span_y = max_p[1] - min_p[1];
  const float span_z = max_p[2] - min_p[2];
  float span = std::max({span_x, span_y, span_z});
  if (!std::isfinite(span) || span < 1e-12f) span = 1.0f;
  const float s = 1.8f / span;

  // --- Draw simulation box (lattice cell) in particle coordinate space ---
  if (show_lattice_ && std::fabs(lattice_.volume()) > 1e-30) {
    auto to_view = [cx, cy, cz, s](float x, float y, float z, float out[3]) {
      out[0] = (x - cx) * s;
      out[1] = (y - cy) * s;
      out[2] = (z - cz) * s;
    };
    const float la1[3] = { static_cast<float>(lattice_.a1[0]),
                           static_cast<float>(lattice_.a1[1]),
                           static_cast<float>(lattice_.a1[2]) };
    const float la2[3] = { static_cast<float>(lattice_.a2[0]),
                           static_cast<float>(lattice_.a2[1]),
                           static_cast<float>(lattice_.a2[2]) };
    const float la3[3] = { static_cast<float>(lattice_.a3[0]),
                           static_cast<float>(lattice_.a3[1]),
                           static_cast<float>(lattice_.a3[2]) };

    // 8 corners of the parallelepiped, transformed to particle view coordinates
    float v[8][3];
    to_view(0, 0, 0, v[0]);
    to_view(la1[0], la1[1], la1[2], v[1]);
    to_view(la2[0], la2[1], la2[2], v[2]);
    to_view(la3[0], la3[1], la3[2], v[3]);
    to_view(la1[0]+la2[0], la1[1]+la2[1], la1[2]+la2[2], v[4]);
    to_view(la1[0]+la3[0], la1[1]+la3[1], la1[2]+la3[2], v[5]);
    to_view(la2[0]+la3[0], la2[1]+la3[1], la2[2]+la3[2], v[6]);
    to_view(la1[0]+la2[0]+la3[0], la1[1]+la2[1]+la3[1], la1[2]+la2[2]+la3[2], v[7]);

    glColor3f(0.35f, 0.55f, 0.78f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    // 12 edges of parallelepiped
    // From origin
    glVertex3fv(v[0]); glVertex3fv(v[1]);
    glVertex3fv(v[0]); glVertex3fv(v[2]);
    glVertex3fv(v[0]); glVertex3fv(v[3]);
    // From a1
    glVertex3fv(v[1]); glVertex3fv(v[4]);
    glVertex3fv(v[1]); glVertex3fv(v[5]);
    // From a2
    glVertex3fv(v[2]); glVertex3fv(v[4]);
    glVertex3fv(v[2]); glVertex3fv(v[6]);
    // From a3
    glVertex3fv(v[3]); glVertex3fv(v[5]);
    glVertex3fv(v[3]); glVertex3fv(v[6]);
    // Top edges
    glVertex3fv(v[4]); glVertex3fv(v[7]);
    glVertex3fv(v[5]); glVertex3fv(v[7]);
    glVertex3fv(v[6]); glVertex3fv(v[7]);
    glEnd();
    glLineWidth(1.0f);
  }

  // --- Draw particles as color-coded points ---
  glEnable(GL_POINT_SMOOTH);
  glPointSize(std::max(5.0f, particle_radius_ * 120.0f));

  glBegin(GL_POINTS);
  for (std::size_t i = 0; i < particle_system_->size(); ++i) {
    const auto& p = (*particle_system_)[i];
    if (std::isfinite(p.pos[0]) && std::isfinite(p.pos[1]) && std::isfinite(p.pos[2])) {
      const float speed = static_cast<float>(std::sqrt(
          p.vel[0] * p.vel[0] + p.vel[1] * p.vel[1] + p.vel[2] * p.vel[2]));
      const float heat = std::min(1.0f, speed / 250.0f);
      glColor3f(0.25f + 0.75f * heat, 0.85f - 0.55f * heat, 1.0f - 0.75f * heat);
      const float px = (static_cast<float>(p.pos[0]) - cx) * s;
      const float py = (static_cast<float>(p.pos[1]) - cy) * s;
      const float pz = (static_cast<float>(p.pos[2]) - cz) * s;
      glVertex3f(px, py, pz);
    }
  }
  glEnd();
  glDisable(GL_POINT_SMOOTH);

  // --- Draw wireframe spheres for 3D depth perception ---
  const float sphere_r = std::max(0.015f, particle_radius_ * 0.35f);
  for (std::size_t i = 0; i < particle_system_->size(); ++i) {
    const auto& p = (*particle_system_)[i];
    if (std::isfinite(p.pos[0]) && std::isfinite(p.pos[1]) && std::isfinite(p.pos[2])) {
      const float speed = static_cast<float>(std::sqrt(
          p.vel[0] * p.vel[0] + p.vel[1] * p.vel[1] + p.vel[2] * p.vel[2]));
      const float heat = std::min(1.0f, speed / 250.0f);
      const float particle_color[3] = {0.25f + 0.75f * heat, 0.85f - 0.55f * heat, 1.0f - 0.75f * heat};
      float pos[3] = {
        (static_cast<float>(p.pos[0]) - cx) * s,
        (static_cast<float>(p.pos[1]) - cy) * s,
        (static_cast<float>(p.pos[2]) - cz) * s
      };
      draw_particle_sphere(pos, sphere_r, particle_color);
    }
  }
}

// ---------------------------------------------------------------------------
// Thermal colormap: dark blue-black → purple → crimson → orange → gold → bright
// Inspired by matplotlib "inferno"; 6-point piecewise-linear interpolation.
// Input t ∈ [0,1];  output r, g, b ∈ [0,1].
// ---------------------------------------------------------------------------
void View3D::colormap_thermal(float t, float& r, float& g, float& b) {
    // Clamp input.
    t = std::max(0.0f, std::min(1.0f, t));

    // Control points (t, R, G, B).
    struct CP { float t, r, g, b; };
    static constexpr CP pts[] = {
        {0.00f, 0.00f, 0.00f, 0.07f},   // near-black blue
        {0.20f, 0.27f, 0.004f, 0.43f},   // deep purple
        {0.40f, 0.65f, 0.12f, 0.42f},    // crimson-magenta
        {0.60f, 0.91f, 0.35f, 0.15f},    // warm orange
        {0.80f, 0.98f, 0.72f, 0.07f},    // golden
        {1.00f, 0.99f, 0.99f, 0.75f},    // bright yellow-white
    };
    constexpr int n = static_cast<int>(sizeof(pts) / sizeof(pts[0]));

    // Find segment.
    int seg = 0;
    for (int k = 1; k < n; ++k) {
        if (t <= pts[k].t) { seg = k - 1; break; }
        seg = k - 1;
    }
    const float seg_len = pts[seg + 1].t - pts[seg].t;
    const float frac = (seg_len > 1e-6f) ? (t - pts[seg].t) / seg_len : 0.0f;
    r = pts[seg].r + frac * (pts[seg + 1].r - pts[seg].r);
    g = pts[seg].g + frac * (pts[seg + 1].g - pts[seg].g);
    b = pts[seg].b + frac * (pts[seg + 1].b - pts[seg].b);
}

// ---------------------------------------------------------------------------
// draw_temperature_field: renders the 2D temperature grid as smooth quads.
//
// Uses bilinear vertex interpolation for smooth gradients:
//  - Each vertex (vi,vj) at a cell corner gets the average temperature of
//    the (up to 4) adjacent cells.
//  - OpenGL Gouraud shading interpolates within each quad.
// ---------------------------------------------------------------------------
void View3D::draw_temperature_field() {
    if (temp_field_.empty() || temp_nx_ < 3 || temp_ny_ < 3) return;

    const std::size_t nx = temp_nx_;
    const std::size_t ny = temp_ny_;
    const float inv_range = (temp_T_hot_ > temp_T_cold_)
                            ? 1.0f / static_cast<float>(temp_T_hot_ - temp_T_cold_)
                            : 1.0f;
    const float T_cold_f = static_cast<float>(temp_T_cold_);

    // Helper: cell temperature (clamped indices).
    auto cell_T = [&](int ci, int cj) -> float {
        ci = std::max(0, std::min(ci, static_cast<int>(nx) - 1));
        cj = std::max(0, std::min(cj, static_cast<int>(ny) - 1));
        return static_cast<float>(temp_field_[static_cast<std::size_t>(cj) * nx
                                              + static_cast<std::size_t>(ci)]);
    };

    // Vertex temperature: average of up to 4 adjacent cells.
    // Vertex (vi,vj) sits at the corner shared by cells (vi-1,vj-1), (vi,vj-1),
    //                                                   (vi-1,vj),   (vi,vj).
    auto vertex_T = [&](int vi, int vj) -> float {
        // Determine which cells exist around this vertex.
        const int ci_lo = vi - 1;
        const int ci_hi = vi;
        const int cj_lo = vj - 1;
        const int cj_hi = vj;
        float sum = 0.0f;
        int count = 0;
        if (ci_lo >= 0 && cj_lo >= 0)                                   { sum += cell_T(ci_lo, cj_lo); ++count; }
        if (ci_hi < static_cast<int>(nx) && cj_lo >= 0)                 { sum += cell_T(ci_hi, cj_lo); ++count; }
        if (ci_lo >= 0 && cj_hi < static_cast<int>(ny))                 { sum += cell_T(ci_lo, cj_hi); ++count; }
        if (ci_hi < static_cast<int>(nx) && cj_hi < static_cast<int>(ny)) { sum += cell_T(ci_hi, cj_hi); ++count; }
        return (count > 0) ? sum / static_cast<float>(count) : T_cold_f;
    };

    // Map grid to GL coordinates: [-1.0, 1.0] × [-1.0, 1.0] at z = 0.
    const float cell_w = 2.0f / static_cast<float>(nx);
    const float cell_h = 2.0f / static_cast<float>(ny);

    glShadeModel(GL_SMOOTH);

    // Draw row-by-row using GL_QUAD_STRIP for efficiency.
    for (std::size_t j = 0; j < ny; ++j) {
        glBegin(GL_QUAD_STRIP);
        for (std::size_t i = 0; i <= nx; ++i) {
            const int vi = static_cast<int>(i);
            const float x = -1.0f + static_cast<float>(i) * cell_w;

            // Bottom vertex of this row's strip (vertex row j).
            {
                const float y_bot = -1.0f + static_cast<float>(j) * cell_h;
                const float t = (vertex_T(vi, static_cast<int>(j)) - T_cold_f) * inv_range;
                float r, g, b;
                colormap_thermal(t, r, g, b);
                glColor3f(r, g, b);
                glVertex3f(x, y_bot, 0.0f);
            }
            // Top vertex of this row's strip (vertex row j+1).
            {
                const float y_top = -1.0f + static_cast<float>(j + 1) * cell_h;
                const float t = (vertex_T(vi, static_cast<int>(j) + 1) - T_cold_f) * inv_range;
                float r, g, b;
                colormap_thermal(t, r, g, b);
                glColor3f(r, g, b);
                glVertex3f(x, y_top, 0.0f);
            }
        }
        glEnd();
    }

    glShadeModel(GL_FLAT);
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
