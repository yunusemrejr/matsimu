#include <matsimu/ui/lattice_tab.hpp>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QMessageBox> // For QMessageBox

namespace matsimu {

LatticeTab::LatticeTab(QWidget* parent) : QWidget(parent) {
  build_ui();
  // Call update_volume_label initially to display volume for default lattice,
  // and it will implicitly call sync_from_ui which now includes validation.
  update_volume_label(); 
}

LatticeTab::~LatticeTab() = default;

Lattice LatticeTab::lattice() const {
  // We cannot call sync_from_ui() in a const method, so return the current internal state.
  // The internal state `lattice_` is always validated before it's updated.
  return lattice_;
}

void LatticeTab::set_lattice(const Lattice& lat) {
  // Before setting, validate the incoming lattice (e.g. from file load or reset)
  auto validation_error = lat.validate();
  if (validation_error) {
    QMessageBox::warning(this, tr("Invalid Lattice"),
                         tr("The provided lattice is invalid: %1").arg(QString::fromStdString(*validation_error)));
    return; // Do not set invalid lattice
  }

  lattice_ = lat;
  if (a1_[0]) {
    a1_[0]->setValue(lat.a1[0]);
    a1_[1]->setValue(lat.a1[1]);
    a1_[2]->setValue(lat.a1[2]);
    a2_[0]->setValue(lat.a2[0]);
    a2_[1]->setValue(lat.a2[1]);
    a2_[2]->setValue(lat.a2[2]);
    a3_[0]->setValue(lat.a3[0]);
    a3_[1]->setValue(lat.a3[1]);
    a3_[2]->setValue(lat.a3[2]);
  }
  update_volume_label();
  emit lattice_changed(lattice_); // Emit signal after setting valid lattice
}

void LatticeTab::build_ui() {
  auto* layout = new QVBoxLayout(this);

  auto* intro = new QLabel(tr("Define the repeating box (unit cell) for your material. "
                              "The three vectors a₁, a₂, a₃ span the cell; lengths in metres."), this);
  intro->setWordWrap(true);
  intro->setStyleSheet("color: #555;");
  layout->addWidget(intro);

  basis_group_ = new QGroupBox(tr("Basis vectors (m)"), this);
  basis_group_->setToolTip(tr("Right-handed basis a₁, a₂, a₃. Unit cell volume V = a₁·(a₂×a₃). Advanced: used for periodic boundaries and min-image."));
  auto* grid = new QGridLayout(basis_group_);

  auto mk_spin = [this](int row, int col, Real val) {
    auto* s = new QDoubleSpinBox(this);
    s->setRange(-1e6, 1e6);
    s->setDecimals(6);
    s->setValue(val);
    s->setSingleStep(1e-10);
    connect(s, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this]() { 
              sync_from_ui(); // This now performs validation and emits if valid
            });
    return s;
  };

  grid->addWidget(new QLabel(tr("a₁:")), 0, 0);
  a1_[0] = mk_spin(0, 1, 1);
  a1_[1] = mk_spin(0, 2, 0);
  a1_[2] = mk_spin(0, 3, 0);
  grid->addWidget(a1_[0], 0, 1);
  grid->addWidget(a1_[1], 0, 2);
  grid->addWidget(a1_[2], 0, 3);

  grid->addWidget(new QLabel(tr("a₂:")), 1, 0);
  a2_[0] = mk_spin(1, 1, 0);
  a2_[1] = mk_spin(1, 2, 1);
  a2_[2] = mk_spin(1, 3, 0);
  grid->addWidget(a2_[0], 1, 1);
  grid->addWidget(a2_[1], 1, 2);
  grid->addWidget(a2_[2], 1, 3);

  grid->addWidget(new QLabel(tr("a₃:")), 2, 0);
  a3_[0] = mk_spin(2, 1, 0);
  a3_[1] = mk_spin(2, 2, 0);
  a3_[2] = mk_spin(2, 3, 1);
  grid->addWidget(a3_[0], 2, 1);
  grid->addWidget(a3_[1], 2, 2);
  grid->addWidget(a3_[2], 2, 3);

  layout->addWidget(basis_group_);

  label_volume_ = new QLabel(tr("Volume: — m³"), this);
  label_volume_->setToolTip(tr("Unit cell volume in m³."));
  layout->addWidget(label_volume_);

  layout->addStretch();
}

void LatticeTab::assign_spinbox_values_to_vector(Real target_array[3], QDoubleSpinBox* spinboxes[3]) {
  target_array[0] = spinboxes[0]->value();
  target_array[1] = spinboxes[1]->value();
  target_array[2] = spinboxes[2]->value();
}

void LatticeTab::sync_from_ui() {
  if (!a1_[0]) return;
  Lattice current_input_lattice; // Create a temporary lattice from UI values

  assign_spinbox_values_to_vector(current_input_lattice.a1, a1_);
  assign_spinbox_values_to_vector(current_input_lattice.a2, a2_);
  assign_spinbox_values_to_vector(current_input_lattice.a3, a3_);

  auto validation_error = current_input_lattice.validate();
  if (validation_error) {
    QMessageBox::warning(this, tr("Invalid Lattice Input"),
                         tr("Please correct the lattice vectors: %1").arg(QString::fromStdString(*validation_error)));
    // Do not update internal lattice_ or emit signal
    // The UI still shows the invalid input, but internal state and 3D view are not updated.
    return;
  }
  
  // If valid, update internal lattice_ and then proceed
  lattice_ = current_input_lattice;
  update_volume_label();
  emit lattice_changed(lattice_);
}

void LatticeTab::update_volume_label() {
  // Do not call sync_from_ui() here anymore, it's called from QDoubleSpinBox::valueChanged signal.
  // This function now just updates the label based on the already validated `lattice_`.
  if (label_volume_) {
    auto validation_error = lattice_.validate();
    if (validation_error) {
       label_volume_->setText(tr("Volume: Invalid (error: %1)").arg(QString::fromStdString(*validation_error)));
    } else {
       label_volume_->setText(tr("Volume: %1 m³").arg(lattice_.volume(), 0, 'g', 6));
    }
  }
}

}  // namespace matsimu
