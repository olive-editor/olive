/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "effectui.h"

#include <QPoint>

#include "timeline/clip.h"
#include "ui/menuhelper.h"
#include "ui/keyframenavigator.h"
#include "ui/clickablelabel.h"
#include "ui/menu.h"
#include "panels/panels.h"

EffectUI::EffectUI(Effect* e) :
  effect_(e),
  node_parent_(nullptr)
{
  Q_ASSERT(e != nullptr);

  QString effect_name;

  // If this effect is actually a transition
  if (e->meta->type == EFFECT_TYPE_TRANSITION) {

    Transition* t = static_cast<Transition*>(e);

    // Since effects can have two clip attachments, find out which one is selected
    Clip* selected_clip = t->parent_clip;
    bool both_selected = false;

    // Check if this is a shared transition
    if (t->secondary_clip != nullptr) {

      // Check which clips are selected
      if (t->secondary_clip->IsSelected()) {

        selected_clip = t->secondary_clip;

        if (t->parent_clip->IsSelected()) {
          // Both clips are selected
          both_selected = true;
        }

      } else if (!t->parent_clip->IsSelected()) {

        // Neither are selected, but the naming scheme (no "opening" or "closing" modifier) will be the same
        both_selected = true;

      }

    }

    // See if the transition is the clip's opening or closing transition and label it accordingly
    if (both_selected) {
      effect_name = t->name;
    } else if (selected_clip->opening_transition.get() == t) {
      effect_name = tr("%1 (Opening)").arg(t->name);
    } else {
      effect_name = tr("%1 (Closing)").arg(t->name);
    }

  } else {

    // Otherwise just set the title normally
    effect_name = e->name;

  }

  SetTitle(effect_name);

  QWidget* ui = new QWidget(this);
  ui->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  SetContents(ui);

  title_bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

  SetExpanded(e->IsExpanded());
  connect(this, SIGNAL(visibleChanged(bool)), e, SLOT(SetExpanded(bool)));

  layout_ = new QGridLayout(ui);
  layout_->setSpacing(4);

  connect(title_bar,
          SIGNAL(customContextMenuRequested(const QPoint&)),
          this,
          SLOT(show_context_menu(const QPoint&)));

  widgets_.resize(e->row_count());
  keyframe_navigators_.resize(e->row_count());

  for (int i=0;i<e->row_count();i++) {
    EffectRow* row = e->row(i);

    ClickableLabel* row_label = new ClickableLabel(row->name());
    connect(row_label, SIGNAL(clicked()), row, SLOT(FocusRow()));

    labels_.append(row_label);

    layout_->addWidget(row_label, i, 0);

    widgets_[i].resize(row->FieldCount());

    QGridLayout* field_layout = new QGridLayout();
    for (int j=0;j<row->FieldCount();j++) {
      EffectField* field = row->Field(j);

      QWidget* widget = field->CreateWidget();

      widgets_[i][j] = widget;

      field_layout->addWidget(widget, 0, j);
    }
    layout_->addLayout(field_layout, i, 1);

    KeyframeNavigator* nav;

    if (row->IsKeyframable()) {

      nav = new KeyframeNavigator();

      nav->enable_keyframes(row->IsKeyframing());

      AttachKeyframeNavigationToRow(row, nav);

      layout_->addWidget(nav, i, 2);

    } else {

      nav = nullptr;

    }

    keyframe_navigators_[i] = nav;
  }

  enabled_check->setChecked(e->IsEnabled());
  connect(enabled_check, SIGNAL(toggled(bool)), e, SLOT(SetEnabled(bool)));
  connect(enabled_check, SIGNAL(toggled(bool)), e, SLOT(FieldChanged()));
}

void EffectUI::AddAdditionalEffect(Effect *e)
{
  // Ensure this is the same kind of effect and will be fully compatible
  Q_ASSERT(e->meta == effect_->meta);

  // Add multiple modifer to header label (but only once)
  if (additional_effects_.isEmpty()) {
    QString new_title = tr("%1 (multiple)").arg(Title());

    SetTitle(new_title);
  }

  // Add effect to list
  additional_effects_.append(e);

  // Attach this UI's widgets to the additional effect
  for (int i=0;i<effect_->row_count();i++) {

    EffectRow* row = effect_->row(i);

    // Attach existing keyframe navigator to this effect's row
    AttachKeyframeNavigationToRow(e->row(i), keyframe_navigators_.at(i));

    for (int j=0;j<row->FieldCount();j++) {

      // Attach existing field widget to this effect's field
      e->row(i)->Field(j)->CreateWidget(Widget(i, j));

    }

  }
}

Effect *EffectUI::GetEffect()
{
  return effect_;
}

int EffectUI::GetRowY(int row, QWidget* mapToWidget) {

  // Currently to get a Y value in the context of `mapToWidget`, we use `panel_effect_controls` as the base. Mapping
  // to global doesn't work for some reason, so this is the best reference point we have.

  QLabel* row_label = labels_.at(row);

  int mapped_coord;
  if (mapToWidget == nullptr) {
    mapped_coord = contents->pos().y();
  } else {
    // FIXME Problematic now that EffectUIs are used outside of EffectControls
    mapped_coord = mapToWidget->mapFrom(panel_effect_controls, contents->mapTo(panel_effect_controls, contents->pos())).y();
    mapped_coord -= title_bar->height();
  }

  // Get center point of label (label->rect()->center()->y() - instead of y()+height/2 - produces an inaccurate result)
  return row_label->y()
      + row_label->height() / 2
      + mapped_coord;
}

