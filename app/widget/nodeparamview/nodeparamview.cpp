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
#include <QSplitter>

#include "node/output/viewer/viewer.h"

NodeParamView::NodeParamView(QWidget *parent) :
  QWidget(parent)
{
  // Create horizontal layout to place scroll area in (and keyframe editing eventually)
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  QSplitter* splitter = new QSplitter(Qt::Horizontal);
  layout->addWidget(splitter);

  // Set up scroll area for params
  QScrollArea* scroll_area = new QScrollArea();
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
  keyframe_area_layout->addWidget(keyframe_view_);

  // Connect ruler and keyframe view together
  connect(ruler_, SIGNAL(TimeChanged(const int64_t&)), keyframe_view_, SLOT(SetTime(const int64_t&)));
  connect(keyframe_view_, SIGNAL(TimeChanged(const int64_t&)), ruler_, SLOT(SetTime(const int64_t&)));

  splitter->addWidget(keyframe_area);

  // Disable collapsing param view (but collapsing keyframe view is permitted)
  splitter->setCollapsible(0, false);
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

  // Set the internal list to the one we've received
  nodes_ = nodes;

  // For each node, create a widget
  foreach (Node* node, nodes_) {

    // See if we already have an item for this node type

    bool merged_node = false;

    foreach (NodeParamViewItem* existing_item, items_) {
      if (existing_item->CanAddNode(node)) {
        existing_item->AttachNode(node);
        merged_node = true;
        break;
      }
    }

    // If we couldn't merge this node into the existing item, create a new one
    if (!merged_node) {
      NodeParamViewItem* item = new NodeParamViewItem(this);

      item->AttachNode(node);

      // Insert the widget before the stretch
      param_layout_->insertWidget(param_layout_->count() - 1, item);

      items_.append(item);
    }
  }

  if (!nodes_.isEmpty()) {
    const ViewerOutput* viewer = nodes_.first()->FindOutputNode<ViewerOutput>();

    if (viewer) {
      SetTimebase(viewer->video_params().time_base());
    }
  }
}

const QList<Node *> &NodeParamView::nodes()
{
  return nodes_;
}

const double &NodeParamView::GetScale() const
{
  return ruler_->scale();
}

void NodeParamView::SetScale(const double& scale)
{
  ruler_->SetScale(scale);
  keyframe_view_->SetScale(scale);
}

void NodeParamView::SetTimebase(const rational &timebase)
{
  ruler_->SetTimebase(timebase);
  keyframe_view_->SetTimebase(timebase);
}
