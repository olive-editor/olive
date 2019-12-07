#include "trackview.h"

#include <QSplitter>
#include <QVBoxLayout>

TrackView::TrackView(QWidget *parent) :
  QWidget(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  QSplitter* splitter = new QSplitter(Qt::Vertical);
  splitter->setChildrenCollapsible(false);
  splitter->setHandleWidth(1);
  layout->addWidget(splitter);

  QWidget* test1 = new QWidget();
  test1->setStyleSheet("background: red;");
  splitter->addWidget(test1);

  QWidget* test3 = new QWidget();
  test3->setStyleSheet("background: yellow;");
  splitter->addWidget(test3);

  QWidget* test2 = new QWidget();
  test2->setStyleSheet("background: blue;");
  splitter->addWidget(test2);
}
