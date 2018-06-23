#include "timeline.h"
#include "ui_timeline.h"

#include "panels/panels.h"
#include "panels/project.h"
#include "panels/effectcontrols.h"
#include "ui/timelinewidget.h"
#include "project/sequence.h"
#include "project/clip.h"
#include "ui/viewerwidget.h"
#include "playback/audio.h"
#include "panels/viewer.h"
#include "playback/cacher.h"
#include "playback/playback.h"
#include "ui_viewer.h"

#include <QTime>
#include <QScrollBar>
#include <QtMath>

Timeline::Timeline(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::Timeline)
{    
    selecting = moving_init = moving_proc = splitting = importing = playing = trim_in = snapped = false;
    edit_tool_also_seeks = false;
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

    update_sequence();

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

void Timeline::previous_cut() {
    bool seek_enabled = false;
    long p_cut = 0;
    for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        if (c->timeline_out > p_cut && c->timeline_out < playhead) {
            p_cut = c->timeline_out;
            seek_enabled = true;
        } else if (c->timeline_in > p_cut && c->timeline_in < playhead) {
            p_cut = c->timeline_in;
            seek_enabled = true;
        }
    }
    if (seek_enabled) seek(p_cut);
}

void Timeline::next_cut() {
    bool seek_enabled = false;
    long n_cut = LONG_MAX;
    for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        if (c->timeline_in < n_cut && c->timeline_in > playhead) {
            n_cut = c->timeline_in;
            seek_enabled = true;
        } else if (c->timeline_out < n_cut && c->timeline_out > playhead) {
            n_cut = c->timeline_out;
            seek_enabled = true;
        }
    }
    if (seek_enabled) seek(n_cut);
}

void Timeline::reset_all_audio() {
    // reset all clip audio
    for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        c->reset_audio = true;
        c->frame_sample_index = 0;
        c->audio_buffer_write = 0;
    }
    clear_audio_ibuffer();
}

void Timeline::seek(long p) {
    pause();

    reset_all_audio();
    audio_ibuffer_frame = p;

	playhead = p;

    repaint_timeline();
}

void Timeline::toggle_play() {
	if (playing) {
		pause();
	} else {
		play();
    }
}

void Timeline::play() {
    playhead_start = playhead;
    start_msecs = QDateTime::currentMSecsSinceEpoch();
	playback_updater.start();
    playing = true;
    panel_viewer->set_playpause_icon(false);
}

void Timeline::pause() {
	playing = false;
    panel_viewer->set_playpause_icon(true);
}

void Timeline::go_to_end() {
	seek(sequence->getEndFrame());
}

int Timeline::get_track_height_size(bool video) {
    if (video) {
        return video_track_heights.size();
    } else {
        return audio_track_heights.size();
    }
}

int Timeline::calculate_track_height(int track, int value) {
    int index = (track < 0) ? qAbs(track + 1) : track;
    QVector<int>& vector = (track < 0) ? video_track_heights : audio_track_heights;
    while (vector.size() < index+1) {
        vector.append(TRACK_DEFAULT_HEIGHT);
    }
    if (value > -1) {
        vector[index] = value;
    }
    return vector.at(index);
}

void Timeline::update_sequence() {
    bool null_sequence = (sequence == NULL);

	for (int i=0;i<tool_buttons.count();i++) {
		tool_buttons[i]->setEnabled(!null_sequence);
	}
    ui->snappingButton->setEnabled(!null_sequence);
	ui->pushButton_4->setEnabled(!null_sequence);
	ui->pushButton_5->setEnabled(!null_sequence);
    ui->headers->setEnabled(!null_sequence);
	ui->video_area->setEnabled(!null_sequence);
	ui->audio_area->setEnabled(!null_sequence);

	if (null_sequence) {
		setWindowTitle("Timeline: <none>");
	} else {
		setWindowTitle("Timeline: " + sequence->name);
        redraw_all_clips(false);
        playback_updater.setInterval(qFloor(1000 / sequence->frame_rate));
	}
}

int Timeline::get_snap_range() {
    return getFrameFromScreenPoint(10);
}

bool Timeline::focused() {
    return (ui->headers->hasFocus() || ui->video_area->hasFocus() || ui->audio_area->hasFocus());
}

void Timeline::undo() {
    qDebug() << "[INFO] Undo/redo was so buggy, it's not been disabled. Sorry for any inconvenience";
//    if (sequence != NULL) {
//        current_clips.clear();
//        panel_effect_controls->set_clip(NULL);
//        sequence->undo();
//        redraw_all_clips(false);
//    }
}

void Timeline::redo() {
    qDebug() << "[INFO] Undo/redo was so buggy, it's not been disabled. Sorry for any inconvenience";
//    if (sequence != NULL) {
//        current_clips.clear();
//        panel_effect_controls->set_clip(NULL);
//        sequence->redo();
//        redraw_all_clips(false);
//    }
}

