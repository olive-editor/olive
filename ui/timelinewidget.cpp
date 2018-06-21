#include "timelinewidget.h"

#include "panels/panels.h"
#include "io/config.h"
#include "project/sequence.h"
#include "project/clip.h"
#include "panels/project.h"
#include "panels/timeline.h"
#include "io/media.h"
#include "ui/sourcetable.h"
#include "panels/effectcontrols.h"

#include "effects/effects.h"

#include <QPainter>
#include <QColor>
#include <QDebug>
#include <QMouseEvent>
#include <QObject>
#include <QVariant>
#include <QPointF>
#include <QtMath>

TimelineWidget::TimelineWidget(QWidget *parent) : QWidget(parent)
{
	bottom_align = false;
	setMouseTracking(true);
    track_height = 80;

    setAcceptDrops(true);
}

bool same_sign(int a, int b) {
	return (a < 0) == (b < 0);
}

void TimelineWidget::resizeEvent(QResizeEvent*) {
    if (sequence != NULL) redraw_clips();
}

void TimelineWidget::dragEnterEvent(QDragEnterEvent *event) {
	if (static_cast<SourceTable*>(event->source()) == panel_project->source_table) {
		QPoint pos = event->pos();
		event->accept();
		QList<QTreeWidgetItem*> items = panel_project->source_table->selectedItems();
        long entry_point = panel_timeline->getFrameFromScreenPoint(pos.x());
        panel_timeline->drag_frame_start = entry_point + panel_timeline->getFrameFromScreenPoint(50);
		panel_timeline->drag_track_start = (bottom_align) ? -1 : 0;
		for (int i=0;i<items.size();i++) {
			bool ignore_infinite_length = false;
            Media* m = panel_project->get_media_from_tree(items.at(i));
            long duration = m->get_length_in_frames(sequence->frame_rate);
			Ghost g = {NULL, entry_point, entry_point + duration};
			g.media = m;
			g.clip_in = 0;
			for (int j=0;j<m->audio_tracks.size();j++) {
				g.track = j;
                g.media_stream = m->audio_tracks.at(j);
				ignore_infinite_length = true;
				panel_timeline->ghosts.append(g);
			}
			for (int j=0;j<m->video_tracks.size();j++) {
				g.track = -1-j;
                g.media_stream = m->video_tracks.at(j);
                if (m->video_tracks[j]->infinite_length && !ignore_infinite_length) g.out = g.in + 100;
				panel_timeline->ghosts.append(g);
			}
			entry_point += duration;
		}
		init_ghosts();
		panel_timeline->importing = true;
	}
}

void TimelineWidget::dragMoveEvent(QDragMoveEvent *event) {
	if (panel_timeline->importing) {
		QPoint pos = event->pos();
		update_ghosts(pos);
		panel_timeline->repaint_timeline();
	}
}

void TimelineWidget::dragLeaveEvent(QDragLeaveEvent*) {
	if (panel_timeline->importing) {
		panel_timeline->ghosts.clear();
		panel_timeline->importing = false;
		panel_timeline->repaint_timeline();
	}
}

void TimelineWidget::dropEvent(QDropEvent* event) {
	if (panel_timeline->importing) {
		event->accept();

        QVector<Clip*> added_clips;

        // delete areas before adding
        QVector<Selection> delete_areas;
        for (int i=0;i<panel_timeline->ghosts.size();i++) {
            const Ghost& g = panel_timeline->ghosts.at(i);
            delete_areas.append({g.in, g.out, g.track});
        }
        panel_timeline->delete_areas_and_relink(delete_areas);

        // add clips
		for (int i=0;i<panel_timeline->ghosts.size();i++) {
            const Ghost& g = panel_timeline->ghosts.at(i);

            Clip* c = new Clip();
            c->media = g.media;
            c->media_stream = g.media_stream;
            c->timeline_in = g.in;
            c->timeline_out = g.out;
            c->clip_in = g.clip_in;
            c->color_r = 128;
            c->color_g = 128;
            c->color_b = 192;
            c->sequence = sequence;
            c->track = g.track;
            c->name = c->media->name;

            if (c->track < 0) {
				// add default video effects
                c->effects.append(create_effect(VIDEO_TRANSFORM_EFFECT, c));
			} else {
				// add default audio effects
                c->effects.append(create_effect(AUDIO_VOLUME_EFFECT, c));
                c->effects.append(create_effect(AUDIO_PAN_EFFECT, c));
            }

            sequence->add_clip(c);
            added_clips.append(c);
		}

        // link clips from the same media
        for (int i=0;i<added_clips.size();i++) {
            Clip* c = added_clips.at(i);
            for (int j=0;j<added_clips.size();j++) {
                Clip* cc = added_clips.at(j);
                if (c != cc && c->media == cc->media) {
                    c->linked.append(cc);
                }
            }
        }

		panel_timeline->ghosts.clear();
		panel_timeline->importing = false;
        panel_timeline->snapped = false;

		panel_timeline->redraw_all_clips();
	}
}

