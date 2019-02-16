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

#include "timeline.h"

#include "oliveglobal.h"

#include "panels/panels.h"
#include "project/projectelements.h"

#include "ui/timelinewidget.h"
#include "ui/viewerwidget.h"
#include "playback/audio.h"
#include "playback/cacher.h"
#include "playback/playback.h"
#include "io/config.h"
#include "io/clipboard.h"
#include "ui/timelineheader.h"
#include "ui/resizablescrollbar.h"
#include "ui/audiomonitor.h"
#include "ui/flowlayout.h"
#include "mainwindow.h"
#include "debug.h"

#include <QTime>
#include <QScrollBar>
#include <QtMath>
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSplitter>
#include <QStatusBar>

Timeline::Timeline(QWidget *parent) :
	QDockWidget(parent),
	cursor_frame(0),
	cursor_track(0),
	zoom(1.0),
	zoom_just_changed(false),
	showing_all(false),
	snapping(true),
	snapped(false),
	snap_point(0),
	selecting(false),
	rect_select_init(false),
	rect_select_proc(false),
	moving_init(false),
	moving_proc(false),
	move_insert(false),
	trim_target(-1),
	trim_in_point(false),
	splitting(false),
	importing(false),
	importing_files(false),
	creating(false),
	transition_tool_init(false),
	transition_tool_proc(false),
	transition_tool_pre_clip(-1),
	transition_tool_post_clip(-1),
	hand_moving(false),
	block_repaints(false),
	scroll(0)
{
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	setup_ui();

	default_track_height = qRound((QGuiApplication::primaryScreen()->logicalDotsPerInch() / 96) * TRACK_DEFAULT_HEIGHT);

	headers->viewer = panel_sequence_viewer;

	video_area->bottom_align = true;
	video_area->scrollBar = videoScrollbar;
	audio_area->scrollBar = audioScrollbar;

	tool_buttons.append(toolArrowButton);
	tool_buttons.append(toolEditButton);
	tool_buttons.append(toolRippleButton);
	tool_buttons.append(toolRazorButton);
	tool_buttons.append(toolSlipButton);
	tool_buttons.append(toolSlideButton);
	tool_buttons.append(toolTransitionButton);
	tool_buttons.append(toolHandButton);

	toolArrowButton->click();

	connect(horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(setScroll(int)));
	connect(videoScrollbar, SIGNAL(valueChanged(int)), video_area, SLOT(setScroll(int)));
	connect(audioScrollbar, SIGNAL(valueChanged(int)), audio_area, SLOT(setScroll(int)));
	connect(horizontalScrollBar, SIGNAL(resize_move(double)), this, SLOT(resize_move(double)));

	update_sequence();
}

Timeline::~Timeline() {}

void Timeline::previous_cut() {
    if (olive::ActiveSequence != nullptr
            && olive::ActiveSequence->playhead > 0) {
		long p_cut = 0;
		for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
			Clip* c = olive::ActiveSequence->clips.at(i);
			if (c != nullptr) {
				if (c->timeline_out > p_cut && c->timeline_out < olive::ActiveSequence->playhead) {
					p_cut = c->timeline_out;
				} else if (c->timeline_in > p_cut && c->timeline_in < olive::ActiveSequence->playhead) {
					p_cut = c->timeline_in;
				}
			}
		}
		panel_sequence_viewer->seek(p_cut);
	}
}

void Timeline::next_cut() {
    if (olive::ActiveSequence != nullptr) {
        bool seek_enabled = false;
        long n_cut = LONG_MAX;
        for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
            Clip* c = olive::ActiveSequence->clips.at(i);
            if (c != nullptr) {
                if (c->timeline_in < n_cut && c->timeline_in > olive::ActiveSequence->playhead) {
                    n_cut = c->timeline_in;
                    seek_enabled = true;
                } else if (c->timeline_out < n_cut && c->timeline_out > olive::ActiveSequence->playhead) {
                    n_cut = c->timeline_out;
                    seek_enabled = true;
                }
            }
        }
        if (seek_enabled) panel_sequence_viewer->seek(n_cut);
    }
}

void ripple_clips(ComboAction* ca, Sequence *s, long point, long length, const QVector<int>& ignore) {
	ca->append(new RippleAction(s, point, length, ignore));
}

void Timeline::toggle_show_all() {
	if (olive::ActiveSequence != nullptr) {
		showing_all = !showing_all;
		if (showing_all) {
			old_zoom = zoom;
			set_zoom_value(double(timeline_area->width() - 200) / double(olive::ActiveSequence->getEndFrame()));
		} else {
			set_zoom_value(old_zoom);
		}
	}
}

void Timeline::create_ghosts_from_media(Sequence* seq, long entry_point, QVector<Media*>& media_list) {
	video_ghosts = false;
	audio_ghosts = false;

	for (int i=0;i<media_list.size();i++) {
		bool can_import = true;

		Media* medium = media_list.at(i);
		Footage* m = nullptr;
		Sequence* s = nullptr;
		long sequence_length = 0;
		long default_clip_in = 0;
		long default_clip_out = 0;

		switch (medium->get_type()) {
		case MEDIA_TYPE_FOOTAGE:
			m = medium->to_footage();
			can_import = m->ready;
			if (m->using_inout) {
				double source_fr = 30;
				if (m->video_tracks.size() > 0 && !qIsNull(m->video_tracks.at(0).video_frame_rate)) source_fr = m->video_tracks.at(0).video_frame_rate * m->speed;
				default_clip_in = refactor_frame_number(m->in, source_fr, seq->frame_rate);
				default_clip_out = refactor_frame_number(m->out, source_fr, seq->frame_rate);
			}
			break;
		case MEDIA_TYPE_SEQUENCE:
			s = medium->to_sequence();
			sequence_length = s->getEndFrame();
			if (seq != nullptr) sequence_length = refactor_frame_number(sequence_length, s->frame_rate, seq->frame_rate);
			can_import = (s != seq && sequence_length != 0);
			if (s->using_workarea) {
				default_clip_in = refactor_frame_number(s->workarea_in, s->frame_rate, seq->frame_rate);
				default_clip_out = refactor_frame_number(s->workarea_out, s->frame_rate, seq->frame_rate);
			}
			break;
		default:
			can_import = false;
		}

		if (can_import) {
			Ghost g;
			g.clip = -1;
			g.trimming = false;
			g.old_clip_in = g.clip_in = default_clip_in;
			g.media = medium;
			g.in = entry_point;
			g.transition = nullptr;

			switch (medium->get_type()) {
			case MEDIA_TYPE_FOOTAGE:
				// is video source a still image?
				if (m->video_tracks.size() > 0 && m->video_tracks.at(0).infinite_length && m->audio_tracks.size() == 0) {
					g.out = g.in + 100;
				} else {
					long length = m->get_length_in_frames(seq->frame_rate);
					g.out = entry_point + length - default_clip_in;
					if (m->using_inout) {
						g.out -= (length - default_clip_out);
					}
				}

				for (int j=0;j<m->audio_tracks.size();j++) {
					if (m->audio_tracks.at(j).enabled) {
						g.track = j;
						g.media_stream = m->audio_tracks.at(j).file_index;
						ghosts.append(g);
						audio_ghosts = true;
					}
				}
				for (int j=0;j<m->video_tracks.size();j++) {
					if (m->video_tracks.at(j).enabled) {
						g.track = -1-j;
						g.media_stream = m->video_tracks.at(j).file_index;
						ghosts.append(g);
						video_ghosts = true;
					}
				}
				break;
			case MEDIA_TYPE_SEQUENCE:
				g.out = entry_point + sequence_length - default_clip_in;

				if (s->using_workarea) {
					g.out -= (sequence_length - default_clip_out);
				}

				g.track = -1;
				ghosts.append(g);
				g.track = 0;
				ghosts.append(g);

				video_ghosts = true;
				audio_ghosts = true;
				break;
			}
			entry_point = g.out;
		}
	}
	for (int i=0;i<ghosts.size();i++) {
		Ghost& g = ghosts[i];
		g.old_in = g.in;
		g.old_out = g.out;
		g.old_track = g.track;
	}
}