QString frame_to_timecode(long f) {
    int hours = 0;
    int mins = 0;
    int secs = 0;
    int frames = 0;
    if (sequence != NULL) {
        int int_fps = qRound(sequence->frame_rate);
        hours = f/ (3600 * int_fps);
        mins = f / (60*int_fps) % 60;
        secs = f/int_fps % 60;
        frames = f%int_fps;
    }
    return QString(QString::number(hours).rightJustified(2, '0') +
                   ":" + QString::number(mins).rightJustified(2, '0') +
                   ":" + QString::number(secs).rightJustified(2, '0') +
                   ":" + QString::number(frames).rightJustified(2, '0')
                );
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
    panel_viewer->ui->currentTimecode->setText(frame_to_timecode(playhead));
}

void Timeline::redraw_all_clips(bool changed) {
    if (sequence != NULL) {
        if (changed) {
            project_changed = true;
            if (!playing) reset_all_audio();
            panel_viewer->viewer_widget->update();
        }

        ui->video_area->redraw_clips();
        ui->audio_area->redraw_clips();
        ui->headers->update();

        panel_viewer->ui->endTimecode->setText(frame_to_timecode(sequence->getEndFrame()));
    }
}

void Timeline::select_all() {
	selections.clear();
	for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        selections.append({c->timeline_in, c->timeline_out, c->track});
	}
    repaint_timeline();
}

void Timeline::delete_selection(bool ripple_delete) {
	if (selections.size() > 0) {
		panel_effect_controls->set_clip(NULL);

		long ripple_point = selections.at(0).in;
		long ripple_length = selections.at(0).out - selections.at(0).in;

        for (int i=0;i<selections.size();i++) {
            const Selection& s = selections.at(i);
            if (ripple_delete) {
                if (ripple_point > s.in) ripple_point = s.in;
                if (ripple_length > s.out - s.in) ripple_length = s.out - s.in;
            }
        }
        delete_areas_and_relink(selections);
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

        redraw_all_clips(true);
	}
}

int lerp(int a, int b, float t) {
    return ((1.0f - t) * a) + (t * b);
}

void Timeline::set_zoom(bool in) {
    if (in) {
        zoom *= 2;
    } else {
        zoom /= 2;
    }
    ui->timeline_area->horizontalScrollBar()->setValue(
                lerp(
                    ui->timeline_area->horizontalScrollBar()->value(),
                    getScreenPointFromFrame(playhead) - (ui->timeline_area->width()/2),
                    0.99f
                )
            );
    redraw_all_clips(false);
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
    set_zoom(true);
}

void Timeline::on_pushButton_5_clicked()
{
    set_zoom(false);
}

bool Timeline::is_clip_selected(Clip* clip) {
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

void Timeline::split_clip_and_relink(Clip* pre_clip, long frame, bool relink) {
    Clip* post = sequence->split_clip(pre_clip, frame);

    // if alt is not down, split clips links too
    if (relink) {
        QVector<Clip*> splits;
        splits.append(post);

        // find linked clips of old clip
        for (int i=0;i<pre_clip->linked.size();i++) {
            Clip* link = pre_clip->linked.at(i);
            if (!panel_timeline->is_clip_selected(link)) {
                Clip* s = sequence->split_clip(link, frame);
                if (s != NULL) splits.append(s);
            }
        }

        // re-link all new clips
        for (int i=0;i<splits.size();i++) {
            Clip* c = splits.at(i);
            for (int j=0;j<splits.size();j++) {
                Clip* cc = splits.at(j);
                if (c != cc) {
                    c->linked.append(cc);
                }
            }
        }
    }
}

void Timeline::delete_areas_and_relink(QVector<Selection> areas) {
    QVector<Clip*> pre_clips;
    QVector<Clip*> post_clips;

    for (int i=0;i<areas.size();i++) {
        const Selection& s = areas.at(i);
        for (int j=0;j<sequence->clip_count();j++) {
            Clip* c = sequence->get_clip(j);
            if (c->track == s.track && !c->undeletable) {
                if (c->timeline_in >= s.in && c->timeline_out <= s.out) {
                    // clips falls entirely within deletion area
                    sequence->delete_clip(j);
                    j--;
                } else if (c->timeline_in < s.in && c->timeline_out > s.out) {
                    // middle of clip is within deletion area

                    // duplicate clip
                    Clip* post = c->copy();

                    c->timeline_out = s.in;
                    post->timeline_in = s.out;
                    post->clip_in = c->clip_in + c->getLength() + (s.out - s.in);

                    pre_clips.append(c);
                    post_clips.append(post);

                    sequence->add_clip(post);
                } else if (c->timeline_in < s.in && c->timeline_out > s.in) {
                    // only out point is in deletion area
                    c->timeline_out = s.in;
                } else if (c->timeline_in < s.out && c->timeline_out > s.out) {
                    // only in point is in deletion area
                    c->clip_in += s.out - c->timeline_in;
                    c->timeline_in = s.out;
                }
            }
        }
    }
    for (int i=0;i<pre_clips.size();i++) {
        Clip* c = pre_clips.at(i);
        for (int j=0;j<pre_clips.size();j++) {
            for (int k=0;k<c->linked.size();k++) {
                if (c->linked.at(k) == pre_clips.at(j)) {
                    post_clips.at(i)->linked.append(post_clips.at(j));
                }
            }
        }
    }
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
        QVector<Selection> delete_areas;
        for (int i=0;i<clip_clipboard.size();i++) {
            Clip* c = clip_clipboard.at(i);
            delete_areas.append({c->timeline_in + playhead, c->timeline_out + playhead, c->track});
        }
        delete_areas_and_relink(delete_areas);
        for (int i=0;i<clip_clipboard.size();i++) {
            Clip* c = clip_clipboard.at(i);
            Clip* cc = c->copy();
            cc->timeline_in += playhead;
            cc->timeline_out += playhead;
            cc->sequence = sequence;
            sequence->add_clip(cc);
        }
        redraw_all_clips(true);
    }
}