void TimelineWidget::mouseDoubleClickEvent(QMouseEvent *event) {

}

void TimelineWidget::mousePressEvent(QMouseEvent *event) {
    if (panel_timeline->tool == TIMELINE_TOOL_EDIT || panel_timeline->tool == TIMELINE_TOOL_RAZOR) {
        panel_timeline->drag_frame_start = panel_timeline->cursor_frame;
        panel_timeline->drag_track_start = panel_timeline->cursor_track;
    } else {
        QPoint pos = event->pos();
        panel_timeline->drag_frame_start = panel_timeline->getFrameFromScreenPoint(pos.x());
        panel_timeline->drag_track_start = getTrackFromScreenPoint(pos.y());
    }

	int clip_index = panel_timeline->trim_target;
	if (clip_index == -1) clip_index = getClipIndexFromCoords(panel_timeline->drag_frame_start, panel_timeline->drag_track_start);

	switch (panel_timeline->tool) {
	case TIMELINE_TOOL_POINTER:
	case TIMELINE_TOOL_RIPPLE:
    case TIMELINE_TOOL_SLIP:
	{
		if (clip_index >= 0) {
            if (panel_timeline->is_clip_selected(sequence->get_clip(clip_index))) {
				// TODO if shift is down, deselect it
			} else {
				// if "shift" is not down
				if (!(event->modifiers() & Qt::ShiftModifier)) {
					panel_timeline->selections.clear();
				}

                Clip* clip = sequence->get_clip(clip_index);
                panel_timeline->selections.append({clip->timeline_in, clip->timeline_out, clip->track});

                // if alt is not down, select links
                if (!(event->modifiers() & Qt::AltModifier)) {
                    for (int i=0;i<clip->linked.size();i++) {
                        Clip* link = clip->linked.at(i);
                        if (!panel_timeline->is_clip_selected(link)) {
                            panel_timeline->selections.append({link->timeline_in, link->timeline_out, link->track});
                        }
                    }
                }
			}
			panel_timeline->moving_init = true;
		} else {
			panel_timeline->selections.clear();
		}
		panel_timeline->repaint_timeline();
	}
		break;
	case TIMELINE_TOOL_EDIT:
		panel_timeline->seek(panel_timeline->drag_frame_start);
        panel_timeline->selecting = true;
		break;
	case TIMELINE_TOOL_RAZOR:
	{
		if (clip_index >= 0) {
            panel_timeline->split_clip_and_relink(sequence->get_clip(clip_index), panel_timeline->drag_frame_start, !(event->modifiers() & Qt::AltModifier));
		}
		panel_timeline->splitting = true;
		panel_timeline->redraw_all_clips();
	}
		break;
	}
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent *event) {
	if (panel_timeline->moving_proc) {
		if (event->modifiers() & Qt::AltModifier) { // if holding alt, duplicate rather than move
			// duplicate clips
            QVector<Clip*> copy_clips;
            QVector<Selection> delete_areas;
			for (int i=0;i<panel_timeline->ghosts.size();i++) {
                const Ghost& g = panel_timeline->ghosts.at(i);

                Clip* c = g.clip->copy();

                c->timeline_in = g.in;
                c->timeline_out = g.out;
                c->track = g.track;

                copy_clips.append(c);
			}
            panel_timeline->delete_areas_and_relink(delete_areas);
            for (int i=0;i<copy_clips.size();i++) {
                // step 2 - delete anything that exists in area that clip is moving to
                sequence->add_clip(copy_clips.at(i));
            }
		} else {
			// move clips
			// TODO can we do this better than 3 consecutive for loops?
			for (int i=0;i<panel_timeline->ghosts.size();i++) {
				// step 1 - set clips that are moving to "undeletable" (to avoid step 2 deleting any part of them)
				panel_timeline->ghosts[i].clip->undeletable = true;
			}
            // step 2 - delete areas
            QVector<Selection> delete_areas;
            for (int i=0;i<panel_timeline->ghosts.size();i++) {
                const Ghost& g = panel_timeline->ghosts.at(i);

                if (panel_timeline->tool == TIMELINE_TOOL_POINTER) {
                    // step 2 - delete anything that exists in area that clip is moving to
                    // note: ripples are non-destructive so this is pointer-tool exclusive
                    delete_areas.append({g.in, g.out, g.track});
                }
            }
            panel_timeline->delete_areas_and_relink(delete_areas);
			for (int i=0;i<panel_timeline->ghosts.size();i++) {
                Ghost& g = panel_timeline->ghosts[i];

				// step 3 - move clips
				g.clip->timeline_in = g.in;
				g.clip->timeline_out = g.out;
				g.clip->track = g.track;
				g.clip->clip_in = g.clip_in;
			}
			for (int i=0;i<panel_timeline->ghosts.size();i++) {
				// step 4 - set clips back to deletable
				panel_timeline->ghosts[i].clip->undeletable = false;
			}
		}

		// ripple ops
		if (panel_timeline->tool == TIMELINE_TOOL_RIPPLE) {
			long ripple_length, ripple_point;
			if (panel_timeline->trim_in) {
				ripple_length = panel_timeline->ghosts.at(0).old_in - panel_timeline->ghosts.at(0).in;
				ripple_point = (ripple_length < 0) ? panel_timeline->ghosts.at(0).old_in : panel_timeline->ghosts.at(0).in;
			} else {
				ripple_length = panel_timeline->ghosts.at(0).old_out - panel_timeline->ghosts.at(0).out;
				ripple_point = (ripple_length < 0) ? panel_timeline->ghosts.at(0).old_out : panel_timeline->ghosts.at(0).out;
			}
			for (int i=0;i<panel_timeline->ghosts.size();i++) {
				long comp_point;
				if (panel_timeline->trim_in) {
					comp_point = (ripple_length < 0) ? panel_timeline->ghosts.at(i).old_in : panel_timeline->ghosts.at(i).in;
				} else {
					comp_point = (ripple_length < 0) ? panel_timeline->ghosts.at(i).old_out : panel_timeline->ghosts.at(i).out;
				}
				if (ripple_point > comp_point) ripple_point = comp_point;
			}
			if (panel_timeline->trim_in) {
				panel_timeline->ripple(ripple_point, ripple_length);
			} else {
				panel_timeline->ripple(ripple_point, -ripple_length);
			}
		}

		panel_timeline->redraw_all_clips();
	}

	// destroy all ghosts
	panel_timeline->ghosts.clear();

	panel_timeline->selecting = false;
	panel_timeline->moving_proc = false;
	panel_timeline->moving_init = false;
	panel_timeline->splitting = false;
    panel_timeline->snapped = false;
	pre_clips.clear();
	post_clips.clear();

	// find out how many clips are selected
	bool single_select = false;
	int selected_clip = 0;
    for (int i=0;i<sequence->clip_count();i++) {
        if (panel_timeline->is_clip_selected(sequence->get_clip(i))) {
			if (!single_select) {
				// found ONE selected clip
				selected_clip = i;
				single_select = true;
			} else {
				// more than one clip is selected
				single_select = false;
				break;
			}
		}
	}
	if (single_select) {
        panel_effect_controls->set_clip(sequence->get_clip(selected_clip));
	} else {
		panel_effect_controls->set_clip(NULL);
	}
}

