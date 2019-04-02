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

#include "focusfilter.h"

#include "panels/panels.h"
#include "timeline/sequence.h"
#include "ui/timelineheader.h"

FocusFilter olive::FocusFilter;

FocusFilter::FocusFilter() {}

void FocusFilter::go_to_in() {
  QDockWidget* focused_panel = get_focused_panel();
  if (focused_panel == panel_footage_viewer) {
    panel_footage_viewer->go_to_in();
  } else {
    panel_sequence_viewer->go_to_in();
  }
}

void FocusFilter::go_to_out() {
  QDockWidget* focused_panel = get_focused_panel();
  if (focused_panel == panel_footage_viewer) {
    panel_footage_viewer->go_to_out();
  } else {
    panel_sequence_viewer->go_to_out();
  }
}

void FocusFilter::go_to_start() {
  QDockWidget* focused_panel = get_focused_panel();
  if (focused_panel == panel_footage_viewer) {
    panel_footage_viewer->go_to_start();
  } else {
    panel_sequence_viewer->go_to_start();
  }
}

void FocusFilter::prev_frame() {
  QDockWidget* focused_panel = get_focused_panel();
  if (focused_panel == panel_footage_viewer) {
    panel_footage_viewer->previous_frame();
  } else {
    panel_sequence_viewer->previous_frame();
  }
}

void FocusFilter::play_in_to_out() {
  QDockWidget* focused_panel = get_focused_panel();
  if (focused_panel == panel_footage_viewer) {
    panel_footage_viewer->play(true);
  } else {
    panel_sequence_viewer->play(true);
  }
}

void FocusFilter::next_frame() {
  QDockWidget* focused_panel = get_focused_panel();
  if (focused_panel == panel_footage_viewer) {
    panel_footage_viewer->next_frame();
  } else {
    panel_sequence_viewer->next_frame();
  }
}

void FocusFilter::go_to_end() {
  QDockWidget* focused_panel = get_focused_panel();
  if (focused_panel == panel_footage_viewer) {
    panel_footage_viewer->go_to_end();
  } else {
    panel_sequence_viewer->go_to_end();
  }
}

void FocusFilter::set_viewer_fullscreen() {
  if (get_focused_panel() == panel_footage_viewer) {
    panel_footage_viewer->viewer_widget()->set_fullscreen();
  } else {
    panel_sequence_viewer->viewer_widget()->set_fullscreen();
  }
}

void FocusFilter::set_marker() {
  if (Timeline::GetTopSequence() != nullptr) {
    QDockWidget* focused_panel = get_focused_panel();

    if (focused_panel == panel_footage_viewer) {
      panel_footage_viewer->set_marker();
    } else if (focused_panel == panel_sequence_viewer) {
      panel_sequence_viewer->set_marker();
    } else {
      panel_timeline.first()->set_marker();
    }
  }
}

void FocusFilter::playpause() {
  QDockWidget* focused_panel = get_focused_panel();
  if (focused_panel == panel_footage_viewer) {
    panel_footage_viewer->toggle_play();
  } else {
    panel_sequence_viewer->toggle_play();
  }
}

void FocusFilter::pause() {
  QDockWidget* focused_panel = get_focused_panel();
  if (focused_panel == panel_footage_viewer) {
    panel_footage_viewer->pause();
  } else {
    panel_sequence_viewer->pause();
  }
}

void FocusFilter::increase_speed() {
  QDockWidget* focused_panel = get_focused_panel();
  if (focused_panel == panel_footage_viewer) {
    panel_footage_viewer->increase_speed();
  } else {
    panel_sequence_viewer->increase_speed();
  }
}

void FocusFilter::decrease_speed() {
  QDockWidget* focused_panel = get_focused_panel();
  if (focused_panel == panel_footage_viewer) {
    panel_footage_viewer->decrease_speed();
  } else {
    panel_sequence_viewer->decrease_speed();
  }
}

void FocusFilter::set_in_point() {
  if (get_focused_panel() == panel_footage_viewer) {
    panel_footage_viewer->set_in_point();
  } else {
    panel_sequence_viewer->set_in_point();
  }
}

