#ifndef PANELS_H
#define PANELS_H

class Project;
class EffectControls;
class Viewer;
class Timeline;
class GraphEditor;

class QWidget;
class QDockWidget;

extern Project* panel_project;
extern EffectControls* panel_effect_controls;
extern Viewer* panel_sequence_viewer;
extern Viewer* panel_footage_viewer;
extern Timeline* panel_timeline;
extern GraphEditor* panel_graph_editor;

void update_ui(bool modified);
QDockWidget* get_focused_panel();
void alloc_panels(QWidget *parent);
void free_panels();

#endif // PANELS_H
