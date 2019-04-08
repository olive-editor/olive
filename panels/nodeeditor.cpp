#include "nodeeditor.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsProxyWidget>
#include <QDebug>

NodeEditor::NodeEditor(QWidget *parent) :
  EffectsPanel(parent),
  view_(&scene_)
{
  resize(720, 480);

  QWidget* central_widget = new QWidget();
  setWidget(central_widget);

  QVBoxLayout* layout = new QVBoxLayout(central_widget);
  layout->addWidget(&view_);

  view_.setInteractive(true);
  view_.setDragMode(QGraphicsView::RubberBandDrag);
  connect(&view_, SIGNAL(ScrollChanged(qreal, qreal)), this, SLOT(Scroll(qreal, qreal)));
}

void NodeEditor::Retranslate()
{

}

void NodeEditor::LoadEvent()
{
  for (int i=0;i<open_effects_.size();i++) {
    scene_.addWidget(open_effects_.at(i));
  }
}

void NodeEditor::Scroll(qreal x, qreal y)
{
  NodeUI* node;
  foreach (node, nodes_) {
    node->moveBy(x, y);
  }
}
