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
#include "effects/transition.h"
#include "ui_viewer.h"
#include "project/undo.h"
#include "io/config.h"

#include <QTime>
#include <QScrollBar>
#include <QtMath>
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QMenu>

long refactor_frame_number(long framenumber, double source_frame_rate, double target_frame_rate) {
    if (source_frame_rate == target_frame_rate) return framenumber;
    return qRound(((double)framenumber/source_frame_rate)*target_frame_rate);
}

void draw_selection_rectangle(QPainter& painter, const QRect& rect) {
	painter.setPen(QColor(204, 204, 204));
	painter.setBrush(QColor(0, 0, 0, 32));
	painter.drawRect(rect);
}

Timeline::Timeline(QWidget *parent) :
    QDockWidget(parent),
    playhead(0),
    playing(false),
    cursor_frame(0),
    cursor_track(0),
    zoom(1.0),
    snapping(true),
    snapped(false),
    snap_point(0),
    selecting(false),
    rect_select_init(false),
    rect_select_proc(false),
    moving_init(false),
    moving_proc(false),
    trim_target(-1),
    trim_in_point(false),
    splitting(false),
    importing(false),
    zoomChanged(false),
    ui(new Ui::Timeline),
	last_frame(0),
	creating(false)
{
	default_track_height = (QGuiApplication::primaryScreen()->logicalDotsPerInch() / 96) * TRACK_DEFAULT_HEIGHT;

	ui->setupUi(this);

	ui->video_area->bottom_align = true;

	ui->video_area->container = ui->videoScrollArea;
	ui->audio_area->container = ui->audioScrollArea;

	tool_buttons.append(ui->toolArrowButton);
	tool_buttons.append(ui->toolEditButton);
	tool_buttons.append(ui->toolRippleButton);
	tool_buttons.append(ui->toolRazorButton);
	tool_buttons.append(ui->toolSlipButton);
	tool_buttons.append(ui->toolRollingButton);
    tool_buttons.append(ui->toolSlideButton);

	ui->toolArrowButton->click();

	int timeline_area_height = (ui->timeline_area->height()>>1);
	ui->videoScrollArea->resize(ui->videoScrollArea->width(), timeline_area_height);
	ui->audioScrollArea->resize(ui->audioScrollArea->width(), timeline_area_height);
	connect(ui->audioScrollArea->horizontalScrollBar(), SIGNAL(valueChanged(int)), ui->videoScrollArea->horizontalScrollBar(), SLOT(setValue(int)));
	connect(ui->audioScrollArea->horizontalScrollBar(), SIGNAL(valueChanged(int)), ui->headerScrollArea->horizontalScrollBar(), SLOT(setValue(int)));

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
    if (playhead > 0) seek(playhead-1);
}

void Timeline::next_frame() {
	seek(playhead+1);
}

void Timeline::previous_cut() {
    if (playhead > 0) {
        long p_cut = 0;
        for (int i=0;i<sequence->clip_count();i++) {
            Clip* c = sequence->get_clip(i);
            if (c != NULL) {
                if (c->timeline_out > p_cut && c->timeline_out < playhead) {
                    p_cut = c->timeline_out;
                } else if (c->timeline_in > p_cut && c->timeline_in < playhead) {
                    p_cut = c->timeline_in;
                }
            }
        }
        seek(p_cut);
    }
}

void Timeline::next_cut() {
    bool seek_enabled = false;
    long n_cut = LONG_MAX;
    for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        if (c != NULL) {
            if (c->timeline_in < n_cut && c->timeline_in > playhead) {
                n_cut = c->timeline_in;
                seek_enabled = true;
            } else if (c->timeline_out < n_cut && c->timeline_out > playhead) {
                n_cut = c->timeline_out;
                seek_enabled = true;
            }
        }
    }
    if (seek_enabled) seek(n_cut);
}

void Timeline::reset_all_audio() {
    // reset all clip audio
	audio_ibuffer_frame = playhead;
	if (sequence != NULL) {
		for (int i=0;i<sequence->clip_count();i++) {
			Clip* c = sequence->get_clip(i);
			if (c != NULL) {
				c->reset_audio();
			}
		}
	}
	ui->audio_monitor->reset();
    clear_audio_ibuffer();
}

