#include "keyframeviewbase.h"

#include <QMouseEvent>
#include <QVBoxLayout>

#include "keyframeviewundo.h"
#include "widget/menu/menu.h"
#include "widget/menu/menushared.h"
#include "widget/nodeparamview/nodeparamviewundo.h"

KeyframeViewBase::KeyframeViewBase(QWidget *parent) :
  TimelineViewBase(parent)
{
  setDragMode(RubberBandDrag);
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, &KeyframeViewBase::customContextMenuRequested, this, &KeyframeViewBase::ShowContextMenu);
  connect(Core::instance(), &Core::ToolChanged, this, &KeyframeViewBase::ApplicationToolChanged);
}

void KeyframeViewBase::Clear()
{
  QMap<NodeKeyframe*, KeyframeViewItem*>::iterator iterator;

  for (iterator=item_map_.begin();iterator!=item_map_.end();iterator++) {
    delete iterator.value();
  }

  item_map_.clear();
}

void KeyframeViewBase::RemoveKeyframe(NodeKeyframePtr key)
{
  KeyframeAboutToBeRemoved(key.get());

  delete item_map_.take(key.get());
}

KeyframeViewItem *KeyframeViewBase::AddKeyframeInternal(NodeKeyframePtr key)
{
  KeyframeViewItem* item = new KeyframeViewItem(key);
  item->SetScale(scale_);
  item_map_.insert(key.get(), item);
  scene()->addItem(item);
  return item;
}

void KeyframeViewBase::mousePressEvent(QMouseEvent *event)
{
  if (PlayheadPress(event)) {
    return;
  }

  active_tool_ = Core::instance()->tool();

  rubberBandSelectionMode();

  if (event->button() == Qt::LeftButton) {
    QGraphicsView::mousePressEvent(event);

    if (active_tool_ == Tool::kPointer) {
      QGraphicsItem* item_under_cursor = itemAt(event->pos());

      if (item_under_cursor) {
        QList<QGraphicsItem*> selected_items = scene()->selectedItems();

        drag_start_ = event->pos();

        selected_keys_.resize(selected_items.size());

        for (int i=0;i<selected_items.size();i++) {
          KeyframeViewItem* key = static_cast<KeyframeViewItem*>(selected_items.at(i));

          selected_keys_.replace(i, {key, key->x(), key->key()->time()});
        }
      }
    }
  }
}

void KeyframeViewBase::mouseMoveEvent(QMouseEvent *event)
{
  if (PlayheadMove(event)) {
    return;
  }

  if (event->buttons() & Qt::LeftButton) {
    QGraphicsView::mouseMoveEvent(event);

    if (active_tool_ == Tool::kPointer && !selected_keys_.isEmpty()) {
      int x_diff = event->pos().x() - drag_start_.x();
      double x_diff_scaled = static_cast<double>(x_diff) / scale_;

      foreach (const KeyframeItemAndTime& keypair, selected_keys_) {
        // FIXME: Find some way to do single frame updates as the NodeParamViewWidgetBridge does?
        keypair.key->key()->set_time(CalculateNewTimeFromScreen(keypair.time, x_diff_scaled));
      }
    }
  }
}

void KeyframeViewBase::mouseReleaseEvent(QMouseEvent *event)
{
  if (PlayheadRelease(event)) {
    return;
  }

  if (event->button() == Qt::LeftButton) {
    QGraphicsView::mouseReleaseEvent(event);

    if (active_tool_ == Tool::kPointer && !selected_keys_.isEmpty()) {
      QUndoCommand* command = new QUndoCommand();

      // Calculate X movement and scaling to timeline time
      int x_diff = event->pos().x() - drag_start_.x();
      double x_diff_scaled = static_cast<double>(x_diff) / scale_;

      foreach (const KeyframeItemAndTime& keypair, selected_keys_) {
        KeyframeViewItem* item = keypair.key;

        // Calculate the new time for this keyframe
        rational new_time = CalculateNewTimeFromScreen(keypair.time, x_diff_scaled);

        // Commit movement
        new NodeParamSetKeyframeTimeCommand(item->key(),
                                            new_time,
                                            keypair.time,
                                            command);
      }

      Core::instance()->undo_stack()->push(command);

      selected_keys_.clear();
    }
  }
}

void KeyframeViewBase::ScaleChangedEvent(double scale)
{
  QMap<NodeKeyframe*, KeyframeViewItem*>::const_iterator iterator;

  for (iterator=item_map_.begin();iterator!=item_map_.end();iterator++) {
    iterator.value()->SetScale(scale);
  }
}

const QMap<NodeKeyframe *, KeyframeViewItem *> &KeyframeViewBase::item_map() const
{
  return item_map_;
}

void KeyframeViewBase::KeyframeAboutToBeRemoved(NodeKeyframe *)
{
}

rational KeyframeViewBase::CalculateNewTimeFromScreen(const rational &old_time, double cursor_diff)
{
  return rational::fromDouble(old_time.toDouble() + cursor_diff);
}

void KeyframeViewBase::ShowContextMenu()
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

void KeyframeViewBase::ApplicationToolChanged(Tool::Item tool)
{
  if (tool == Tool::kHand) {
    setDragMode(ScrollHandDrag);
  } else {
    setDragMode(RubberBandDrag);
  }
}