bool Timeline::split_selection() {
    bool split = false;

    // temporary relinking vectors
    QVector<Clip*> pre_splits;
    QVector<Clip*> post_splits;

    // find clips within selection and split
    for (int j=0;j<sequence->clip_count();j++) {
        for (int i=0;i<selections.size();i++) {
            const Selection& s = selections.at(i);
            Clip* clip = sequence->get_clip(j);
            if (s.track == clip->track) {
                Clip* post_a = sequence->split_clip(clip, s.in);
                Clip* post_b = sequence->split_clip(clip, s.out);

                if (post_a != NULL) {
                    pre_splits.append(clip);
                    post_splits.append(post_a);
                }
                if (post_b != NULL) {
                    pre_splits.append(clip);
                    post_splits.append(post_b);
                }

                split = true;
            }
        }
    }

    // relink after splitting
    for (int i=0;i<pre_splits.size();i++) {
        // see how many "pre"s were linked and relink the "post"s
        Clip* pre = pre_splits.at(i);
        for (int j=0;j<pre_splits.size();j++) {
            for (int k=0;k<pre->linked.size();k++) {
                if (pre->linked.at(k) == pre_splits.at(j)) {
                    post_splits.at(i)->linked.append(post_splits.at(j));
                }
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
            Clip* clip = sequence->get_clip(j);
            if (is_clip_selected(clip)) {
                // should this relink?
                sequence->split_clip(clip, playhead);
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
            // always relinks
            split_clip_and_relink(sequence->get_clip(j), playhead, true);
            split_selected = true;
        }
    }

    if (split_selected) redraw_all_clips(true);
}

bool Timeline::snap_to_point(long point, long* l) {
    int limit = get_snap_range();
    if (*l > point-limit-1 && *l < point+limit+1) {
        snap_point = point;
        *l = point;
        snapped = true;
        return true;
    }
    return false;
}

void Timeline::snap_to_clip(long* l, bool playhead_inclusive) {
    snapped = false;
    if (snapping) {
        if (!playhead_inclusive || !snap_to_point(playhead, l)) {
            for (int i=0;i<sequence->clip_count();i++) {
                Clip* c = sequence->get_clip(i);
                if (snap_to_point(c->timeline_in, l)) {
                    break;
                } else if (snap_to_point(c->timeline_out, l)) {
                    break;
                }
            }
        }
    }
}

void Timeline::increase_track_height() {
    for (int i=0;i<video_track_heights.size();i++) {
        video_track_heights[i] += TRACK_HEIGHT_INCREMENT;
    }
    for (int i=0;i<audio_track_heights.size();i++) {
        audio_track_heights[i] += TRACK_HEIGHT_INCREMENT;
    }
}

void Timeline::decrease_track_height() {
    for (int i=0;i<video_track_heights.size();i++) {
        video_track_heights[i] -= TRACK_HEIGHT_INCREMENT;
        if (video_track_heights[i] < TRACK_MIN_HEIGHT) video_track_heights[i] = TRACK_MIN_HEIGHT;
    }
    for (int i=0;i<audio_track_heights.size();i++) {
        audio_track_heights[i] -= TRACK_HEIGHT_INCREMENT;
        if (audio_track_heights[i] < TRACK_MIN_HEIGHT) audio_track_heights[i] = TRACK_MIN_HEIGHT;
    }
}

void Timeline::deselect() {
    selections.clear();
    repaint_timeline();
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
