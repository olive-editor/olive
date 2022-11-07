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

#ifndef NODEPARAMVIEW_H
#define NODEPARAMVIEW_H

#include <QVBoxLayout>
#include <QWidget>

#include "node/node.h"
#include "node/project/serializer/serializer.h"
#include "nodeparamviewcontext.h"
#include "nodeparamviewdockarea.h"
#include "nodeparamviewitem.h"
#include "widget/keyframeview/keyframeview.h"
#include "widget/timebased/timebasedwidget.h"

namespace olive {

class NodeParamView : public TimeBasedWidget
{
  Q_OBJECT
public:
  NodeParamView(bool create_keyframe_view, QWidget* parent = nullptr);
  NodeParamView(QWidget* parent = nullptr) :
    NodeParamView(true, parent)
  {
  }

  virtual ~NodeParamView() override;

  void CloseContextsBelongingToProject(Project *p);

  ViewerOutput *GetTimeTarget() const;

  void DeleteSelected();

  void SelectAll()
  {
    keyframe_view_->SelectAll();
  }

  void DeselectAll()
  {
    keyframe_view_->DeselectAll();
  }

  void SetSelectedNodes(const QVector<NodeParamViewItem *> &nodes, bool handle_focused_node = true, bool emit_signal = true);
  void SetSelectedNodes(const QVector<Node::ContextPair> &nodes, bool emit_signal = true);

  Node *GetNodeWithID(const QString &id);
  Node *GetNodeWithIDAndIgnoreList(const QString &id, const QVector<Node*> &ignore);

  const QVector<Node*> &GetContexts() const
  {
    return contexts_;
  }

  virtual bool CopySelected(bool cut) override;

  virtual bool Paste() override;
  static bool Paste(QWidget *parent, std::function<QHash<Node *, Node*>(const ProjectSerializer::Result &)> get_existing_map_function);

public slots:
  void SetContexts(const QVector<Node*> &contexts);

  void UpdateElementY();

signals:
  void FocusedNodeChanged(Node* n);

  void SelectedNodesChanged(const QVector<Node::ContextPair> &nodes);

  void RequestViewerToStartEditingText();

protected:
  virtual void resizeEvent(QResizeEvent *event) override;

  virtual void ScaleChangedEvent(const double &) override;
  virtual void TimebaseChangedEvent(const rational&) override;

  virtual void ConnectedNodeChangeEvent(ViewerOutput* n) override;

  virtual const QVector<KeyframeViewInputConnection*> *GetSnapKeyframes() const override
  {
    return keyframe_view_ ? &keyframe_view_->GetKeyframeTracks() : nullptr;
  }

  virtual const std::vector<NodeKeyframe*> *GetSnapIgnoreKeyframes() const override
  {
    return keyframe_view_ ? &keyframe_view_->GetSelectedKeyframes() : nullptr;
  }

  virtual const TimeTargetObject *GetKeyframeTimeTarget() const override
  {
    return keyframe_view_;
  }

private:
  void QueueKeyframePositionUpdate();

  void AddContext(Node *context);

  void RemoveContext(Node *context);

  void AddNode(Node* n, Node *ctx, NodeParamViewContext *context);

  void SortItemsInContext(NodeParamViewContext *context);

  NodeParamViewContext *GetContextItemFromContext(Node *context);

  bool IsGroupMode() const
  {
    return contexts_.size() == 1 && dynamic_cast<NodeGroup*>(contexts_.first());
  }

  void ToggleSelect(NodeParamViewItem *item);

  QHash<Node *, Node *> GenerateExistingPasteMap(const ProjectSerializer::Result &r);

  KeyframeView* keyframe_view_;

  QVector<NodeParamViewContext*> context_items_;

  QScrollBar* vertical_scrollbar_;

  int last_scroll_val_;

  QScrollArea* param_scroll_area_;

  QWidget* param_widget_container_;

  NodeParamViewDockArea* param_widget_area_;

  QVector<Node*> pinned_nodes_;

  QVector<Node*> active_nodes_;

  NodeParamViewItem* focused_node_;
  QVector<NodeParamViewItem*> selected_nodes_;

  ViewerOutput *time_target_;

  QVector<Node*> contexts_;
  QVector<Node*> current_contexts_;

  bool show_all_nodes_;

private slots:
  void UpdateGlobalScrollBar();

  void PinNode(bool pin);

  //void FocusChanged(QWidget *old, QWidget *now);

  void KeyframeViewDragged(int x, int y);
  void KeyframeViewReleased();

  void NodeAddedToContext(Node *n);

  void NodeRemovedFromContext(Node *n);

  void InputCheckBoxChanged(const NodeInput &input, bool e);

  void GroupInputPassthroughAdded(olive::NodeGroup *group, const olive::NodeInput &input);

  void GroupInputPassthroughRemoved(olive::NodeGroup *group, const olive::NodeInput &input);

  void UpdateContexts();

  void ItemAboutToBeRemoved(NodeParamViewItem *item);

  void ItemClicked();

  void SelectNodeFromConnectedLink(Node *node);

  void RequestEditTextInViewer();

};

}

#endif // NODEPARAMVIEW_H
