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

#ifndef NODETREEVIEW_H
#define NODETREEVIEW_H

#include <QTreeWidget>

#include "node/node.h"

namespace olive {

class NodeTreeView : public QTreeWidget
{
  Q_OBJECT
public:
  NodeTreeView(QWidget *parent = nullptr);

  bool IsNodeEnabled(Node* n) const;

  bool IsInputEnabled(const NodeKeyframeTrackReference& ref) const;

  void SetKeyframeTrackColor(const NodeKeyframeTrackReference& ref, const QColor& color);

  void SetOnlyShowKeyframable(bool e)
  {
    only_show_keyframable_ = e;
  }

  void SetShowKeyframeTracksAsRows(bool e)
  {
    show_keyframe_tracks_as_rows_ = e;
  }

public slots:
  void SetNodes(const QVector<Node *> &nodes);

signals:
  void NodeEnableChanged(Node* n, bool e);

  void InputEnableChanged(const NodeKeyframeTrackReference& ref, bool e);

  void InputSelectionChanged(const NodeKeyframeTrackReference& ref);

  void InputDoubleClicked(const NodeKeyframeTrackReference& ref);

protected:
  virtual void changeEvent(QEvent* e) override;

  virtual void mouseDoubleClickEvent(QMouseEvent* e) override;

private:
  void Retranslate();

  NodeKeyframeTrackReference GetSelectedInput();

  QTreeWidgetItem *CreateItem(QTreeWidgetItem* parent, const NodeKeyframeTrackReference& ref);

  void CreateItemsForTracks(QTreeWidgetItem* parent, const NodeInput& input, int track_count);

  enum ItemType {
    kItemTypeNode,
    kItemTypeInput
  };

  static const int kItemType = Qt::UserRole;
  static const int kItemInputReference = Qt::UserRole + 1;
  static const int kItemNodePointer = Qt::UserRole + 1;

  QVector<Node*> nodes_;

  QVector<Node*> disabled_nodes_;

  QVector<NodeKeyframeTrackReference> disabled_inputs_;

  QHash<NodeKeyframeTrackReference, QTreeWidgetItem*> item_map_;

  bool only_show_keyframable_;

  bool show_keyframe_tracks_as_rows_;

  QHash<NodeKeyframeTrackReference, QColor> keyframe_colors_;

private slots:
  void ItemCheckStateChanged(QTreeWidgetItem* item, int column);

  void SelectionChanged();

};

}

#endif // NODETREEVIEW_H