void Timeline::add_clips_from_ghosts(ComboAction* ca, Sequence* s) {
	// add clips
	long earliest_point = LONG_MAX;
	QVector<Clip*> added_clips;
	for (int i=0;i<ghosts.size();i++) {
		const Ghost& g = ghosts.at(i);

		earliest_point = qMin(earliest_point, g.in);

		Clip* c = new Clip(s);
		c->media = g.media;
		c->media_stream = g.media_stream;
		c->timeline_in = g.in;
		c->timeline_out = g.out;
		c->clip_in = g.clip_in;
		c->track = g.track;
		if (c->media->get_type() == MEDIA_TYPE_FOOTAGE) {
			Footage* m = c->media->to_footage();
			if (m->video_tracks.size() == 0) {
				// audio only (greenish)
				c->color_r = 128;
				c->color_g = 192;
				c->color_b = 128;
			} else if (m->audio_tracks.size() == 0) {
				// video only (orangeish)
				c->color_r = 192;
				c->color_g = 160;
				c->color_b = 128;
			} else {
				// video and audio (blueish)
				c->color_r = 128;
				c->color_g = 128;
				c->color_b = 192;
			}
			c->name = m->name;
		} else if (c->media->get_type() == MEDIA_TYPE_SEQUENCE) {
			// sequence (red?ish?)
			c->color_r = 192;
			c->color_g = 128;
			c->color_b = 128;

			Sequence* media = c->media->to_sequence();
			c->name = media->name;
		}
		c->recalculateMaxLength();
		added_clips.append(c);
	}
	ca->append(new AddClipCommand(s, added_clips));

	// link clips from the same media
	for (int i=0;i<added_clips.size();i++) {
		Clip* c = added_clips.at(i);
		for (int j=0;j<added_clips.size();j++) {
			Clip* cc = added_clips.at(j);
			if (c != cc && c->media == cc->media) {
				c->linked.append(j);
			}
		}

        if (olive::CurrentConfig.add_default_effects_to_clips) {
            if (c->track < 0) {
                // add default video effects
                c->effects.append(create_effect(c, get_internal_meta(EFFECT_INTERNAL_TRANSFORM, EFFECT_TYPE_EFFECT)));
            } else {
                // add default audio effects
                c->effects.append(create_effect(c, get_internal_meta(EFFECT_INTERNAL_VOLUME, EFFECT_TYPE_EFFECT)));
                c->effects.append(create_effect(c, get_internal_meta(EFFECT_INTERNAL_PAN, EFFECT_TYPE_EFFECT)));
            }
        }
	}
	if (olive::CurrentConfig.enable_seek_to_import) {
		panel_sequence_viewer->seek(earliest_point);
	}
    ghosts.clear();
    importing = false;
    snapped = false;
}

int Timeline::get_track_height_size(bool video) {
	if (video) {
		return video_track_heights.size();
	} else {
		return audio_track_heights.size();
	}
}

void Timeline::add_transition() {
	ComboAction* ca = new ComboAction();
	bool adding = false;

	for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
		Clip* c = olive::ActiveSequence->clips.at(i);
		if (c != nullptr && is_clip_selected(c, true)) {
            int transition_to_add = (c->track < 0) ? TRANSITION_INTERNAL_CROSSDISSOLVE : TRANSITION_INTERNAL_LINEARFADE;
			if (c->get_opening_transition() == nullptr) {
                ca->append(new AddTransitionCommand(c, nullptr, nullptr, get_internal_meta(transition_to_add, EFFECT_TYPE_TRANSITION), TA_OPENING_TRANSITION, 30));
				adding = true;
			}
			if (c->get_closing_transition() == nullptr) {
                ca->append(new AddTransitionCommand(c, nullptr, nullptr, get_internal_meta(transition_to_add, EFFECT_TYPE_TRANSITION), TA_CLOSING_TRANSITION, 30));
				adding = true;
			}
		}
	}

	if (adding) {
        olive::UndoStack.push(ca);
	} else {
		delete ca;
	}

    update_ui(true);
}

void Timeline::nest() {
    if (olive::ActiveSequence != nullptr) {
        QVector<int> selected_clips;
        long earliest_point = LONG_MAX;

        // get selected clips
        for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
            Clip* c = olive::ActiveSequence->clips.at(i);
            if (c != nullptr && is_clip_selected(c, true)) {
                selected_clips.append(i);
                earliest_point = qMin(c->timeline_in, earliest_point);
            }
        }

        // nest them
        if (!selected_clips.isEmpty()) {
            ComboAction* ca = new ComboAction();

            Sequence* s = new Sequence();

            // create "nest" sequence
            s->name = panel_project->get_next_sequence_name(tr("Nested Sequence"));
            s->width = olive::ActiveSequence->width;
            s->height = olive::ActiveSequence->height;
            s->frame_rate = olive::ActiveSequence->frame_rate;
            s->audio_frequency = olive::ActiveSequence->audio_frequency;
            s->audio_layout = olive::ActiveSequence->audio_layout;

            // copy all selected clips to the nest
            for (int i=0;i<selected_clips.size();i++) {
                // delete clip from old sequence
                ca->append(new DeleteClipAction(olive::ActiveSequence, selected_clips.at(i)));

                // copy to new
                Clip* copy = olive::ActiveSequence->clips.at(selected_clips.at(i))->copy(s);
                copy->timeline_in -= earliest_point;
                copy->timeline_out -= earliest_point;
                s->clips.append(copy);
            }

            // relink clips in new nested sequences
            relink_clips_using_ids(selected_clips, s->clips);

            // add sequence to project
            Media* m = panel_project->create_sequence_internal(ca, s, false, nullptr);

            // add nested sequence to active sequence
            QVector<Media*> media_list;
            media_list.append(m);
            create_ghosts_from_media(olive::ActiveSequence, earliest_point, media_list);
            add_clips_from_ghosts(ca, olive::ActiveSequence);

            panel_effect_controls->clear_effects(true);
            olive::ActiveSequence->selections.clear();

            olive::UndoStack.push(ca);

            update_ui(true);
        }
    }
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

void Timeline::update_sequence() {
	bool null_sequence = (olive::ActiveSequence == nullptr);

	for (int i=0;i<tool_buttons.count();i++) {
		tool_buttons[i]->setEnabled(!null_sequence);
	}
	snappingButton->setEnabled(!null_sequence);
	zoomInButton->setEnabled(!null_sequence);
	zoomOutButton->setEnabled(!null_sequence);
	recordButton->setEnabled(!null_sequence);
	addButton->setEnabled(!null_sequence);
	headers->setEnabled(!null_sequence);

	QString title = tr("Timeline: ");
	if (null_sequence) {
		setWindowTitle(title + tr("<none>"));
	} else {
		setWindowTitle(title + olive::ActiveSequence->name);
		update_ui(false);
	}
}

int Timeline::get_snap_range() {
	return getFrameFromScreenPoint(zoom, 10);
}

bool Timeline::focused() {
	return (olive::ActiveSequence != nullptr && (headers->hasFocus() || video_area->hasFocus() || audio_area->hasFocus()));
}

void Timeline::repaint_timeline() {
	if (!block_repaints) {
		bool draw = true;

		if (olive::ActiveSequence != nullptr
				&& !horizontalScrollBar->isSliderDown()
				&& !horizontalScrollBar->is_resizing()
				&& panel_sequence_viewer->playing
				&& !zoom_just_changed) {
			// auto scroll
			if (olive::CurrentConfig.autoscroll == AUTOSCROLL_PAGE_SCROLL) {
                int playhead_x = getTimelineScreenPointFromFrame(olive::ActiveSequence->playhead);
				if (playhead_x < 0 || playhead_x > (editAreas->width() - videoScrollbar->width())) {
					horizontalScrollBar->setValue(getScreenPointFromFrame(zoom, olive::ActiveSequence->playhead));
					draw = false;
				}
			} else if (olive::CurrentConfig.autoscroll == AUTOSCROLL_SMOOTH_SCROLL) {
				if (center_scroll_to_playhead(horizontalScrollBar, zoom, olive::ActiveSequence->playhead)) {
					draw = false;
				}
			}
		}

		if (draw) {
			headers->update();
			video_area->update();
			audio_area->update();

            if (olive::ActiveSequence != nullptr
                    && !zoom_just_changed) {
				set_sb_max();
			}
		}

        zoom_just_changed = false;
	}
}

void Timeline::select_all() {
	if (olive::ActiveSequence != nullptr) {
		olive::ActiveSequence->selections.clear();
		for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
			Clip* c = olive::ActiveSequence->clips.at(i);
			if (c != nullptr) {
				Selection s;
				s.in = c->timeline_in;
				s.out = c->timeline_out;
				s.track = c->track;
				olive::ActiveSequence->selections.append(s);
			}
		}
		repaint_timeline();
	}
}

void Timeline::scroll_to_frame(long frame) {
	scroll_to_frame_internal(horizontalScrollBar, frame, zoom, timeline_area->width());
}

void Timeline::select_from_playhead() {
	olive::ActiveSequence->selections.clear();
	for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
		Clip* c = olive::ActiveSequence->clips.at(i);
		if (c != nullptr
				&& c->timeline_in <= olive::ActiveSequence->playhead
				&& c->timeline_out > olive::ActiveSequence->playhead) {
			Selection s;
			s.in = c->timeline_in;
			s.out = c->timeline_out;
			s.track = c->track;
			olive::ActiveSequence->selections.append(s);
		}
	}
}