void Timeline::seek(long p) {
	pause();
	playhead = p;
	reset_all_audio();
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
	audio_thread->notifyReceiver();
}

void Timeline::pause() {
	playing = false;
    panel_viewer->set_playpause_icon(true);
	playback_updater.stop();
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

void Timeline::add_transition() {
	TimelineAction* ta = new TimelineAction();
	bool adding = false;

    for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        if (c != NULL && is_clip_selected(c, true)) {
            if (c->opening_transition == NULL) {
				ta->add_transition(sequence, i, 0, TA_OPENING_TRANSITION);
				adding = true;
            }
            if (c->closing_transition == NULL) {
				ta->add_transition(sequence, i, 0, TA_CLOSING_TRANSITION);
				adding = true;
            }
        }
    }

	if (adding) {
		undo_stack.push(ta);
	} else {
		delete ta;
	}

	redraw_all_clips(true);
}

int Timeline::calculate_track_height(int track, int value) {
    int index = (track < 0) ? qAbs(track + 1) : track;
    QVector<int>& vector = (track < 0) ? video_track_heights : audio_track_heights;
    while (vector.size() < index+1) {
		vector.append(default_track_height);
    }
    if (value > -1) {
        vector[index] = value;
    }
    return vector.at(index);
}

void Timeline::set_in_point() {
    ui->headers->set_in_point(playhead);
}

void Timeline::set_out_point() {
    ui->headers->set_out_point(playhead);
}

void Timeline::update_sequence() {
    bool null_sequence = (sequence == NULL);

	for (int i=0;i<tool_buttons.count();i++) {
		tool_buttons[i]->setEnabled(!null_sequence);
	}
    ui->snappingButton->setEnabled(!null_sequence);
	ui->pushButton_4->setEnabled(!null_sequence);
	ui->pushButton_5->setEnabled(!null_sequence);
	ui->addButton->setEnabled(!null_sequence);
    ui->headers->setEnabled(!null_sequence);

	if (null_sequence) {
		setWindowTitle("Timeline: <none>");
	} else {
		setWindowTitle("Timeline: " + sequence->name);
        reset_all_audio();
        redraw_all_clips(false);
        playback_updater.setInterval(qFloor(1000 / sequence->frame_rate));
	}
}

int Timeline::get_snap_range() {
	return getTimelineFrameFromScreenPoint(10);
}

bool Timeline::focused() {
	return (sequence != NULL && (ui->headers->hasFocus() || ui->video_area->hasFocus() || ui->audio_area->hasFocus()));
}

void Timeline::repaint_timeline() {
	if (playing) {
        playhead = round(playhead_start + ((QDateTime::currentMSecsSinceEpoch()-start_msecs) * 0.001 * sequence->frame_rate));
	}

	ui->headers->update_header(zoom);
	ui->video_area->update();
	ui->audio_area->update();
	panel_effect_controls->update_keyframes();

    if (last_frame != playhead) {
		panel_viewer->viewer_widget->update();
        ui->audio_monitor->update();
		last_frame = playhead;
	}
	panel_viewer->update_playhead_timecode(playhead);
}

void Timeline::redraw_all_clips(bool changed) {
	if (changed) {
        if (!playing) reset_all_audio();
        panel_viewer->viewer_widget->update();
    }

	ui->headers->update_header(zoom);
    ui->video_area->redraw_clips();
    ui->audio_area->redraw_clips();
    panel_effect_controls->update_keyframes();

    if (zoomChanged) {
        ui->audioScrollArea->update();
        ui->audioScrollArea->horizontalScrollBar()->setValue(getTimelineScreenPointFromFrame(playhead)-(ui->audioScrollArea->width()/2));
        zoomChanged = false;
    }

	panel_viewer->update_end_timecode();
}

void Timeline::select_all() {
	if (sequence != NULL) {
		selections.clear();
		for (int i=0;i<sequence->clip_count();i++) {
			Clip* c = sequence->get_clip(i);
			if (c != NULL) {
				Selection s;
				s.in = c->timeline_in;
				s.out = c->timeline_out;
				s.track = c->track;
				selections.append(s);
			}
		}
		repaint_timeline();
	}
}

