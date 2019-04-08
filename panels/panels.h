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

#ifndef PANELS_H
#define PANELS_H

#include "timeline.h"
#include "effectcontrols.h"
#include "viewer.h"
#include "grapheditor.h"
#include "project.h"
#include "nodeeditor.h"

extern QVector<Project*> panel_project;
extern EffectControls* panel_effect_controls;
extern Viewer* panel_sequence_viewer;
extern Viewer* panel_footage_viewer;
extern QVector<Timeline*> panel_timeline;
extern GraphEditor* panel_graph_editor;
extern NodeEditor* panel_node_editor;

void update_ui(bool modified);
QDockWidget* get_focused_panel(bool force_hover = false);
void alloc_panels(QWidget *parent);
void free_panels();
void scroll_to_frame_internal(QScrollBar* bar, long frame, double zoom, int area_width);

#endif // PANELS_H