void TimelineWidget::init_ghosts() {
	for (int i=0;i<panel_timeline->ghosts.size();i++) {
		Ghost& g = panel_timeline->ghosts[i];
		g.old_in = g.in;
		g.old_out = g.out;
		g.old_track = g.track;
		g.old_clip_in = g.clip_in;

        if (panel_timeline->trim_target > -1 || panel_timeline->tool == TIMELINE_TOOL_SLIP) {
			// used for trim ops
			g.ghost_length = g.old_out - g.old_in;
            g.media_length = g.clip->media->get_length_in_frames(sequence->frame_rate);
		}
	}
	for (int i=0;i<panel_timeline->selections.size();i++) {
		Selection& s = panel_timeline->selections[i];
		s.old_in = s.in;
		s.old_out = s.out;
		s.old_track = s.track;
	}
}

bool subvalidate_snapping(Ghost& g, long* frame_diff, long snap_point) {
    int snap_range = 15;
    long in_validator = g.old_in + *frame_diff - snap_point;
    long out_validator = g.old_out + *frame_diff - snap_point;

    if ((panel_timeline->trim_target == -1 || panel_timeline->trim_in) && in_validator > -snap_range && in_validator < snap_range) {
        *frame_diff -= in_validator;
        panel_timeline->snap_point = snap_point;
        panel_timeline->snapped = true;
        return true;
    } else if (!panel_timeline->trim_in && out_validator > -snap_range && out_validator < snap_range) {
        *frame_diff -= out_validator;
        panel_timeline->snap_point = snap_point;
        panel_timeline->snapped = true;
        return true;
    }
    return false;
}

void validate_snapping(Ghost& g, long* frame_diff) {
    panel_timeline->snapped = false;
    if (panel_timeline->snapping) {
        if (!subvalidate_snapping(g, frame_diff, panel_timeline->playhead)) {
            for (int j=0;j<sequence->clip_count();j++) {
                Clip* c = sequence->get_clip(j);
                if (!subvalidate_snapping(g, frame_diff, c->timeline_in)) {
                    subvalidate_snapping(g, frame_diff, c->timeline_out);
                }
                if (panel_timeline->snapped) break;
            }
        }
    }
}

