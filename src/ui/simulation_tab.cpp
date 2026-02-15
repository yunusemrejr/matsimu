#include <matsimu/ui/simulation_tab.hpp>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>

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
  if (btn_example_) btn_example_->setEnabled(!running);
  if (combo_examples_) combo_examples_->setEnabled(!running);
}

void SimulationTab::set_time(Real t) {
  if (label_time_)
    label_time_->setText(tr("Time: %1 s").arg(t, 0, 'e', 4));
}

void SimulationTab::set_params(const SimulationParams& p) {
  if (spin_dx_) spin_dx_->setValue(p.dx);
  if (spin_dt_) spin_dt_->setValue(p.dt);
  if (spin_end_time_) spin_end_time_->setValue(p.end_time);
}

void SimulationTab::build_ui() {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(20, 20, 20, 20);
  layout->setSpacing(14);

  auto* intro = new QLabel(tr("Set how long the simulation runs and how often we update (time step). "
                              "Think of it like a flipbook: Δt is how far time jumps per page, and end time is when the book stops. "
                              "All values are in SI units (metres, seconds)."), this);
  intro->setWordWrap(true);
  intro->setStyleSheet("color: #d3deea; font-size: 14px; line-height: 1.3;");
  layout->addWidget(intro);

  params_group_ = new QGroupBox(tr("Main parameters"), this);
  params_group_->setToolTip(tr("Main run controls: spacing, frame interval, and stop time."));
  auto* grid = new QGridLayout(params_group_);
  grid->setHorizontalSpacing(14);
  grid->setVerticalSpacing(10);

  grid->addWidget(new QLabel(tr("Grid spacing Δx (m):")), 0, 0);
  spin_dx_ = new QDoubleSpinBox(this);
  spin_dx_->setRange(1e-12, 1e-2);
  spin_dx_->setDecimals(4);
  spin_dx_->setValue(1e-9);
  spin_dx_->setSingleStep(1e-10);
  spin_dx_->setToolTip(tr("Distance between sample points. 1e-10 m is about one angstrom (atomic scale)."));
  grid->addWidget(spin_dx_, 0, 1);

  grid->addWidget(new QLabel(tr("Time step Δt (s):")), 1, 0);
  spin_dt_ = new QDoubleSpinBox(this);
  spin_dt_->setRange(1e-18, 1e-6);
  spin_dt_->setDecimals(4);
  spin_dt_->setValue(1e-15);
  spin_dt_->setSingleStep(1e-16);
  spin_dt_->setToolTip(tr("Time jump per update, like frame interval in a slow-motion video. Smaller Δt is steadier but needs more steps."));
  grid->addWidget(spin_dt_, 1, 1);

  grid->addWidget(new QLabel(tr("End time (s):")), 2, 0);
  spin_end_time_ = new QDoubleSpinBox(this);
  spin_end_time_->setRange(0, 1e6);
  spin_end_time_->setDecimals(4);
  spin_end_time_->setValue(0.0);
  spin_end_time_->setToolTip(tr("Total simulated duration. Set to 0 to run continuously until you press Stop."));
  grid->addWidget(spin_end_time_, 2, 1);

  layout->addWidget(params_group_);

  auto* examples_group = new QGroupBox(tr("One-click examples"), this);
  auto* examples_row = new QHBoxLayout(examples_group);
  combo_examples_ = new QComboBox(this);
  combo_examples_->addItem(tr("Argon Crystal Relaxation (dense)"), QString("argon_crystal"));
  combo_examples_->addItem(tr("Thermal Shock (colliding clusters)"), QString("thermal_shock"));
  combo_examples_->addItem(tr("Heat Diffusion: Hot Center (copper)"), QString("heat_hot_center"));
  combo_examples_->addItem(tr("Heat Diffusion: Quenching (steel)"), QString("heat_quench"));
  combo_examples_->setToolTip(tr("Prebuilt simulations: atomic dynamics and continuum heat diffusion."));
  btn_example_ = new QPushButton(tr("Run Example"), this);
  btn_example_->setObjectName("runButton");
  btn_example_->setToolTip(tr("Build and run the selected complex atomic example in one click."));
  connect(btn_example_, &QPushButton::clicked, this, &SimulationTab::on_example_clicked);
  examples_row->addWidget(combo_examples_, 1);
  examples_row->addWidget(btn_example_);
  layout->addWidget(examples_group);

  btn_run_ = new QPushButton(tr("Run"), this);
  btn_run_->setObjectName("runButton");
  btn_run_->setToolTip(tr("Start simulation (F5)."));
  btn_run_->setAccessibleName(tr("Run simulation"));
  connect(btn_run_, &QPushButton::clicked, this, &SimulationTab::on_run_clicked);

  btn_stop_ = new QPushButton(tr("Stop"), this);
  btn_stop_->setObjectName("stopButton");
  btn_stop_->setEnabled(false);
  btn_stop_->setToolTip(tr("Stop simulation (F6)."));
  connect(btn_stop_, &QPushButton::clicked, this, &SimulationTab::on_stop_clicked);

  btn_reset_ = new QPushButton(tr("Reset"), this);
  btn_reset_->setObjectName("resetButton");
  btn_reset_->setToolTip(tr("Reset time to zero."));
  connect(btn_reset_, &QPushButton::clicked, this, &SimulationTab::on_reset_clicked);

  auto* btn_layout = new QHBoxLayout();
  btn_layout->setSpacing(10);
  btn_layout->addWidget(btn_run_);
  btn_layout->addWidget(btn_stop_);
  btn_layout->addWidget(btn_reset_);
  btn_layout->addStretch();
  layout->addLayout(btn_layout);

  label_time_ = new QLabel(tr("Time: 0.0000e+00 s"), this);
  label_time_->setStyleSheet("font-size: 14px; font-weight: 700; color: #8ed0ff;");
  layout->addWidget(label_time_);
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

void SimulationTab::on_example_clicked() {
  if (!combo_examples_) return;
  emit example_requested(combo_examples_->currentData().toString());
}

}  // namespace matsimu
