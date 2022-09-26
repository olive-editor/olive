#include "multicamwidget.h"
#include "widget/nodeparamview/nodeparamviewundo.h"

namespace olive {

#define super ViewerWidget

MulticamWidget::MulticamWidget(QWidget *parent) :
  super{parent},
  node_(nullptr)
{
  auto_cacher()->SetMulticamMode(true);

  connect(display_widget(), &ViewerDisplayWidget::DragStarted, this, &MulticamWidget::DisplayClicked);
}

RenderTicketPtr MulticamWidget::GetSingleFrame(const rational &t, bool dry)
{
  if (node_) {
    return auto_cacher()->GetSingleFrame(node_, t, dry);
  } else {
    return super::GetSingleFrame(t, dry);
  }
}

void MulticamWidget::DisplayClicked(const QPoint &p)
{
  if (!node_) {
    return;
  }

  QPointF click = display_widget()->ScreenToScenePoint(p);
  int width = display_widget()->GetVideoParams().width();
  int height = display_widget()->GetVideoParams().height();

  if (click.x() < 0 || click.y() < 0 || click.x() >= width || click.y() >= height) {
    return;
  }

  int rows, cols;
  node_->GetRowsAndColumns(&rows, &cols);

  int multi = std::max(cols, rows);

  int c = click.x() / (width/multi);
  int r = click.y() / (height/multi);

  MultiUndoCommand *command = new MultiUndoCommand();
  command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(NodeInput(node_, node_->kCurrentInput)), node_->RowsColsToIndex(r, c, rows, cols)));
  Core::instance()->undo_stack()->push(command);
}

}
