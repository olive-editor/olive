#include "nodeviewtoolbar.h"

#include <QEvent>
#include <QHBoxLayout>

namespace olive {

#define super QWidget

NodeViewToolBar::NodeViewToolBar(QWidget *parent) :
  QWidget(parent)
{
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setMargin(0);

  minimap_btn_ = new QPushButton();
  minimap_btn_->setCheckable(true);
  connect(minimap_btn_, &QPushButton::clicked, this, &NodeViewToolBar::MiniMapEnabledToggled);
  layout->addWidget(minimap_btn_);

  layout->addStretch();

  Retranslate();
  UpdateIcons();
}

void NodeViewToolBar::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  } else if (e->type() == QEvent::StyleChange) {
    UpdateIcons();
  }
  super::changeEvent(e);
}

void NodeViewToolBar::Retranslate()
{
  minimap_btn_->setText(tr("Mini-Map"));
  minimap_btn_->setToolTip(tr("Toggle Mini-Map"));
}

void NodeViewToolBar::UpdateIcons()
{
}

}
