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

#include "panels.h"

#include "timeline/sequence.h"
#include "timeline/clip.h"
#include "effects/transition.h"
#include "global/config.h"
#include "global/debug.h"
#include "global/math.h"

#include <QScrollBar>
#include <QCoreApplication>

QVector<Project*> panel_project;
EffectControls* panel_effect_controls = nullptr;
Viewer* panel_sequence_viewer = nullptr;
Viewer* panel_footage_viewer = nullptr;
QVector<Timeline*> panel_timeline;
GraphEditor* panel_graph_editor = nullptr;
NodeEditor* panel_node_editor = nullptr;

void update_ui(bool modified) {
  if (modified) {
    panel_effect_controls->SetClips();
  }
  panel_effect_controls->update_keyframes();
  for (int i=0;i<panel_timeline.size();i++) {
    panel_timeline.at(i)->repaint_timeline();
  }
  panel_sequence_viewer->update_viewer();
  panel_graph_editor->update_panel();
}

QDockWidget *get_focused_panel(bool force_hover) {
  QDockWidget* w = nullptr;
  if (olive::config.hover_focus || force_hover) {
    for (int i=0;i<olive::panels.size();i++) {
      if (olive::panels.at(i)->underMouse()) {
        w = olive::panels.at(i);
        break;
      }
    }
  }
  if (w == nullptr) {
    for (int i=0;i<olive::panels.size();i++) {
      if (olive::panels.at(i)->focused()) {
        w = olive::panels.at(i);
        break;
      }
    }
  }
  return w;
}

void alloc_panels(QWidget* parent) {
  panel_sequence_viewer = new Viewer(parent);
  panel_sequence_viewer->setObjectName("seq_viewer");
  panel_footage_viewer = new Viewer(parent);
  panel_footage_viewer->setObjectName("footage_viewer");
  panel_footage_viewer->show_videoaudio_buttons(true);
  Project* first_project_panel = new Project(parent);
  first_project_panel->setObjectName("proj_root");
  panel_project.append(first_project_panel);
  panel_effect_controls = new EffectControls(parent);
  panel_effect_controls->setObjectName("fx_controls");
  Timeline* first_timeline_panel = new Timeline(parent);
  first_timeline_panel->setObjectName("timeline");
  panel_timeline.append(first_timeline_panel);
  panel_graph_editor = new GraphEditor(parent);
  panel_graph_editor->setObjectName("graph_editor");
  panel_node_editor = new NodeEditor(parent);
  panel_node_editor->setObjectName("node_editor");
}

void free_panels() {
  delete panel_sequence_viewer;
  panel_sequence_viewer = nullptr;
  delete panel_footage_viewer;
  panel_footage_viewer = nullptr;

  for (int i=0;i<panel_project.size();i++) {
    delete panel_project.at(i);
  }
  panel_project.clear();

  delete panel_effect_controls;
  panel_effect_controls = nullptr;

  for (int i=0;i<panel_timeline.size();i++) {
    delete panel_timeline.at(i);
  }
  panel_timeline.clear();
}

void scroll_to_frame_internal(QScrollBar* bar, long frame, double zoom, int area_width) {
  if (bar->value() == bar->minimum() || bar->value() == bar->maximum()) {
    return;
  }

  int screen_point = getScreenPointFromFrame(zoom, frame) - bar->value();
  int min_x = area_width*0.1;
  int max_x = area_width-min_x;
  if (screen_point < min_x) {
    bar->setValue(getScreenPointFromFrame(zoom, frame) - min_x);
  } else if (screen_point > max_x) {
    bar->setValue(getScreenPointFromFrame(zoom, frame) - max_x);
  }
}