void Timeline::delete_in_out(bool ripple) {
    if (sequence != NULL && sequence->using_workarea) {
        QVector<Selection> areas;
        int video_tracks = 0, audio_tracks = 0;
        sequence->get_track_limits(&video_tracks, &audio_tracks);
        for (int i=video_tracks;i<=audio_tracks;i++) {
            Selection s;
            s.in = sequence->workarea_in;
            s.out = sequence->workarea_out;
            s.track = i;
            areas.append(s);
        }
        TimelineAction* ta = new TimelineAction();
		delete_areas_and_relink(ta, areas);
        if (ripple) ta->ripple(sequence, sequence->workarea_in, sequence->workarea_in - sequence->workarea_out);
        ta->set_in_out(sequence, false, 0, 0);
        undo_stack.push(ta);
        redraw_all_clips(true);
    }
}

void Timeline::delete_selection(bool ripple_delete) {
	if (selections.size() > 0) {
        panel_effect_controls->clear_effects(true);

        TimelineAction* ta = new TimelineAction();

        delete_areas_and_relink(ta, selections);

        if (ripple_delete) {
            long ripple_point = selections.at(0).in;
            long ripple_length = selections.at(0).out - selections.at(0).in;

            // retrieve ripple_point and ripple_length from current selection
            for (int i=1;i<selections.size();i++) {
                const Selection& s = selections.at(i);
                ripple_point = qMin(ripple_point, s.in);
                ripple_length = qMin(ripple_length, s.out - s.in);
            }

            // it feels a little more intuitive with this here
            ripple_point++;

            bool can_ripple = true;
			for (int i=0;i<sequence->clip_count();i++) {
                Clip* c = sequence->get_clip(i);
                if (c->timeline_in < ripple_point && c->timeline_out > ripple_point) {
                    // conflict detected, but this clip may be getting deleted so let's check
                    bool deleted = false;
                    for (int j=0;j<selections.size();j++) {
                        const Selection& s = selections.at(j);
                        if (s.track == c->track
                                && !(c->timeline_in < s.in && c->timeline_out < s.in)
                                && !(c->timeline_in > s.out && c->timeline_out > s.out)) {
                            deleted = true;
                            break;
                        }
                    }
                    if (!deleted) {
                        for (int j=0;j<sequence->clip_count();j++) {
                            Clip* cc = sequence->get_clip(j);
                            if (cc->track == c->track
                                    && cc->timeline_in > c->timeline_out
                                    && cc->timeline_in < c->timeline_out + ripple_length) {
                                ripple_length = cc->timeline_in - c->timeline_out;
                            }
                        }
                    }
                }
			}
            if (can_ripple) ta->ripple(sequence, ripple_point, -ripple_length);
		}

        selections.clear();

        undo_stack.push(ta);

        redraw_all_clips(true);
	}
}

int lerp(int a, int b, double t) {
	return ((1.0 - t) * a) + (t * b);
}

void Timeline::set_zoom(bool in) {
    if (in) {
        zoom *= 2;
    } else {
        zoom /= 2;
    }
    zoomChanged = true;
    redraw_all_clips(false);
}

void Timeline::decheck_tool_buttons(QObject* sender) {
	for (int i=0;i<tool_buttons.count();i++) {
        tool_buttons[i]->setChecked(tool_buttons.at(i) == sender);
	}
}

QVector<int> Timeline::get_tracks_of_linked_clips(int i) {
    QVector<int> tracks;
    Clip* clip = sequence->get_clip(i);
    for (int j=0;j<clip->linked.size();j++) {
        tracks.append(sequence->get_clip(clip->linked.at(j))->track);
    }
    return tracks;
}

void Timeline::on_pushButton_4_clicked()
{
    set_zoom(true);
}

void Timeline::on_pushButton_5_clicked()
{
    set_zoom(false);
}

bool Timeline::is_clip_selected(Clip* clip, bool containing) {
	for (int i=0;i<selections.size();i++) {
		const Selection& s = selections.at(i);
        if (clip->track == s.track && ((clip->timeline_in >= s.in && clip->timeline_out <= s.out && containing) ||
                (!containing && !(clip->timeline_in < s.in && clip->timeline_out < s.in) && !(clip->timeline_in > s.in && clip->timeline_out > s.in)))) {
			return true;
		}
	}
	return false;
}

