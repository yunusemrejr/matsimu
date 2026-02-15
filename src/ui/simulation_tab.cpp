#include <matsimu/ui/simulation_tab.hpp>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>

namespace matsimu {

SimulationTab::SimulationTab(QWidget* parent) : QWidget(parent) {
  build_ui();
}

SimulationTab::~SimulationTab() = default;

SimulationParams SimulationTab::params() const {
  SimulationParams p;
  if (spin_dx_) p.dx = spin_dx_->value();
  if (spin_dt_) p.dt = spin_dt_->value();
  if (spin_end_time_) p.end_time = spin_end_time_->value();
  return p;
}

void SimulationTab::set_running(bool running) {
  running_ = running;
  if (btn_run_) btn_run_->setEnabled(!running);
  if (btn_stop_) btn_stop_->setEnabled(running);
}

void SimulationTab::set_time(Real t) {
  if (label_time_)
    label_time_->setText(tr("Time: %1 s").arg(t, 0, 'g', 4));
}

void SimulationTab::set_params(const SimulationParams& p) {
  if (spin_dx_) spin_dx_->setValue(p.dx);
  if (spin_dt_) spin_dt_->setValue(p.dt);
  if (spin_end_time_) spin_end_time_->setValue(p.end_time);
}

void SimulationTab::build_ui() {
  auto* layout = new QVBoxLayout(this);

  auto* intro = new QLabel(tr("Set how long the simulation runs and how often we update (time step). "
                              "All values are in SI units (metres, seconds)."), this);
  intro->setWordWrap(true);
  intro->setStyleSheet("color: #555;");
  layout->addWidget(intro);

  params_group_ = new QGroupBox(tr("Main parameters"), this);
  params_group_->setToolTip(tr("Grid spacing (m), time step (s), end time (s)."));
  auto* grid = new QGridLayout(params_group_);

  grid->addWidget(new QLabel(tr("Grid spacing Δx (m):")), 0, 0);
  spin_dx_ = new QDoubleSpinBox(this);
  spin_dx_->setRange(1e-12, 1e-2);
  spin_dx_->setDecimals(12);
  spin_dx_->setValue(1e-9);
  spin_dx_->setSingleStep(1e-10);
  spin_dx_->setToolTip(tr("Distance between grid points in metres. Used by heat diffusion; for MD this is optional."));
  grid->addWidget(spin_dx_, 0, 1);

  grid->addWidget(new QLabel(tr("Time step Δt (s):")), 1, 0);
  spin_dt_ = new QDoubleSpinBox(this);
  spin_dt_->setRange(1e-18, 1e-6);
  spin_dt_->setDecimals(15);
  spin_dt_->setValue(1e-15);
  spin_dt_->setSingleStep(1e-16);
  spin_dt_->setToolTip(tr("How often the simulation updates (seconds). Must stay below stability limit (e.g. for diffusion: dt ≤ dx²/(2α))."));
  grid->addWidget(spin_dt_, 1, 1);

  grid->addWidget(new QLabel(tr("End time (s):")), 2, 0);
  spin_end_time_ = new QDoubleSpinBox(this);
  spin_end_time_->setRange(0, 1e6);
  spin_end_time_->setDecimals(6);
  spin_end_time_->setValue(2e-15);
  spin_end_time_->setToolTip(tr("Simulation stops when time reaches this value (seconds)."));
  grid->addWidget(spin_end_time_, 2, 1);

  layout->addWidget(params_group_);

  btn_run_ = new QPushButton(tr("Run"), this);
  btn_run_->setToolTip(tr("Start simulation (F5)."));
  btn_run_->setAccessibleName(tr("Run simulation"));
  connect(btn_run_, &QPushButton::clicked, this, &SimulationTab::on_run_clicked);

  btn_stop_ = new QPushButton(tr("Stop"), this);
  btn_stop_->setEnabled(false);
  btn_stop_->setToolTip(tr("Stop simulation (F6)."));
  connect(btn_stop_, &QPushButton::clicked, this, &SimulationTab::on_stop_clicked);

  btn_reset_ = new QPushButton(tr("Reset"), this);
  btn_reset_->setToolTip(tr("Reset time to zero."));
  connect(btn_reset_, &QPushButton::clicked, this, &SimulationTab::on_reset_clicked);

  auto* btn_layout = new QVBoxLayout();
  btn_layout->addWidget(btn_run_);
  btn_layout->addWidget(btn_stop_);
  btn_layout->addWidget(btn_reset_);
  layout->addLayout(btn_layout);

  label_time_ = new QLabel(tr("Time: 0 s"), this);
  layout->addWidget(label_time_);

  layout->addStretch();
}

void SimulationTab::on_run_clicked() {
  emit run_requested();
}

void SimulationTab::on_stop_clicked() {
  emit stop_requested();
}

void SimulationTab::on_reset_clicked() {
  emit reset_requested();
}

}  // namespace matsimu
