#ifndef PANELS_H
#define PANELS_H

class Project;
class EffectControls;
class Viewer;
class Timeline;
class GraphEditor;

class QWidget;
class QDockWidget;
class QScrollBar;

extern Project* panel_project;
extern EffectControls* panel_effect_controls;
extern Viewer* panel_sequence_viewer;
extern Viewer* panel_footage_viewer;
extern Timeline* panel_timeline;
extern GraphEditor* panel_graph_editor;

void update_ui(bool modified);
QDockWidget* get_focused_panel(bool force_hover = false);
void alloc_panels(QWidget *parent);
void free_panels();
void scroll_to_frame_internal(QScrollBar* bar, long frame, double zoom, int area_width);

#endif // PANELS_H
