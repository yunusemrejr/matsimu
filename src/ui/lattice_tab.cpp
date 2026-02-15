#include <matsimu/ui/lattice_tab.hpp>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QMessageBox> // For QMessageBox
#include <QSignalBlocker>

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
    const QSignalBlocker b10(a1_[0]);
    const QSignalBlocker b11(a1_[1]);
    const QSignalBlocker b12(a1_[2]);
    const QSignalBlocker b20(a2_[0]);
    const QSignalBlocker b21(a2_[1]);
    const QSignalBlocker b22(a2_[2]);
    const QSignalBlocker b30(a3_[0]);
    const QSignalBlocker b31(a3_[1]);
    const QSignalBlocker b32(a3_[2]);
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
  layout->setContentsMargins(20, 20, 20, 20);
  layout->setSpacing(14);

  auto* intro = new QLabel(tr("Define the repeating box (unit cell) for your material. "
                              "Imagine a stamp that repeats in 3D: a₁, a₂, a₃ are the three edges of that stamp. "
                              "Changing them reshapes every repeated cell in the world."), this);
  intro->setWordWrap(true);
  intro->setStyleSheet("color: #d3deea; font-size: 14px;");
  layout->addWidget(intro);

  auto* preview_hint = new QLabel(
      tr("Live geometry preview: open the 3D View tab while editing these vectors."), this);
  preview_hint->setStyleSheet("color: #8ed0ff; font-size: 13px; font-weight: 600;");
  layout->addWidget(preview_hint);

  basis_group_ = new QGroupBox(tr("Basis vectors (m)"), this);
  basis_group_->setToolTip(tr("Right-handed basis vectors. Think width/depth/height arrows for one tile. "
                              "Volume V = a₁·(a₂×a₃), used for periodic boundaries."));
  auto* grid = new QGridLayout(basis_group_);
  grid->setHorizontalSpacing(12);
  grid->setVerticalSpacing(10);

  auto mk_spin = [this](Real val) {
    auto* s = new QDoubleSpinBox(this);
    s->setRange(-1e6, 1e6);
    s->setDecimals(4);
    s->setValue(val);
    s->setSingleStep(1e-2);
    connect(s, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this]() { 
              sync_from_ui(); // This now performs validation and emits if valid
            });
    return s;
  };

  auto* lbl_x = new QLabel(tr("X"), this);
  auto* lbl_y = new QLabel(tr("Y"), this);
  auto* lbl_z = new QLabel(tr("Z"), this);
  lbl_x->setStyleSheet("font-weight:700; color:#8ed0ff;");
  lbl_y->setStyleSheet("font-weight:700; color:#8ed0ff;");
  lbl_z->setStyleSheet("font-weight:700; color:#8ed0ff;");
  grid->addWidget(lbl_x, 0, 1);
  grid->addWidget(lbl_y, 0, 2);
  grid->addWidget(lbl_z, 0, 3);

  grid->addWidget(new QLabel(tr("Lattice Vector 1")), 1, 0);
  a1_[0] = mk_spin(1);
  a1_[1] = mk_spin(0);
  a1_[2] = mk_spin(0);
  grid->addWidget(a1_[0], 1, 1);
  grid->addWidget(a1_[1], 1, 2);
  grid->addWidget(a1_[2], 1, 3);

  grid->addWidget(new QLabel(tr("Lattice Vector 2")), 2, 0);
  a2_[0] = mk_spin(0);
  a2_[1] = mk_spin(1);
  a2_[2] = mk_spin(0);
  grid->addWidget(a2_[0], 2, 1);
  grid->addWidget(a2_[1], 2, 2);
  grid->addWidget(a2_[2], 2, 3);

  grid->addWidget(new QLabel(tr("Lattice Vector 3")), 3, 0);
  a3_[0] = mk_spin(0);
  a3_[1] = mk_spin(0);
  a3_[2] = mk_spin(1);
  grid->addWidget(a3_[0], 3, 1);
  grid->addWidget(a3_[1], 3, 2);
  grid->addWidget(a3_[2], 3, 3);

  layout->addWidget(basis_group_);

  label_volume_ = new QLabel(tr("Cell volume: — m³"), this);
  label_volume_->setStyleSheet("color:#8ed0ff; font-weight:700; font-size:14px;");
  label_volume_->setToolTip(tr("Unit-cell volume: how much 3D space one repeating tile occupies."));
  layout->addWidget(label_volume_);
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
    if (label_volume_) {
      label_volume_->setText(tr("Cell volume: invalid (%1)").arg(QString::fromStdString(*validation_error)));
    }
    // Do not update internal lattice_ or emit signal while current edit is invalid.
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
       label_volume_->setText(tr("Cell volume: invalid (%1)").arg(QString::fromStdString(*validation_error)));
    } else {
       label_volume_->setText(tr("Cell volume: %1 m³ (space in one repeating tile)")
                                .arg(lattice_.volume(), 0, 'e', 4));
    }
  }
}

}  // namespace matsimu
