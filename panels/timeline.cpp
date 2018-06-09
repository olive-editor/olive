#include "timeline.h"
#include "ui_timeline.h"

#include "panels/panels.h"
#include "panels/effectcontrols.h"
#include "ui/timelinewidget.h"
#include "project/sequence.h"
#include "project/clip.h"
#include "ui/viewerwidget.h"
#include "playback/audio.h"
#include "panels/viewer.h"
#include "playback/cacher.h"
#include "playback/playback.h"

#include <QTime>

Timeline::Timeline(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::Timeline)
{
    selecting = moving_init = moving_proc = splitting = importing = playing = trim_in = snapped = false;
    snapping = true;
    last_frame = playhead = snap_point = cursor_frame = cursor_track = 0;
	trim_target = -1;

	ui->setupUi(this);

	ui->video_area->bottom_align = true;

	tool_buttons.append(ui->toolArrowButton);
	tool_buttons.append(ui->toolEditButton);
	tool_buttons.append(ui->toolRippleButton);
	tool_buttons.append(ui->toolRazorButton);
	tool_buttons.append(ui->toolSlipButton);
	tool_buttons.append(ui->toolRollingButton);

	ui->toolArrowButton->click();

	zoom = 1.0f;

	sequence = NULL;
	set_sequence(NULL);

    connect(&playback_updater, SIGNAL(timeout()), this, SLOT(repaint_timeline()));
}

Timeline::~Timeline()
{
	delete ui;
}

void Timeline::go_to_start() {
	seek(0);
}

void Timeline::previous_frame() {
	seek(playhead-1);
}

void Timeline::next_frame() {
	seek(playhead+1);
}

void Timeline::seek(long p) {
	pause();

	// reset all clip audio
	for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        c->reset_audio = true;
        c->frame_sample_index = 0;
        c->audio_buffer_write = 0;
	}
    clear_audio_ibuffer();
    audio_ibuffer_read = 0;

	playhead = p;

    repaint_timeline();
}

bool Timeline::toggle_play() {
	if (playing) {
		pause();
	} else {
		play();
	}
	return playing;
}

void Timeline::play() {
    playhead_start = playhead;
    start_msecs = QDateTime::currentMSecsSinceEpoch();
	playback_updater.start();
    playing = true;
}

void Timeline::pause() {
	playing = false;
}

void Timeline::go_to_end() {
	seek(sequence->getEndFrame());
}

void Timeline::set_sequence(Sequence *s) {
	if (sequence != NULL) {
		// clean up - close all open clips
		for (int i=0;i<sequence->clip_count();i++) {
            Clip* c = sequence->get_clip(i);
            if (c->open) {
                close_clip(c);
			}
		}
	}

	sequence = s;
	bool null_sequence = (s == NULL);

	for (int i=0;i<tool_buttons.count();i++) {
		tool_buttons[i]->setEnabled(!null_sequence);
	}
	ui->pushButton_4->setEnabled(!null_sequence);
	ui->pushButton_5->setEnabled(!null_sequence);
    ui->headers->setEnabled(!null_sequence);
	ui->video_area->setEnabled(!null_sequence);
	ui->audio_area->setEnabled(!null_sequence);

	if (null_sequence) {
		setWindowTitle("Timeline: <none>");
	} else {
		setWindowTitle("Timeline: " + sequence->name);
		redraw_all_clips();
		playback_updater.setInterval(floor(1000 / sequence->frame_rate));
	}
}

bool Timeline::focused() {
    return (ui->headers->hasFocus() || ui->video_area->hasFocus() || ui->audio_area->hasFocus());
}

void Timeline::undo() {
    if (sequence != NULL) {
        current_clips.clear();
        sequence->undo();
        ui->video_area->redraw_clips();
        ui->audio_area->redraw_clips();
    }
}

void Timeline::redo() {
    if (sequence != NULL) {
        current_clips.clear();
        sequence->redo();
        ui->video_area->redraw_clips();
        ui->audio_area->redraw_clips();
    }
}

void Timeline::repaint_timeline() {
    if (playing) {
        playhead = round(playhead_start + ((QDateTime::currentMSecsSinceEpoch()-start_msecs) * 0.001 * sequence->frame_rate));
	}
    ui->headers->update();
	ui->video_area->update();
	ui->audio_area->update();
    if (last_frame != playhead) {
		panel_viewer->viewer_widget->update();
		last_frame = playhead;
	}
}