bool Timeline::can_ripple_empty_space(long frame, int track) {
	bool can_ripple_delete = true;
	bool at_end_of_sequence = true;
	rc_ripple_min = 0;
	rc_ripple_max = LONG_MAX;

	for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
		Clip* c = olive::ActiveSequence->clips.at(i);
		if (c != nullptr) {
			if (c->timeline_in > frame || c->timeline_out > frame) {
				at_end_of_sequence = false;
			}
			if (c->track == track) {
				if (c->timeline_in <= frame && c->timeline_out >= frame) {
					can_ripple_delete = false;
					break;
				} else if (c->timeline_out < frame) {
					rc_ripple_min = qMax(rc_ripple_min, c->timeline_out);
				} else if (c->timeline_in > frame) {
					rc_ripple_max = qMin(rc_ripple_max, c->timeline_in);
				}
			}
		}
	}

	return (can_ripple_delete && !at_end_of_sequence);
}

void Timeline::ripple_delete_empty_space() {
	QVector<Selection> sels;

	Selection s;
	s.in = rc_ripple_min;
	s.out = rc_ripple_max;
    s.track = cursor_track;

	sels.append(s);

    delete_selection(sels, true);
}

void Timeline::resizeEvent(QResizeEvent *) {
	// adjust maximum scrollbar
	if (olive::ActiveSequence != nullptr) set_sb_max();


	// resize tool button widget to its contents
	QList<QWidget*> tool_button_children = tool_button_widget->findChildren<QWidget*>();

    int horizontal_spacing = static_cast<FlowLayout*>(tool_button_widget->layout())->horizontalSpacing();
    int vertical_spacing = static_cast<FlowLayout*>(tool_button_widget->layout())->verticalSpacing();
    int total_area = tool_button_widget->height();

    int button_count = tool_button_children.size();
    int button_height = tool_button_children.at(0)->sizeHint().height() + vertical_spacing;

    int cols = 0;

    int col_height;

    if (button_height < total_area) {
        do {
            cols++;
            col_height = (qCeil(double(button_count)/double(cols))*button_height)-vertical_spacing;
        } while (col_height > total_area);
    } else {
        cols = button_count;
    }

	tool_button_widget->setFixedWidth((tool_button_children.at(0)->sizeHint().width())*cols + horizontal_spacing*(cols-1) + 1);
}

void Timeline::delete_in_out_internal(bool ripple) {
	if (olive::ActiveSequence != nullptr && olive::ActiveSequence->using_workarea) {
		QVector<Selection> areas;
		int video_tracks = 0, audio_tracks = 0;
		olive::ActiveSequence->getTrackLimits(&video_tracks, &audio_tracks);
		for (int i=video_tracks;i<=audio_tracks;i++) {
			Selection s;
			s.in = olive::ActiveSequence->workarea_in;
			s.out = olive::ActiveSequence->workarea_out;
			s.track = i;
			areas.append(s);
		}
		ComboAction* ca = new ComboAction();
		delete_areas_and_relink(ca, areas, true);
		if (ripple) ripple_clips(ca, olive::ActiveSequence, olive::ActiveSequence->workarea_in, olive::ActiveSequence->workarea_in - olive::ActiveSequence->workarea_out);
		ca->append(new SetTimelineInOutCommand(olive::ActiveSequence, false, 0, 0));
        olive::UndoStack.push(ca);
		update_ui(true);
    }
}

void Timeline::toggle_enable_on_selected_clips() {
    if (olive::ActiveSequence != nullptr) {
        ComboAction* ca = new ComboAction();
        bool push_undo = false;
        for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
            Clip* c = olive::ActiveSequence->clips.at(i);
            if (c != nullptr && is_clip_selected(c, true)) {
                ca->append(new SetBool(&c->enabled, !c->enabled));
                push_undo = true;
            }
        }
        if (push_undo) {
            olive::UndoStack.push(ca);
            update_ui(true);
        } else {
            delete ca;
        }
    }
}

void Timeline::delete_selection(QVector<Selection>& selections, bool ripple_delete) {
	if (selections.size() > 0) {
		panel_effect_controls->clear_effects(true);

		ComboAction* ca = new ComboAction();

		delete_areas_and_relink(ca, selections, true);

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
			for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
				Clip* c = olive::ActiveSequence->clips.at(i);
				if (c != nullptr && c->timeline_in < ripple_point && c->timeline_out > ripple_point) {
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
						for (int j=0;j<olive::ActiveSequence->clips.size();j++) {
							Clip* cc = olive::ActiveSequence->clips.at(j);
							if (cc != nullptr
									&& cc->track == c->track
									&& cc->timeline_in > c->timeline_out
									&& cc->timeline_in < c->timeline_out + ripple_length) {
								ripple_length = cc->timeline_in - c->timeline_out;
							}
						}
					}
				}
			}

			if (can_ripple) {
				ripple_clips(ca, olive::ActiveSequence, ripple_point, -ripple_length);
				panel_sequence_viewer->seek(ripple_point-1);
			}
		}

        olive::UndoStack.push(ca);

		update_ui(true);
	}
}

void Timeline::set_zoom_value(double v) {
    // set zoom value
	zoom = v;

    // update header zoom to match
    headers->update_zoom(zoom);

    // set flag that zoom has just changed to prevent auto-scrolling since we change the scroll below
    zoom_just_changed = true;

    // set scrollbar to center the playhead
    if (olive::ActiveSequence != nullptr
            && !horizontalScrollBar->is_resizing()) {
        // update scrollbar maximum value for new zoom
        set_sb_max();

		center_scroll_to_playhead(horizontalScrollBar, zoom, olive::ActiveSequence->playhead);
    }

    // repaint the timeline for the new zoom/location
    repaint_timeline();
}

void Timeline::multiply_zoom(double m) {
	showing_all = false;
    set_zoom_value(zoom * m);
}

void Timeline::decheck_tool_buttons(QObject* sender) {
	for (int i=0;i<tool_buttons.count();i++) {
		tool_buttons[i]->setChecked(tool_buttons.at(i) == sender);
	}
}

QVector<int> Timeline::get_tracks_of_linked_clips(int i) {
	QVector<int> tracks;
	Clip* clip = olive::ActiveSequence->clips.at(i);
	for (int j=0;j<clip->linked.size();j++) {
		tracks.append(olive::ActiveSequence->clips.at(clip->linked.at(j))->track);
	}
	return tracks;
}

void Timeline::zoom_in() {
    multiply_zoom(2.0);
}

void Timeline::zoom_out() {
    multiply_zoom(0.5);
}

bool is_clip_selected(Clip* clip, bool containing) {
	for (int i=0;i<clip->sequence->selections.size();i++) {
		const Selection& s = clip->sequence->selections.at(i);
		if (clip->track == s.track && ((clip->timeline_in >= s.in && clip->timeline_out <= s.out && containing) ||
				(!containing && !(clip->timeline_in < s.in && clip->timeline_out < s.in) && !(clip->timeline_in > s.in && clip->timeline_out > s.in)))) {
			return true;
		}
	}
	return false;
}

void Timeline::snapping_clicked(bool checked) {
	snapping = checked;
}

Clip* Timeline::split_clip(ComboAction* ca, bool transitions, int p, long frame) {
	return split_clip(ca, transitions, p, frame, frame);
}

Clip* Timeline::split_clip(ComboAction* ca, bool transitions, int p, long frame, long post_in) {
	Clip* pre = olive::ActiveSequence->clips.at(p);
	if (pre != nullptr && pre->timeline_in < frame && pre->timeline_out > frame) { // guard against attempts to split at in/out points
		bool splitting_closing_dual_transition = false;

		if (transitions
				&& pre->get_closing_transition() != nullptr
				&& pre->get_closing_transition()->secondary_clip != nullptr) {
			splitting_closing_dual_transition = true;
		}

		Clip* post = pre->copy(olive::ActiveSequence, transitions && !splitting_closing_dual_transition);

		long new_clip_length = frame - pre->timeline_in;

		post->timeline_in = post_in;
		post->clip_in = pre->clip_in + (post->timeline_in - pre->timeline_in);

		move_clip(ca, pre, pre->timeline_in, frame, pre->clip_in, pre->track);

		if (pre->get_opening_transition() != nullptr) {
//			if (frame < pre->timeline_in + pre->get_opening_transition()->length && pre->get_opening_transition()->secondary_clip != nullptr) {
				// separate shared transition
//				ca->append(new SetPointer((void**) &pre->get_opening_transition()->secondary_clip, nullptr));
//				pre->get_opening_transition()->secondary_clip->closing_transition = pre->get_opening_transition()->copy(pre->get_opening_transition()->secondary_clip, nullptr);
//			}

			if (pre->get_opening_transition()->get_true_length() > new_clip_length) {
				ca->append(new ModifyTransitionCommand(pre, TA_OPENING_TRANSITION, new_clip_length));
			}

			post->sequence->hard_delete_transition(post, TA_OPENING_TRANSITION);
		}
		if (pre->get_closing_transition() != nullptr) {
			if (splitting_closing_dual_transition) {
				// just move closing transition to post clip

				// WORKAROUND
				ca->append(new DeleteTransitionCommand(pre->sequence, pre->closing_transition));
			} else {
				ca->append(new DeleteTransitionCommand(pre->sequence, pre->closing_transition));

				if (post->get_closing_transition() != nullptr) {
					if (pre->get_closing_transition()->secondary_clip == nullptr) {
						post->get_closing_transition()->set_length(qMin(long(post->get_closing_transition()->get_true_length()), post->getLength()));
					}

					if (post->get_closing_transition()->get_length() > post->getLength()) {
						post->get_closing_transition()->set_length(post->getLength());
					}
				}
			}
		}

		return post;
	}
	return nullptr;
}

