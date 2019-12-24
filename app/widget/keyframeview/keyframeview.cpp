#include "keyframeview.h"

#include <QVBoxLayout>

KeyframeView::KeyframeView(QWidget *parent) :
  QWidget(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  ruler_ = new TimeRuler();
  ruler_->SetTextVisible(true);
  layout->addWidget(ruler_);

  view_ = new QGraphicsView();
  view_->setScene(&scene_);
  view_->setBackgroundRole(QPalette::Base);
  view_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  view_->setDragMode(QGraphicsView::RubberBandDrag);
  layout->addWidget(view_);
}

void KeyframeView::SetTimebase(const rational &timebase)
{
  ruler_->SetTimebase(timebase);
}
