#include "keyframeview.h"

#include <QVBoxLayout>

#include "core.h"
#include "keyframeviewundo.h"
#include "widget/menu/menu.h"
#include "widget/menu/menushared.h"

KeyframeView::KeyframeView(QWidget *parent) :
  TimelineViewBase(parent)
{
  setBackgroundRole(QPalette::Base);
  setAlignment(Qt::AlignLeft | Qt::AlignTop);
  setDragMode(NoDrag);
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, &KeyframeView::customContextMenuRequested, this, &KeyframeView::ShowContextMenu);
}

void KeyframeView::Clear()
{
  QMap<NodeKeyframe*, KeyframeViewItem*>::iterator iterator;

  for (iterator=item_map_.begin();iterator!=item_map_.end();iterator++) {
    delete iterator.value();
  }

  item_map_.clear();
}

void KeyframeView::AddKeyframe(NodeKeyframePtr key, int y)
{
  QPoint global_pt(0, y);
  QPoint local_pt = mapFromGlobal(global_pt);
  QPointF scene_pt = mapToScene(local_pt);

  KeyframeViewItem* item = new KeyframeViewItem(key, scene_pt.y());
  item->SetScale(scale_);
  item_map_.insert(key.get(), item);
  scene()->addItem(item);
}

void KeyframeView::RemoveKeyframe(NodeKeyframePtr key)
{
  delete item_map_.value(key.get());
}

void KeyframeView::mousePressEvent(QMouseEvent *event)
{
  if (PlayheadPress(event)) {
    return;
  }

  QGraphicsView::mousePressEvent(event);
}

void KeyframeView::mouseMoveEvent(QMouseEvent *event)
{
  if (PlayheadMove(event)) {
    return;
  }

  QGraphicsView::mouseMoveEvent(event);
}

void KeyframeView::mouseReleaseEvent(QMouseEvent *event)
{
  if (PlayheadRelease(event)) {
    return;
  }

  QGraphicsView::mouseReleaseEvent(event);
}

void KeyframeView::ScaleChangedEvent(double scale)
{
  QMap<NodeKeyframe*, KeyframeViewItem*>::const_iterator iterator;

  for (iterator=item_map_.begin();iterator!=item_map_.end();iterator++) {
    iterator.value()->SetScale(scale);
  }
}

void KeyframeView::ShowContextMenu()
{
  Menu m;

  MenuShared::instance()->AddItemsForEditMenu(&m);

  QAction* linear_key_action;
  QAction* bezier_key_action;
  QAction* hold_key_action;

  QList<QGraphicsItem*> items = scene()->selectedItems();
  if (!items.isEmpty()) {
    bool all_keys_are_same_type = true;
    NodeKeyframe::Type type = static_cast<KeyframeViewItem*>(items.first())->key()->type();

    for (int i=1;i<items.size();i++) {
      KeyframeViewItem* key_item = static_cast<KeyframeViewItem*>(items.at(i));
      KeyframeViewItem* prev_item = static_cast<KeyframeViewItem*>(items.at(i-1));

      if (key_item->key()->type() != prev_item->key()->type()) {
        all_keys_are_same_type = false;
        break;
      }
    }

    m.addSeparator();

    linear_key_action = m.addAction(tr("Linear"));
    bezier_key_action = m.addAction(tr("Bezier"));
    hold_key_action = m.addAction(tr("Hold"));

    if (all_keys_are_same_type) {
      switch (type) {
      case NodeKeyframe::kLinear:
        linear_key_action->setChecked(true);
        break;
      case NodeKeyframe::kBezier:
        bezier_key_action->setChecked(true);
        break;
      case NodeKeyframe::kHold:
        hold_key_action->setChecked(true);
        break;
      }
    }
  }

  QAction* selected = m.exec(QCursor::pos());

  // Process keyframe type changes
  if (!items.isEmpty()) {
    if (selected == linear_key_action
        || selected == bezier_key_action
        || selected == hold_key_action) {
      NodeKeyframe::Type new_type;

      if (selected == hold_key_action) {
        new_type = NodeKeyframe::kHold;
      } else if (selected == bezier_key_action) {
        new_type = NodeKeyframe::kBezier;
      } else {
        new_type = NodeKeyframe::kLinear;
      }

      QUndoCommand* command = new QUndoCommand();
      foreach (QGraphicsItem* item, items) {
        new KeyframeSetTypeCommand(static_cast<KeyframeViewItem*>(item)->key(),
                                   new_type,
                                   command);
      }
      Core::instance()->undo_stack()->pushIfHasChildren(command);
    }
  }
}