bool Timeline::has_clip_been_split(int c) {
	for (int i=0;i<split_cache.size();i++) {
		if (split_cache.at(i) == c) {
			return true;
		}
	}
	return false;
}

bool Timeline::split_clip_and_relink(ComboAction *ca, int clip, long frame, bool relink) {
	// see if we split this clip before
	if (has_clip_been_split(clip)) {
		return false;
	}

	split_cache.append(clip);

	Clip* c = olive::ActiveSequence->clips.at(clip);
	if (c != nullptr) {
		QVector<int> pre_clips;
		QVector<Clip*> post_clips;

		Clip* post = split_clip(ca, true, clip, frame);

		// if alt is not down, split clips links too
		if (post == nullptr) {
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
						Clip* link = olive::ActiveSequence->clips.at(l);
						if ((original_clip_is_selected && is_clip_selected(link, true)) || !original_clip_is_selected) {
							split_cache.append(l);
							Clip* s = split_clip(ca, true, l, frame);
							if (s != nullptr) {
								pre_clips.append(l);
								post_clips.append(s);
							}
						}
					}
				}

				relink_clips_using_ids(pre_clips, post_clips);
			}
			ca->append(new AddClipCommand(olive::ActiveSequence, post_clips));
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

bool selection_contains_transition(const Selection& s, Clip* c, int type) {
	if (type == TA_OPENING_TRANSITION) {
		return c->get_opening_transition() != nullptr
				&& s.out == c->timeline_in + c->get_opening_transition()->get_true_length()
				&& ((c->get_opening_transition()->secondary_clip == nullptr && s.in == c->timeline_in)
				|| (c->get_opening_transition()->secondary_clip != nullptr && s.in == c->timeline_in - c->get_opening_transition()->get_true_length()));
	} else {
		return c->get_closing_transition() != nullptr
				&& s.in == c->timeline_out - c->get_closing_transition()->get_true_length()
			   && ((c->get_closing_transition()->secondary_clip == nullptr && s.out == c->timeline_out)
			   || (c->get_closing_transition()->secondary_clip != nullptr && s.out == c->timeline_out + c->get_closing_transition()->get_true_length()));
	}
}

void Timeline::delete_areas_and_relink(ComboAction* ca, QVector<Selection>& areas, bool deselect_areas) {
	clean_up_selections(areas);
	panel_effect_controls->clear_effects(true);

	QVector<int> pre_clips;
	QVector<Clip*> post_clips;

	for (int i=0;i<areas.size();i++) {
		const Selection& s = areas.at(i);
		for (int j=0;j<olive::ActiveSequence->clips.size();j++) {
			Clip* c = olive::ActiveSequence->clips.at(j);
			if (c != nullptr && c->track == s.track && !c->undeletable) {
				if (selection_contains_transition(s, c, TA_OPENING_TRANSITION)) {
					// delete opening transition
					ca->append(new DeleteTransitionCommand(c->sequence, c->opening_transition));
				} else if (selection_contains_transition(s, c, TA_CLOSING_TRANSITION)) {
					// delete closing transition
					ca->append(new DeleteTransitionCommand(c->sequence, c->closing_transition));
				} else if (c->timeline_in >= s.in && c->timeline_out <= s.out) {
					// clips falls entirely within deletion area
					ca->append(new DeleteClipAction(olive::ActiveSequence, j));
				} else if (c->timeline_in < s.in && c->timeline_out > s.out) {
					// middle of clip is within deletion area

					// duplicate clip
					Clip* post = split_clip(ca, true, j, s.in, s.out);

					pre_clips.append(j);
					post_clips.append(post);
				} else if (c->timeline_in < s.in && c->timeline_out > s.in) {
					// only out point is in deletion area
					move_clip(ca, c, c->timeline_in, s.in, c->clip_in, c->track);

					if (c->get_closing_transition() != nullptr) {
						if (s.in < c->timeline_out - c->get_closing_transition()->get_true_length()) {
							ca->append(new DeleteTransitionCommand(c->sequence, c->closing_transition));
						} else {
							ca->append(new ModifyTransitionCommand(c, TA_CLOSING_TRANSITION, c->get_closing_transition()->get_true_length() - (c->timeline_out - s.in)));
						}
					}
				} else if (c->timeline_in < s.out && c->timeline_out > s.out) {
					// only in point is in deletion area
					move_clip(ca, c, s.out, c->timeline_out, c->clip_in + (s.out - c->timeline_in), c->track);

					if (c->get_opening_transition() != nullptr) {
						if (s.out > c->timeline_in + c->get_opening_transition()->get_true_length()) {
							ca->append(new DeleteTransitionCommand(c->sequence, c->opening_transition));
						} else {
							ca->append(new ModifyTransitionCommand(c, TA_OPENING_TRANSITION, c->get_opening_transition()->get_true_length() - (s.out - c->timeline_in)));
						}
					}
				}
			}
		}
	}

	// deselect selected clip areas
	if (deselect_areas) {
		QVector<Selection> area_copy = areas;
		for (int i=0;i<area_copy.size();i++) {
			const Selection& s = area_copy.at(i);
			deselect_area(s.in, s.out, s.track);
		}
	}

	relink_clips_using_ids(pre_clips, post_clips);
	ca->append(new AddClipCommand(olive::ActiveSequence, post_clips));
}

void Timeline::copy(bool del) {
	bool cleared = false;
	bool copied = false;

	long min_in = 0;

	for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
		Clip* c = olive::ActiveSequence->clips.at(i);
		if (c != nullptr) {
			for (int j=0;j<olive::ActiveSequence->selections.size();j++) {
				const Selection& s = olive::ActiveSequence->selections.at(j);
				if (s.track == c->track && !((c->timeline_in <= s.in && c->timeline_out <= s.in) || (c->timeline_in >= s.out && c->timeline_out >= s.out))) {
					if (!cleared) {
						clear_clipboard();
						cleared = true;
						clipboard_type = CLIPBOARD_TYPE_CLIP;
					}

					Clip* copied_clip = c->copy(nullptr);

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

					clipboard.append(copied_clip);
				}
			}
		}
	}

	for (int i=0;i<clipboard.size();i++) {
		// initialize all timeline_ins to 0 or offsets of
		static_cast<Clip*>(clipboard.at(i))->timeline_in -= min_in;
		static_cast<Clip*>(clipboard.at(i))->timeline_out -= min_in;
	}

	if (del && copied) {
		delete_selection(olive::ActiveSequence->selections, false);
	}
}

void Timeline::relink_clips_using_ids(QVector<int>& old_clips, QVector<Clip*>& new_clips) {
	// relink pasted clips
	for (int i=0;i<old_clips.size();i++) {
		// these indices should correspond
		Clip* oc = olive::ActiveSequence->clips.at(old_clips.at(i));
		for (int j=0;j<oc->linked.size();j++) {
			for (int k=0;k<old_clips.size();k++) { // find clip with that ID
				if (oc->linked.at(j) == old_clips.at(k)) {
					if (new_clips.at(i) != nullptr) {
						new_clips.at(i)->linked.append(k);
					}
				}
			}
		}
	}
}