void FocusFilter::set_out_point() {
  if (get_focused_panel() == panel_footage_viewer) {
    panel_footage_viewer->set_out_point();
  } else {
    panel_sequence_viewer->set_out_point();
  }
}

void FocusFilter::clear_in() {
  if (get_focused_panel() == panel_footage_viewer) {
    panel_footage_viewer->clear_in();
  } else {
    panel_sequence_viewer->clear_in();
  }
}

void FocusFilter::clear_out() {
  if (get_focused_panel() == panel_footage_viewer) {
    panel_footage_viewer->clear_out();
  } else {
    panel_sequence_viewer->clear_out();
  }
}

void FocusFilter::clear_inout() {
  if (get_focused_panel() == panel_footage_viewer) {
    panel_footage_viewer->clear_inout_point();
  } else {
    panel_sequence_viewer->clear_inout_point();
  }
}

void FocusFilter::delete_function() {
  if (panel_timeline.first()->headers->hasFocus()) {
    panel_timeline.first()->headers->delete_markers();
  } else if (panel_footage_viewer->headers->hasFocus()) {
    panel_footage_viewer->headers->delete_markers();
  } else if (panel_sequence_viewer->headers->hasFocus()) {
    panel_sequence_viewer->headers->delete_markers();
  } else if (panel_effect_controls->focused()) {
    panel_effect_controls->DeleteSelectedEffects();
  } else if (panel_project.first()->focused()) {
    panel_project.first()->delete_selected_media();
  } else if (panel_effect_controls->focused()) {
    panel_effect_controls->delete_selected_keyframes();
  } else if (panel_graph_editor->focused()) {
    panel_graph_editor->delete_selected_keys();
  } else {
    Sequence* top_sequence = Timeline::GetTopSequence().get();
    ComboAction* ca = new ComboAction();
    top_sequence->DeleteAreas(ca, top_sequence->Selections(), true);
    olive::undo_stack.push(ca);
  }
}

void FocusFilter::duplicate() {
  if (panel_project.first()->focused()) {
    panel_project.first()->duplicate_selected();
  }
}

void FocusFilter::select_all() {
  QDockWidget* focused_panel = get_focused_panel();
  if (focused_panel == panel_graph_editor) {
    panel_graph_editor->select_all();
  } else {
    panel_timeline.first()->select_all();
  }
}

void FocusFilter::zoom_in() {
  QDockWidget* focused_panel = get_focused_panel();
  if (focused_panel == panel_effect_controls) {
    panel_effect_controls->set_zoom(true);
  } else if (focused_panel == panel_footage_viewer) {
    panel_footage_viewer->set_zoom(true);
  } else if (focused_panel == panel_sequence_viewer) {
    panel_sequence_viewer->set_zoom(true);
  } else {
    panel_timeline.first()->zoom_in();
  }
}

void FocusFilter::zoom_out() {
  QDockWidget* focused_panel = get_focused_panel();
  if (focused_panel == panel_effect_controls) {
    panel_effect_controls->set_zoom(false);
  } else if (focused_panel == panel_footage_viewer) {
    panel_footage_viewer->set_zoom(false);
  } else if (focused_panel == panel_sequence_viewer) {
    panel_sequence_viewer->set_zoom(false);
  } else {
    panel_timeline.first()->zoom_out();
  }
}

void FocusFilter::cut() {
  if (Timeline::GetTopSequence() != nullptr) {
    QDockWidget* focused_panel = get_focused_panel();
    if (panel_effect_controls == focused_panel) {
      panel_effect_controls->copy(true);
    } else {
      panel_timeline.first()->copy(true);
    }
  }
}

void FocusFilter::copy() {
  if (Timeline::GetTopSequence() != nullptr) {
    QDockWidget* focused_panel = get_focused_panel();
    if (panel_effect_controls == focused_panel) {
      panel_effect_controls->copy(false);
    } else {
      panel_timeline.first()->copy(false);
    }
  }
}
