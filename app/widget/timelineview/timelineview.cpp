#include "timelineview.h"

TimelineView::TimelineView(QWidget *parent) :
  QGraphicsView(parent),
  sequence_(nullptr)
{
}

void TimelineView::SetSequence(Sequence *s)
{
  scene_.clear();

  sequence_ = s;

  QList<Node*> sequence_nodes = sequence_->nodes();
}