void Timeline::paste(bool insert) {
	if (clipboard.size() > 0) {
		if (clipboard_type == CLIPBOARD_TYPE_CLIP) {
			ComboAction* ca = new ComboAction();

			// create copies and delete areas that we'll be pasting to
			QVector<Selection> delete_areas;
			QVector<Clip*> pasted_clips;
			long paste_start = LONG_MAX;
			long paste_end = LONG_MIN;
			for (int i=0;i<clipboard.size();i++) {
				Clip* c = static_cast<Clip*>(clipboard.at(i));

				// create copy of clip and offset by playhead
				Clip* cc = c->copy(olive::ActiveSequence);

				// convert frame rates
				cc->timeline_in = refactor_frame_number(cc->timeline_in, c->cached_fr, olive::ActiveSequence->frame_rate);
				cc->timeline_out = refactor_frame_number(cc->timeline_out, c->cached_fr, olive::ActiveSequence->frame_rate);
				cc->clip_in = refactor_frame_number(cc->clip_in, c->cached_fr, olive::ActiveSequence->frame_rate);

				cc->timeline_in += olive::ActiveSequence->playhead;
				cc->timeline_out += olive::ActiveSequence->playhead;
				cc->track = c->track;

				paste_start = qMin(paste_start, cc->timeline_in);
				paste_end = qMax(paste_end, cc->timeline_out);

				pasted_clips.append(cc);

				if (!insert) {
					Selection s;
					s.in = cc->timeline_in;
					s.out = cc->timeline_out;
					s.track = c->track;
					delete_areas.append(s);
				}
			}
			if (insert) {
				split_cache.clear();
				split_all_clips_at_point(ca, olive::ActiveSequence->playhead);
				ripple_clips(ca, olive::ActiveSequence, paste_start, paste_end - paste_start);
			} else {
				delete_areas_and_relink(ca, delete_areas, false);
			}

			// correct linked clips
			for (int i=0;i<clipboard.size();i++) {
				// these indices should correspond
				Clip* oc = static_cast<Clip*>(clipboard.at(i));

				for (int j=0;j<oc->linked.size();j++) {
					for (int k=0;k<clipboard.size();k++) { // find clip with that ID
						Clip* comp = static_cast<Clip*>(clipboard.at(k));
						if (comp->load_id == oc->linked.at(j)) {
							pasted_clips.at(i)->linked.append(k);
						}
					}
				}
			}

			ca->append(new AddClipCommand(olive::ActiveSequence, pasted_clips));

            olive::UndoStack.push(ca);

			update_ui(true);

			if (olive::CurrentConfig.paste_seeks) {
				panel_sequence_viewer->seek(paste_end);
			}
		} else if (clipboard_type == CLIPBOARD_TYPE_EFFECT) {
			ComboAction* ca = new ComboAction();
			bool push = false;

			bool replace = false;
			bool skip = false;
			bool ask_conflict = true;

			for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
				Clip* c = olive::ActiveSequence->clips.at(i);
				if (c != nullptr && is_clip_selected(c, true)) {
					for (int j=0;j<clipboard.size();j++) {
						Effect* e = static_cast<Effect*>(clipboard.at(j));
						if ((c->track < 0) == (e->meta->subtype == EFFECT_TYPE_VIDEO)) {
							int found = -1;
							if (ask_conflict) {
								replace = false;
								skip = false;
							}
							for (int k=0;k<c->effects.size();k++) {
								if (c->effects.at(k)->meta == e->meta) {
									found = k;
									break;
								}
							}
							if (found >= 0 && ask_conflict) {
								QMessageBox box(this);
								box.setWindowTitle(tr("Effect already exists"));
								box.setText(tr("Clip '%1' already contains a '%2' effect. Would you like to replace it with the pasted one or add it as a separate effect?").arg(c->name, e->meta->name));
								box.setIcon(QMessageBox::Icon::Question);

								box.addButton(tr("Add"), QMessageBox::YesRole);
								QPushButton* replace_button = box.addButton(tr("Replace"), QMessageBox::NoRole);
								QPushButton* skip_button = box.addButton(tr("Skip"), QMessageBox::RejectRole);

								QCheckBox* future_box = new QCheckBox(tr("Do this for all conflicts found"), &box);
								box.setCheckBox(future_box);

								box.exec();

								if (box.clickedButton() == replace_button) {
									replace = true;
								} else if (box.clickedButton() == skip_button) {
									skip = true;
								}
								ask_conflict = !future_box->isChecked();
							}

							if (found >= 0 && skip) {
								// do nothing
							} else if (found >= 0 && replace) {
								EffectDeleteCommand* delcom = new EffectDeleteCommand();
								delcom->clips.append(c);
								delcom->fx.append(found);
								ca->append(delcom);

								ca->append(new AddEffectCommand(c, e->copy(c), nullptr, found));
								push = true;
							} else {
								ca->append(new AddEffectCommand(c, e->copy(c), nullptr));
								push = true;
							}
						}
					}
				}
			}
			if (push) {
                ca->appendPost(new ReloadEffectsCommand());
                olive::UndoStack.push(ca);
			} else {
				delete ca;
			}
			update_ui(true);
		}
	}
}

void Timeline::edit_to_point_internal(bool in, bool ripple) {
	if (olive::ActiveSequence != nullptr) {
		if (olive::ActiveSequence->clips.size() > 0) {
			// get track count
			int track_min = INT_MAX;
			int track_max = INT_MIN;
			long sequence_end = 0;

			bool playhead_falls_on_in = false;
			bool playhead_falls_on_out = false;
			long next_cut = LONG_MAX;
			long prev_cut = 0;

			// find closest in point to playhead
			for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
				Clip* c = olive::ActiveSequence->clips.at(i);
				if (c != nullptr) {
					track_min = qMin(track_min, c->track);
					track_max = qMax(track_max, c->track);

					sequence_end = qMax(c->timeline_out, sequence_end);

					if (c->timeline_in == olive::ActiveSequence->playhead)
						playhead_falls_on_in = true;
					if (c->timeline_out == olive::ActiveSequence->playhead)
						playhead_falls_on_out = true;
					if (c->timeline_in > olive::ActiveSequence->playhead)
						next_cut = qMin(c->timeline_in, next_cut);
					if (c->timeline_out > olive::ActiveSequence->playhead)
						next_cut = qMin(c->timeline_out, next_cut);
					if (c->timeline_in < olive::ActiveSequence->playhead)
						prev_cut = qMax(c->timeline_in, prev_cut);
					if (c->timeline_out < olive::ActiveSequence->playhead)
						prev_cut = qMax(c->timeline_out, prev_cut);
				}
			}

			next_cut = qMin(sequence_end, next_cut);

			QVector<Selection> areas;
			ComboAction* ca = new ComboAction();
			bool push_undo = true;
			long seek = olive::ActiveSequence->playhead;

			if ((in && (playhead_falls_on_out || (playhead_falls_on_in && olive::ActiveSequence->playhead == 0)))
					|| (!in && (playhead_falls_on_in || (playhead_falls_on_out && olive::ActiveSequence->playhead == sequence_end)))) { // one frame mode
				if (ripple) {
					// set up deletion areas based on track count
					long in_point = olive::ActiveSequence->playhead;
					if (!in) {
						in_point--;
						seek--;
					}

					if (in_point >= 0) {
						Selection s;
						s.in = in_point;
						s.out = in_point + 1;
						for (int i=track_min;i<=track_max;i++) {
							s.track = i;
							areas.append(s);
						}

						// trim and move clips around the in point
						delete_areas_and_relink(ca, areas, true);

						if (ripple) ripple_clips(ca, olive::ActiveSequence, in_point, -1);
					} else {
						push_undo = false;
					}
				} else {
					push_undo = false;
				}
			} else {
				// set up deletion areas based on track count
				Selection s;
				if (in) seek = prev_cut;
				s.in = in ? prev_cut : olive::ActiveSequence->playhead;
				s.out = in ? olive::ActiveSequence->playhead : next_cut;

				if (s.in == s.out) {
					push_undo = false;
				} else {
					for (int i=track_min;i<=track_max;i++) {
						s.track = i;
						areas.append(s);
					}

					// trim and move clips around the in point
					delete_areas_and_relink(ca, areas, true);
					if (ripple) ripple_clips(ca, olive::ActiveSequence, s.in, s.in - s.out);
				}
			}

			if (push_undo) {
                olive::UndoStack.push(ca);

				update_ui(true);

				if (seek != olive::ActiveSequence->playhead && ripple) panel_sequence_viewer->seek(seek);
			} else {
				delete ca;
			}
		} else {
			panel_sequence_viewer->seek(0);
		}
	}
}

bool Timeline::split_selection(ComboAction* ca) {
	bool split = false;

	// temporary relinking vectors
	QVector<int> pre_splits;
	QVector<Clip*> post_splits;
	QVector<Clip*> secondary_post_splits;

	// find clips within selection and split
	for (int j=0;j<olive::ActiveSequence->clips.size();j++) {
		Clip* clip = olive::ActiveSequence->clips.at(j);
		if (clip != nullptr) {
			for (int i=0;i<olive::ActiveSequence->selections.size();i++) {
				const Selection& s = olive::ActiveSequence->selections.at(i);
				if (s.track == clip->track) {
					Clip* post_b = split_clip(ca, true, j, s.out);
					Clip* post_a = split_clip(ca, post_b == nullptr, j, s.in);
					pre_splits.append(j);
					post_splits.append(post_a);
					secondary_post_splits.append(post_b);

					if (post_a != nullptr) {
						post_a->timeline_out = qMin(post_a->timeline_out, s.out);
					}

					split = true;
				}
			}
		}
	}

	if (split) {
		// relink after splitting
		relink_clips_using_ids(pre_splits, post_splits);
		relink_clips_using_ids(pre_splits, secondary_post_splits);

		ca->append(new AddClipCommand(olive::ActiveSequence, post_splits));
		ca->append(new AddClipCommand(olive::ActiveSequence, secondary_post_splits));

		return true;
	}
	return false;
}