void TimelineWidget::update_ghosts(QPoint& mouse_pos) {
	int mouse_track = getTrackFromScreenPoint(mouse_pos.y());
    long frame_diff = panel_timeline->getFrameFromScreenPoint(mouse_pos.x()) - panel_timeline->drag_frame_start;
	int track_diff = mouse_track - panel_timeline->drag_track_start;

	long validator;
    if (panel_timeline->trim_target > -1) { // if trimming
		// trim ops

		// validate ghosts
		for (int i=0;i<panel_timeline->ghosts.size();i++) {
			Ghost& g = panel_timeline->ghosts[i];

			if (panel_timeline->trim_in) {
				// prevent clip length from being less than 1 frame long
				validator = g.ghost_length - frame_diff;
				if (validator < 1) frame_diff -= (1 - validator);

				// prevent timeline in from going below 0
				validator = g.old_in + frame_diff;
				if (validator < 0) frame_diff -= validator;

				if (!g.clip->media_stream->infinite_length) {
					// prevent clip_in from going below 0
					validator = g.old_clip_in + frame_diff;
					if (validator < 0) frame_diff -= validator;
				}

				// ripple ops
				if (panel_timeline->tool == TIMELINE_TOOL_RIPPLE) {
					for (int j=0;j<post_clips.size();j++) {
						// prevent any rippled clip from going below 0
						Clip* post = post_clips.at(j);
						validator = post->timeline_in - frame_diff;
						if (validator < 0) frame_diff += validator;

						// prevent any post-clips colliding with pre-clips
						for (int k=0;k<pre_clips.size();k++) {
							Clip* pre = pre_clips.at(k);
							if (pre->track == post->track) {
								validator = post->timeline_in - frame_diff - pre->timeline_out;
								if (validator < 0) frame_diff += validator;
							}
						}
					}
                }
			} else {
				// prevent clip length from being less than 1 frame long
				validator = g.ghost_length + frame_diff;
				if (validator < 1) frame_diff += (1 - validator);

				if (!g.clip->media_stream->infinite_length) {
					// prevent clip length exceeding media length
					validator = g.ghost_length + frame_diff;
					if (validator > g.media_length) frame_diff -= validator - g.media_length;
				}

				// ripple ops
				if (panel_timeline->tool == TIMELINE_TOOL_RIPPLE) {
					for (int j=0;j<post_clips.size();j++) {
						Clip* post = post_clips.at(j);

						// prevent any post-clips colliding with pre-clips
						for (int k=0;k<pre_clips.size();k++) {
							Clip* pre = pre_clips.at(k);
							if (pre->track == post->track) {
								validator = post->timeline_in + frame_diff - pre->timeline_out;
								if (validator < 0) frame_diff -= validator;
							}
						}
					}
				}
			}

            validate_snapping(g, &frame_diff);
		}

		// resize ghosts
		for (int i=0;i<panel_timeline->ghosts.size();i++) {
			Ghost& g = panel_timeline->ghosts[i];

			if (panel_timeline->trim_in) {
				g.in = g.old_in + frame_diff;
				g.clip_in = g.old_clip_in + frame_diff;
			} else {
				g.out = g.old_out + frame_diff;
			}
		}

		// resize selections
		for (int i=0;i<panel_timeline->selections.size();i++) {
			Selection& s = panel_timeline->selections[i];

			if (panel_timeline->trim_in) {
				s.in = s.old_in + frame_diff;
			} else {
				s.out = s.old_out + frame_diff;
			}
		}
	} else if (panel_timeline->tool == TIMELINE_TOOL_POINTER || panel_timeline->importing) { // only move clips on pointer (not ripple or rolling)
		// validate ghosts
		for (int i=0;i<panel_timeline->ghosts.size();i++) {
			Ghost& g = panel_timeline->ghosts[i];

			// prevent clips from moving below 0 on the timeline
			validator = g.old_in + frame_diff;
			if (validator < 0) frame_diff -= validator;

			// prevent clips from crossing tracks
			if (same_sign(g.old_track, panel_timeline->drag_track_start)) {
				while (!same_sign(g.old_track, g.old_track + track_diff)) {
					if (g.old_track < 0) {
						track_diff--;
					} else {
						track_diff++;
					}
				}
			}

            validate_snapping(g, &frame_diff);
        }

		// move ghosts
		for (int i=0;i<panel_timeline->ghosts.size();i++) {
			Ghost& g = panel_timeline->ghosts[i];
			g.in = g.old_in + frame_diff;
			g.out = g.old_out + frame_diff;

			g.track = g.old_track;

			if (panel_timeline->importing) {
				int abs_track_diff = abs(track_diff);
				if (g.old_track < 0) { // clip is video
					g.track -= abs_track_diff;
				} else { // clip is audio
					g.track += abs_track_diff;
				}
			} else {
				if (same_sign(g.old_track, panel_timeline->drag_track_start)) g.track += track_diff;
			}
		}

		// move selections
		if (!panel_timeline->importing) {
			for (int i=0;i<panel_timeline->selections.size();i++) {
				Selection& s = panel_timeline->selections[i];
				s.in = s.old_in + frame_diff;
				s.out = s.old_out + frame_diff;
				s.track = s.old_track;
				if (panel_timeline->importing) {
					int abs_track_diff = abs(track_diff);
					if (s.old_track < 0) {
						s.track -= abs_track_diff;
					} else {
						s.track += abs_track_diff;
					}
				} else {
					if (same_sign(s.track, panel_timeline->drag_track_start)) s.track += track_diff;
				}
			}
		}
    } else if (panel_timeline->tool == TIMELINE_TOOL_SLIP) {
        // validate ghosts
        for (int i=0;i<panel_timeline->ghosts.size();i++) {
            Ghost& g = panel_timeline->ghosts[i];
            if (!g.clip->media_stream->infinite_length) {
                // prevent slip moving a clip below 0 clip_in
                validator = g.old_clip_in - frame_diff;
                if (validator < 0) frame_diff += validator;

                // prevent slip moving clip beyond media length
                validator += g.ghost_length;
                qDebug() << "validator:" << validator;
                if (validator > g.media_length) frame_diff += validator - g.media_length;
            }
        }

        // slip ghosts
        for (int i=0;i<panel_timeline->ghosts.size();i++) {
            Ghost& g = panel_timeline->ghosts[i];

            g.clip_in = g.old_clip_in - frame_diff;
        }
    }
}