void Timeline::redraw_all_clips() {
    // add current sequence to the undo stack
    sequence->undo_add_current();

	ui->video_area->redraw_clips();
	ui->audio_area->redraw_clips();
}

void Timeline::select_all() {
	selections.clear();
	for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        selections.append({c->timeline_in, c->timeline_out, c->track});
	}
}

void Timeline::delete_selection(bool ripple_delete) {
	if (selections.size() > 0) {
		panel_effect_controls->set_clip(NULL);

		long ripple_point = selections.at(0).in;
		long ripple_length = selections.at(0).out - selections.at(0).in;

		for (int i=0;i<selections.size();i++) {
			const Selection& s = selections.at(i);
			sequence->delete_area(s.in, s.out, s.track);
			if (ripple_delete) {
				if (ripple_point > s.in) ripple_point = s.in;
				if (ripple_length > s.out - s.in) ripple_length = s.out - s.in;
			}
		}
		selections.clear();

		if (ripple_delete) {
			long validator;
			for (int i=0;i<sequence->clip_count();i++) {
				// check every clip after and see if it'll collide
				// NOTE, we could probably re-use the validation code for the ripple tool here for optimization (since it's technically better code I think)
                Clip* c = sequence->get_clip(i);
                if (c->timeline_in >= ripple_point) {
					for (int j=0;j<sequence->clip_count();j++) {
                        Clip* cc = sequence->get_clip(j);
                        if (cc->timeline_in < ripple_point) {
                            validator = c->timeline_in - ripple_length - cc->timeline_out;
							if (validator < 0) ripple_length += validator;

							if (ripple_length <= 0) {
								// we've seen all we need to see here (can't ripple so stop looping)
								i = j = sequence->clip_count();
							}
						}
					}
				}
			}
			if (ripple_length > 0) ripple(ripple_point, -ripple_length);
		}

		redraw_all_clips();
	}
}

void Timeline::zoom_in() {
	zoom *= 2;
	redraw_all_clips();
}

void Timeline::zoom_out() {
	zoom /= 2;
	redraw_all_clips();
}

void Timeline::ripple(long ripple_point, long ripple_length) {
	for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        if (c->timeline_in >= ripple_point) {
            c->timeline_in += ripple_length;
            c->timeline_out += ripple_length;
		}
	}
	for (int i=0;i<selections.size();i++) {
		Selection& s = selections[i];
		if (s.in >= ripple_point) {
			s.in += ripple_length;
			s.out += ripple_length;
		}
	}
}

void Timeline::decheck_tool_buttons(QObject* sender) {
	for (int i=0;i<tool_buttons.count();i++) {
		if (tool_buttons[i] != sender) {
			tool_buttons[i]->setChecked(false);
		}
	}
}

void Timeline::on_toolEditButton_toggled(bool checked)
{
	if (checked) {
		decheck_tool_buttons(sender());
		ui->timeline_area->setCursor(Qt::IBeamCursor);
		tool = TIMELINE_TOOL_EDIT;
	}
}

void Timeline::on_toolArrowButton_toggled(bool checked)
{
	if (checked) {
		decheck_tool_buttons(sender());
		ui->timeline_area->setCursor(Qt::ArrowCursor);
		tool = TIMELINE_TOOL_POINTER;
	}
}

void Timeline::on_toolRazorButton_toggled(bool checked)
{
	if (checked) {
		decheck_tool_buttons(sender());
		ui->timeline_area->setCursor(Qt::IBeamCursor);
		tool = TIMELINE_TOOL_RAZOR;
	}
}

void Timeline::on_pushButton_4_clicked()
{
	zoom_in();
}

void Timeline::on_pushButton_5_clicked()
{
	zoom_out();
}

bool Timeline::is_clip_selected(int clip_index) {
    Clip* clip = sequence->get_clip(clip_index);
	for (int i=0;i<selections.size();i++) {
		const Selection& s = selections.at(i);
        if (clip->track == s.track && clip->timeline_in >= s.in && clip->timeline_out <= s.out) {
			return true;
		}
	}
	return false;
}

void Timeline::on_toolRippleButton_toggled(bool checked)
{
	if (checked) {
		decheck_tool_buttons(sender());
		ui->timeline_area->setCursor(Qt::ArrowCursor);
		tool = TIMELINE_TOOL_RIPPLE;
	}
}

