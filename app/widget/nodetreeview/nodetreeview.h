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

  bool IsInputEnabled(NodeInput* i, int element, int track) const;

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

  void InputEnableChanged(NodeInput* i, int element, int track, bool e);

  void InputSelectionChanged(NodeInput* input, int element, int track);

  void InputDoubleClicked(NodeInput* input, int element, int track);

protected:
  virtual void changeEvent(QEvent* e) override;

  virtual void mouseDoubleClickEvent(QMouseEvent* e) override;

private:
  void Retranslate();

  NodeInput::KeyframeTrackReference GetSelectedInput();

  QTreeWidgetItem *CreateItem(QTreeWidgetItem* parent, NodeInput* input, int element, int track);

  enum ItemType {
    kItemTypeNode,
    kItemTypeInput
  };

  static const int kItemType = Qt::UserRole;
  static const int kItemPointer = Qt::UserRole + 1;
  static const int kItemElement = Qt::UserRole + 2;
  static const int kItemTrack = Qt::UserRole + 3;

  QVector<Node*> nodes_;

  QVector<Node*> disabled_nodes_;

  QVector<NodeInput::KeyframeTrackReference> disabled_inputs_;

  bool only_show_keyframable_;

  bool show_keyframe_tracks_as_rows_;

private slots:
  void ItemCheckStateChanged(QTreeWidgetItem* item, int column);

  void SelectionChanged();

};

}

#endif // NODETREEVIEW_H
