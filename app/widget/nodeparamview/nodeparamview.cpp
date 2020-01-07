/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "nodeparamview.h"

#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>

#include "common/timecodefunctions.h"
#include "node/output/viewer/viewer.h"

NodeParamView::NodeParamView(QWidget *parent) :
  QWidget(parent),
  last_scroll_val_(0)
{
  // Create horizontal layout to place scroll area in (and keyframe editing eventually)
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  QSplitter* splitter = new QSplitter(Qt::Horizontal);
  layout->addWidget(splitter);

  // Set up scroll area for params
  QScrollArea* scroll_area = new QScrollArea();
  scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll_area->setWidgetResizable(true);
  splitter->addWidget(scroll_area);

  // Param widget
  QWidget* param_widget_area = new QWidget();
  scroll_area->setWidget(param_widget_area);

  // Set up scroll area layout
  param_layout_ = new QVBoxLayout(param_widget_area);
  param_layout_->setSpacing(0);
  param_layout_->setMargin(0);

  // Add a stretch to allow empty space at the bottom of the layout
  param_layout_->addStretch();

  // Set up keyframe view
  QWidget* keyframe_area = new QWidget();
  QVBoxLayout* keyframe_area_layout = new QVBoxLayout(keyframe_area);
  keyframe_area_layout->setSpacing(0);
  keyframe_area_layout->setMargin(0);

  // Create ruler object
  ruler_ = new TimeRuler(true);
  keyframe_area_layout->addWidget(ruler_);

  // Create keyframe view
  keyframe_view_ = new KeyframeView();
  keyframe_view_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  bottom_item_ = keyframe_view_->scene()->addRect(0, 0, 1, 1);
  keyframe_area_layout->addWidget(keyframe_view_);

  // Connect ruler and keyframe view together
  connect(ruler_, &TimeRuler::TimeChanged, keyframe_view_, &KeyframeView::SetTime);
  connect(keyframe_view_, &KeyframeView::TimeChanged, ruler_, &TimeRuler::SetTime);
  connect(ruler_, &TimeRuler::TimeChanged, this, &NodeParamView::RulerTimeChanged);
  connect(keyframe_view_, &KeyframeView::TimeChanged, this, &NodeParamView::RulerTimeChanged);

  // Connect keyframe view scaling to this
  connect(keyframe_view_, &KeyframeView::ScaleChanged, this, &NodeParamView::SetScale);

  splitter->addWidget(keyframe_area);

  // Set both widgets to 50/50
  splitter->setSizes({INT_MAX, INT_MAX});

  // Disable collapsing param view (but collapsing keyframe view is permitted)
  splitter->setCollapsible(0, false);

  // Create global vertical scrollbar on the right
  vertical_scrollbar_ = new QScrollBar();
  vertical_scrollbar_->setMaximum(0);
  layout->addWidget(vertical_scrollbar_);

  // Connect scrollbars together
  connect(scroll_area->verticalScrollBar(), &QScrollBar::rangeChanged, vertical_scrollbar_, &QScrollBar::setRange);
  connect(scroll_area->verticalScrollBar(), &QScrollBar::rangeChanged, this, &NodeParamView::ForceKeyframeViewToScroll);

  connect(keyframe_view_->verticalScrollBar(), &QScrollBar::valueChanged, vertical_scrollbar_, &QScrollBar::setValue);
  connect(keyframe_view_->verticalScrollBar(), &QScrollBar::valueChanged, scroll_area->verticalScrollBar(), &QScrollBar::setValue);
  connect(scroll_area->verticalScrollBar(), &QScrollBar::valueChanged, vertical_scrollbar_, &QScrollBar::setValue);
  connect(scroll_area->verticalScrollBar(), &QScrollBar::valueChanged, keyframe_view_->verticalScrollBar(), &QScrollBar::setValue);
  connect(vertical_scrollbar_, &QScrollBar::valueChanged, scroll_area->verticalScrollBar(), &QScrollBar::setValue);
  connect(vertical_scrollbar_, &QScrollBar::valueChanged, keyframe_view_->verticalScrollBar(), &QScrollBar::setValue);

  connect(keyframe_view_->horizontalScrollBar(), SIGNAL(valueChanged(int)), ruler_, SLOT(SetScroll(int)));

  // Set a default scale - FIXME: Hardcoded
  SetScale(120);
}