void EffectUI::UpdateFromEffect()
{
  Effect* effect = GetEffect();

  for (int j=0;j<effect->row_count();j++) {

    EffectRow* row = effect->row(j);

    for (int k=0;k<row->FieldCount();k++) {
      EffectField* field = row->Field(k);

      // Check if this UI object is attached to one effect or many
      if (additional_effects_.isEmpty()) {

        field->UpdateWidgetValue(Widget(j, k), effect->Now());

      } else {

        bool same_value = true;

        for (int i=0;i<additional_effects_.size();i++) {
          EffectField* previous_field = i > 0 ? additional_effects_.at(i-1)->row(j)->Field(k) : field;
          EffectField* additional_field = additional_effects_.at(i)->row(j)->Field(k);

          if (additional_field->GetValueAt(additional_effects_.at(i)->Now()) != previous_field->GetValueAt(additional_effects_.at(i-1)->Now())) {
            same_value = false;
            break;
          }
        }

        if (same_value) {
          field->UpdateWidgetValue(Widget(j, k), effect->Now());
        } else {
          field->UpdateWidgetValue(Widget(j, k), qSNaN());
        }

      }
    }
  }
}

bool EffectUI::IsAttachedToClip(Clip *c)
{
  if (GetEffect()->parent_clip == c) {
    return true;
  }

  for (int i=0;i<additional_effects_.size();i++) {
    if (additional_effects_.at(i)->parent_clip == c) {
      return true;
    }
  }

  return false;
}

void EffectUI::SetNodeParent(NodeUI *parent)
{
  node_parent_ = parent;
}

void EffectUI::resizeEvent(QResizeEvent *event)
{
  if (node_parent_ != nullptr) {
    node_parent_->Resize(event->size());
  }
}

bool EffectUI::event(QEvent *event)
{
  if (node_parent_ != nullptr
      && (event->type() == QEvent::MouseButtonPress
      || event->type() == QEvent::MouseButtonRelease
      || event->type() == QEvent::MouseMove
      || event->type() == QEvent::MouseButtonDblClick)
      && node_parent_->scene()->sendEvent(node_parent_, event)) {
    return true;
  }
  return CollapsibleWidget::event(event);
}

QWidget *EffectUI::Widget(int row, int field)
{
  return widgets_.at(row).at(field);
}

void EffectUI::AttachKeyframeNavigationToRow(EffectRow *row, KeyframeNavigator *nav)
{
  if (nav == nullptr) {
    return;
  }

  connect(nav, SIGNAL(goto_previous_key()), row, SLOT(GoToPreviousKeyframe()));
  connect(nav, SIGNAL(toggle_key()), row, SLOT(ToggleKeyframe()));
  connect(nav, SIGNAL(goto_next_key()), row, SLOT(GoToNextKeyframe()));
  connect(nav, SIGNAL(keyframe_enabled_changed(bool)), row, SLOT(SetKeyframingEnabled(bool)));
  connect(nav, SIGNAL(clicked()), row, SLOT(FocusRow()));
  connect(row, SIGNAL(KeyframingSetChanged(bool)), nav, SLOT(enable_keyframes(bool)));
}

void EffectUI::show_context_menu(const QPoint& pos) {
  if (effect_->meta->type == EFFECT_TYPE_EFFECT) {
    Menu menu;

    Clip* c = effect_->parent_clip;

    int index = c->IndexOfEffect(effect_);

    QAction* cut_action = menu.addAction(tr("Cu&t"));
    connect(cut_action, SIGNAL(triggered(bool)), this, SIGNAL(CutRequested()));

    QAction* copy_action = menu.addAction(tr("&Copy"));
    connect(copy_action, SIGNAL(triggered(bool)), this, SIGNAL(CopyRequested()));

    olive::MenuHelper.create_effect_paste_action(&menu);

    menu.addSeparator();

    QAction* move_up_action = nullptr;
    QAction* move_down_action = nullptr;

    if (index > 0) {
      move_up_action = menu.addAction(tr("Move &Up"), GetEffect(), SLOT(move_up()));
    }

    if (index < c->effects.size() - 1) {
      move_down_action = menu.addAction(tr("Move &Down"), GetEffect(), SLOT(move_down()));
    }

    menu.addSeparator();

    QAction* delete_action = menu.addAction(tr("D&elete"), GetEffect(), SLOT(delete_self()));

    // Loop through additional effects and link these too
    for (int i=0;i<additional_effects_.size();i++) {
      if (move_up_action != nullptr) {
        connect(move_up_action, SIGNAL(triggered(bool)), additional_effects_.at(i), SLOT(move_up()));
      }

      if (move_down_action != nullptr) {
        connect(move_down_action, SIGNAL(triggered(bool)), additional_effects_.at(i), SLOT(move_down()));
      }

      connect(delete_action, SIGNAL(triggered(bool)), additional_effects_.at(i), SLOT(delete_self()));
    }

    menu.addSeparator();

    menu.addAction(tr("Load Settings From File"), GetEffect(), SLOT(load_from_file()));

    menu.addAction(tr("Save Settings to File"), GetEffect(), SLOT(save_to_file()));

    menu.exec(title_bar->mapToGlobal(pos));
  }
}
