#include <matsimu/ui/view_3d_tab.hpp>
#include <matsimu/ui/view_3d.hpp>
#include <QLabel>
#include <QString>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <algorithm>
#include <cmath>
#include <memory>

namespace matsimu {

View3DTab::View3DTab(QWidget* parent) : QWidget(parent) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(14, 14, 14, 14);
  layout->setSpacing(10);

  auto* intro = new QLabel(
      tr("Think of this like a clear shoebox for atoms: the wireframe is one repeating tile. "
         "Drag to rotate, scroll to zoom. While simulation runs, the box animates and warms in color."),
      this);
  intro->setWordWrap(true);
  intro->setStyleSheet("color: #555;");
  layout->addWidget(intro);

  auto* status_row = new QHBoxLayout();
  state_dot_ = new QLabel("●", this);
  state_dot_->setStyleSheet("color:#f2b742;font-size:18px;font-weight:700;");
  run_info_ = new QLabel(tr("Run: idle"), this);
  run_info_->setStyleSheet("color:#dce7f3;font-size:13px;font-weight:600;");
  lattice_info_ = new QLabel(tr("Lattice: |a1|=1, |a2|=1, |a3|=1, V=1 m³"), this);
  lattice_info_->setStyleSheet("color:#b8c8d8;font-size:13px;");
  btn_reset_view_ = new QPushButton(tr("Reset Camera"), this);
  btn_reset_view_->setToolTip(tr("Center and reorient the 3D camera."));
  connect(btn_reset_view_, &QPushButton::clicked, this, [this]() {
    if (view_) view_->reset_view();
  });
  status_row->addWidget(state_dot_);
  status_row->addWidget(run_info_, 1);
  status_row->addWidget(btn_reset_view_);
  layout->addLayout(status_row);
  layout->addWidget(lattice_info_);

  view_ = new View3D(this);
  view_->setMinimumSize(700, 520);
  view_->reset_view();  // Sensible initial camera angle instead of (0,0) edge-on
  layout->addWidget(view_);
  layout->setStretch(layout->count() - 1, 1);
}

View3DTab::~View3DTab() = default;

void View3DTab::set_lattice(const Lattice& lat) {
  if (view_)
    view_->set_lattice(lat);
  if (lattice_info_) {
    const auto validation_error = lat.validate();
    if (validation_error) {
      lattice_info_->setText(tr("Lattice: invalid (%1)")
                                 .arg(QString::fromStdString(*validation_error)));
      return;
    }
    auto mag = [](const Real v[3]) -> Real {
      return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    };
    const Real a1 = mag(lat.a1);
    const Real a2 = mag(lat.a2);
    const Real a3 = mag(lat.a3);
    const Real vol = lat.volume();
    lattice_info_->setText(tr("Lattice: |a1|=%1, |a2|=%2, |a3|=%3, V=%4 m³")
                               .arg(a1, 0, 'g', 5)
                               .arg(a2, 0, 'g', 5)
                               .arg(a3, 0, 'g', 5)
                               .arg(vol, 0, 'g', 5));
  }
}

void View3DTab::set_particles(const ParticleSystem& system) {
  if (!view_) return;
  view_->set_particle_system(std::make_shared<ParticleSystem>(system));
}

void View3DTab::clear_particles() {
  if (!view_) return;
  view_->clear_particle_system();
}

void View3DTab::set_temperature_field(const std::vector<Real>& T,
                                      std::size_t nx, std::size_t ny,
                                      Real T_cold, Real T_hot) {
  if (!view_) return;
  view_->set_temperature_field(T, nx, ny, T_cold, T_hot);
}

void View3DTab::clear_temperature_field() {
  if (!view_) return;
  view_->clear_temperature_field();
}

void View3DTab::set_simulation_state(bool running, Real time_s, Real end_time_s, std::size_t step_count) {
  if (view_)
    view_->set_simulation_state(running, time_s, end_time_s, step_count);
  if (state_dot_) {
    state_dot_->setStyleSheet(running
                                  ? "color:#29d17d;font-size:18px;font-weight:700;"
                                  : "color:#f2b742;font-size:18px;font-weight:700;");
  }
  if (run_info_) {
    const Real end = (std::isfinite(end_time_s) && end_time_s > 0.0) ? end_time_s : 0.0;
    const Real t = std::isfinite(time_s) ? std::max<Real>(0.0, time_s) : 0.0;
    const Real frac = (end > 0.0) ? std::min<Real>(1.0, t / end) : 0.0;
    if (running) {
      if (end > 0.0) {
        run_info_->setText(tr("Run: active | t=%1 s | steps=%2 | %3%")
                               .arg(t, 0, 'g', 5)
                               .arg(step_count)
                               .arg(frac * 100.0, 0, 'f', 1));
      } else {
        run_info_->setText(tr("Run: active (continuous) | t=%1 s | steps=%2")
                               .arg(t, 0, 'g', 5)
                               .arg(step_count));
      }
    } else {
      run_info_->setText(tr("Run: idle | last t=%1 s | steps=%2")
                             .arg(t, 0, 'g', 5)
                             .arg(step_count));
    }
  }
}

}  // namespace matsimu
