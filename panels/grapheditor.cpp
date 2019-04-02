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

#include "grapheditor.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVariant>

#include "ui/keyframenavigator.h"
#include "ui/timelineheader.h"
#include "timeline/timelinetools.h"
#include "ui/labelslider.h"
#include "ui/graphview.h"
#include "effects/effect.h"
#include "effects/effectfields.h"
#include "effects/effectrow.h"
#include "timeline/clip.h"
#include "rendering/renderfunctions.h"
#include "panels.h"
#include "ui/icons.h"
#include "global/debug.h"

GraphEditor::GraphEditor(QWidget* parent) : Panel(parent), row(nullptr) {
  resize(720, 480);

  QWidget* main_widget = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(main_widget);
  setWidget(main_widget);

  QWidget* tool_widget = new QWidget();
  tool_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  QHBoxLayout* tools = new QHBoxLayout(tool_widget);

  QWidget* left_tool_widget = new QWidget();
  QHBoxLayout* left_tool_layout = new QHBoxLayout(left_tool_widget);
  left_tool_layout->setSpacing(0);
  left_tool_layout->setMargin(0);
  tools->addWidget(left_tool_widget);
  QWidget* center_tool_widget = new QWidget();
  QHBoxLayout* center_tool_layout = new QHBoxLayout(center_tool_widget);
  center_tool_layout->setSpacing(0);
  center_tool_layout->setMargin(0);
  tools->addWidget(center_tool_widget);
  QWidget* right_tool_widget = new QWidget();
  QHBoxLayout* right_tool_layout = new QHBoxLayout(right_tool_widget);
  right_tool_layout->setSpacing(0);
  right_tool_layout->setMargin(0);
  tools->addWidget(right_tool_widget);

  keyframe_nav = new KeyframeNavigator(nullptr, false);
  keyframe_nav->enable_keyframes(true);
  keyframe_nav->enable_keyframe_toggle(false);
  left_tool_layout->addWidget(keyframe_nav);
  left_tool_layout->addStretch();

  linear_button = new QPushButton();
  linear_button->setProperty("type", EFFECT_KEYFRAME_LINEAR);
  linear_button->setCheckable(true);
  bezier_button = new QPushButton();
  bezier_button->setProperty("type", EFFECT_KEYFRAME_BEZIER);
  bezier_button->setCheckable(true);
  hold_button = new QPushButton();
  hold_button->setProperty("type", EFFECT_KEYFRAME_HOLD);
  hold_button->setCheckable(true);

  center_tool_layout->addStretch();
  center_tool_layout->addWidget(linear_button);
  center_tool_layout->addWidget(bezier_button);
  center_tool_layout->addWidget(hold_button);

  layout->addWidget(tool_widget);

  QWidget* central_widget = new QWidget();
  QVBoxLayout* central_layout = new QVBoxLayout(central_widget);
  central_layout->setSpacing(0);
  central_layout->setMargin(0);
  header = new TimelineHeader();
  header->viewer = panel_sequence_viewer;
  central_layout->addWidget(header);
  view = new GraphView();
  central_layout->addWidget(view);

  layout->addWidget(central_widget);

  QWidget* value_widget = new QWidget();
  value_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  QHBoxLayout* values = new QHBoxLayout(value_widget);
  values->addStretch();

  QWidget* central_value_widget = new QWidget();
  value_layout = new QHBoxLayout(central_value_widget);
  value_layout->setMargin(0);
  value_layout->addWidget(new QLabel("")); // a spacer so the layout doesn't jump
  values->addWidget(central_value_widget);

  values->addStretch();
  layout->addWidget(value_widget);

  current_row_desc = new QLabel();
  current_row_desc->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  current_row_desc->setAlignment(Qt::AlignCenter);
  layout->addWidget(current_row_desc);

  connect(view, SIGNAL(zoom_changed(double, double)), header, SLOT(update_zoom(double)));
  connect(view, SIGNAL(x_scroll_changed(int)), header, SLOT(set_scroll(int)));
  connect(view, SIGNAL(selection_changed(bool, int)), this, SLOT(set_key_button_enabled(bool, int)));

  connect(linear_button, SIGNAL(clicked(bool)), this, SLOT(set_keyframe_type()));
  connect(bezier_button, SIGNAL(clicked(bool)), this, SLOT(set_keyframe_type()));
  connect(hold_button, SIGNAL(clicked(bool)), this, SLOT(set_keyframe_type()));

  Retranslate();
}

