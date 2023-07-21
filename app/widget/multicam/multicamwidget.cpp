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

#include <QShortcut>

#include "node/nodeundo.h"
#include "timeline/timelineundosplit.h"
#include "widget/timeruler/timeruler.h"

namespace olive {

#define super TimeBasedWidget

MulticamWidget::MulticamWidget(QWidget *parent) :
  super{false, false, parent},
  node_(nullptr),
  clip_(nullptr)
{
  auto layout = new QVBoxLayout(this);

  sizer_ = new ViewerSizer(this);
  layout->addWidget(sizer_);

  display_ = new MulticamDisplay(this);
  display_->SetShowWidgetBackground(true);
  connect(display_, &ViewerDisplayWidget::DragStarted, this, &MulticamWidget::DisplayClicked);

  connect(sizer_, &ViewerSizer::RequestScale, display_, &ViewerDisplayWidget::SetMatrixZoom);
  connect(sizer_, &ViewerSizer::RequestTranslate, display_, &ViewerDisplayWidget::SetMatrixTranslate);
  connect(display_, &ViewerDisplayWidget::HandDragMoved, sizer_, &ViewerSizer::HandDragMove);
  sizer_->SetWidget(display_);

  layout->addWidget(this->ruler());
  layout->addWidget(this->scrollbar());

  for (int i=0; i<9; i++) {
    new QShortcut(QStringLiteral("Ctrl+%1").arg(QString::number(i+1)), this, this, [this, i]{Switch(i, false);});
    new QShortcut(QString::number(i+1), this, this, [this, i]{Switch(i, true);});
  }
}

void MulticamWidget::SetMulticamNodeInternal(ViewerOutput *viewer, MultiCamNode *n, ClipBlock *clip)
{
  if (GetConnectedNode() != viewer) {
    ConnectViewerNode(viewer);
  }

  if (node_ != n) {
    node_ = n;
    display_->SetMulticamNode(n);
  }

  if (clip_ != clip) {
    clip_ = clip;
  }
}

void MulticamWidget::SetMulticamNode(ViewerOutput *viewer, MultiCamNode *n, ClipBlock *clip, const rational &time)
{
  if (time.isNaN() || !GetConnectedNode() || time == GetConnectedNode()->GetPlayhead()) {
    SetMulticamNodeInternal(viewer, n, clip);
    play_queue_.clear();
  } else {
    MulticamNodeQueue m = {time, viewer, n, clip};
    play_queue_.push_back(m);
  }
}

void MulticamWidget::ConnectNodeEvent(ViewerOutput *n)
{
  connect(n, &ViewerOutput::SizeChanged, sizer_, &ViewerSizer::SetChildSize);
  connect(n, &ViewerOutput::PixelAspectChanged, sizer_, &ViewerSizer::SetPixelAspectRatio);

  VideoParams vp = n->GetVideoParams();
  sizer_->SetChildSize(vp.width(), vp.height());
  sizer_->SetPixelAspectRatio(vp.pixel_aspect_ratio());
}

void MulticamWidget::DisconnectNodeEvent(ViewerOutput *n)
{
  disconnect(n, &ViewerOutput::SizeChanged, sizer_, &ViewerSizer::SetChildSize);
  disconnect(n, &ViewerOutput::PixelAspectChanged, sizer_, &ViewerSizer::SetPixelAspectRatio);
}

void MulticamWidget::TimeChangedEvent(const rational &t)
{
  super::TimeChangedEvent(t);

  if (!play_queue_.empty()) {
    const MulticamNodeQueue &m = play_queue_.front();
    if (m.time >= t) {
      SetMulticamNodeInternal(m.viewer, m.node, m.clip);
      play_queue_.pop_front();
    }
  }
}

void MulticamWidget::Switch(int source, bool split_clip)
{
  if (!node_) {
    return;
  }

  MultiUndoCommand *command = new MultiUndoCommand();

  MultiCamNode *cam = node_;
  ClipBlock *clip = clip_;

  BlockSplitPreservingLinksCommand *split = nullptr;

  if (clip_ && split_clip && clip_->in() < GetConnectedNode()->GetPlayhead() && clip_->out() > GetConnectedNode()->GetPlayhead()) {
    QVector<Block*> blocks;

    blocks.append(clip_);
    blocks.append(clip_->block_links());

    split = new BlockSplitPreservingLinksCommand(blocks, {GetConnectedNode()->GetPlayhead()});
    split->redo_now();
    command->add_child(split);

    clip = static_cast<ClipBlock*>(split->GetSplit(clip_, 0));

    cam = clip->FindMulticam();
  }

  command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(NodeInput(cam, cam->kCurrentInput)), source));

  for (Block *link : clip->block_links()) {
    if (ClipBlock *clink = dynamic_cast<ClipBlock*>(link)) {
      if (MultiCamNode *mlink = clink->FindMulticam()) {
        command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(NodeInput(mlink, mlink->kCurrentInput)), source));
      }
    }
  }

  Core::instance()->undo_stack()->push(command, tr("Switched Multi-Camera Source"));

  display_->update();

  emit Switched();
}

void MulticamWidget::DisplayClicked(const QPoint &p)
{
  if (!node_) {
    return;
  }

  QPointF click = display_->ScreenToScenePoint(p);
  int width = display_->GetVideoParams().width();
  int height = display_->GetVideoParams().height();

  if (click.x() < 0 || click.y() < 0 || click.x() >= width || click.y() >= height) {
    return;
  }

  int rows, cols;
  node_->GetRowsAndColumns(&rows, &cols);

  int multi = std::max(cols, rows);

  int c = click.x() / (width/multi);
  int r = click.y() / (height/multi);

  int source = node_->RowsColsToIndex(r, c, rows, cols);

  Switch(source, true);
}

}