void Timeline::on_toolRollingButton_toggled(bool checked)
{
	if (checked) {
		decheck_tool_buttons(sender());
		ui->timeline_area->setCursor(Qt::ArrowCursor);
		tool = TIMELINE_TOOL_ROLLING;
	}
}

void Timeline::on_toolSlipButton_toggled(bool checked)
{
	if (checked) {
		decheck_tool_buttons(sender());
		ui->timeline_area->setCursor(Qt::ArrowCursor);
		tool = TIMELINE_TOOL_SLIP;
	}
}

void Timeline::on_snappingButton_toggled(bool checked)
{
    snapping = checked;
}

void Timeline::copy(bool del) {
    bool cleared = false;
    bool copied = false;

    long min_in = 0;

    for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        for (int j=0;j<selections.size();j++) {
            const Selection& s = selections.at(j);
            if (s.track == c->track && !((c->timeline_in < s.in && c->timeline_out < s.in) || (c->timeline_in > s.out && c->timeline_out > s.out))) {
                if (!cleared) {
                    clip_clipboard.clear();
                    cleared = true;
                }

                Clip* copied_clip = c->copy();

                if (copied_clip->timeline_in < s.in) {
                    copied_clip->clip_in += (s.in - copied_clip->timeline_in);
                    copied_clip->timeline_in = s.in;
                }

                if (copied_clip->timeline_out > s.out) {
                    copied_clip->timeline_out = s.out;
                }

                if (copied) {
                    min_in = qMin(min_in, s.in);
                } else {
                    min_in = s.in;
                    copied = true;
                }

                clip_clipboard.append(copied_clip);
            }
        }
    }

    for (int i=0;i<clip_clipboard.size();i++) {
        // initialize all timeline_ins to 0 or offsets of
        clip_clipboard[i]->timeline_in -= min_in;
        clip_clipboard[i]->timeline_out -= min_in;
    }

    if (del && copied) {
        delete_selection(false);
    }
}

void Timeline::paste() {
    if (clip_clipboard.size() > 0) {
        for (int i=0;i<clip_clipboard.size();i++) {
            Clip* c = clip_clipboard.at(i);
            sequence->delete_area(c->timeline_in + playhead, c->timeline_out + playhead, c->track);
            Clip* cc = c->copy();
            cc->timeline_in += playhead;
            cc->timeline_out += playhead;
            sequence->add_clip(cc);
        }
        redraw_all_clips();
    }
}

bool Timeline::split_selection() {
    bool split = false;
    for (int j=0;j<sequence->clip_count();j++) {
        for (int i=0;i<selections.size();i++) {
            const Selection& s = selections.at(i);
            if (s.track == sequence->get_clip(j)->track) {
                sequence->split_clip(j, s.in);
                sequence->split_clip(j, s.out);
                split = true;
            }
        }
    }
    return split;
}

void Timeline::split_at_playhead() {
    bool split_selected = false;

    if (selections.size() > 0) {
        // see if whole clips are selected
        for (int j=0;j<sequence->clip_count();j++) {
            if (is_clip_selected(j)) {
                sequence->split_clip(j, playhead);
                split_selected = true;
            }
        }

        // split a selection if not
        if (!split_selected) {
            split_selected = split_selection();
        }
    }

    // if nothing was selected or no selections fell within playhead, simply split at playhead
    if (!split_selected) {
        for (int j=0;j<sequence->clip_count();j++) {
            sequence->split_clip(j, playhead);
            split_selected = true;
        }
    }

    if (split_selected) redraw_all_clips();
}

void Timeline::snap_to_clip(long* l) {
    int limit = 10;
    snapped = false;
    for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        if (*l > c->timeline_in-limit-1 &&
                *l < c->timeline_in+limit+1) {
            *l = c->timeline_in;
            snapped = true;
            snap_point = c->timeline_in;
            break;
        } else if (*l > c->timeline_out-limit-1 &&
                   *l < c->timeline_out+limit+1) {
            *l = c->timeline_out;
            snapped = true;
            snap_point = c->timeline_out;
            break;
        }
    }
}

long Timeline::getFrameFromScreenPoint(int x) {
    long f = round((float) x / zoom);
    if (f < 0) {
        return 0;
    }
    return f;
}

int Timeline::getScreenPointFromFrame(long frame) {
    return (int) round(frame*zoom);
}
