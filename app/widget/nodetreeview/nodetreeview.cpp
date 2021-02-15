/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "nodetreeview.h"

#include <QEvent>

namespace olive {

NodeTreeView::NodeTreeView(QWidget *parent) :
  QTreeWidget(parent),
  only_show_keyframable_(false),
  show_keyframe_tracks_as_rows_(false)
{
  connect(this, &NodeTreeView::itemChanged, this, &NodeTreeView::ItemCheckStateChanged);
  connect(this, &NodeTreeView::itemSelectionChanged, this, &NodeTreeView::SelectionChanged);

  Retranslate();
}

bool NodeTreeView::IsNodeEnabled(Node *n) const
{
  return !disabled_nodes_.contains(n);
}

bool NodeTreeView::IsInputEnabled(const NodeKeyframeTrackReference &ref) const
{
  return !disabled_inputs_.contains(ref);
}

void NodeTreeView::SetKeyframeTrackColor(const NodeKeyframeTrackReference &ref, const QColor &color)
{
  // Insert into hashmap
  keyframe_colors_.insert(ref, color);

  // If we currently have an item for this, set it
  QTreeWidgetItem* item = item_map_.value(ref);
  if (item) {
    item->setForeground(0, color);
  }
}

void NodeTreeView::SetNodes(const QVector<Node *> &nodes)
{
  nodes_ = nodes;

  this->clear();
  item_map_.clear();

  foreach (Node* n, nodes_) {
    QTreeWidgetItem* node_item = new QTreeWidgetItem();
    node_item->setText(0, n->Name());
    node_item->setCheckState(0, disabled_nodes_.contains(n) ? Qt::Unchecked : Qt::Checked);
    node_item->setData(0, kItemType, kItemTypeNode);
    node_item->setData(0, kItemNodePointer, Node::PtrToValue(n));

    foreach (const QString& input, n->inputs()) {
      if (only_show_keyframable_ && !n->IsInputKeyframable(input)) {
        continue;
      }

      QTreeWidgetItem* input_item = nullptr;

      int arr_sz = n->InputArraySize(input);
      for (int i=-1; i<arr_sz; i++) {
        NodeInput input_ref(n, input, i);
        const QVector<NodeKeyframeTrack>& key_tracks = n->GetKeyframeTracks(input_ref);

        int this_element_track;

        if (show_keyframe_tracks_as_rows_
            && (key_tracks.size() == 1 || (i == -1 && n->InputIsArray(input)))) {
          this_element_track = 0;
        } else {
          this_element_track = -1;
        }

        QTreeWidgetItem* element_item;

        if (input_item) {
          element_item = CreateItem(input_item, NodeKeyframeTrackReference(input_ref, this_element_track));
        } else {
          input_item = CreateItem(node_item, NodeKeyframeTrackReference(input_ref, this_element_track));
          element_item = input_item;
        }

        if (show_keyframe_tracks_as_rows_ && key_tracks.size() > 1 && (!n->InputIsArray(input) || i >= 0)) {
          CreateItemsForTracks(element_item, input_ref, key_tracks.size());
        }
      }

    }

    // Add at the end to prevent unnecessary signalling while we're setting these objects up
    if (node_item->childCount() > 0) {
      this->addTopLevelItem(node_item);
    } else {
      delete node_item;
    }
  }
}

void NodeTreeView::changeEvent(QEvent *e)
{
  QTreeWidget::changeEvent(e);

  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
}

void NodeTreeView::mouseDoubleClickEvent(QMouseEvent *e)
{
  QTreeWidget::mouseDoubleClickEvent(e);

  NodeKeyframeTrackReference ref = GetSelectedInput();

  if (ref.input().IsValid()) {
    emit InputDoubleClicked(ref);
  }
}

void NodeTreeView::Retranslate()
{
  setHeaderLabel(tr("Nodes"));
}

NodeKeyframeTrackReference NodeTreeView::GetSelectedInput()
{
  QList<QTreeWidgetItem*> sel = selectedItems();

  NodeKeyframeTrackReference selected_ref;

  if (!sel.isEmpty()) {
    QTreeWidgetItem* item = sel.first();

    if (item->data(0, kItemType).toInt() == kItemTypeInput) {
      selected_ref = item->data(0, kItemInputReference).value<NodeKeyframeTrackReference>();
    }
  }

  return selected_ref;
}

QTreeWidgetItem* NodeTreeView::CreateItem(QTreeWidgetItem *parent, const NodeKeyframeTrackReference& ref)
{
  QTreeWidgetItem* input_item = new QTreeWidgetItem(parent);

  QString item_name;
  if (ref.track() == -1 || NodeValue::get_number_of_keyframe_tracks(ref.input().GetDataType()) == 1) {
    item_name = ref.input().name();
  } else {
    switch (ref.track()) {
    case 0:
      item_name = tr("X");
      break;
    case 1:
      item_name = tr("Y");
      break;
    case 2:
      item_name = tr("Z");
      break;
    case 3:
      item_name = tr("W");
      break;
    default:
      item_name = QString::number(ref.track());
    }
  }
  input_item->setText(0, item_name);

  input_item->setCheckState(0, disabled_inputs_.contains(ref) ? Qt::Unchecked : Qt::Checked);
  input_item->setData(0, kItemType, kItemTypeInput);
  input_item->setData(0, kItemInputReference, QVariant::fromValue(ref));

  if (keyframe_colors_.contains(ref)) {
    input_item->setForeground(0, keyframe_colors_.value(ref));
  }

  item_map_.insert(ref, input_item);

  return input_item;
}

void NodeTreeView::CreateItemsForTracks(QTreeWidgetItem *parent, const NodeInput& input, int track_count)
{
  for (int j=0; j<track_count; j++) {
    CreateItem(parent, NodeKeyframeTrackReference(input, j));
  }
}

void NodeTreeView::ItemCheckStateChanged(QTreeWidgetItem *item, int column)
{
  Q_UNUSED(column)

  switch (item->data(0, kItemType).toInt()) {
  case kItemTypeNode:
  {
    Node* n = Node::ValueToPtr<Node>(item->data(0, kItemNodePointer));

    if (item->checkState(0) == Qt::Checked) {
      if (disabled_nodes_.contains(n)) {
        disabled_nodes_.removeOne(n);
        emit NodeEnableChanged(n, true);
      }
    } else if (!disabled_nodes_.contains(n)) {
      disabled_nodes_.append(n);
      emit NodeEnableChanged(n, false);
    }
    break;
  }
  case kItemTypeInput:
  {
    NodeKeyframeTrackReference i = item->data(0, kItemInputReference).value<NodeKeyframeTrackReference>();

    if (item->checkState(0) == Qt::Checked) {
      if (disabled_inputs_.contains(i)) {
        disabled_inputs_.removeOne(i);
        emit InputEnableChanged(i, true);
      }
    } else if (!disabled_inputs_.contains(i)) {
      disabled_inputs_.append(i);
      emit InputEnableChanged(i, false);
    }
    break;
  }
  }
}

void NodeTreeView::SelectionChanged()
{
  emit InputSelectionChanged(GetSelectedInput());
}

}
