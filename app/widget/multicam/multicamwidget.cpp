/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "multicamwidget.h"
#include "widget/nodeparamview/nodeparamviewundo.h"
#include "widget/timelinewidget/undo/timelineundosplit.h"

namespace olive {

#define super ViewerWidget

MulticamWidget::MulticamWidget(QWidget *parent) :
  super{new MulticamDisplay(), parent},
  node_(nullptr),
  clip_(nullptr)
{
  auto_cacher()->SetMulticamMode(true);
  auto_cacher()->SetIgnoreCacheRequests(true);

  connect(display_widget(), &ViewerDisplayWidget::DragStarted, this, &MulticamWidget::DisplayClicked);
}

void MulticamWidget::SetMulticamNode(MultiCamNode *n)
{
  node_ = n;
  static_cast<MulticamDisplay*>(display_widget())->SetMulticamNode(n);
}

void MulticamWidget::SetClip(ClipBlock *clip)
{
  clip_ = clip;
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

  const bool enable_split = true;

  MultiCamNode *cam = node_;
  ClipBlock *clip = clip_;

  if (clip_ && enable_split && clip_->in() < GetTime() && clip_->out() > GetTime()) {
    QVector<Block*> blocks;

    blocks.append(clip_);
    blocks.append(clip_->block_links());

    auto split = new BlockSplitPreservingLinksCommand(blocks, {GetTime()});
    split->redo_now();
    command->add_child(split);

    clip = static_cast<ClipBlock*>(split->GetSplit(clip_, 0));

    cam = clip->FindMulticam();
  }

  command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(NodeInput(cam, cam->kCurrentInput)), cam->RowsColsToIndex(r, c, rows, cols)));
  Core::instance()->undo_stack()->push(command);

  if (cam != node_) {
    SetMulticamNode(cam);
  }
  if (clip != clip_) {
    SetClip(clip);
  }
}

}
