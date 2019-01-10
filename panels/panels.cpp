#include "panels.h"

#include "timeline.h"
#include "effectcontrols.h"
#include "viewer.h"
#include "project.h"
#include "project/sequence.h"
#include "project/clip.h"
#include "project/transition.h"
#include "io/config.h"
#include "grapheditor.h"
#include "debug.h"

#include <QScrollBar>

Project* panel_project = 0;
EffectControls* panel_effect_controls = 0;
Viewer* panel_sequence_viewer = 0;
Viewer* panel_footage_viewer = 0;
Timeline* panel_timeline = 0;
GraphEditor* panel_graph_editor = 0;

void update_effect_controls() {
	// SEND CLIPS TO EFFECT CONTROLS
	// find out how many clips are selected
	// limits to one video clip and one audio clip and only if they're linked
	// one of these days it might be nice to have multiple clips in the effects panel
	bool multiple = false;
	int vclip = -1;
	int aclip = -1;
	QVector<int> selected_clips;
	int mode = TA_NO_TRANSITION;
	if (sequence != NULL) {
		for (int i=0;i<sequence->clips.size();i++) {
			Clip* clip = sequence->clips.at(i);
			if (clip != NULL) {
				for (int j=0;j<sequence->selections.size();j++) {
					const Selection& s = sequence->selections.at(j);
					bool add = true;
					if (clip->timeline_in >= s.in && clip->timeline_out <= s.out && clip->track == s.track) {
						mode = TA_NO_TRANSITION;
					} else if (selection_contains_transition(s, clip, TA_OPENING_TRANSITION)) {
						mode = TA_OPENING_TRANSITION;
					} else if (selection_contains_transition(s, clip, TA_CLOSING_TRANSITION)) {
						mode = TA_CLOSING_TRANSITION;
					} else {
						add = false;
					}

					if (add) {
						if (clip->track < 0 && vclip == -1) {
							vclip = i;
						} else if (clip->track >= 0 && aclip == -1) {
							aclip = i;
						} else {
							vclip = -2;
							aclip = -2;
							multiple = true;
							multiple = true;
							break;
						}
					}
				}
			}
		}

		if (!multiple) {
			// check if aclip is linked to vclip
			if (vclip >= 0) selected_clips.append(vclip);
			if (aclip >= 0) selected_clips.append(aclip);
			if (vclip >= 0 && aclip >= 0) {
				bool found = false;
				Clip* vclip_ref = sequence->clips.at(vclip);
				for (int i=0;i<vclip_ref->linked.size();i++) {
					if (vclip_ref->linked.at(i) == aclip) {
						found = true;
						break;
					}
				}
				if (!found) {
					// only display multiple clips if they're linked
					selected_clips.clear();
					multiple = true;
				}
			}
		}
	}

	bool same = (selected_clips.size() == panel_effect_controls->selected_clips.size());
	if (same) {
		for (int i=0;i<selected_clips.size();i++) {
			if (selected_clips.at(i) != panel_effect_controls->selected_clips.at(i)) {
				same = false;
				break;
			}
		}
	}

	if (panel_effect_controls->multiple != multiple || !same) {
		panel_effect_controls->multiple = multiple;
		panel_effect_controls->set_clips(selected_clips, mode);
	}
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

QDockWidget *get_focused_panel() {
	QDockWidget* w = NULL;
	if (config.hover_focus) {
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
		}
	}
	if (w == NULL) {
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
	panel_sequence_viewer = NULL;
	delete panel_footage_viewer;
	panel_footage_viewer = NULL;
	delete panel_project;
	panel_project = NULL;
	delete panel_effect_controls;
	panel_effect_controls = NULL;
	delete panel_timeline;
	panel_timeline = NULL;
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