void NodeParamView::SetNodes(QList<Node *> nodes)
{
  // If we already have item widgets, delete them all now
  foreach (NodeParamViewItem* item, items_) {
    delete item;
  }
  items_.clear();

  // Reset keyframe view
  SetTimebase(rational());
  keyframe_view_->Clear();

  // Set the internal list to the one we've received
  nodes_ = nodes;

  // For each node, create a widget
  foreach (Node* node, nodes_) {
    NodeParamViewItem* item = new NodeParamViewItem(node);

    // Insert the widget before the stretch
    param_layout_->insertWidget(param_layout_->count() - 1, item);

    connect(item, &NodeParamViewItem::KeyframeAdded, keyframe_view_, &KeyframeView::AddKeyframe);
    connect(item, &NodeParamViewItem::KeyframeRemoved, keyframe_view_, &KeyframeView::RemoveKeyframe);
    connect(item, &NodeParamViewItem::RequestSetTime, this, &NodeParamView::ItemRequestedTimeChanged);
    connect(item, &NodeParamViewItem::InputClicked, this, &NodeParamView::SelectedInputChanged);

    items_.append(item);

    QTimer::singleShot(1, item, &NodeParamViewItem::SignalAllKeyframes);
  }

  if (!nodes_.isEmpty()) {
    const ViewerOutput* viewer = nodes_.first()->FindOutputNode<ViewerOutput>();

    if (viewer) {
      SetTimebase(viewer->video_params().time_base());
    }
  }

  SetTime(0);

  // FIXME: Test code only!
  if (nodes_.isEmpty()) {
    emit SelectedInputChanged(nullptr);
  } else {
    emit SelectedInputChanged(static_cast<NodeInput*>(nodes_.first()->parameters().first()));
  }
}

void NodeParamView::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);

  vertical_scrollbar_->setPageStep(vertical_scrollbar_->height());
}

const QList<Node *> &NodeParamView::nodes()
{
  return nodes_;
}

const double &NodeParamView::GetScale() const
{
  return ruler_->scale();
}

void NodeParamView::SetScale(double scale)
{
  scale = qMin(scale, TimelineViewBase::kMaximumScale);

  ruler_->SetScale(scale);
  keyframe_view_->SetScale(scale, true);
}

void NodeParamView::SetTime(const int64_t &timestamp)
{
  ruler_->SetTime(timestamp);
  keyframe_view_->SetTime(timestamp);

  UpdateItemTime(timestamp);
}

void NodeParamView::SetTimebase(const rational &timebase)
{
  ruler_->SetTimebase(timebase);
  keyframe_view_->SetTimebase(timebase);

  emit TimebaseChanged(timebase);
}

void NodeParamView::UpdateItemTime(const int64_t &timestamp)
{
  rational time = Timecode::timestamp_to_time(timestamp, keyframe_view_->timebase());

  foreach (NodeParamViewItem* item, items_) {
    item->SetTime(time);
  }
}

void NodeParamView::RulerTimeChanged(const int64_t &timestamp)
{
  UpdateItemTime(timestamp);

  emit TimeChanged(timestamp);
}

void NodeParamView::ItemRequestedTimeChanged(const rational &time)
{
  int64_t timestamp = Timecode::time_to_timestamp(time, keyframe_view_->timebase());
  SetTime(timestamp);
  emit TimeChanged(timestamp);
}

void NodeParamView::ForceKeyframeViewToScroll(int min, int max)
{
  bottom_item_->setY(keyframe_view_->viewport()->height() + max);
}