EffectRow *GraphEditor::get_row()
{
  return row;
}

void GraphEditor::Retranslate() {
  setWindowTitle(tr("Graph Editor"));
  linear_button->setText(tr("Linear"));
  bezier_button->setText(tr("Bezier"));
  hold_button->setText(tr("Hold"));
}

void GraphEditor::update_panel() {
  if (isVisible()) {
    if (row != nullptr) {
      int slider_index = 0;
      for (int i=0;i<row->FieldCount();i++) {
        EffectField* field = row->Field(i);
        if (field->type() == EffectField::EFFECT_FIELD_DOUBLE) {
          field->UpdateWidgetValue(field_sliders_.at(slider_index), field->Now());
          slider_index++;
        }
      }
    }

    header->update();
    view->update();
  }
}

bool GraphEditor::focused()
{
  return hasFocus() || view->hasFocus() || header->hasFocus();
}

void GraphEditor::set_row(EffectRow *r) {
  for (int i=0;i<field_sliders_.size();i++) {
    delete field_sliders_.at(i);
    delete field_enable_buttons.at(i);
  }
  field_sliders_.clear();
  field_enable_buttons.clear();

  if (row != nullptr) {
    // clear old row connections
    disconnect(keyframe_nav, SIGNAL(goto_previous_key()), row, SLOT(GoToPreviousKeyframe()));
    disconnect(keyframe_nav, SIGNAL(toggle_key()), row, SLOT(ToggleKeyframe()));
    disconnect(keyframe_nav, SIGNAL(goto_next_key()), row, SLOT(GoToNextKeyframe()));
  }

  bool found_vals = false;

  if (r != nullptr && r->IsKeyframing()) {
    for (int i=0;i<r->FieldCount();i++) {
      EffectField* field = r->Field(i);
      if (field->type() == EffectField::EFFECT_FIELD_DOUBLE) {
        QPushButton* slider_button = new QPushButton();
        slider_button->setCheckable(true);
        slider_button->setChecked(field->IsEnabled());
        slider_button->setIcon(olive::icon::CreateIconFromSVG(":/icons/record.svg", false));
        slider_button->setProperty("field", i);
        slider_button->setIconSize(slider_button->iconSize()*0.5);
        connect(slider_button, SIGNAL(toggled(bool)), this, SLOT(set_field_visibility(bool)));
        field_enable_buttons.append(slider_button);
        value_layout->addWidget(slider_button);

        LabelSlider* slider = static_cast<LabelSlider*>(field->CreateWidget());
        slider->SetColor(get_curve_color(i, r->FieldCount()).name());
        field_sliders_.append(slider);
        value_layout->addWidget(slider);

        found_vals = true;
      }
    }
  }

  if (found_vals) {
    row = r;
    current_row_desc->setText(row->GetParentEffect()->parent_clip->name()
                              + " :: " + row->GetParentEffect()->meta->name
                              + " :: " + row->name());
    header->set_visible_in(r->GetParentEffect()->parent_clip->timeline_in());

    connect(keyframe_nav, SIGNAL(goto_previous_key()), row, SLOT(GoToPreviousKeyframe()));
    connect(keyframe_nav, SIGNAL(toggle_key()), row, SLOT(ToggleKeyframe()));
    connect(keyframe_nav, SIGNAL(goto_next_key()), row, SLOT(GoToNextKeyframe()));
  } else {
    row = nullptr;
    current_row_desc->setText(nullptr);
  }
  view->set_row(row);
  update_panel();
}

void GraphEditor::delete_selected_keys() {
  view->delete_selected_keys();
}

void GraphEditor::select_all() {
  view->select_all();
}

void GraphEditor::set_key_button_enabled(bool e, int type) {
  linear_button->setEnabled(e);
  linear_button->setChecked(type == EFFECT_KEYFRAME_LINEAR);
  bezier_button->setEnabled(e);
  bezier_button->setChecked(type == EFFECT_KEYFRAME_BEZIER);
  hold_button->setEnabled(e);
  hold_button->setChecked(type == EFFECT_KEYFRAME_HOLD);
}

void GraphEditor::set_keyframe_type() {
  linear_button->setChecked(linear_button == sender());
  bezier_button->setChecked(bezier_button == sender());
  hold_button->setChecked(hold_button == sender());
  view->set_selected_keyframe_type(sender()->property("type").toInt());
}

void GraphEditor::set_field_visibility(bool b) {
  view->set_field_visibility(sender()->property("field").toInt(), b);
}
