#include "panels.h"

#include "timeline.h"
#include "effectcontrols.h"
#include "viewer.h"
#include "project.h"
#include "project/sequence.h"
#include "project/clip.h"
#include "project/transition.h"
#include "debug.h"

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
    project_model.update_data();
	panel_effect_controls->update_keyframes();
	panel_timeline->repaint_timeline();
	panel_sequence_viewer->update_viewer();
}
