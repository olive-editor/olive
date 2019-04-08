#include "nodeeditor.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

NodeEditor::NodeEditor(QWidget *parent) :
  Panel(parent),
  view_(&scene_)
{
  resize(720, 480);

  QWidget* central_widget = new QWidget();
  setWidget(central_widget);

  QVBoxLayout* layout = new QVBoxLayout(central_widget);
  layout->addWidget(&view_);

  view_.setInteractive(true);
  view_.setDragMode(QGraphicsView::RubberBandDrag);

  QPushButton* push_button = new QPushButton("heck!!!");
  scene_.addWidget(push_button);
}

void NodeEditor::Retranslate()
{

}
