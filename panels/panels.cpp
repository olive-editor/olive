#include "panels.h"

#include "timeline.h"
#include "effectcontrols.h"
#include "viewer.h"
#include "project/sequence.h"
#include "project/clip.h"

Project* panel_project = 0;
EffectControls* panel_effect_controls = 0;
Viewer* panel_sequence_viewer = 0;
Viewer* panel_footage_viewer = 0;
Timeline* panel_timeline = 0;

void update_effect_controls() {
	// SEND CLIPS TO EFFECT CONTROLS
	// find out how many clips are selected
	// limits to one video clip and one audio clip and only if they're linked
	// one of these days it might be nice to have multiple clips in the effects panel
	panel_effect_controls->multiple = false;
	int vclip = -1;
	int aclip = -1;
	QVector<int> selected_clips;
	if (sequence != NULL) {
		for (int i=0;i<sequence->clips.size();i++) {
			Clip* clip = sequence->clips.at(i);
			if (clip != NULL && panel_timeline->is_clip_selected(clip, true)) {
				if (clip->track < 0 && vclip == -1) {
					vclip = i;
				} else if (clip->track >= 0 && aclip == -1) {
					aclip = i;
				} else {
					vclip = -2;
					aclip = -2;
					panel_effect_controls->multiple = true;
					break;
				}
			}
		}
		// check if aclip is linked to vclip
		if (!panel_effect_controls->multiple) {
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
					panel_effect_controls->multiple = true;
				}
			}
		}
	}
	panel_effect_controls->set_clips(selected_clips);
}

void update_ui(bool modified) {
	if (modified) {
//		panel_sequence_viewer->reset_all_audio();
		update_effect_controls();
	}
	panel_effect_controls->update_keyframes();
	panel_timeline->repaint_timeline();
	panel_sequence_viewer->update_viewer();
}