void TimelineWidget::mouseMoveEvent(QMouseEvent *event) {
    panel_timeline->cursor_frame = panel_timeline->getFrameFromScreenPoint(event->pos().x());
    panel_timeline->cursor_track = getTrackFromScreenPoint(event->pos().y());

    if (panel_timeline->tool == TIMELINE_TOOL_EDIT || panel_timeline->tool == TIMELINE_TOOL_RAZOR) {
        int limit = panel_timeline->getFrameFromScreenPoint(10);
        if (panel_timeline->snapping) {
            if (!panel_timeline->selecting &&
                    panel_timeline->cursor_frame > panel_timeline->playhead-limit-1 &&
                    panel_timeline->cursor_frame < panel_timeline->playhead+limit+1) {
                panel_timeline->cursor_frame = panel_timeline->playhead;
                panel_timeline->snapped = true;
                panel_timeline->snap_point = panel_timeline->playhead;
            } else {
                panel_timeline->snap_to_clip(&panel_timeline->cursor_frame);
            }
        }
    }
    if (panel_timeline->selecting) {
        int selection_count = 1 + qMax(panel_timeline->cursor_track, panel_timeline->drag_track_start) - qMin(panel_timeline->cursor_track, panel_timeline->drag_track_start);
		if (panel_timeline->selections.size() != selection_count) {
			panel_timeline->selections.resize(selection_count);
		}
        int minimum_selection_track = qMin(panel_timeline->cursor_track, panel_timeline->drag_track_start);
		for (int i=0;i<selection_count;i++) {
			Selection* s = &panel_timeline->selections[i];
			s->track = minimum_selection_track + i;
			long in = panel_timeline->drag_frame_start;
            long out = panel_timeline->cursor_frame;
			s->in = qMin(in, out);
			s->out = qMax(in, out);
		}
        panel_timeline->seek(qMin(panel_timeline->drag_frame_start, panel_timeline->cursor_frame));
    } else if (panel_timeline->moving_init) {
		if (panel_timeline->moving_proc) {
            QPoint pos = event->pos();
            update_ghosts(pos);
		} else {
			// set up movement
			// create ghosts
            for (int i=0;i<sequence->clip_count();i++) {
                Clip* c = sequence->get_clip(i);
                if (panel_timeline->is_clip_selected(c)) {
                    panel_timeline->ghosts.append({c, c->timeline_in, c->timeline_out, c->track, c->clip_in});
				}
			}

			// ripple edit prep
			if (panel_timeline->tool == TIMELINE_TOOL_RIPPLE) {
				for (int i=0;i<panel_timeline->ghosts.size();i++) {
					// get clips before and after ripple point
                    for (int j=0;j<sequence->clip_count();j++) {
						// don't cache any currently selected clips
						Clip* c = panel_timeline->ghosts.at(i).clip;
                        Clip* cc = sequence->get_clip(j);
						bool is_selected = false;
						for (int k=0;k<panel_timeline->ghosts.size();k++) {
                            if (panel_timeline->ghosts.at(k).clip == cc) {
								is_selected = true;
								break;
							}
						}

						if (!is_selected) {
                            if (cc->timeline_in < c->timeline_in) {
								// add clip to pre-cache UNLESS there is already a clip on that track closer to the ripple point
								bool found = false;
								for (int k=0;k<pre_clips.size();k++) {
									Clip* ccc = pre_clips.at(k);
                                    if (ccc->track == cc->track) {
                                        if (ccc->timeline_in < cc->timeline_in) {
											// clip is closer to ripple point than the one in cache, replace it
                                            ccc = cc;
										}
										found = true;
									}
								}
								if (!found) {
									// no clip from that track in the cache, add it
                                    pre_clips.append(cc);
								}
							} else {
								// add clip to post-cache UNLESS there is already a clip on that track closer to the ripple point
								bool found = false;
								for (int k=0;k<post_clips.size();k++) {
									Clip* ccc = post_clips.at(k);
                                    if (ccc->track == cc->track) {
                                        if (ccc->timeline_in > cc->timeline_in) {
											// clip is closer to ripple point than the one in cache, replace it
                                            ccc = cc;
										}
										found = true;
									}
								}
								if (!found) {
									// no clip from that track in the cache, add it
                                    post_clips.append(cc);
								}
							}
						}
					}
				}

				// debug code - print the information we got
//				qDebug() << "found" << pre_clips.size() << "preceding clips and" << post_clips.size() << "following";
			}

			init_ghosts();

			panel_timeline->moving_proc = true;
		}
		panel_timeline->repaint_timeline();
    } else if (panel_timeline->splitting) {
        int track = panel_timeline->cursor_track;
		bool repaint = false;
        for (int i=0;i<sequence->clip_count();i++) {
            Clip* clip = sequence->get_clip(i);
            if (clip->track == track) {
                panel_timeline->split_clip_and_relink(clip, panel_timeline->drag_frame_start, !(event->modifiers() & Qt::AltModifier));
				repaint = true;
			}
		}

		// redraw clips since we changed them
		if (repaint) panel_timeline->redraw_all_clips();
	} else if (panel_timeline->tool == TIMELINE_TOOL_POINTER || panel_timeline->tool == TIMELINE_TOOL_RIPPLE) {
		QPoint pos = event->pos();

		int lim = 5;
        int mouse_track = getTrackFromScreenPoint(pos.y());
        long mouse_frame_lower = panel_timeline->getFrameFromScreenPoint(pos.x()-lim)-1;
        long mouse_frame_upper = panel_timeline->getFrameFromScreenPoint(pos.x()+lim)+1;
        bool found = false;
        int closeness = INT_MAX;
        for (int i=0;i<sequence->clip_count();i++) {
            Clip* c = sequence->get_clip(i);
            if (c->track == mouse_track) {
                if (c->timeline_in > mouse_frame_lower && c->timeline_in < mouse_frame_upper) {
                    int nc = abs(c->timeline_in + 1 - panel_timeline->cursor_frame);
                    if (nc < closeness) {
                        panel_timeline->trim_target = i;
                        panel_timeline->trim_in = true;
                        closeness = nc;
                        found = true;
                    }
                }
                if (c->timeline_out > mouse_frame_lower && c->timeline_out < mouse_frame_upper) {
                    int nc = abs(c->timeline_out - 1 - panel_timeline->cursor_frame);
                    if (nc < closeness) {
                        panel_timeline->trim_target = i;
                        panel_timeline->trim_in = false;
                        closeness = nc;
                        found = true;
                    }
				}
			}
        }
		if (found) {
			setCursor(Qt::SizeHorCursor);
		} else {
			unsetCursor();
			panel_timeline->trim_target = -1;
		}
	} else if (panel_timeline->tool == TIMELINE_TOOL_EDIT || panel_timeline->tool == TIMELINE_TOOL_RAZOR) {
		// redraw because we have a cursor
		panel_timeline->repaint_timeline();
    } else if (panel_timeline->tool == TIMELINE_TOOL_SLIP) {
        if (getClipIndexFromCoords(panel_timeline->cursor_frame, panel_timeline->cursor_track) > -1) {
            setCursor(Qt::SizeHorCursor);
        } else {
            unsetCursor();
        }
    }
}

