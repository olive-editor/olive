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
#include "effects/effectloaders.h"
#include "global/debug.h"

#include <QScrollBar>
#include <QCoreApplication>

Project* panel_project = nullptr;
EffectControls* panel_effect_controls = nullptr;
Viewer* panel_sequence_viewer = nullptr;
Viewer* panel_footage_viewer = nullptr;
Timeline* panel_timeline = nullptr;
GraphEditor* panel_graph_editor = nullptr;

void update_effect_controls() {
  QVector<Clip*> selected_clips;
  int mode = kTransitionNone;

  if (olive::ActiveSequence != nullptr) {
    selected_clips = olive::ActiveSequence->SelectedClips();

    // TODO handle selecting transitions
  }

  panel_effect_controls->SetClips(selected_clips, mode);
}

void update_ui(bool modified) {
  if (modified) {
    update_effect_controls();
  }
  panel_effect_controls->update_keyframes();
  panel_timeline->repaint_timeline();
  panel_sequence_viewer->update_viewer();
  panel_graph_editor->update_panel();
}

QDockWidget *get_focused_panel(bool force_hover) {
  QDockWidget* w = nullptr;
  if (olive::CurrentConfig.hover_focus || force_hover) {
    if (panel_project->underMouse()) {
      w = panel_project;
    } else if (panel_effect_controls->underMouse()) {
      w = panel_effect_controls;
    } else if (panel_sequence_viewer->underMouse()) {
      w = panel_sequence_viewer;
    } else if (panel_footage_viewer->underMouse()) {
      w = panel_footage_viewer;
    } else if (panel_timeline->underMouse()) {
      w = panel_timeline;
    } else if (panel_graph_editor->view_is_under_mouse()) {
      w = panel_graph_editor;
    }
  }
  if (w == nullptr) {
    if (panel_project->is_focused()) {
      w = panel_project;
    } else if (panel_effect_controls->keyframe_focus() || panel_effect_controls->is_focused()) {
      w = panel_effect_controls;
    } else if (panel_sequence_viewer->is_focused()) {
      w = panel_sequence_viewer;
    } else if (panel_footage_viewer->is_focused()) {
      w = panel_footage_viewer;
    } else if (panel_timeline->focused()) {
      w = panel_timeline;
    } else if (panel_graph_editor->view_is_focused()) {
      w = panel_graph_editor;
    }
  }
  return w;
}

void alloc_panels(QWidget* parent) {
  // TODO maybe replace these with non-pointers later on?
  panel_sequence_viewer = new Viewer(parent);
  panel_sequence_viewer->setObjectName("seq_viewer");
  panel_footage_viewer = new Viewer(parent);
  panel_footage_viewer->setObjectName("footage_viewer");
  panel_project = new Project(parent);
  panel_project->setObjectName("proj_root");
  panel_effect_controls = new EffectControls(parent);
  init_effects();
  panel_effect_controls->setObjectName("fx_controls");
  panel_timeline = new Timeline(parent);
  panel_timeline->setObjectName("timeline");
  panel_graph_editor = new GraphEditor(parent);
  panel_graph_editor->setObjectName("graph_editor");
}

void free_panels() {
  delete panel_sequence_viewer;
  panel_sequence_viewer = nullptr;
  delete panel_footage_viewer;
  panel_footage_viewer = nullptr;
  delete panel_project;
  panel_project = nullptr;
  delete panel_effect_controls;
  panel_effect_controls = nullptr;
  delete panel_timeline;
  panel_timeline = nullptr;
}

void scroll_to_frame_internal(QScrollBar* bar, long frame, double zoom, int area_width) {
  int screen_point = getScreenPointFromFrame(zoom, frame) - bar->value();
  int min_x = area_width*0.1;
  int max_x = area_width-min_x;
  if (screen_point < min_x) {
    bar->setValue(getScreenPointFromFrame(zoom, frame) - min_x);
  } else if (screen_point > max_x) {
    bar->setValue(getScreenPointFromFrame(zoom, frame) - max_x);
  }
}