void Timeline::on_snappingButton_toggled(bool checked) {
    snapping = checked;
}

Clip* Timeline::split_clip(TimelineAction* ta, int p, long frame) {
	return split_clip(ta, p, frame, frame);
}

Clip* Timeline::split_clip(TimelineAction* ta, int p, long frame, long post_in) {
    Clip* pre = sequence->get_clip(p);
    if (pre != NULL && pre->timeline_in < frame && pre->timeline_out > frame) { // guard against attempts to split at in/out points
        Clip* post = pre->copy(sequence);

		long new_clip_length = frame - pre->timeline_in;

		post->timeline_in = post_in;
		post->clip_in = pre->clip_in + (post->timeline_in - pre->timeline_in);

		ta->set_timeline_out(sequence, p, frame);

		if (pre->opening_transition != NULL) {
			if (pre->opening_transition->length > new_clip_length) {
				ta->modify_transition(sequence, p, TA_OPENING_TRANSITION, new_clip_length);
			}

			delete post->opening_transition;
			post->opening_transition = NULL;
		}
		if (pre->closing_transition != NULL) {
			ta->delete_transition(sequence, p, TA_CLOSING_TRANSITION);

			post->closing_transition->length = qMin((long) post->closing_transition->length, post->getLength());
		}

        return post;
    }
    return NULL;
}

bool Timeline::has_clip_been_split(int c) {
    for (int i=0;i<split_cache.size();i++) {
        if (split_cache.at(i) == c) {
            return true;
        }
    }
    return false;
}

bool Timeline::split_clip_and_relink(TimelineAction* ta, int clip, long frame, bool relink) {
    // see if we split this clip before
    if (has_clip_been_split(clip)) {
        return false;
    }

    split_cache.append(clip);

    Clip* c = sequence->get_clip(clip);
    if (c != NULL) {
        QVector<int> pre_clips;
        QVector<Clip*> post_clips;

        Clip* post = split_clip(ta, clip, frame);

        // if alt is not down, split clips links too
        if (post == NULL) {
            return false;
        } else {
            post_clips.append(post);
            if (relink) {
                pre_clips.append(clip);

                bool original_clip_is_selected = is_clip_selected(c, true);

                // find linked clips of old clip
                for (int i=0;i<c->linked.size();i++) {
                    int l = c->linked.at(i);
                    if (!has_clip_been_split(l)) {
                        Clip* link = sequence->get_clip(l);
                        if ((original_clip_is_selected && is_clip_selected(link, true)) || !original_clip_is_selected) {
                            split_cache.append(l);
                            Clip* s = split_clip(ta, l, frame);
                            if (s != NULL) {
                                pre_clips.append(l);
                                post_clips.append(s);
                            }
                        }
                    }
                }

                relink_clips_using_ids(pre_clips, post_clips);
            }
            ta->add_clips(sequence, post_clips);
            return true;
        }
    }
    return false;
}

void Timeline::clean_up_selections(QVector<Selection>& areas) {
    for (int i=0;i<areas.size();i++) {
        Selection& s = areas[i];
        for (int j=0;j<areas.size();j++) {
            if (i != j) {
                Selection& ss = areas[j];
                if (s.track == ss.track) {
                    bool remove = false;
                    if (s.in < ss.in && s.out > ss.out) {
                        // do nothing
                    } else if (s.in >= ss.in && s.out <= ss.out) {
                        remove = true;
                    } else if (s.in <= ss.out && s.out > ss.out) {
                        ss.out = s.out;
                        remove = true;
                    } else if (s.out >= ss.in && s.in < ss.in) {
                        ss.in = s.in;
                        remove = true;
                    }
                    if (remove) {
                        areas.removeAt(i);
                        i--;
                        break;
                    }
                }
            }
        }
    }
}

