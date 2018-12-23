#ifndef PANELS_H
#define PANELS_H

class Project;
class EffectControls;
class Viewer;
class Timeline;
class QDockWidget;

extern Project* panel_project;
extern EffectControls* panel_effect_controls;
extern Viewer* panel_sequence_viewer;
extern Viewer* panel_footage_viewer;
extern Timeline* panel_timeline;

void update_ui(bool modified);
QDockWidget* get_focused_panel();

#endif // PANELS_H