int color_brightness(int r, int g, int b) {
	return (0.2126*r + 0.7152*g + 0.0722*b);
}

void TimelineWidget::redraw_clips() {
    // Draw clips
    int panel_width = panel_timeline->getScreenPointFromFrame(sequence->getEndFrame()) + 100;

    if (minimumWidth() != panel_width) {
        setMinimumWidth(panel_width);
        clip_pixmap = QPixmap(panel_width, height());
    }

    clip_pixmap.fill(Qt::transparent);
    QPainter clip_painter(&clip_pixmap);
	int video_track_limit = 0;
	int audio_track_limit = 0;
    for (int i=0;i<sequence->clip_count();i++) {
        Clip* clip = sequence->get_clip(i);
        if (is_track_visible(clip->track)) {
            if (clip->track < 0 && clip->track < video_track_limit) { // video clip
                video_track_limit = clip->track;
            } else if (clip->track > audio_track_limit) {
                audio_track_limit = clip->track;
            }

            QRect clip_rect(panel_timeline->getScreenPointFromFrame(clip->timeline_in), getScreenPointFromTrack(clip->track), clip->getLength() * panel_timeline->zoom, track_height);
            clip_painter.fillRect(clip_rect, QColor(clip->color_r, clip->color_g, clip->color_b));

            // draw thumbnail/waveform
            if (clip->media_stream->preview_done) {
                if (clip->track < 0) {
                    int thumb_y = clip_painter.fontMetrics().height()+CLIP_TEXT_PADDING+CLIP_TEXT_PADDING;
                    int thumb_height = clip_rect.height()-thumb_y;
                    int thumb_width = (thumb_height*((float)clip->media_stream->video_preview.width()/(float)clip->media_stream->video_preview.height()));
                    if (thumb_height > thumb_y && clip_rect.width() > thumb_width) { // at small clip heights, don't even draw it
                        QRect thumb_rect(clip_rect.x(), clip_rect.y()+thumb_y, thumb_width, thumb_height);
                        clip_painter.drawImage(thumb_rect, clip->media_stream->video_preview);
                    }
                } else {
                    long length = clip->media->get_length_in_frames(clip->sequence->frame_rate);
                    int waveform_x = ((float)clip->clip_in/(float)length) * clip->media_stream->audio_preview.size();
                    int waveform_width = (((float)clip->getLength()/(float)length) * clip->media_stream->audio_preview.size());
                    int divider = clip->media_stream->audio_channels*2;
                    int waveform_scaled_length = qFloor((clip->media_stream->audio_preview.size() / (length*panel_timeline->zoom))/divider)*divider;

                    clip_painter.setPen(QColor(80, 80, 80));
                    int draw_x = clip_rect.x();
                    int channel_height = clip_rect.height()/clip->media_stream->audio_channels;
                    for (int i=0;i<clip_rect.width();i++) {
                        int waveform_index = qFloor((((float)i / (float)clip_rect.width()) * clip->media_stream->audio_preview.size())/divider)*divider;
                        for (int j=0;j<clip->media_stream->audio_channels;j++) {
                            int mid = clip_rect.top()+channel_height*j+(channel_height/2);
                            int offset = waveform_index+(j*2);
                            qint8 min = (float)clip->media_stream->audio_preview[offset] / 128.0f * (channel_height/2);
                            qint8 max = (float)clip->media_stream->audio_preview[offset+1] / 128.0f * (channel_height/2);
                            clip_painter.drawLine(clip_rect.left()+i, mid+min, clip_rect.left()+i, mid+max);
                        }
                    }
                }
            }

            // top left bevel
            clip_painter.setPen(Qt::white);
            clip_painter.drawLine(clip_rect.bottomLeft(), clip_rect.topLeft());
            clip_painter.drawLine(clip_rect.topLeft(), clip_rect.topRight());

            // draw text
            if (color_brightness(clip->color_r, clip->color_g, clip->color_b) > 160) {
                // set to black if color is bright
                clip_painter.setPen(Qt::black);
            }
            QRect text_rect(clip_rect.left() + CLIP_TEXT_PADDING, clip_rect.top() + CLIP_TEXT_PADDING, clip_rect.width() - CLIP_TEXT_PADDING - CLIP_TEXT_PADDING, clip_rect.height() - CLIP_TEXT_PADDING - CLIP_TEXT_PADDING);
            clip_painter.drawText(text_rect, 0, clip->name, &text_rect);
            if (clip->linked.size() > 0) {
                int underline_y = CLIP_TEXT_PADDING + clip_painter.fontMetrics().height() + clip_rect.top();
                int underline_width = qMin(clip_rect.width() - CLIP_TEXT_PADDING, clip_rect.left() + CLIP_TEXT_PADDING+clip_painter.fontMetrics().width(clip->name));
                clip_painter.drawLine(clip_rect.left() + CLIP_TEXT_PADDING, underline_y, underline_width, underline_y);
            }

            // bottom right gray
            clip_painter.setPen(Qt::gray);
            clip_painter.drawLine(clip_rect.bottomLeft(), clip_rect.bottomRight());
            clip_painter.drawLine(clip_rect.bottomRight(), clip_rect.topRight());
        }
    }

	// Draw track lines
	if (show_track_lines) {
		clip_painter.setPen(QColor(0, 0, 0, 96));
		audio_track_limit++;
		if (video_track_limit == 0) video_track_limit--;

		if (bottom_align) {
			// only draw lines for video tracks
			for (int i=video_track_limit;i<0;i++) {
				int line_y = getScreenPointFromTrack(i) - 1;
				clip_painter.drawLine(0, line_y, rect().width(), line_y);
			}
		} else {
			// only draw lines for audio tracks
			for (int i=0;i<audio_track_limit;i++) {
				int line_y = getScreenPointFromTrack(i) + track_height;
				clip_painter.drawLine(0, line_y, rect().width(), line_y);
			}
		}
	}

	update();
}