bool Timeline::split_all_clips_at_point(ComboAction* ca, long point) {
	bool split = false;
	for (int j=0;j<olive::ActiveSequence->clips.size();j++) {
		Clip* c = olive::ActiveSequence->clips.at(j);
		if (c != nullptr) {
			// always relinks
			if (split_clip_and_relink(ca, j, point, true)) {
				split = true;
			}
		}
	}
	return split;
}

void Timeline::split_at_playhead() {
	ComboAction* ca = new ComboAction();
	bool split_selected = false;
	split_cache.clear();

	if (olive::ActiveSequence->selections.size() > 0) {
		// see if whole clips are selected
		QVector<int> pre_clips;
		QVector<Clip*> post_clips;
		for (int j=0;j<olive::ActiveSequence->clips.size();j++) {
			Clip* clip = olive::ActiveSequence->clips.at(j);
			if (clip != nullptr && is_clip_selected(clip, true)) {
				Clip* s = split_clip(ca, true, j, olive::ActiveSequence->playhead);
				if (s != nullptr) {
					pre_clips.append(j);
					post_clips.append(s);
					split_selected = true;
				}
			}
		}

		if (split_selected) {
			// relink clips if we split
			relink_clips_using_ids(pre_clips, post_clips);
			ca->append(new AddClipCommand(olive::ActiveSequence, post_clips));
		} else {
			// split a selection if not
			split_selected = split_selection(ca);
		}
	}

	// if nothing was selected or no selections fell within playhead, simply split at playhead
	if (!split_selected) {
		split_selected = split_all_clips_at_point(ca, olive::ActiveSequence->playhead);
	}

	if (split_selected) {
        olive::UndoStack.push(ca);
		update_ui(true);
	} else {
		delete ca;
    }
}

void Timeline::ripple_delete() {
    if (olive::ActiveSequence != nullptr) {
        if (olive::ActiveSequence->selections.size() > 0) {
            panel_timeline->delete_selection(olive::ActiveSequence->selections, true);
        } else if (olive::CurrentConfig.hover_focus && get_focused_panel() == panel_timeline) {
            if (panel_timeline->can_ripple_empty_space(panel_timeline->cursor_frame, panel_timeline->cursor_track)) {
                panel_timeline->ripple_delete_empty_space();
            }
        }
    }
}