void Timeline::delete_areas_and_relink(TimelineAction* ta, QVector<Selection>& areas) {
    clean_up_selections(areas);

    QVector<int> pre_clips;
    QVector<Clip*> post_clips;

    for (int i=0;i<areas.size();i++) {
        const Selection& s = areas.at(i);
        for (int j=0;j<sequence->clip_count();j++) {
            Clip* c = sequence->get_clip(j);
            if (c != NULL && c->track == s.track && !c->undeletable) {
				if (c->opening_transition != NULL && s.in == c->timeline_in && s.out == c->timeline_in + c->opening_transition->length) {
					// delete opening transition
					ta->delete_transition(sequence, j, TA_OPENING_TRANSITION);
				} else if (c->closing_transition != NULL && s.out == c->timeline_out && s.in == c->timeline_out - c->closing_transition->length) {
					// delete closing transition
					ta->delete_transition(sequence, j, TA_CLOSING_TRANSITION);
				} else if (c->timeline_in >= s.in && c->timeline_out <= s.out) {
                    // clips falls entirely within deletion area
                    ta->delete_clip(sequence, j);
                } else if (c->timeline_in < s.in && c->timeline_out > s.out) {
                    // middle of clip is within deletion area

                    // duplicate clip
					Clip* post = split_clip(ta, j, s.in, s.out);

					pre_clips.append(j);
					post_clips.append(post);
                } else if (c->timeline_in < s.in && c->timeline_out > s.in) {
                    // only out point is in deletion area
                    ta->set_timeline_out(sequence, j, s.in);

					if (c->closing_transition != NULL) {
						if (s.in < c->timeline_out - c->closing_transition->length) {
							ta->delete_transition(sequence, j, TA_CLOSING_TRANSITION);
						} else {
							ta->modify_transition(sequence, j, TA_CLOSING_TRANSITION, c->closing_transition->length - (c->timeline_out - s.in));
						}
					}
                } else if (c->timeline_in < s.out && c->timeline_out > s.out) {
                    // only in point is in deletion area
                    ta->increase_clip_in(sequence, j, s.out - c->timeline_in);
                    ta->set_timeline_in(sequence, j, s.out);

					if (c->opening_transition != NULL) {
						if (s.out > c->timeline_in + c->opening_transition->length) {
							ta->delete_transition(sequence, j, TA_OPENING_TRANSITION);
						} else {
							ta->modify_transition(sequence, j, TA_OPENING_TRANSITION, c->opening_transition->length - (s.out - c->timeline_in));
						}
					}
                }
            }
        }
    }
    relink_clips_using_ids(pre_clips, post_clips);
    ta->add_clips(sequence, post_clips);
}