void TimelineWidget::paintEvent(QPaintEvent*) {
    if (sequence != NULL) {
		QPainter p(this);

        p.drawPixmap(0, 0, minimumWidth(), height(), clip_pixmap);

		// Draw selections
		for (int i=0;i<panel_timeline->selections.size();i++) {
			const Selection& s = panel_timeline->selections.at(i);
			if (is_track_visible(s.track)) {
				int selection_y = getScreenPointFromTrack(s.track);
                int selection_x = panel_timeline->getScreenPointFromFrame(s.in);
                p.fillRect(selection_x, selection_y, panel_timeline->getScreenPointFromFrame(s.out) - selection_x, track_height, QColor(0, 0, 0, 64));
			}
		}

		// Draw ghosts
		for (int i=0;i<panel_timeline->ghosts.size();i++) {
			const Ghost& g = panel_timeline->ghosts.at(i);
			if (is_track_visible(g.track)) {
                int ghost_x = panel_timeline->getScreenPointFromFrame(g.in);
				int ghost_y = getScreenPointFromTrack(g.track);
                int ghost_width = panel_timeline->getScreenPointFromFrame(g.out - g.in) - 1;
				int ghost_height = track_height - 1;
				p.setPen(QColor(255, 255, 0));
				for (int j=0;j<GHOST_THICKNESS;j++) {
					p.drawRect(ghost_x+j, ghost_y+j, ghost_width-j-j, ghost_height-j-j);
				}
			}
		}

        // Draw playhead
        p.setPen(Qt::red);
        int playhead_x = panel_timeline->getScreenPointFromFrame(panel_timeline->playhead);
        p.drawLine(playhead_x, rect().top(), playhead_x, rect().bottom());

        p.setPen(QColor(0, 0, 0, 64));
        int edge_y = (bottom_align) ? rect().height()-1 : 0;
        p.drawLine(0, edge_y, rect().width(), edge_y);

        // draw snap point
        if (panel_timeline->snapping && panel_timeline->snapped) {
            p.setPen(Qt::white);
            int snap_x = panel_timeline->getScreenPointFromFrame(panel_timeline->snap_point);
            p.drawLine(snap_x, 0, snap_x, height());
        }

		// Draw edit cursor
		if (panel_timeline->tool == TIMELINE_TOOL_EDIT || panel_timeline->tool == TIMELINE_TOOL_RAZOR) {
            if (is_track_visible(panel_timeline->cursor_track)) {
                int cursor_x = panel_timeline->getScreenPointFromFrame(panel_timeline->cursor_frame);
                int cursor_y = getScreenPointFromTrack(panel_timeline->cursor_track);

				p.setPen(Qt::gray);
				p.drawLine(cursor_x, cursor_y, cursor_x, cursor_y + track_height);
			}
        }
	}
}

bool TimelineWidget::is_track_visible(int track) {
	return ((bottom_align && track < 0) || (!bottom_align && track >= 0));
}

// **************************************
// screen point <-> frame/track functions
// **************************************

int TimelineWidget::getTrackFromScreenPoint(int y) {	
	if (bottom_align) {
		y -= rect().bottom();
		if (show_track_lines) y -= 1;
	} else {
		y -= 1;
	}
	int temp_track_height = track_height;
	if (show_track_lines) temp_track_height--;
    return (int)qFloor((float) (y)/ (float) temp_track_height);
}

int TimelineWidget::getScreenPointFromTrack(int track) {
	int temp_track_height = track_height;
	if (show_track_lines) temp_track_height++;
	int y = track * temp_track_height;

	if (bottom_align) { // video track
		y += rect().bottom();
		if (show_track_lines) y += 1;
	} else { // audio track
		y += 1;
	}
	return y;
}

int TimelineWidget::getClipIndexFromCoords(long frame, int track) {
    for (int i=0;i<sequence->clip_count();i++) {
        Clip* c = sequence->get_clip(i);
        if (c->track == track) {
            if (frame >= c->timeline_in && frame < c->timeline_out) {
				return i;
			}
		}
	}
	return -1;
}