void Timeline::deselect_area(long in, long out, int track) {
	int len = olive::ActiveSequence->selections.size();
	for (int i=0;i<len;i++) {
		Selection& s = olive::ActiveSequence->selections[i];
		if (s.track == track) {
			if (s.in >= in && s.out <= out) {
				// whole selection is in deselect area
				olive::ActiveSequence->selections.removeAt(i);
				i--;
				len--;
			} else if (s.in < in && s.out > out) {
				// middle of selection is in deselect area
				Selection new_sel;
				new_sel.in = out;
				new_sel.out = s.out;
				new_sel.track = s.track;
				olive::ActiveSequence->selections.append(new_sel);

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

bool Timeline::snap_to_timeline(long* l, bool use_playhead, bool use_markers, bool use_workarea) {
	snapped = false;
	if (snapping) {
		if (use_playhead && !panel_sequence_viewer->playing) {
			// snap to playhead
			if (snap_to_point(olive::ActiveSequence->playhead, l)) return true;
		}

		// snap to marker
		if (use_markers) {
			for (int i=0;i<olive::ActiveSequence->markers.size();i++) {
				if (snap_to_point(olive::ActiveSequence->markers.at(i).frame, l)) return true;
			}
		}

		// snap to in/out
		if (use_workarea && olive::ActiveSequence->using_workarea) {
			if (snap_to_point(olive::ActiveSequence->workarea_in, l)) return true;
			if (snap_to_point(olive::ActiveSequence->workarea_out, l)) return true;
		}

		// snap to clip/transition
		for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
			Clip* c = olive::ActiveSequence->clips.at(i);
			if (c != nullptr) {
				if (snap_to_point(c->timeline_in, l)) {
					return true;
				} else if (snap_to_point(c->timeline_out, l)) {
					return true;
				} else if (c->get_opening_transition() != nullptr
						   && snap_to_point(c->timeline_in + c->get_opening_transition()->get_true_length(), l)) {
					return true;
				} else if (c->get_closing_transition() != nullptr
						   && snap_to_point(c->timeline_out - c->get_closing_transition()->get_true_length(), l)) {
					return true;
				} else {
					// try to snap to clip markers
					for (int j=0;j<c->get_markers().size();j++) {
						if (snap_to_point(c->get_markers().at(j).frame + c->timeline_in - c->clip_in, l)) {
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

void Timeline::set_marker() {
	// determine if any clips are selected, and if so add markers to clips rather than the sequence
	QVector<int> clips_selected;
	bool clip_mode = false;

	for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
		Clip* c = olive::ActiveSequence->clips.at(i);
		if (c != nullptr
				&& is_clip_selected(c, true)) {

			// only add markers if the playhead is inside the clip
			if (olive::ActiveSequence->playhead >= c->timeline_in
					&& olive::ActiveSequence->playhead <= c->timeline_out) {
				clips_selected.append(i);
			}

			// we are definitely adding markers to clips though
			clip_mode = true;

		}
	}

	// if we've selected clips but none of the clips are within the playhead,
	// nothing to do here
	if (clip_mode && clips_selected.size() == 0) {
		return;
	}

	// pass off to internal set marker function
	set_marker_internal(olive::ActiveSequence, clips_selected);

}

void Timeline::delete_inout() {
    panel_timeline->delete_in_out_internal(false);
}

void Timeline::ripple_delete_inout() {
    panel_timeline->delete_in_out_internal(true);
}

void Timeline::ripple_to_in_point() {
    panel_timeline->edit_to_point_internal(true, true);
}

void Timeline::ripple_to_out_point() {
    panel_timeline->edit_to_point_internal(false, true);
}

void Timeline::edit_to_in_point() {
    panel_timeline->edit_to_point_internal(true, false);
}

void Timeline::edit_to_out_point() {
    panel_timeline->edit_to_point_internal(false, false);
}

void Timeline::toggle_links() {
	LinkCommand* command = new LinkCommand();
	command->s = olive::ActiveSequence;
	for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
		Clip* c = olive::ActiveSequence->clips.at(i);
		if (c != nullptr && is_clip_selected(c, true)) {
			if (!command->clips.contains(i)) command->clips.append(i);

			if (c->linked.size() > 0) {
				command->link = false; // prioritize unlinking

				for (int j=0;j<c->linked.size();j++) { // add links to the command
					if (!command->clips.contains(c->linked.at(j))) command->clips.append(c->linked.at(j));
				}
			}
		}
	}
	if (command->clips.size() > 0) {
        olive::UndoStack.push(command);
		repaint_timeline();
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
	repaint_timeline();
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
	repaint_timeline();
}

void Timeline::deselect() {
	olive::ActiveSequence->selections.clear();
	repaint_timeline();
}

long getFrameFromScreenPoint(double zoom, int x) {
	long f = qCeil(double(x) / zoom);
	if (f < 0) {
		return 0;
	}
	return f;
}

int getScreenPointFromFrame(double zoom, long frame) {
	return qFloor(double(frame)*zoom);
}

long Timeline::getTimelineFrameFromScreenPoint(int x) {
	return getFrameFromScreenPoint(zoom, x + scroll);
}

int Timeline::getTimelineScreenPointFromFrame(long frame) {
	return getScreenPointFromFrame(zoom, frame) - scroll;
}

void Timeline::add_btn_click() {
	QMenu add_menu(this);

	QAction* titleMenuItem = new QAction(&add_menu);
	titleMenuItem->setText(tr("Title..."));
	titleMenuItem->setData(ADD_OBJ_TITLE);
	add_menu.addAction(titleMenuItem);

	QAction* solidMenuItem = new QAction(&add_menu);
	solidMenuItem->setText(tr("Solid Color..."));
	solidMenuItem->setData(ADD_OBJ_SOLID);
	add_menu.addAction(solidMenuItem);

	QAction* barsMenuItem = new QAction(&add_menu);
	barsMenuItem->setText(tr("Bars..."));
	barsMenuItem->setData(ADD_OBJ_BARS);
	add_menu.addAction(barsMenuItem);

	add_menu.addSeparator();

	QAction* toneMenuItem = new QAction(&add_menu);
	toneMenuItem->setText(tr("Tone..."));
	toneMenuItem->setData(ADD_OBJ_TONE);
	add_menu.addAction(toneMenuItem);

	QAction* noiseMenuItem = new QAction(&add_menu);
	noiseMenuItem->setText(tr("Noise..."));
	noiseMenuItem->setData(ADD_OBJ_NOISE);
	add_menu.addAction(noiseMenuItem);

	connect(&add_menu, SIGNAL(triggered(QAction*)), this, SLOT(add_menu_item(QAction*)));

	add_menu.exec(QCursor::pos());
}

void Timeline::add_menu_item(QAction* action) {
	creating = true;
	creating_object = action->data().toInt();
}

void Timeline::setScroll(int s) {
	scroll = s;
	headers->set_scroll(s);
	repaint_timeline();
}

void Timeline::record_btn_click() {
	if (olive::ActiveProjectFilename.isEmpty()) {
		QMessageBox::critical(this,
							  tr("Unsaved Project"),
							  tr("You must save this project before you can record audio in it."),
							  QMessageBox::Ok);
	} else {
		creating = true;
		creating_object = ADD_OBJ_AUDIO;
        olive::MainWindow->statusBar()->showMessage(
					tr("Click on the timeline where you want to start recording (drag to limit the recording to a certain timeframe)"),
					10000);
	}
}

void Timeline::transition_tool_click() {
	creating = false;

	QMenu transition_menu(this);

	for (int i=0;i<effects.size();i++) {
		const EffectMeta& em = effects.at(i);
		if (em.type == EFFECT_TYPE_TRANSITION && em.subtype == EFFECT_TYPE_VIDEO) {
			QAction* a = transition_menu.addAction(em.name);
			a->setObjectName("v");
			a->setData(reinterpret_cast<quintptr>(&em));
		}
	}

	transition_menu.addSeparator();

	for (int i=0;i<effects.size();i++) {
		const EffectMeta& em = effects.at(i);
		if (em.type == EFFECT_TYPE_TRANSITION && em.subtype == EFFECT_TYPE_AUDIO) {
			QAction* a = transition_menu.addAction(em.name);
			a->setObjectName("a");
			a->setData(reinterpret_cast<quintptr>(&em));
		}
	}

	connect(&transition_menu, SIGNAL(triggered(QAction*)), this, SLOT(transition_menu_select(QAction*)));

	toolTransitionButton->setChecked(false);

	transition_menu.exec(QCursor::pos());
}

void Timeline::transition_menu_select(QAction* a) {
	transition_tool_meta = reinterpret_cast<const EffectMeta*>(a->data().value<quintptr>());

	if (a->objectName() == "v") {
		transition_tool_side = -1;
	} else {
		transition_tool_side = 1;
	}

	decheck_tool_buttons(sender());
	timeline_area->setCursor(Qt::CrossCursor);
	tool = TIMELINE_TOOL_TRANSITION;
	toolTransitionButton->setChecked(true);
}

void Timeline::resize_move(double z) {
	set_zoom_value(zoom * z);
}

void Timeline::set_sb_max() {
	headers->set_scrollbar_max(horizontalScrollBar, olive::ActiveSequence->getEndFrame(), editAreas->width() - getScreenPointFromFrame(zoom, 200));
}

void Timeline::setup_ui() {
	QWidget* dockWidgetContents = new QWidget();

	QHBoxLayout* horizontalLayout = new QHBoxLayout(dockWidgetContents);
	horizontalLayout->setSpacing(0);
	horizontalLayout->setMargin(0);

	setWidget(dockWidgetContents);

	tool_button_widget = new QWidget();
	tool_button_widget->setObjectName("timeline_toolbar");
	tool_button_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	FlowLayout* tool_buttons_layout = new FlowLayout(tool_button_widget);
	tool_buttons_layout->setSpacing(4);
	tool_buttons_layout->setMargin(0);

	toolArrowButton = new QPushButton();
	QIcon arrow_icon;
	arrow_icon.addFile(QStringLiteral(":/icons/arrow.png"), QSize(), QIcon::Normal, QIcon::Off);
	arrow_icon.addFile(QStringLiteral(":/icons/arrow-disabled.png"), QSize(), QIcon::Disabled, QIcon::Off);
	toolArrowButton->setIcon(arrow_icon);
	toolArrowButton->setCheckable(true);
	toolArrowButton->setToolTip(tr("Pointer Tool") + " (V)");
	toolArrowButton->setProperty("tool", TIMELINE_TOOL_POINTER);
	connect(toolArrowButton, SIGNAL(clicked(bool)), this, SLOT(set_tool()));
	tool_buttons_layout->addWidget(toolArrowButton);

	toolEditButton = new QPushButton();
	QIcon icon1;
	icon1.addFile(QStringLiteral(":/icons/beam.png"), QSize(), QIcon::Normal, QIcon::Off);
	icon1.addFile(QStringLiteral(":/icons/beam-disabled.png"), QSize(), QIcon::Disabled, QIcon::Off);
	toolEditButton->setIcon(icon1);
	toolEditButton->setCheckable(true);
	toolEditButton->setToolTip(tr("Edit Tool") + " (X)");
	toolEditButton->setProperty("tool", TIMELINE_TOOL_EDIT);
	connect(toolEditButton, SIGNAL(clicked(bool)), this, SLOT(set_tool()));
	tool_buttons_layout->addWidget(toolEditButton);

	toolRippleButton = new QPushButton();
	QIcon icon2;
	icon2.addFile(QStringLiteral(":/icons/ripple.png"), QSize(), QIcon::Normal, QIcon::Off);
	icon2.addFile(QStringLiteral(":/icons/ripple-disabled.png"), QSize(), QIcon::Disabled, QIcon::Off);
	toolRippleButton->setIcon(icon2);
	toolRippleButton->setCheckable(true);
	toolRippleButton->setToolTip(tr("Ripple Tool") + " (B)");
	toolRippleButton->setProperty("tool", TIMELINE_TOOL_RIPPLE);
	connect(toolRippleButton, SIGNAL(clicked(bool)), this, SLOT(set_tool()));
	tool_buttons_layout->addWidget(toolRippleButton);

	toolRazorButton = new QPushButton();
	QIcon icon4;
	icon4.addFile(QStringLiteral(":/icons/razor.png"), QSize(), QIcon::Normal, QIcon::Off);
	icon4.addFile(QStringLiteral(":/icons/razor-disabled.png"), QSize(), QIcon::Disabled, QIcon::Off);
	toolRazorButton->setIcon(icon4);
	toolRazorButton->setCheckable(true);
	toolRazorButton->setToolTip(tr("Razor Tool") + " (C)");
	toolRazorButton->setProperty("tool", TIMELINE_TOOL_RAZOR);
	connect(toolRazorButton, SIGNAL(clicked(bool)), this, SLOT(set_tool()));
	tool_buttons_layout->addWidget(toolRazorButton);

	toolSlipButton = new QPushButton();
	QIcon icon5;
	icon5.addFile(QStringLiteral(":/icons/slip.png"), QSize(), QIcon::Normal, QIcon::On);
	icon5.addFile(QStringLiteral(":/icons/slip-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
	toolSlipButton->setIcon(icon5);
	toolSlipButton->setCheckable(true);
	toolSlipButton->setToolTip(tr("Slip Tool") + " (Y)");
	toolSlipButton->setProperty("tool", TIMELINE_TOOL_SLIP);
	connect(toolSlipButton, SIGNAL(clicked(bool)), this, SLOT(set_tool()));
	tool_buttons_layout->addWidget(toolSlipButton);

	toolSlideButton = new QPushButton();
	QIcon icon6;
	icon6.addFile(QStringLiteral(":/icons/slide.png"), QSize(), QIcon::Normal, QIcon::On);
	icon6.addFile(QStringLiteral(":/icons/slide-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
	toolSlideButton->setIcon(icon6);
	toolSlideButton->setCheckable(true);
	toolSlideButton->setToolTip(tr("Slide Tool") + " (U)");
	toolSlideButton->setProperty("tool", TIMELINE_TOOL_SLIDE);
	connect(toolSlideButton, SIGNAL(clicked(bool)), this, SLOT(set_tool()));
	tool_buttons_layout->addWidget(toolSlideButton);

	toolHandButton = new QPushButton();
	QIcon icon7;
	icon7.addFile(QStringLiteral(":/icons/hand.png"), QSize(), QIcon::Normal, QIcon::On);
	icon7.addFile(QStringLiteral(":/icons/hand-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
	toolHandButton->setIcon(icon7);
	toolHandButton->setCheckable(true);
	toolHandButton->setToolTip(tr("Hand Tool") + " (H)");
	toolHandButton->setProperty("tool", TIMELINE_TOOL_HAND);
	connect(toolHandButton, SIGNAL(clicked(bool)), this, SLOT(set_tool()));
	tool_buttons_layout->addWidget(toolHandButton);

	toolTransitionButton = new QPushButton();
	QIcon icon8;
	icon8.addFile(QStringLiteral(":/icons/transition-tool.png"), QSize(), QIcon::Normal, QIcon::On);
	icon8.addFile(QStringLiteral(":/icons/transition-tool-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
	toolTransitionButton->setIcon(icon8);
	toolTransitionButton->setCheckable(true);
	toolTransitionButton->setToolTip(tr("Transition Tool") + " (T)");
	connect(toolTransitionButton, SIGNAL(clicked(bool)), this, SLOT(transition_tool_click()));
	tool_buttons_layout->addWidget(toolTransitionButton);

	snappingButton = new QPushButton();
	QIcon icon9;
	icon9.addFile(QStringLiteral(":/icons/magnet.png"), QSize(), QIcon::Normal, QIcon::On);
	icon9.addFile(QStringLiteral(":/icons/magnet-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
	snappingButton->setIcon(icon9);
	snappingButton->setCheckable(true);
	snappingButton->setChecked(true);
	snappingButton->setToolTip(tr("Snapping") + " (S)");
	connect(snappingButton, SIGNAL(toggled(bool)), this, SLOT(snapping_clicked(bool)));
	tool_buttons_layout->addWidget(snappingButton);

	zoomInButton = new QPushButton();
	QIcon icon10;
	icon10.addFile(QStringLiteral(":/icons/zoomin.png"), QSize(), QIcon::Normal, QIcon::On);
	icon10.addFile(QStringLiteral(":/icons/zoomin-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
	zoomInButton->setIcon(icon10);
	zoomInButton->setToolTip(tr("Zoom In") + " (=)");
	connect(zoomInButton, SIGNAL(clicked(bool)), this, SLOT(zoom_in()));
	tool_buttons_layout->addWidget(zoomInButton);

	zoomOutButton = new QPushButton();
	QIcon icon11;
	icon11.addFile(QStringLiteral(":/icons/zoomout.png"), QSize(), QIcon::Normal, QIcon::On);
	icon11.addFile(QStringLiteral(":/icons/zoomout-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
	zoomOutButton->setIcon(icon11);
	zoomOutButton->setToolTip(tr("Zoom Out") + " (-)");
	connect(zoomOutButton, SIGNAL(clicked(bool)), this, SLOT(zoom_out()));
	tool_buttons_layout->addWidget(zoomOutButton);

	recordButton = new QPushButton();
	QIcon icon12;
	icon12.addFile(QStringLiteral(":/icons/record.png"), QSize(), QIcon::Normal, QIcon::On);
	icon12.addFile(QStringLiteral(":/icons/record-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
	recordButton->setIcon(icon12);
	recordButton->setToolTip(tr("Record audio"));
	connect(recordButton, SIGNAL(clicked(bool)), this, SLOT(record_btn_click()));
	tool_buttons_layout->addWidget(recordButton);

	addButton = new QPushButton();
	QIcon icon13;
	icon13.addFile(QStringLiteral(":/icons/add-button.png"), QSize(), QIcon::Normal, QIcon::On);
	icon13.addFile(QStringLiteral(":/icons/add-button-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
	addButton->setIcon(icon13);
	addButton->setToolTip(tr("Add title, solid, bars, etc."));
	connect(addButton, SIGNAL(clicked()), this, SLOT(add_btn_click()));
	tool_buttons_layout->addWidget(addButton);

	horizontalLayout->addWidget(tool_button_widget);

	timeline_area = new QWidget();
	QSizePolicy timeline_area_policy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	timeline_area_policy.setHorizontalStretch(1);
	timeline_area_policy.setVerticalStretch(0);
	timeline_area_policy.setHeightForWidth(timeline_area->sizePolicy().hasHeightForWidth());
	timeline_area->setSizePolicy(timeline_area_policy);

	QVBoxLayout* timeline_area_layout = new QVBoxLayout(timeline_area);
	timeline_area_layout->setSpacing(0);
	timeline_area_layout->setContentsMargins(0, 0, 0, 0);

	headers = new TimelineHeader();
	timeline_area_layout->addWidget(headers);

	editAreas = new QWidget();
	QHBoxLayout* editAreaLayout = new QHBoxLayout(editAreas);
	editAreaLayout->setSpacing(0);
	editAreaLayout->setContentsMargins(0, 0, 0, 0);

	QSplitter* splitter = new QSplitter();
	splitter->setChildrenCollapsible(false);
	splitter->setOrientation(Qt::Vertical);

	QWidget* videoContainer = new QWidget();

	QHBoxLayout* videoContainerLayout = new QHBoxLayout(videoContainer);
	videoContainerLayout->setSpacing(0);
	videoContainerLayout->setContentsMargins(0, 0, 0, 0);

	video_area = new TimelineWidget();
	video_area->setFocusPolicy(Qt::ClickFocus);
	videoContainerLayout->addWidget(video_area);

	videoScrollbar = new QScrollBar();
	videoScrollbar->setMaximum(0);
	videoScrollbar->setSingleStep(20);
	videoScrollbar->setOrientation(Qt::Vertical);
	videoContainerLayout->addWidget(videoScrollbar);

	splitter->addWidget(videoContainer);

	QWidget* audioContainer = new QWidget();
	QHBoxLayout* audioContainerLayout = new QHBoxLayout(audioContainer);
	audioContainerLayout->setSpacing(0);
	audioContainerLayout->setContentsMargins(0, 0, 0, 0);

	audio_area = new TimelineWidget();
	audio_area->setFocusPolicy(Qt::ClickFocus);

	audioContainerLayout->addWidget(audio_area);

	audioScrollbar = new QScrollBar();
	audioScrollbar->setMaximum(0);
	audioScrollbar->setOrientation(Qt::Vertical);

	audioContainerLayout->addWidget(audioScrollbar);

	splitter->addWidget(audioContainer);

	editAreaLayout->addWidget(splitter);

	timeline_area_layout->addWidget(editAreas);

	horizontalScrollBar = new ResizableScrollBar();
	horizontalScrollBar->setMaximum(0);
	horizontalScrollBar->setSingleStep(20);
	horizontalScrollBar->setOrientation(Qt::Horizontal);

	timeline_area_layout->addWidget(horizontalScrollBar);

	horizontalLayout->addWidget(timeline_area);

	audio_monitor = new AudioMonitor();
	audio_monitor->setMinimumSize(QSize(50, 0));

	horizontalLayout->addWidget(audio_monitor);

	setWidget(dockWidgetContents);
}

void move_clip(ComboAction* ca, Clip *c, long iin, long iout, long iclip_in, int itrack, bool verify_transitions, bool relative) {
	ca->append(new MoveClipAction(c, iin, iout, iclip_in, itrack, relative));

	if (verify_transitions) {
		if (c->get_opening_transition() != nullptr && c->get_opening_transition()->secondary_clip != nullptr && c->get_opening_transition()->secondary_clip->timeline_out != iin) {
			// separate transition
			ca->append(new SetPointer(reinterpret_cast<void**>(&c->get_opening_transition()->secondary_clip), nullptr));
			ca->append(new AddTransitionCommand(c->get_opening_transition()->secondary_clip, nullptr, c->get_opening_transition(), nullptr, TA_CLOSING_TRANSITION, 0));
		}

		if (c->get_closing_transition() != nullptr && c->get_closing_transition()->secondary_clip != nullptr && c->get_closing_transition()->parent_clip->timeline_in != iout) {
			// separate transition
			ca->append(new SetPointer(reinterpret_cast<void**>(&c->get_closing_transition()->secondary_clip), nullptr));
			ca->append(new AddTransitionCommand(c, nullptr, c->get_closing_transition(), nullptr, TA_CLOSING_TRANSITION, 0));
		}
	}
}

void Timeline::set_tool() {
	QPushButton* button = static_cast<QPushButton*>(sender());
	decheck_tool_buttons(button);
	tool = button->property("tool").toInt();
	creating = false;
	switch (tool) {
	case TIMELINE_TOOL_EDIT:
	case TIMELINE_TOOL_RAZOR:
		timeline_area->setCursor(Qt::IBeamCursor);
		break;
	case TIMELINE_TOOL_HAND:
		timeline_area->setCursor(Qt::OpenHandCursor);
		break;
	default:
		timeline_area->setCursor(Qt::ArrowCursor);
	}
}