void Timeline::copy(bool del) {
    bool cleared = false;
    bool copied = false;

    long min_in = 0;

    for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        if (c != NULL) {
            for (int j=0;j<selections.size();j++) {
                const Selection& s = selections.at(j);
                if (s.track == c->track && !((c->timeline_in <= s.in && c->timeline_out <= s.in) || (c->timeline_in >= s.out && c->timeline_out >= s.out))) {
                    if (!cleared) {
                        clip_clipboard.clear();
                        cleared = true;
                    }

                    Clip* copied_clip = c->copy(NULL);

                    // copy linked IDs (we correct these later in paste())
                    copied_clip->linked = c->linked;

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

                    copied_clip->load_id = i;

                    clip_clipboard.append(copied_clip);
                }
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

void Timeline::relink_clips_using_ids(QVector<int>& old_clips, QVector<Clip*>& new_clips) {
    // relink pasted clips
    for (int i=0;i<old_clips.size();i++) {
        // these indices should correspond
        Clip* oc = sequence->get_clip(old_clips.at(i));
        for (int j=0;j<oc->linked.size();j++) {
            for (int k=0;k<old_clips.size();k++) { // find clip with that ID
                if (oc->linked.at(j) == old_clips.at(k)) {
                    new_clips.at(i)->linked.append(k);
                }
            }
        }
    }
}

void Timeline::paste() {
    if (clip_clipboard.size() > 0) {
        TimelineAction* ta = new TimelineAction();

        // create copies and delete areas that we'll be pasting to
        QVector<Selection> delete_areas;
        QVector<Clip*> pasted_clips;
        long paste_end = 0;
        for (int i=0;i<clip_clipboard.size();i++) {
            Clip* c = clip_clipboard.at(i);

            // create copy of clip and offset by playhead
            Clip* cc = c->copy(sequence);
            cc->timeline_in += playhead;
            cc->timeline_out += playhead;
            cc->track = c->track;
            if (cc->timeline_out > paste_end) paste_end = cc->timeline_out;
            cc->sequence = sequence;
            pasted_clips.append(cc);

            Selection s;
            s.in = cc->timeline_in;
            s.out = cc->timeline_out;
            s.track = c->track;
            delete_areas.append(s);
        }
        delete_areas_and_relink(ta, delete_areas);

        // ADAPT
        for (int i=0;i<clip_clipboard.size();i++) {
            // these indices should correspond
            Clip* oc = clip_clipboard.at(i);

            for (int j=0;j<oc->linked.size();j++) {
                for (int k=0;k<clip_clipboard.size();k++) { // find clip with that ID
                    if (clip_clipboard.at(k)->load_id == oc->linked.at(j)) {
                        pasted_clips.at(i)->linked.append(k);
                    }
                }
            }
        }
        // ADAPT

        ta->add_clips(sequence, pasted_clips);

        undo_stack.push(ta);

        redraw_all_clips(true);

        if (config.paste_seeks) {
            seek(paste_end);
        }
    }
}

void Timeline::ripple_to_in_point(bool in) {
    if (sequence != NULL) {
        // get track count
        int track_min = 0;
        int track_max = 0;

        bool one_frame_mode = false;

        // find closest in point to playhead
        long in_point = in ? 0 : LONG_MAX;
        for (int i=0;i<sequence->clip_count();i++) {
            Clip* c = sequence->get_clip(i);
            track_min = qMin(track_min, c->track);
            track_max = qMax(track_max, c->track);
            if ((in && c->timeline_in > in_point && c->timeline_in <= playhead)
                    || (!in && c->timeline_in < in_point && c->timeline_in >= playhead)) {
                in_point = c->timeline_in;
                if (playhead == in_point) {
                    one_frame_mode = true;
                    break;
                }
            }
            if ((in && c->timeline_out > in_point && c->timeline_out <= playhead)
                    || (!in && c->timeline_out < in_point && c->timeline_out > playhead)) {
                in_point = c->timeline_out;
                if (playhead == in_point) {
                    one_frame_mode = true;
                    break;
                }
            }
        }

        if (one_frame_mode) {

        } else {
            // set up deletion areas based on track count
            QVector<Selection> areas;
            for (int i=track_min;i<=track_max;i++) {
                Selection s;
                s.in = qMin(in_point, playhead);
                s.out = qMax(in_point, playhead);
                s.track = i;
                areas.append(s);
            }

            // trim and move clips around the in point
            TimelineAction* ta = new TimelineAction();
            delete_areas_and_relink(ta, areas);
            ta->ripple(sequence, in_point, (in) ? (in_point - playhead) : (playhead - in_point));
            undo_stack.push(ta);

            redraw_all_clips(true);

            if (in) seek(in_point);
        }
    }
}

bool Timeline::split_selection(TimelineAction* ta) {
    bool split = false;

    // temporary relinking vectors
    QVector<int> pre_splits;
    QVector<Clip*> post_splits;
    QVector<Clip*> secondary_post_splits;

    // find clips within selection and split
    for (int j=0;j<sequence->clip_count();j++) {
        Clip* clip = sequence->get_clip(j);
        if (clip != NULL) {
            for (int i=0;i<selections.size();i++) {
                const Selection& s = selections.at(i);
                if (s.track == clip->track) {
					if (clip->timeline_in < s.in && clip->timeline_out > s.out) {
                        Clip* split_A = clip->copy(sequence);
                        split_A->clip_in += (s.in - clip->timeline_in);
                        split_A->timeline_in = s.in;
                        split_A->timeline_out = s.out;
                        pre_splits.append(j);
						post_splits.append(split_A);

                        Clip* split_B = clip->copy(sequence);
                        split_B->clip_in += (s.out - clip->timeline_in);
                        split_B->timeline_in = s.out;
                        secondary_post_splits.append(split_B);

						if (clip->opening_transition != NULL) {
							delete split_B->opening_transition;
							split_B->opening_transition = NULL;

							delete split_A->opening_transition;
							split_A->opening_transition = NULL;
						}

						if (clip->closing_transition != NULL) {
							ta->delete_transition(sequence, j, TA_CLOSING_TRANSITION);

							delete split_A->closing_transition;
							split_A->closing_transition = NULL;
						}

                        ta->set_timeline_out(sequence, j, s.in);
                        split = true;
                    } else {
                        Clip* post_a = split_clip(ta, j, s.in);
                        Clip* post_b = split_clip(ta, j, s.out);
                        if (post_a != NULL) {
                            pre_splits.append(j);
                            post_splits.append(post_a);
                            split = true;
                        }
                        if (post_b != NULL) {
                            if (post_a != NULL) {
                                pre_splits.append(j);
                                post_splits.append(post_b);
                            } else {
                                secondary_post_splits.append(post_b);
                            }
                            split = true;
                        }
                    }
                }
            }
        }
    }

    if (split) {
        // relink after splitting
        relink_clips_using_ids(pre_splits, post_splits);
        relink_clips_using_ids(pre_splits, secondary_post_splits);
        ta->add_clips(sequence, post_splits);
        ta->add_clips(sequence, secondary_post_splits);

        return true;
    }
    return false;
}

void Timeline::split_at_playhead() {
    TimelineAction* ta = new TimelineAction();
    bool split_selected = false;
    split_cache.clear();

    if (selections.size() > 0) {
        // see if whole clips are selected
        QVector<int> pre_clips;
        QVector<Clip*> post_clips;
        for (int j=0;j<sequence->clip_count();j++) {
            Clip* clip = sequence->get_clip(j);
            if (clip != NULL && is_clip_selected(clip, true)) {
                Clip* s = split_clip(ta, j, playhead);
                if (s != NULL) {
                    pre_clips.append(j);
                    post_clips.append(s);
                    split_selected = true;
                }
            }
        }

        if (split_selected) {
            // relink clips if we split
            relink_clips_using_ids(pre_clips, post_clips);
            ta->add_clips(sequence, post_clips);
        } else {
            // split a selection if not
            split_selected = split_selection(ta);
        }
    }

    // if nothing was selected or no selections fell within playhead, simply split at playhead
    if (!split_selected) {
        for (int j=0;j<sequence->clip_count();j++) {
            Clip* c = sequence->get_clip(j);
            if (c != NULL) {
                // always relinks
                if (split_clip_and_relink(ta, j, playhead, true)) {
                    split_selected = true;
                }
            }
        }
    }

    if (split_selected) {
        undo_stack.push(ta);
        redraw_all_clips(true);
    } else {
        delete ta;
    }
}

void Timeline::deselect_area(long in, long out, int track) {
	int len = selections.size();
	for (int i=0;i<len;i++) {
		Selection& s = selections[i];
		if (s.track == track) {
			if (s.in >= in && s.out <= out) {
				// whole selection is in deselect area
				selections.removeAt(i);
				i--;
				len--;
			} else if (s.in < in && s.out > out) {
				// middle of selection is in deselect area
				Selection new_sel;
				new_sel.in = out;
				new_sel.out = s.out;
				new_sel.track = s.track;
				selections.append(new_sel);

				s.out = in;
			} else if (s.in < in && s.out > in) {
				// only out point is in deselect area
				s.out = in;
			} else if (s.in < out && s.out > out) {
				// only in point is in deselect area
				s.in = out;
			}
		}
	}
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
        if (playhead_inclusive && !playing) {
            playhead_inclusive = snap_to_point(playhead, l);
        } else {
            playhead_inclusive = false;
        }
        if (!playhead_inclusive) {
            for (int i=0;i<sequence->clip_count();i++) {
                Clip* c = sequence->get_clip(i);
                if (c != NULL) {
                    if (snap_to_point(c->timeline_in, l)) {
                        break;
                    } else if (snap_to_point(c->timeline_out, l)) {
                        break;
					} else if (c->opening_transition != NULL && snap_to_point(c->timeline_in + c->opening_transition->length, l)) {
						break;
					} else if (c->closing_transition != NULL && snap_to_point(c->timeline_out - c->closing_transition->length, l)) {
						break;
					}
                }
            }
        }
    }
}

void Timeline::toggle_links() {
    LinkCommand* command = new LinkCommand();
    for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        if (c != NULL && is_clip_selected(c, true)) {
            command->clips.append(c);
            if (c->linked.size() > 0) {
                command->link = false; // prioritize unlinking

                for (int j=0;j<c->linked.size();j++) { // add links to the command
                    bool found = false;
                    Clip* link = sequence->get_clip(c->linked.at(j));
                    for (int k=0;k<command->clips.size();k++) {
                        if (command->clips.at(k) == link) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        command->clips.append(link);
                    }
                }
            }
        }
    }
    if (command->clips.size() > 0) {
        undo_stack.push(command);
        redraw_all_clips(true);
    } else {
        delete command;
    }
}

void Timeline::increase_track_height() {
    for (int i=0;i<video_track_heights.size();i++) {
        video_track_heights[i] += TRACK_HEIGHT_INCREMENT;
    }
    for (int i=0;i<audio_track_heights.size();i++) {
        audio_track_heights[i] += TRACK_HEIGHT_INCREMENT;
    }
    redraw_all_clips(false);
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
    redraw_all_clips(false);
}

void Timeline::deselect() {
    selections.clear();
    repaint_timeline();
}

long getFrameFromScreenPoint(double zoom, int x) {
	long f = round((float) x / zoom);
	if (f < 0) {
		return 0;
	}
	return f;
}

int getScreenPointFromFrame(double zoom, long frame) {
	return (int) round(frame*zoom);
}

long Timeline::getTimelineFrameFromScreenPoint(int x) {
	return getFrameFromScreenPoint(zoom, x);
}

int Timeline::getTimelineScreenPointFromFrame(long frame) {
	return getScreenPointFromFrame(zoom, frame);
}

void Timeline::on_toolArrowButton_clicked() {
    decheck_tool_buttons(sender());
    ui->timeline_area->setCursor(Qt::ArrowCursor);
    tool = TIMELINE_TOOL_POINTER;
}

void Timeline::on_toolEditButton_clicked() {
    decheck_tool_buttons(sender());
    ui->timeline_area->setCursor(Qt::IBeamCursor);
    tool = TIMELINE_TOOL_EDIT;
}

void Timeline::on_toolRippleButton_clicked() {
    decheck_tool_buttons(sender());
    ui->timeline_area->setCursor(Qt::ArrowCursor);
    tool = TIMELINE_TOOL_RIPPLE;
}

void Timeline::on_toolRollingButton_clicked() {
    decheck_tool_buttons(sender());
    ui->timeline_area->setCursor(Qt::ArrowCursor);
    tool = TIMELINE_TOOL_ROLLING;
}

void Timeline::on_toolRazorButton_clicked()
{
    decheck_tool_buttons(sender());
    ui->timeline_area->setCursor(Qt::IBeamCursor);
    tool = TIMELINE_TOOL_RAZOR;
}

void Timeline::on_toolSlipButton_clicked()
{
    decheck_tool_buttons(sender());
    ui->timeline_area->setCursor(Qt::ArrowCursor);
    tool = TIMELINE_TOOL_SLIP;
}

void Timeline::on_toolSlideButton_clicked()
{
    decheck_tool_buttons(sender());
    ui->timeline_area->setCursor(Qt::ArrowCursor);
    tool = TIMELINE_TOOL_SLIDE;
}

void Timeline::on_addButton_clicked() {
	QMenu add_menu(this);

	QAction* titleMenuItem = new QAction(&add_menu);
	titleMenuItem->setText("Title...");
	titleMenuItem->setData(ADD_OBJ_TITLE);
	add_menu.addAction(titleMenuItem);

	QAction* solidMenuItem = new QAction(&add_menu);
	solidMenuItem->setText("Solid Color...");
	solidMenuItem->setData(ADD_OBJ_SOLID);
	add_menu.addAction(solidMenuItem);

	QAction* barsMenuItem = new QAction(&add_menu);
	barsMenuItem->setText("Bars...");
	barsMenuItem->setData(ADD_OBJ_BARS);
	add_menu.addAction(barsMenuItem);

	QAction* toneMenuItem = new QAction(&add_menu);
	toneMenuItem->setText("Tone...");
	toneMenuItem->setData(ADD_OBJ_TONE);
	add_menu.addAction(toneMenuItem);

	connect(&add_menu, SIGNAL(triggered(QAction*)), this, SLOT(addMenuItem(QAction*)));

	add_menu.exec(QCursor::pos());
}

void Timeline::addMenuItem(QAction* action) {
	creating = true;
	creatingObject = action->data().toInt();
}
