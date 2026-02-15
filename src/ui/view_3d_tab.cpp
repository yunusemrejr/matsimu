#include <matsimu/ui/view_3d_tab.hpp>
#include <matsimu/ui/view_3d.hpp>
#include <QVBoxLayout>

namespace matsimu {

View3DTab::View3DTab(QWidget* parent) : QWidget(parent) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  view_ = new View3D(this);
  view_->setMinimumSize(400, 300);
  layout->addWidget(view_);
}

View3DTab::~View3DTab() = default;

void View3DTab::set_lattice(const Lattice& lat) {
  if (view_)
    view_->set_lattice(lat);
}

}  // namespace matsimu
